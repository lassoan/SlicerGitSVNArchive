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

#include "vtkSlicerAbstractWidget.h"
#include "vtkSlicerAbstractRepresentation.h"
#include "vtkSliceViewInteractorStyle.h" // for vtkSlicerInteractionEventData
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkPointPlacer.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkSphereSource.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"

//----------------------------------------------------------------------
vtkSlicerAbstractWidget::vtkSlicerAbstractWidget()
{
  this->ManagesCursor    = 1;
  this->WidgetState      = vtkSlicerAbstractWidget::Idle;
  //this->CurrentHandle    = -1;
  this->FollowCursor     = false;
  this->ManagesCursorOff();

  // Select = move control point
  // Translate = move entire widget

  this->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::NoModifier, vtkWidgetEvent::Select, vtkSlicerAbstractWidget::ControlPointMoveAction);
  this->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent, vtkEvent::AnyModifier, vtkWidgetEvent::EndSelect, vtkSlicerAbstractWidget::EndAction);

  this->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent, vtkEvent::NoModifier, vtkWidgetEvent::Translate, vtkSlicerAbstractWidget::TranslateAction);
  this->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent, vtkEvent::AnyModifier, vtkWidgetEvent::EndTranslate, vtkSlicerAbstractWidget::EndAction);

  //this->SetCallbackMethod(vtkCommand::RightButtonPressEvent, vtkEvent::NoModifier, vtkWidgetEvent::EndScale, vtkSlicerAbstractWidget::EndAction);
  //this->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent, vtkEvent::AnyModifier, vtkWidgetEvent::EndScale, vtkSlicerAbstractWidget::EndAction);

  this->SetCallbackMethod(vtkCommand::MouseMoveEvent, vtkEvent::AnyModifier, vtkWidgetEvent::Move3D, vtkSlicerAbstractWidget::MoveAction);

  this->SetCallbackMethod(vtkCommand::LeftButtonDoubleClickEvent, vtkEvent::AnyModifier, vtkWidgetEvent::Pick, vtkSlicerAbstractWidget::PickAction);
  this->SetCallbackMethod(vtkCommand::MiddleButtonDoubleClickEvent, vtkEvent::AnyModifier, vtkWidgetEvent::Pick, vtkSlicerAbstractWidget::PickAction);
  this->SetCallbackMethod(vtkCommand::RightButtonDoubleClickEvent, vtkEvent::AnyModifier, vtkWidgetEvent::Pick, vtkSlicerAbstractWidget::PickAction);

  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::ShiftModifier, 127, 1, "Delete",
                                          vtkWidgetEvent::Reset,
                                          this, vtkSlicerAbstractWidget::ResetAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::NoModifier, 127, 1, "Delete",
                                          vtkWidgetEvent::Delete,
                                          this, vtkSlicerAbstractWidget::DeleteAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::NoModifier, 8, 1, "BackSpace",
                                          vtkWidgetEvent::Delete,
                                          this, vtkSlicerAbstractWidget::DeleteAction);

  this->HorizontalActiveKeyCode = 'x';
  this->VerticalActiveKeyCode = 'y';
  this->DepthActiveKeyCode = 'z';
  this->KeyEventCallbackCommand = vtkSmartPointer<vtkCallbackCommand>::New();
  this->KeyEventCallbackCommand->SetClientData(this);
  this->KeyEventCallbackCommand->SetCallback(vtkSlicerAbstractWidget::ProcessKeyEvents);

  this->Enabled = 1;
}

