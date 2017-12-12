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

#ifndef __vtkMRMLMarkupsCurveDisplayableManager_h
#define __vtkMRMLMarkupsCurveDisplayableManager_h

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsFiducialDisplayableManager.h"

class vtkMRMLMarkupsCurveNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;
class vtkTextWidget;
class vtkMarkupsCurveWidget;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsCurveDisplayableManager :
  public vtkMRMLMarkupsFiducialDisplayableManager
{
public:

  static vtkMRMLMarkupsCurveDisplayableManager *New();
  vtkTypeMacro(vtkMRMLMarkupsCurveDisplayableManager, vtkMRMLMarkupsFiducialDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:

  vtkMRMLMarkupsCurveDisplayableManager(){this->Focus="vtkMRMLMarkupsCurveNode";}
  virtual ~vtkMRMLMarkupsCurveDisplayableManager(){}

protected:

  virtual vtkMarkupsWidget * CreateWidgetInstance();
  virtual vtkMarkupsWidget * CreateProjectionWidgetInstance() VTK_OVERRIDE;

private:

  vtkMRMLMarkupsCurveDisplayableManager(const vtkMRMLMarkupsCurveDisplayableManager&); /// Not implemented
  void operator=(const vtkMRMLMarkupsCurveDisplayableManager&); /// Not Implemented
};

#endif
