/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __vtkMRMLOrientationMarkerDisplayableManager_h
#define __vtkMRMLOrientationMarkerDisplayableManager_h

// MRMLDisplayableManager includes
#include "vtkMRMLAbstractDisplayableManager.h"
#include "vtkMRMLDisplayableManagerWin32Header.h"

// MRML includes
class vtkMRMLCameraNode;

/// \brief Displayable manager that displays orienatation marker in a slice or 3D view
class VTK_MRML_DISPLAYABLEMANAGER_EXPORT vtkMRMLOrientationMarkerDisplayableManager
  : public vtkMRMLAbstractDisplayableManager
{

public:
  static vtkMRMLOrientationMarkerDisplayableManager* New();
  vtkTypeMacro(vtkMRMLOrientationMarkerDisplayableManager,vtkMRMLAbstractDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:

  vtkMRMLOrientationMarkerDisplayableManager();
  virtual ~vtkMRMLOrientationMarkerDisplayableManager();

  /// Receives events from the view and the camera nodes.
  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData);
  virtual void ProcessWidgetsEvents(vtkObject *caller, unsigned long event, void *callData);

  virtual void AdditionalInitializeStep();

  /// Observe the View node and initialize the renderer accordingly.
  virtual void Create();

  /// Called each time the view node is modified.
  /// Internally update the renderer from the view node.
  /// \sa UpdateFromMRMLViewNode()
  virtual void OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller);

  /// Update the renderer from the view node properties.
  /// \sa UpdateFromCameraNode
  void UpdateFromViewNode();

  /// Set, Observe and Update from the camera node.
  /// \sa UpdateFromMRMLCameraNode()
  void SetAndObserveCameraNode(vtkMRMLCameraNode* cameraNodeToObserve);

  /// Each time the camera is modified, the closest label to the camera is hidden.
  /// \sa SetAndObserveFromMRMLCameraNode, UpdateFromViewNode()
  void UpdateFromCameraNode();

private:

  vtkMRMLOrientationMarkerDisplayableManager(const vtkMRMLOrientationMarkerDisplayableManager&);// Not implemented
  void operator=(const vtkMRMLOrientationMarkerDisplayableManager&); // Not Implemented

  class vtkInternal;
  vtkInternal * Internal;
};

#endif
