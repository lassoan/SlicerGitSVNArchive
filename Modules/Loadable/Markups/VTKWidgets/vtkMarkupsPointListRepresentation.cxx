/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsPointListRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarkupsPointListRepresentation.h"

#include "vtkActor2D.h"
#include "vtkCoordinate.h"
#include "vtkHandleRepresentation.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include <iterator>
#include <list>

vtkStandardNewMacro(vtkMarkupsPointListRepresentation);

//----------------------------------------------------------------------
vtkMarkupsPointListRepresentation::vtkMarkupsPointListRepresentation()
{
}

//----------------------------------------------------------------------
vtkMarkupsPointListRepresentation::~vtkMarkupsPointListRepresentation()
{
}

//----------------------------------------------------------------------
int vtkMarkupsPointListRepresentation::
ComputeInteractionState(int vtkNotUsed(X), int vtkNotUsed(Y), int vtkNotUsed(modify))
{
  // Loop over all the handles to see if the point is close to any of them.
  int i;
  HandleListType::iterator iter;
  for ( i = 0, iter = this->Handles.begin(); iter != this->Handles.end(); ++iter, ++i )
    {
    if ( *iter != NULL )
      {
      if ( (*iter)->GetInteractionState() != vtkHandleRepresentation::Outside )
        {
        this->ActiveHandle = i;
        this->InteractionState = vtkMarkupsRepresentation::NearHandle;
        return this->InteractionState;
        }
      }
    }

  // Nothing found, so it's outside
  this->InteractionState = vtkMarkupsRepresentation::Outside;
  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkMarkupsPointListRepresentation::BuildRepresentation()
{
  if ( this->ActiveHandle >=0 && this->ActiveHandle < static_cast<int>(this->Handles.size()) )
    {
    vtkHandleRepresentation *rep = this->GetHandleRepresentation(this->ActiveHandle);
    if ( rep )
      {
      rep->BuildRepresentation();
      }
    }
}

//----------------------------------------------------------------------
void vtkMarkupsPointListRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Tolerance: " << this->Tolerance <<"\n";
}
