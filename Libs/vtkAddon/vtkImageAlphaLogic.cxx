#include "vtkInformation.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkImageStencilData.h"
#include "vtkImageStencilIterator.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageAlphaLogic);

//----------------------------------------------------------------------------
vtkImageAlphaLogic::vtkImageAlphaLogic()
{
  this->BackgroundColor[0] = 1;
  this->BackgroundColor[1] = 1;
  this->BackgroundColor[2] = 1;
  this->SetNumberOfInputPorts(3);
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageAlphaLogicExecute(vtkImageAlphaLogic *self, vtkImageData *inData,
                                vtkImageData *outData, int outExt[6], int id, T *)
{
  int numScalarsImage = inData[0][0]->GetNumberOfScalarComponents();
  int numScalarsMask  = inData[1][0]->GetNumberOfScalarComponents();
  int maskAlphaComponent = numScalarsMask - 1;

  vtkImageStencilData *stencil = vtkImageStencilData::SafeDownCast(inData[2][0]);
  vtkImageStencilIterator<T> outIter(outData, stencil, outExt, self, id);
  vtkImageIterator<T> inImageIter(inData[0][0], outExt);
  vtkImageIterator<T> inMaskIter(inData[1][0], outExt);

  // if no background image is provided in inData2
  T *inImagePtr        = inImageIter.BeginSpan();
  T *inImageSpanEndPtr = inImageIter.EndSpan();
  T *inMaskPtr         = inMaskIter.BeginSpan();
  T *inMaskSpanEndPtr  = inMaskIter.EndSpan();
  while (!outIter.IsAtEnd())
    {
    T* outPtr = outIter.BeginSpan();
    T* outSpanEndPtr = outIter.EndSpan();
    if (outIter.IsInStencil())
      {
      while (outPtr != outSpanEndPtr)
        {
        if (inMaskPtr[maskAlphaComponent] != 0)
          {
          for (int i = 0; i < numScalarsImage; i++)
            {
            outPtr[i] = inImagePtr[i];
            }
          }
        outPtr     += numScalarsImage;
        inImagePtr += numScalarsImage;
        inMaskPtr  += numScalarsMask;
        }
      }
    else // !IsInStencil()
      {
      vtkIdType outSpanSize = static_cast<vtkIdType>(outSpanEndPtr - outPtr);
      vtkIdType inImageSpanSize = outSpanSize;
      vtkIdType inMaskSpanSize  = outSpanSize/numScalarsImage*numScalarsMask;
      inImagePtr += inImageSpanSize;
      inMaskPtr  += inMaskSpanSize;
      }

    // go to the next span
    outIter.NextSpan();
    if (inPtr == inSpanEndPtr)
      {
      inIter.NextSpan();
      inPtr = inIter.BeginSpan();
      inSpanEndPtr = inIter.EndSpan();
      }
    }
}



//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageAlphaLogic::ThreadedRequestData (
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  if (inData[0][0] == NULL)
    {
    vtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
    }

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() !=
        outData[0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, "
                  << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }

  // this filter expects that inputs that have the same number of components
  if (inData[0][0]->GetNumberOfScalarComponents() !=
        outData[0]->GetNumberOfScalarComponents())
    {
    vtkErrorMacro(<< "Execute: input NumberOfScalarComponents, "
                  << inData[0][0]->GetNumberOfScalarComponents()
                  << ", must match output NumberOfScalarComponents "
                  << inData[1][0]->GetNumberOfScalarComponents());
    return;
    }

  // TODO: Check size of images?

  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageAlphaLogicExecute(this, inData[0][0],
                                outData[0], outExt, id,
                                static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
int vtkImageAlphaLogic::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 2)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageAlphaLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}