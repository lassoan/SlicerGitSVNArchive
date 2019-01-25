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
 * @class   vtkSlicerCurveWidget
 * @brief   create a curve with a set of points
 *
 * The vtkSlicerCurveWidget is used to select a set of points.
 * The widget handles all processing of widget
 * events (that are triggered by VTK events). The vtkSlicerCurveRepresentation is
 * responsible for all placement of the points, and
 * curve manipulation. This is done through a main helper class:
 * vtkPointPlacer. The representation is also
 * responsible for drawing the points.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - triggers a Select event
 *   Alt + LeftButtonPressEvent - triggers a Rotate event
 *   MiddleButtonPressEvent - triggers a Translate event
 *   RightButtonPressEvent - triggers a AddFinalPoint event
 *
 *   MouseMoveEvent - triggers a Move event
 *
 *   LeftButtonReleaseEvent - triggers an EndSelect event
 *   MiddleButtonReleaseEvent - triggers an EndTranslate event
 *   RightButtonReleaseEvent - triggers an EndScale event
 *
 *   LeftButtonDoubleClickEvent - triggers an PickOne event
 *   MiddleButtonDoubleClickEvent - triggers an PickTwo event
 *   RightButtonDoubleClickEvent - triggers an PickThree event
 *
 *
 *   Delete key event - triggers a Delete event
 *   Shift + Delete key event - triggers a Reset event
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkSlicerCurveWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select
 *        widget state is:
 *            Start or
 *            Define: If we already have at least 2 nodes, test
 *                 whether the current (X,Y) location is near an existing
 *                 node. If so, close the curve and change to Manipulate
 *                 state. Otherwise, attempt to add a node at this (X,Y)
 *                 location.
 *            Manipulate: If this (X,Y) location activates a node, then
 *                 set the current operation to Translate.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::PickPoint
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: If this (X,Y) location activates a node. The node or line
 *                 will be selected, but no translate will be possible.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Rotate
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: If this (X,Y) location activates a node or the line, then
 *                 set the current operation to Rotate.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Translate
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: If this (X,Y) location activates a node or the line, then
 *                 set the current operation to Shift.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Scale
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Do nothing.
 *            Manipulate: If this (X,Y) location activates a node or the line, then
 *                 set the current operation to Scale.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::AddFinalPoint
 *        widget state is:
 *            Start: Do nothing.
 *            Define: If we already have at least 2 nodes, test
 *                 whether the current (X,Y) location is near an existing
 *                 node. If so, close the curve and change to Manipulate
 *                 state. Otherwise, attempt to add a node at this (X,Y)
 *                 location. If we do, then leave the curve open and
 *                 change to Manipulate state.
 *            Manipulate: Do nothing.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Move
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is Translate, Shift or Scale, then invoke
 *                  WidgetInteraction() on the representation. If our
 *                  operation is Inactive, then just attempt to activate
 *                  a node at this (X,Y) location.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::EndSelect
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is not Inactive, set it to
 *                  Inactive.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::EndTranslate
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is not Inactive, set it to
 *                  Inactive.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::EndScale
 *        widget state is:
 *            Start or
 *            Define: Do nothing.
 *            Manipulate: If our operation is not Inactive, set it to
 *                  Inactive.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Delete
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Remove the last point on the widget.
 *            Manipulate: Attempt to activate a node at (X,Y). If
 *                   we do activate a node, delete it. If we now
 *                   have less than 3 nodes, go back to Define state.
 *
 * @par Event Bindings:
 *   vtkWidgetEvent::Reset
 *        widget state is:
 *            Start: Do nothing.
 *            Define: Remove all points of the widget.
 *                 Essentially calls Initialize(nullptr)
 *            Manipulate: Do nothing.
 * </pre>
 *
 * @par Event Bindings:
 * This widget invokes the following VTK events on itself (which observers
 * can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (beginning to interact;
 *                                      call data includes handle id)
 *   vtkCommand::EndInteractionEvent (completing interaction;
 *                                    call data includes handle id)
 *   vtkCommand::InteractionEvent (moving after selecting something;
 *                                 call data includes handle id)
 *   vtkCommand::PlacePointEvent (after point is positioned;
 *                                call data includes handle id)
 *   vtkCommand::DeletePointEvent (after point is positioned;
 *                                 call data includes handle id)
 *
 *   Note: handle id conuter start from 0.
 * </pre>
 *
*/

#ifndef vtkSlicerCurveWidget_h
#define vtkSlicerCurveWidget_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerAbstractWidget.h"

class vtkSlicerAbstractRepresentation;
class vtkPolyData;
class vtkIdList;

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkSlicerCurveWidget : public vtkSlicerAbstractWidget
{
public:
  /// Instantiate this class.
  static vtkSlicerCurveWidget *New();

  /// Standard methods for a VTK class.
  vtkTypeMacro(vtkSlicerCurveWidget,vtkSlicerAbstractWidget);

  /// Create the default widget representation if one is not set.
  virtual void CreateDefaultRepresentation() override;

  /// Add a point to the current active Markup at input World coordiantes.
  int AddPointToRepresentationFromWorldCoordinate(double worldCoordinates [3], bool persistence = false);

protected:
  vtkSlicerCurveWidget();
  ~vtkSlicerCurveWidget() override;

  // Callback interface to capture evets when
  // placing the widget.
  static void AddPointOnCurveAction(vtkAbstractWidget*);

private:
  vtkSlicerCurveWidget(const vtkSlicerCurveWidget&) = delete;
  void operator=(const vtkSlicerCurveWidget&) = delete;
};

#endif
