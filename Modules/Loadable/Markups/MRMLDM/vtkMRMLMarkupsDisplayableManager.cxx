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
#include <vtkMRMLModelDisplayableManager.h>
#include <vtkSliceViewInteractorStyle.h>

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
#include <vtkSlicerAbstractRepresentation.h>
#include <vtkSlicerAbstractRepresentation2D.h>
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
// vtkMRMLMarkupsDisplayableManager Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsFiducialWidgetCallback2D : public vtkCommand
{
public:
  static vtkMarkupsFiducialWidgetCallback2D *New()
  {
    return new vtkMarkupsFiducialWidgetCallback2D;
  }

  vtkMarkupsFiducialWidgetCallback2D()
    : Widget(nullptr)
    , Node(nullptr)
    , DisplayableManager(nullptr)
    , LastInteractionEventMarkupIndex(-1)
    , SelectionButton(0)
    , PointMovedSinceStartInteraction(false)
  {
  }

  virtual void Execute(vtkObject *vtkNotUsed(caller), unsigned long event, void *callData)
  {

    // sanity checks
    if (!this->DisplayableManager)
    {
      return;
    }
    if (!this->Node)
    {
      return;
    }
    if (!this->Widget)
    {
      return;
    }

    // If calldata is NULL, invoking an event may cause a crash (e.g., Python observer
    // tries to dereference the NULL pointer), therefore it's important to always pass a valid pointer
    // and indicate invalidity with value (-1).
    this->LastInteractionEventMarkupIndex = (callData ? *(reinterpret_cast<int *>(callData)) : -1);
    if (event == vtkCommand::PlacePointEvent)
    {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointAddedEvent, &this->LastInteractionEventMarkupIndex);
    }
    else if (event == vtkCommand::DeletePointEvent)
    {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRemovedEvent, &this->LastInteractionEventMarkupIndex);
    }
    else if (event == vtkCommand::StartInteractionEvent)
    {
      // save the state of the node when starting interaction
      if (this->Node->GetScene())
      {
        this->Node->GetScene()->SaveStateForUndo();
      }

      this->PointMovedSinceStartInteraction = false;
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointStartInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (this->Widget)
      {
        this->SelectionButton = this->Widget->GetSelectionButton();
      }
      // no need to propagate to MRML, just notify external observers that the user selected a markup
      return;
    }
    else if (event == vtkCommand::EndInteractionEvent)
    {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointEndInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (!this->PointMovedSinceStartInteraction)
      {
        if (this->SelectionButton == vtkSlicerAbstractWidget::LeftButton)
        {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftClickedEvent, &this->LastInteractionEventMarkupIndex);
        }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::MiddleButton)
        {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleClickedEvent, &this->LastInteractionEventMarkupIndex);
        }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::RightButton)
        {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRightClickedEvent, &this->LastInteractionEventMarkupIndex);
        }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::LeftButtonDoubleClick)
        {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
        }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::MiddleButtonDoubleClick)
        {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
        }
        else if (this->SelectionButton == vtkSlicerAbstractWidget::RightButtonDoubleClick)
        {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRightDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
        }
      }
      this->LastInteractionEventMarkupIndex = -1;
    }
    else if (event == vtkCommand::InteractionEvent)
    {
      this->PointMovedSinceStartInteraction = true;
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointModifiedEvent, &this->LastInteractionEventMarkupIndex);
    }
  }

  void SetWidget(vtkSlicerAbstractWidget *w)
  {
    this->Widget = w;
  }
  void SetNode(vtkMRMLMarkupsNode *n)
  {
    this->Node = n;
  }
  void SetDisplayableManager(vtkMRMLMarkupsDisplayableManager * dm)
  {
    this->DisplayableManager = dm;
  }

  vtkSlicerAbstractWidget * Widget;
  vtkMRMLMarkupsNode * Node;
  vtkMRMLMarkupsDisplayableManager * DisplayableManager;
  int LastInteractionEventMarkupIndex;
  int SelectionButton;
  bool PointMovedSinceStartInteraction;
};

