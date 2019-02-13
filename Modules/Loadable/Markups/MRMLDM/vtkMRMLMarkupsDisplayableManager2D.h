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

#ifndef __vtkMRMLMarkupsDisplayableManager2D_h
#define __vtkMRMLMarkupsDisplayableManager2D_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManagerHelper.h"

// MRMLDisplayableManager includes
#include <vtkMRMLAbstractSliceViewDisplayableManager.h>

// VTK includes
#include <vtkSlicerAbstractWidget.h>

#include <set>

class vtkMRMLMarkupsNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;
class vtkAbstractWidget;

/// \ingroup Slicer_QtModules_Markups
class  VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsDisplayableManager2D :
    public vtkMRMLAbstractSliceViewDisplayableManager
{
public:

  static vtkMRMLMarkupsDisplayableManager2D *New();
  vtkTypeMacro(vtkMRMLMarkupsDisplayableManager2D, vtkMRMLAbstractSliceViewDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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

  /// Create a new widget for this markups node and save it to the helper.
  /// Returns widget on success, null on failure.
  vtkSlicerAbstractWidget *AddWidget(vtkMRMLMarkupsDisplayNode *markupsDisplayNode);

  vtkMRMLMarkupsDisplayableManagerHelper *  GetHelper() { return this->Helper; };

  /// Checks if this 2D displayable manager is in light box mode. Returns true
  /// if there is a slice node and it has grid columns or rows greater than 1,
  /// and false otherwise.
  bool IsInLightboxMode();

  /// Gets the world coordinate of the markups node point, transforms it to
  /// display coordinates, takes the z element to calculate the light box index.
  /// Returns -1 if not in lightbox mode or the indices are out of range.
  int GetLightboxIndex(vtkMRMLMarkupsNode *node, int pointIndex);

  bool CanProcessInteractionEvent(vtkEventData* eventData, double &closestDistance2) VTK_OVERRIDE;
  void ProcessInteractionEvent(vtkEventData* eventData) VTK_OVERRIDE;
  void SetHasFocus(bool hasFocus) VTK_OVERRIDE;

protected:

  vtkMRMLMarkupsDisplayableManager2D();
  virtual ~vtkMRMLMarkupsDisplayableManager2D();

  vtkSlicerAbstractWidget* FindClosestWidget(vtkEventData *callData, double &closestDistance2);

  bool UpdatePointPlacePreview(const int displayPosition[2], const double worldPosition[3]);

  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData) VTK_OVERRIDE;

  /// Wrap the superclass render request in a check for batch processing
  virtual void RequestRender();

  /// Remove MRML observers
  virtual void RemoveMRMLObservers() VTK_OVERRIDE;

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

  /// Called after the corresponding MRML View container was modified
  virtual void OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller) VTK_OVERRIDE;

  /// Handler for specific SliceView actions, iterate over the widgets in the helper
  virtual void OnMRMLSliceNodeModifiedEvent() VTK_OVERRIDE;

  /// Check, if the widget is displayable in the current slice geometry for
  /// this markup, returns true if a 3d displayable manager
  virtual bool IsWidgetDisplayableOnSlice(vtkMRMLMarkupsNode* node);

  /// Observe one node
  void SetAndObserveNode(vtkMRMLMarkupsNode *markupsNode);
  /// Observe all associated nodes.
  void SetAndObserveNodes();

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

  /// Get the coordinates of a click in the RenderWindow
  /// It calls OnClickInRenderWindow.
  void OnClickInRenderWindowGetCoordinates();
  /// Callback for click in RenderWindow
  void OnClickInRenderWindow(double x, double y,
                                     const char *associatedNodeID = NULL);

  /// Convert display to world coordinates
  void GetWorldToDisplayCoordinates(double r, double a, double s, double * displayCoordinates);
  void GetWorldToDisplayCoordinates(double * worldCoordinates, double * displayCoordinates);

  /// Convert display to viewport coordinates
  void GetDisplayToViewportCoordinates(double x, double y, double * viewportCoordinates);
  void GetDisplayToViewportCoordinates(double *displayCoordinates, double * viewportCoordinates);

  /// Create a widget.
  vtkSlicerAbstractWidget * CreateWidget(vtkMRMLMarkupsDisplayNode* node);
  /// Gets called when widget was created
  void OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node);
  /// Get the widget of a node.
  vtkAbstractWidget * GetWidget(vtkMRMLMarkupsDisplayNode * node);

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
  vtkMRMLMarkupsDisplayableManager2D(const vtkMRMLMarkupsDisplayableManager2D&); /// Not implemented
  void operator=(const vtkMRMLMarkupsDisplayableManager2D&); /// Not Implemented

  int DisableInteractorStyleEventsProcessing;

  vtkMRMLSliceNode * SliceNode;
};

#endif
