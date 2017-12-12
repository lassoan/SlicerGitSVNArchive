/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsCurveRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarkupsCurveRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBoundingBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkHandleRepresentation.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"

//----------------------------------------------------------------------------
vtkMarkupsCurveRepresentation::vtkMarkupsCurveRepresentation()
{
  this->LastEventPosition[0] = VTK_DOUBLE_MAX;
  this->LastEventPosition[1] = VTK_DOUBLE_MAX;
  this->LastEventPosition[2] = VTK_DOUBLE_MAX;

  this->Bounds[0] =  VTK_DOUBLE_MAX;
  this->Bounds[1] = -VTK_DOUBLE_MAX;
  this->Bounds[2] =  VTK_DOUBLE_MAX;
  this->Bounds[3] = -VTK_DOUBLE_MAX;
  this->Bounds[4] =  VTK_DOUBLE_MAX;
  this->Bounds[5] = -VTK_DOUBLE_MAX;

  this->Closed = 0;

  // Default bounds to get started
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };

  this->LineActor = vtkSmartPointer<vtkActor>::New();
  this->LinePicker = vtkSmartPointer<vtkCellPicker>::New();
  this->LinePicker->SetTolerance(0.01);
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->PickFromListOn();

  this->LastPickPosition[0] = VTK_DOUBLE_MAX;
  this->LastPickPosition[1] = VTK_DOUBLE_MAX;
  this->LastPickPosition[2] = VTK_DOUBLE_MAX;

  this->Transform = vtkSmartPointer<vtkTransform>::New();

  // Set up the initial properties
  this->HandleProperty = vtkSmartPointer<vtkProperty>::New();
  this->SelectedHandleProperty = vtkSmartPointer<vtkProperty>::New();
  this->LineProperty = vtkSmartPointer<vtkProperty>::New();
  this->SelectedLineProperty = vtkSmartPointer<vtkProperty>::New();

  this->CreateDefaultProperties();

  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkMarkupsCurveRepresentation::~vtkMarkupsCurveRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::SetClosed(bool closed)
{
  if ( this->Closed == closed )
    {
    return;
    }
  this->Closed = closed;

  this->BuildRepresentation();
}

