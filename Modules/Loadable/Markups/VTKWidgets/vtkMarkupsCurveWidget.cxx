/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsCurveWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarkupsCurveWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkMarkupsSplineRepresentation.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkHandleWidget.h"

vtkStandardNewMacro(vtkMarkupsCurveWidget);
//----------------------------------------------------------------------------
vtkMarkupsCurveWidget::vtkMarkupsCurveWidget()
{
  this->WidgetState = vtkMarkupsWidget::Idle;
  this->ManagesCursor = 1;

  // Define widget events
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkMarkupsCurveWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkMarkupsCurveWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
                                          vtkWidgetEvent::Translate,
                                          this, vtkMarkupsCurveWidget::TranslateAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
                                          vtkWidgetEvent::EndTranslate,
                                          this, vtkMarkupsCurveWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Scale,
                                          this, vtkMarkupsCurveWidget::ScaleAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
                                          vtkWidgetEvent::EndScale,
                                          this, vtkMarkupsCurveWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkMarkupsCurveWidget::MoveAction);
}

//----------------------------------------------------------------------------
vtkMarkupsCurveWidget::~vtkMarkupsCurveWidget()
{
}

//----------------------------------------------------------------------
void vtkMarkupsCurveWidget::SelectAction(vtkAbstractWidget *w)
{
  // We are in a static method, cast to ourself
  vtkMarkupsCurveWidget *self = vtkMarkupsCurveWidget::SafeDownCast(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer ||
    !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkMarkupsWidget::Idle;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkMarkupsSplineRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkMarkupsWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);

  if (interactionState == vtkMarkupsSplineRepresentation::OnLine &&
    self->Interactor->GetControlKey())
  {
    // Add point.
    reinterpret_cast<vtkMarkupsSplineRepresentation*>(self->WidgetRep)->
      SetInteractionState(vtkMarkupsSplineRepresentation::Inserting);
  }
  else if (interactionState == vtkMarkupsSplineRepresentation::OnHandle &&
    self->Interactor->GetShiftKey())
  {
    // remove point.
    reinterpret_cast<vtkMarkupsSplineRepresentation*>(self->WidgetRep)->
      SetInteractionState(vtkMarkupsSplineRepresentation::Erasing);
  }
  else
  {
    reinterpret_cast<vtkMarkupsSplineRepresentation*>(self->WidgetRep)->
      SetInteractionState(vtkMarkupsSplineRepresentation::Moving);
  }

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkMarkupsCurveWidget::TranslateAction(vtkAbstractWidget *w)
{
  // Not sure this should be any different that SelectAction
  vtkMarkupsCurveWidget::SelectAction(w);
}

//----------------------------------------------------------------------
void vtkMarkupsCurveWidget::ScaleAction(vtkAbstractWidget *w)
{
  // We are in a static method, cast to ourself
  vtkMarkupsCurveWidget *self = reinterpret_cast<vtkMarkupsCurveWidget*>(w);

  // Get the event position
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, make sure that the pick is in the current renderer
  if (!self->CurrentRenderer ||
    !self->CurrentRenderer->IsInViewport(X, Y))
  {
    self->WidgetState = vtkMarkupsWidget::Idle;
    return;
  }

  // Begin the widget interaction which has the side effect of setting the
  // interaction state.
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->StartWidgetInteraction(e);
  int interactionState = self->WidgetRep->GetInteractionState();
  if (interactionState == vtkMarkupsSplineRepresentation::Outside)
  {
    return;
  }

  // We are definitely selected
  self->WidgetState = vtkMarkupsWidget::Active;
  self->GrabFocus(self->EventCallbackCommand);
  //Scale
  reinterpret_cast<vtkMarkupsSplineRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkMarkupsSplineRepresentation::Scaling);

  // start the interaction
  self->EventCallbackCommand->SetAbortFlag(1);
  self->StartInteraction();
  self->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkMarkupsCurveWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkMarkupsCurveWidget *self = reinterpret_cast<vtkMarkupsCurveWidget*>(w);

  // See whether we're active
  if (self->WidgetState == vtkMarkupsWidget::Idle)
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(e);

  // moving something
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkMarkupsCurveWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkMarkupsCurveWidget *self = reinterpret_cast<vtkMarkupsCurveWidget*>(w);
  if (self->WidgetState == vtkMarkupsWidget::Idle)
  {
    return;
  }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // Okay, adjust the representation
  double e[2];
  e[0] = static_cast<double>(X);
  e[1] = static_cast<double>(Y);

  self->WidgetRep->EndWidgetInteraction(e);

  // Return state to not active
  self->WidgetState = vtkMarkupsWidget::Idle;
  reinterpret_cast<vtkMarkupsSplineRepresentation*>(self->WidgetRep)->
    SetInteractionState(vtkMarkupsSplineRepresentation::Outside);
  self->ReleaseFocus();

  self->EventCallbackCommand->SetAbortFlag(1);
  self->EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent, NULL);
  self->Render();
}

//----------------------------------------------------------------------
void vtkMarkupsCurveWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    vtkNew<vtkMarkupsSplineRepresentation> rep;
    this->SetWidgetRepresentation(rep.GetPointer());
  }
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}