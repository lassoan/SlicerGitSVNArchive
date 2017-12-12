/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __vtkMRMLMarkupsFiducialDisplayableManager_h
#define __vtkMRMLMarkupsFiducialDisplayableManager_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManager.h"

class vtkMRMLMarkupsFiducialNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;
class vtkTextWidget;
class vtkOrientedPolygonalHandleRepresentation3D;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsFiducialDisplayableManager :
    public vtkMRMLMarkupsDisplayableManager
{
public:

  static vtkMRMLMarkupsFiducialDisplayableManager *New();
  vtkTypeMacro(vtkMRMLMarkupsFiducialDisplayableManager, vtkMRMLMarkupsDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:

  vtkMRMLMarkupsFiducialDisplayableManager(){this->Focus="vtkMRMLMarkupsFiducialNode";}
  virtual ~vtkMRMLMarkupsFiducialDisplayableManager(){}

  /// Callback for click in RenderWindow
  virtual void OnClickInRenderWindow(double x, double y, const char *associatedNodeID) VTK_OVERRIDE;
  /// Create a widget.
  virtual vtkMarkupsWidget * CreateWidget(vtkMRMLMarkupsNode* node) VTK_OVERRIDE;
  /// Create new handle on widget when a new markup is added to a markups node
  virtual void OnMRMLMarkupsNodeMarkupAddedEvent(vtkMRMLMarkupsNode * markupsNode, int n) VTK_OVERRIDE;
  /// Respond to the nth markup modified event
  virtual void OnMRMLMarkupsNodeNthMarkupModifiedEvent(vtkMRMLMarkupsNode * markupsNode, int n) VTK_OVERRIDE;
  /// Respond to a markup being removed from the markups node
  virtual void OnMRMLMarkupsNodeMarkupRemovedEvent(vtkMRMLMarkupsNode * markupsNode, int n) VTK_OVERRIDE;

  /// Gets called when widget was created
  virtual void OnWidgetCreated(vtkMarkupsWidget * widget, vtkMRMLMarkupsNode * node) VTK_OVERRIDE;

  /// Update a single handle from MRML
  void SetNthHandle(int n, vtkMRMLMarkupsFiducialNode* fiducialNode, vtkMarkupsWidget *markupsWidget);

  void SetNthHandleGlyphType(int n, vtkMRMLMarkupsDisplayNode *displayNode, vtkOrientedPolygonalHandleRepresentation3D *handleRep);

  /// Propagate properties of MRML node to widget.
  virtual void PropagateMRMLToWidget(vtkMRMLMarkupsNode* node, vtkMarkupsWidget * widget) VTK_OVERRIDE;

  /// Propagate properties of widget to MRML node.
  virtual void PropagateWidgetToMRML(vtkMarkupsWidget * widget, vtkMRMLMarkupsNode* node) VTK_OVERRIDE;

  /// Set up an observer on the interactor style to watch for key press events
  virtual void AdditionnalInitializeStep();
  /// Respond to the interactor style event
  virtual void OnInteractorStyleEvent(int eventid) VTK_OVERRIDE;

  /// Update a single handle position from the node, return true if the position changed
  virtual bool UpdateNthHandlePositionFromMRML(int n, vtkMarkupsWidget *widget, vtkMRMLMarkupsNode* markupsNode) VTK_OVERRIDE;

  /// Update just the position for the widget, implemented by subclasses.
  virtual void UpdatePosition(vtkMarkupsWidget *widget, vtkMRMLMarkupsNode* markupsNode) VTK_OVERRIDE;

  // Clean up when scene closes
  virtual void OnMRMLSceneEndClose() VTK_OVERRIDE;

protected:

  virtual vtkMarkupsWidget * CreateWidgetInstance();
  virtual vtkMarkupsWidget * CreateProjectionWidgetInstance();

private:

  vtkMRMLMarkupsFiducialDisplayableManager(const vtkMRMLMarkupsFiducialDisplayableManager&); /// Not implemented
  void operator=(const vtkMRMLMarkupsFiducialDisplayableManager&); /// Not Implemented
};

#endif
