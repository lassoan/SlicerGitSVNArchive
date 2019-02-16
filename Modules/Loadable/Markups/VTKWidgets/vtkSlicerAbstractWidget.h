/*=========================================================================

 Copyright (c) ProxSim ltd., Kwun Tong, Hong Kong. All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 This file was originally developed by Davide Punzo, punzodavide@hotmail.it,
 and development was supported by ProxSim ltd.

=========================================================================*/

/**
 * @class   vtkSlicerAbstractWidget
 * @brief   Process interaction events to update state of MRML widget nodes
 *
 * vtkSlicerAbstractWidget is the abstract class that handles interaction events
 * received from the displayable manager. To decide which widget gets the chance
 * to process an interaction event, this class does not use VTK picking manager,
 * but interactor style class queries displayable managers about what events they can
 * process, displayable manager queries its widgets, and based on the returned
 * information, interactor style selects a displayable manager and the displayable
 * manager selects a widget.
 *
 * vtkAbstractWidget uses vtkSlicerWidgetEventTranslator to translate VTK
 * interaction events (defined in vtkCommand.h) into widget events (defined in
 * vtkSlicerAbstractWidget.h and subclasses). This class allows modification
 * of event bindings.
 *
 * @sa
 * vtkSlicerWidgetRepresentation vtkSlicerWidgetEventTranslator
 *
*/

#ifndef vtkSlicerAbstractWidget_h
#define vtkSlicerAbstractWidget_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkAbstractWidget.h"
#include "vtkWidgetCallbackMapper.h"

#include "vtkMRMLMarkupsNode.h"

class vtkSlicerAbstractWidgetRepresentation;
class vtkSlicerInteractionEventData;
class vtkPolyData;
class vtkIdList;


