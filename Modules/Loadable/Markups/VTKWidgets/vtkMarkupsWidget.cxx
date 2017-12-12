/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarkupsWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkCoordinate.h"
#include "vtkEvent.h"
#include "vtkHandleRepresentation.h"
#include "vtkHandleWidget.h"
#include "vtkMarkupsRepresentation.h"
#include "vtkMarkupsPointListRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include <iterator>
#include <list>

//----------------------------------------------------------------------
vtkMarkupsWidget::vtkMarkupsWidget()
{
  this->ManagesCursor = 1;
  this->WidgetState = vtkMarkupsWidget::Idle;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::AddPoint,
                                          this, vtkMarkupsWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkMarkupsWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkMarkupsWidget::EndSelectAction);
}

//----------------------------------------------------------------------
int vtkMarkupsWidget::AddHandle(double* worldPosition, double* displayPosition)
{
  vtkMarkupsRepresentation *rep = vtkMarkupsRepresentation::SafeDownCast(this->WidgetRep);
  if (!rep)
  {
    vtkErrorMacro(<< "Please set, or create a widget representation "
      << "before adding requesting creation of a new handle.");
    return -1;
  }

  // Add a new handle to the representation
  int currentHandleNumber = rep->CreateHandle(worldPosition, displayPosition);
  vtkHandleRepresentation *handleRep = rep->GetHandle(currentHandleNumber);
  if (!handleRep)
  {
    vtkErrorMacro("vtkMarkupsWidget::AddHandle: failed to create a new handle");
    return NULL;
  }
  handleRep->SetRenderer(this->CurrentRenderer);

  // Add a new handle to the widget
  vtkNew<vtkHandleWidget> widget;
  widget->SetParent(this);
  widget->SetInteractor(this->Interactor);
  widget->SetRepresentation(handleRep);
  widget->SetEnabled(1);
  this->Handles.push_back(widget.GetPointer());
  return currentHandleNumber;
}

//----------------------------------------------------------------------
void vtkMarkupsWidget::RemoveHandle(int i)
{
  if (i<0)
    {
    return;
    }
  if(static_cast< size_t >(i) >= this->Handles.size())
    {
    return;
    }

  HandleWidgetList::iterator iter = this->Handles.begin();
  std::advance(iter,i);
  (*iter)->SetEnabled(0);
  (*iter)->RemoveObservers(vtkCommand::StartInteractionEvent);
  (*iter)->RemoveObservers(vtkCommand::InteractionEvent);
  (*iter)->RemoveObservers(vtkCommand::EndInteractionEvent);
  this->Handles.erase(iter);

  vtkMarkupsRepresentation *rep = vtkMarkupsRepresentation::SafeDownCast(this->WidgetRep);
  if (rep)
    {
    rep->RemoveHandle(i);
    }
}

//----------------------------------------------------------------------
vtkMarkupsWidget::~vtkMarkupsWidget()
{
  // Loop over all handles releasing their observers and deleting them
  while(!this->Handles.empty())
    {
    this->RemoveHandle(static_cast<int>(this->Handles.size())-1);
    }
}

//----------------------------------------------------------------------
vtkHandleWidget * vtkMarkupsWidget::GetHandle(int i)
{
 if(this->Handles.size() <= static_cast< size_t >(i))
   {
   return NULL;
   }
  HandleWidgetList::iterator iter = this->Handles.begin();
  std::advance(iter,i);
  return *iter;
}

//----------------------------------------------------------------------
void vtkMarkupsWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);

  HandleWidgetList::iterator iter;
  for (iter = this->Handles.begin(); iter != this->Handles.end(); ++iter)
    {
    (*iter)->SetEnabled(enabling);
    }

  if (!enabling)
    {
    this->RequestCursorShape(VTK_CURSOR_DEFAULT);
    this->WidgetState = vtkMarkupsWidget::Idle;
    }

  this->Render();
}

