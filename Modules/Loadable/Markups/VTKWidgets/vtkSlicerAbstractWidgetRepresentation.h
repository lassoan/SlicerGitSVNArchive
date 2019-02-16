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
 * @class   vtkSlicerAbstractWidgetRepresentation
 * @brief   Class for rendering a markups node
 *
 * This class can display a markups node in the scene.
 * It plays a similar role to vtkWidgetRepresentation, but it is
 * simplified and specialized for optimal use in Slicer.
 * It state is stored in the associated MRML display node to
 * avoid extra synchronization mechanisms.
 * The representation only observes MRML node changes,
 * it does not directly process any interaction events directly
 * (interaction events are processed by vtkSlicerAbstractWidget,
 * which then modifies MRML nodes).
 *
 * This class (and subclasses) are a type of
 * vtkProp; meaning that they can be associated with a vtkRenderer end
 * embedded in a scene like any other vtkActor.
*
 * @sa
 * vtkSlicerAbstractWidgetRepresentation vtkSlicerAbstractWidget vtkPointPlacer
 * vtkSlicerLineInterpolator
*/

#ifndef vtkSlicerAbstractRepresentation_h
#define vtkSlicerAbstractRepresentation_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkWidgetRepresentation.h"

#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsNode.h"

#include <vector> // STL Header; Required for vector

class vtkMarkupsGlyphSource2D;
class vtkPolyData;
class vtkPoints;

class vtkSlicerLineInterpolator;
class vtkIncrementalOctreePointLocator;
class vtkPointPlacer;
class vtkPolyData;
class vtkIdList;
class vtkPointSetToLabelHierarchy;
class vtkSphereSource;
class vtkStringArray;
class vtkTextProperty;

#include "vtkBoundingBox.h"

class ControlPointsPipeline;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAbstractWidgetRepresentation : public vtkProp
{
public:
  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerAbstractWidgetRepresentation, vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;


  //@{
  /**
  * Subclasses of vtkWidgetRepresentation must implement these methods. This is
  * considered the minimum API for a widget representation.
  * <pre>
  * SetRenderer() - the renderer in which the representations draws itself.
  * Typically the renderer is set by the associated widget.
  * Use the widget's SetCurrentRenderer() method in most cases;
  * otherwise there is a risk of inconsistent behavior as events
  * and drawing may be performed in different viewports.
  * BuildRepresentation() - update the geometry of the widget based on its
  * current state.
  * </pre>
  * WARNING: The renderer is NOT reference counted by the representation,
  * in order to avoid reference loops.  Be sure that the representation
  * lifetime does not extend beyond the renderer lifetime.
  */
  virtual void SetRenderer(vtkRenderer *ren);
  virtual vtkRenderer* GetRenderer();
  virtual void BuildRepresentation() = 0;
  //@}


  /**
  * Methods to make this class behave as a vtkProp. They are repeated here (from the
  * vtkProp superclass) as a reminder to the widget implementor. Failure to implement
  * these methods properly may result in the representation not appearing in the scene
  * (i.e., not implementing the Render() methods properly) or leaking graphics resources
  * (i.e., not implementing ReleaseGraphicsResources() properly).
  */
  double *GetBounds() VTK_SIZEHINT(6) override { return nullptr; }
  void ShallowCopy(vtkProp *prop) override;
  void GetActors(vtkPropCollection *) override {}
  void GetActors2D(vtkPropCollection *) override {}
  void GetVolumes(vtkPropCollection *) override {}
  void ReleaseGraphicsResources(vtkWindow *) override {}
  int RenderOverlay(vtkViewport *vtkNotUsed(viewport)) override { return 0; }
  int RenderOpaqueGeometry(vtkViewport *vtkNotUsed(viewport)) override { return 0; }
  int RenderTranslucentPolygonalGeometry(vtkViewport *vtkNotUsed(viewport)) override { return 0; }
  int RenderVolumetricGeometry(vtkViewport *vtkNotUsed(viewport)) override { return 0; }
  vtkTypeBool HasTranslucentPolygonalGeometry() override { return 0; }



  /// Add a node at a specific world position. Returns 0 if the
  /// node could not be added, 1 otherwise.
  virtual int AddNodeAtWorldPosition(double x, double y, double z);
  virtual int AddNodeAtWorldPosition(const double worldPos[3]);

  /// Add a node at a specific display position. This will be
  /// converted into a world position according to the current
  /// constraints of the point placer. Return 0 if a point could
  /// not be added, 1 otherwise.
  virtual int AddNodeAtDisplayPosition(const double displayPos[2]);
  virtual int AddNodeAtDisplayPosition(const int displayPos[2]);
  virtual int AddNodeAtDisplayPosition(int X, int Y);

