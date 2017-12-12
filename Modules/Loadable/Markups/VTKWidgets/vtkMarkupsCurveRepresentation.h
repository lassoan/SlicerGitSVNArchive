/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsCurveRepresentation

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMarkupsCurveRepresentation - vtkMarkupsRepresentation
// base class for a widget that represents an curve that connects control
// points.
// .SECTION Description
// Base class for widgets used to define curves from points, such as
// vtkPolyLineRepresentation and vtkSplineRepresentation.  This class
// uses handles, the number of which can be changed, to represent the
// points that define the curve. The handles can be picked can be
// picked on the curve itself to translate or rotate it in the scene.

#ifndef vtkMarkupsCurveRepresentation_h
#define vtkMarkupsCurveRepresentation_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"

#include "vtkMarkupsRepresentation.h"
#include "vtkProperty.h"
#include "vtkSmartPointer.h"
#include <deque>

class vtkActor;
class vtkCellPicker;
class vtkDoubleArray;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkProp;
class vtkProperty;
class vtkSphereSource;
class vtkTransform;
class vtkPlaneSource;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkMarkupsCurveRepresentation : public vtkMarkupsRepresentation
{
public:
  vtkTypeMacro(vtkMarkupsCurveRepresentation, vtkMarkupsRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Grab the polydata (including points) that defines the
  // interpolating curve. Points are guaranteed to be up-to-date when
  // either the InteractionEvent or EndInteraction events are
  // invoked. The user provides the vtkPolyData and the points and
  // polyline are added to it.
  virtual void GetPolyData(vtkPolyData *pd) = 0;

  // Description:
  // Set/Get the handle properties (the spheres are the handles). The
  // properties of the handles when selected and unselected can be manipulated.
  vtkProperty* GetHandleProperty() { return this->HandleProperty; };
  vtkProperty* GetSelectedHandleProperty() { return this->SelectedHandleProperty; };

  // Description:
  // Set/Get the line properties. The properties of the line when selected
  // and unselected can be manipulated.
  vtkProperty* GetLineProperty() { return this->LineProperty; };
  vtkProperty* GetSelectedLineProperty() { return this->SelectedLineProperty; };

  // Description:
  // Set/Get the position of the handles. Call GetNumberOfHandles
  // to determine the valid range of handle indices.
  virtual void SetHandlePosition(int handle, double x, double y, double z);
  virtual void SetHandlePosition(int handle, double xyz[3]);
  virtual void GetHandlePosition(int handle, double xyz[3]);
  virtual double* GetHandlePosition(int handle);
  virtual vtkDoubleArray* GetHandlePositions() = 0;

  // Description:
  // Control whether the curve is open or closed. A closed forms a
  // continuous loop: the first and last points are the same.  A
  // minimum of 3 handles are required to form a closed loop.
  void SetClosed(bool closed);
  vtkGetMacro(Closed, bool);
  vtkBooleanMacro(Closed, bool);

  // Description:
  // Get the approximate vs. the true arc length of the curve. Calculated as
  // the summed lengths of the individual straight line segments. Use
  // SetResolution to control the accuracy.
  virtual double GetSummedLength() = 0;

  // Description:
  // Convenience method to allocate and set the handles from a
  // vtkPoints instance.  If the first and last points are the same,
  // the curve sets Closed to the on InteractionState and disregards
  // the last point, otherwise Closed remains unchanged.
  virtual void InitializeHandles(vtkPoints* points) = 0;

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's
  // API. Note that a version of place widget is available where the
  // center and handle position are specified.
  virtual void BuildRepresentation() = 0;
  virtual int ComputeInteractionState(int X, int Y, int modify=0);
  virtual void StartWidgetInteraction(double e[2]);
  virtual void WidgetInteraction(double e[2]);
  virtual void EndWidgetInteraction(double e[2]);
  virtual double *GetBounds();

  // Description:
  // Methods supporting, and required by, the rendering process.
  virtual void ReleaseGraphicsResources(vtkWindow*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int RenderOverlay(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Convenience method to set the line color.
  // Ideally one should use GetLineProperty()->SetColor().
  void SetLineColor(double r, double g, double b);

  // Projection capabilities
  void ProjectPointsToPlane();
  void ProjectPointsToOrthoPlane();
  void ProjectPointsToObliquePlane();

protected:
  vtkMarkupsCurveRepresentation();
  ~vtkMarkupsCurveRepresentation();

  double LastEventPosition[3];
  double Bounds[6];

  bool Closed;

  // The line segments
  vtkSmartPointer<vtkActor> LineActor;
  void HighlightLine(int highlight);

  void Initialize();
  int  HighlightHandle(vtkProp *prop); //returns handle index or -1 on fail
  virtual void SizeHandles();
  virtual void InsertHandleOnLine(double* pos) = 0;
  void EraseHandle(const int&);

  // Do the picking
  vtkSmartPointer<vtkCellPicker> LinePicker;
  double LastPickPosition[3];

  // Register internal Pickers within PickingManager
  virtual void RegisterPickers();


  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkSmartPointer<vtkProperty> LineProperty;
  vtkSmartPointer<vtkProperty> SelectedLineProperty;
  void CreateDefaultProperties();

  // For efficient spinning
  double Centroid[3];
  void CalculateCentroid();

private:
  vtkMarkupsCurveRepresentation(const vtkMarkupsCurveRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMarkupsCurveRepresentation&) VTK_DELETE_FUNCTION;

};


#endif
