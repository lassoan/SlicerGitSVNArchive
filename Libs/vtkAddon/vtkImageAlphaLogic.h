// .NAME vtkImageAlphaLogic
// .SECTION Description
// vtkImageAlphaLogic implements Slicer's slice rendering.
// The filter takes three inputs: an image, a stencil, and a mask
// The output is the image with the stencil and mask applied.
// The filter operates on the image in place.


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

  // Description:
  // Set the Mask data
  void SetMaskData(vtkDataObject *input) { this->SetInputData(1,input);};

  // Description:
  // Set the Stencil data
  void SetStencilData(vtkImageStencilData *input) { this->SetInputData(2,input);};

  // Description:
  // Get the Stencil data
  vtkImageStencilData *GetStencilData() { return (vtkImageStencilData*)this->GetInput(0);};

  // Description:
  // Set the Stencil connection
  //void SetStencilConnection(vtkAlgorithmOutput* outputPort) { this->SetInputConnection(2, outputPort);};

  // TODO: Set up connections for all of the inputs

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

private:
  vtkImageAlphaLogic(const vtkImageAlphaLogic&);  // Not implemented.
  void operator=(const vtkImageAlphaLogic&);  // Not implemented.
};

#endif