//----------------------------------------------------------------------
vtkSlicerAbstractWidget::~vtkSlicerAbstractWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetCallbackMethod(vtkEventData *edata, unsigned long widgetEvent, vtkWidgetCallbackMapper::CallbackType f)
{
  this->CallbackMapper->SetCallbackMethod(edata->GetType(), edata, widgetEvent, this, f);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetCallbackMethod(unsigned long interactionEvent, int modifiers,
  unsigned long widgetEvent, vtkWidgetCallbackMapper::CallbackType f)
{
  vtkNew<vtkSlicerInteractionEventData> ed;
  ed->SetType(interactionEvent);
  ed->SetModifiers(modifiers);
  this->SetCallbackMethod(ed, widgetEvent, f);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetCursor(int state)
{
  if (!this->ManagesCursor)
    {
    return;
    }

  switch (state)
    {
    case 0:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
      break;
    case 1:
      this->RequestCursorShape(VTK_CURSOR_HAND);
      break;
    default:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::CloseLoop()
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return;
    }

  if (!rep->GetClosedLoop() && rep->GetNumberOfNodes() > 1)
    {
    rep->ClosedLoopOn();
    this->Render();
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetEnabled(int enabling)
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!this->Interactor || !rep)
    {
    return;
    }

  if (enabling)
    {

    if (!this->CurrentRenderer)
      {
      vtkErrorMacro("vtkSlicerAbstractWidget::SetEnabled failed: current renderer has not been set");
      return;
      }

    // We're ready to enable
    this->Enabled = 1;

    // In usual VTK widgets , we would add event observers to the interactor
    // by calling this->EventTranslator->AddEventsToInteractor.
    // We do not do it here because we get the events from the displayable manager.

    rep->SetRenderer(this->CurrentRenderer);
    rep->VisibilityOn();
    this->CurrentRenderer->AddViewProp(rep);

    this->InvokeEvent(vtkCommand::EnableEvent, nullptr);
    }
  else
    {
    this->Enabled = 0;

    if (this->CurrentRenderer)
      {
      this->CurrentRenderer->RemoveViewProp(rep);
      }

    this->InvokeEvent(vtkCommand::DisableEvent, nullptr);
    }

  this->Superclass::SetEnabled(enabling);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::SetRepresentation(vtkSlicerAbstractRepresentation *rep)
{
  if (rep == nullptr || rep == this->WidgetRep)
    {
    return;
    }

  int wasWnabled = this->Enabled;
  if (this->Enabled)
    {
    this->SetEnabled(0);
    }

  if (this->WidgetRep)
    {
    this->WidgetRep->Delete();
    }
  this->WidgetRep = rep;
  if (this->WidgetRep)
    {
    this->WidgetRep->Register(this);
    }
  this->Modified();

  if (wasWnabled)
    {
    this->SetEnabled(1);
    }
}

//-------------------------------------------------------------------------
vtkSlicerAbstractRepresentation* vtkSlicerAbstractWidget::GetRepresentation()
{
  return reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::BuildRepresentation()
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return;
    }

  rep->BuildRepresentation();
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::BuildLocator()
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return;
    }

  rep->SetRebuildLocator(true);
}

// The following methods are the callbacks that the contour widget responds to.
//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ControlPointMoveAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState != vtkSlicerAbstractWidget::OnControlPoint ||
      !rep)
    {
    return;
    }

  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  int activeControlPoint = markupsNode->GetActiveControlPoint();
  if (activeControlPoint < 0)
    {
    return;
    }
  if (markupsNode->GetNthControlPointLocked(activeControlPoint))
    {
    return;
    }

  //self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
  //self->InvokeEvent(vtkCommand::StartInteractionEvent, &activeControlPoint);

  self->SetWidgetState(TranslateControlPoint);
  self->StartWidgetInteraction();
  self->EventCallbackCommand->SetAbortFlag(1);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::PickAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if ((self->WidgetState != vtkSlicerAbstractWidget::OnControlPoint
    && self->WidgetState != vtkSlicerAbstractWidget::OnWidget) ||
      !rep)
    {
    return;
    }

  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  int activeControlPoint = markupsNode->GetActiveControlPoint();

  self->InvokeEvent(vtkCommand::PickEvent, &activeControlPoint);

  self->EventCallbackCommand->SetAbortFlag(1);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::RotateAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if ((self->WidgetState != vtkSlicerAbstractWidget::OnControlPoint
    && self->WidgetState != vtkSlicerAbstractWidget::OnWidget) ||
    !rep)
    {
    return;
    }

  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  int activeControlPoint = markupsNode->GetActiveControlPoint();
  if (activeControlPoint < 0)
    {
    return;
    }

  //self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
  //self->InvokeEvent(vtkCommand::StartInteractionEvent, &activeControlPoint);

  self->SetWidgetState(Rotate);
  self->StartWidgetInteraction();
  self->EventCallbackCommand->SetAbortFlag(1);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ScaleAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if ((self->WidgetState != vtkSlicerAbstractWidget::OnControlPoint
    && self->WidgetState != vtkSlicerAbstractWidget::OnWidget) ||
    !rep)
  {
    return;
  }

  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  int activeControlPoint = markupsNode->GetActiveControlPoint();
  if (activeControlPoint < 0)
  {
    return;
  }

  //self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
  //self->InvokeEvent(vtkCommand::StartInteractionEvent, &activeControlPoint);

  self->SetWidgetState(Scale);
  self->StartWidgetInteraction();
  self->EventCallbackCommand->SetAbortFlag(1);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::TranslateAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if ((self->WidgetState != vtkSlicerAbstractWidget::OnControlPoint
    && self->WidgetState != vtkSlicerAbstractWidget::OnWidget) ||
    !rep)
  {
    return;
  }

  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  int activeControlPoint = markupsNode->GetActiveControlPoint();
  if (activeControlPoint < 0)
  {
    return;
  }

  //self->SetCursor(state != vtkSlicerAbstractRepresentation::Outside);
  //self->InvokeEvent(vtkCommand::StartInteractionEvent, &activeControlPoint);

  self->SetWidgetState(Translate);
  self->StartWidgetInteraction();
  self->EventCallbackCommand->SetAbortFlag(1);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (!rep)
  {
    return;
  }
  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }

  vtkEventData* eventData = reinterpret_cast<vtkEventData*>(self->CallData);
  vtkSlicerInteractionEventData* interactionEventData = vtkSlicerInteractionEventData::SafeDownCast(eventData);
  if (!interactionEventData)
    {
    // only 3D event types are supported
    return;
    }

  int state = self->WidgetState;

  if (state == vtkSlicerAbstractWidget::Define)
  {
    // update preview point position
    if (self->FollowCursor && rep->GetNumberOfNodes() > 0)
    {
      const int* displayPosition = interactionEventData->GetDisplayPosition();
      rep->SetNthNodeDisplayPosition(rep->GetNumberOfNodes() - 1, displayPosition[0], displayPosition[1]);
    }
  }
  else if (state == Idle || state == OnControlPoint || state == OnWidget)
  {
    // update state
    const int* displayPosition = interactionEventData->GetDisplayPosition();
    const double* worldPosition = interactionEventData->GetWorldPosition();
    double closestDistance2 = 0.0;
    int itemIndex = -1;
    int interaction = rep->CanInteract(displayPosition, worldPosition, closestDistance2, itemIndex);
    if (interaction == vtkSlicerAbstractRepresentation::InteractControlPoint)
    {
      markupsNode->SetActiveControlPoint(itemIndex);
      self->SetWidgetState(OnControlPoint);
    }
    else
    {
      markupsNode->SetActiveControlPoint(-1);
      if (interaction == vtkSlicerAbstractRepresentation::InteractCentroid
        || interaction == vtkSlicerAbstractRepresentation::InteractLine)
      {
        self->SetWidgetState(OnWidget);
      }
      else
      {
        self->SetWidgetState(Idle);
      }
    }
  }
  // Process the motion
  // Based on the displacement vector (computed in display coordinates) and
  // the cursor state (which corresponds to which part of the widget has been
  // selected), the widget points are modified.
  // First construct a local coordinate system based on the display coordinates
  // of the widget.
  else if (state == TranslateControlPoint)
  {
    self->TranslateNode(eventPos);
  }
  else if (state == Translate)
  {
    self->TranslateWidget(eventPos);
  }
  else if (state == Scale)
  {
    self->ScaleWidget(eventPos);
  }
  else if (state == Rotate)
  {
    self->RotateWidget(eventPos);
  }

  self->LastEventPosition[0] = eventPos[0];
  self->LastEventPosition[1] = eventPos[1];

  self->EventCallbackCommand->SetAbortFlag(1);
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::EndAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (!rep)
    {
    return;
    }

  if ((self->WidgetState != vtkSlicerAbstractWidget::TranslateControlPoint
    && self->WidgetState != vtkSlicerAbstractWidget::Translate
    && self->WidgetState != vtkSlicerAbstractWidget::Scale
    && self->WidgetState != vtkSlicerAbstractWidget::Rotate
    ) || !rep)
  {
    return;
  }

  //rep->SetActiveNode(-1);
  //self->SetCursor(0);
  //self->InvokeEvent(vtkCommand::EndInteractionEvent, &self->CurrentHandle);
  self->SetWidgetState(Idle);

  self->EndWidgetInteraction();
  self->EventCallbackCommand->SetAbortFlag(1);

  self->Render();
  self->SelectionButton = vtkAbstractWidget::None;
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ResetAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep = reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);
  if (!rep)
  {
    return;
  }
  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }
  markupsNode->RemoveAllControlPoints();
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::DeleteAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if ((!self->WidgetState != Define
    && !self->WidgetState != OnControlPoint)
    || !rep)
    {
    return;
    }

  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  if (!markupsNode)
    {
    return;
    }

  int controlPointToDelete = -1;
  if (self->WidgetState == Define)
  {
    controlPointToDelete = markupsNode->GetNumberOfControlPoints() - 2;
  }
  else if (self->WidgetState == OnControlPoint)
  {
    controlPointToDelete = markupsNode->GetActiveControlPoint();
  }

  if (controlPointToDelete >= 0 && controlPointToDelete < markupsNode->GetNumberOfControlPoints())
  {
    markupsNode->RemoveNthControlPoint(controlPointToDelete);
    self->InvokeEvent(vtkCommand::DeletePointEvent, &controlPointToDelete);
    self->SetCursor(0);
  }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ProcessKeyEvents(vtkObject* , unsigned long vtkNotUsed(event),
                                               void* clientdata, void*)
{
  vtkSlicerAbstractWidget *self = static_cast<vtkSlicerAbstractWidget*>(clientdata);
  vtkRenderWindowInteractor *iren = self->GetInteractor();
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (!rep || !iren)
    {
    return;
    }

  if (iren->GetKeyCode() == self->HorizontalActiveKeyCode)
    {
    rep->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictToX);
    }
  else if (iren->GetKeyCode() == self->VerticalActiveKeyCode)
    {
    // RAS coordinates (Y <-> Z), this should take care also of transform?
    rep->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictToZ);
    }
  else if (iren->GetKeyCode() == self->DepthActiveKeyCode)
    {
    // RAS coordinates (Y <-> Z)
    rep->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictToY);
    }
  else if (iren->GetKeyCode() == 'q')
    {
    rep->SetRestrictFlag(vtkSlicerAbstractRepresentation::RestrictNone);
    }
}

