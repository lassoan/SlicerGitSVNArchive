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

#ifndef __vtkMRMLMarkupsCurveDisplayableManager3D_h
#define __vtkMRMLMarkupsCurveDisplayableManager3D_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsFiducialDisplayableManager3D.h"

class vtkMRMLMarkupsCurveNode;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsCurveDisplayableManager3D :
    public vtkMRMLMarkupsFiducialDisplayableManager3D
{
public:

  static vtkMRMLMarkupsCurveDisplayableManager3D *New();
  vtkTypeMacro(vtkMRMLMarkupsCurveDisplayableManager3D, vtkMRMLMarkupsFiducialDisplayableManager3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:

  vtkMRMLMarkupsCurveDisplayableManager3D(){this->Focus="vtkMRMLMarkupsCurveNode";}
  virtual ~vtkMRMLMarkupsCurveDisplayableManager3D(){}

  /// Callback for click in RenderWindow
  virtual void OnClickInRenderWindow(double x, double y,
                                     const char *associatedNodeID,
                                     int action = vtkMRMLMarkupsDisplayableManager3D::AddPoint) VTK_OVERRIDE;
  /// Create a widget.
  virtual vtkSlicerAbstractWidget * CreateWidget(vtkMRMLMarkupsNode* node) VTK_OVERRIDE;
  /// Gets called when widget was created
  virtual void OnWidgetCreated(vtkSlicerAbstractWidget * widget, vtkMRMLMarkupsNode * node) VTK_OVERRIDE;
  /// Clean up when scene closes
  virtual void OnMRMLSceneEndClose() VTK_OVERRIDE;
  /// Respond to interactor style events
  virtual void OnInteractorStyleEvent(int eventid) VTK_OVERRIDE;

private:

  vtkMRMLMarkupsCurveDisplayableManager3D(const vtkMRMLMarkupsCurveDisplayableManager3D&); /// Not implemented
  void operator=(const vtkMRMLMarkupsCurveDisplayableManager3D&); /// Not Implemented
};

#endif
