#include "vtkStaticOpacityTexture.h"
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

vtkStandardNewMacro(vtkStaticOpacityTexture);

//----------------------------------------------------------------------------
vtkStaticOpacityTexture::vtkStaticOpacityTexture()
{
  this->Opacity = vtkStaticOpacityTexture::AUTO;
}

//----------------------------------------------------------------------------
void vtkStaticOpacityTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Opacity: " << this->OpacityToString(this->GetOpacity()) << "\n";
}

//----------------------------------------------------------------------------
int vtkStaticOpacityTexture::IsTranslucent()
{
  switch (this->Opacity)
  {
    case vtkStaticOpacityTexture::ALWAYS_OPAQUE:
      return 0;
    case vtkStaticOpacityTexture::ALWAYS_TRANSLUCENT:
      return 1;
    case vtkStaticOpacityTexture::AUTO:
    default:
      return this->Superclass::IsTranslucent();
  }
}

//----------------------------------------------------------------------------
const char* vtkStaticOpacityTexture::OpacityToString(int opacity)
{
  switch (opacity)
  {
    case vtkStaticOpacityTexture::ALWAYS_OPAQUE:
      return "ALWAYS_OPAQUE";
    case vtkStaticOpacityTexture::ALWAYS_TRANSLUCENT:
      return "ALWAYS_TRANSLUCENT";
    case vtkStaticOpacityTexture::AUTO:
      return "AUTO";
    default:
      return "INVALID";
  }
}