//-------------------------------------------------------------------------
int vtkSlicerAbstractWidget::AddPreviewPointToRepresentationFromWorldCoordinate(const double worldCoordinates[3])
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return -1;
    }

  this->FollowCursor = true;
  this->WidgetState = vtkSlicerAbstractWidget::Define;
  rep->AddNodeAtWorldPosition(worldCoordinates);

  return rep->GetNumberOfNodes() - 1;
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::UpdatePreviewPointPositionFromWorldCoordinate(const double worldCoordinates[3])
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return;
    }
  if (!this->FollowCursor)
    {
    this->AddPreviewPointToRepresentationFromWorldCoordinate(worldCoordinates);
    }
  else
    {
    rep->SetNthNodeWorldPosition(rep->GetNumberOfNodes()-1, worldCoordinates);
    }
}

//-------------------------------------------------------------------------
void vtkSlicerAbstractWidget::RemoveLastPreviewPointToRepresentation()
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return;
    }

  if (this->FollowCursor)
    {
    rep->DeleteLastNode();
    this->FollowCursor = false;
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WidgetState: " << this->WidgetState << endl;
  os << indent << "FollowCursor: " << (this->FollowCursor ? "On" : "Off") << endl;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidget::ProcessInteractionEvent(vtkEventData* eventData)
{
  // vtkCommand::Move3DEvent is used because that indicates there is eventData.
  // Event ID stored in eventData->EventType is used for further processing.
  vtkAbstractWidget::ProcessEventsHandler(NULL, vtkCommand::Move3DEvent, this, eventData);
}

//-----------------------------------------------------------------------------
bool vtkSlicerAbstractWidget::CanProcessInteractionEvent(vtkEventData* eventData, double &distance2)
{
  if (!this->GetRepresentation() || !this->GetInteractor())
    {
    return false;
    }

  // If we are currently translating then we interact everywhere
  if (this->WidgetState == TranslateControlPoint
    || this->WidgetState == Translate
    || this->WidgetState == Rotate
    || this->WidgetState == Scale)
    {
    distance2 = 0.0;
    return true;
    }

  vtkSlicerInteractionEventData* interactionEventData = vtkSlicerInteractionEventData::SafeDownCast(eventData);
  if (!interactionEventData)
  {
    // only 3D event types are supported for now
    return false;
  }

  return this->GetRepresentation()->CanInteract(interactionEventData->GetDisplayPosition(), interactionEventData->GetWorldPosition(), distance2);
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidget::Leave()
{
  this->RemoveLastPreviewPointToRepresentation();
  vtkSlicerAbstractRepresentation *rep = reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);
  if (!rep)
    {
    return;
    }

  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  markupsNode->SetActiveControlPoint(-1);
  this->SetCursor(0);
  //    slicerWidget->SetManagesCursor(false);
  this->SetWidgetState(Idle);
  this->Render();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::StartWidgetInteraction()
{
  vtkSlicerAbstractRepresentation *rep = reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);
  if (!rep)
  {
    return;
  }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartInteraction();
  double startEventPos[2]
    {
    this->Interactor->GetEventPosition()[0],
    this->Interactor->GetEventPosition()[1]
    };

  // How far is this in pixels from the position of this widget?
  // Maintain this during interaction such as translating (don't
  // force center of widget to snap to mouse position)

  // GetActiveNode position
  double pos[2] = { 0.0 };
  if (rep->GetActiveNodeDisplayPosition(pos))
  {
    // save offset
    this->StartEventOffsetPosition[0] = startEventPos[0] - pos[0];
    this->StartEventOffsetPosition[1] = startEventPos[1] - pos[1];
  }
  else
  {
    this->StartEventOffsetPosition[0] = 0;
    this->StartEventOffsetPosition[1] = 0;
  }

  // save also the cursor pos
  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::EndWidgetInteraction()
{
  this->ReleaseFocus();
  this->EndInteraction();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::TranslateNode(double eventPos[2])
{
  vtkSlicerAbstractRepresentation *rep = reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);
  if (!rep)
  {
    return;
  }
  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }

  eventPos[0] -= this->StartEventOffsetPosition[0];
  eventPos[1] -= this->StartEventOffsetPosition[1];

  double oldWorldPos[3];
  if (!rep->GetActiveNodeWorldPosition(oldWorldPos))
  {
    return;
  }

  double worldPos[3], worldOrient[9];
  if (rep->GetPointPlacer()->ComputeWorldPosition(this->CurrentRenderer,
    eventPos, oldWorldPos, worldPos,
    worldOrient))
  {
    if (this->RestrictFlag != vtkSlicerAbstractRepresentation::RestrictNone)
    {
      for (int i = 0; i < 3; ++i)
      {
        worldPos[i] = (this->RestrictFlag == (i + 1)) ? worldPos[i] : oldWorldPos[i];
      }
    }
    this->MarkupsNode->DisableModifiedEventOn();
    this->SetActiveNodeToWorldPosition(worldPos);
    this->MarkupsNode->DisableModifiedEventOff();
  }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::TranslateWidget(double eventPos[2])
{
  if (!this->MarkupsNode)
  {
    return;
  }

  double ref[3] = { 0. };
  double displayPos[2] = { 0. };
  double worldPos[3], worldOrient[9];

  displayPos[0] = this->LastEventPosition[0];
  displayPos[1] = this->LastEventPosition[1];

  if (this->PointPlacer->ComputeWorldPosition(this->Renderer,
    displayPos, ref, worldPos,
    worldOrient))
  {
    ref[0] = worldPos[0];
    ref[1] = worldPos[1];
    ref[2] = worldPos[2];
  }
  else
  {
    return;
  }

  displayPos[0] = eventPos[0];
  displayPos[1] = eventPos[1];

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
    displayPos, ref, worldPos,
    worldOrient))
  {
    return;
  }

  double vector[3];
  vector[0] = worldPos[0] - ref[0];
  vector[1] = worldPos[1] - ref[1];
  vector[2] = worldPos[2] - ref[2];

  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    if (this->GetNthNodeLocked(i))
    {
      continue;
    }

    this->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
    {
      if (this->RestrictFlag != vtkSlicerAbstractRepresentation::RestrictNone)
      {
        worldPos[j] = (this->RestrictFlag == (j + 1)) ? ref[j] + vector[j] : ref[j];
      }
      else
      {
        worldPos[j] = ref[j] + vector[j];
      }
    }

    this->SetNthNodeWorldPosition(i, worldPos);
  }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::ScaleWidget(double eventPos[2])
{
  if (!this->MarkupsNode)
  {
    return;
  }

  double ref[3] = { 0. };
  double displayPos[2] = { 0. };
  double worldPos[3], worldOrient[9];

  displayPos[0] = this->LastEventPosition[0];
  displayPos[1] = this->LastEventPosition[1];

  if (this->PointPlacer->ComputeWorldPosition(this->Renderer,
    displayPos, ref, worldPos,
    worldOrient))
  {
    for (int i = 0; i < 3; i++)
    {
      ref[i] = worldPos[i];
    }
  }
  else
  {
    return;
  }

  displayPos[0] = eventPos[0];
  displayPos[1] = eventPos[1];

  double centroid[3];
  this->UpdateCentroid();
  this->MarkupsNode->GetCentroidPosition(centroid);

  double r2 = vtkMath::Distance2BetweenPoints(ref, centroid);

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
    displayPos, ref, worldPos,
    worldOrient))
  {
    return;
  }
  double d2 = vtkMath::Distance2BetweenPoints(worldPos, centroid);
  if (d2 < 0.0000001)
  {
    return;
  }

  double ratio = sqrt(d2 / r2);

  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    if (this->GetNthNodeLocked(i))
    {
      continue;
    }

    this->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
    {
      worldPos[j] = centroid[j] + ratio * (ref[j] - centroid[j]);
    }

    this->SetNthNodeWorldPosition(i, worldPos);
  }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation3D::RotateWidget(double eventPos[2])
{
  if (!this->MarkupsNode)
  {
    return;
  }

  double ref[3] = { 0. };
  double displayPos[2] = { 0. };
  double lastWorldPos[3] = { 0. };
  double worldPos[3], worldOrient[9];

  displayPos[0] = this->LastEventPosition[0];
  displayPos[1] = this->LastEventPosition[1];

  if (this->PointPlacer->ComputeWorldPosition(this->Renderer,
    displayPos, ref, lastWorldPos,
    worldOrient))
  {
    for (int i = 0; i < 3; i++)
    {
      ref[i] = worldPos[i];
    }
  }
  else
  {
    return;
  }

  displayPos[0] = eventPos[0];
  displayPos[1] = eventPos[1];

  double centroid[3];
  this->UpdateCentroid();
  this->MarkupsNode->GetCentroidPosition(centroid);

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
    displayPos, ref, worldPos,
    worldOrient))
  {
    return;
  }

  double d2 = vtkMath::Distance2BetweenPoints(worldPos, centroid);
  if (d2 < 0.0000001)
  {
    return;
  }

  for (int i = 0; i < 3; i++)
  {
    lastWorldPos[i] -= centroid[i];
    worldPos[i] -= centroid[i];
  }
  double angle = -vtkMath::DegreesFromRadians
  (vtkMath::AngleBetweenVectors(lastWorldPos, worldPos));

  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    this->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
    {
      ref[j] -= centroid[j];
    }
    vtkNew<vtkTransform> RotateTransform;
    RotateTransform->RotateY(angle);
    RotateTransform->TransformPoint(ref, worldPos);

    for (int j = 0; j < 3; j++)
    {
      worldPos[j] += centroid[j];
    }

    this->SetNthNodeWorldPosition(i, worldPos);
  }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}








