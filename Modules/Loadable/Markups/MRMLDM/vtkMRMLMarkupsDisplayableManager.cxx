#include "vtkMRMLMarkupsDisplayableManager.h"

#include "vtkMRMLAbstractSliceViewDisplayableManager.h"

// MarkupsModule/MRML includes
#include <vtkMRMLMarkupsAngleNode.h>
#include <vtkMRMLMarkupsClosedCurveNode.h>
#include <vtkMRMLMarkupsCurveNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLMarkupsLineNode.h>
#include <vtkMRMLMarkupsNode.h>

// MarkupsModule/VTKWidgets includes
#include <vtkSlicerLineWidget.h>
#include <vtkSlicerLineRepresentation2D.h>
#include <vtkSlicerLineRepresentation3D.h>
#include <vtkSlicerClosedCurveWidget.h>
#include <vtkSlicerCurveWidget.h>
#include <vtkSlicerCurveRepresentation2D.h>
#include <vtkSlicerCurveRepresentation3D.h>
#include <vtkSlicerAngleWidget.h>
#include <vtkSlicerAngleRepresentation2D.h>
#include <vtkSlicerAngleRepresentation3D.h>
#include <vtkSlicerPointsWidget.h>
#include <vtkSlicerPointsRepresentation2D.h>
#include <vtkSlicerPointsRepresentation3D.h>

// MRMLDisplayableManager includes
#include <vtkMRMLDisplayableManagerGroup.h>
#include <vtkMRMLInteractionEventData.h>
#include <vtkMRMLModelDisplayableManager.h>

// MRML includes
#include <vtkMRMLApplicationLogic.h>
#include <vtkEventBroker.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkAbstractWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkGeneralTransform.h>
#include <vtkMarkupsGlyphSource2D.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPropCollection.h>
#include <vtkProperty2D.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSlicerAbstractWidgetRepresentation2D.h>
#include <vtkSlicerAbstractWidget.h>
#include <vtkSphereSource.h>
#include <vtkTextProperty.h>
#include <vtkWidgetRepresentation.h>

// STD includes
#include <algorithm>
#include <map>
#include <vector>
#include <sstream>
#include <string>

/*

#include <vtkMarkupsGlyphSource2D.h>

// VTK includes
#include <vtkFollower.h>
#include <vtkHandleRepresentation.h>
#include <vtkInteractorStyle.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOrientedPolygonalHandleRepresentation3D.h>
#include <vtkPickingManager.h>
#include <vtkPointHandleRepresentation2D.h>
#include <vtkProperty2D.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>


*/

typedef void (*fp)(void);