//---------------------------------------------------------------------------
// vtkMRMLMarkupsDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManager::vtkMRMLMarkupsDisplayableManager()
{

  this->LastActiveWidget = NULL;

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
  int *nPtr = nullptr;
  int n = -1;
  if (callData != nullptr)
    {
    nPtr = reinterpret_cast<int *>(callData);
    if (nPtr)
      {
      n = *nPtr;
      }
    }
  if (markupsNode)
    {
    switch(event)
      {
      case vtkCommand::ModifiedEvent:
        this->OnMRMLMarkupsNodeModifiedEvent(markupsNode);
        break;
      case vtkMRMLMarkupsNode::PointModifiedEvent:
        this->OnMRMLMarkupsNthPointModifiedEvent(markupsNode, n);
        break;
      case vtkMRMLMarkupsNode::PointAddedEvent:
        this->OnMRMLMarkupsPointAddedEvent(markupsNode, n);
        break;
      case vtkMRMLMarkupsNode::PointRemovedEvent:
        this->OnMRMLMarkupsPointRemovedEvent(markupsNode, n);
        break;
      case vtkMRMLMarkupsNode::AllPointsRemovedEvent:
        this->OnMRMLMarkupsAllPointsRemovedEvent(markupsNode);
        break;
      case vtkMRMLTransformableNode::TransformModifiedEvent:
        this->OnMRMLMarkupsNodeTransformModifiedEvent(markupsNode);
        break;
      case vtkMRMLMarkupsNode::LockModifiedEvent:
        this->OnMRMLMarkupsNodeLockModifiedEvent(markupsNode);
        break;
      case vtkMRMLDisplayableNode::DisplayModifiedEvent:
        // get the display node and process the change
        this->OnMRMLMarkupsDisplayNodeModifiedEvent(markupsNode);
        break;
      }

    // Force re-render
    for (int i = 0; i < markupsNode->GetNumberOfDisplayNodes(); i++)
      {
      vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(i));
      vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(displayNode);
      if (widget)
        {
        vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast(widget->GetRepresentation());
        if (rep)
          {
          if (rep->GetNeedToRender())
            {
            this->RequestRender();
            rep->NeedToRenderOff();
            }
          }
        }
      }

    }
  else if (interactionNode)
    {
    if (event == vtkMRMLInteractionNode::InteractionModeChangedEvent)
      {
      this->Helper->UpdateAllWidgetsFromInteractionNode(interactionNode);
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
bool vtkMRMLMarkupsDisplayableManager::IsPointDisplayableOnSlice(vtkMRMLMarkupsNode *node, int pointIndex)
{
  if (this->IsInLightboxMode())
    {
    // TBD: issue 1690: disable angle in light box mode as they appear
    // in the wrong location
    return false;
    }

  vtkMRMLSliceNode* sliceNode = this->GetMRMLSliceNode();
  // if no slice node, it doesn't constrain the visibility, so return that
  // it's visible
  if (!sliceNode)
    {
    // 3D view
    return 1;
    }

  // if there's no node, it's not visible
  if (!node)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the markups node.")
    return 0;
    }

  bool showPoint = true;

  // allow annotations to appear only in designated viewers
  vtkMRMLDisplayNode *displayNode = node->GetDisplayNode();
  if (displayNode && !displayNode->IsDisplayableInView(sliceNode->GetID()))
    {
    return 0;
    }

  // down cast the node as a controlpoints node to get the coordinates
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the controlpoints node.")
    return 0;
    }

  double transformedWorldCoordinates[4];
  markupsNode->GetNthControlPointPositionWorld(pointIndex, transformedWorldCoordinates);

  // now get the displayCoordinates for the transformed worldCoordinates
  double displayCoordinates[4];
  this->GetWorldToDisplayCoordinates(transformedWorldCoordinates,displayCoordinates);

  if (this->IsInLightboxMode())
    {
    //
    // Lightbox specific code
    //
    // get the corresponding lightbox index for this display coordinate and
    // check if it's in the range of the current number of light boxes being
    // displayed in the grid rows/columns.
    int lightboxIndex = this->GetLightboxIndex(markupsNode, pointIndex);
    int numberOfLightboxes = sliceNode->GetLayoutGridColumns() * sliceNode->GetLayoutGridRows();
    if (lightboxIndex < 0 ||
        lightboxIndex >= numberOfLightboxes)
      {
      showPoint = false;
      }
    }
  // check if the point is close enough to the slice to be shown
  if (showPoint)
    {
    if (this->IsInLightboxMode())
      {
      // get the volume's spacing to determine the distance between the slice
      // location and the markup
      // default to spacing 1.0 in case can't get volume slice spacing from
      // the logic as that will be a multiplicative no-op
      double spacing = 1.0;
      vtkMRMLSliceLogic *sliceLogic = nullptr;
      vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
      if (mrmlAppLogic)
        {
        sliceLogic = mrmlAppLogic->GetSliceLogic(sliceNode);
        }
      if (sliceLogic)
        {
        double *volumeSliceSpacing = sliceLogic->GetLowestVolumeSliceSpacing();
        if (volumeSliceSpacing != nullptr)
          {
          vtkDebugMacro("Slice node " << sliceNode->GetName()
                        << ": volumeSliceSpacing = "
                        << volumeSliceSpacing[0] << ", "
                        << volumeSliceSpacing[1] << ", "
                        << volumeSliceSpacing[2]);
          spacing = volumeSliceSpacing[2];
          }
        }
      vtkDebugMacro("displayCoordinates: "
                    << displayCoordinates[0] << ","
                    << displayCoordinates[1] << ","
                    << displayCoordinates[2] << "\n\tworld coords: "
                    << transformedWorldCoordinates[0] << ","
                    << transformedWorldCoordinates[1] << ","
                    << transformedWorldCoordinates[2]);
      // calculate the distance from the point in world space to the
      // plane defined by the slice node normal and origin (using same
      // convention as the vtkMRMLThreeDReformatDisplayableManager)
      vtkMatrix4x4 *sliceToRAS = sliceNode->GetSliceToRAS();
      double slicePlaneNormal[3], slicePlaneOrigin[3];
      slicePlaneNormal[0] = sliceToRAS->GetElement(0,2);
      slicePlaneNormal[1] = sliceToRAS->GetElement(1,2);
      slicePlaneNormal[2] = sliceToRAS->GetElement(2,2);
      slicePlaneOrigin[0] = sliceToRAS->GetElement(0,3);
      slicePlaneOrigin[1] = sliceToRAS->GetElement(1,3);
      slicePlaneOrigin[2] = sliceToRAS->GetElement(2,3);
      double distanceToPlane = slicePlaneNormal[0]*(transformedWorldCoordinates[0]-slicePlaneOrigin[0]) +
        slicePlaneNormal[1]*(transformedWorldCoordinates[1]-slicePlaneOrigin[1]) +
        slicePlaneNormal[2]*(transformedWorldCoordinates[2]-slicePlaneOrigin[2]);
      // this gives the distance to light box plane 0, but have to offset by
      // number of light box planes (as determined by the light box index) times the volume
      // slice spacing
      int lightboxIndex = this->GetLightboxIndex(markupsNode, pointIndex);
      double lightboxOffset = lightboxIndex * spacing;
      double distanceToSlice = distanceToPlane - lightboxOffset;
      double maxDistance = 0.5;
      vtkDebugMacro("\n\tdistance to plane = " << distanceToPlane
                    << "\n\tlightboxIndex = " << lightboxIndex
                    << "\n\tlightboxOffset = " << lightboxOffset
                    << "\n\tdistance to slice = " << distanceToSlice);
      // check that it's within 0.5mm
      if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
        {
        vtkDebugMacro("Distance to slice is greater than max distance, not showing the widget");
        showPoint = false;
        }
      }
    else
      {
      // the third coordinate of the displayCoordinates is the distance to the slice
      double distanceToSlice = displayCoordinates[2];
      double maxDistance = 0.5 + (sliceNode->GetDimensions()[2] - 1);
      vtkDebugMacro("Slice node " << sliceNode->GetName()
                    << ": distance to slice = " << distanceToSlice
                    << ", maxDistance = " << maxDistance
                    << "\n\tslice node dimensions[2] = "
                    << sliceNode->GetDimensions()[2]);
      if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
        {
        // if the distance to the slice is more than 0.5mm, we know that at least one coordinate of the widget is outside the current activeSlice
        // hence, we do not want to show this widget
        showPoint = false;
        }
      }
    }

  return showPoint;
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::IsCentroidDisplayableOnSlice(vtkMRMLMarkupsNode *node)
{
  if (this->IsInLightboxMode())
    {
    // TBD: issue 1690: disable angle in light box mode as they appear
    // in the wrong location
    return false;
    }

  vtkMRMLSliceNode* sliceNode = this->GetMRMLSliceNode();
  // if no slice node, it doesn't constrain the visibility, so return that
  // it's visible
  if (!sliceNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the sliceNode.")
    return 1;
    }

  // if there's no node, it's not visible
  if (!node)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the markups node.")
    return 0;
    }

  bool showPoint = true;

  // allow annotations to appear only in designated viewers
  vtkMRMLDisplayNode *displayNode = node->GetDisplayNode();
  if (displayNode && !displayNode->IsDisplayableInView(sliceNode->GetID()))
    {
    return 0;
    }

  // down cast the node as a controlpoints node to get the coordinates
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the controlpoints node.")
    return 0;
    }

  double transformedWorldCoordinates[4];
  markupsNode->GetCentroidPositionWorld(transformedWorldCoordinates);

  // now get the displayCoordinates for the transformed worldCoordinates
  double displayCoordinates[4];
  this->GetWorldToDisplayCoordinates(transformedWorldCoordinates,displayCoordinates);

  if (this->IsInLightboxMode())
    {
    //
    // Lightbox specific code
    //
    // get the corresponding lightbox index for this display coordinate and
    // check if it's in the range of the current number of light boxes being
    // displayed in the grid rows/columns.
    int lightboxIndex = this->GetLightboxIndex(markupsNode, -3);
    int numberOfLightboxes = sliceNode->GetLayoutGridColumns() * sliceNode->GetLayoutGridRows();
    if (lightboxIndex < 0 ||
        lightboxIndex >= numberOfLightboxes)
      {
      showPoint = false;
      }
    }
  // check if the point is close enough to the slice to be shown
  if (showPoint)
    {
    if (this->IsInLightboxMode())
      {
      // get the volume's spacing to determine the distance between the slice
      // location and the markup
      // default to spacing 1.0 in case can't get volume slice spacing from
      // the logic as that will be a multiplicative no-op
      double spacing = 1.0;
      vtkMRMLSliceLogic *sliceLogic = nullptr;
      vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
      if (mrmlAppLogic)
        {
        sliceLogic = mrmlAppLogic->GetSliceLogic(sliceNode);
        }
      if (sliceLogic)
        {
        double *volumeSliceSpacing = sliceLogic->GetLowestVolumeSliceSpacing();
        if (volumeSliceSpacing != nullptr)
          {
          vtkDebugMacro("Slice node " << sliceNode->GetName()
                        << ": volumeSliceSpacing = "
                        << volumeSliceSpacing[0] << ", "
                        << volumeSliceSpacing[1] << ", "
                        << volumeSliceSpacing[2]);
          spacing = volumeSliceSpacing[2];
          }
        }
      vtkDebugMacro("displayCoordinates: "
                    << displayCoordinates[0] << ","
                    << displayCoordinates[1] << ","
                    << displayCoordinates[2] << "\n\tworld coords: "
                    << transformedWorldCoordinates[0] << ","
                    << transformedWorldCoordinates[1] << ","
                    << transformedWorldCoordinates[2]);
      // calculate the distance from the point in world space to the
      // plane defined by the slice node normal and origin (using same
      // convention as the vtkMRMLThreeDReformatDisplayableManager)
      vtkMatrix4x4 *sliceToRAS = sliceNode->GetSliceToRAS();
      double slicePlaneNormal[3], slicePlaneOrigin[3];
      slicePlaneNormal[0] = sliceToRAS->GetElement(0,2);
      slicePlaneNormal[1] = sliceToRAS->GetElement(1,2);
      slicePlaneNormal[2] = sliceToRAS->GetElement(2,2);
      slicePlaneOrigin[0] = sliceToRAS->GetElement(0,3);
      slicePlaneOrigin[1] = sliceToRAS->GetElement(1,3);
      slicePlaneOrigin[2] = sliceToRAS->GetElement(2,3);
      double distanceToPlane = slicePlaneNormal[0]*(transformedWorldCoordinates[0]-slicePlaneOrigin[0]) +
        slicePlaneNormal[1]*(transformedWorldCoordinates[1]-slicePlaneOrigin[1]) +
        slicePlaneNormal[2]*(transformedWorldCoordinates[2]-slicePlaneOrigin[2]);
      // this gives the distance to light box plane 0, but have to offset by
      // number of light box planes (as determined by the light box index) times the volume
      // slice spacing
      int lightboxIndex = this->GetLightboxIndex(markupsNode, -3);
      double lightboxOffset = lightboxIndex * spacing;
      double distanceToSlice = distanceToPlane - lightboxOffset;
      double maxDistance = 0.5;
      vtkDebugMacro("\n\tdistance to plane = " << distanceToPlane
                    << "\n\tlightboxIndex = " << lightboxIndex
                    << "\n\tlightboxOffset = " << lightboxOffset
                    << "\n\tdistance to slice = " << distanceToSlice);
      // check that it's within 0.5mm
      if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
        {
        vtkDebugMacro("Distance to slice is greater than max distance, not showing the widget");
        showPoint = false;
        }
      }
    else
      {
      // the third coordinate of the displayCoordinates is the distance to the slice
      double distanceToSlice = displayCoordinates[2];
      double maxDistance = 0.5 + (sliceNode->GetDimensions()[2] - 1);
      vtkDebugMacro("Slice node " << sliceNode->GetName()
                    << ": distance to slice = " << distanceToSlice
                    << ", maxDistance = " << maxDistance
                    << "\n\tslice node dimensions[2] = "
                    << sliceNode->GetDimensions()[2]);
      if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
        {
        // if the distance to the slice is more than 0.5mm, we know that at least one coordinate of the widget is outside the current activeSlice
        // hence, we do not want to show this widget
        showPoint = false;
        }
      }
    }

  return showPoint;
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
void vtkMRMLMarkupsDisplayableManager::OnMRMLMarkupsNodeModifiedEvent(vtkMRMLNode* node)
{
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeModifiedEvent: Can not access node.")
    return;
    }

  for (int displayNodeIndex = 0; displayNodeIndex < markupsNode->GetNumberOfDisplayNodes(); displayNodeIndex++)
  {
    vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(displayNodeIndex));
    vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(displayNode);
    if (!widget)
    {
      continue;
    }

    // Points widgets have only one Markup/Representation
    vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
      (widget->GetRepresentation());
    if (!rep)
      {
      continue;
      }

    for (int PointIndex = 0; PointIndex < markupsNode->GetNumberOfControlPoints(); PointIndex++)
      {
      bool visibility = this->IsPointDisplayableOnSlice(markupsNode, PointIndex);
      rep->SetNthPointSliceVisibility(PointIndex, visibility);
      }
    if (rep->GetClosedLoop())
      {
      bool visibility = this->IsCentroidDisplayableOnSlice(markupsNode);
      rep->SetCentroidSliceVisibility(visibility);
      }

    widget->BuildRepresentation();
    }

  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLMarkupsDisplayNodeModifiedEvent(vtkMRMLNode* node)
{
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
  {
    vtkErrorMacro("OnMRMLMarkupsNodeModifiedEvent: Can not access node.");
    return;
  }

  for (int displayNodeIndex = 0; displayNodeIndex < markupsNode->GetNumberOfDisplayNodes(); displayNodeIndex++)
  {
    vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(displayNodeIndex));
    vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(displayNode);
    if (!widget)
    {
      this->Helper->AddDisplayNode(displayNode);
    }
    else
    {
      widget->BuildRepresentation();
    }
  }
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLMarkupsNthPointModifiedEvent(vtkMRMLNode *node, int n)
{
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
  {
    vtkErrorMacro("OnMRMLMarkupsNodeModifiedEvent: Can not access node.");
    return;
  }

  for (int displayNodeIndex = 0; displayNodeIndex < markupsNode->GetNumberOfDisplayNodes(); displayNodeIndex++)
  {
    vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(displayNodeIndex));
    vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(displayNode);
    if (!widget)
    {
      continue;
    }

    // Check if the Point is on the slice
    bool visibility = this->IsPointDisplayableOnSlice(markupsNode, n);
    vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
    (widget->GetRepresentation());
    if (rep && rep->GetVisibility())
    {
      rep->SetNthPointSliceVisibility(n, visibility);
      if (rep->GetClosedLoop())
      {
        visibility = this->IsCentroidDisplayableOnSlice(markupsNode);
        rep->SetCentroidSliceVisibility(visibility);
      }
    }

    // Rebuild representation
    widget->BuildRepresentation();
  }

  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLMarkupsPointAddedEvent(vtkMRMLNode *node, int vtkNotUsed(n))
{
  vtkDebugMacro("OnMRMLMarkupsPointAddedEvent");
  if (!node)
    {
    return;
    }

  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    return;
    }

  for (int displayNodeIndex = 0; displayNodeIndex < markupsNode->GetNumberOfDisplayNodes(); displayNodeIndex++)
  {
    vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(displayNodeIndex));
    vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(displayNode);
    if (!widget)
      {
      continue;
      }

    // widgets have only one Markup/Representation
    vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
      (widget->GetRepresentation());
    if (!rep)
      {
      continue;
      }

    // TODO: move these to the representation class
    for (int PointIndex = 0; PointIndex < markupsNode->GetNumberOfControlPoints(); PointIndex++)
      {
      bool visibility = this->IsPointDisplayableOnSlice(markupsNode, PointIndex);
      rep->SetNthPointSliceVisibility(PointIndex, visibility);
      }
    if (rep->GetClosedLoop())
      {
      bool visibility = this->IsCentroidDisplayableOnSlice(markupsNode);
      rep->SetCentroidSliceVisibility(visibility);
      }

    // Rebuild representation
    widget->BuildRepresentation();
    }
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLMarkupsPointRemovedEvent(vtkMRMLNode *node, int vtkNotUsed(n))
{
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
  {
    vtkErrorMacro("OnMRMLMarkupsNodeModifiedEvent: Can not access node.");
    return;
  }
  for (int displayNodeIndex = 0; displayNodeIndex < markupsNode->GetNumberOfDisplayNodes(); displayNodeIndex++)
  {
    vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(displayNodeIndex));
    vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(displayNode);
    if (!widget)
    {
      continue;
    }
    // Rebuild representation
    widget->BuildRepresentation();
  }
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLMarkupsAllPointsRemovedEvent(vtkMRMLNode *node)
{
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
  {
    vtkErrorMacro("OnMRMLMarkupsNodeModifiedEvent: Can not access node.");
    return;
  }
  for (int displayNodeIndex = 0; displayNodeIndex < markupsNode->GetNumberOfDisplayNodes(); displayNodeIndex++)
  {
    vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(displayNodeIndex));
    vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(displayNode);
    if (!widget)
    {
      continue;
    }
    // Rebuild representation
    widget->BuildRepresentation();
  }
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLMarkupsNodeTransformModifiedEvent(vtkMRMLNode* node)
{
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
  {
    vtkErrorMacro("OnMRMLMarkupsNodeModifiedEvent: Can not access node.");
    return;
  }
  for (int displayNodeIndex = 0; displayNodeIndex < markupsNode->GetNumberOfDisplayNodes(); displayNodeIndex++)
  {
    vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(displayNodeIndex));
    vtkSlicerAbstractWidget *widget = this->Helper->GetWidget(displayNode);
    if (!widget)
    {
      continue;
    }
    // Rebuild representation
    widget->BuildLocator(); // transform changes points positions therefore locator needs to be updated
    widget->BuildRepresentation();
  }
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnMRMLMarkupsNodeLockModifiedEvent(vtkMRMLNode* node)
{
  vtkDebugMacro("OnMRMLMarkupsNodeLockModifiedEvent");
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!markupsNode)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeLockModifiedEvent - Can not access node.")
    return;
    }
  // Update the standard settings of all widgets.
  for (int displayNodeIndex = 0; displayNodeIndex < markupsNode->GetNumberOfDisplayNodes(); displayNodeIndex++)
  {
    vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(markupsNode->GetNthDisplayNode(displayNodeIndex));
    vtkSlicerAbstractWidget* widget = this->Helper->GetWidget(displayNode);
    if (!widget)
    {
      continue;
    }
    // TODO: it could be more efficient if we only updated locked state
    widget->GetRepresentation()->BuildRepresentation();
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
    // (f.e. change slice number, zoom etc.)

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
  // run through all markup nodes in the helper
  vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt it
    = this->Helper->MarkupsDisplayNodesToWidgets.begin();
  bool requestRender = false;
  while(it != this->Helper->MarkupsDisplayNodesToWidgets.end())
    {
    // we loop through all nodes
    vtkMRMLMarkupsDisplayNode * markupsDisplayNode = (it->first);

    vtkSlicerAbstractWidget* widget = this->Helper->GetWidget(markupsDisplayNode);
    if (widget)
      {
      // Rebuild representation
      vtkSlicerAbstractRepresentation2D *rep = vtkSlicerAbstractRepresentation2D::SafeDownCast
        (widget->GetRepresentation());
      if (!rep)
        {
        return;
        }
      vtkMRMLMarkupsNode* markupsNode = markupsDisplayNode->GetMarkupsNode();
      for (int PointIndex = 0; PointIndex < markupsNode->GetNumberOfControlPoints(); PointIndex++)
        {
        bool visibility = this->IsPointDisplayableOnSlice(markupsNode, PointIndex);
        rep->SetNthPointSliceVisibility(PointIndex, visibility);
        }

      if (rep->GetClosedLoop())
        {
        bool visibility = this->IsCentroidDisplayableOnSlice(markupsNode);
        rep->SetCentroidSliceVisibility(visibility);
        }

      widget->BuildRepresentation();
      requestRender = true;
      }

    ++it;
    }

  if (requestRender)
    {
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsDisplayableManager::IsWidgetDisplayableOnSlice(vtkMRMLMarkupsNode* node)
{
  if (this->IsInLightboxMode())
    {
    // TBD: issue 1690: disable fiducials in light box mode as they appear
    // in the wrong location
    return false;
    }

  vtkMRMLSliceNode* sliceNode = this->GetMRMLSliceNode();
  // if no slice node, it doesn't constrain the visibility, so return that
  // it's visible
  if (!sliceNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the sliceNode.")
    return 1;
    }

  // if there's no node, it's not visible
  if (!node)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the markups node.")
    return 0;
    }

  bool showWidget = true;
  bool inViewport = false;

  // allow annotations to appear only in designated viewers
  vtkMRMLDisplayNode *displayNode = node->GetDisplayNode();
  if (displayNode && !displayNode->IsDisplayableInView(sliceNode->GetID()))
    {
    return 0;
    }

  // down cast the node as a controlpoints node to get the coordinates
  vtkMRMLMarkupsNode * controlPointsNode = vtkMRMLMarkupsNode::SafeDownCast(node);

  if (!controlPointsNode)
    {
    vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not get the controlpoints node.")
    return 0;
    }

  int numberOfControlPoints = controlPointsNode->GetNumberOfControlPoints();
  for (int i=0; i < numberOfControlPoints; i++)
    {
    // we loop through all controlpoints of each node
    double transformedWorldCoordinates[4];
    controlPointsNode->GetNthControlPointPositionWorld(i, transformedWorldCoordinates);

    // now get the displayCoordinates for the transformed worldCoordinates
    double displayCoordinates[4];
    this->GetWorldToDisplayCoordinates(transformedWorldCoordinates,displayCoordinates);

    if (this->IsInLightboxMode())
      {
      //
      // Lightbox specific code
      //
      // get the corresponding lightbox index for this display coordinate and
      // check if it's in the range of the current number of light boxes being
      // displayed in the grid rows/columns.
      int lightboxIndex = this->GetLightboxIndex(controlPointsNode, i);
      int numberOfLightboxes = sliceNode->GetLayoutGridColumns() * sliceNode->GetLayoutGridRows();
      if (lightboxIndex < 0 ||
          lightboxIndex >= numberOfLightboxes)
        {
        showWidget = false;
        }
      }
    // check if the markup is close enough to the slice to be shown
    if (showWidget)
      {
      if (this->IsInLightboxMode())
        {
        // get the volume's spacing to determine the distance between the slice
        // location and the markup
        // default to spacing 1.0 in case can't get volume slice spacing from
        // the logic as that will be a multiplicative no-op
        double spacing = 1.0;
        vtkMRMLSliceLogic *sliceLogic = nullptr;
        vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
        if (mrmlAppLogic)
          {
          sliceLogic = mrmlAppLogic->GetSliceLogic(sliceNode);
          }
        if (sliceLogic)
          {
          double *volumeSliceSpacing = sliceLogic->GetLowestVolumeSliceSpacing();
          if (volumeSliceSpacing != nullptr)
            {
            vtkDebugMacro("Slice node " << sliceNode->GetName()
                          << ": volumeSliceSpacing = "
                          << volumeSliceSpacing[0] << ", "
                          << volumeSliceSpacing[1] << ", "
                          << volumeSliceSpacing[2]);
            spacing = volumeSliceSpacing[2];
            }
          }
        vtkDebugMacro("displayCoordinates: "
                      << displayCoordinates[0] << ","
                      << displayCoordinates[1] << ","
                      << displayCoordinates[2] << "\n\tworld coords: "
                      << transformedWorldCoordinates[0] << ","
                      << transformedWorldCoordinates[1] << ","
                      << transformedWorldCoordinates[2]);
        // calculate the distance from the point in world space to the
        // plane defined by the slice node normal and origin (using same
        // convention as the vtkMRMLThreeDReformatDisplayableManager)
        vtkMatrix4x4 *sliceToRAS = sliceNode->GetSliceToRAS();
        double slicePlaneNormal[3], slicePlaneOrigin[3];
        slicePlaneNormal[0] = sliceToRAS->GetElement(0,2);
        slicePlaneNormal[1] = sliceToRAS->GetElement(1,2);
        slicePlaneNormal[2] = sliceToRAS->GetElement(2,2);
        slicePlaneOrigin[0] = sliceToRAS->GetElement(0,3);
        slicePlaneOrigin[1] = sliceToRAS->GetElement(1,3);
        slicePlaneOrigin[2] = sliceToRAS->GetElement(2,3);
        double distanceToPlane = slicePlaneNormal[0]*(transformedWorldCoordinates[0]-slicePlaneOrigin[0]) +
          slicePlaneNormal[1]*(transformedWorldCoordinates[1]-slicePlaneOrigin[1]) +
          slicePlaneNormal[2]*(transformedWorldCoordinates[2]-slicePlaneOrigin[2]);
        // this gives the distance to light box plane 0, but have to offset by
        // number of light box planes (as determined by the light box index) times the volume
        // slice spacing
        int lightboxIndex = this->GetLightboxIndex(controlPointsNode, i);
        double lightboxOffset = lightboxIndex * spacing;
        double distanceToSlice = distanceToPlane - lightboxOffset;
        double maxDistance = 0.5;
        vtkDebugMacro("\n\tdistance to plane = " << distanceToPlane
                      << "\n\tlightboxIndex = " << lightboxIndex
                      << "\n\tlightboxOffset = " << lightboxOffset
                      << "\n\tdistance to slice = " << distanceToSlice);
        // check that it's within 0.5mm
        if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
          {
          vtkDebugMacro("Distance to slice is greater than max distance, not showing the widget");
          showWidget = false;
          break;
          }
        }
      else
        {
        // the third coordinate of the displayCoordinates is the distance to the slice
        double distanceToSlice = displayCoordinates[2];
        double maxDistance = 0.5 + (sliceNode->GetDimensions()[2] - 1);
        vtkDebugMacro("Slice node " << sliceNode->GetName()
                      << ": distance to slice = " << distanceToSlice
                      << ", maxDistance = " << maxDistance
                      << "\n\tslice node dimensions[2] = "
                      << sliceNode->GetDimensions()[2]);
        if (distanceToSlice < -0.5 || distanceToSlice >= maxDistance)
          {
          // if the distance to the slice is more than 0.5mm, we know that at least one coordinate of the widget is outside the current activeSlice
          // hence, we do not want to show this widget
          showWidget = false;
          // we don't even need to continue parsing the controlpoints, because we know the widget will not be shown
          break;
          }
        }
      }

    // -----------------------------------------
    // special cases when the slices get panned:

    // if all of the controlpoints are outside the viewport coordinates, the widget should not be shown
    // if one controlpoint is inside the viewport coordinates, the widget should be shown

    // we need to check if we are inside the viewport
    double coords[2] = {displayCoordinates[0], displayCoordinates[1]};

    vtkRenderer* pokedRenderer = this->GetInteractor()->
      FindPokedRenderer(static_cast<int> (coords[0]), static_cast<int> (coords[1]));
    if (!pokedRenderer)
      {
      vtkErrorMacro("IsWidgetDisplayableOnSlice: Could not find the poked renderer!")
      return false;
      }

    pokedRenderer->DisplayToNormalizedDisplay(coords[0],coords[1]);
    pokedRenderer->NormalizedDisplayToViewport(coords[0],coords[1]);
    pokedRenderer->ViewportToNormalizedViewport(coords[0],coords[1]);

    if ((coords[0]>0.0) && (coords[0]<1.0) && (coords[1]>0.0) && (coords[1]<1.0))
      {
      // current point is inside of view
      inViewport = true;
      }

    } // end of for loop through control points

  return showWidget && inViewport;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::OnInteractorStyleEvent(int eventid)
{
  Superclass::OnInteractorStyleEvent(eventid);

}

//---------------------------------------------------------------------------
vtkAbstractWidget * vtkMRMLMarkupsDisplayableManager::GetWidget(vtkMRMLMarkupsDisplayNode * node)
{
  return this->Helper->GetWidget(node);
}

//---------------------------------------------------------------------------
std::string vtkMRMLMarkupsDisplayableManager::GetAssociatedNodeID(vtkEventData* eventData)
{
  vtkSlicerInteractionEventData* interactionEventData = vtkSlicerInteractionEventData::SafeDownCast(eventData);
  if (interactionEventData)
  {
    double windowWidth = this->GetInteractor()->GetRenderWindow()->GetSize()[0];
    double windowHeight = this->GetInteractor()->GetRenderWindow()->GetSize()[1];
    const int* displayPosition = interactionEventData->GetDisplayPosition();
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
/// Convert world to display coordinates
void vtkMRMLMarkupsDisplayableManager::GetWorldToDisplayCoordinates(double r, double a, double s, double * displayCoordinates)
{
  if (!this->GetMRMLSliceNode())
    {
    vtkErrorMacro("GetWorldToDisplayCoordinates: no slice node!");
    return;
    }

  // we will get the transformation matrix to convert world coordinates to the display coordinates of the specific sliceNode

  vtkMatrix4x4 * xyToRasMatrix = this->GetMRMLSliceNode()->GetXYToRAS();
  vtkNew<vtkMatrix4x4> rasToXyMatrix;

  // we need to invert this matrix
  xyToRasMatrix->Invert(xyToRasMatrix, rasToXyMatrix.GetPointer());

  double worldCoordinates[4];
  worldCoordinates[0] = r;
  worldCoordinates[1] = a;
  worldCoordinates[2] = s;
  worldCoordinates[3] = 1;

  rasToXyMatrix->MultiplyPoint(worldCoordinates,displayCoordinates);
  xyToRasMatrix = nullptr;
}

//---------------------------------------------------------------------------
// Convert world to display coordinates
void vtkMRMLMarkupsDisplayableManager::GetWorldToDisplayCoordinates(double * worldCoordinates, double * displayCoordinates)
{
  if (worldCoordinates == nullptr)
    {
    return;
    }

  this->GetWorldToDisplayCoordinates(worldCoordinates[0], worldCoordinates[1], worldCoordinates[2], displayCoordinates);
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
int  vtkMRMLMarkupsDisplayableManager::GetLightboxIndex(vtkMRMLMarkupsNode *node, int pointIndex)
{
  if (pointIndex == -3)
    {
    double transformedWorldCoordinates[4], displayCoordinates[4];
    node->GetCentroidPositionWorld(transformedWorldCoordinates);
    this->GetWorldToDisplayCoordinates(transformedWorldCoordinates,displayCoordinates);

    return static_cast<int> (floor(displayCoordinates[2]+0.5));
    }

  int index = -1;

  if (!node || !this->IsInLightboxMode() ||
      !node->ControlPointExists(pointIndex))
    {
    return index;
    }

  double transformedWorldCoordinates[4], displayCoordinates[4];
  node->GetNthControlPointPositionWorld(pointIndex, transformedWorldCoordinates);
  this->GetWorldToDisplayCoordinates(transformedWorldCoordinates,displayCoordinates);

  return static_cast<int> (floor(displayCoordinates[2]+0.5));
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
vtkSlicerAbstractWidget* vtkMRMLMarkupsDisplayableManager::FindClosestWidget(vtkEventData *callData, double &closestDistance2)
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
bool vtkMRMLMarkupsDisplayableManager::CanProcessInteractionEvent(vtkEventData* eventData, double &closestDistance2)
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

  // Other interactions
  return (this->FindClosestWidget(eventData, closestDistance2) != NULL);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::ProcessInteractionEvent(vtkEventData* eventData)
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
      vtkSlicerInteractionEventData* interactionEventData = vtkSlicerInteractionEventData::SafeDownCast(eventData);
      if (interactionEventData)
      {
        this->UpdatePointPlacePreview(interactionEventData->GetDisplayPosition(), interactionEventData->GetWorldPosition());
      }
    }
    else if (eventid == vtkCommand::LeftButtonReleaseEvent)
    {
      vtkSlicerInteractionEventData* interactionEventData = vtkSlicerInteractionEventData::SafeDownCast(eventData);
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
  if (eventid == vtkCommand::KeyPressEvent)
  {
    char *keySym = this->GetInteractor()->GetKeySym();
    vtkDebugMacro("OnInteractorStyleEvent 3D: key press event position = "
      << this->GetInteractor()->GetEventPosition()[0] << ", "
      << this->GetInteractor()->GetEventPosition()[1]
      << ", key sym = " << (keySym == NULL ? "null" : keySym));
    if (!keySym)
    {
      return;
    }
    if (strcmp(keySym, "p") == 0)
    {
      if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
        vtkSlicerInteractionEventData* interactionEventData = vtkSlicerInteractionEventData::SafeDownCast(eventData);
        if (interactionEventData)
        {
          std::string associatedNodeID = this->GetAssociatedNodeID(eventData);
          if (this->PlacePoint(interactionEventData->GetDisplayPosition(), interactionEventData->GetWorldPosition(), associatedNodeID))
          {
            return;
          }
        }
        else
        {
          vtkDebugMacro("Line DisplayableManager: key press p, but not in Place mode! Returning.");
          return;
        }
      }
    }
    else if (eventid == vtkCommand::KeyReleaseEvent)
    {
      vtkDebugMacro("Got a key release event");
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
  vtkMRMLMarkupsNode *activeMarkupNode = this->GetActiveMarkupsNode(true);

  // If there is no active markups node then create a new one
  if (!activeMarkupNode)
  {
    activeMarkupNode = this->CreateNewMarkupsNode(placeNodeClassName);
    selectionNode->SetActivePlaceNodeID(activeMarkupNode->GetID());
  }

  if (!activeMarkupNode)
  {
    return false;
  }
  vtkSlicerAbstractWidget *slicerWidget = this->Helper->GetWidget(activeMarkupNode);
  if (!slicerWidget)
  {
    return false;
  }

  // Check if the widget has been already placed
  // if yes, set again to define
  /*
  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place
  && slicerWidget->GetWidgetState() == vtkSlicerAbstractWidget::Manipulate)
  {
  slicerWidget->SetWidgetState(vtkSlicerAbstractWidget::Define);
  slicerWidget->SetManagesCursor(false);
  }
  */
  slicerWidget->UpdatePreviewPointPositionFromWorldCoordinate(worldPosition);

  // force update of widgets on other views
  activeMarkupNode->GetMarkupsDisplayNode()->Modified();
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
void vtkMRMLMarkupsDisplayableManager::OnWidgetAdded(vtkMRMLMarkupsDisplayNode* markupsDisplayNode, vtkSlicerAbstractWidget* newWidget)
{
  // Add callback to the widget to be able to save undo states
  vtkNew<vtkMarkupsFiducialWidgetCallback2D> myCallback;
  myCallback->SetNode(markupsDisplayNode->GetMarkupsNode());
  myCallback->SetWidget(newWidget);
  myCallback->SetDisplayableManager(this);
  newWidget->AddObserver(vtkCommand::StartInteractionEvent, myCallback);
  newWidget->AddObserver(vtkCommand::EndInteractionEvent, myCallback);
  newWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);
  newWidget->AddObserver(vtkCommand::PlacePointEvent, myCallback);
  newWidget->AddObserver(vtkCommand::DeletePointEvent, myCallback);
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget * vtkMRMLMarkupsDisplayableManager::CreateWidget(vtkMRMLMarkupsDisplayNode* markupsDisplayNode)
{
  vtkMRMLMarkupsNode* markupsNode = markupsDisplayNode->GetMarkupsNode();
  if (!markupsNode)
  {
    return nullptr;
  }

  vtkMRMLMarkupsAngleNode* angleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(markupsNode);
  vtkMRMLMarkupsFiducialNode* fiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(markupsNode);
  vtkMRMLMarkupsLineNode* lineNode = vtkMRMLMarkupsLineNode::SafeDownCast(markupsNode);
  vtkMRMLMarkupsCurveNode* curveNode = vtkMRMLMarkupsCurveNode::SafeDownCast(markupsNode);
  vtkMRMLMarkupsClosedCurveNode* closedCurveNode = vtkMRMLMarkupsClosedCurveNode::SafeDownCast(markupsNode);

  vtkSlicerAbstractWidget* widget = NULL;
  vtkSmartPointer<vtkSlicerAbstractRepresentation> rep = NULL;
  if (fiducialNode)
  {
    widget = vtkSlicerPointsWidget::New();
    if (this->Is2DDisplayableManager())
    {
      rep = vtkSmartPointer<vtkSlicerPointsRepresentation2D>::New();
    }
    else
    {
      rep = vtkSmartPointer<vtkSlicerPointsRepresentation3D>::New();
    }
  }
  else if (angleNode)
  {
    widget = vtkSlicerAngleWidget::New();
    if (this->Is2DDisplayableManager())
    {
      rep = vtkSmartPointer<vtkSlicerAngleRepresentation2D>::New();
    }
    else
    {
      rep = vtkSmartPointer<vtkSlicerAngleRepresentation3D>::New();
    }
  }
  else if (lineNode)
  {
    widget = vtkSlicerLineWidget::New();
    if (this->Is2DDisplayableManager())
    {
      rep = vtkSmartPointer<vtkSlicerLineRepresentation2D>::New();
    }
    else
    {
      rep = vtkSmartPointer<vtkSlicerLineRepresentation3D>::New();
    }
  }
  else if (curveNode)
  {
    widget = vtkSlicerCurveWidget::New();
    if (this->Is2DDisplayableManager())
    {
      rep = vtkSmartPointer<vtkSlicerCurveRepresentation2D>::New();
    }
    else
    {
      rep = vtkSmartPointer<vtkSlicerCurveRepresentation3D>::New();
    }
  }
  else if (closedCurveNode)
  {
    widget = vtkSlicerClosedCurveWidget::New();
    if (this->Is2DDisplayableManager())
    {
      rep = vtkSmartPointer<vtkSlicerCurveRepresentation2D>::New();
    }
    else
    {
      rep = vtkSmartPointer<vtkSlicerCurveRepresentation3D>::New();
    }
    rep->SetClosedLoop(true);
  }
  else
  {
    return nullptr;
  }

  widget->SetInteractor(this->GetInteractor());
  widget->SetCurrentRenderer(this->GetRenderer());

  // Set up representation
  rep->SetRenderer(this->GetRenderer());
  if (vtkSlicerAbstractRepresentation2D::SafeDownCast(rep))
  {
    vtkSlicerAbstractRepresentation2D::SafeDownCast(rep)->SetSliceNode(this->GetMRMLSliceNode());
  }

  rep->SetMarkupsDisplayNode(markupsDisplayNode);
  widget->SetRepresentation(rep);
  widget->On();

  return widget;
}



//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManager::ConvertDeviceToXYZ(double x, double y, double xyz[3])
{
  vtkMRMLAbstractSliceViewDisplayableManager::ConvertDeviceToXYZ(this->GetInteractor(), this->GetMRMLSliceNode(), x, y, xyz);
}
