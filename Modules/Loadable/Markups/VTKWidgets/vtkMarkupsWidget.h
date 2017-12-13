/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarkupsWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMarkupsWidget - place multiple handles
// .SECTION Description
// The vtkMarkupsWidget is used to placed multiple handles in the scene.
// The handle points can be used for operations like connectivity, segmentation,
// and region growing.
//
// To use this widget, specify an instance of vtkMarkupsWidget and a
// representation (a subclass of vtkMarkupsRepresentation). The widget is
// implemented using multiple instances of vtkHandleWidget which can be used
// to position the handle points (after they are initially placed). The
// representations for these handle widgets are provided by the
// vtkMarkupsRepresentation.
//
// .SECTION Event Bindings
// By default, the widget responds to the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
//   LeftButtonPressEvent - add a point or select a handle
//   RightButtonPressEvent - finish adding the handles
//   MouseMoveEvent - move a handle
//   LeftButtonReleaseEvent - release the selected handle
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events
// into the vtkMarkupsWidget's widget events:
// <pre>
//   vtkWidgetEvent::Select -- if near handle, select handle on click.
//   vtkWidgetEvent::Completed -- finished adding handles.
//   vtkWidgetEvent::Move -- move the second point or handle depending on the state.
//   vtkWidgetEvent::EndSelect -- the handle manipulation process has completed.
// </pre>
//
// This widget invokes the following VTK events on itself (which observers
// can listen for):
// <pre>
//   vtkCommand::StartInteractionEvent (beginning to interact)
//   vtkCommand::EndInteractionEvent (completing interaction)
//   vtkCommand::InteractionEvent (moving after selecting something)
//   vtkCommand::PlacePointEvent (after point is positioned;
//                                call data includes handle id (0,1))
// </pre>

// .SECTION See Also
// vtkHandleWidget vtkMarkupsReoresentation


#ifndef vtkMarkupsWidget_h
#define vtkMarkupsWidget_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkAbstractWidget.h"
#include "vtkSmartPointer.h"

#include <deque>

class vtkHandleRepresentation;
class vtkHandleWidget;
class vtkMarkupsRepresentation;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkMarkupsWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Standard methods for a VTK class.
  static vtkMarkupsWidget *New();
  vtkTypeMacro(vtkMarkupsWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The method for activating and deactivating this widget. This method
  // must be overridden because it is a composite widget and does more than
  // its superclasses' vtkAbstractWidget::SetEnabled() method.
  virtual void SetEnabled(int);

  // Description:
  // Set the current renderer. This method also propagates to all the child
  // handle widgets, if any exist
  virtual void SetCurrentRenderer( vtkRenderer * );

  // Description:
  // Set the interactor. This method also propagates to all the child
  // handle widgets, if any exist
  virtual void SetInteractor( vtkRenderWindowInteractor * );

  // Description:
  // Use this method to programmatically create a new handle. In interactive
  // mode, (when the widget is in the PlacingHandles state) this method is
  // automatically invoked. The method returns the handle created.
  // A valid markups representation must exist for the widget to create a new
  // handle.
  virtual int AddHandle(double* position);

  // If insertAfterHandleIndex is -1 then the handle is appended to the end.
  // Otherwise new handle is appended after the specified index value
  virtual int InsertHandle(double* position, int insertBeforeHandleIndex);


  // Description:
  // Delete the nth handle. Index of all handles after the deleted handle will be decremented.
  // Child class must override this to update representations
  virtual void RemoveHandle(int n);

  // Description:
  // Get the nth handle
  vtkHandleWidget * GetHandle( int n );

  // Description:
  // Methods to change the whether the widget responds to interaction.
  // Overridden to pass the state to component widgets.
  virtual void SetProcessEvents(int);

  // Description:
  // Get the widget state.
  vtkGetMacro( WidgetState, int );

  // The state of the widget

  enum
    {
    Idle = 0, // click on an empty area has no effect
    MovingHandle, // moving handle, will return to Idle state when completed
    Active // for curve
    };

  virtual void SetRepresentation(vtkMarkupsRepresentation* widgetRep);

  virtual void CreateDefaultRepresentation();

protected:
  vtkMarkupsWidget();
  ~vtkMarkupsWidget();

  typedef std::deque< vtkSmartPointer<vtkHandleWidget> > HandleWidgetList;

  int WidgetState;

  // There may be NULL handles in the list, which indicates an undefined (non-placed) handle.
  HandleWidgetList Handles;

  // Callback interface to capture events when interacting with the widget.
  static void SelectAction( vtkAbstractWidget* );
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction( vtkAbstractWidget* );
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);

private:
  vtkMarkupsWidget(const vtkMarkupsWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMarkupsWidget&) VTK_DELETE_FUNCTION;
};

#endif