#define NUMERIC_ZERO 0.001

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsDisplayableManager);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManager::vtkMRMLMarkupsDisplayableManager()
{
  this->Focus.insert("vtkMRMLMarkupsAngleNode");
  this->Focus.insert("vtkMRMLMarkupsFiducialNode");
  this->Focus.insert("vtkMRMLMarkupsLineNode");
  this->Focus.insert("vtkMRMLMarkupsCurveNode");
  this->Focus.insert("vtkMRMLMarkupsClosedCurveNode");

  this->Helper = vtkMRMLMarkupsDisplayableManagerHelper::New();
  this->Helper->SetDisplayableManager(this);
  this->DisableInteractorStyleEventsProcessing = 0;

  this->Focus.insert("vtkMRMLMarkupsNode");

  // by default, this displayableManager handles a 2d view, so the SliceNode
  // must be set when it's assigned to a viewer
  this->SliceNode = nullptr;

  this->LastClickWorldCoordinates[0]=0.0;
  this->LastClickWorldCoordinates[1]=0.0;
  this->LastClickWorldCoordinates[2]=0.0;
  this->LastClickWorldCoordinates[3]=1.0;
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManager::~vtkMRMLMarkupsDisplayableManager()
{
  this->DisableInteractorStyleEventsProcessing = 0;

  this->Helper->Delete();

  this->SliceNode = nullptr;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DisableInteractorStyleEventsProcessing = " << this->DisableInteractorStyleEventsProcessing << std::endl;
  if (this->SliceNode &&
      this->SliceNode->GetID())
    {
    os << indent << "Slice node id = " << this->SliceNode->GetID() << std::endl;
    }
  else
    {
    os << indent << "No slice node" << std::endl;
    }
}

//---------------------------------------------------------------------------
vtkMRMLSliceNode * vtkMRMLMarkupsDisplayableManager::GetMRMLSliceNode()
{
  return vtkMRMLSliceNode::SafeDownCast(this->GetMRMLDisplayableNode());
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::Is2DDisplayableManager()
{
  return this->GetMRMLSliceNode() != 0;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::RequestRender()
{
  if (!this->GetMRMLScene())
    {
    return;
    }
  if (!this->GetMRMLScene()->IsBatchProcessing())
    {
    this->Superclass::RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::UpdateFromMRML()
{
  // this gets called from RequestRender, so make sure to jump out quickly if possible
  if (this->GetMRMLScene() == nullptr || this->Focus.empty())
    {
    return;
    }

  // turn off update from mrml requested, as we're doing it now, and create
  // widget requests a render which checks this flag before calling update
  // from mrml again
  this->SetUpdateFromMRMLRequested(0);

  std::vector<vtkMRMLNode*> markupNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLMarkupsNode", markupNodes);
  for (std::vector< vtkMRMLNode* >::iterator nodeIt = markupNodes.begin(); nodeIt != markupNodes.end(); ++nodeIt)
  {
    vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(*nodeIt);
    if (!markupsNode)
    {
      continue;
    }
    if (this->GetHelper()->MarkupsNodes.find(markupsNode) != this->GetHelper()->MarkupsNodes.end())
    {
      // node added already
      continue;
    }
    this->GetHelper()->AddMarkupsNode(markupsNode);
  }

  std::vector<vtkMRMLNode*> markupsDisplayNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLMarkupsDisplayNode", markupsDisplayNodes);
  for (std::vector< vtkMRMLNode* >::iterator nodeIt = markupsDisplayNodes.begin(); nodeIt != markupsDisplayNodes.end(); ++nodeIt)
  {
    vtkMRMLMarkupsDisplayNode *markupsDisplayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(*nodeIt);
    if (!markupsDisplayNode)
    {
      continue;
    }
    if (this->GetHelper()->MarkupsDisplayNodesToWidgets.find(markupsDisplayNode) != this->GetHelper()->MarkupsDisplayNodesToWidgets.end())
    {
      // node added already
      continue;
    }
    this->GetHelper()->AddDisplayNode(markupsDisplayNode);
  }

  // Remove observed markups nodes that have been deleted from the scene
  for (vtkMRMLMarkupsDisplayableManagerHelper::MarkupsNodesIt markupsIterator = this->Helper->MarkupsNodes.begin();
    markupsIterator != this->Helper->MarkupsNodes.end();
    /*upon deletion the increment is done already, so don't increment here*/)
  {
    vtkMRMLMarkupsNode *markupsNode = *markupsIterator;
    if (this->GetMRMLScene()->IsNodePresent(markupsNode))
    {
      ++markupsIterator;
    }
    else
    {
      // display node is not in the scene anymore, delete the widget
      vtkMRMLMarkupsDisplayableManagerHelper::MarkupsNodesIt markupsIteratorToRemove = markupsIterator;
      ++markupsIterator;
      this->Helper->RemoveMarkupsNode(*markupsIteratorToRemove);
      this->Helper->MarkupsNodes.erase(markupsIteratorToRemove);
    }
  }

  // Remove widgets corresponding deleted display nodes
  for (vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt widgetIterator = this->Helper->MarkupsDisplayNodesToWidgets.begin();
    widgetIterator != this->Helper->MarkupsDisplayNodesToWidgets.end();
    /*upon deletion the increment is done already, so don't increment here*/)
  {
    vtkMRMLMarkupsDisplayNode *markupsDisplayNode = widgetIterator->first;
    if (this->GetMRMLScene()->IsNodePresent(markupsDisplayNode))
    {
      ++widgetIterator;
    }
    else
    {
      // display node is not in the scene anymore, delete the widget
      vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt widgetIteratorToRemove = widgetIterator;
      ++widgetIterator;
      vtkSlicerAbstractWidget* widgetToRemove = widgetIteratorToRemove->second;
      this->Helper->DeleteWidget(widgetToRemove);
      this->Helper->MarkupsDisplayNodesToWidgets.erase(widgetIteratorToRemove);
    }
  }

}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  Superclass::SetMRMLSceneInternal(newScene);

  // after a new scene got associated, we want to make sure everything old is gone
  this->OnMRMLSceneEndClose();

  if (newScene)
    {
    this->AddObserversToInteractionNode();
    }
  else
    {
    // there's no scene to get the interaction node from, so this won't do anything
    this->RemoveObserversFromInteractionNode();
    }
  vtkDebugMacro("SetMRMLSceneInternal: add observer on interaction node now?");

  // clear out the map of glyph types
  //this->Helper->ClearNodeGlyphTypes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager
::ProcessMRMLNodesEvents(vtkObject *caller,unsigned long event,void *callData)
{
  vtkMRMLMarkupsNode * markupsNode = vtkMRMLMarkupsNode::SafeDownCast(caller);
  vtkMRMLInteractionNode * interactionNode = vtkMRMLInteractionNode::SafeDownCast(caller);
  if (markupsNode)
    {
    bool renderRequested = false;

    for (int displayNodeIndex = 0; displayNodeIndex < markupsNode->GetNumberOfDisplayNodes(); displayNodeIndex++)
      {
      vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(displayNodeIndex));
      vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(displayNode);
      if (!widget)
        {
        // if a new display node is added or display node view node IDs are changed then we may need to create a new widget
        this->Helper->AddDisplayNode(displayNode);
        widget = this->Helper->GetWidget(displayNode);
        }
      if (!widget)
        {
        continue;
        }
      widget->UpdateFromMRML(markupsNode, event, callData);
      if (widget->GetNeedToRender())
        {
        renderRequested = true;
        widget->NeedToRenderOff();
        }
      }

    if (renderRequested)
      {
      this->RequestRender();
      }
    }
  else if (interactionNode)
    {
    if (event == vtkMRMLInteractionNode::InteractionModeChangedEvent)
      {
      // loop through all widgets and update the widget status
      for (vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt widgetIterator = this->Helper->MarkupsDisplayNodesToWidgets.begin();
        widgetIterator != this->Helper->MarkupsDisplayNodesToWidgets.end(); ++widgetIterator)
        {
        vtkSlicerAbstractWidget* widget = widgetIterator->second;
        if (!widget)
          {
          continue;
          }
        widget->Leave();
        }
      }
    }
  else
    {
    this->Superclass::ProcessMRMLNodesEvents(caller, event, callData);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLSceneEndClose()
{
  vtkDebugMacro("OnMRMLSceneEndClose: remove observers?");
  // run through all nodes and remove node and widget
  this->Helper->RemoveAllWidgetsAndNodes();

  this->SetUpdateFromMRMLRequested(1);
  this->RequestRender();

}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLSceneEndImport()
{
  this->SetUpdateFromMRMLRequested(1);
  this->UpdateFromMRMLScene();
  //this->Helper->SetAllWidgetsToManipulate();
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::UpdateFromMRMLScene()
{
  if (this->GetMRMLSliceNode())
    {
    this->UpdateFromMRML();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !this->GetMRMLScene())
    {
    return;
    }

  // if the scene is still updating, jump out
  if (this->GetMRMLScene()->IsBatchProcessing())
    {
    this->SetUpdateFromMRMLRequested(1);
    return;
    }

  if (node->IsA("vtkMRMLInteractionNode"))
    {
    this->AddObserversToInteractionNode();
    return;
    }

  if (node->IsA("vtkMRMLMarkupsNode"))
  {
    this->Helper->AddMarkupsNode(vtkMRMLMarkupsNode::SafeDownCast(node));
  }

  // and render again
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::AddObserversToInteractionNode()
{
  if (!this->GetMRMLScene())
    {
    return;
    }
  // also observe the interaction node for changes
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode)
    {
    vtkDebugMacro("AddObserversToInteractionNode: interactionNode found");
    vtkNew<vtkIntArray> interactionEvents;
    interactionEvents->InsertNextValue(vtkMRMLInteractionNode::InteractionModeChangedEvent);
    interactionEvents->InsertNextValue(vtkMRMLInteractionNode::InteractionModePersistenceChangedEvent);
    interactionEvents->InsertNextValue(vtkMRMLInteractionNode::EndPlacementEvent);
    vtkObserveMRMLNodeEventsMacro(interactionNode, interactionEvents.GetPointer());
    }
  else { vtkDebugMacro("AddObserversToInteractionNode: No interaction node!"); }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::RemoveObserversFromInteractionNode()
{
  if (!this->GetMRMLScene())
    {
    return;
    }

  // find the interaction node
  vtkMRMLInteractionNode *interactionNode =  this->GetInteractionNode();
  if (interactionNode)
    {
    vtkUnObserveMRMLNodeMacro(interactionNode);
  }
}


//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  bool modified = false;

  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (markupsNode)
  {
    this->Helper->RemoveMarkupsNode(markupsNode);
    modified = true;
  }

  vtkMRMLMarkupsDisplayNode *markupsDisplayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(node);
  if (markupsDisplayNode)
    {
    this->Helper->RemoveDisplayNode(markupsDisplayNode);
    modified = true;
    }

  if (modified)
  {
    this->RequestRender();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller)
{
  vtkDebugMacro("OnMRMLDisplayableNodeModifiedEvent");

  if (!caller)
    {
    vtkErrorMacro("OnMRMLDisplayableNodeModifiedEvent: Could not get caller.")
    return;
    }

  vtkMRMLSliceNode * sliceNode = vtkMRMLSliceNode::SafeDownCast(caller);
  if (sliceNode)
    {
    // the associated renderWindow is a 2D SliceView
    // this is the entry point for all events fired by one of the three sliceviews
    // (e.g. change slice number, zoom etc.)

    // we remember that this instance of the displayableManager deals with 2D
    // this is important for widget creation etc. and save the actual SliceNode
    // because during Slicer startup the SliceViews fire events, it will be always set correctly
    this->SliceNode = sliceNode;

    // now we call the handle for specific sliceNode actions
    this->OnMRMLSliceNodeModifiedEvent();

    // and exit
    return;
    }

  vtkMRMLViewNode * viewNode = vtkMRMLViewNode::SafeDownCast(caller);
  if (viewNode)
    {
    // the associated renderWindow is a 3D View
    vtkDebugMacro("OnMRMLDisplayableNodeModifiedEvent: This displayableManager handles a ThreeD view.")
    return;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLSliceNodeModifiedEvent()
{
  bool renderRequested = false;

  // run through all markup nodes in the helper
  vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt it
    = this->Helper->MarkupsDisplayNodesToWidgets.begin();
  while(it != this->Helper->MarkupsDisplayNodesToWidgets.end())
    {
    // we loop through all widgets
    vtkMRMLMarkupsDisplayNode * markupsDisplayNode = (it->first);
    vtkSlicerAbstractWidget* widget = (it->second);
    widget->UpdateFromMRML(this->SliceNode, vtkCommand::ModifiedEvent);
    if (widget->GetNeedToRender())
      {
      renderRequested = true;
      widget->NeedToRenderOff();
      }
    ++it;
    }

  if (renderRequested)
  {
    this->RequestRender();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnInteractorStyleEvent(int eventid)
{
  Superclass::OnInteractorStyleEvent(eventid);

}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget* vtkMRMLMarkupsDisplayableManager::GetWidget(vtkMRMLMarkupsDisplayNode * node)
{
  return this->Helper->GetWidget(node);
}

//---------------------------------------------------------------------------
std::string vtkMRMLMarkupsDisplayableManager::GetAssociatedNodeID(vtkMRMLInteractionEventData* eventData)
{
  if (eventData)
  {
    double windowWidth = this->GetInteractor()->GetRenderWindow()->GetSize()[0];
    double windowHeight = this->GetInteractor()->GetRenderWindow()->GetSize()[1];
    const int* displayPosition = eventData->GetDisplayPosition();
    if (displayPosition[0] < windowWidth || displayPosition[1] < windowHeight)
    {
      return "";
    }
  }

  // is there a volume in the background?
  if (!this->GetMRMLSliceNode())
  {
    // this only works for slice views for now
    return "";
  }
  // find the slice composite node in the scene with the matching layout name
  vtkMRMLSliceLogic *sliceLogic = nullptr;
  vtkMRMLSliceCompositeNode* sliceCompositeNode = nullptr;
  vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
  if (mrmlAppLogic)
    {
    sliceLogic = mrmlAppLogic->GetSliceLogic(this->GetMRMLSliceNode());
    }
  if (sliceLogic)
    {
    sliceCompositeNode = sliceLogic->GetSliceCompositeNode(this->GetMRMLSliceNode());
    }
  if (!sliceCompositeNode)
    {
    return "";
    }
  if (sliceCompositeNode->GetBackgroundVolumeID())
    {
    return sliceCompositeNode->GetBackgroundVolumeID();
    }
  else if (sliceCompositeNode->GetForegroundVolumeID())
    {
    return sliceCompositeNode->GetForegroundVolumeID();
    }
  else if (sliceCompositeNode->GetLabelVolumeID())
    {
    return sliceCompositeNode->GetLabelVolumeID();
    }
  return "";
}

//---------------------------------------------------------------------------
// Coordinate conversions
//---------------------------------------------------------------------------
/// Convert display to world coordinates
void vtkMRMLMarkupsDisplayableManager::GetDisplayToWorldCoordinates(double x, double y, double * worldCoordinates)
{

  // we will get the transformation matrix to convert display coordinates to RAS

  vtkRenderer* pokedRenderer = this->GetInteractor()->
    FindPokedRenderer(static_cast<int> (x), static_cast<int> (y));

  vtkMatrix4x4 * xyToRasMatrix = this->GetMRMLSliceNode()->GetXYToRAS();

  double displayCoordinates[4];
  displayCoordinates[0] = x - pokedRenderer->GetOrigin()[0];
  displayCoordinates[1] = y - pokedRenderer->GetOrigin()[1];
  displayCoordinates[2] = 0;
  displayCoordinates[3] = 1;

  xyToRasMatrix->MultiplyPoint(displayCoordinates, worldCoordinates);

}

//---------------------------------------------------------------------------
/// Convert display to world coordinates
void vtkMRMLMarkupsDisplayableManager::GetDisplayToWorldCoordinates(double * displayCoordinates, double * worldCoordinates)
{

  this->GetDisplayToWorldCoordinates(displayCoordinates[0], displayCoordinates[1], worldCoordinates);

}


//---------------------------------------------------------------------------
// Convert display to viewport coordinates
void vtkMRMLMarkupsDisplayableManager::GetDisplayToViewportCoordinates(double x, double y, double * viewportCoordinates)
{

  if (viewportCoordinates == nullptr)
    {
    return;
    }
  if (!this->GetInteractor())
    {
    vtkErrorMacro("GetDisplayToViewportCoordinates: No interactor!");
    return;
    }

  double displayCoordinates[4];
  this->ConvertDeviceToXYZ(x, y, displayCoordinates);
  displayCoordinates[3] = 1;

  double windowWidth = this->GetInteractor()->GetRenderWindow()->GetSize()[0];
  double windowHeight = this->GetInteractor()->GetRenderWindow()->GetSize()[1];
  if (windowWidth != 0.0)
    {
    viewportCoordinates[0] = displayCoordinates[0]/windowWidth;
    }
  if (windowHeight != 0.0)
    {
    viewportCoordinates[1] = displayCoordinates[1]/windowHeight;
    }
  vtkDebugMacro("GetDisplayToViewportCoordinates: x = " << x << ", y = " << y
                << ", display coords calc as "
                << displayCoordinates[0] << ", " << displayCoordinates[1]
                << ", returning viewport = "
                << viewportCoordinates[0] << ", " << viewportCoordinates[1]);
}

//---------------------------------------------------------------------------
/// Convert display to viewport coordinates
void vtkMRMLMarkupsDisplayableManager::GetDisplayToViewportCoordinates(double * displayCoordinates, double * viewportCoordinates)
{
  if (displayCoordinates && viewportCoordinates)
    {
    this->GetDisplayToViewportCoordinates(displayCoordinates[0], displayCoordinates[1], viewportCoordinates);
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::RestrictDisplayCoordinatesToViewport(double* displayCoordinates)
{
  double coords[2] = {displayCoordinates[0], displayCoordinates[1]};
  bool restricted = false;

  vtkRenderer* pokedRenderer = this->GetInteractor()->
    FindPokedRenderer(static_cast<int> (coords[0]), static_cast<int> (coords[1]));
  if (!pokedRenderer)
    {
    vtkErrorMacro("RestrictDisplayCoordinatesToViewport: Could not find the poked renderer!")
    return restricted;
    }

  pokedRenderer->DisplayToNormalizedDisplay(coords[0],coords[1]);
  pokedRenderer->NormalizedDisplayToViewport(coords[0],coords[1]);
  pokedRenderer->ViewportToNormalizedViewport(coords[0],coords[1]);

  if (coords[0]<0.001)
    {
    coords[0] = 0.001;
    restricted = true;
    }
  else if (coords[0]>0.999)
    {
    coords[0] = 0.999;
    restricted = true;
    }

  if (coords[1]<0.001)
    {
    coords[1] = 0.001;
    restricted = true;
    }
  else if (coords[1]>0.999)
    {
    coords[1] = 0.999;
    restricted = true;
    }

  pokedRenderer->NormalizedViewportToViewport(coords[0],coords[1]);
  pokedRenderer->ViewportToNormalizedDisplay(coords[0],coords[1]);
  pokedRenderer->NormalizedDisplayToDisplay(coords[0],coords[1]);

  displayCoordinates[0] = coords[0];
  displayCoordinates[1] = coords[1];

  return restricted;
}

//---------------------------------------------------------------------------
/// Check if there are real changes between two sets of displayCoordinates
bool vtkMRMLMarkupsDisplayableManager::GetDisplayCoordinatesChanged(double * displayCoordinates1, double * displayCoordinates2)
{
  bool changed = false;

  if (sqrt((displayCoordinates1[0] - displayCoordinates2[0]) * (displayCoordinates1[0] - displayCoordinates2[0])
           + (displayCoordinates1[1] - displayCoordinates2[1]) * (displayCoordinates1[1] - displayCoordinates2[1]))>1.0)
    {
    changed = true;
    }
  else
    {
    // if in lightbox mode, the third element in the vector may have changed
    if (this->IsInLightboxMode())
      {
      // one of the arguments may be coming from a widget, the other should be
      // the index into the light box array
      double dist = sqrt((displayCoordinates1[2] - displayCoordinates2[2]) * (displayCoordinates1[2] - displayCoordinates2[2]));
      if (dist > 1.0)
        {
        changed = true;
        }
      }
    }
  return changed;
}

//---------------------------------------------------------------------------
/// Check if there are real changes between two sets of displayCoordinates
bool vtkMRMLMarkupsDisplayableManager::GetWorldCoordinatesChanged(double * worldCoordinates1, double * worldCoordinates2)
{
  bool changed = false;

  double distance = sqrt(vtkMath::Distance2BetweenPoints(worldCoordinates1,worldCoordinates2));

  // TODO find a better value?
  // - use a smaller number to make fiducial seeding more smooth
  if (distance > VTK_DBL_EPSILON)
    {
    changed = true;
    }

  return changed;
}

//---------------------------------------------------------------------------
/// Check if it is the correct displayableManager
//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::IsCorrectDisplayableManager()
{
  vtkMRMLSelectionNode *selectionNode = this->GetMRMLApplicationLogic()->GetSelectionNode();
  if (selectionNode == nullptr)
    {
    vtkErrorMacro ("IsCorrectDisplayableManager: No selection node in the scene.");
    return false;
    }
  if (selectionNode->GetActivePlaceNodeClassName() == nullptr)
    {
    return false;
    }
  // the purpose of the displayableManager is hardcoded
  return this->IsManageable(selectionNode->GetActivePlaceNodeClassName());

}
//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::IsManageable(vtkMRMLNode* node)
{
  return (this->Focus.find(node->GetClassName()) != this->Focus.end());
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::IsManageable(const char* nodeClassName)
{
  return nodeClassName && (this->Focus.find(nodeClassName) != this->Focus.end());
}

//---------------------------------------------------------------------------
/// Convert world coordinates to local using mrml parent transform
void vtkMRMLMarkupsDisplayableManager::GetWorldToLocalCoordinates(vtkMRMLMarkupsNode *node,
                                                                     double *worldCoordinates,
                                                                     double *localCoordinates)
{
  if (node == nullptr)
    {
    vtkErrorMacro("GetWorldToLocalCoordinates: node is null");
    return;
    }

  for (int i  =0; i < 3; i++)
    {
    localCoordinates[i] = worldCoordinates[i];
    }

  vtkMRMLTransformNode* tnode = node->GetParentTransformNode();

  vtkGeneralTransform *transformToWorld = vtkGeneralTransform::New();
  transformToWorld->Identity();
  if (tnode != nullptr && !tnode->IsTransformToWorldLinear())
    {
    tnode->GetTransformFromWorld(transformToWorld);
    }
  else if (tnode != nullptr && tnode->IsTransformToWorldLinear())
    {
    vtkNew<vtkMatrix4x4> matrixTransformToWorld;
    matrixTransformToWorld->Identity();
    tnode->GetMatrixTransformToWorld(matrixTransformToWorld.GetPointer());
    matrixTransformToWorld->Invert();
    transformToWorld->Concatenate(matrixTransformToWorld.GetPointer());
  }
  double p[4];
  p[3] = 1;
  int i;
  for (i=0; i<3; i++)
    {
    p[i] = worldCoordinates[i];
    }
  double *xyz = transformToWorld->TransformDoublePoint(p);
  for (i=0; i<3; i++)
    {
    localCoordinates[i] = xyz[i];
    }
  transformToWorld->Delete();
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::IsInLightboxMode()
{
  bool flag = false;
  vtkMRMLSliceNode *sliceNode = this->GetMRMLSliceNode();
  if (sliceNode)
    {
    int numberOfColumns = sliceNode->GetLayoutGridColumns();
    int numberOfRows = sliceNode->GetLayoutGridRows();
    if (numberOfColumns > 1 ||
        numberOfRows > 1)
      {
      flag = true;
      }
    }
  return flag;
}












//---------------------------------------------------------------------------
vtkMRMLMarkupsNode* vtkMRMLMarkupsDisplayableManager::CreateNewMarkupsNode(
  const std::string &markupsNodeClassName)
{
  // create the MRML node
  std::string nodeName = "M";
  if (markupsNodeClassName == "vtkMRMLMarkupsFiducialNode")
  {
    nodeName = "F";
  }
  else if (markupsNodeClassName == "vtkMRMLMarkupsAngleNode")
  {
    nodeName = "A";
  }
  else if (markupsNodeClassName == "vtkMRMLMarkupsLineNode")
  {
    nodeName = "L";
  }
  else if (markupsNodeClassName == "vtkMRMLMarkupsCurveNode")
  {
    nodeName = "C";
  }
  else if (markupsNodeClassName == "vtkMRMLMarkupsClosedCurveNode")
  {
    nodeName = "C";
  }
  vtkMRMLMarkupsNode* markupsNode = vtkMRMLMarkupsNode::SafeDownCast(
    this->GetMRMLScene()->AddNewNodeByClass(markupsNodeClassName, nodeName));
  markupsNode->AddDefaultStorageNode();
  markupsNode->CreateDefaultDisplayNodes();
  return markupsNode;
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget* vtkMRMLMarkupsDisplayableManager::FindClosestWidget(vtkMRMLInteractionEventData *callData, double &closestDistance2)
{
  vtkSlicerAbstractWidget* closestWidget = NULL;
  closestDistance2 = VTK_DOUBLE_MAX;

  for (vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt widgetIterator = this->Helper->MarkupsDisplayNodesToWidgets.begin();
    widgetIterator != this->Helper->MarkupsDisplayNodesToWidgets.end(); ++widgetIterator)
    {
    vtkSlicerAbstractWidget* widget = widgetIterator->second;
    if (!widget)
      {
      continue;
      }
    double distance2FromWidget = VTK_DOUBLE_MAX;
    if (widget->CanProcessInteractionEvent(callData, distance2FromWidget))
      {
      if (!closestWidget || distance2FromWidget < closestDistance2)
        {
        closestDistance2 = distance2FromWidget;
        closestWidget = widget;
        }
      }
    }
  return closestWidget;
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::CanProcessInteractionEvent(vtkMRMLInteractionEventData* eventData, double &closestDistance2)
{
  // New point can be placed anywhere
  int eventid = eventData->GetType();
  if ( eventid == vtkCommand::MouseMoveEvent
    || eventid == vtkCommand::LeftButtonPressEvent
    || eventid == vtkCommand::LeftButtonReleaseEvent
    || eventid == vtkCommand::RightButtonReleaseEvent
    || eventid == vtkCommand::EnterEvent
    || eventid == vtkCommand::LeaveEvent)
    {
    vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
    vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
    if (!interactionNode || !selectionNode)
      {
      return false;
      }
    if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      this->IsManageable(selectionNode->GetActivePlaceNodeClassName()))
      {
      closestDistance2 = 0.0;
      return true;
      }
    }

  if (eventid == vtkCommand::LeaveEvent && this->LastActiveWidget != NULL)
    {
    if (this->LastActiveWidget->GetMarkupsDisplayNode()
      && this->LastActiveWidget->GetMarkupsDisplayNode()->GetActiveComponentType() != vtkMRMLMarkupsDisplayNode::ComponentNone)
      {
      // this widget has active component, therefore leave event is relevant
      closestDistance2 = 0.0;
      return this->LastActiveWidget;
      }
    }

  // Other interactions
  bool canProcess = (this->FindClosestWidget(eventData, closestDistance2) != NULL);

  if (!canProcess && this->LastActiveWidget != nullptr
    && eventid == vtkCommand::MouseMoveEvent)
    {
    // mouse is moved away from the widget -> deactivate
    closestDistance2 = 0.0;
    return this->LastActiveWidget;
    }

  return canProcess;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::ProcessInteractionEvent(vtkMRMLInteractionEventData* eventData)
{
  if (this->GetDisableInteractorStyleEventsProcessing())
  {
    return;
  }

  // Place point
  int eventid = eventData->GetType();

  if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
    {
    if (eventid == vtkCommand::MouseMoveEvent)
      {
      vtkMRMLInteractionEventData* interactionEventData = vtkMRMLInteractionEventData::SafeDownCast(eventData);
      if (interactionEventData)
        {
        this->UpdatePointPlacePreview(interactionEventData->GetDisplayPosition(), interactionEventData->GetWorldPosition());
        }
      }
    else if (eventid == vtkCommand::LeftButtonReleaseEvent)
      {
      vtkMRMLInteractionEventData* interactionEventData = vtkMRMLInteractionEventData::SafeDownCast(eventData);
      if (interactionEventData)
        {
        std::string associatedNodeID = this->GetAssociatedNodeID(eventData);
        this->PlacePoint(interactionEventData->GetDisplayPosition(), interactionEventData->GetWorldPosition(), associatedNodeID);
        }
      }
    else if (eventid == vtkCommand::RightButtonReleaseEvent)
      {
      // if we're in persistent place mode, go back to view transform mode, but
      // leave the persistent flag on
      if (this->GetInteractionNode()->GetPlaceModePersistence() == 1)
        {
        vtkInfoMacro("Switch to transform mode");
        // add point after is switched to no place
        // the manager helper will take care to set the right status on the widgets
        this->GetInteractionNode()->SwitchToViewTransformMode();
        this->RemovePointPlacePreview();
        }
      }
    }

  if (eventid == vtkCommand::EnterEvent)
    {
    /*
    if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      this->OnClickInRenderWindowGetCoordinates(vtkMRMLMarkupsDisplayableManager::AddPreview);
      }
    this->RequestRender();
    */
    }
  else if (eventid == vtkCommand::LeaveEvent)
    {
    if (this->LastActiveWidget != NULL)
      {
      this->LastActiveWidget->Leave();
      this->LastActiveWidget = NULL;
      }
    }

  double closestDistance2 = VTK_DOUBLE_MAX;
  vtkSlicerAbstractWidget* closestWidget = this->FindClosestWidget(eventData, closestDistance2);
  if (this->LastActiveWidget != NULL && this->LastActiveWidget != closestWidget)
    {
    this->LastActiveWidget->Leave();
    }
  this->LastActiveWidget = closestWidget;
  if (!closestWidget)
    {
    // deactive widget is moving far from it
    if (eventid == vtkCommand::MouseMoveEvent && this->LastActiveWidget != NULL)
      {
      this->LastActiveWidget->Leave();
      this->LastActiveWidget = NULL;
      }
    return;
    }
  closestWidget->ProcessInteractionEvent(eventData);
  return;
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsNode* vtkMRMLMarkupsDisplayableManager::GetActiveMarkupsNode(bool forPlacement/*=false*/)
{
  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (!selectionNode)
  {
    return nullptr;
  }
  const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(activeMarkupsID));
  if (!markupsNode)
  {
    return nullptr;
  }

  if (!forPlacement)
  {
    return markupsNode;
  }

  // Additional checks for placement
  const char* placeNodeClassName = selectionNode->GetActivePlaceNodeClassName();
  if (!placeNodeClassName)
  {
    return nullptr;
  }
  if (!this->IsManageable(placeNodeClassName))
  {
    return nullptr;
  }
  if (std::string(markupsNode->GetClassName()) != placeNodeClassName)
  {
    return nullptr;
  }
  return markupsNode;
}

//---------------------------------------------------------------------------
int vtkMRMLMarkupsDisplayableManager::GetCurrentInteractionMode()
{
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (!interactionNode)
  {
    return 0;
  }
  return interactionNode->GetCurrentInteractionMode();
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::UpdatePointPlacePreview(const int displayPosition[2], const double worldPosition[3])
{
  if (this->GetCurrentInteractionMode() != vtkMRMLInteractionNode::Place)
  {
    return false;
  }

  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (!selectionNode)
  {
    return false;
  }
  std::string placeNodeClassName = (selectionNode->GetActivePlaceNodeClassName() ? selectionNode->GetActivePlaceNodeClassName() : nullptr);
  if (!this->IsManageable(placeNodeClassName.c_str()))
  {
    return false;
  }

  // Check if the active markups node is already the right class, and if yes then use that
  vtkMRMLMarkupsNode *activeMarkupsNode = this->GetActiveMarkupsNode(true);

  if (activeMarkupsNode && activeMarkupsNode->GetMaximumNumberOfControlPoints() > 0
    && activeMarkupsNode->GetNumberOfControlPoints() >= activeMarkupsNode->GetMaximumNumberOfControlPoints())
  {
    // maybe reached maximum number of points - if yes, then create a new widget
    if (activeMarkupsNode->GetNumberOfControlPoints() == activeMarkupsNode->GetMaximumNumberOfControlPoints())
    {
      // one more point than the maximum
      vtkSlicerAbstractWidget *slicerWidget = this->Helper->GetWidget(activeMarkupsNode);
      if (slicerWidget && !slicerWidget->GetFollowCursor())
      {
        // no preview is shown, so the widget is actually complete
        activeMarkupsNode = nullptr;
      }

    }
    else
    {
      // clearly over the maximum number of points
      activeMarkupsNode = nullptr;
    }

  }

  // If there is no active markups node then create a new one
  if (!activeMarkupsNode)
  {
    activeMarkupsNode = this->CreateNewMarkupsNode(placeNodeClassName);
    selectionNode->SetActivePlaceNodeID(activeMarkupsNode->GetID());
  }

  if (!activeMarkupsNode)
  {
    return false;
  }
  vtkSlicerAbstractWidget *slicerWidget = this->Helper->GetWidget(activeMarkupsNode);
  if (!slicerWidget)
  {
    return false;
  }

  slicerWidget->UpdatePreviewPointPositionFromWorldCoordinate(worldPosition);

  // force update of widgets on other views
  activeMarkupsNode->GetMarkupsDisplayNode()->Modified();
  return true;
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::RemovePointPlacePreview()
{
  vtkSlicerAbstractWidget *slicerWidget = this->Helper->GetWidget(this->GetActiveMarkupsNode(true));
  if (!slicerWidget)
  {
    return false;
  }
  return slicerWidget->RemovePreviewPoint();
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::PlacePoint(const int displayPosition[2],
  const double worldPosition[3], std::string associatedNodeID)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetActiveMarkupsNode(true);
  vtkSlicerAbstractWidget *slicerWidget = this->Helper->GetWidget(markupsNode);
  if (!slicerWidget)
  {
    return false;
  }

  // save for undo and add the node to the scene after any reset of the
  // interaction node so that don't end up back in place mode
  this->GetMRMLScene()->SaveStateForUndo();

  int controlPointIndex = slicerWidget->AddPointFromWorldCoordinate(worldPosition);

  // Save associated node ID
  vtkMRMLMarkupsFiducialNode* fiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(markupsNode);
  if (fiducialNode)
  {
    fiducialNode->SetNthFiducialAssociatedNodeID(controlPointIndex, associatedNodeID.c_str());
  }

  // if this was a one time place, go back to view transform mode
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode && !interactionNode->GetPlaceModePersistence()
    && markupsNode->GetNumberOfControlPoints() >= markupsNode->GetPreferredNumberOfControlPoints())
  {
    vtkDebugMacro("End of one time place, place mode persistence = " << interactionNode->GetPlaceModePersistence());
    interactionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::ViewTransform);
  }

  return (controlPointIndex >= 0);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::SetHasFocus(bool hasFocus)
{
  if (!hasFocus && this->LastActiveWidget!=NULL)
    {
    this->LastActiveWidget->Leave();
    this->LastActiveWidget = NULL;
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::GetGrabFocus()
{
  return (this->LastActiveWidget != NULL && this->LastActiveWidget->GetGrabFocus());
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::GetInteractive()
{
  return (this->LastActiveWidget != NULL && this->LastActiveWidget->GetInteractive());
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget * vtkMRMLMarkupsDisplayableManager::CreateWidget(vtkMRMLMarkupsDisplayNode* markupsDisplayNode)
{
  vtkMRMLMarkupsNode* markupsNode = markupsDisplayNode->GetMarkupsNode();
  if (!markupsNode)
  {
    return nullptr;
  }
  vtkMRMLAbstractViewNode* viewNode = vtkMRMLAbstractViewNode::SafeDownCast(this->GetMRMLDisplayableNode());
  vtkRenderer* renderer = this->GetRenderer();

  vtkSlicerAbstractWidget* widget = nullptr;
  if (vtkMRMLMarkupsAngleNode::SafeDownCast(markupsNode))
  {
    widget = vtkSlicerAngleWidget::New();
  }
  else if (vtkMRMLMarkupsFiducialNode::SafeDownCast(markupsNode))
  {
    widget = vtkSlicerPointsWidget::New();
  }
  else if (vtkMRMLMarkupsLineNode::SafeDownCast(markupsNode))
  {
    widget = vtkSlicerLineWidget::New();
  }
  else if (vtkMRMLMarkupsCurveNode::SafeDownCast(markupsNode))
  {
    widget = vtkSlicerCurveWidget::New();
  }
  else if (vtkMRMLMarkupsClosedCurveNode::SafeDownCast(markupsNode))
  {
    widget = vtkSlicerClosedCurveWidget::New();
  }
  else
  {
    vtkErrorMacro("vtkMRMLMarkupsDisplayableManager::CreateWidget failed: invalid display node class " << markupsNode->GetClassName());
    return nullptr;
  }

  widget->CreateDefaultRepresentation(markupsDisplayNode, viewNode, renderer);

  return widget;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::ConvertDeviceToXYZ(double x, double y, double xyz[3])
{
  vtkMRMLAbstractSliceViewDisplayableManager::ConvertDeviceToXYZ(this->GetInteractor(), this->GetMRMLSliceNode(), x, y, xyz);
}