  /// Move the active node to a specified world position.
  /// Will return 0 if there is no active node or the node
  /// could not be moved to that position. 1 will be returned
  /// on success.
  virtual int SetActiveNodeToWorldPosition(const double pos[3]);

  /// Move the active node based on a specified display position.
  /// The display position will be converted into a world
  /// position. If the new position is not valid or there is
  /// no active node, a 0 will be returned. Otherwise, on
  /// success a 1 will be returned.
  virtual int SetActiveNodeToDisplayPosition(const double pos[2]);
  virtual int SetActiveNodeToDisplayPosition(const int pos[2]);
  virtual int SetActiveNodeToDisplayPosition(int X, int Y);
  //@}

  /// Get/Set the active node.
  /// If index is from 0 to N it indicates the active point index.
  /// If is -1 indicates that nothing is selected.
  /// If is -2 indicates that the line is selected.
  /// If is -3 indicates that the centroid is selected.
  virtual int GetActiveNode();
  virtual void SetActiveNode(int index);

  /// Get the world position of the active node. Will return
  /// 0 if there is no active node, or 1 otherwise.
  virtual int GetActiveNodeWorldPosition(double pos[3]);

  /// Get the display position of the active node. Will return
  /// 0 if there is no active node, or 1 otherwise.
  virtual int GetActiveNodeDisplayPosition(double pos[2]);

  /// Set the active node's visibility.
  virtual void SetActiveNodeVisibility(bool visibility);

  /// Get the active node's visibility.
  virtual bool GetActiveNodeVisibility();

  /// Set if the active node is selected.
  virtual void SetActiveNodeSelected(bool selected);

  /// Get if the active node is locked.
  virtual bool GetActiveNodeSelected();

  /// Set if the active node is locked.
  virtual void SetActiveNodeLocked(bool locked);

  /// Get if the active node is locked.
  virtual bool GetActiveNodeLocked();

  /// Set the active node's orietation.
  virtual void SetActiveNodeOrientation(double orientation[4]);

  /// Get the active node's orietation.
  virtual void GetActiveNodeOrientation(double orientation[4]);

  /// Set the active node's label.
  virtual void SetActiveNodeLabel(vtkStdString label);

  /// Get the active node's label.
  virtual vtkStdString GetActiveNodeLabel();

  /// Get the number of nodes.
  virtual int GetNumberOfNodes();

  /// Get the nth node's display position. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting).
  virtual int GetNthNodeDisplayPosition(int n, double pos[2]);

  /// Get the nth node's world position. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting).
  virtual int GetNthNodeWorldPosition(int n, double pos[3]);

  /// Get the nth node's visibility.
  virtual bool GetNthNodeVisibility(int n);

  /// Set the nth node's visibility.
  virtual void SetNthNodeVisibility(int n, bool visibility);

  /// Get the if nth node is selected.
  virtual bool GetNthNodeSelected(int n);

  /// Set the nth node is selected.
  virtual void SetNthNodeSelected(int n, bool selected);

  /// Get if the nth node is locked.
  virtual bool GetNthNodeLocked(int n);

  /// Set if the nth node is locked.
  virtual void SetNthNodeLocked(int n, bool locked);

  /// Set the nth node's orietation.
  virtual void SetNthNodeOrientation(int n , double orientation[4]);

  /// Get the nth node's orietation.
  virtual void GetNthNodeOrientation(int n , double orientation[4]);

  /// Set the nth node's label.
  virtual void SetNthNodeLabel(int n , vtkStdString label);

  /// Get the nth node's label.
  virtual vtkStdString GetNthNodeLabel(int n);

  /// Get the nth node.
  virtual ControlPoint *GetNthNode(int n);

  /// Return true if n is a valid node, false otherwise
  bool NodeExists(int n);

  /// Set the nth node's display position. Display position
  /// will be converted into world position according to the
  /// constraints of the point placer. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting) or the world position
  /// is not valid.
  virtual int SetNthNodeDisplayPosition(int n, int X, int Y);
  virtual int SetNthNodeDisplayPosition(int n, const int pos[2]);
  virtual int SetNthNodeDisplayPosition(int n, const double pos[2]);

  /// Set the nth node's world position. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting) or the world
  /// position is not valid according to the point
  /// placer.
  virtual int SetNthNodeWorldPosition(int n, const double pos[3]);

  /// Get the nth node's slope. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting).
  virtual int  GetNthNodeSlope(int idx, double slope[3]);

  /// For a given node n, get the number of intermediate
  /// points between this node and the node at
  /// (n+1). If n is the last node and the loop is
  /// closed, this is the number of intermediate points
  /// between node n and node 0. 0 is returned if n is
  /// out of range.
  virtual int GetNumberOfIntermediatePoints(int n);

