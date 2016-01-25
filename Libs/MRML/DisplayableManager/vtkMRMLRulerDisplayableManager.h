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

  This file was originally developed by Andras Lasso, PerkLab, Queen's University.

==============================================================================*/

#ifndef __vtkMRMLRulerDisplayableManager_h
#define __vtkMRMLRulerDisplayableManager_h

// MRMLDisplayableManager includes
#include "vtkMRMLAbstractDisplayableManager.h"
#include "vtkMRMLDisplayableManagerWin32Header.h"

// MRML includes
class vtkMRMLCameraNode;

/// \brief Displayable manager that displays orienatation marker in a slice or 3D view
class VTK_MRML_DISPLAYABLEMANAGER_EXPORT vtkMRMLRulerDisplayableManager
  : public vtkMRMLAbstractDisplayableManager
{
  friend class vtkRulerRendererUpdateObserver;

public:
  static vtkMRMLRulerDisplayableManager* New();
  vtkTypeMacro(vtkMRMLRulerDisplayableManager,vtkMRMLAbstractDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:

  vtkMRMLRulerDisplayableManager();
  virtual ~vtkMRMLRulerDisplayableManager();

  /// Observe the View node and initialize the renderer accordingly.
  virtual void Create();

  /// Called each time the view node is modified.
  /// Internally update the renderer from the view node.
  /// \sa UpdateFromMRMLViewNode()
  virtual void OnMRMLDisplayableNodeModifiedEvent(vtkObject* caller);

  /// Update the renderer from the view node properties.
  void UpdateFromViewNode();

  /// Update the renderer based on the master renderer (the one that the orientation marker follows)
  void UpdateFromRenderer();

private:

  vtkMRMLRulerDisplayableManager(const vtkMRMLRulerDisplayableManager&);// Not implemented
  void operator=(const vtkMRMLRulerDisplayableManager&); // Not Implemented

  class vtkInternal;
  vtkInternal * Internal;
};

#endif