//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::TranslateNode(double eventPos[2])
{
  if (this->GetActiveNodeLocked() || !this->MarkupsNode)
  {
    return;
  }

  eventPos[0] -= this->StartEventOffsetPosition[0];
  eventPos[1] -= this->StartEventOffsetPosition[1];

  double worldPos[3];
  this->GetSliceToWorldCoordinates(eventPos, worldPos);

  if (this->RestrictFlag != vtkSlicerAbstractRepresentation::RestrictNone)
  {
    double oldWorldPos[3];
    this->GetActiveNodeWorldPosition(oldWorldPos);
    for (int i = 0; i < 3; ++i)
    {
      worldPos[i] = (this->RestrictFlag == (i + 1)) ? worldPos[i] : oldWorldPos[i];
    }
  }
  this->MarkupsNode->DisableModifiedEventOn();
  this->SetActiveNodeToWorldPosition(worldPos);
  this->MarkupsNode->DisableModifiedEventOff();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::TranslateWidget(double eventPos[2])
{
  if (!this->MarkupsNode)
  {
    return;
  }

  double ref[3] = { 0. };
  double slicePos[2] = { 0. };
  double worldPos[3] = { 0. };

  slicePos[0] = this->LastEventPosition[0];
  slicePos[1] = this->LastEventPosition[1];

  this->GetSliceToWorldCoordinates(slicePos, ref);

  slicePos[0] = eventPos[0];
  slicePos[1] = eventPos[1];

  this->GetSliceToWorldCoordinates(slicePos, worldPos);

  double vector[3];
  vector[0] = worldPos[0] - ref[0];
  vector[1] = worldPos[1] - ref[1];
  vector[2] = worldPos[2] - ref[2];

  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    if (this->GetNthNodeLocked(i))
    {
      continue;
    }

    this->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
    {
      if (this->RestrictFlag != vtkSlicerAbstractRepresentation::RestrictNone)
      {
        worldPos[j] = (this->RestrictFlag == (j + 1)) ? ref[j] + vector[j] : ref[j];
      }
      else
      {
        worldPos[j] = ref[j] + vector[j];
      }
    }

    this->SetNthNodeWorldPosition(i, worldPos);
  }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::ScaleWidget(double eventPos[2])
{
  if (!this->MarkupsNode)
  {
    return;
  }

  double ref[3] = { 0. };
  double slicePos[2] = { 0. };
  double worldPos[3] = { 0. };

  slicePos[0] = this->LastEventPosition[0];
  slicePos[1] = this->LastEventPosition[1];

  this->GetSliceToWorldCoordinates(slicePos, ref);

  slicePos[0] = eventPos[0];
  slicePos[1] = eventPos[1];

  double centroid[3];
  this->UpdateCentroid();
  this->MarkupsNode->GetCentroidPosition(centroid);

  double r2 = vtkMath::Distance2BetweenPoints(ref, centroid);

  this->GetSliceToWorldCoordinates(slicePos, worldPos);

  double d2 = vtkMath::Distance2BetweenPoints(worldPos, centroid);
  if (d2 < 0.0000001)
  {
    return;
  }

  double ratio = sqrt(d2 / r2);

  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    if (this->GetNthNodeLocked(i))
    {
      continue;
    }

    this->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
    {
      worldPos[j] = centroid[j] + ratio * (ref[j] - centroid[j]);
    }

    this->SetNthNodeWorldPosition(i, worldPos);
  }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::RotateWidget(double eventPos[2])
{
  if (!this->MarkupsNode)
  {
    return;
  }

  double ref[3] = { 0. };
  double slicePos[2] = { 0. };
  double worldPos[3] = { 0. };
  double lastWorldPos[3] = { 0. };

  slicePos[0] = this->LastEventPosition[0];
  slicePos[1] = this->LastEventPosition[1];

  this->GetSliceToWorldCoordinates(slicePos, lastWorldPos);

  slicePos[0] = eventPos[0];
  slicePos[1] = eventPos[1];

  double centroid[3];
  this->UpdateCentroid();
  this->MarkupsNode->GetCentroidPosition(centroid);

  this->GetSliceToWorldCoordinates(slicePos, worldPos);

  double d2 = vtkMath::Distance2BetweenPoints(worldPos, centroid);
  if (d2 < 0.0000001)
  {
    return;
  }

  for (int i = 0; i < 3; i++)
  {
    lastWorldPos[i] -= centroid[i];
    worldPos[i] -= centroid[i];
  }
  double angle = -vtkMath::DegreesFromRadians
  (vtkMath::AngleBetweenVectors(lastWorldPos, worldPos));

  this->MarkupsNode->DisableModifiedEventOn();
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    this->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
    {
      ref[j] -= centroid[j];
    }
    vtkNew<vtkTransform> RotateTransform;
    RotateTransform->RotateY(angle);
    RotateTransform->TransformPoint(ref, worldPos);

    for (int j = 0; j < 3; j++)
    {
      worldPos[j] += centroid[j];
    }

    this->SetNthNodeWorldPosition(i, worldPos);
  }
  this->MarkupsNode->DisableModifiedEventOff();
  this->MarkupsNode->Modified();
}

//----------------------------------------------------------------------
vtkMRMLMarkupsNode* vtkSlicerAbstractWidget::GetMarkupsNode()
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);
  if (!rep)
  {
    return nullptr;
  }
  return rep->GetMarkupsNode();
}
