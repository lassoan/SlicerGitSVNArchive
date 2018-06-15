/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceIntersectionWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSliceIntersectionWidget.h"

#include "vtkMRMLApplicationLogic.h"
#include "vtkSliceIntersectionRepresentation2D.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkSliceIntersectionWidget);

//----------------------------------------------------------------------------------
vtkSliceIntersectionWidget::vtkSliceIntersectionWidget()
{
  // Set the initial state
  this->WidgetState = vtkSliceIntersectionWidget::Start;

  this->SliceLogicsModifiedCommand->SetClientData(this);
  this->SliceLogicsModifiedCommand->SetCallback(vtkSliceIntersectionWidget::SliceLogicsModifiedCallback);

  this->ModifierActive = 0;

  // Okay, define the events for this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkSliceIntersectionWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkSliceIntersectionWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSliceIntersectionWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkWidgetEvent::ModifyEvent,
                                          this, vtkSliceIntersectionWidget::ModifyEventAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyReleaseEvent,
                                          vtkWidgetEvent::ModifyEvent,
                                          this, vtkSliceIntersectionWidget::ModifyEventAction);
}

//----------------------------------------------------------------------------------
vtkSliceIntersectionWidget::~vtkSliceIntersectionWidget()
{
  this->SetMRMLApplicationLogic(NULL);
}

//----------------------------------------------------------------------
void vtkSliceIntersectionWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------
void vtkSliceIntersectionWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep )
  {
    vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
    if (!mrmlAppLogic)
    {
      vtkWarningMacro("vtkSliceIntersectionWidget::CreateDefaultRepresentation failed: application logic invalid");
      return;
    }
    vtkSliceIntersectionRepresentation2D* newRep = vtkSliceIntersectionRepresentation2D::New();
    newRep->SetMRMLApplicationLogic(mrmlAppLogic);
    this->WidgetRep = newRep;
  }
}

//-------------------------------------------------------------------------
void vtkSliceIntersectionWidget::SetCursor(int cState)
{
  switch (cState)
  {
    case vtkSliceIntersectionRepresentation2D::StateRotate:
      this->RequestCursorShape(VTK_CURSOR_HAND);
      break;
    case vtkSliceIntersectionRepresentation2D::StateTranslate:
      this->RequestCursorShape(VTK_CURSOR_SIZEALL);
      break;
    case vtkSliceIntersectionRepresentation2D::StateOutside:
    default:
      this->RequestCursorShape(VTK_CURSOR_DEFAULT);
  }
}

//-------------------------------------------------------------------------
void vtkSliceIntersectionWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkSliceIntersectionWidget *self = reinterpret_cast<vtkSliceIntersectionWidget*>(w);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  self->ModifierActive = self->Interactor->GetControlKey();
  reinterpret_cast<vtkSliceIntersectionRepresentation2D*>(self->WidgetRep)->
    ComputeInteractionState(X, Y, self->ModifierActive);

  if ( self->WidgetRep->GetInteractionState() == vtkSliceIntersectionRepresentation2D::StateOutside )
  {
    return;
  }

  self->GrabFocus(self->EventCallbackCommand);
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(eventPos);

  // We are definitely selected
  self->WidgetState = vtkSliceIntersectionWidget::Active;
  self->SetCursor(self->WidgetRep->GetInteractionState());

  // Highlight as necessary
  self->WidgetRep->Highlight(1);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent,nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkSliceIntersectionWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkSliceIntersectionWidget *self = reinterpret_cast<vtkSliceIntersectionWidget*>(w);

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Set the cursor appropriately
  if ( self->WidgetState == vtkSliceIntersectionWidget::Start )
  {
    self->ModifierActive = self->Interactor->GetControlKey();
    int state = self->WidgetRep->GetInteractionState();
    reinterpret_cast<vtkSliceIntersectionRepresentation2D*>(self->WidgetRep)->
      ComputeInteractionState(X, Y, self->ModifierActive );
    self->SetCursor(self->WidgetRep->GetInteractionState());
    if ( state != self->WidgetRep->GetInteractionState() )
    {
      self->Render();
    }
    return;
  }

  if (!self->Interactor->GetControlKey())
  {
    return;
  }

  // Okay, adjust the representation
  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(eventPosition);

  // Got this event, we are finished
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,nullptr);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkSliceIntersectionWidget::ModifyEventAction(vtkAbstractWidget *w)
{
  vtkSliceIntersectionWidget *self = reinterpret_cast<vtkSliceIntersectionWidget*>(w);
  if ( self->WidgetState == vtkSliceIntersectionWidget::Start )
  {
    int modifierActive = self->Interactor->GetControlKey();
    if ( self->ModifierActive != modifierActive )
    {
      self->ModifierActive = modifierActive;
      int X = self->Interactor->GetEventPosition()[0];
      int Y = self->Interactor->GetEventPosition()[1];
      reinterpret_cast<vtkSliceIntersectionRepresentation2D*>(self->WidgetRep)->
        ComputeInteractionState(X, Y, self->ModifierActive );
      self->SetCursor(self->WidgetRep->GetInteractionState());
    }
  }
}


