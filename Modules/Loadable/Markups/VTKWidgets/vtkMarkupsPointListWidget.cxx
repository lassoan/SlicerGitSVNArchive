/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsPointListWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarkupsPointListWidget.h"

/*
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkCoordinate.h"
#include "vtkEvent.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include <iterator>
#include <list>
*/

#include "vtkMarkupsPointListRepresentation.h"

#include "vtkHandleRepresentation.h"
#include "vtkHandleWidget.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMarkupsPointListWidget);

//----------------------------------------------------------------------
vtkMarkupsPointListWidget::vtkMarkupsPointListWidget()
{
}

//----------------------------------------------------------------------
int vtkMarkupsPointListWidget::AddHandle(double* worldPosition, double* displayPosition)
{
  vtkMarkupsPointListRepresentation *rep = vtkMarkupsPointListRepresentation::SafeDownCast(this->WidgetRep);
  if (!rep)
    {
    vtkErrorMacro(<< "Please set, or create a widget representation "
        << "before adding requesting creation of a new handle.");
    return -1;
    }

  // Create the handle widget or reuse an old one
  vtkNew<vtkHandleWidget> widget;

  // Configure the handle widget
  widget->SetParent(this);
  widget->SetInteractor(this->Interactor);

  int currentHandleNumber = -1;
  vtkHandleRepresentation *handleRep = NULL;
  if (displayPosition)
    {
    currentHandleNumber = rep->CreateHandle(displayPosition);
    handleRep = rep->GetHandle(currentHandleNumber);
    }
  else
    {
    currentHandleNumber = static_cast<int>(this->Handles.size());
    // GetHandleRepresentation creates a new one representation if called with non-existent handle number
    handleRep = rep->GetHandleRepresentation(currentHandleNumber);
    if (worldPosition != NULL)
      {
      handleRep->SetWorldPosition(worldPosition);
      }
    }
  if (!handleRep)
    {
    return NULL;
    }
  handleRep->SetRenderer(this->CurrentRenderer);
  widget->SetRepresentation(handleRep);
  widget->SetEnabled(1);
  this->Handles.push_back(widget.GetPointer());
  return currentHandleNumber;
}

//----------------------------------------------------------------------
void vtkMarkupsPointListWidget::RemoveHandle(int i)
{
  this->Superclass::RemoveHandle(i);

  vtkMarkupsPointListRepresentation *rep = vtkMarkupsPointListRepresentation::SafeDownCast(this->WidgetRep);
  if (rep)
    {
    rep->RemoveHandle(i);
    }
}

//----------------------------------------------------------------------
vtkMarkupsPointListWidget::~vtkMarkupsPointListWidget()
{
}

//----------------------------------------------------------------------
void vtkMarkupsPointListWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
    {
    vtkNew<vtkMarkupsPointListRepresentation> rep;
    this->SetWidgetRepresentation(rep.GetPointer());
    }
}

//----------------------------------------------------------------------
void vtkMarkupsPointListWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
