/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsSplineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMarkupsSplineRepresentation - representation for a spline.
// .SECTION Description
// vtkMarkupsSplineRepresentation is a vtkWidgetRepresentation for a spline.
// This 3D widget defines a spline that can be interactively placed in a
// scene. The spline has handles, the number of which can be changed, plus it
// can be picked on the spline itself to translate or rotate it in the scene.
// This is based on vtkSplineWidget.
// .SECTION See Also
// vtkSplineWidget, vtkSplineWidget2

#ifndef vtkMarkupsSplineRepresentation_h
#define vtkMarkupsSplineRepresentation_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkMarkupsCurveRepresentation.h"

class vtkActor;
class vtkCellPicker;
class vtkDoubleArray;
class vtkParametricFunctionSource;
class vtkParametricSpline;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkProp;
class vtkProperty;
class vtkSphereSource;
class vtkTransform;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkMarkupsSplineRepresentation : public vtkMarkupsCurveRepresentation
{
public:
  static vtkMarkupsSplineRepresentation* New();
  vtkTypeMacro(vtkMarkupsSplineRepresentation, vtkMarkupsCurveRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Grab the polydata (including points) that defines the spline.  The
  // polydata consists of points and line segments numbering Resolution + 1
  // and Resolution, respectively. Points are guaranteed to be up-to-date when
  // either the InteractionEvent or  EndInteraction events are invoked. The
  // user provides the vtkPolyData and the points and polyline are added to it.
  void GetPolyData(vtkPolyData *pd);

  // Description:
  // Set/Get the number of line segments representing the spline for
  // this widget.
  void SetResolution(int resolution);
  vtkGetMacro(Resolution,int);

  // Description:
  // Set the parametric spline object. Through vtkParametricSpline's API, the
  // user can supply and configure one of two types of spline:
  // vtkCardinalSpline, vtkKochanekSpline. The widget controls the open
  // or closed configuration of the spline.
  // WARNING: The widget does not enforce internal consistency so that all
  // three are of the same type.
  virtual void SetParametricSpline(vtkParametricSpline*);
  vtkGetObjectMacro(ParametricSpline,vtkParametricSpline);

  // Description:
  // Get the position of the spline handles.
  vtkDoubleArray* GetHandlePositions();

  // Description:
  // Get the approximate vs. the true arc length of the spline. Calculated as
  // the summed lengths of the individual straight line segments. Use
  // SetResolution to control the accuracy.
  double GetSummedLength();

 // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API. Note that a
  // version of place widget is available where the center and handle position
  // are specified.
  virtual void BuildRepresentation();

  // Specialized method to insert a handle on the poly line.
  virtual void InsertHandleOnLine(double* pos);

protected:
  vtkMarkupsSplineRepresentation();
  ~vtkMarkupsSplineRepresentation();

  // The spline
  vtkParametricSpline *ParametricSpline;
  vtkSmartPointer<vtkParametricFunctionSource> ParametricFunctionSource;

  // The number of line segments used to represent the spline.
  int Resolution;

private:
  vtkMarkupsSplineRepresentation(const vtkMarkupsSplineRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMarkupsSplineRepresentation&) VTK_DELETE_FUNCTION;

};

#endif
