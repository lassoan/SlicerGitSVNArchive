/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkImageBimodalAnalysis.cxx,v $
  Date:      $Date: 2006/06/14 20:44:14 $
  Version:   $Revision: 1.21 $

=========================================================================auto=*/
#include "vtkImageBimodalAnalysis.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkImageData.h"

//#include <math.h>
//#include <cstdlib>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkImageBimodalAnalysis);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageBimodalAnalysis::vtkImageBimodalAnalysis()
{
  this->Modality  = VTK_BIMODAL_MODALITY_CT;
  this->Threshold = 0.0;
  this->Window    = 0.0;
  this->Level     = 0.0;
  this->Min       = 0.0;
  this->Max       = 0.0;
  this->Offset    = 0.0;
  this->SignalRange[0] = 0.0;
  this->SignalRange[1] = 0.0;
  for (int i = 0; i < 6; ++i)
    {
    this->ClipExtent[i] = 0;
    }
}

//----------------------------------------------------------------------------
int vtkImageBimodalAnalysis::RequestInformation(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageBimodalAnalysisExecute(vtkImageBimodalAnalysis *self,
                      vtkImageData *inData, T *inPtr,
                      vtkImageData *outData, float *outPtr)
{
  // Process x dimension only
  int numberOfValues = outData->GetExtent()[1] - outData->GetExtent()[0] + 1;

  // Zero output
  std::fill(outPtr, outPtr+numberOfValues, 0.0);

  int startIndex = 0;

  // For CT data, make sure we ignore -32768
  if (self->GetModality() == VTK_BIMODAL_MODALITY_CT)
    {
    if (numberOfValues>0)
      {
      startIndex++;
      }
    }

  // Find first non-zero value in histogram
  int firstNonZeroValueIndex = startIndex;
  for (int valueIndex = 0; valueIndex < numberOfValues; ++valueIndex)
    {
    if (inPtr[valueIndex] > 0)
      {
      firstNonZeroValueIndex = valueIndex;
      break;
      }
    }
  // Find last non-zero value in histogram
  int lastNonZeroValueIndex = numberOfValues-1;
  for (int valueIndex = numberOfValues-1; valueIndex >= 0 ; --valueIndex)
  {
    if (inPtr[valueIndex] > 0)
    {
      lastNonZeroValueIndex = valueIndex;
      break;
    }
  }

  // Smooth histogram with moving average filter
  int filterRadius = 2;
  float filterWeightFactor = 1.0 / static_cast<float>(filterRadius * 2 + 1);
  for (int valueIndex = firstNonZeroValueIndex; valueIndex <= lastNonZeroValueIndex; valueIndex++)
    {
    for (int k=-filterRadius; k <= filterRadius; k++)
      {
      if (valueIndex+k > firstNonZeroValueIndex && valueIndex+k < numberOfValues) // skip any that would be outside range of outPtr see Bug #3429
        {
        outPtr[valueIndex] += (float)inPtr[valueIndex+k];
        }
      }
    // Regardless of how many points we added, we always divide by the same number, which
    // means that we get lower the values at the near the two boundaries.
    // Since this will not impact further analysis steps, we leave it as is.
    outPtr[valueIndex] *= filterWeightFactor;
    }

  // Find first local minimum (trough) of smoothed histogram
  int firstLocalMinimumIndex = firstNonZeroValueIndex;
  bool valueIncreasing = true;
  for (int valueIndex = firstNonZeroValueIndex+1; valueIndex < lastNonZeroValueIndex; valueIndex++)
    {
    if (valueIncreasing)
      {
      if (outPtr[valueIndex] > outPtr[valueIndex + 1])
        {
        valueIncreasing = false;
        }
      }
    else
      {
      if (outPtr[valueIndex] < outPtr[valueIndex +1])
        {
        firstLocalMinimumIndex = valueIndex;
        break;
        }
      }
    }

  // Compute centroid of the histogram that PRECEDES the first local minimum
  // (noise lobe)
  int noiseCentroid = firstLocalMinimumIndex;
    {
    double indexWeightedSum = 0.0;
    double sum = 0.0;
    for (int valueIndex = firstNonZeroValueIndex; valueIndex <= firstLocalMinimumIndex; valueIndex++)
      {
      double value = static_cast<double>(inPtr[valueIndex]);
      indexWeightedSum += static_cast<double>(valueIndex) * value;
      sum += value;
      }
    if (sum > 0.0)
      {
      noiseCentroid = static_cast<int>(indexWeightedSum / sum);
      }
    }
  // Compute centroid of the histogram that FOLLOWS the first local minimum
  // (signal lobe, and not noise lobe) and the minimum and maximum value
  int signalCentroid = firstLocalMinimumIndex;
  double minSignal = inPtr[firstLocalMinimumIndex];
  double maxSignal = inPtr[firstLocalMinimumIndex];
    {
    double indexWeightedSum = 0.0;
    double sum = 0.0;
    for (int valueIndex = firstLocalMinimumIndex; valueIndex <= lastNonZeroValueIndex; valueIndex++)
      {
      double value = static_cast<double>(inPtr[valueIndex]);
      if (value > maxSignal)
        {
        maxSignal = value;
        }
      else if (value < minSignal)
        {
        minSignal = value;
        }
      indexWeightedSum += static_cast<double>(valueIndex) * value;
      sum += value;
      }
    if (sum > 0.0)
      {
      noiseCentroid = static_cast<int>(indexWeightedSum / sum);
      }
    }

  // Compute the window as twice the width as the smaller half
  // of the signal lobe
  double window = 0.0;
  if (signalCentroid - noiseCentroid < lastNonZeroValueIndex - signalCentroid)
    {
    window = (signalCentroid - noiseCentroid) * 2.0;
    }
  else
    {
    window = (lastNonZeroValueIndex - signalCentroid) * 2.0;
    }

  // Record findings
  double offset = inData->GetOrigin()[0];
  self->SetOffset(offset);
  self->SetThreshold(firstLocalMinimumIndex + offset);
  self->SetMin(firstNonZeroValueIndex + offset);
  self->SetMax(lastNonZeroValueIndex + offset);
  self->SetLevel(signalCentroid + offset);
  self->SetWindow(window);
  self->SetSignalRange((int)minSignal, (int)maxSignal);

  int clipExt[6] = { 0 };
  outData->GetExtent(clipExt);
  clipExt[0] = firstNonZeroValueIndex;
  clipExt[1] = lastNonZeroValueIndex;
  self->SetClipExtent(clipExt);
}



//----------------------------------------------------------------------------
// This method is passed a input and output Data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the Datas data types.
void vtkImageBimodalAnalysis::ExecuteDataWithInformation(vtkDataObject *out, vtkInformation* outInfo)
{
  vtkImageData *inData = vtkImageData::SafeDownCast(this->GetInput());
  vtkImageData *outData = this->AllocateOutputData(out, outInfo);
  void *inPtr  = inData->GetScalarPointer();
  float *outPtr = (float *)outData->GetScalarPointer();

  // Components turned into x, y and z
  int c = inData->GetNumberOfScalarComponents();
  if (c > 1)
    {
    vtkErrorMacro("This filter requires 1 scalar component, not " << c);
    return;
    }

  // this filter expects that output is type float.
  if (outData->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro(<< "ExecuteData: out ScalarType " << outData->GetScalarType()
      << " must be float\n");
    return;
    }

  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(vtkImageBimodalAnalysisExecute(this, inData,
                                                    static_cast<VTK_TT *>(inPtr),
                                                    outData, outPtr));
    default:
      vtkErrorMacro(<< "ExecuteData: Unsupported ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkImageBimodalAnalysis::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Modality: " << this->Modality << " (" << (this->Modality == VTK_BIMODAL_MODALITY_CT ? "CT" : "MR") << ")\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
  os << indent << "Window: " << this->Window << "\n";
  os << indent << "Level: " << this->Level << "\n";
  os << indent << "Min: " << this->Min << "\n";
  os << indent << "Max: " << this->Max << "\n";
  os << indent << "ClipExtent: " << this->ClipExtent[0] << "," << this->ClipExtent[1] << "," << this->ClipExtent[2] << "," << this->ClipExtent[3] << "," << this->ClipExtent[4] << "," << this->ClipExtent[5] << "\n";
  os << indent << "SignalRange: " << this->SignalRange[0] << "," << this->SignalRange[1] << "\n";

}

