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
