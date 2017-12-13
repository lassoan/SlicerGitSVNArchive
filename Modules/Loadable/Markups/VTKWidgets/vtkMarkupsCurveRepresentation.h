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
  // Set/Get the line properties. The properties of the line when selected
  // and unselected can be manipulated.
  vtkProperty* GetLineProperty() { return this->LineProperty; };
  vtkProperty* GetSelectedLineProperty() { return this->SelectedLineProperty; };

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
  // These are methods that satisfy vtkWidgetRepresentation's
  // API. Note that a version of place widget is available where the
  // center and handle position are specified.
  virtual void BuildRepresentation();
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

  // Description:
  // Set/Get the number of line segments representing the curve for
  // this widget.
  virtual void SetResolution(int resolution);
  vtkGetMacro(Resolution, int);

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

  // Do the picking
  vtkSmartPointer<vtkCellPicker> LinePicker;

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

  // The number of line segments used to represent the curve.
  int Resolution;


private:
  vtkMarkupsCurveRepresentation(const vtkMarkupsCurveRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMarkupsCurveRepresentation&) VTK_DELETE_FUNCTION;

};


#endif
