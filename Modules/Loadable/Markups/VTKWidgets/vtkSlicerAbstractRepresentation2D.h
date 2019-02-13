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
 * @class   vtkSlicerAbstractRepresentation2D
 * @brief   Default representation for the slicer markups widget
 *
 * This class provides the default concrete representation for the
 * vtkSlicerAbstractWidget. It works in conjunction with the
 * vtkSlicerLineInterpolator and vtkPointPlacer. See vtkSlicerAbstractWidget
 * for details.
 * @sa
 * vtkSlicerAbstractRepresentation2D vtkSlicerAbstractWidget vtkPointPlacer
 * vtkSlicerLineInterpolator
*/

#ifndef vtkSlicerAbstractRepresentation2D_h
#define vtkSlicerAbstractRepresentation2D_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractRepresentation.h"

#include "vtkMRMLSliceNode.h"

class vtkActor2D;
class vtkGlyph2D;
class vtkLabelPlacementMapper;
class vtkMarkupsGlyphSource2D;
class vtkOpenGLPolyDataMapper2D;
class vtkProperty2D;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAbstractRepresentation2D : public vtkSlicerAbstractRepresentation
{
public:
  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerAbstractRepresentation2D,vtkSlicerAbstractRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Position is displayed (slice) position
  int CanInteract(const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &itemIndex) VTK_OVERRIDE;

  /// Checks if interaction with straight line between visible points is possible.
  /// Can be used on the output of CanInteract, as if no better component is found then the input is returned.
  void CanInteractWithLine(int &foundComponentType, const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &componentIndex);

  /// Given a display position, activate a node. The closest
  /// node within tolerance will be activated. If a node is
  /// activated, 1 will be returned, otherwise 0 will be
  /// returned.
  virtual int ActivateNode(int X, int Y) VTK_OVERRIDE;

  /// Subclasses of vtkSlicerAbstractRepresentation2D must implement these methods. These
  /// are the methods that the widget and its representation use to
  /// communicate with each other.
  void BuildRepresentation() VTK_OVERRIDE;
  int ComputeInteractionState(int X, int Y, int modified=0) VTK_OVERRIDE;
  void WidgetInteraction(double eventPos[2]) VTK_OVERRIDE;

  /// Methods to make this class behave as a vtkProp.
  void GetActors(vtkPropCollection *) VTK_OVERRIDE;
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;
  int RenderOverlay(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  vtkTypeBool HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /// Specify the cursor shape. Keep in mind that the shape will be
  /// aligned with the constraining plane by orienting it such that
  /// the x axis of the geometry lies along the normal of the plane.
  void SetPointMarkerShape(vtkPolyData *pointMarkerShape);
  vtkPolyData *GetPointMarkerShape();

  /// Specify the cursor shape. Keep in mind that the shape will be
  /// aligned with the constraining plane by orienting it such that
  /// the x axis of the geometry lies along the normal of the plane.
  void SetSelectedPointMarkerShape(vtkPolyData *selectedShape);
  vtkPolyData *GetSelectedPointMarkerShape();

  /// Specify the shape of the cursor (handle) when it is active.
  /// This is the geometry that will be used when the mouse is
  /// close to the handle or if the user is manipulating the handle.
  void SetActivePointMarkerShape(vtkPolyData *activeShape);
  vtkPolyData *GetActivePointMarkerShape();

  /// Set/Get method to the sliceNode connected to this representation
  void SetSliceNode(vtkMRMLSliceNode *sliceNode);
  vtkMRMLSliceNode *GetSliceNode();

  /// Get the nth node's position on the slice. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting).
  int GetNthNodeDisplayPosition(int n, double pos[2]) VTK_OVERRIDE;

  /// Get the display position of the intermediate point at
  /// index idx between nodes n and (n+1) (or n and 0 if
  /// n is the last node and the loop is closed). Returns
  /// 1 on success or 0 if n or idx are out of range.
  virtual int GetIntermediatePointDisplayPosition(int n,
                                                  int idx, double pos[2]) VTK_OVERRIDE;

  /// Set the nth node's position on the slice. slice position
  /// will be converted into world position according to the
  /// GetXYToRAS matrix. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting) or the world position
  /// is not valid.
  int SetNthNodeDisplayPosition(int n, const double pos[2]) VTK_OVERRIDE;

  /// Add a node at a specific position on the slice. This will be
  /// converted into a world position according to the current
  /// constraints of the point placer. Return 0 if a point could
  /// not be added, 1 otherwise.
  int AddNodeAtDisplayPosition(const double slicePos[2]) VTK_OVERRIDE;

  /// Set the Nth node slice visibility (i.e. if it is on the slice).
  virtual void SetNthPointSliceVisibility(int n, bool visibility);

  /// Set the centroid slice visibility (i.e. if it is on the slice).
  virtual void SetCentroidSliceVisibility(bool visibility);

  void GetSliceToWorldCoordinates(const double[2], double[3]);
  void GetWorldToSliceCoordinates(const double worldPos[3], double slicePos[2]);

  /// Set/Get the 2d scale factor to divide 3D scale by to show 2D elements appropriately (usually set to 300)
  vtkSetMacro(ScaleFactor2D, double);
  vtkGetMacro(ScaleFactor2D, double);

protected:
  vtkSlicerAbstractRepresentation2D();
  ~vtkSlicerAbstractRepresentation2D() VTK_OVERRIDE;

  vtkWeakPointer<vtkMRMLSliceNode> SliceNode;

  class ControlPointsPipeline2D : public ControlPointsPipeline
  {
  public:
    ControlPointsPipeline2D();

    vtkSmartPointer<vtkActor2D> Actor;
    vtkSmartPointer<vtkOpenGLPolyDataMapper2D> Mapper;
    vtkSmartPointer<vtkGlyph2D> Glypher;
    vtkSmartPointer<vtkActor2D> LabelsActor;
    vtkSmartPointer<vtkLabelPlacementMapper> LabelsMapper;
    // Properties used to control the appearance of selected objects and
    // the manipulator in general.
    vtkSmartPointer<vtkProperty2D> Property;
  };

  ControlPointsPipeline2D* GetControlPointsPipeline(int controlPointType);

  vtkSmartPointer<vtkIntArray> PointsVisibilityOnSlice;
  bool                        CentroidVisibilityOnSlice;

  /// Scale factor for 2d windows
  double ScaleFactor2D;

  virtual void  CreateDefaultProperties() VTK_OVERRIDE;
  virtual void BuildRepresentationPointsAndLabels(double labelsOffset);
  void BuildLocator() VTK_OVERRIDE;
  virtual void AddNodeAtPositionInternal(const double worldPos[3]) VTK_OVERRIDE;

private:
  vtkSlicerAbstractRepresentation2D(const vtkSlicerAbstractRepresentation2D&) = delete;
  void operator=(const vtkSlicerAbstractRepresentation2D&) = delete;
};

#endif
