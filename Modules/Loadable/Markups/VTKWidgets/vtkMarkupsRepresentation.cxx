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

vtkStandardNewMacro(vtkMarkupsRepresentation);

vtkCxxSetObjectMacro(vtkMarkupsRepresentation, ProjectionPlaneSource, vtkPlaneSource);

vtkCxxSetObjectMacro(vtkMarkupsRepresentation, HandleRepresentation, vtkHandleRepresentation);

//----------------------------------------------------------------------------
vtkMarkupsRepresentation::vtkMarkupsRepresentation()
{
  this->UseDisplayPosition = false;
  this->Tolerance = 5;
  this->HandleRepresentation = NULL;
  this->ProjectToPlane = 0;  //default off
  this->ProjectionPlaneSource = NULL;
  this->ActiveHandle = -1;
  this->InteractionState = vtkMarkupsRepresentation::Outside;
  this->LastEventPosition[0] = VTK_DOUBLE_MAX;
  this->LastEventPosition[1] = VTK_DOUBLE_MAX;
  this->LastEventPosition[2] = VTK_DOUBLE_MAX;
  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkMarkupsRepresentation::~vtkMarkupsRepresentation()
{
  this->SetProjectionPlaneSource(NULL);
  this->SetHandleRepresentation(NULL);
}

//----------------------------------------------------------------------------
void vtkMarkupsRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Tolerance: " << this->Tolerance <<"\n";
  os << indent << "Project To Plane: "
     << (this->ProjectToPlane ? "On" : "Off") << "\n";
}

//----------------------------------------------------------------------
int vtkMarkupsRepresentation::CreateHandle(double* position)
{
  return this->InsertHandle(this->Handles.size(), position);
}