class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerAbstractWidget : public vtkObject
{
public:
  /// Standard methods for a VTK class.
  vtkTypeMacro(vtkSlicerAbstractWidget, vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Set the representation.
  /// The widget takes over the ownership of this actor.
  virtual void SetRepresentation(vtkSlicerAbstractWidgetRepresentation *r);

  /// Get the representation
  virtual vtkSlicerAbstractWidgetRepresentation *GetRepresentation();

  /// Build the actors of the representation with the info stored in the vtkMRMLMarkupsNode
  virtual void UpdateFromMRML();

  /// Build the locator with the info stored in the vtkMRMLMarkupsNode
  virtual void BuildLocator();

  /// Create the default widget representation if one is not set.
  virtual void CreateDefaultRepresentation() = 0;

  /// Convenient method to close the contour loop.
  virtual void CloseLoop();

  /// Convenient method to change what state the widget is in.
  vtkSetMacro(WidgetState,int);

  /// Convenient method to determine the state of the method
  vtkGetMacro(WidgetState,int);

  /// During definition, the last node of the
  /// contour will automatically follow the cursor, without waiting for the
  /// point to be dropped. This is useful for some interpolators, such as the
  /// live-wire interpolator to see the shape of the contour that will be placed
  /// as you move the mouse cursor.
  vtkSetMacro(FollowCursor, vtkTypeBool);
  vtkGetMacro(FollowCursor, vtkTypeBool);
  vtkBooleanMacro(FollowCursor, vtkTypeBool);

  /// Convenient method to remap the horizonbtal axes constrain key.
  vtkSetMacro(HorizontalActiveKeyCode, const char);
  vtkGetMacro(HorizontalActiveKeyCode, char);

  /// Convenient method to remap the vertical axes constrain key.
  vtkSetMacro(VerticalActiveKeyCode, const char);
  vtkGetMacro(VerticalActiveKeyCode, char);

  /// Convenient method to remap the depth axes constrain key.
  vtkSetMacro(DepthActiveKeyCode, const char);
  vtkGetMacro(DepthActiveKeyCode, char);

  /// The state of the widget
  enum
  {
    Idle, // mouse pointer is outside the widget, click does not do anything
    Define, // click in empty area will place a new point
    OnWidget, // mouse pointer is over the widget, clicking will add a point or manipulate the line
    TranslateControlPoint, // translating the active point by mouse move
    Translate, // mouse move transforms the entire widget
    Scale,     // mouse move transforms the entire widget
    Rotate,     // mouse move transforms the entire widget
  };

  /// Widget events
  enum WidgetEvents
  {
    WidgetNoEvent,
    WidgetMouseMove,
    WidgetControlPointMoveStart,
    WidgetControlPointMoveEnd,
    WidgetControlPointDelete,
    WidgetTranslateStart,
    WidgetTranslateEnd,
    WidgetRotateStart,
    WidgetRotateEnd,
    WidgetScaleStart,
    WidgetScaleEnd,
    WidgetControlPointInsert,
    // MRML events
    WidgetPick, // generates a MRML Pick event (e.g., on left click)
    WidgetAction, // generates a MRML Action event (e.g., left double-click)
    WidgetCustomAction1, // generates a MRML CustomAction1 event (allows modules to define custom widget actions and get notification via MRML node event)
    WidgetCustomAction2, // generates a MRML CustomAction1 event
    WidgetCustomAction3, // generates a MRML CustomAction1 event
    WidgetSelect, // change MRML node Selected attribute
    WidgetUnselect, // change MRML node Selected attribute
    WidgetToggleSelect, // change MRML node Selected attribute
    // Other actions
    WidgetMenu, // show context menu
    WidgetReset, // reset widget to initial state (clear all points)


  };

  /// Add a point preview to the current active Markup at input World coordiantes.
  int AddPreviewPointToRepresentationFromWorldCoordinate(const double worldCoordinates [3]);

  /// Add/update preview point position
  void UpdatePreviewPointPositionFromWorldCoordinate(const double worldCoordinates[3]);

  /// Remove the point preview to the current active Markup.
  /// Returns true is preview point existed and now it is removed.
  bool RemovePreviewPoint();

  /// Add a point to the current active Markup at input World coordiantes.
  virtual int AddPointFromWorldCoordinate(const double worldCoordinates[3]);

  /// Return true if the widget can process the event.
  /// Distance2 is the squared distance in display coordinates from the closest interaction position.
  /// The displayable manager with the closest distance will get the chance to process the interaction event.
  virtual bool CanProcessInteractionEvent(vtkSlicerInteractionEventData* eventData, double &distance2);

  /// Allows injecting interaction events for processing, without directly observing window interactor events.
  /// Return true if the widget processed the event.
  virtual bool ProcessInteractionEvent(vtkSlicerInteractionEventData* eventData);

  virtual unsigned long TranslateInteractionEventToWidgetEvent(vtkSlicerInteractionEventData* eventData);

  /// Called when the the widget loses the focus.
  virtual void Leave();

  void SetEventTranslation(unsigned long interactionEvent, int modifiers, unsigned long widgetEvent);
  void SetKeyboardEventTranslation(int modifier, char keyCode, int repeatCount, const char* keySym, unsigned long widgetEvent);

  void SetRenderer(vtkRenderer* renderer);
  vtkGetMacro(Renderer, vtkRenderer*);

  // Allows the widget to request a cursor shape
  virtual int GetCursor();

  // Allows the widget to request grabbing of all events (even when the mouse pointer moves out of view)
  virtual bool GetGrabFocus();

  // Allows the widget to request interactive mode (faster updates)
  virtual bool GetInteractive();


protected:
  vtkSlicerAbstractWidget();
  ~vtkSlicerAbstractWidget() VTK_OVERRIDE;

  /// Render immediately
  void Render();

  /// Request render (may be slightly delayed)
  void RequestRender();

  void StartWidgetInteraction(vtkSlicerInteractionEventData* eventData);
  void EndWidgetInteraction();

  virtual void TranslateNode(double eventPos[2]);
  virtual void TranslateWidget(double eventPos[2]);
  virtual void ScaleWidget(double eventPos[2]);
  virtual void RotateWidget(double eventPos[2]);

  vtkMRMLMarkupsNode* GetMarkupsNode();
  vtkMRMLMarkupsDisplayNode* GetMarkupsDisplayNode();
  int GetActiveControlPoint();

  bool IsAnyControlPointLocked();

  vtkRenderer* Renderer;

  // Translates interaction event to widget event.
  // In the future, a vector of event translators could be added
  // (one for each state) to be able to define events
  // that are only allowed in a specific state.
  vtkSmartPointer<vtkWidgetEventTranslator> EventTranslator;

  int WidgetState;
  vtkTypeBool FollowCursor;

  // Callback interface to capture events when
  // placing the widget.
  // Return true if the event is processed.
  bool ProcessMouseMove(vtkSlicerInteractionEventData* eventData);
  bool ProcessControlPointDelete(vtkSlicerInteractionEventData* eventData);
  bool ProcessControlPointMove(vtkSlicerInteractionEventData* eventData);
  bool ProcessEndMouseDrag(vtkSlicerInteractionEventData* eventData);
  bool ProcessWidgetReset(vtkSlicerInteractionEventData* eventData);
  bool ProcessWidgetRotate(vtkSlicerInteractionEventData* eventData);
  bool ProcessWidgetScale(vtkSlicerInteractionEventData* eventData);
  bool ProcessWidgetTranslate(vtkSlicerInteractionEventData* eventData);

  // Manual axis constrain
  char HorizontalActiveKeyCode;
  char VerticalActiveKeyCode;
  char DepthActiveKeyCode;
  vtkSmartPointer<vtkCallbackCommand> KeyEventCallbackCommand;
  static void ProcessKeyEvents(vtkObject *, unsigned long, void *, void *);

  // Variables for translate/rotate/scale
  double LastEventPosition[2];
  double StartEventOffsetPosition[2];

  vtkSmartPointer<vtkSlicerAbstractWidgetRepresentation> WidgetRep;

private:
  vtkSlicerAbstractWidget(const vtkSlicerAbstractWidget&) = delete;
  void operator=(const vtkSlicerAbstractWidget&) = delete;
};

#endif
