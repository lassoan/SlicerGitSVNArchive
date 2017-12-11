/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsSplineRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarkupsSplineRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkBoundingBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkHandleRepresentation.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricSpline.h"
#include "vtkPickingManager.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereHandleRepresentation.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkDoubleArray.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMarkupsSplineRepresentation);

//----------------------------------------------------------------------------
vtkMarkupsSplineRepresentation::vtkMarkupsSplineRepresentation()
{
  // Build the representation of the widget
  this->ParametricSpline = NULL;
  this->ParametricFunctionSource = vtkSmartPointer<vtkParametricFunctionSource>::New();

  // Define the points and line segments representing the spline
  this->Resolution = 100;

  this->ParametricFunctionSource = vtkParametricFunctionSource::New();
  //this->ParametricFunctionSource->SetParametricFunction(parametricSpline.GetPointer());
  this->ParametricFunctionSource->SetScalarModeToNone();
  this->ParametricFunctionSource->GenerateTextureCoordinatesOff();
  this->ParametricFunctionSource->SetUResolution(this->Resolution);
  //this->ParametricFunctionSource->Update();

  vtkNew<vtkParametricSpline> parametricSpline;
  this->SetParametricSpline(parametricSpline.GetPointer());

  vtkNew<vtkPolyDataMapper> lineMapper;
  lineMapper->SetInputConnection(
    this->ParametricFunctionSource->GetOutputPort()) ;
  lineMapper->ImmediateModeRenderingOn();
  lineMapper->SetResolveCoincidentTopologyToPolygonOffset();

  this->LineActor->SetMapper(lineMapper.GetPointer());
}

//----------------------------------------------------------------------------
vtkMarkupsSplineRepresentation::~vtkMarkupsSplineRepresentation()
{
  this->SetParametricSpline(NULL);
}

//----------------------------------------------------------------------------
void vtkMarkupsSplineRepresentation::SetParametricSpline(vtkParametricSpline* spline)
{
  if (this->ParametricSpline == spline)
    {
    return;
    }
  if (this->ParametricSpline)
    {
    this->ParametricSpline->UnRegister(this);
    }
  this->ParametricSpline = spline;
  if (this->ParametricSpline)
    {
    this->ParametricSpline->Register(this);
    }
  if (this->ParametricFunctionSource)
  {
    this->ParametricFunctionSource->SetParametricFunction(this->ParametricSpline);
  }
}

//----------------------------------------------------------------------------
vtkDoubleArray* vtkMarkupsSplineRepresentation::GetHandlePositions()
{
  return vtkArrayDownCast<vtkDoubleArray>(
    this->ParametricSpline->GetPoints()->GetData());
}

//----------------------------------------------------------------------------
void vtkMarkupsSplineRepresentation::BuildRepresentation()
{
  this->ValidPick = 1;
  // TODO: Avoid unnecessary rebuilds.
  // Handles have changed position, re-compute the spline coeffs
  vtkPoints* points = this->ParametricSpline->GetPoints();
  if (!points)
    {
    vtkNew<vtkPoints> newPoints;
    points = newPoints.GetPointer();
    this->ParametricSpline->SetPoints(points);
    }
  if (points->GetNumberOfPoints() != this->Handles.size())
    {
    points->SetNumberOfPoints(this->Handles.size());
    }

  vtkBoundingBox bbox;
  for (int i = 0; i < this->Handles.size(); ++i)
    {
    double pt[3];
    this->HandleGeometry[i]->GetCenter(pt);
    points->SetPoint(i, pt);
    bbox.AddPoint(pt);
    }
  this->ParametricSpline->SetClosed(this->Closed);
  this->ParametricSpline->Modified();

  double bounds[6];
  bbox.GetBounds(bounds);
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  this->SizeHandles();
}

