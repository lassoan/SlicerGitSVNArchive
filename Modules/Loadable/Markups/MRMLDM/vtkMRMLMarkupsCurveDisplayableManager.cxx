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

// MarkupsModule/MRML includes
#include <vtkMRMLMarkupsCurveNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsCurveDisplayableManager.h"

// MarkupsModule/VTKWidgets includes
#include <vtkMarkupsCurveWidget.h>
#include <vtkMarkupsGlyphSource2D.h>

// MRMLDisplayableManager includes
#include <vtkSliceViewInteractorStyle.h>

// MRML includes
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkMarkupsWidget.h>
#include <vtkFollower.h>
#include <vtkHandleRepresentation.h>
#include <vtkInteractorStyle.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOrientedPolygonalHandleRepresentation3D.h>
#include <vtkPickingManager.h>
#include <vtkProperty2D.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkMarkupsCurveWidget.h>
#include <vtkMarkupsPointListWidget.h>
#include <vtkSmartPointer.h>
#include <vtkMarkupsSplineRepresentation.h>
#include <vtkSphereSource.h>
#include <vtkSplineWidget2.h>
#include <vtkMarkupsSplineRepresentation.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsCurveDisplayableManager);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsCurveDisplayableManager methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkMarkupsWidget* vtkMRMLMarkupsCurveDisplayableManager::CreateWidgetInstance()
{
  return vtkMarkupsCurveWidget::New();
}

//---------------------------------------------------------------------------
vtkMarkupsWidget* vtkMRMLMarkupsCurveDisplayableManager::CreateProjectionWidgetInstance()
{
  return vtkMarkupsPointListWidget::New();
}
