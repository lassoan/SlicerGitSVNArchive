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

#include "vtkSlicerPointsRepresentation2D.h"
#include "vtkAppendPolyData.h"
#include "vtkCleanPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkGlyph3D.h"
#include "vtkCursor2D.h"
#include "vtkCylinderSource.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkCamera.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkFocalPlanePointPlacer.h"
#include "vtkSphereSource.h"
#include "vtkVolumePicker.h"
#include "vtkPickingManager.h"

vtkStandardNewMacro(vtkSlicerPointsRepresentation2D);

//----------------------------------------------------------------------
vtkSlicerPointsRepresentation2D::vtkSlicerPointsRepresentation2D()
{
}

//----------------------------------------------------------------------
vtkSlicerPointsRepresentation2D::~vtkSlicerPointsRepresentation2D()
{
}

//----------------------------------------------------------------------
void vtkSlicerPointsRepresentation2D::BuildLines()
{
  return;
}

//----------------------------------------------------------------------
void vtkSlicerPointsRepresentation2D::RotateWidget(double eventPos[2])
{
  // If any node is locked return
  for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (this->GetNthNodeLocked(i))
      {
      return;
      }
    }

  this->Superclass::ScaleWidget(eventPos);
}
