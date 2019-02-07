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

// MarkupsModule/MRML includes
#include <vtkMRMLMarkupsAngleNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsAngleDisplayableManager2D.h"

// MarkupsModule/VTKWidgets includes
#include <vtkMarkupsGlyphSource2D.h>

// MRMLDisplayableManager includes
#include <vtkSliceViewInteractorStyle.h>

// MRML includes
#include <vtkMRMLApplicationLogic.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkAbstractWidget.h>
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
#include <vtkSlicerAngleWidget.h>
#include <vtkSlicerAngleRepresentation2D.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsAngleDisplayableManager2D);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsAngleDisplayableManager2D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsAngleWidgetCallback2D : public vtkCommand
{
public:
  static vtkMarkupsAngleWidgetCallback2D *New()
  { return new vtkMarkupsAngleWidgetCallback2D; }

  vtkMarkupsAngleWidgetCallback2D()
    : Widget(nullptr)
    , Node(nullptr)
    , DisplayableManager(nullptr)
    , LastInteractionEventMarkupIndex(-1)
    , SelectionButton(0)
    , PointMovedSinceStartInteraction(false)
  {
  }

  virtual void Execute (vtkObject *vtkNotUsed(caller), unsigned long event, void *callData)
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
    if (event ==  vtkCommand::PlacePointEvent)
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
      vtkSlicerAngleWidget* slicerAngleWidget = vtkSlicerAngleWidget::SafeDownCast(this->Widget);
      if (slicerAngleWidget)
        {
        this->SelectionButton = slicerAngleWidget->GetSelectionButton();
        }
      // no need to propagate to MRML, just notify external observers that the user selected a markup
      return;
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointEndInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (!this->PointMovedSinceStartInteraction)
        {
        if (this->SelectionButton == vtkSlicerAngleWidget::LeftButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::MiddleButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::RightButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRightClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::LeftButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::MiddleButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerAngleWidget::RightButtonDoubleClick)
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
  void SetDisplayableManager(vtkMRMLMarkupsDisplayableManager2D * dm)
    {
    this->DisplayableManager = dm;
    }

  vtkSlicerAbstractWidget * Widget;
  vtkMRMLMarkupsNode * Node;
  vtkMRMLMarkupsDisplayableManager2D * DisplayableManager;
  int LastInteractionEventMarkupIndex;
  int SelectionButton;
  bool PointMovedSinceStartInteraction;
};

//---------------------------------------------------------------------------
// vtkMRMLMarkupsAngleDisplayableManager2D methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget * vtkMRMLMarkupsAngleDisplayableManager2D::CreateWidget(vtkMRMLMarkupsNode* node)
{
  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return nullptr;
    }

  // 2d glyphs and text need to be scaled by 1/60 to show up properly in the 2d slice windows
  this->SetScaleFactor2D(0.01667);

  vtkMRMLMarkupsAngleNode* angleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(node);
  if (!angleNode)
    {
    return nullptr;
    }

  // widget
  vtkSlicerAngleWidget * slicerAngleWidget = vtkSlicerAngleWidget::New();

  if (this->GetInteractor()->GetPickingManager())
    {
    if (!(this->GetInteractor()->GetPickingManager()->GetEnabled()))
      {
      // Managed picking is on by default on the widget, but the interactor
      // will need to have it's picking manager turned on once widgets are
      // going to be used to avoid dragging points that are behind others.
      // Enabling it before setting the interactor on the widget seems to
      // work better with tests of two angles.
      this->GetInteractor()->GetPickingManager()->EnabledOn();
      }
    }

  slicerAngleWidget->SetInteractor(this->GetInteractor());
  slicerAngleWidget->SetCurrentRenderer(this->GetRenderer());
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode)
    {
    if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      slicerAngleWidget->SetManagesCursor(false);
      }
    else
      {
      slicerAngleWidget->SetManagesCursor(true);
      }
    }
  vtkDebugMacro("Fids CreateWidget: Created widget for node " << angleNode->GetID() << " with a representation");

  // Add the Representation
  vtkNew<vtkSlicerAngleRepresentation2D> rep;
  rep->SetRenderer(this->GetRenderer());
  rep->SetSliceNode(this->GetMRMLSliceNode());
  rep->SetMarkupsNode(angleNode);
  slicerAngleWidget->SetRepresentation(rep);
  slicerAngleWidget->On();

  return slicerAngleWidget;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node)
{
  if (!widget)
    {
    vtkErrorMacro("OnWidgetCreated: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("OnWidgetCreated: MRML node was null!")
    return;
    }

  // add the callback
  vtkMarkupsAngleWidgetCallback2D *myCallback = vtkMarkupsAngleWidgetCallback2D::New();
  myCallback->SetNode(node);
  myCallback->SetWidget(widget);
  myCallback->SetDisplayableManager(this);
  widget->AddObserver(vtkCommand::StartInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::EndInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::InteractionEvent,myCallback);
  widget->AddObserver(vtkCommand::PlacePointEvent, myCallback);
  widget->AddObserver(vtkCommand::DeletePointEvent, myCallback);
  myCallback->Delete();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnClickInRenderWindow(double x, double y,
                                                                    const char *associatedNodeID,
                                                                    int action /*= 0 */)
{
  if (!this->IsCorrectDisplayableManager())
    {
    // jump out
    vtkDebugMacro("OnClickInRenderWindow: x = " << x << ", y = " << y << ", incorrect displayable manager, focus = " << this->FocusStr << ", jumping out");
    return;
    }

  // Get World coordinates from the display ones
  double displayCoordinates[2], worldCoordinates[4];
  displayCoordinates[0] = x;
  displayCoordinates[1] = y;

  this->GetDisplayToWorldCoordinates(displayCoordinates, worldCoordinates);

  // Is there an active markups node that's a angle node?
  vtkMRMLMarkupsAngleNode *activeAngleNode = nullptr;
  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (selectionNode)
    {
    const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
    if (mrmlNode &&
        mrmlNode->IsA("vtkMRMLMarkupsAngleNode"))
      {
      activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(mrmlNode);
      }
    else
      {
      vtkDebugMacro("OnClickInRenderWindow: active markup id = "
            << (activeMarkupsID ? activeMarkupsID : "null")
            << ", mrml node is "
            << (mrmlNode ? mrmlNode->GetID() : "null")
            << ", not a vtkMRMLMarkupsAngleNode");
      }
    }

  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (!interactionNode)
    {
    return;
    }

  if (!activeAngleNode &&
      interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
    {
    // create the MRML node
    activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsAngleNode"));
    activeAngleNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("A"));
    activeAngleNode->AddDefaultStorageNode();
    activeAngleNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeAngleNode->GetID());
    }

  vtkSlicerAngleWidget *slicerWidget = vtkSlicerAngleWidget::SafeDownCast
    (this->Helper->GetWidget(activeAngleNode));
  if (slicerWidget == nullptr)
    {
    return;
    }

  // Check if the widget angle has been already place
  // if yes, create a new node.
  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate &&
      activeAngleNode->GetNumberOfPoints() < 3)
    {
    slicerWidget->SetWidgetState(vtkSlicerAngleWidget::Define);
    slicerWidget->SetFollowCursor(true);
    slicerWidget->SetManagesCursor(false);
    }

  if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate)
    {
    activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsAngleNode"));
    activeAngleNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("A"));
    activeAngleNode->AddDefaultStorageNode();
    activeAngleNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeAngleNode->GetID());
    slicerWidget = vtkSlicerAngleWidget::SafeDownCast
      (this->Helper->GetWidget(activeAngleNode));
    if (slicerWidget == nullptr)
      {
      return;
      }
    }

  // save for undo
  this->GetMRMLScene()->SaveStateForUndo();

  if (action == vtkMRMLMarkupsAngleDisplayableManager2D::AddPoint)
    {
    int pointIndex = slicerWidget->AddPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeAngleNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsAngleDisplayableManager2D::AddPreview)
    {
    int pointIndex = slicerWidget->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeAngleNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }
  else if (action == vtkMRMLMarkupsAngleDisplayableManager2D::RemovePreview)
    {
    slicerWidget->RemoveLastPreviewPointToRepresentation();
    }

  // if this was a one time place, go back to view transform mode
  if (interactionNode->GetPlaceModePersistence() == 0 &&
      slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate)
    {
    interactionNode->SwitchToViewTransformMode();
    }

  // if persistence and last widget is placed, add new markups and a previewPoint
  if (interactionNode->GetPlaceModePersistence() == 1 &&
      interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place &&
      action == vtkMRMLMarkupsAngleDisplayableManager2D::AddPoint &&
      slicerWidget->GetWidgetState() == vtkSlicerAngleWidget::Manipulate)
    {
    activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast
      (this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLMarkupsAngleNode"));
    activeAngleNode->SetName(this->GetMRMLScene()->GetUniqueNameByString("A"));
    activeAngleNode->AddDefaultStorageNode();
    activeAngleNode->CreateDefaultDisplayNodes();
    selectionNode->SetActivePlaceNodeID(activeAngleNode->GetID());
    slicerWidget = vtkSlicerAngleWidget::SafeDownCast
      (this->Helper->GetWidget(activeAngleNode));
    if (slicerWidget == nullptr)
      {
      return;
      }
    int pointIndex = slicerWidget->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
    // is there a node associated with this?
    if (associatedNodeID)
      {
      activeAngleNode->SetNthPointAssociatedNodeID(pointIndex, associatedNodeID);
      }
    }

  // force update of widgets on other views
  activeAngleNode->GetMarkupsDisplayNode()->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnMRMLSceneEndClose()
{
  // make sure to delete widgets and projections
  this->Superclass::OnMRMLSceneEndClose();

  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}