  /// Get the world position of the intermediate point at
  /// index idx between nodes n and (n+1) (or n and 0 if
  /// n is the last node and the loop is closed). Returns
  /// 1 on success or 0 if n or idx are out of range.
  virtual int GetIntermediatePointWorldPosition(int n,
                                                int idx, double point[3]);

  /// Get the display position of the intermediate point at
  /// index idx between nodes n and (n+1) (or n and 0 if
  /// n is the last node and the loop is closed). Returns
  /// 1 on success or 0 if n or idx are out of range.
  virtual int GetIntermediatePointDisplayPosition(int n,
                                                  int idx, double pos[2]);

  /// Add an intermediate point between node n and n+1
  /// (or n and 0 if n is the last node and the loop is closed).
  /// Returns 1 on success or 0 if n is out of range.
  virtual int AddIntermediatePointWorldPosition(int n,
                                                const double point[3]);

  /// Delete the last node. Returns 1 on success or 0 if
  /// there were not any nodes.
  virtual int DeleteLastNode();

  /// Delete the active node. Returns 1 on success or 0 if
  /// the active node did not indicate a valid node.
  virtual int DeleteActiveNode();

  /// Delete the nth node. Return 1 on success or 0 if n
  /// is out of range.
  virtual int DeleteNthNode(int n);

  /// Delete all nodes.
  virtual void ClearAllNodes();

  /// Given a specific X, Y pixel location, add a new node
  /// on the widget at this location.
  virtual int AddNodeOnWidget(int X, int Y);

  /// Specify tolerance for performing pick operations of points
  /// (by the locator, see ActivateNode).
  /// Tolerance is defined in terms of percentage of the handle size.
  /// Default value is 0.5
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);

  /// Set / get the Point Placer. The point placer is
  /// responsible for converting display coordinates into
  /// world coordinates according to some constraints, and
  /// for validating world positions.
  void SetPointPlacer(vtkPointPlacer *);
  vtkPointPlacer* GetPointPlacer();

  /// Set / Get the Line Interpolator. The line interpolator
  /// is responsible for generating the line segments connecting
  /// nodes.
  void SetLineInterpolator(vtkSlicerLineInterpolator *);
  vtkSlicerLineInterpolator* GetLineInterpolator();

  /// Controls whether the widget should always appear on top
  /// of other actors in the scene. (In effect, this will disable OpenGL
  /// Depth buffer tests while rendering the widget).
  /// Default is to set it to false.
  vtkSetMacro(AlwaysOnTop, vtkTypeBool);
  vtkGetMacro(AlwaysOnTop, vtkTypeBool);
  vtkBooleanMacro(AlwaysOnTop, vtkTypeBool);


  /// Handle when rebuilding the locator
  vtkSetMacro(RebuildLocator,bool);

  /// Initialize with poly data
  ///
  //virtual void Initialize(vtkPolyData *);

  /// Set / Get the ClosedLoop value. This ivar indicates whether the widget
  /// forms a closed loop.
  void SetClosedLoop(vtkTypeBool val);
  vtkGetMacro(ClosedLoop, vtkTypeBool);
  vtkBooleanMacro(ClosedLoop, vtkTypeBool);

  /// Set/Get the vtkMRMLMarkipsNode connected with this representation
  virtual void SetMarkupsDisplayNode(vtkMRMLMarkupsDisplayNode *markupsDisplayNode);
  virtual vtkMRMLMarkupsDisplayNode* GetMarkupsDisplayNode();
  virtual vtkMRMLMarkupsNode* GetMarkupsNode();

  /// Compute the centroid by sampling the points along
  /// the polyline of the widget at equal distances.
  /// and it also updates automatically the centroid pos stored in the Markups node
  virtual void UpdateCentroid();

  /// Translation, rotation, scaling will happen around this position
  virtual bool GetTransformationReferencePoint(double referencePointWorld[3]);

  /// Return found component type (as vtkMRMLMarkupsDisplayNode::ComponentType).
  /// closestDistance2 is the squared distance in display coordinates from the closest position where interaction is possible.
  /// componentIndex returns index of the found component (e.g., if control point is found then control point index is returned).
  virtual int CanInteract(const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &componentIndex);

  //@{
  /**
  * This data member is used to keep track of whether to render
  * or not (i.e., to minimize the total number of renders).
  */
  vtkGetMacro(NeedToRender, bool);
  vtkSetMacro(NeedToRender, bool);
  vtkBooleanMacro(NeedToRender, bool);
  //@}

