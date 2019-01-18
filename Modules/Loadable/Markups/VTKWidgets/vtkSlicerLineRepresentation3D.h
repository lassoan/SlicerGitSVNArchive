/*=========================================================================

 Copyright (c) ProxSim ltd., Kwun Tong, Hong Kong. All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 This file was originally developed by Davide Punzo, punzodavide@hotmail.it,
 and development was supported by ProxSim ltd.

=========================================================================*/

/**
 * @class   vtkSlicerLineRepresentation3D
 * @brief   Default representation for the line widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerLineWidget. It works in conjunction with the
 * vtkLinearSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerLineWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation vtkSlicerLineWidget vtkPointPlacer
 * vtkLinearSlicerLineInterpolator
*/

#ifndef vtkSlicerLineRepresentation3D_h
#define vtkSlicerLineRepresentation3D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation3D.h"

class vtkAppendPolyData;
class vtkOpenGLActor;
class vtkOpenGLPolyDataMapper;
class vtkPolyData;
class vtkProperty;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerLineRepresentation3D : public vtkSlicerAbstractRepresentation3D
{
public:
  /// Instantiate this class.
  static vtkSlicerLineRepresentation3D *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerLineRepresentation3D,vtkSlicerAbstractRepresentation3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// This is the property used by the line is not active
  /// (the mouse is not near the line)
  vtkGetObjectMacro(LinesProperty,vtkProperty);

  /// This is the property used when the user is interacting
  /// with the line.
  vtkGetObjectMacro(ActiveLinesProperty,vtkProperty);

  /// Subclasses of vtkSlicerAbstractRepresentation must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void Highlight(int highlight) VTK_OVERRIDE;

  /// Methods to make this class behave as a vtkProp.
  void GetActors(vtkPropCollection *) VTK_OVERRIDE;
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  vtkTypeBool HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /// Set/Get the three leaders used to create this representation.
  /// By obtaining these leaders the user can set the appropriate
  /// properties, etc.
  vtkGetObjectMacro(LinesActor,vtkOpenGLActor);

  /// Get the points in this contour as a vtkPolyData.
  vtkPolyData *GetWidgetRepresentationAsPolyData() VTK_OVERRIDE;

  /// Return the bounds of the representation
  double *GetBounds() VTK_OVERRIDE;

protected:
  vtkSlicerLineRepresentation3D();
  ~vtkSlicerLineRepresentation3D() VTK_OVERRIDE;

  vtkPolyData                *Lines;
  vtkOpenGLPolyDataMapper    *LinesMapper;
  vtkOpenGLActor             *LinesActor;
  virtual void         CreateDefaultProperties();

  vtkProperty   *LinesProperty;
  vtkProperty   *ActiveLinesProperty;

  virtual void BuildLines() VTK_OVERRIDE;

  vtkAppendPolyData *appendActors;

private:
  vtkSlicerLineRepresentation3D(const vtkSlicerLineRepresentation3D&) = delete;
  void operator=(const vtkSlicerLineRepresentation3D&) = delete;
};

#endif