//----------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::RegisterPickers()
{
  vtkPickingManager* pickingManager = this->Renderer->GetRenderWindow()->GetInteractor()->GetPickingManager();
  pickingManager->AddPicker(this->HandlePicker, this);
  pickingManager->AddPicker(this->LinePicker, this);
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::SetHandlePosition(int handle, double x, double y, double z)
{
  if (handle < 0 || handle >= this->Handles.size())
    {
    vtkErrorMacro(<<"vtkMarkupsCurveRepresentation: handle index "<<handle<<" out of range.");
    return;
    }
  this->HandleGeometry[handle]->SetCenter(x,y,z);
  this->HandleGeometry[handle]->Update();
  if ( this->ProjectToPlane )
    {
    this->ProjectPointsToPlane();
    }
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::SetHandlePosition(int handle, double xyz[3])
{
  this->SetHandlePosition(handle,xyz[0],xyz[1],xyz[2]);
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::GetHandlePosition(int handle, double xyz[3])
{
  if ( handle < 0 || handle >= this->Handles.size() )
    {
    vtkErrorMacro(<< "vtkMarkupsCurveRepresentation: handle index " <<handle<< " out of range.");
    return;
    }

  this->HandleGeometry[handle]->GetCenter(xyz);
}

//----------------------------------------------------------------------------
double* vtkMarkupsCurveRepresentation::GetHandlePosition(int handle)
{
  if ( handle < 0 || handle >= this->Handles.size() )
    {
    vtkErrorMacro(<<"vtkMarkupsCurveRepresentation: handle index out of range.");
    return NULL;
    }

  return this->HandleGeometry[handle]->GetCenter();
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::ProjectPointsToPlane()
{
  if (this->ProjectToPlane && this->ProjectionPlaneSource != NULL)
  {
    this->ProjectPointsToObliquePlane();
  }
  /*
  if ( this->ProjectionNormal == VTK_PROJECTION_OBLIQUE )
    {
    if ( this->PlaneSource != NULL )
      {
      this->ProjectPointsToObliquePlane();
      }
    else
      {
      vtkGenericWarningMacro(<<"Set the plane source for oblique projections...");
      }
    }
  else
    {
    this->ProjectPointsToOrthoPlane();
    }
  */
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::ProjectPointsToObliquePlane()
{
  double o[3];
  double u[3];
  double v[3];

  this->ProjectionPlaneSource->GetPoint1(u);
  this->ProjectionPlaneSource->GetPoint2(v);
  this->ProjectionPlaneSource->GetOrigin(o);

  for (int i = 0; i < 3; ++i )
    {
    u[i] = u[i] - o[i];
    v[i] = v[i] - o[i];
    }
  vtkMath::Normalize(u);
  vtkMath::Normalize(v);

  double o_dot_u = vtkMath::Dot(o,u);
  double o_dot_v = vtkMath::Dot(o,v);
  double fac1;
  double fac2;
  double ctr[3];
  for (int i = 0; i < this->Handles.size(); ++i )
    {
    this->HandleGeometry[i]->GetCenter(ctr);
    fac1 = vtkMath::Dot(ctr,u) - o_dot_u;
    fac2 = vtkMath::Dot(ctr,v) - o_dot_v;
    ctr[0] = o[0] + fac1*u[0] + fac2*v[0];
    ctr[1] = o[1] + fac1*u[1] + fac2*v[1];
    ctr[2] = o[2] + fac1*u[2] + fac2*v[2];
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
    }
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::ProjectPointsToOrthoPlane()
{
  /*
  double ctr[3];
  for ( int i = 0; i < this->Handles.size(); ++i )
    {
    this->HandleGeometry[i]->GetCenter(ctr);
    ctr[this->ProjectionNormal] = this->ProjectionPosition;
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
    }
    */
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::HighlightLine(int highlight)
{
  if ( highlight )
    {
    this->LineActor->SetProperty(this->SelectedLineProperty);
    }
  else
    {
    this->LineActor->SetProperty(this->LineProperty);
    }
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::CreateDefaultProperties()
{
  this->HandleProperty->SetColor(1, 1, 1);

  this->SelectedHandleProperty->SetColor(1,0,0);

  this->LineProperty->SetRepresentationToWireframe();
  this->LineProperty->SetAmbient(1.0);
  this->LineProperty->SetColor(1.0,1.0,0.0);
  this->LineProperty->SetLineWidth(2.0);

  this->SelectedLineProperty->SetRepresentationToWireframe();
  this->SelectedLineProperty->SetAmbient(1.0);
  this->SelectedLineProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedLineProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::SizeHandles()
{
  if (this->Handles.size() > 0)
    {
    double radius = this->SizeHandlesInPixels(1.5,
      this->HandleGeometry[0]->GetCenter());
    //cout << "Raduis: " << radius << endl;
    for ( int i = 0; i < this->Handles.size(); ++i )
      {
      this->HandleGeometry[i]->SetRadius(radius);
      }
    }
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::CalculateCentroid()
{
  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;

  double ctr[3];
  for ( int i = 0; i < this->Handles.size(); ++i )
    {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Centroid[0] += ctr[0];
    this->Centroid[1] += ctr[1];
    this->Centroid[2] += ctr[2];
    }

  this->Centroid[0] /= this->Handles.size();
  this->Centroid[1] /= this->Handles.size();
  this->Centroid[2] /= this->Handles.size();
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::InsertHandleOnLine(double* pos)
{
  if (this->Handles.size() < 2)
    {
    return;
    }

  vtkIdType id = this->LinePicker->GetCellId();
  if (id < 0)
    {
    return;
    }

  vtkIdType subid = this->LinePicker->GetSubId();

  vtkSmartPointer<vtkPoints> newpoints = vtkSmartPointer<vtkPoints>::Take(vtkPoints::New(VTK_DOUBLE));
  newpoints->SetNumberOfPoints(this->Handles.size()+1);

  int istart = subid;
  int istop = istart + 1;
  int count = 0;
  for (int i = 0; i <= istart; ++i )
    {
    newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
    }
  newpoints->SetPoint(count++,pos);

  for (int i = istop; i < this->Handles.size(); ++i )
    {
    newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
    }

  this->InitializeHandles(newpoints);
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::ReleaseGraphicsResources(vtkWindow* win)
{
  this->LineActor->ReleaseGraphicsResources(win);
  for (int cc=0; cc < this->Handles.size(); cc++)
    {
    this->Handles[cc]->ReleaseGraphicsResources(win);
    }
}

//----------------------------------------------------------------------------
int vtkMarkupsCurveRepresentation::RenderOpaqueGeometry(vtkViewport* win)
{
  this->BuildRepresentation();

  int count = 0;
  count += this->LineActor->RenderOpaqueGeometry(win);
  for (int cc=0; cc < this->Handles.size(); cc++)
    {
    count+= this->Handles[cc]->RenderOpaqueGeometry(win);
    }
  return count;
}

//----------------------------------------------------------------------------
int vtkMarkupsCurveRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport* win)
{
  int count = 0;
  count += this->LineActor->RenderTranslucentPolygonalGeometry(win);
  for (int cc=0; cc < this->Handles.size(); cc++)
    {
    count += this->Handles[cc]->RenderTranslucentPolygonalGeometry(win);
    }
  return count;
}

//----------------------------------------------------------------------------
int vtkMarkupsCurveRepresentation::RenderOverlay(vtkViewport* win)
{
  int count = 0;
  count += this->LineActor->RenderOverlay(win);
  for (int cc=0; cc < this->Handles.size(); cc++)
    {
    count += this->Handles[cc]->RenderOverlay(win);
    }
  return count;
}

//----------------------------------------------------------------------------
int vtkMarkupsCurveRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();
  int count = 0;
  count |= this->LineActor->HasTranslucentPolygonalGeometry();
  for (int cc=0; cc < this->Handles.size(); cc++)
    {
    count |= this->Handles[cc]->HasTranslucentPolygonalGeometry();
    }
  return count;
}

//----------------------------------------------------------------------------
int vtkMarkupsCurveRepresentation::ComputeInteractionState(int x, int y,
  int vtkNotUsed(modify))
{
  this->InteractionState = vtkMarkupsCurveRepresentation::Outside;
  if (!this->Renderer || !this->Renderer->IsInViewport(x, y))
    {
    return this->InteractionState;
    }

  // Try and pick a handle first. This allows the picking of the handle even
  // if it is "behind" the poly line.
  bool handlePicked = false;

  vtkAssemblyPath* path = NULL;
  if (!this->Handles.empty())
    {
    this->GetAssemblyPath(x, y, 0., this->HandlePicker);
    }

  if ( path != NULL )
    {
    this->ValidPick = 1;
    this->InteractionState = vtkMarkupsCurveRepresentation::OnHandle;
    this->CurrentHandleIndex =
      this->HighlightHandle(path->GetFirstNode()->GetViewProp());
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
    handlePicked = true;
    }
  else
    {
    this->CurrentHandleIndex = this->HighlightHandle(NULL);
    }


  if (this->Handles.size()>1)
    {
    if (!handlePicked)
      {
      path = this->GetAssemblyPath(x, y, 0., this->LinePicker);
      if ( path != NULL )
        {
        this->ValidPick = 1;
        this->LinePicker->GetPickPosition(this->LastPickPosition);
        this->HighlightLine(1);
        this->InteractionState = vtkMarkupsCurveRepresentation::OnLine;
        }
      else
        {
        this->HighlightLine(0);
        }
      }
    else
      {
      this->HighlightLine(0);
      }
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::StartWidgetInteraction(double e[2])
{
  // Store the start position
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;

  this->ComputeInteractionState(static_cast<int>(e[0]),static_cast<int>(e[1]),0);
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::WidgetInteraction(double e[2])
{
  // Convert events to appropriate coordinate systems
  vtkCamera *camera = this->Renderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  // Compute the two points defining the motion vector
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer,
    this->LastPickPosition[0], this->LastPickPosition[1], this->LastPickPosition[2],
    focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,this->LastEventPosition[0],
                                               this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == vtkMarkupsCurveRepresentation::Moving)
    {
    if (this->CurrentHandleIndex != -1)
      {
      this->MovePoint(prevPickPoint, pickPoint);
      }
    else
      {
       this->Translate(prevPickPoint, pickPoint);
      }
    }
  else if (this->InteractionState == vtkMarkupsCurveRepresentation::Scaling)
    {
    this->Scale(prevPickPoint, pickPoint,
      static_cast<int>(e[0]), static_cast<int>(e[1]));
    }
  else if (this->InteractionState == vtkMarkupsCurveRepresentation::Spinning)
    {
    camera->GetViewPlaneNormal(vpn);
    this->Spin(prevPickPoint, pickPoint, vpn);
    }

  if (this->ProjectToPlane)
    {
    this->ProjectPointsToPlane();
    }

  this->BuildRepresentation();

  // Store the position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::EndWidgetInteraction(double[2])
{
  switch (this->InteractionState)
    {
  case vtkMarkupsCurveRepresentation::Inserting:
    this->InsertHandleOnLine(this->LastPickPosition);
    break;

  case vtkMarkupsCurveRepresentation::Erasing:
    if (this->CurrentHandleIndex)
      {
      int index = this->CurrentHandleIndex;
      this->CurrentHandleIndex = this->HighlightHandle(NULL);
      this->EraseHandle(index);
      }
    }

  this->HighlightLine(0);
  this->InteractionState = vtkMarkupsCurveRepresentation::Outside;
}

//----------------------------------------------------------------------------
double* vtkMarkupsCurveRepresentation::GetBounds()
{
  this->BuildRepresentation();

  vtkBoundingBox bbox;
  if (!this->Handles.empty())
    {
    bbox.AddBounds(this->LineActor->GetBounds());
    for (int cc = 0; cc < this->Handles.size(); cc++)
      {
      bbox.AddBounds(this->HandleGeometry[cc]->GetOutput()->GetBounds());
      }
    }
  bbox.GetBounds(this->Bounds);
  return this->Bounds;
}


//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::SetLineColor(double r, double g, double b)
{
  this->GetLineProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkMarkupsCurveRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->HandleProperty )
    {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
    }
  else
    {
    os << indent << "Handle Property: (none)\n";
    }
  if ( this->SelectedHandleProperty )
    {
    os << indent << "Selected Handle Property: "
       << this->SelectedHandleProperty << "\n";
    }
  else
    {
    os << indent << "Selected Handle Property: (none)\n";
    }
  if ( this->LineProperty )
    {
    os << indent << "Line Property: " << this->LineProperty << "\n";
    }
  else
    {
    os << indent << "Line Property: (none)\n";
    }
  if ( this->SelectedLineProperty )
    {
    os << indent << "Selected Line Property: "
       << this->SelectedLineProperty << "\n";
    }
  else
    {
    os << indent << "Selected Line Property: (none)\n";
    }

  os << indent << "Project To Plane: "
     << (this->ProjectToPlane ? "On" : "Off") << "\n";
  os << indent << "Number Of Handles: " << this->Handles.size() << "\n";
  os << indent << "Closed: "
     << (this->Closed ? "On" : "Off") << "\n";
  os << indent << "InteractionState: " << this->InteractionState << endl;
}