//----------------------------------------------------------------------------
void vtkMarkupsSplineRepresentation::SetNumberOfHandles(int npts)
{
  if (this->Handles.size() == npts)
    {
    return;
    }

  // Ensure that no handle is current
  this->HighlightHandle(NULL);

  this->Initialize();

  double radius = 10;
  vtkSphereHandleRepresentation* sphereHandle = vtkSphereHandleRepresentation::SafeDownCast(this->HandleRepresentation);
  if (sphereHandle)
    {
    radius = sphereHandle->GetSphereRadius();
    }

  this->Handles.resize(npts);

  // Create the handles

  for (int i = 0; i < this->Handles.size(); ++i)
    {
    if (this->HandleGeometry.size() <= i)
      {
      this->HandleGeometry.push_back(vtkSmartPointer<vtkSphereSource>::New());
      }
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    vtkNew<vtkPolyDataMapper> handleMapper;
    handleMapper->SetInputConnection(
      this->HandleGeometry[i]->GetOutputPort());
    if (this->Handles[i].GetPointer() == NULL)
      {
      vtkNew<vtkSphereHandleRepresentation> newHandleRep;
      this->Handles[i] = newHandleRep.GetPointer();
      }
    vtkActor* handleActor = vtkActor::SafeDownCast(this->Handles[i]);
    handleActor->SetMapper(handleMapper.GetPointer());
    handleActor->SetProperty(this->HandleProperty);
    /*
    double u[3], pt[3];
    u[0] = i/(this->Handles.size() - 1.0);
    this->ParametricSpline->Evaluate(u, pt, NULL);
    this->HandleGeometry[i]->SetCenter(pt);*/
    this->HandleGeometry[i]->SetRadius(radius);
    this->HandlePicker->AddPickList(this->Handles[i]);
    }

  if (this->CurrentHandleIndex >= 0 &&
    this->CurrentHandleIndex < this->Handles.size())
    {
    this->CurrentHandleIndex =
      this->HighlightHandle(this->Handles[this->CurrentHandleIndex]);
    }
  else
    {
    this->CurrentHandleIndex = this->HighlightHandle(NULL);
    }

  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkMarkupsSplineRepresentation::SetResolution(int resolution)
{
  if (this->Resolution == resolution || resolution < (this->Handles.size()-1))
    {
    return;
    }

  this->Resolution = resolution;
  this->ParametricFunctionSource->SetUResolution(this->Resolution);
  this->ParametricFunctionSource->Modified();
}

//----------------------------------------------------------------------------
void vtkMarkupsSplineRepresentation::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy(this->ParametricFunctionSource->GetOutput());
}

//----------------------------------------------------------------------------
double vtkMarkupsSplineRepresentation::GetSummedLength()
{
  vtkPoints* points = this->ParametricFunctionSource->GetOutput()->GetPoints();
  int npts = points->GetNumberOfPoints();

  if (npts < 2) { return 0.0; }

  double a[3];
  double b[3];
  double sum = 0.0;
  int i = 0;
  points->GetPoint(i, a);
  int imax = (npts % 2 == 0) ? npts-2 : npts-1;

  while (i < imax)
    {
    points->GetPoint(i+1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
    i = i + 2;
    points->GetPoint(i, a);
    sum = sum + sqrt(vtkMath::Distance2BetweenPoints(a, b));
    }

  if (npts % 2 == 0)
    {
    points->GetPoint(i+1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
    }

  return sum;
}

//----------------------------------------------------------------------------
void vtkMarkupsSplineRepresentation::InsertHandleOnLine(double* pos)
{
  if (this->Handles.size() < 2) { return; }

  vtkIdType id = this->LinePicker->GetCellId();
  if (id == -1)
    {
    // Didn't click on a line segment
    this->InsertHandle(pos);
    return;
    }

  vtkIdType subid = this->LinePicker->GetSubId();

  //vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
  vtkPoints* newPoints = vtkPoints::New();
  newPoints->SetDataType(VTK_DOUBLE);
  newPoints->SetNumberOfPoints(this->Handles.size() + 1);

  int istart = vtkMath::Floor(subid*(this->Handles.size() + this->Closed - 1.0)/
    static_cast<double>(this->Resolution));
  int istop = istart + 1;
  int count = 0;
  for (int i = 0; i <= istart; ++i)
    {
    newPoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
    }

  newPoints->SetPoint(count++, pos);

  for (int i = istop; i < this->Handles.size(); ++i)
    {
    newPoints->SetPoint(count++, this->HandleGeometry[i]->GetCenter());
    }

  this->InitializeHandles(newPoints);
}

void vtkMarkupsSplineRepresentation::InsertHandle(double* pos)
{
  vtkNew<vtkPoints> newPoints;
  newPoints->SetDataType(VTK_DOUBLE);
  newPoints->SetNumberOfPoints(this->Handles.size() + 1);

  for (int i = 0; i < this->Handles.size(); ++i)
    {
    newPoints->SetPoint(i, this->HandleGeometry[i]->GetCenter());
    }
  newPoints->SetPoint(this->Handles.size(), pos);

  this->InitializeHandles(newPoints.GetPointer());
}

//----------------------------------------------------------------------------
void vtkMarkupsSplineRepresentation::InitializeHandles(vtkPoints* points)
{
  if (!points){ return; }

  int npts = points->GetNumberOfPoints();

  double p0[3];
  double p1[3];

  points->GetPoint(0,p0);
  points->GetPoint(npts-1,p1);

  this->SetNumberOfHandles(npts);
  for (int i = 0; i < npts; ++i)
    {
    this->SetHandlePosition(i,points->GetPoint(i));
    }
}

//----------------------------------------------------------------------------
void vtkMarkupsSplineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (this->ParametricSpline)
    {
    os << indent << "ParametricSpline: "
       << this->ParametricSpline << "\n";
    }
  else
    {
    os << indent << "ParametricSpline: (none)\n";
    }
}
