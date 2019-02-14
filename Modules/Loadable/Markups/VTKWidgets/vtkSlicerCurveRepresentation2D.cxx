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

#include "vtkSlicerCurveRepresentation2D.h"
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
#include "vtkBezierSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkAppendPolyData.h"
#include "vtkTubeFilter.h"
#include "vtkStringArray.h"
#include "vtkPickingManager.h"
#include "vtkVectorText.h"
#include "vtkOpenGLTextActor.h"
#include "cmath"
#include "vtkMRMLMarkupsDisplayNode.h"

vtkStandardNewMacro(vtkSlicerCurveRepresentation2D);

//----------------------------------------------------------------------
vtkSlicerCurveRepresentation2D::vtkSlicerCurveRepresentation2D()
{
  this->LineInterpolator = vtkSmartPointer<vtkBezierSlicerLineInterpolator>::New();

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
vtkSlicerCurveRepresentation2D::~vtkSlicerCurveRepresentation2D()
{
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::BuildLines()
{
  this->BuildLine(this->Line, true);
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::BuildRepresentation()
{
  // Make sure we are up to date with any changes made in the placer
  //this->UpdateWidget(true);

  Superclass::BuildRepresentation();

  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (markupsNode == nullptr)
    {
    return;
    }

  vtkMRMLMarkupsDisplayNode* markupsDisplayNode = this->GetMarkupsDisplayNode();
  if (markupsDisplayNode == nullptr)
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

  //this->BuildRepresentationPointsAndLabels(scale * this->ControlPointSize);

  bool allNodeVisibile = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (!this->PointsVisibilityOnSlice->GetValue(ii) ||
        !this->GetNthNodeVisibility(ii))
      {
      allNodeVisibile = false;
      break;
      }
    }

  this->LineActor->SetVisibility(allNodeVisibile);

  bool allNodeSelected = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (!this->GetNthNodeSelected(ii))
      {
      allNodeSelected = false;
      break;
      }
    }

  if (markupsDisplayNode->GetActiveComponentType() == vtkMRMLMarkupsDisplayNode::ComponentLine)
    {
    this->LineActor->SetProperty(this->GetControlPointsPipeline(Active)->Property);
    }
  else if (allNodeSelected)
    {
    this->LineActor->SetProperty(this->GetControlPointsPipeline(Selected)->Property);
    }
  else
    {
    this->LineActor->SetProperty(this->GetControlPointsPipeline(Unselected)->Property);
    }

  bool allNodesHidden = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (this->PointsVisibilityOnSlice->GetValue(ii) &&
        this->GetNthNodeVisibility(ii))
      {
      allNodesHidden = false;
      break;
      }
    }

  if (this->ClosedLoop && this->GetNumberOfNodes() > 2 && this->CentroidVisibilityOnSlice && !allNodesHidden)
  {
    double centroidPosWorld[3], centroidPosDisplay[3], orient[3] = { 0 };
    markupsNode->GetCentroidPosition(centroidPosWorld);
    this->GetWorldToSliceCoordinates(centroidPosWorld, centroidPosDisplay);
    int centroidControlPointType = allNodeSelected ? Selected : Unselected;
    if (markupsDisplayNode->GetActiveComponentType() == vtkMRMLMarkupsDisplayNode::ComponentCentroid)
    {
      centroidControlPointType = Active;
      this->GetControlPointsPipeline(centroidControlPointType)->ControlPoints->SetNumberOfPoints(0);
      this->GetControlPointsPipeline(centroidControlPointType)->ControlPointsPolyData->GetPointData()->GetNormals()->SetNumberOfTuples(0);
    }
    this->GetControlPointsPipeline(centroidControlPointType)->ControlPoints->InsertNextPoint(centroidPosDisplay);
    this->GetControlPointsPipeline(centroidControlPointType)->ControlPointsPolyData->GetPointData()->GetNormals()->InsertNextTuple(orient);

    this->GetControlPointsPipeline(centroidControlPointType)->ControlPoints->Modified();
    this->GetControlPointsPipeline(centroidControlPointType)->ControlPointsPolyData->GetPointData()->GetNormals()->Modified();
    this->GetControlPointsPipeline(centroidControlPointType)->ControlPointsPolyData->Modified();
    if (centroidControlPointType == Active)
    {
      this->GetControlPointsPipeline(centroidControlPointType)->Actor->VisibilityOn();
      this->GetControlPointsPipeline(centroidControlPointType)->LabelsActor->VisibilityOff();
    }
  }
}

//----------------------------------------------------------------------
int vtkSlicerCurveRepresentation2D::CanInteract(const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &componentIndex)
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

  // TODO: implement quick search of nearby points on the curve
  return foundComponentType;
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::GetActors(vtkPropCollection *pc)
{
  this->LineActor->GetActors(pc);
  this->Superclass::GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->LineActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerCurveRepresentation2D::RenderOverlay(vtkViewport *viewport)
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
int vtkSlicerCurveRepresentation2D::RenderOpaqueGeometry(
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
  count += this->Superclass::RenderOpaqueGeometry(viewport);

  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerCurveRepresentation2D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  if (this->LineActor->GetVisibility())
    {
    count += this->LineActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  count += this->Superclass::RenderTranslucentPolygonalGeometry(viewport);

  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerCurveRepresentation2D::HasTranslucentPolygonalGeometry()
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
double *vtkSlicerCurveRepresentation2D::GetBounds()
{
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkSlicerCurveRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
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
