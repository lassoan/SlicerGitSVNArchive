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
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsFiducialDisplayableManager3D.h"

// MarkupsModule/VTKWidgets includes
#include <vtkMarkupsGlyphSource2D.h>

// MRMLDisplayableManager includes
#include <vtkSliceViewInteractorStyle.h>

// MRML includes
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkSlicerAbstractWidget.h>
#include <vtkFollower.h>
#include <vtkHandleRepresentation.h>
#include <vtkInteractorStyle.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOrientedPolygonalHandleRepresentation3D.h>
#include <vtkPickingManager.h>
#include <vtkProperty2D.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSlicerPointsWidget.h>
#include <vtkSmartPointer.h>
#include <vtkSlicerPointsRepresentation3D.h>
#include <vtkSphereSource.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsFiducialDisplayableManager3D);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsFiducialDisplayableManager3D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsFiducialWidgetCallback3D : public vtkCommand
{
public:
  static vtkMarkupsFiducialWidgetCallback3D *New()
  { return new vtkMarkupsFiducialWidgetCallback3D; }

  vtkMarkupsFiducialWidgetCallback3D()
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
      vtkSlicerPointsWidget* slicerPointsWidget = vtkSlicerPointsWidget::SafeDownCast(this->Widget);
      if (slicerPointsWidget)
        {
        this->SelectionButton = slicerPointsWidget->GetSelectionButton();
        }
      // no need to propagate to MRML, just notify external observers that the user selected a markup
      return;
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointEndInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (!this->PointMovedSinceStartInteraction)
        {
        if (this->SelectionButton == vtkSlicerPointsWidget::LeftButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerPointsWidget::MiddleButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerPointsWidget::RightButton)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointRightClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerPointsWidget::LeftButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointLeftDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerPointsWidget::MiddleButtonDoubleClick)
          {
          this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointMiddleDoubleClickedEvent, &this->LastInteractionEventMarkupIndex);
          }
        else if (this->SelectionButton == vtkSlicerPointsWidget::RightButtonDoubleClick)
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
    else if (event == vtkCommand::PlacePointEvent)
      {
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointAddedEvent, &this->LastInteractionEventMarkupIndex);
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
  void SetDisplayableManager(vtkMRMLMarkupsDisplayableManager3D * dm)
    {
    this->DisplayableManager = dm;
    }

  vtkSlicerAbstractWidget * Widget;
  vtkMRMLMarkupsNode * Node;
  vtkMRMLMarkupsDisplayableManager3D * DisplayableManager;
  int LastInteractionEventMarkupIndex;
  int SelectionButton;
  bool PointMovedSinceStartInteraction;
};

//---------------------------------------------------------------------------
// vtkMRMLMarkupsFiducialDisplayableManager3D methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
/// Create a new widget.
vtkSlicerAbstractWidget * vtkMRMLMarkupsFiducialDisplayableManager3D::CreateWidget(vtkMRMLMarkupsNode* node)
{
  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return nullptr;
    }

  vtkMRMLMarkupsFiducialNode* fiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);

  if (!fiducialNode)
    {
    vtkErrorMacro("CreateWidget: Could not get fiducial node!")
    return nullptr;
    }

  //SlicerPoints widget
  vtkSlicerPointsWidget * slicerPointsWidget = vtkSlicerPointsWidget::New();

  if (this->GetInteractor()->GetPickingManager())
    {
    if (!(this->GetInteractor()->GetPickingManager()->GetEnabled()))
      {
      // Managed picking is on by default on the widget, but the interactor
      // will need to have it's picking manager turned on once widgets are
      // going to be used to avoid dragging points that are behind others.
      // Enabling it before setting the interactor on the widget seems to
      // work better with tests of two fiducial lists.
      this->GetInteractor()->GetPickingManager()->EnabledOn();
      }
    }

  slicerPointsWidget->SetInteractor(this->GetInteractor());
  slicerPointsWidget->SetCurrentRenderer(this->GetRenderer());
  slicerPointsWidget->On();
  vtkDebugMacro("Fids CreateWidget: Created widget for node " << fiducialNode->GetID() << " with a representation");

  // Add the Representation
  vtkNew<vtkSlicerPointsRepresentation3D> rep;
  rep->SetRenderer(this->GetRenderer());
  rep->SetMarkupsNode(fiducialNode);
  slicerPointsWidget->SetRepresentation(rep);

  return slicerPointsWidget;
  }

//---------------------------------------------------------------------------
/// Tear down the widget creation
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node)
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
  vtkMarkupsFiducialWidgetCallback3D *myCallback = vtkMarkupsFiducialWidgetCallback3D::New();
  myCallback->SetNode(node);
  myCallback->SetWidget(widget);
  myCallback->SetDisplayableManager(this);
  widget->AddObserver(vtkCommand::StartInteractionEvent,myCallback);
  widget->AddObserver(vtkCommand::EndInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::InteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::PlacePointEvent, myCallback);
  widget->AddObserver(vtkCommand::DeletePointEvent, myCallback);
  myCallback->Delete();
}