// The following methods are the callbacks that the markups widget responds to.
//-------------------------------------------------------------------------
void vtkMarkupsWidget::AddPointAction(vtkAbstractWidget *w)
{
  vtkMarkupsWidget *self = reinterpret_cast<vtkMarkupsWidget*>(w);

  // Need to distinguish between placing handles and manipulating handles
  if (self->WidgetState == vtkMarkupsWidget::MovingHandle)
    {
    return;
    }

  // compute some info we need for all cases
  int x = self->Interactor->GetEventPosition()[0];
  int y = self->Interactor->GetEventPosition()[1];

  // When a handle is placed, a new handle widget must be created and enabled.
  int state = self->WidgetRep->ComputeInteractionState(x,y);
  if (state == vtkMarkupsRepresentation::NearHandle)
    {
    self->WidgetState = vtkMarkupsWidget::MovingHandle;

    // Invoke an event on ourself for the handles
    self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    self->Superclass::StartInteraction();
    vtkMarkupsRepresentation *rep = static_cast<
      vtkMarkupsRepresentation * >(self->WidgetRep);
    int handleIdx = rep->GetActiveHandle();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, &handleIdx);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->Render();
    }

  // Point placement is managed externally, using vtkMRMLMarkupsDisplayableManager::OnClickInRenderWindowGetCoordinates()
  // the widget does not have point placement mode.

}

//-------------------------------------------------------------------------
void vtkMarkupsWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkMarkupsWidget *self = reinterpret_cast<vtkMarkupsWidget*>(w);

  // Do nothing if outside
  /*if (self->WidgetState != vtkMarkupsWidget::MovingHandleWhileIdle
    && self->WidgetState != vtkMarkupsWidget::MovingHandleWhilePlacing)
    {
    return;
    }*/

  // else we are moving a handle

  self->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);

  // set the cursor shape to a hand if we are near a handle.
  int x = self->Interactor->GetEventPosition()[0];
  int y = self->Interactor->GetEventPosition()[1];
  int state = self->WidgetRep->ComputeInteractionState(x, y);

  // Change the cursor shape to a hand and invoke an interaction event if we
  // are near the handle
  if (state == vtkMarkupsRepresentation::NearHandle)
    {
    self->RequestCursorShape(VTK_CURSOR_HAND);

    vtkMarkupsRepresentation *rep = static_cast< vtkMarkupsRepresentation* >(self->WidgetRep);
    int handleIdx = rep->GetActiveHandle();
    self->InvokeEvent(vtkCommand::InteractionEvent, &handleIdx);

    self->EventCallbackCommand->SetAbortFlag(1);
    }
  else
    {
    self->RequestCursorShape(VTK_CURSOR_DEFAULT);
    }

  self->Render();
}

//-------------------------------------------------------------------------
void vtkMarkupsWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkMarkupsWidget *self = reinterpret_cast<vtkMarkupsWidget*>(w);

  // Do nothing if outside
  /*if (self->WidgetState != vtkMarkupsWidget::MovingHandle)
    {
    return;
    }*/
  if (self->WidgetState != vtkMarkupsWidget::MovingHandle)
    {
    return;
    }

  // Revert back to the mode we were in prior to selection.
  self->WidgetState = vtkMarkupsWidget::Idle;

  // Invoke event for handle
  self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Superclass::EndInteraction();
  self->Render();
}

//----------------------------------------------------------------------
void vtkMarkupsWidget::SetProcessEvents(int pe)
{
  this->Superclass::SetProcessEvents(pe);

  HandleWidgetList::iterator iter = this->Handles.begin();
  for (; iter != this->Handles.end(); ++iter)
    {
    (*iter)->SetProcessEvents(pe);
    }
}

//----------------------------------------------------------------------
void vtkMarkupsWidget::SetInteractor(vtkRenderWindowInteractor *rwi)
{
  this->Superclass::SetInteractor(rwi);
  HandleWidgetList::iterator iter = this->Handles.begin();
  for (; iter != this->Handles.end(); ++iter)
    {
    (*iter)->SetInteractor(rwi);
    }
}

//----------------------------------------------------------------------
void vtkMarkupsWidget::SetCurrentRenderer(vtkRenderer *ren)
{
  this->Superclass::SetCurrentRenderer(ren);
  HandleWidgetList::iterator iter = this->Handles.begin();
  for (; iter != this->Handles.end(); ++iter)
    {
    if (!ren)
      {
      // Disable widget if its being removed from the the renderer
      (*iter)->EnabledOff();
      }
    (*iter)->SetCurrentRenderer(ren);
    }
}

//----------------------------------------------------------------------
void vtkMarkupsWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WidgetState: " << this->WidgetState << endl;
}


//----------------------------------------------------------------------
void vtkMarkupsWidget::SetRepresentation(vtkMarkupsRepresentation* widgetRep)
{
  this->SetWidgetRepresentation(widgetRep);
}

//----------------------------------------------------------------------
void vtkMarkupsWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
    {
    vtkNew<vtkMarkupsPointListRepresentation> rep;
    this->SetWidgetRepresentation(rep.GetPointer());
    }
}
