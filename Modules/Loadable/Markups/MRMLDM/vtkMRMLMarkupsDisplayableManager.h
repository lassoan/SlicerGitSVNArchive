/*=auto=========================================================================

 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Program:   3D Slicer

 =========================================================================auto=*/

#ifndef __vtkMRMLMarkupsDisplayableManager_h
#define __vtkMRMLMarkupsDisplayableManager_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MRMLDisplayableManager includes
#include <vtkMRMLAbstractDisplayableManager.h>


// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsClickCounter.h"
#include "vtkMRMLMarkupsDisplayableManagerHelper.h"

// MRMLDisplayableManager includes
#include <vtkMRMLAbstractThreeDViewDisplayableManager.h>

// VTK includes

class vtkMRMLMarkupsNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;



// Slicer includes
class vtkMRMLSliceNode;
class vtkSlicerViewerWidget;

// VTK includes
class vtkMarkupsWidget;
class vtkHandleWidget;

/// \ingroup Slicer_QtModules_Annotation
class VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT
vtkMRMLMarkupsDisplayableManager
  : public vtkMRMLAbstractDisplayableManager
{
public:

  //static vtkMRMLMarkupsDisplayableManager *New();
  vtkTypeMacro(vtkMRMLMarkupsDisplayableManager, vtkMRMLAbstractDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Check if this is a 2d SliceView displayable manager, returns true if so,
  /// false otherwise. Checks return from GetSliceNode for non null, which means
  /// it's a 2d displayable manager
  virtual bool Is2DDisplayableManager();
  /// Get the slice node. If returns non-zero then it is a 2D slice displayable manager.
  vtkMRMLSliceNode * GetMRMLSliceNode();
  /// Get the 3D view node. If returns non-zero then it is a 3D displayable manager.
  vtkMRMLViewNode * GetMRMLViewNode();

  /// Hide/Show a widget so that the node's display node visibility setting
  /// matches that of the widget
  void UpdateWidgetVisibility(vtkMRMLMarkupsNode* node);

  // the following functions must be public to be accessible by the callback
  /// Propagate properties of MRML node to widget.
  virtual void PropagateMRMLToWidget(vtkMRMLMarkupsNode* node, vtkMarkupsWidget * widget);
  /// Propagate properties of widget to MRML node.
  virtual void PropagateWidgetToMRML(vtkMarkupsWidget * widget, vtkMRMLMarkupsNode* node);
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
  virtual void SetParentTransformToWidget(vtkMRMLMarkupsNode *vtkNotUsed(node), vtkMarkupsWidget *vtkNotUsed(widget)){};


  /// Set/Get the 2d scale factor to divide 3D scale by to show 2D elements appropriately (usually set to 300)
  vtkSetMacro(ScaleFactor2D, double);
  vtkGetMacro(ScaleFactor2D, double);

  /// Return true if in lightbox mode - there is a slice node that has layout
  /// grid columns or rows > 1
  bool IsInLightboxMode();

  int  GetLightboxIndex(vtkMRMLMarkupsNode *node, int markupIndex, int pointIndex);

  bool IsWidgetDisplayableOnSlice(vtkMRMLMarkupsNode* node, int markupIndex);

  bool RestrictDisplayCoordinatesToViewport(double* displayCoordinates);

  /// Create a new widget for this markups node and save it to the helper.
  /// Returns widget on success, null on failure.
  vtkMarkupsWidget *AddWidget(vtkMRMLMarkupsNode *markupsNode);

  //  vtkMRMLMarkupsDisplayableManagerHelper *  GetHelper() { return this->Helper; };
  vtkGetObjectMacro(Helper, vtkMRMLMarkupsDisplayableManagerHelper);

  bool UpdateNthMarkupPositionFromWidget(int n, vtkMRMLMarkupsNode* pointsNode, vtkMarkupsWidget * widget);

protected:

  vtkMRMLMarkupsDisplayableManager();
  virtual ~vtkMRMLMarkupsDisplayableManager();

  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData) VTK_OVERRIDE;

  //  virtual void Create();

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
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node) VTK_OVERRIDE;
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) VTK_OVERRIDE;

  /// Called after the corresponding MRML View container was modified
  virtual void OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller) VTK_OVERRIDE;

  virtual void OnMRMLSelectionNodeUnitModifiedEvent(vtkMRMLSelectionNode*) {}

  /// Handler for specific SliceView actions
  virtual void OnMRMLSliceNodeModifiedEvent();

  /// Observe one node
  void SetAndObserveNode(vtkMRMLMarkupsNode *markupsNode);
  /// Observe all associated nodes.
  void SetAndObserveNodes();

  /// Observe the interaction node.
  void AddObserversToInteractionNode();
  void RemoveObserversFromInteractionNode();

  /// Observe the selection node for:
  ///    * vtkMRMLSelectionNode::UnitModifiedEvent
  /// events to update the unit of the annotation nodes.
  /// \sa RemoveObserversFromSelectionNode(), AddObserversToInteractionNode(),
  /// OnMRMLSelectionNodeUnitModifiedEvent()
  void AddObserversToSelectionNode();
  void RemoveObserversFromSelectionNode();

  /// Preset functions for certain events.
  void OnMRMLMarkupsNodeModifiedEvent(vtkMRMLNode* node);
  void OnMRMLMarkupsNodeTransformModifiedEvent(vtkMRMLNode* node);
  void OnMRMLMarkupsNodeLockModifiedEvent(vtkMRMLNode* node);
  void OnMRMLMarkupsDisplayNodeModifiedEvent(vtkMRMLNode *node);
  void OnMRMLMarkupsPointModifiedEvent(vtkMRMLNode *node, int n);
  /// Subclasses need to react to new markups being added to or removed
  /// from a markups node or modified
  virtual void OnMRMLMarkupsNodeMarkupAddedEvent(vtkMRMLMarkupsNode * vtkNotUsed(markupsNode), int vtkNotUsed(n)) {};
  virtual void OnMRMLMarkupsNodeMarkupRemovedEvent(vtkMRMLMarkupsNode * vtkNotUsed(markupsNode), int vtkNotUsed(n)) {};
  virtual void OnMRMLMarkupsNodeNthMarkupModifiedEvent(vtkMRMLMarkupsNode* vtkNotUsed(node), int vtkNotUsed(n)) {};

  //
  // Handling of interaction within the RenderWindow
  //

  // Get the coordinates of a click in the RenderWindow
  void OnClickInRenderWindowGetCoordinates();
  /// Callback for click in RenderWindow
  virtual void OnClickInRenderWindow(double x, double y, const char *associatedNodeID = NULL);
  /// Counter for clicks in Render Window
  vtkMRMLMarkupsClickCounter* ClickCounter;

  /// Update a single handle from markup position, implemented by the subclasses, return
  /// true if the position changed
  virtual bool UpdateNthHandlePositionFromMRML(int n, vtkMarkupsWidget *widget, vtkMRMLMarkupsNode* markupsNode) = 0;

  /// Update just the position for the widget, implemented by subclasses.
  virtual void UpdatePosition(vtkMarkupsWidget *widget, vtkMRMLMarkupsNode* markupsNode) = 0;

  //
  // Handles for widget placement
  //

  /// Place a handle for widgets
  virtual void PlaceHandle(double x, double y);
  /// Return the placed handles
  vtkHandleWidget * GetHandle(int index);

  //
  // Coordinate Conversions
  //

  /// Convert display to world coordinates
  void GetWorldToDisplayCoordinates(double r, double a, double s, double * displayCoordinates);
  void GetWorldToDisplayCoordinates(double * worldCoordinates, double * displayCoordinates);

  /// Convert display to viewport coordinates
  void GetDisplayToViewportCoordinates(double x, double y, double * viewportCoordinates);
  void GetDisplayToViewportCoordinates(double *displayCoordinates, double * viewportCoordinates);

  //
  // Widget functionality
  //

  /// Create a widget.
  virtual vtkMarkupsWidget * CreateWidget(vtkMRMLMarkupsNode* node);
  /// Gets called when widget was created
  virtual void OnWidgetCreated(vtkMarkupsWidget * widget, vtkMRMLMarkupsNode * node);
  /// Get the widget of a node.
  vtkMarkupsWidget * GetWidget(vtkMRMLMarkupsNode * node);

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

  /// Helper function to return active place node from selection node
  const char* GetActivePlaceNodeClassName();

  /// Focus of this displayableManager is set to a specific markups type when inherited
  const char* Focus;

  /// Disable processing when updating is in progress.
  int Updating;

  /// Respond to interactor style events
  virtual void OnInteractorStyleEvent(int eventid) VTK_OVERRIDE;

  /// Accessor for internal flag that disables interactor style event processing
  vtkGetMacro(DisableInteractorStyleEventsProcessing, int);

  vtkMRMLMarkupsDisplayableManagerHelper * Helper;

  double LastClickWorldCoordinates[4];

private:

  vtkMRMLMarkupsDisplayableManager(const vtkMRMLMarkupsDisplayableManager&); /// Not implemented
  void operator=(const vtkMRMLMarkupsDisplayableManager&); /// Not Implemented

  /// Scale factor for 2d windows
  double ScaleFactor2D;

  int DisableInteractorStyleEventsProcessing;
};

#endif