//---------------------------------------------------------------------------
/// observe key press events
void vtkMRMLMarkupsFiducialDisplayableManager3D::AdditionnalInitializeStep()
{
  // don't add the key press event, as it triggers a crash on start up
  //vtkDebugMacro("Adding an observer on the key press event");
  this->AddInteractorStyleObservableEvent(vtkCommand::KeyPressEvent);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnInteractorStyleEvent(int eventid)
{
  this->Superclass::OnInteractorStyleEvent(eventid);

  if (this->GetDisableInteractorStyleEventsProcessing())
    {
    vtkWarningMacro("OnInteractorStyleEvent: Processing of events was disabled.")
    return;
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
        this->OnClickInRenderWindowGetCoordinates();
        }
      else
        {
        vtkDebugMacro("Fiducial DisplayableManager: key press p, but not in Place mode! Returning.");
        return;
        }
      }
    }
  else if (eventid == vtkCommand::KeyReleaseEvent)
    {
    vtkDebugMacro("Got a key release event");
    }
}

//---------------------------------------------------------------------------
/// Create a markupsMRMLnode
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnClickInRenderWindow(double x, double y, const char *associatedNodeID)
{
  if (!this->IsCorrectDisplayableManager())
    {
    // jump out
    vtkDebugMacro("OnClickInRenderWindow: x = " << x << ", y = " << y << ", incorrect displayable manager, focus = " << this->Focus << ", jumping out");
    return;
    }

  // place the seed where the user clicked
  vtkDebugMacro("OnClickInRenderWindow: placing seed at " << x << ", " << y);

  // Get World coordinates from the display ones
  double displayCoordinates[2], worldCoordinates[4];
  displayCoordinates[0] = x;
  displayCoordinates[1] = y;

  this->GetDisplayToWorldCoordinates(displayCoordinates, worldCoordinates);

  // Is there an active markups node that's a fiducial node?
  vtkMRMLMarkupsFiducialNode *activeFiducialNode = nullptr;

  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (selectionNode)
    {
    const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
    if (mrmlNode &&
        mrmlNode->IsA("vtkMRMLMarkupsFiducialNode"))
      {
      activeFiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(mrmlNode);
      }
    else
      {
      vtkDebugMacro("OnClickInRenderWindow: active markup id = "
            << (activeMarkupsID ? activeMarkupsID : "null")
            << ", mrml node is "
            << (mrmlNode ? mrmlNode->GetID() : "null")
            << ", not a vtkMRMLMarkupsFiducialNode");
      }
    }

  bool newNode = false;
  if (!activeFiducialNode)
    {
    newNode = true;
    // create the MRML node
    activeFiducialNode = vtkMRMLMarkupsFiducialNode::New();
    activeFiducialNode->SetName("F");
    }

  // if this was a one time place, go back to view transform mode
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode && interactionNode->GetPlaceModePersistence() != 1)
    {
    vtkDebugMacro("End of one time place, place mode persistence = " << interactionNode->GetPlaceModePersistence());
    interactionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::ViewTransform);
    }

  // save for undo and add the node to the scene after any reset of the
  // interaction node so that don't end up back in place mode
  this->GetMRMLScene()->SaveStateForUndo();

  int fiducialIndex = 0;
  if (newNode)
    {
    // create a display node and add node and display node to scene
    vtkMRMLMarkupsDisplayNode *displayNode = vtkMRMLMarkupsDisplayNode::New();
    this->GetMRMLScene()->AddNode(displayNode);
    // let the logic know that it needs to set it to defaults
    displayNode->InvokeEvent(vtkMRMLMarkupsDisplayNode::ResetToDefaultsEvent);

    activeFiducialNode->AddAndObserveDisplayNodeID(displayNode->GetID());
    this->GetMRMLScene()->AddNode(activeFiducialNode);

    // save it as the active markups list
    if (selectionNode)
      {
      selectionNode->SetActivePlaceNodeID(activeFiducialNode->GetID());
      }

    // Add Control Point
    fiducialIndex = this->AddControlPoint(activeFiducialNode, worldCoordinates);

    // clean up
    displayNode->Delete();
    activeFiducialNode->Delete();
    }
  else
    {
    fiducialIndex = this->AddControlPoint(activeFiducialNode, worldCoordinates);
    }

  // is there a node associated with this?
  if (associatedNodeID)
    {
    activeFiducialNode->SetNthFiducialAssociatedNodeID(fiducialIndex, associatedNodeID);
    }
}

//---------------------------------------------------------------------------
int vtkMRMLMarkupsFiducialDisplayableManager3D::AddControlPoint(vtkMRMLMarkupsFiducialNode *markupsNode,
                                                                double worldCoordinates[4])
{
  vtkSlicerPointsWidget *slicerWidget = vtkSlicerPointsWidget::SafeDownCast
    (this->Helper->GetWidget(markupsNode));
  if (slicerWidget == nullptr)
    {
    return -1;
    }

  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  bool persistence = false;
  if (interactionNode && interactionNode->GetPlaceModePersistence() == 1 &&
      interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
    {
    persistence = true;
    }
  slicerWidget->AddPointToRepresentationFromWorldCoordinate(worldCoordinates, persistence);

  return markupsNode->GetNumberOfFiducials() - 1;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager3D::OnMRMLSceneEndClose()
{
  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}
