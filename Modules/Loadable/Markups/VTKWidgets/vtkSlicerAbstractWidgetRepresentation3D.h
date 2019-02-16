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
 * @class   vtkSlicerAbstractWidgetRepresentation3D
 * @brief   Default representation for the slicer markups widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerAbstractWidget. It works in conjunction with the
 * vtkSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerAbstractWidget
 * for details.
 * @sa
 * vtkSlicerAbstractWidgetRepresentation3D vtkSlicerAbstractWidget vtkPointPlacer
 * vtkSlicerLineInterpolator
*/

#ifndef vtkSlicerAbstractWidgetRepresentation3D_h
#define vtkSlicerAbstractWidgetRepresentation3D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractWidgetRepresentation.h"

#include "vtkMRMLMarkupsNode.h"

#include <vector> // STL Header; Required for vector

class vtkProperty;
class vtkOpenGLActor;
class vtkOpenGLPolyDataMapper;
class vtkGlyph3D;
class vtkLabelPlacementMapper;
class vtkPointSetToLabelHierarchy;
class vtkStringArray;
class vtkActor2D;
class vtkTextProperty;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAbstractWidgetRepresentation3D : public vtkSlicerAbstractWidgetRepresentation
{
public:
  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerAbstractWidgetRepresentation3D,vtkSlicerAbstractWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Subclasses of vtkSlicerAbstractWidgetRepresentation3D must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void UpdateFromMRML() VTK_OVERRIDE;

  /// Methods to make this class behave as a vtkProp.
  void GetActors(vtkPropCollection *) VTK_OVERRIDE;
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  vtkTypeBool HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /// Return the bounds of the representation
  double *GetBounds() VTK_OVERRIDE;

  int CanInteract(const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &componentIndex) VTK_OVERRIDE;

  /// Checks if interaction with straight line between visible points is possible.
  /// Can be used on the output of CanInteract, as if no better component is found then the input is returned.
  void CanInteractWithLine(int &foundComponentType, const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &componentIndex);

protected:
  vtkSlicerAbstractWidgetRepresentation3D();
  ~vtkSlicerAbstractWidgetRepresentation3D() VTK_OVERRIDE;

  class ControlPointsPipeline3D : public ControlPointsPipeline
  {
  public:
    ControlPointsPipeline3D();

    vtkSmartPointer<vtkOpenGLActor> Actor;
    vtkSmartPointer<vtkOpenGLPolyDataMapper> Mapper;
    vtkSmartPointer<vtkGlyph3D> Glypher;
    vtkSmartPointer<vtkActor2D> LabelsActor;
    vtkSmartPointer<vtkLabelPlacementMapper> LabelsMapper;
    // Properties used to control the appearance of selected objects and
    // the manipulator in general.
    vtkSmartPointer<vtkProperty> Property;
  };

  ControlPointsPipeline3D* GetControlPointsPipeline(int controlPointType);

  virtual void BuildRepresentationPointsAndLabels();

private:
  vtkSlicerAbstractWidgetRepresentation3D(const vtkSlicerAbstractWidgetRepresentation3D&) = delete;
  void operator=(const vtkSlicerAbstractWidgetRepresentation3D&) = delete;
};

#endif
