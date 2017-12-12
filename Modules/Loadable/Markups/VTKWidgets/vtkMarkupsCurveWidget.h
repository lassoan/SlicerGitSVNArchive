/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsCurveWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMarkupsCurveWidget - widget for vtkMarkupsSplineRepresentation.
// .SECTION Description
// vtkMarkupsCurveWidget is the vtkMarkupsWidget subclass for
// vtkMarkupsSplineRepresentation which manages the interactions with
// vtkMarkupsSplineRepresentation. This is based on vtkSplineWidget.
// .SECTION See Also
// vtkMarkupsSplineRepresentation, vtkMarkupsCurveWidget

#ifndef vtkMarkupsCurveWidget_h
#define vtkMarkupsCurveWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkMarkupsWidget.h"

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"

class vtkMarkupsSplineRepresentation;
class vtkHandleWidget;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkMarkupsCurveWidget : public vtkMarkupsWidget
{
public:
  static vtkMarkupsCurveWidget* New();
  vtkTypeMacro(vtkMarkupsCurveWidget, vtkMarkupsWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of
  // vtkProp so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkMarkupsSplineRepresentation *r)
    {
    this->Superclass::SetWidgetRepresentation(
      reinterpret_cast<vtkWidgetRepresentation*>(r));
    }

  // Description:
  // Create the default widget representation if one is not set. By default,
  // this is an instance of the vtkMarkupsSplineRepresentation class.
  void CreateDefaultRepresentation();

protected:
  vtkMarkupsCurveWidget();
  ~vtkMarkupsCurveWidget();

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

private:
  vtkMarkupsCurveWidget(const vtkMarkupsCurveWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMarkupsCurveWidget&) VTK_DELETE_FUNCTION;

};

#endif