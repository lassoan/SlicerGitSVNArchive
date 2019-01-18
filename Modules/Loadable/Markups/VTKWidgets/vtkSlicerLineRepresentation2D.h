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
 * @class   vtkSlicerLineRepresentation2D
 * @brief   Default representation for the line widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerLineWidget. It works in conjunction with the
 * vtkLinearSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerLineWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation2D vtkSlicerLineWidget vtkPointPlacer
 * vtkLinearSlicerLineInterpolator
*/

#ifndef vtkSlicerLineRepresentation2D_h
#define vtkSlicerLineRepresentation2D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation2D.h"

class vtkActor2D;
class vtkAppendPolyData;
class vtkOpenGLPolyDataMapper2D;
class vtkPolyData;
class vtkProperty2D;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerLineRepresentation2D : public vtkSlicerAbstractRepresentation2D
{
public:
  /// Instantiate this class.
  static vtkSlicerLineRepresentation2D *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerLineRepresentation2D,vtkSlicerAbstractRepresentation2D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// This is the property used by the line is not active
  /// (the mouse is not near the line)
  vtkGetObjectMacro(LinesProperty,vtkProperty2D);

  /// This is the property used when the user is interacting
  /// with the line.
  vtkGetObjectMacro(ActiveLinesProperty,vtkProperty2D);

  /// Subclasses of vtkContourCurveRepresentation must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void Highlight(int highlight) override;

  /// Methods to make this class behave as a vtkProp.
  void GetActors(vtkPropCollection *) override;
  void ReleaseGraphicsResources(vtkWindow *) override;
  int RenderOverlay(vtkViewport *viewport) override;
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  /// Set/Get the three leaders used to create this representation.
  /// By obtaining these leaders the user can set the appropriate
  /// properties, etc.
  vtkGetObjectMacro(LinesActor,vtkActor2D);

  /// Get the points in this contour as a vtkPolyData.
  vtkPolyData *GetWidgetRepresentationAsPolyData() override;

  /// Return the bounds of the representation
  double *GetBounds() override;

protected:
  vtkSlicerLineRepresentation2D();
  ~vtkSlicerLineRepresentation2D() override;

  vtkPolyData                  *Lines;
  vtkOpenGLPolyDataMapper2D    *LinesMapper;
  vtkActor2D                   *LinesActor;
  virtual void                 CreateDefaultProperties();

  vtkProperty2D   *LinesProperty;
  vtkProperty2D   *ActiveLinesProperty;

  virtual void BuildLines() override;

  vtkAppendPolyData *appendActors;

private:
  vtkSlicerLineRepresentation2D(const vtkSlicerLineRepresentation2D&) = delete;
  void operator=(const vtkSlicerLineRepresentation2D&) = delete;
};

#endif
