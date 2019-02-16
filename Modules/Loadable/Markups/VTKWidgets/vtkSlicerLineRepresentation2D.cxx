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

#include "vtkSlicerLineRepresentation2D.h"
#include "vtkCleanPolyData.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkAssemblyPath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkGlyph2D.h"
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
#include "vtkSlicerLinearLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkPropPicker.h"
#include "vtkAppendPolyData.h"
#include "vtkTubeFilter.h"
#include "vtkStringArray.h"
#include "vtkPickingManager.h"
#include "vtkMRMLMarkupsDisplayNode.h"

vtkStandardNewMacro(vtkSlicerLineRepresentation2D);

//----------------------------------------------------------------------
vtkSlicerLineRepresentation2D::vtkSlicerLineRepresentation2D()
{
  this->LineInterpolator = vtkSmartPointer<vtkSlicerLinearLineInterpolator>::New();

  this->Line = vtkSmartPointer<vtkPolyData>::New();
  this->TubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
  this->TubeFilter->SetInputData(this->Line);
  this->TubeFilter->SetNumberOfSides(20);
  this->TubeFilter->SetRadius(1);

  this->LineMapper = vtkSmartPointer<vtkOpenGLPolyDataMapper2D>::New();
  this->LineMapper->SetInputConnection(this->TubeFilter->GetOutputPort());

  this->LineActor = vtkSmartPointer<vtkActor2D>::New();
  this->LineActor->SetMapper(this->LineMapper);
  this->LineActor->SetProperty(this->GetControlPointsPipeline(Unselected)->Property);
}

//----------------------------------------------------------------------
vtkSlicerLineRepresentation2D::~vtkSlicerLineRepresentation2D()
{
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::BuildRepresentation()
{
  // Make sure we are up to date with any changes made in the placer
  //this->UpdateWidget(true);

  Superclass::BuildRepresentation();

  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }
  if (!this->MarkupsDisplayNode)
  {
    return;
  }

  double scale = this->CalculateViewScaleFactor();
  /*
  for (int controlPointType = 0; controlPointType < NumberOfControlPointTypes; ++controlPointType)
  {
    ControlPointsPipeline2D* controlPoints = this->GetControlPointsPipeline(controlPointType);
    controlPoints->LabelsActor->SetVisibility(this->MarkupsDisplayNode->GetTextVisibility());
    controlPoints->Glypher->SetScaleFactor(scale * this->ControlPointSize);
  }
  */

  this->TubeFilter->SetRadius(scale * this->ControlPointSize * 0.125);

  //this->BuildRepresentationPointsAndLabels(scale * this->ControlPointSize); called by superclass

  bool lineVisibility = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (!this->PointsVisibilityOnSlice->GetValue(ii) ||
        !this->GetNthNodeVisibility(ii))
      {
      lineVisibility = false;
      break;
      }
    }

  this->LineActor->SetVisibility(lineVisibility);

  int controlPointType = Unselected;
  if (this->MarkupsDisplayNode->GetActiveComponentType() == vtkMRMLMarkupsDisplayNode::ComponentLine)
  {
    controlPointType = Active;
  }
  else if (!this->GetNthNodeSelected(0) || !this->GetNthNodeSelected(1))
  {
    controlPointType = Unselected;
  }
  else
  {
    controlPointType = Selected;
  }
  this->LineActor->SetProperty(this->GetControlPointsPipeline(controlPointType)->Property);

}

//----------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::CanInteract(const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &componentIndex)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode || markupsNode->GetLocked() || this->GetNumberOfNodes() < 1)
  {
    return vtkMRMLMarkupsDisplayNode::ComponentNone;
  }
  int foundComponentType = Superclass::CanInteract(displayPosition, worldPosition, closestDistance2, componentIndex);
  if (foundComponentType != vtkMRMLMarkupsDisplayNode::ComponentNone && closestDistance2 == 0.0)
  {
    return foundComponentType;
  }

  this->CanInteractWithLine(foundComponentType, displayPosition, worldPosition, closestDistance2, componentIndex);

  return foundComponentType;
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::BuildLines()
{
  this->BuildLine(this->Line, true);
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::GetActors(vtkPropCollection *pc)
{
  this->LineActor->GetActors(pc);
  this->Superclass::GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->LineActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  if (this->LineActor->GetVisibility())
    {
    count +=  this->LineActor->RenderOverlay(viewport);
    }
  count += this->Superclass::RenderOverlay(viewport);

  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();

  int count=0;
  if (this->LineActor->GetVisibility())
    {
    count += this->LineActor->RenderOpaqueGeometry(viewport);
    }
  count = this->Superclass::RenderOpaqueGeometry(viewport);

  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  if (this->LineActor->GetVisibility())
    {
    count += this->LineActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  count = this->Superclass::RenderTranslucentPolygonalGeometry(viewport);

  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerLineRepresentation2D::HasTranslucentPolygonalGeometry()
{
  int result=0;
  if (this->LineActor->GetVisibility())
    {
    result |= this->LineActor->HasTranslucentPolygonalGeometry();
    }
  result |= this->Superclass::HasTranslucentPolygonalGeometry();

  return result;
}

//----------------------------------------------------------------------
double *vtkSlicerLineRepresentation2D::GetBounds()
{
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->LineActor)
    {
    os << indent << "Line Actor Visibility: " << this->LineActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Line Actor: (none)\n";
    }

}
