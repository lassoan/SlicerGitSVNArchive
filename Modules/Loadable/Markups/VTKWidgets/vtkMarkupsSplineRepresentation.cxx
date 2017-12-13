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
  Superclass::BuildRepresentation();

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

  for (int i = 0; i < this->Handles.size(); ++i)
    {
    points->SetPoint(i, this->GetHandlePosition(i));
    }
  this->ParametricSpline->SetClosed(this->Closed);
  this->ParametricSpline->Modified();
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
  if (this->Handles.size() < 2)
    {
    return;
    }

  vtkIdType id = this->LinePicker->GetCellId();
  if (id == -1)
    {
    // Didn't click on a line segment
    this->CreateHandle(pos);
    return;
    }

  vtkIdType subid = this->LinePicker->GetSubId();

  int istart = vtkMath::Floor(subid*(this->Handles.size() + this->Closed - 1.0)/
    static_cast<double>(this->Resolution));

  this->InsertHandle(istart, pos);

  this->BuildRepresentation();
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
