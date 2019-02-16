/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __vtkMRMLMarkupsDisplayableManager_h
#define __vtkMRMLMarkupsDisplayableManager_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManagerHelper.h"

// MRMLDisplayableManager includes
#include <vtkMRMLAbstractDisplayableManager.h>

// VTK includes
#include <vtkSlicerAbstractWidget.h>

#include <set>

class vtkMRMLMarkupsNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;
class vtkAbstractWidget;

/// \ingroup Slicer_QtModules_Markups
class  VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsDisplayableManager :
    public vtkMRMLAbstractDisplayableManager
{
public:

  // Allow the helper to call protected methods of displayable manager
  friend class vtkMRMLMarkupsDisplayableManagerHelper;

  static vtkMRMLMarkupsDisplayableManager *New();
  vtkTypeMacro(vtkMRMLMarkupsDisplayableManager, vtkMRMLAbstractDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Check if this is a 2d SliceView displayable manager, returns true if so,
  /// false otherwise. Checks return from GetSliceNode for non null, which means
  /// it's a 2d displayable manager
  virtual bool Is2DDisplayableManager();
  /// Get the sliceNode, if registered. This would mean it is a 2D SliceView displayableManager.
  vtkMRMLSliceNode * GetMRMLSliceNode();

  /// Check if the displayCoordinates are inside the viewport and if not,
  /// correct the displayCoordinates. Coordinates are reset if the normalized
  /// viewport coordinates are less than 0.001 or greater than 0.999 and are
  /// reset to those values.
  /// If the coordinates have been reset, return true, otherwise return false.
  bool RestrictDisplayCoordinatesToViewport(double* displayCoordinates);

  /// Check if there are real changes between two sets of displayCoordinates
  bool GetDisplayCoordinatesChanged(double * displayCoordinates1, double * displayCoordinates2);

  /// Check if there are real changes between two sets of worldCoordinates
  bool GetWorldCoordinatesChanged(double * worldCoordinates1, double * worldCoordinates2);

  /// Convert display to world coordinates
  void GetDisplayToWorldCoordinates(double x, double y, double * worldCoordinates);
  void GetDisplayToWorldCoordinates(double * displayCoordinates, double * worldCoordinates);

  /// Convert world coordinates to local using mrml parent transform
  virtual void GetWorldToLocalCoordinates(vtkMRMLMarkupsNode *node,
                                  double *worldCoordinates, double *localCoordinates);

  /// Set mrml parent transform to widgets
  virtual void SetParentTransformToWidget(vtkMRMLMarkupsNode *vtkNotUsed(node), vtkAbstractWidget *vtkNotUsed(widget)){}

  vtkMRMLMarkupsDisplayableManagerHelper *  GetHelper() { return this->Helper; };

  /// Checks if this 2D displayable manager is in light box mode. Returns true
  /// if there is a slice node and it has grid columns or rows greater than 1,
  /// and false otherwise.
  bool IsInLightboxMode();

  /// Gets the world coordinate of the markups node point, transforms it to
  /// display coordinates, takes the z element to calculate the light box index.
  /// Returns -1 if not in lightbox mode or the indices are out of range.
  int GetLightboxIndex(vtkMRMLMarkupsNode *node, int pointIndex);

  bool CanProcessInteractionEvent(vtkSlicerInteractionEventData* eventData, double &closestDistance2) VTK_OVERRIDE;
  void ProcessInteractionEvent(vtkSlicerInteractionEventData* eventData) VTK_OVERRIDE;

  void SetHasFocus(bool hasFocus) VTK_OVERRIDE;
  bool GetGrabFocus() VTK_OVERRIDE;
  bool GetInteractive() VTK_OVERRIDE;

  // Updates markup point preview position.
  // Returns true if the event is processed.
  bool UpdatePointPlacePreview(const int displayPosition[2], const double worldPosition[3]);

  // Remove previewed markup point.
  // Returns true if the event is processed.
  bool RemovePointPlacePreview();

  vtkMRMLMarkupsNode* GetActiveMarkupsNode(bool forPlacement=false);

  int GetCurrentInteractionMode();

  // Places a new markup point.
  // Returns true if the event is processed.
  bool PlacePoint(const int displayPosition[2], const double worldPosition[3], std::string associatedNodeID);

  // Methods from vtkMRMLAbstractSliceViewDisplayableManager

  /// Convert device coordinates (display) to XYZ coordinates (viewport).
  /// Parameter \a xyz is double[3]
  /// \sa ConvertDeviceToXYZ(vtkRenderWindowInteractor *, vtkMRMLSliceNode *, double x, double y, double xyz[3])
  void ConvertDeviceToXYZ(double x, double y, double xyz[3]);

protected:

  vtkMRMLMarkupsDisplayableManager();
  virtual ~vtkMRMLMarkupsDisplayableManager();

  vtkSlicerAbstractWidget* FindClosestWidget(vtkSlicerInteractionEventData *callData, double &closestDistance2);

  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData) VTK_OVERRIDE;

  /// Wrap the superclass render request in a check for batch processing
  virtual void RequestRender();

  /// Called from RequestRender method if UpdateFromMRMLRequested is true
  /// \sa RequestRender() SetUpdateFromMRMLRequested()
  virtual void UpdateFromMRML() VTK_OVERRIDE;

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene) VTK_OVERRIDE;

  /// Called after the corresponding MRML event is triggered, from AbstractDisplayableManager
  /// \sa ProcessMRMLSceneEvents
  virtual void UpdateFromMRMLScene() VTK_OVERRIDE;
  virtual void OnMRMLSceneEndClose() VTK_OVERRIDE;
  virtual void OnMRMLSceneEndImport() VTK_OVERRIDE;
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node) VTK_OVERRIDE;
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) VTK_OVERRIDE;

  /// Create a widget.
  vtkSlicerAbstractWidget* CreateWidget(vtkMRMLMarkupsDisplayNode* node);

  /// Called after the corresponding MRML View container was modified
  virtual void OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller) VTK_OVERRIDE;

  /// Handler for specific SliceView actions, iterate over the widgets in the helper
  virtual void OnMRMLSliceNodeModifiedEvent();

  /// Check, if the widget is displayable in the current slice geometry for
  /// this markup, returns true if a 3d displayable manager
  virtual bool IsWidgetDisplayableOnSlice(vtkMRMLMarkupsNode* node);

  /// Observe the interaction node.
  void AddObserversToInteractionNode();
  void RemoveObserversFromInteractionNode();

  /// Check, if the point is displayable in the current slice geometry
  virtual bool IsPointDisplayableOnSlice(vtkMRMLMarkupsNode* node, int pointIndex = 0);

  /// Check, if the point is displayable in the current slice geometry
  virtual bool IsCentroidDisplayableOnSlice(vtkMRMLMarkupsNode* node);

  /// Preset functions for certain events.
  virtual void OnMRMLMarkupsNodeModifiedEvent(vtkMRMLNode* node);
  virtual void OnMRMLMarkupsNodeTransformModifiedEvent(vtkMRMLNode* node);
  virtual void OnMRMLMarkupsNodeLockModifiedEvent(vtkMRMLNode* node);
  virtual void OnMRMLMarkupsDisplayNodeModifiedEvent(vtkMRMLNode *node);
  virtual void OnMRMLMarkupsNthPointModifiedEvent(vtkMRMLNode *node, int n);
  virtual void OnMRMLMarkupsPointAddedEvent(vtkMRMLNode *node, int n);
  virtual void OnMRMLMarkupsPointRemovedEvent(vtkMRMLNode *node, int n);
  virtual void OnMRMLMarkupsAllPointsRemovedEvent(vtkMRMLNode *node);

  /// enum for action at click events
  enum {AddPoint = 0,AddPreview,RemovePreview};

  std::string GetAssociatedNodeID(vtkSlicerInteractionEventData* eventData);

  /// Convert display to world coordinates
  void GetWorldToDisplayCoordinates(double r, double a, double s, double * displayCoordinates);
  void GetWorldToDisplayCoordinates(double * worldCoordinates, double * displayCoordinates);

  /// Convert display to viewport coordinates
  void GetDisplayToViewportCoordinates(double x, double y, double * viewportCoordinates);
  void GetDisplayToViewportCoordinates(double *displayCoordinates, double * viewportCoordinates);

  /// Get the widget of a node.
  vtkSlicerAbstractWidget* GetWidget(vtkMRMLMarkupsDisplayNode * node);

  /// Check if it is the right displayManager
  virtual bool IsCorrectDisplayableManager();

  /// Return true if this displayable manager supports(can manage) that node,
  /// false otherwise.
  /// Can be reimplemented to add more conditions.
  /// \sa IsManageable(const char*), IsCorrectDisplayableManager()
  virtual bool IsManageable(vtkMRMLNode* node);
  /// Return true if this displayable manager supports(can manage) that node class,
  /// false otherwise.
  /// Can be reimplemented to add more conditions.
  /// \sa IsManageable(vtkMRMLNode*), IsCorrectDisplayableManager()
  virtual bool IsManageable(const char* nodeClassName);

  /// Focus of this displayableManager is set to a specific markups type when inherited
  std::set<std::string> Focus;

  /// Respond to interactor style events
  virtual void OnInteractorStyleEvent(int eventid) VTK_OVERRIDE;

  /// Accessor for internal flag that disables interactor style event processing
  vtkGetMacro(DisableInteractorStyleEventsProcessing, int);

  vtkMRMLMarkupsDisplayableManagerHelper * Helper;

  double LastClickWorldCoordinates[4];

  vtkMRMLMarkupsNode* CreateNewMarkupsNode(const std::string &markupsNodeClassName);

  vtkSlicerAbstractWidget* LastActiveWidget;

private:
  vtkMRMLMarkupsDisplayableManager(const vtkMRMLMarkupsDisplayableManager&); /// Not implemented
  void operator=(const vtkMRMLMarkupsDisplayableManager&); /// Not Implemented

  int DisableInteractorStyleEventsProcessing;

  vtkMRMLSliceNode * SliceNode;
};

#endif
