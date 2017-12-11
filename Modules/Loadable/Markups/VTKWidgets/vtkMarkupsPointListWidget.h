/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsPointListWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMarkupsPointListWidget - place multiple seed points
// .SECTION Description
// The vtkMarkupsPointListWidget is used to placed multiple seed points in the scene.
// It is based on vtkSeedWidget, but most functionalities are implemented in
// vtkMarkupsWidget.

// .SECTION See Also
// vtkSeedWidget vtkMarkupsWidget


#ifndef vtkMarkupsPointListWidget_h
#define vtkMarkupsPointListWidget_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkMarkupsWidget.h"

class vtkHandleRepresentation;
class vtkHandleWidget;
class vtkMarkupsPointListRepresentation;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkMarkupsPointListWidget : public vtkMarkupsWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkMarkupsPointListWidget *New();

  // Description:
  // Standard methods for a VTK class.
  vtkTypeMacro(vtkMarkupsPointListWidget,vtkMarkupsWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create and set the default widget representation if one is not set.
  virtual void CreateDefaultRepresentation();

protected:
  vtkMarkupsPointListWidget();
  ~vtkMarkupsPointListWidget();

private:
  vtkMarkupsPointListWidget(const vtkMarkupsPointListWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMarkupsPointListWidget&) VTK_DELETE_FUNCTION;
};

#endif