//----------------------------------------------------------------------
int vtkMarkupsRepresentation::InsertHandle(int n, double* position)
{
  // Add a new handle at the end
  int insertedHandleIndex = this->Handles.size();
  vtkHandleRepresentation *rep = this->GetHandleRepresentation(insertedHandleIndex);
  if (rep == NULL)
  {
    vtkErrorMacro("InsertHandle: no handle representation set yet! Cannot create a new handle.");
    return -1;
  }

  // TODO: clean this up. It is not nice that we create new handle representation at the end and then
  // we reorder
  if (n != insertedHandleIndex)
  {
    vtkSmartPointer<vtkHandleRepresentation> tmp = this->Handles[insertedHandleIndex];
    this->Handles.pop_back();
    this->Handles.insert(this->Handles.begin()+n, tmp);
  }

  if (this->UseDisplayPosition)
  {
    double pos[3] = { position[0], position[1], 0 };
    rep->SetDisplayPosition(pos);
    rep->SetTolerance(this->Tolerance); //needed to ensure that picking is consistent
  }
  else
  {
    rep->SetWorldPosition(position);
  }

  this->ActiveHandle = n;
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
  if (n < 0 || n >= this->Handles.size())
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
vtkHandleRepresentation *vtkMarkupsRepresentation::GetHandleRepresentation(unsigned int n)
{
  if (n < 0)
    {
    return NULL;
    }
  if (n >= this->Handles.size())
    {
    //create new handle
    if (this->HandleRepresentation == NULL)
      {
      vtkErrorMacro("GetHandleRepresentation " << n << ", no handle representation has been set yet, cannot create a new handle.");
      return NULL;
      }
    while (n >= this->Handles.size())
      {
      vtkSmartPointer<vtkHandleRepresentation> rep = vtkSmartPointer<vtkHandleRepresentation>::Take(this->HandleRepresentation->NewInstance());
      rep->DeepCopy(this->HandleRepresentation);
      this->Handles.push_back(rep);
      }
    }

  // existing handle
  return this->Handles[n];
}

//----------------------------------------------------------------------
int vtkMarkupsRepresentation::GetNumberOfHandles()
{
  return static_cast<int>(this->Handles.size());
}

//----------------------------------------------------------------------
int vtkMarkupsRepresentation::
ComputeInteractionState(int vtkNotUsed(X), int vtkNotUsed(Y), int vtkNotUsed(modify))
{
  // Loop over all the handles to see if the point is close to any of them.
  int i;
  HandleListType::iterator iter;
  for ( i = 0, iter = this->Handles.begin(); iter != this->Handles.end(); ++iter, ++i )
    {
    if (*iter == NULL)
      {
      continue;
      }
    if ( (*iter)->GetInteractionState() != vtkHandleRepresentation::Outside )
      {
      this->ActiveHandle = i;
      this->InteractionState = vtkMarkupsRepresentation::OnHandle;
      return this->InteractionState;
      }
    }

  // Nothing found, so it's outside
  this->InteractionState = vtkMarkupsRepresentation::Outside;
  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkMarkupsRepresentation::BuildRepresentation()
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


//----------------------------------------------------------------------------
void vtkMarkupsRepresentation::Translate(double *p1, double *p2)
{
  // Get the motion vector
  double v[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };

  double newCtr[3];
  for ( int i = 0; i < this->Handles.size(); ++i )
    {
    this->GetHandlePosition(i, newCtr);
    newCtr[0] += v[0];
    newCtr[1] += v[1];
    newCtr[2] += v[2];
    this->SetHandlePosition(i, newCtr);
    }
}

//----------------------------------------------------------------------------
void vtkMarkupsRepresentation::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
{
  int numberOfHandles = this->Handles.size();
  if (numberOfHandles < 2)
  {
    return;
  }

  // Compute center position and average distance from center
  double center[3] = {0.0,0.0,0.0};
  double avgdist = 0.0;
  double *prevctr = NULL;
  for (int i = 0; i < numberOfHandles; ++i )
    {
    double *ctr = this->GetHandlePosition(i);
    center[0] += ctr[0];
    center[1] += ctr[1];
    center[2] += ctr[2];
    if (prevctr)
      {
      avgdist += sqrt(vtkMath::Distance2BetweenPoints(ctr, prevctr));
      }
    prevctr = ctr;
    }
  avgdist /= numberOfHandles;
  center[0] /= numberOfHandles;
  center[1] /= numberOfHandles;
  center[2] /= numberOfHandles;

  // Get the motion vector
  double v[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / avgdist;
  if ( Y > this->LastEventPosition[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }

  // Move the handle points
  double newCtr[3];
  for (int i = 0; i < this->Handles.size(); ++i )
    {
    double* ctr = GetHandlePosition(i);
    for ( int j = 0; j < 3; ++j )
      {
      newCtr[j] = sf * (ctr[j] - center[j]) + center[j];
      }
    this->Handles[i]->SetWorldPosition(newCtr);
    }
}

//----------------------------------------------------------------------------
void vtkMarkupsRepresentation::Spin(double *p1, double *p2, double *vpn)
{
  // Mouse motion vector in world space
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Axis of rotation
  double axis[3] = {0.0,0.0,0.0};

  if (this->ProjectToPlane && this->ProjectionPlaneSource != NULL)
    {
    double* normal = this->ProjectionPlaneSource->GetNormal();
    axis[0] = normal[0];
    axis[1] = normal[1];
    axis[2] = normal[2];
    vtkMath::Normalize( axis );
    }
  else
    {
  // Create axis of rotation and angle of rotation
    vtkMath::Cross(vpn,v,axis);
    if ( vtkMath::Normalize(axis) == 0.0 )
      {
      return;
      }
    }

  // Radius vector (from mean center to cursor position)
  double rv[3] = {p2[0] - this->Centroid[0],
                  p2[1] - this->Centroid[1],
                  p2[2] - this->Centroid[2]};

  // Distance between center and cursor location
  double rs = vtkMath::Normalize(rv);

  // Spin direction
  double ax_cross_rv[3];
  vtkMath::Cross(axis,rv,ax_cross_rv);

  // Spin angle
  double theta = 360.0 * vtkMath::Dot(v,ax_cross_rv) / rs;

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(this->Centroid[0],this->Centroid[1],this->Centroid[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-this->Centroid[0],-this->Centroid[1],-this->Centroid[2]);

  // Set the handle points
  double newCtr[3];
  double ctr[3];
  for ( int i = 0; i < this->Handles.size(); ++i )
    {
    this->GetHandlePosition(i, ctr);
    this->Transform->TransformPoint(ctr,newCtr);
    this->SetHandlePosition(i, newCtr);
    }
}

//----------------------------------------------------------------------------
void vtkMarkupsRepresentation::CalculateCentroid()
{
  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;

  int numberOfHandles = this->Handles.size();
  if (numberOfHandles < 1)
  {
    return;
  }

  for (int i = 0; i < numberOfHandles; ++i)
  {
    double* ctr = this->GetHandlePosition(i);
    this->Centroid[0] += ctr[0];
    this->Centroid[1] += ctr[1];
    this->Centroid[2] += ctr[2];
  }

  this->Centroid[0] /= numberOfHandles;
  this->Centroid[1] /= numberOfHandles;
  this->Centroid[2] /= numberOfHandles;
}

//----------------------------------------------------------------------------
void vtkMarkupsRepresentation::GetHandlePosition(unsigned int handleNum, double pos[3])
{
  if (this->UseDisplayPosition)
  {
    this->Handles[handleNum]->GetDisplayPosition(pos);
  }
  else
  {
    this->Handles[handleNum]->GetWorldPosition(pos);
  }
}

//----------------------------------------------------------------------------
double* vtkMarkupsRepresentation::GetHandlePosition(unsigned int handleNum)
{
  if (this->UseDisplayPosition)
  {
    return this->Handles[handleNum]->GetDisplayPosition();
  }
  else
  {
    return this->Handles[handleNum]->GetWorldPosition();
  }
}

//----------------------------------------------------------------------------
void vtkMarkupsRepresentation::SetHandlePosition(unsigned int handleNum, double pos[3])
{
  if (this->UseDisplayPosition)
  {
    this->Handles[handleNum]->SetDisplayPosition(pos);
  }
  else
  {
    this->Handles[handleNum]->SetWorldPosition(pos);
  }
}

/*
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
*/

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
