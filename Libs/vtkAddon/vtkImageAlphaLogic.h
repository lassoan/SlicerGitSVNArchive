// .NAME vtkImageAlphaLogic
// .SECTION Description
// vtkImageAlphaLogic implements fast alpha channel masking on RGBA images.
// The filter takes an RGBA image as input and an option stencil and/or
// binary mask image.
// The output is the image with the stencil and mask applied.
// vtkImageInPlaceFilter is a filter super class that
// operates directly on the input region (on the image in place).
// The data is copied if the requested region has different extent than the input region
// or some other object is referencing the input region.

#ifndef vtkImageAlphaLogic_h
#define vtkImageAlphaLogic_h

#include "vtkAddon.h"
#include "vtkThreadedImageAlgorithm.h"
#include "vtkImageStencilData.h"

//Which class should I inherit from? vtkThreadedImageAlgorithm, or vtkImageInPlaceFilter?
class VTK_ADDON_EXPORT vtkImageAlphaLogic : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageAlphaLogic *New();
  vtkTypeMacro(vtkImageAlphaLogic,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the default color to use when the second input is not set.
  // This is like SetBackgroundValue, but for multi-component images.
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVector3Macro(BackgroundColor, double);

  // Description:
  // Set the Image data
  void SetImageData(vtkDataObject *input) { this->SetInputData(0,input);};

  void SetImageConnection(vtkAlgorithmOutput* outputPort) { this->SetInputConnection(0, outputPort);};

  // Description:
  // Set the Mask data
  void SetMaskData(vtkDataObject *input) { this->SetInputData(1,input);};

  // Description:
  // Set the Mask input connection
  void SetMaskConnection(vtkAlgorithmOutput* outputPort) { this->SetInputConnection(1, outputPort);};

  // Description:
  // Set the Stencil data
  void SetStencilData(vtkImageStencilData *input) { this->SetInputData(2,input);};

  // Description:
  // Set the Stencil input connection
  void SetStencilConnection(vtkAlgorithmOutput* outputPort) { this->SetInputConnection(2, outputPort);};

  // Description:
  // Get the Stencil data
  vtkImageStencilData *GetStencilData() { return (vtkImageStencilData*)this->GetInput(0);};

  // Description:
  // Enum of bar chart oritentation types
  enum
    {
    IMAGE_INPUT_PORT = 0,
    MASK_INPUT_PORT,
    STENCIL_INPUT_PORT,
    NUMBER_OF_INPUT_PORTS // insert port numbers before this line
    };
  enum
    {
    IMAGE_OUTPUT_PORT = 0,
    NUMBER_OF_OUTPUT_PORTS // insert port numbers before this line
    };

protected:
  vtkImageAlphaLogic();
  ~vtkImageAlphaLogic() {}

  double BackgroundColor[3];

  void ThreadedRequestData (vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector,
                            vtkImageData ***inData, vtkImageData **outData,
                            int ext[6], int id);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Default method overridden to get input image information that will be used
  // in AllocateOutputData.
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // Default method overridden to not just allocate but also copy or pass through the input data to the output.
  virtual void AllocateOutputData(vtkImageData *out,
                                  vtkInformation* outInfo,
                                  int *uExtent);

  void CopyData(vtkImageData *in, vtkImageData *out, int* outExt);

  // The input image data is needed for the overridden AllocateOutputData method in this class,
  // which requires the input image data, but it does not get it as input argument.
  vtkInformation *InputImageInformation;

private:
  vtkImageAlphaLogic(const vtkImageAlphaLogic&);  // Not implemented.
  void operator=(const vtkImageAlphaLogic&);  // Not implemented.
};

#endif
