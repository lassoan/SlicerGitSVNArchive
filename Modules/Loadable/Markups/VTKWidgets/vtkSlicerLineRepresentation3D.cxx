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

#include "vtkSlicerLineRepresentation3D.h"
#include "vtkCleanPolyData.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLActor.h"
#include "vtkActor2D.h"
#include "vtkAssemblyPath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
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
#include "vtkLinearSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkPropPicker.h"
#include "vtkPickingManager.h"
#include "vtkAppendPolyData.h"
#include "vtkStringArray.h"
#include "vtkTubeFilter.h"
#include "vtkMRMLMarkupsDisplayNode.h"

vtkStandardNewMacro(vtkSlicerLineRepresentation3D);

//----------------------------------------------------------------------
vtkSlicerLineRepresentation3D::vtkSlicerLineRepresentation3D()
{
  this->LineInterpolator = vtkSmartPointer<vtkLinearSlicerLineInterpolator>::New();

  this->Line = vtkSmartPointer<vtkPolyData>::New();
  this->TubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
  this->TubeFilter->SetInputData(Line);
  this->TubeFilter->SetNumberOfSides(20);
  this->TubeFilter->SetRadius(1);

  this->LineMapper = vtkSmartPointer<vtkOpenGLPolyDataMapper>::New();
  this->LineMapper->SetInputConnection(this->TubeFilter->GetOutputPort());
  this->LineMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->LineMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1,-1);
  this->LineMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
  this->LineMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->LineActor = vtkSmartPointer<vtkOpenGLActor>::New();
  this->LineActor->SetMapper(this->LineMapper);
  this->LineActor->SetProperty(this->GetControlPointsPipeline(Unselected)->Property);
}

//----------------------------------------------------------------------
vtkSlicerLineRepresentation3D::~vtkSlicerLineRepresentation3D()
{
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::BuildLines()
{
  this->BuildLine(this->Line);
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::GetActors(vtkPropCollection *pc)
{
  this->Superclass::GetActors(pc);
  this->LineActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->LineActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerLineRepresentation3D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  count = this->Superclass::RenderOverlay(viewport);
  if (this->LineActor->GetVisibility())
  {
    count +=  this->LineActor->RenderOverlay(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerLineRepresentation3D::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();

  int count=0;
  count = this->Superclass::RenderOpaqueGeometry(viewport);
  if (this->LineActor->GetVisibility())
  {
    count += this->LineActor->RenderOpaqueGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerLineRepresentation3D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  count = this->Superclass::RenderTranslucentPolygonalGeometry(viewport);
  if (this->LineActor->GetVisibility())
  {
    count += this->LineActor->RenderTranslucentPolygonalGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerLineRepresentation3D::HasTranslucentPolygonalGeometry()
{
  int result=0;
  result |= this->Superclass::HasTranslucentPolygonalGeometry();
  if (this->LineActor->GetVisibility())
  {
    result |= this->LineActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}

//----------------------------------------------------------------------
double *vtkSlicerLineRepresentation3D::GetBounds()
{
  vtkBoundingBox boundingBox;
  const std::vector<vtkProp*> actors({ this->LineActor });
  this->AddActorsBounds(boundingBox, actors, Superclass::GetBounds());
  boundingBox.GetBounds(this->Bounds);
  return this->Bounds;
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::BuildRepresentation()
{
  // Make sure we are up to date with any changes made in the placer
  //this->UpdateWidget(true);

  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }
  if (!this->MarkupsDisplayNode)
  {
    return;
  }

  for (int controlPointType = 0; controlPointType < NumberOfControlPointTypes; ++controlPointType)
  {
    ControlPointsPipeline3D* controlPoints = this->GetControlPointsPipeline(controlPointType);
    controlPoints->LabelsActor->SetVisibility(this->MarkupsDisplayNode->GetTextVisibility());
    controlPoints->Glypher->SetScaleFactor(this->ControlPointSize);

    if (this->AlwaysOnTop)
    {
      // max value 65536 so we subtract 66000 to make sure we are
      // zero or negative
      controlPoints->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
      controlPoints->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
      controlPoints->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    }
    else
    {
      controlPoints->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
      controlPoints->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
      controlPoints->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    }
  }

  if (this->AlwaysOnTop)
    {
    // max value 65536 so we subtract 66000 to make sure we are
    // zero or negative
    this->LineMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    this->LineMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    this->LineMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    }
  else
    {
    this->LineMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    this->LineMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
    this->LineMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    }

  this->TubeFilter->SetRadius(this->ControlPointSize * 0.125);

  this->BuildRepresentationPointsAndLabels();

  bool lineVisibility = true;
  for (int ii = 0; ii < this->GetNumberOfNodes(); ii++)
    {
    if (!this->GetNthNodeVisibility(ii))
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
  else if (!this->GetNthNodeSelected(0) ||
    (this->GetNumberOfNodes() > 1 && !this->GetNthNodeSelected(1)) ||
    (this->GetNumberOfNodes() > 2 && !this->GetNthNodeSelected(2)))
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
int vtkSlicerLineRepresentation3D::CanInteract(const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &componentIndex)
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

//-----------------------------------------------------------------------------
void vtkSlicerLineRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->LineActor)
    {
    os << indent << "Line Visibility: " << this->LineActor->GetVisibility() << "\n";
    }
  else
    {
    os << indent << "Line Visibility: (none)\n";
    }
}