//-------------------------------------------------------------------------
void vtkSliceIntersectionWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkSliceIntersectionWidget *self = reinterpret_cast<vtkSliceIntersectionWidget*>(w);

  if ( self->WidgetState != vtkSliceIntersectionWidget::Active )
  {
    return;
  }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double eventPos[2];
  eventPos[0] = static_cast<double>(X);
  eventPos[1] = static_cast<double>(Y);
  self->WidgetRep->EndWidgetInteraction(eventPos);

  // return to initial state
  self->WidgetState = vtkSliceIntersectionWidget::Start;
  self->ModifierActive = 0;

  // Highlight as necessary
  self->WidgetRep->Highlight(0);

  // stop adjusting
  self->EventCallbackCommand->SetAbortFlag(1);
  self->ReleaseFocus();
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,nullptr);
  self->WidgetState = vtkSliceIntersectionWidget::Start;
  self->Render();
}

//----------------------------------------------------------------------------------
void vtkSliceIntersectionWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

}

//----------------------------------------------------------------------------------
vtkMRMLApplicationLogic* vtkSliceIntersectionWidget::GetMRMLApplicationLogic()
{
  return this->MRMLApplicationLogic;
}

//----------------------------------------------------------------------------------
void vtkSliceIntersectionWidget::SetMRMLApplicationLogic(vtkMRMLApplicationLogic* appLogic)
{
  if (appLogic == this->MRMLApplicationLogic)
  {
    return;
  }
  this->MRMLApplicationLogic = appLogic;
  vtkCollection* sliceLogics = NULL;
  if (appLogic)
  {
    sliceLogics = appLogic->GetSliceLogics();
  }
  if (sliceLogics != this->SliceLogics)
  {
    if (this->SliceLogics)
    {
      this->SliceLogics->RemoveObserver(this->SliceLogicsModifiedCommand);
    }
    if (sliceLogics)
    {
      sliceLogics->AddObserver(vtkCommand::ModifiedEvent, this->SliceLogicsModifiedCommand);
    }
    this->SliceLogics = sliceLogics;
  }
}

//----------------------------------------------------------------------------------
void vtkSliceIntersectionWidget::SliceLogicsModifiedCallback(vtkObject* caller, unsigned long eid, void* clientData, void* callData)
{
  vtkSliceIntersectionWidget* self = vtkSliceIntersectionWidget::SafeDownCast((vtkObject*)clientData);
  if (!self)
  {
    return;
  }
  vtkSliceIntersectionRepresentation2D* rep = vtkSliceIntersectionRepresentation2D::SafeDownCast(self->WidgetRep);
  if (rep)
  {
    rep->UpdateIntersectingSliceNodes();
  }
}

//----------------------------------------------------------------------------------
void vtkSliceIntersectionWidget::SetSliceNode(vtkMRMLSliceNode* sliceNode)
{
  vtkSliceIntersectionRepresentation2D* rep = this->GetSliceIntersectionRepresentation();
  if (!rep)
  {
    vtkErrorMacro("vtkSliceIntersectionWidget::SetSliceNode failed: invalid representation");
    return;
  }
  rep->SetSliceNode(sliceNode);
}

//----------------------------------------------------------------------------------
vtkMRMLSliceNode* vtkSliceIntersectionWidget::GetSliceNode()
{
  vtkSliceIntersectionRepresentation2D* rep = this->GetSliceIntersectionRepresentation();
  if (!rep)
  {
    vtkErrorMacro("vtkSliceIntersectionWidget::GetSliceNode failed: invalid representation");
    return NULL;
  }
  return rep->GetSliceNode();
}