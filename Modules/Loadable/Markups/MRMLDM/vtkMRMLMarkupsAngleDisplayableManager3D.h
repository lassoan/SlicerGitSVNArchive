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

#ifndef __vtkMRMLMarkupsAngleDisplayableManager3D_h
#define __vtkMRMLMarkupsAngleDisplayableManager3D_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManager3D.h"

class vtkAngleRepresentation3D;
class vtkAngleWidget;
class vtkMRMLMarkupsAngleNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;
class vtkTextWidget;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsAngleDisplayableManager3D :
    public vtkMRMLMarkupsDisplayableManager3D
{
public:

  static vtkMRMLMarkupsAngleDisplayableManager3D *New();
  vtkTypeMacro(vtkMRMLMarkupsAngleDisplayableManager3D, vtkMRMLMarkupsDisplayableManager3D);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:

  vtkMRMLMarkupsAngleDisplayableManager3D(){this->Focus="vtkMRMLMarkupsAngleNode";}
  virtual ~vtkMRMLMarkupsAngleDisplayableManager3D(){}

  /// Callback for click in RenderWindow
  virtual void OnClickInRenderWindow(double x, double y, const char *associatedNodeID);
  /// Create a widget.
  virtual vtkAbstractWidget * CreateWidget(vtkMRMLMarkupsNode* node);
  /// Create new handle on widget when a new markup is added to a markups node
  virtual void OnMRMLMarkupsNodeMarkupAddedEvent(vtkMRMLMarkupsNode * markupsNode, int n);
  /// Respond to the nth markup modified event
  virtual void OnMRMLMarkupsNodeNthMarkupModifiedEvent(vtkMRMLMarkupsNode * markupsNode, int n);
  /// Respond to a markup being removed from the markups node
  virtual void OnMRMLMarkupsNodeMarkupRemovedEvent(vtkMRMLMarkupsNode * markupsNode, int n);

  /// Gets called when widget was created
  virtual void OnWidgetCreated(vtkAbstractWidget * widget, vtkMRMLMarkupsNode * node);

  /// Update a single seed from MRML
  void SetNthSeed(int n, vtkMRMLMarkupsAngleNode* fiducialNode, vtkAngleWidget *seedWidget);
  /// Propagate properties of MRML node to widget.
  virtual void PropagateMRMLToWidget(vtkMRMLMarkupsNode* node, vtkAbstractWidget * widget);

  /// Propagate properties of widget to MRML node.
  virtual void PropagateWidgetToMRML(vtkAbstractWidget * widget, vtkMRMLMarkupsNode* node);

  /// Set up an observer on the interactor style to watch for key press events
  virtual void AdditionnalInitializeStep();
  /// Respond to the interactor style event
  virtual void OnInteractorStyleEvent(int eventid);

  /// Update a single seed position from the node, return true if the position changed
  virtual bool UpdateNthSeedPositionFromMRML(int n, vtkAbstractWidget *widget, vtkMRMLMarkupsNode *pointsNode);
  /// Respond to control point modified events
  virtual void UpdatePosition(vtkAbstractWidget *widget, vtkMRMLNode *node);

  // Clean up when scene closes
  virtual void OnMRMLSceneEndClose();

  void GetSeedWorldPosition(vtkAngleRepresentation3D* seedRepresentation, int n, double* position);
  void SetSeedWorldPosition(vtkAngleRepresentation3D* seedRepresentation, int n, double* position);

private:

  vtkMRMLMarkupsAngleDisplayableManager3D(const vtkMRMLMarkupsAngleDisplayableManager3D&); /// Not implemented
  void operator=(const vtkMRMLMarkupsAngleDisplayableManager3D&); /// Not Implemented
};

#endif