protected:
  vtkSlicerAbstractWidgetRepresentation();
  ~vtkSlicerAbstractWidgetRepresentation() VTK_OVERRIDE;

  // The renderer in which this widget is placed
  vtkWeakPointer<vtkRenderer> Renderer;

  class ControlPointsPipeline
  {
  public:
    ControlPointsPipeline();

    /// Specify the glyph that is displayed at each control point position.
    /// Keep in mind that the shape will be
    /// aligned with the constraining plane by orienting it such that
    /// the x axis of the geometry lies along the normal of the plane.
    //vtkSmartPointer<vtkPolyData> PointMarkerShape;
    vtkSmartPointer<vtkMarkupsGlyphSource2D> GlyphSource2D;
    vtkSmartPointer<vtkSphereSource> GlyphSourceSphere;

    vtkSmartPointer<vtkPolyData> ControlPointsPolyData;
    vtkSmartPointer<vtkPoints> ControlPoints;
    vtkSmartPointer<vtkPolyData> LabelControlPointsPolyData;
    vtkSmartPointer<vtkPoints> LabelControlPoints;
    vtkSmartPointer<vtkPointSetToLabelHierarchy> PointSetToLabelHierarchyFilter;
    vtkSmartPointer<vtkStringArray> Labels;
    vtkSmartPointer<vtkStringArray> LabelsPriority;
    vtkSmartPointer<vtkTextProperty> TextProperty;
  };

  double ControlPointSize;

  /// Helper function to add bounds of all listed actors to the supplied bounding box.
  /// additionalBounds is for convenience only, it allows defining additional bounds.
  void AddActorsBounds(vtkBoundingBox& bounds, const std::vector<vtkProp*> &actors, double* additionalBounds = nullptr);

  vtkWeakPointer<vtkMRMLMarkupsDisplayNode> MarkupsDisplayNode;

  // Selection tolerance for the picking of points
  double Tolerance;
  double PixelTolerance;

  bool NeedToRender;

  vtkSmartPointer<vtkPointPlacer> PointPlacer;
  vtkSmartPointer<vtkSlicerLineInterpolator> LineInterpolator;

  int CurrentOperation;
  vtkTypeBool ClosedLoop;

  virtual void AddNodeAtPositionInternal(const double worldPos[3]);
  virtual void SetNthNodeWorldPositionInternal(int n, const double worldPos[3]);
  virtual void FromWorldOrientToOrientationQuaternion(const double worldOrient[9], double orientation[4]);
  virtual void FromOrientationQuaternionToWorldOrient(const double orientation[4], double worldOrient[9]);

  // Given a world position and orientation, this computes the display position
  // using the renderer of this class.
  void GetRendererComputedDisplayPositionFromWorldPosition(const double worldPos[3],
                                                           double displayPos[2]);

  virtual void UpdateLines(int index);
  void UpdateLine(int idx1, int idx2);

  virtual int FindClosestPointOnWidget(int X, int Y,
                                       double worldPos[3],
                                       int *idx);

  virtual void BuildLines()=0;

  // Utility function to build lines between control points.
  // If displayPosition is true then positions will be computed in display coordinate system,
  // otherwise in world coordinate system.
  // displayPosition is normally set to true in 2D, and to false in 3D representations.
  void BuildLine(vtkPolyData* linePolyData, bool displayPosition);

  // This method is called when something changes in the point placer.
  // It will cause all points to be updated, and all lines to be regenerated.
  // It should be extended to detect changes in the line interpolator too.
  virtual int  UpdateWidget(bool force = false);
  vtkTimeStamp WidgetBuildTime;

  void ComputeMidpoint(double p1[3], double p2[3], double mid[3])
  {
      mid[0] = (p1[0] + p2[0])/2;
      mid[1] = (p1[1] + p2[1])/2;
      mid[2] = (p1[2] + p2[2])/2;
  }

  // Adding a point locator to the representation to speed
  // up lookup of the active node when dealing with large datasets (100k+)
  vtkSmartPointer<vtkIncrementalOctreePointLocator> Locator;

  // Deletes the previous locator if it exists and creates
  // a new locator. Also deletes / recreates the attached data set.
  void ResetLocator();

  // Calculate view scale factor
  double CalculateViewScaleFactor();

  virtual void BuildLocator();

  bool RebuildLocator;

  enum
  {
    Unselected,
    Selected,
    Active,
    NumberOfControlPointTypes
  };

  ControlPointsPipeline* ControlPoints[3]; // Unselected, Selected, Active

  vtkTypeBool AlwaysOnTop;

  // Temporary variable to store GetBounds() result
  double Bounds[6];

private:
  vtkSlicerAbstractWidgetRepresentation(const vtkSlicerAbstractWidgetRepresentation&) = delete;
  void operator=(const vtkSlicerAbstractWidgetRepresentation&) = delete;
};

#endif
