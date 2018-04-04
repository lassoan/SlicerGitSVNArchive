// .NAME vtkStaticOpacityTexture
// .SECTION Description
// vtkStaticOpacityTexture allows setting a fixed value opacity/translucency value
// for a texture. This is needed because most of the time translucency of a texture
// is static but vtkTexture class recomputes it every time the texture is changed
// in a very computationally expensive manner.

#ifndef vtkStaticOpacityTexture_h
#define vtkStaticOpacityTexture_h

#include "vtkAddon.h"
#include "vtkOpenGLTexture.h"

class VTK_ADDON_EXPORT vtkStaticOpacityTexture : public vtkOpenGLTexture
{
public:
  static vtkStaticOpacityTexture *New();
  vtkTypeMacro(vtkStaticOpacityTexture,vtkOpenGLTexture);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the method for computing opacity of the texture.
  // Auto means the opacity is recomputed each time the texture is changed (default behavior, slow).
  // Always opaque/translucent means a fixed value will be used (fast, but the user has to make sure the correct value is set).
  vtkGetMacro(Opacity,int);
  vtkSetMacro(Opacity,int);
  void SetOpacityToAuto() { SetOpacity(AUTO); };
  void SetOpacityToAlwaysOpaque() { SetOpacity(ALWAYS_OPAQUE); };
  void SetOpacityToAlwaysTranslucent() { SetOpacity(ALWAYS_TRANSLUCENT); };

  // Description:
  // Utility function for converting opacity value to a human-readable string
  static const char* OpacityToString(int opacity);

  // Description:
  // Is this Texture Translucent?
  // returns false (0) if the texture is either fully opaque or has
  // only fully transparent pixels and fully opaque pixels and the
  // Interpolate flag is turn off.
  // If Opacity is not set to auto then it uses the slow vtkTexture implementation
  // otherwise it returns with the fixed value.
  virtual int IsTranslucent();

  enum
    {
    AUTO = 0,
    ALWAYS_OPAQUE = 1,
    ALWAYS_TRANSLUCENT = 2
    };

protected:
  vtkStaticOpacityTexture();
  ~vtkStaticOpacityTexture() {}

  int Opacity;

private:
  vtkStaticOpacityTexture(const vtkStaticOpacityTexture&);  // Not implemented.
  void operator=(const vtkStaticOpacityTexture&);  // Not implemented.
};

#endif