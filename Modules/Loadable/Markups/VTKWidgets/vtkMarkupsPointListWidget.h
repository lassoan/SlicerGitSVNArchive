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
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation( vtkMarkupsPointListRepresentation *rep )
    {
    this->Superclass::SetWidgetRepresentation(
      reinterpret_cast<vtkWidgetRepresentation*>(rep) );
    }

  // Description:
  // Return the representation as a vtkSeedRepresentation.
  vtkMarkupsPointListRepresentation *GetMarkupsPointListRepresentation()
    {
    return reinterpret_cast<vtkMarkupsPointListRepresentation*>(this->WidgetRep);
    }

  // Description:
  // Create the default widget representation if one is not set.
  void CreateDefaultRepresentation();

  virtual int AddHandle(double* worldPosition, double* displayPosition);

  virtual void RemoveHandle(int i);

  // Description:
  // Get the nth seed
  //vtkHandleWidget * GetHandle( int n );

protected:
  vtkMarkupsPointListWidget();
  ~vtkMarkupsPointListWidget();

  // Callback interface to capture events when
  // placing the widget.
  static void AddPointAction( vtkAbstractWidget* );
  static void CompletedAction( vtkAbstractWidget* );
  static void MoveAction( vtkAbstractWidget* );
  static void EndSelectAction( vtkAbstractWidget* );
  static void DeleteAction( vtkAbstractWidget* );

  // Manipulating or defining ?
  int Defining;

private:
  vtkMarkupsPointListWidget(const vtkMarkupsPointListWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMarkupsPointListWidget&) VTK_DELETE_FUNCTION;
};

#endif
