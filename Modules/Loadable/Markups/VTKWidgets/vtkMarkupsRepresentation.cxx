/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarkupsRepresentation.h"

#include "vtkHandleRepresentation.h"
#include "vtkPlaneSource.h"

vtkCxxSetObjectMacro(vtkMarkupsRepresentation, ProjectionPlaneSource, vtkPlaneSource);

vtkCxxSetObjectMacro(vtkMarkupsRepresentation, HandleRepresentation, vtkHandleRepresentation);

//----------------------------------------------------------------------------
vtkMarkupsRepresentation::vtkMarkupsRepresentation()
{
  this->Tolerance = 5;
  this->HandleRepresentation = NULL;
  this->ProjectToPlane = 0;  //default off
  this->ProjectionPlaneSource = NULL;
  this->ActiveHandle = -1;
}

//----------------------------------------------------------------------------
vtkMarkupsRepresentation::~vtkMarkupsRepresentation()
{
  this->SetProjectionPlaneSource(NULL);
  this->SetHandleRepresentation(NULL);
}

/*
//----------------------------------------------------------------------------
void vtkMarkupsRepresentation::ProjectPointsToOrthoPlane()
{
  double ctr[3];
  int numberOfHandles = this->Handles.size();
  for (int i = 0; i < numberOfHandles; ++i)
    {
    this->HandleGeometry[i]->GetCenter(ctr);
    ctr[this->ProjectionNormal] = this->ProjectionPosition;
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
    }
}
*/

//----------------------------------------------------------------------------
void vtkMarkupsRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Project To Plane: "
     << (this->ProjectToPlane ? "On" : "Off") << "\n";
}

//----------------------------------------------------------------------
int vtkMarkupsRepresentation::CreateHandle(double* worldPosition, double* displayPosition)
{
  vtkHandleRepresentation *rep = this->GetHandleRepresentation(
    static_cast<int>(this->Handles.size()));
  if (rep == NULL)
  {
    vtkErrorMacro("CreateHandle: no handle representation set yet! Cannot create a new handle.");
    return -1;
  }

  if (displayPosition)
  {
    double pos[3] = { displayPosition[0], displayPosition[1], 0 };
    rep->SetDisplayPosition(pos);
    rep->SetTolerance(this->Tolerance); //needed to ensure that picking is consistent
  }
  else if (worldPosition)
  {
    rep->SetWorldPosition(worldPosition);
  }

  this->ActiveHandle = static_cast<int>(this->Handles.size()) - 1;
  return this->ActiveHandle;
}

//----------------------------------------------------------------------
void vtkMarkupsRepresentation::RemoveLastHandle()
{
  if (this->Handles.size() < 1)
  {
    return;
  }

  // Delete last handle
  this->Handles.pop_back();
}

//----------------------------------------------------------------------
void vtkMarkupsRepresentation::RemoveHandle(int n)
{
  // Remove nth handle
  if (n<0 || static_cast<int>(this->Handles.size()) <= n)
  {
    return;
  }

  if (n == this->ActiveHandle)
    {
    this->ActiveHandle = -1;
    }

  HandleListType::iterator iter = this->Handles.begin();
  std::advance(iter, n);
  this->Handles.erase(iter);
}

//----------------------------------------------------------------------
void vtkMarkupsRepresentation::RemoveActiveHandle()
{
  this->RemoveHandle(this->ActiveHandle);
  this->ActiveHandle = -1;
}

//----------------------------------------------------------------------
vtkHandleRepresentation *vtkMarkupsRepresentation::GetHandleRepresentation(unsigned int num)
{
  if (num < this->Handles.size())
  {
    return this->Handles[num];
  }

  //create one
  if (this->HandleRepresentation == NULL)
  {
    vtkErrorMacro("GetHandleRepresentation " << num << ", no handle representation has been set yet, cannot create a new handle.");
    return NULL;
  }
  vtkHandleRepresentation *rep = this->HandleRepresentation->NewInstance();
  rep->DeepCopy(this->HandleRepresentation);
  this->Handles.push_back(rep);
  return rep;
}

//----------------------------------------------------------------------
void vtkMarkupsRepresentation::GetHandleWorldPosition(unsigned int handleNum, double pos[3])
{
  if (handleNum >= this->Handles.size())
  {
    vtkErrorMacro("Trying to access non-existent handle");
    return;
  }
  HandleListType::iterator iter = this->Handles.begin();
  std::advance(iter, handleNum);
  (*iter)->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkMarkupsRepresentation::SetHandleDisplayPosition(unsigned int handleNum, double pos[3])
{
  if (handleNum >= this->Handles.size())
  {
    vtkErrorMacro("Trying to access non-existent handle");
    return;
  }
  HandleListType::iterator iter = this->Handles.begin();
  std::advance(iter, handleNum);
  (*iter)->SetDisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkMarkupsRepresentation::GetHandleDisplayPosition(unsigned int handleNum, double pos[3])
{
  if (handleNum >= this->Handles.size())
  {
    vtkErrorMacro("Trying to access non-existent handle");
    return;
  }
  HandleListType::iterator iter = this->Handles.begin();
  std::advance(iter, handleNum);
  (*iter)->GetDisplayPosition(pos);
}

//----------------------------------------------------------------------
int vtkMarkupsRepresentation::GetNumberOfHandles()
{
  return static_cast<int>(this->Handles.size());
}
