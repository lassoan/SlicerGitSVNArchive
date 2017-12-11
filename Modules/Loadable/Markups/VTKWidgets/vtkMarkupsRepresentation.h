/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsRepresentation

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMarkupsRepresentation
// .SECTION Description
// Base class for all markups representations, providing common
// interface and helper functions.

#ifndef vtkMarkupsRepresentation_h
#define vtkMarkupsRepresentation_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"

#include "vtkWidgetRepresentation.h"
#include "vtkHandleRepresentation.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include <deque>

class vtkHandleRepresentation;
class vtkPlaneSource;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkMarkupsRepresentation : public vtkWidgetRepresentation
{
public:
  vtkTypeMacro(vtkMarkupsRepresentation, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Force the widget to be projected onto one of the orthogonal
  // planes.  Remember that when the InteractionState changes, a
  // ModifiedEvent is invoked.  This can be used to snap the curve to
  // the plane if it is originally not aligned.  The normal in
  // SetProjectionNormal is 0,1,2 for YZ,XZ,XY planes respectively and
  // 3 for arbitrary oblique planes when the widget is tied to a
  // vtkPlaneSource.
  vtkSetMacro(ProjectToPlane,bool);
  vtkGetMacro(ProjectToPlane,bool);
  vtkBooleanMacro(ProjectToPlane,bool);

  // Description:
  // Set up a reference to a vtkPlaneSource that could be from another widget
  // object, e.g. a vtkPolyDataSourceWidget.
  void SetProjectionPlaneSource(vtkPlaneSource* planeSource);
  vtkGetObjectMacro(ProjectionPlaneSource, vtkPlaneSource);

  // Description:
  // Get the handle representations used for a particular handle. A side effect of
  // this method is that it will create a handle representation in the list of
  // representations if one has not yet been created.
  virtual vtkHandleRepresentation *GetHandleRepresentation(unsigned int num);

  // Used to communicate about the state of the representation
  enum
  {
    Outside = 0,
    NearHandle
  };

  // Description:
  // Handle that is currently being interacted with
  vtkGetMacro(ActiveHandle, int);

  // Returns the id of the handle created, -1 on failure. e is the display position.
  virtual int CreateHandle(double* worldPosition, double* displayPosition);

  // Delete last handle created
  virtual void RemoveLastHandle();
  // Delete the currently active handle
  virtual void RemoveActiveHandle();

  // Description:
  // Remove the nth handle.
  virtual void RemoveHandle(int n);

  // Description:
  // Returns the model HandleRepresentation.
  void SetHandleRepresentation(vtkHandleRepresentation *rep);
  vtkGetObjectMacro(HandleRepresentation, vtkHandleRepresentation);

  vtkHandleRepresentation* GetHandle(int n) { return this->Handles[n].GetPointer(); };

  // Description:
  // Methods to Set/Get the coordinates of seed points defining
  // this representation. Note that methods are available for both
  // display and world coordinates. The seeds are accessed by a seed
  // number.
  virtual void GetHandleWorldPosition(unsigned int handleNum, double pos[3]);
  virtual void SetHandleDisplayPosition(unsigned int handleNum, double pos[3]);
  virtual void GetHandleDisplayPosition(unsigned int handleNum, double pos[3]);

  // Description:
  // Return the number of handles (or handles) that have been created.
  int GetNumberOfHandles();

  // Description:
  // The tolerance representing the distance to the widget (in pixels) in
  // which the cursor is considered near enough to the handle points of
  // the widget to be active.
  vtkSetClampMacro(Tolerance, int, 1, 100);
  vtkGetMacro(Tolerance, int);

protected:
  vtkMarkupsRepresentation();
  ~vtkMarkupsRepresentation();

  // Controlling vars
  bool ProjectToPlane;
  vtkPlaneSource* ProjectionPlaneSource;

  typedef std::deque< vtkSmartPointer<vtkHandleRepresentation> > HandleListType;

  HandleListType Handles;

  // The active handle (handle) based on the last ComputeInteractionState()
  int ActiveHandle;

  vtkHandleRepresentation  *HandleRepresentation;

  // Selection tolerance for the handles
  int Tolerance;

private:
  vtkMarkupsRepresentation(const vtkMarkupsRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMarkupsRepresentation&) VTK_DELETE_FUNCTION;

};

#endif
