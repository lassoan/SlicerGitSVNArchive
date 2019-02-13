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
 * @class   vtkSlicerAbstractRepresentation3D
 * @brief   Default representation for the slicer markups widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerAbstractWidget. It works in conjunction with the
 * vtkSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerAbstractWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation3D vtkSlicerAbstractWidget vtkPointPlacer
 * vtkSlicerLineInterpolator
*/

#ifndef vtkSlicerAbstractRepresentation3D_h
#define vtkSlicerAbstractRepresentation3D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation.h"

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

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAbstractRepresentation3D : public vtkSlicerAbstractRepresentation
{
public:
  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerAbstractRepresentation3D,vtkSlicerAbstractRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Subclasses of vtkSlicerAbstractRepresentation3D must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void BuildRepresentation() VTK_OVERRIDE;
  int ComputeInteractionState(int X, int Y, int modified = 0) VTK_OVERRIDE { return 0; };
  void WidgetInteraction(double eventPos[2]) VTK_OVERRIDE { return; };

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
  vtkSlicerAbstractRepresentation3D();
  ~vtkSlicerAbstractRepresentation3D() VTK_OVERRIDE;

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
  vtkSlicerAbstractRepresentation3D(const vtkSlicerAbstractRepresentation3D&) = delete;
  void operator=(const vtkSlicerAbstractRepresentation3D&) = delete;
};

#endif
