/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsPointListRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMarkupsPointListRepresentation - represent the vtkMarkupsPointListWidget
// .SECTION Description
// The vtkMarkupsPointListRepresentation is a superclass for classes representing the
// vtkSeedWidget. This representation consists of one or more handles
// (vtkHandleRepresentation) which are used to place and manipulate the
// points defining the collection of seeds.
// Based on vtkSeedsRepresentation.

// .SECTION See Also
// vtkSeedWidget vtkHandleRepresentation vtkMarkupsPointListRepresentation


#ifndef vtkMarkupsPointListRepresentation_h
#define vtkMarkupsPointListRepresentation_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkMarkupsRepresentation.h"

class vtkHandleRepresentation;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkMarkupsPointListRepresentation : public vtkMarkupsRepresentation
{
public:
  // Description:
  // Instantiate class.
  static vtkMarkupsPointListRepresentation *New();

  // Description:
  // Standard VTK methods.
  vtkTypeMacro(vtkMarkupsPointListRepresentation,vtkMarkupsRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The tolerance representing the distance to the widget (in pixels) in
  // which the cursor is considered near enough to the handle points of
  // the widget to be active.
  vtkSetClampMacro( Tolerance, int, 1, 100 );
  vtkGetMacro( Tolerance, int );

  // Description:
  // These are methods specific to vtkMarkupsPointListRepresentation and which are
  // invoked from vtkMarkupsPointListWidget.

  // Returns the id of the handle created, -1 on failure. e is the display position.
  virtual int CreateHandle( double e[2] );

  // Description:
  // These are methods that satisfy vtkWidgetRepresentation's API.
  virtual void BuildRepresentation();
  virtual int ComputeInteractionState( int X, int Y, int modify = 0 );

protected:
  vtkMarkupsPointListRepresentation();
  ~vtkMarkupsPointListRepresentation();

  // Selection tolerance for the handles
  int Tolerance;

private:
  vtkMarkupsPointListRepresentation(const vtkMarkupsPointListRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMarkupsPointListRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
