/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceIntersectionWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSliceIntersectionWidget
 * @brief   perform affine transformations
 *
 * The vtkSliceIntersectionWidget allows moving slices by interacting with
 * displayed slice intersecrion lines.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   Ctrl+LeftButtonPressEvent - select widget: depending on which part is selected
 *                               slices may be translated or rotated.
 *   LeftButtonReleaseEvent - end selection of widget.
 *   Ctrl+MouseMoveEvent - interactively move slices.
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkSliceIntersectionWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- focal point is being selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for widget motion
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkSliceIntersectionWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
*/

#ifndef vtkSliceIntersectionWidget_h
#define vtkSliceIntersectionWidget_h

#include "vtkMRMLDisplayableManagerExport.h" // For export macro
#include "vtkAbstractWidget.h"

#include <vtkCollection.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

class vtkSliceIntersectionRepresentation2D;
class vtkMRMLApplicationLogic;
class vtkMRMLSliceNode;

class VTK_MRML_DISPLAYABLEMANAGER_EXPORT vtkSliceIntersectionWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkSliceIntersectionWidget *New();

  //@{
  /**
   * Standard VTK class macros.
   */
  vtkTypeMacro(vtkSliceIntersectionWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkSliceIntersectionRepresentation2D *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  /**
   * Return the representation as a vtkSliceIntersectionRepresentation2D.
   */
  vtkSliceIntersectionRepresentation2D *GetSliceIntersectionRepresentation()
    {return reinterpret_cast<vtkSliceIntersectionRepresentation2D*>(this->GetRepresentation());}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Methods for activating this widget. This implementation extends the
   * superclasses' in order to resize the widget handles due to a render
   * start event.
   */
  void SetEnabled(int) override;

  void SetMRMLApplicationLogic(vtkMRMLApplicationLogic*);
  vtkMRMLApplicationLogic* GetMRMLApplicationLogic();

  void SetSliceNode(vtkMRMLSliceNode* sliceNode);
  vtkMRMLSliceNode* GetSliceNode();

protected:
  vtkSliceIntersectionWidget();
  ~vtkSliceIntersectionWidget() override;

  // These are the callbacks for this widget
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void ModifyEventAction(vtkAbstractWidget*);

  // helper methods for cursor management
  void SetCursor(int state) override;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start=0,
    Active
  };

  static void SliceLogicsModifiedCallback(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  // Keep track whether key modifier key is pressed
  int ModifierActive;

  vtkWeakPointer<vtkMRMLApplicationLogic> MRMLApplicationLogic;
  vtkWeakPointer<vtkCollection> SliceLogics;
  vtkNew<vtkCallbackCommand> SliceLogicsModifiedCommand;

private:
  vtkSliceIntersectionWidget(const vtkSliceIntersectionWidget&) = delete;
  void operator=(const vtkSliceIntersectionWidget&) = delete;
};

#endif
