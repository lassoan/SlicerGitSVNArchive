#include "vtkImageAlphaLogic.h"
#include "vtkInformation.h"
#include "vtkImageData.h"
#include "vtkImageIterator.h"
#include "vtkImageStencilData.h"
#include "vtkImageStencilIterator.h"
#include "vtkInformationVector.h"
#include "vtkLargeInteger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

vtkStandardNewMacro(vtkImageAlphaLogic);

//----------------------------------------------------------------------------
vtkImageAlphaLogic::vtkImageAlphaLogic()
{
  this->BackgroundColor[0] = 0;
  this->BackgroundColor[1] = 0;
  this->BackgroundColor[2] = 0;
  this->SetNumberOfInputPorts(vtkImageAlphaLogic::NUMBER_OF_INPUT_PORTS);
  this->InputImageInformation = NULL;
}

/*
//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageAlphaLogicExecuteRGBA(vtkImageAlphaLogic *self, vtkImageData ***inData,
                                vtkImageData **outData, int outExt[6], int id, T *)
{
  int numScalarsImage = inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0]->GetNumberOfScalarComponents();

  vtkImageStencilData *stencil = vtkImageStencilData::SafeDownCast(inData[vtkImageAlphaLogic::STENCIL_INPUT_PORT][0]);
  vtkImageStencilIterator<T> outIter(outData[vtkImageAlphaLogic::IMAGE_OUTPUT_PORT], stencil, outExt, self, id);
  vtkImageIterator<T> inImageIter(inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0], outExt);
  vtkImageIterator<T> inMaskIter(inData[vtkImageAlphaLogic::MASK_INPUT_PORT][0], outExt);

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
        if (*(inMaskPtr++) != 0)
          {
          *(outPtr++) = *(inImagePtr++);
          *(outPtr++) = *(inImagePtr++);
          *(outPtr++) = *(inImagePtr++);
          }
        }
      }
    else // !IsInStencil()
      {
      vtkIdType outSpanSize = static_cast<vtkIdType>(outSpanEndPtr - outPtr);
      vtkIdType inImageSpanSize = outSpanSize;
      vtkIdType inMaskSpanSize  = outSpanSize/numScalarsImage;
      inImagePtr += inImageSpanSize;
      inMaskPtr  += inMaskSpanSize;
      }

    // go to the next span
    outIter.NextSpan();
    }
}
*/

