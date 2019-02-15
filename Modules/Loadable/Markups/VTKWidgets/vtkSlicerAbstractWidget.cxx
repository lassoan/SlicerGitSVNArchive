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
#include "vtkSlicerAbstractRepresentation2D.h"
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
#include "vtkTransform.h"
#include "vtkRenderWindow.h"

//----------------------------------------------------------------------
vtkSlicerAbstractWidget::vtkSlicerAbstractWidget()
{
  this->ManagesCursor    = 1;
  this->WidgetState      = vtkSlicerAbstractWidget::Idle;
  //this->CurrentHandle    = -1;
  this->FollowCursor     = false;
  this->RestrictFlag = RestrictNone;

  this->ManagesCursorOff();

  // Select = move control point
  // Translate = move entire widget

  this->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkEvent::NoModifier, vtkWidgetEvent::Select, vtkSlicerAbstractWidget::ControlPointMoveAction);
  this->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent, vtkEvent::AnyModifier, vtkWidgetEvent::EndSelect, vtkSlicerAbstractWidget::EndAction);

  this->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent, vtkEvent::NoModifier, vtkWidgetEvent::Translate, vtkSlicerAbstractWidget::TranslateAction);
  this->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent, vtkEvent::AnyModifier, vtkWidgetEvent::EndTranslate, vtkSlicerAbstractWidget::EndAction);

  //this->SetCallbackMethod(vtkCommand::RightButtonPressEvent, vtkEvent::NoModifier, vtkWidgetEvent::EndScale, vtkSlicerAbstractWidget::EndAction);
  //this->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent, vtkEvent::AnyModifier, vtkWidgetEvent::EndScale, vtkSlicerAbstractWidget::EndAction);

  this->SetCallbackMethod(vtkCommand::MouseMoveEvent, vtkEvent::AnyModifier, vtkWidgetEvent::Move3D, vtkSlicerAbstractWidget::MouseMoveAction);

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

  if (self->WidgetState != vtkSlicerAbstractWidget::OnWidget || !rep)
    {
    return;
    }

  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  vtkMRMLMarkupsDisplayNode* markupsDisplayNode = rep->GetMarkupsDisplayNode();
  int activeControlPoint = markupsDisplayNode->GetActiveControlPoint();
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

  if (self->WidgetState != vtkSlicerAbstractWidget::OnWidget || !rep)
    {
    return;
    }

  vtkMRMLMarkupsDisplayNode* markupsDisplayNode = rep->GetMarkupsDisplayNode();
  int activeControlPoint = markupsDisplayNode->GetActiveControlPoint();

  self->InvokeEvent(vtkCommand::PickEvent, &activeControlPoint);

  self->EventCallbackCommand->SetAbortFlag(1);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::RotateAction(vtkAbstractWidget *w)
{
  vtkSlicerAbstractWidget *self = reinterpret_cast<vtkSlicerAbstractWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if (self->WidgetState != vtkSlicerAbstractWidget::OnWidget || !rep)
    {
    return;
    }

  if (self->IsAnyControlPointLocked())
  {
    return;
  }

  vtkMRMLMarkupsDisplayNode* markupsDisplayNode = rep->GetMarkupsDisplayNode();
  int activeControlPoint = markupsDisplayNode->GetActiveControlPoint();
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

  if (self->WidgetState != vtkSlicerAbstractWidget::OnWidget || !rep)
  {
    return;
  }

  if (self->IsAnyControlPointLocked())
  {
    return;
  }

  vtkMRMLMarkupsDisplayNode* markupsDisplayNode = rep->GetMarkupsDisplayNode();
  int activeControlPoint = markupsDisplayNode->GetActiveControlPoint();
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

  if (self->WidgetState != vtkSlicerAbstractWidget::OnWidget || !rep)
  {
    return;
  }

  if (self->IsAnyControlPointLocked())
  {
    return;
  }

  vtkMRMLMarkupsDisplayNode* markupsDisplayNode = rep->GetMarkupsDisplayNode();
  int activeControlPoint = markupsDisplayNode->GetActiveControlPoint();
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
void vtkSlicerAbstractWidget::MouseMoveAction(vtkAbstractWidget *w)
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
  else if (state == Idle || state == OnWidget)
  {
    // update state
    const int* displayPosition = interactionEventData->GetDisplayPosition();
    const double* worldPosition = interactionEventData->GetWorldPosition();
    double closestDistance2 = 0.0;
    int componentIndex = -1;
    int foundComponent = rep->CanInteract(displayPosition, worldPosition, closestDistance2, componentIndex);
    if (foundComponent == vtkMRMLMarkupsDisplayNode::ComponentNone)
    {
      self->SetWidgetState(Idle);
    }
    else
    {
      self->SetWidgetState(OnWidget);
      if (foundComponent == vtkMRMLMarkupsDisplayNode::ComponentControlPoint)
      {
        rep->SetActiveNode(componentIndex);
      }
    }
  }
  else
  {
    // Process the motion
    // Based on the displacement vector (computed in display coordinates) and
    // the cursor state (which corresponds to which part of the widget has been
    // selected), the widget points are modified.
    // First construct a local coordinate system based on the display coordinates
    // of the widget.
    double eventPos[2]
    {
      static_cast<double>(interactionEventData->GetDisplayPosition()[0]),
      static_cast<double>(interactionEventData->GetDisplayPosition()[1]),
    };
    if (state == TranslateControlPoint)
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

    if (rep->GetClosedLoop())
    {
      rep->UpdateCentroid();
    }

    self->LastEventPosition[0] = eventPos[0];
    self->LastEventPosition[1] = eventPos[1];
  }

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

  if ((self->WidgetState != Define
    && self->WidgetState != OnWidget)
    || !rep)
    {
    return;
    }

  vtkMRMLMarkupsNode* markupsNode = rep->GetMarkupsNode();
  vtkMRMLMarkupsDisplayNode* markupsDisplayNode = rep->GetMarkupsDisplayNode();
  if (!markupsNode || !markupsDisplayNode)
  {
    return;
  }
  int controlPointToDelete = -1;
  if (self->WidgetState == Define)
  {
    controlPointToDelete = markupsNode->GetNumberOfControlPoints() - 2;
  }
  else if (self->WidgetState == OnWidget)
  {
    controlPointToDelete = markupsDisplayNode->GetActiveControlPoint();
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
    self->SetRestrictFlag(vtkSlicerAbstractWidget::RestrictToX);
    }
  else if (iren->GetKeyCode() == self->VerticalActiveKeyCode)
    {
    // RAS coordinates (Y <-> Z), this should take care also of transform?
    self->SetRestrictFlag(vtkSlicerAbstractWidget::RestrictToZ);
    }
  else if (iren->GetKeyCode() == self->DepthActiveKeyCode)
    {
    // RAS coordinates (Y <-> Z)
    self->SetRestrictFlag(vtkSlicerAbstractWidget::RestrictToY);
    }
  else if (iren->GetKeyCode() == 'q')
    {
    self->SetRestrictFlag(vtkSlicerAbstractWidget::RestrictNone);
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
bool vtkSlicerAbstractWidget::RemovePreviewPoint()
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
    {
    return false;
    }

  if (!this->FollowCursor)
  {
    return false;
  }
  rep->DeleteLastNode();
  this->FollowCursor = false;
  return true;
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

  int itemIndex;
  return this->GetRepresentation()->CanInteract(interactionEventData->GetDisplayPosition(), interactionEventData->GetWorldPosition(), distance2, itemIndex);
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidget::Leave()
{
  this->RemovePreviewPoint();
  vtkSlicerAbstractRepresentation *rep = reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);
  if (!rep)
    {
    return;
    }

  vtkMRMLMarkupsDisplayNode* markupsDisplayNode = rep->GetMarkupsDisplayNode();
  if (markupsDisplayNode)
  {
    markupsDisplayNode->SetActiveComponent(vtkMRMLMarkupsDisplayNode::ComponentNone, -1);
  }
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
    static_cast<double>(this->Interactor->GetEventPosition()[0]),
    static_cast<double>(this->Interactor->GetEventPosition()[1])
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
  vtkSlicerAbstractRepresentation2D* rep2d = vtkSlicerAbstractRepresentation2D::SafeDownCast(rep);
  if (rep2d)
  {
    // 2D view
    rep2d->GetSliceToWorldCoordinates(eventPos, worldPos);
  }
  else
  {
    // 3D view
    if (!rep->GetPointPlacer()->ComputeWorldPosition(this->CurrentRenderer,
      eventPos, oldWorldPos, worldPos,
      worldOrient))
    {
      return;
    }
  }

  if (this->RestrictFlag != vtkSlicerAbstractWidget::RestrictNone)
  {
    for (int i = 0; i < 3; ++i)
    {
      worldPos[i] = (this->RestrictFlag == (i + 1)) ? worldPos[i] : oldWorldPos[i];
    }
  }
  rep->SetActiveNodeToWorldPosition(worldPos);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::TranslateWidget(double eventPos[2])
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

  double ref[3] = { 0. };
  double worldPos[3], worldOrient[9];

  vtkSlicerAbstractRepresentation2D* rep2d = vtkSlicerAbstractRepresentation2D::SafeDownCast(rep);
  if (rep2d)
  {
    // 2D view
    double slicePos[3] = { 0. };
    slicePos[0] = this->LastEventPosition[0];
    slicePos[1] = this->LastEventPosition[1];
    rep2d->GetSliceToWorldCoordinates(slicePos, ref);

    slicePos[0] = eventPos[0];
    slicePos[1] = eventPos[1];
    rep2d->GetSliceToWorldCoordinates(slicePos, worldPos);
  }
  else
  {
    // 3D view
    double displayPos[2] = { 0. };

    displayPos[0] = this->LastEventPosition[0];
    displayPos[1] = this->LastEventPosition[1];

    if (!rep->GetPointPlacer()->ComputeWorldPosition(this->CurrentRenderer,
      eventPos, ref, worldPos,
      worldOrient))
    {
      return;
    }
    ref[0] = worldPos[0];
    ref[1] = worldPos[1];
    ref[2] = worldPos[2];

    displayPos[0] = eventPos[0];
    displayPos[1] = eventPos[1];

    if (!rep->GetPointPlacer()->ComputeWorldPosition(this->CurrentRenderer,
      displayPos, ref, worldPos,
      worldOrient))
    {
      return;
    }
  }


  double vector[3];
  vector[0] = worldPos[0] - ref[0];
  vector[1] = worldPos[1] - ref[1];
  vector[2] = worldPos[2] - ref[2];

  int wasModified = markupsNode->StartModify();
  for (int i = 0; i < rep->GetNumberOfNodes(); i++)
  {
    if (rep->GetNthNodeLocked(i))
    {
      continue;
    }

    rep->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
    {
      if (this->RestrictFlag != vtkSlicerAbstractWidget::RestrictNone)
      {
        worldPos[j] = (this->RestrictFlag == (j + 1)) ? ref[j] + vector[j] : ref[j];
      }
      else
      {
        worldPos[j] = ref[j] + vector[j];
      }
    }

    rep->SetNthNodeWorldPosition(i, worldPos);
  }
  markupsNode->EndModify(wasModified);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::ScaleWidget(double eventPos[2])
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

  double ref[3] = { 0. };
  double worldPos[3], worldOrient[9];

  double centroid[3];
  rep->GetTransformationReferencePoint(centroid);

  vtkSlicerAbstractRepresentation2D* rep2d = vtkSlicerAbstractRepresentation2D::SafeDownCast(rep);
  if (rep2d)
  {
    double slicePos[3] = { 0. };
    slicePos[0] = this->LastEventPosition[0];
    slicePos[1] = this->LastEventPosition[1];
    rep2d->GetSliceToWorldCoordinates(slicePos, ref);

    slicePos[0] = eventPos[0];
    slicePos[1] = eventPos[1];
    rep2d->GetSliceToWorldCoordinates(slicePos, worldPos);
  }
  else
  {
    double displayPos[2] = { 0. };
    displayPos[0] = this->LastEventPosition[0];
    displayPos[1] = this->LastEventPosition[1];
    if (rep->GetPointPlacer()->ComputeWorldPosition(this->CurrentRenderer,
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
    double r2 = vtkMath::Distance2BetweenPoints(ref, centroid);

    if (!rep->GetPointPlacer()->ComputeWorldPosition(this->CurrentRenderer,
      displayPos, ref, worldPos,
      worldOrient))
    {
      return;
    }

  }

  double r2 = vtkMath::Distance2BetweenPoints(ref, centroid);
  double d2 = vtkMath::Distance2BetweenPoints(worldPos, centroid);
  if (d2 < 0.0000001)
  {
    return;
  }

  double ratio = sqrt(d2 / r2);

  int wasModified = markupsNode->StartModify();
  for (int i = 0; i < rep->GetNumberOfNodes(); i++)
  {
    if (rep->GetNthNodeLocked(i))
    {
      continue;
    }

    rep->GetNthNodeWorldPosition(i, ref);
    for (int j = 0; j < 3; j++)
    {
      worldPos[j] = centroid[j] + ratio * (ref[j] - centroid[j]);
    }

    rep->SetNthNodeWorldPosition(i, worldPos);
  }
  markupsNode->EndModify(wasModified);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidget::RotateWidget(double eventPos[2])
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

  double ref[3] = { 0. };
  double lastWorldPos[3] = { 0. };
  double worldPos[3], worldOrient[9];

  double centroid[3];
  rep->GetTransformationReferencePoint(centroid);

  vtkSlicerAbstractRepresentation2D* rep2d = vtkSlicerAbstractRepresentation2D::SafeDownCast(rep);
  if (rep2d)
  {
    double slicePos[3] = { 0. };
    slicePos[0] = this->LastEventPosition[0];
    slicePos[1] = this->LastEventPosition[1];

    rep2d->GetSliceToWorldCoordinates(slicePos, lastWorldPos);

    slicePos[0] = eventPos[0];
    slicePos[1] = eventPos[1];
    rep2d->GetSliceToWorldCoordinates(slicePos, worldPos);


  }
  else
  {
    double displayPos[2] = { 0. };

    displayPos[0] = this->LastEventPosition[0];
    displayPos[1] = this->LastEventPosition[1];

    if (rep->GetPointPlacer()->ComputeWorldPosition(this->CurrentRenderer,
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

    if (!rep->GetPointPlacer()->ComputeWorldPosition(this->CurrentRenderer,
      displayPos, ref, worldPos,
      worldOrient))
    {
      return;
    }
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

  int wasModified = markupsNode->StartModify();
  for (int i = 0; i < rep->GetNumberOfNodes(); i++)
  {
    rep->GetNthNodeWorldPosition(i, ref);
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

    rep->SetNthNodeWorldPosition(i, worldPos);
  }
  markupsNode->EndModify(wasModified);
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


//----------------------------------------------------------------------
bool vtkSlicerAbstractWidget::IsAnyControlPointLocked()
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return false;
  }
  // If any node is locked return
  for (int i = 0; i < markupsNode->GetNumberOfControlPoints(); i++)
  {
    if (markupsNode->GetNthControlPointLocked(i))
    {
      return true;
    }
  }
  return false;
}

//-------------------------------------------------------------------------
int vtkSlicerAbstractWidget::AddPointFromWorldCoordinate(const double worldCoordinates[3])
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if (!rep)
  {
    return -1;
  }

  if (this->FollowCursor)
  {
    // convert point preview to final point
    rep->SetNthNodeWorldPosition(rep->GetNumberOfNodes() - 1, worldCoordinates);
    this->FollowCursor = false;
  }
  else
  {
    if (!rep->AddNodeAtWorldPosition(worldCoordinates))
    {
      return -1;
    }
  }
  rep->SetActiveNode(rep->GetNumberOfNodes() - 1);

  return rep->GetNumberOfNodes() - 1;
}