template <class T>
void vtkImageAlphaLogicExecute(vtkImageAlphaLogic *self, vtkImageData ***inData,
                                vtkImageData **outData, int outExt[6], int id, T *)
{
  return;
  int numScalarsImage = inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0]->GetNumberOfScalarComponents();
  int numScalarsMask  = inData[vtkImageAlphaLogic::MASK_INPUT_PORT][0]->GetNumberOfScalarComponents();
  int maskAlphaComponent = 0;

  vtkImageStencilData *stencil = vtkImageStencilData::SafeDownCast(inData[vtkImageAlphaLogic::STENCIL_INPUT_PORT][0]);
  vtkImageStencilIterator<T> outIter(outData[vtkImageAlphaLogic::IMAGE_OUTPUT_PORT], stencil, outExt, self, id);
  vtkImageIterator<T> inImageIter(inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0], outExt);
  vtkImageIterator<T> inMaskIter(inData[vtkImageAlphaLogic::MASK_INPUT_PORT][0], outExt);

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
  if (inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0] == NULL)
    {
    vtkErrorMacro(<< "Input " << 0 << " must be specified.");
    return;
    }

  // this filter expects that input is the same type as output.
  if (inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0]->GetScalarType() !=
        outData[vtkImageAlphaLogic::IMAGE_OUTPUT_PORT]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, "
                  << inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0]->GetScalarType()
                  << ", must match out ScalarType "
                  << outData[vtkImageAlphaLogic::IMAGE_OUTPUT_PORT]->GetScalarType());
    return;
    }

  // this filter expects that input and output have the same number of components
  if (inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0]->GetNumberOfScalarComponents() !=
        outData[vtkImageAlphaLogic::IMAGE_OUTPUT_PORT]->GetNumberOfScalarComponents())
    {
    vtkErrorMacro(<< "Execute: input NumberOfScalarComponents, "
                  << inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0]->GetNumberOfScalarComponents()
                  << ", must match output NumberOfScalarComponents "
                  << outData[vtkImageAlphaLogic::IMAGE_OUTPUT_PORT]->GetNumberOfScalarComponents());
    return;
    }

  // TODO: Check size of images?

  switch (inData[vtkImageAlphaLogic::IMAGE_INPUT_PORT][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageAlphaLogicExecute(this, inData,
                                outData, outExt, id,
                                static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
int vtkImageAlphaLogic::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
    {
    case vtkImageAlphaLogic::IMAGE_INPUT_PORT:
    case vtkImageAlphaLogic::MASK_INPUT_PORT:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
      info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
      return 1;
    case vtkImageAlphaLogic::STENCIL_INPUT_PORT:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
      info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
      return 1;
    default:
      return 0;
    }
}

//----------------------------------------------------------------------------
void vtkImageAlphaLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkImageAlphaLogic::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the input image data before calling the default implementation.
  this->InputImageInformation = NULL;
  if (this->GetNumberOfInputPorts()>vtkImageAlphaLogic::IMAGE_INPUT_PORT)
    {
    this->InputImageInformation = inputVector[vtkImageAlphaLogic::IMAGE_INPUT_PORT]->GetInformationObject(0);
    }
  int result = this->Superclass::RequestData(request, inputVector, outputVector);
  // Set InputImageInformation pointer to NULL to prevent having any dangling pointers
  this->InputImageInformation = NULL;
  return result;
}

//----------------------------------------------------------------------------
void vtkImageAlphaLogic::AllocateOutputData(vtkImageData *output,
                                           vtkInformation *outInfo,
                                           int *outExt)
{
  if (this->InputImageInformation)
    {
    vtkImageData *input = vtkImageData::SafeDownCast(this->InputImageInformation->Get(vtkDataObject::DATA_OBJECT()));
    int *inExt = this->InputImageInformation->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

    // if the total size of the data is the same then can be in place
    vtkLargeInteger inSize;
    inSize = (inExt[1] - inExt[0] + 1);
    inSize = inSize * (inExt[3] - inExt[2] + 1);
    inSize = inSize * (inExt[5] - inExt[4] + 1);
    vtkLargeInteger outSize;
    outSize = (outExt[1] - outExt[0] + 1);
    outSize = outSize * (outExt[3] - outExt[2] + 1);
    outSize = outSize * (outExt[5] - outExt[4] + 1);
    if (inSize == outSize
      // && (vtkDataObject::GetGlobalReleaseDataFlag() || this->InputImageInformation->Get(vtkDemandDrivenPipeline::RELEASE_DATA()))
         )
      {
      // pass the data
      output->GetPointData()->PassData(input->GetPointData());
      output->SetExtent(outExt);
      return;
      }
    }

  // We cannot pass through the input. We copy the input to the output  here so that the processing
  // can always be done in place.
  output->SetExtent(outExt);
  output->AllocateScalars(outInfo);
  if (this->InputImageInformation)
    {
    vtkImageData *input = vtkImageData::SafeDownCast(this->InputImageInformation->Get(vtkDataObject::DATA_OBJECT()));
    this->CopyData(input,output,outExt);
    }
}

//----------------------------------------------------------------------------
void vtkImageAlphaLogic::CopyData(vtkImageData *inData,
                                     vtkImageData *outData,
                                     int* outExt)
{
  char *inPtr = static_cast<char *>(inData->GetScalarPointerForExtent(outExt));
  char *outPtr =
    static_cast<char *>(outData->GetScalarPointerForExtent(outExt));
  int rowLength, size;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int idxY, idxZ, maxY, maxZ;

  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  size = inData->GetScalarSize();
  rowLength *= size;
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];

  // Get increments to march through data
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // adjust increments for this loop
  inIncY = inIncY*size + rowLength;
  outIncY = outIncY*size + rowLength;
  inIncZ *= size;
  outIncZ *= size;

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memcpy(outPtr,inPtr,rowLength);
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}
