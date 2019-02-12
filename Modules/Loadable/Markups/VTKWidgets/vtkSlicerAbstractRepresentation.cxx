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

#include "vtkSlicerAbstractRepresentation.h"
#include "vtkCleanPolyData.h"
#include "vtkGeneralTransform.h"
#include "vtkMRMLTransformNode.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
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
#include "vtkSlicerLineInterpolator.h"
#include "vtkLinearSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkBox.h"
#include "vtkIntArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointPlacer.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWindow.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkStringArray.h"
#include "vtkLabelHierarchy.h"
#include "vtkTextProperty.h"

#include <set>
#include <algorithm>
#include <iterator>

vtkSlicerAbstractRepresentation::ControlPointsPipeline::ControlPointsPipeline()
{
  this->TextProperty = vtkSmartPointer<vtkTextProperty>::New();
  this->TextProperty->SetFontSize(15);
  this->TextProperty->SetFontFamily(vtkTextProperty::GetFontFamilyFromString("Arial"));
  this->TextProperty->SetColor(0.4, 1.0, 1.0);
  this->TextProperty->SetOpacity(1.);

  this->ControlPoints = vtkSmartPointer<vtkPoints>::New();
  this->ControlPoints->Allocate(100);
  this->ControlPoints->SetNumberOfPoints(1);
  this->ControlPoints->SetPoint(0, 0.0, 0.0, 0.0);

  vtkNew<vtkDoubleArray> controlPointNormals;
  controlPointNormals->SetNumberOfComponents(3);
  controlPointNormals->Allocate(100);
  controlPointNormals->SetNumberOfTuples(1);
  double n[3] = { 0, 0, 0 };
  controlPointNormals->SetTuple(0, n);

  this->ControlPointsPolyData = vtkSmartPointer<vtkPolyData>::New();
  this->ControlPointsPolyData->SetPoints(this->ControlPoints);
  this->ControlPointsPolyData->GetPointData()->SetNormals(controlPointNormals);

  this->LabelControlPoints = vtkSmartPointer<vtkPoints>::New();
  this->LabelControlPoints->Allocate(100);
  this->LabelControlPoints->SetNumberOfPoints(1);
  this->LabelControlPoints->SetPoint(0, 0.0, 0.0, 0.0);

  vtkNew<vtkDoubleArray> labelNormals;
  labelNormals->SetNumberOfComponents(3);
  labelNormals->Allocate(100);
  labelNormals->SetNumberOfTuples(1);
  labelNormals->SetTuple(0, n);

  this->LabelControlPointsPolyData = vtkSmartPointer<vtkPolyData>::New();
  this->LabelControlPointsPolyData->SetPoints(this->LabelControlPoints);
  this->LabelControlPointsPolyData->GetPointData()->SetNormals(labelNormals);

  this->Labels = vtkSmartPointer<vtkStringArray>::New();
  this->Labels->SetName("labels");
  this->Labels->Allocate(100);
  this->Labels->SetNumberOfValues(1);
  this->Labels->SetValue(0, "F");
  this->LabelsPriority = vtkSmartPointer<vtkStringArray>::New();
  this->LabelsPriority->SetName("priority");
  this->LabelsPriority->Allocate(100);
  this->LabelsPriority->SetNumberOfValues(1);
  this->LabelsPriority->SetValue(0, "1");
  this->LabelControlPointsPolyData->GetPointData()->AddArray(this->Labels);
  this->LabelControlPointsPolyData->GetPointData()->AddArray(this->LabelsPriority);
  this->PointSetToLabelHierarchyFilter = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
  this->PointSetToLabelHierarchyFilter->SetTextProperty(this->TextProperty);
  this->PointSetToLabelHierarchyFilter->SetLabelArrayName("labels");
  this->PointSetToLabelHierarchyFilter->SetPriorityArrayName("priority");
  this->PointSetToLabelHierarchyFilter->SetInputData(this->LabelControlPointsPolyData);
};

//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation::vtkSlicerAbstractRepresentation()
{
  this->Tolerance                = 0.4;
  this->PixelTolerance           = 1;
  this->Locator                  = nullptr;
  this->RebuildLocator           = false;
  this->NeedToRender             = 0;
  this->ClosedLoop               = 0;

  this->ResetLocator();

  this->PointPlacer = vtkSmartPointer<vtkFocalPlanePointPlacer>::New();

  for (int i = 0; i<NumberOfControlPointTypes; i++)
  {
    this->ControlPoints[i] = nullptr;
  }

  this->AlwaysOnTop = 0;

  this->RestrictFlag = RestrictNone;
}

//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation::~vtkSlicerAbstractRepresentation()
{
  for (int i=0; i<NumberOfControlPointTypes; i++)
  {
    delete this->ControlPoints[i];
    this->ControlPoints[i] = nullptr;
  }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::ResetLocator()
{
  this->Locator = vtkSmartPointer<vtkIncrementalOctreePointLocator>::New();
  this->Locator->SetBuildCubicOctree(1);
  this->RebuildLocator = true;
}

//----------------------------------------------------------------------
double vtkSlicerAbstractRepresentation::CalculateViewScaleFactor()
{
  double p1[4], p2[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint(p1);
  p1[3] = 1.0;
  this->Renderer->SetWorldPoint(p1);
  this->Renderer->WorldToView();
  this->Renderer->GetViewPoint(p1);

  double depth = p1[2];
  double aspect[2];
  this->Renderer->ComputeAspect();
  this->Renderer->GetAspect(aspect);

  p1[0] = -aspect[0];
  p1[1] = -aspect[1];
  this->Renderer->SetViewPoint(p1);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(p1);

  p2[0] = aspect[0];
  p2[1] = aspect[1];
  p2[2] = depth;
  p2[3] = 1.0;
  this->Renderer->SetViewPoint(p2);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(p2);

  double distance = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));

  int *size = this->Renderer->GetRenderWindow()->GetSize();
  double viewport[4];
  this->Renderer->GetViewport(viewport);

  double x, y, distance2;

  x = size[0] * (viewport[2] - viewport[0]);
  y = size[1] * (viewport[3] - viewport[1]);

  distance2 = sqrt(x * x + y * y);
  return distance2 / distance;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::ClearAllNodes()
{
  this->ResetLocator();
  this->BuildLines();
  this->BuildLocator();
  this->NeedToRender = 1;
  this->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::AddNodeAtPositionInternal(const double worldPos[3])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    return;
    }

  // Add a new point at this position
  vtkVector3d pos(worldPos[0], worldPos[1], worldPos[2]);
  markupsNode->DisableModifiedEventOn();
  markupsNode->AddControlPoint(pos);
  markupsNode->DisableModifiedEventOff();

  this->UpdateLines(this->GetNumberOfNodes() - 1);
  this->NeedToRender = 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtWorldPosition(const double worldPos[3])
{
  this->AddNodeAtPositionInternal(worldPos);
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtWorldPosition(
  double x, double y, double z)
{
  double worldPos[3] = {x, y, z};
  return this->AddNodeAtWorldPosition(worldPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtDisplayPosition(const double displayPos[2])
{
  double worldPos[3], worldOrient[9], orientation[4];
  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               (double*)displayPos, worldPos,
                                               worldOrient))
    {
    return 0;
    }

  this->AddNodeAtPositionInternal(worldPos);
  this->FromWorldOrientToOrientationQuaternion(worldOrient, orientation);
  this->SetNthNodeOrientation(this->GetNumberOfNodes() - 1,  orientation);
  return 1;
}
//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtDisplayPosition(const int displayPos[2])
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->AddNodeAtDisplayPosition(doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeAtDisplayPosition(int X, int Y)
{
  double displayPos[2];
  displayPos[0] = X;
  displayPos[1] = Y;
  return this->AddNodeAtDisplayPosition(displayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetActiveNodeToWorldPosition(const double worldPos[3])
{
  if (this->GetActiveNode() < 0 ||
       this->GetActiveNode() >= this->GetNumberOfNodes())
    {
    return 0;
    }

  // Check if this is a valid location
  if (!this->PointPlacer->ValidateWorldPosition((double*)worldPos))
    {
    return 0;
    }

  this->SetNthNodeWorldPositionInternal(this->GetActiveNode(),
                                        worldPos);
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetActiveNodeToDisplayPosition(const double displayPos[2])
{
  if (this->GetActiveNode() < 0 ||
       this->GetActiveNode() >= this->GetNumberOfNodes())
    {
    return 0;
    }

  double worldPos[3], worldOrient[9];
  this->GetActiveNodeOrientation(worldOrient);

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               (double*)displayPos, worldPos,
                                               worldOrient))
    {
    return 0;
    }

  this->SetNthNodeWorldPositionInternal(this->GetActiveNode(),
                                        worldPos);
  this->SetActiveNodeOrientation(worldOrient);
  return 1;
}
//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetActiveNodeToDisplayPosition(const int displayPos[2])
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->SetActiveNodeToDisplayPosition(doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetActiveNodeToDisplayPosition(int X, int Y)
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = X;
  doubleDisplayPos[1] = Y;
  return this->SetActiveNodeToDisplayPosition(doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetActiveNode()
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return -1;
  }

  return markupsNode->GetActiveControlPoint();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNode(int index)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }

  markupsNode->SetActiveControlPoint(index);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetActiveNodeWorldPosition(double pos[3])
{
  return this->GetNthNodeWorldPosition(this->GetActiveNode(), pos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetActiveNodeDisplayPosition(double pos[2])
{
  return this->GetNthNodeDisplayPosition(this->GetActiveNode(), pos);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeVisibility(bool visibility)
{
  this->SetNthNodeVisibility(this->GetActiveNode(), visibility);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetActiveNodeVisibility()
{
  return this->GetNthNodeVisibility(this->GetActiveNode());
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeSelected(bool selected)
{
  this->SetNthNodeSelected(this->GetActiveNode(), selected);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetActiveNodeSelected()
{
  return this->GetNthNodeSelected(this->GetActiveNode());
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeLocked(bool locked)
{
  this->SetNthNodeLocked(this->GetActiveNode(), locked);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetActiveNodeLocked()
{
  return this->GetNthNodeLocked(this->GetActiveNode());
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeOrientation(double orientation[4])
{
  this->SetNthNodeOrientation(this->GetActiveNode(), orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::GetActiveNodeOrientation(double orientation[4])
{
  this->GetNthNodeOrientation(this->GetActiveNode(), orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetActiveNodeLabel(vtkStdString label)
{
  this->SetNthNodeLabel(this->GetActiveNode(), label);
}

//----------------------------------------------------------------------
vtkStdString vtkSlicerAbstractRepresentation::GetActiveNodeLabel()
{
  return this->GetNthNodeLabel(this->GetActiveNode());
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetNumberOfNodes()
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return 0;
  }

  return static_cast<int>(markupsNode->GetNumberOfControlPoints());
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetNumberOfIntermediatePoints(int n)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return 0;
  }

  if (!this->NodeExists(n))
    {
    return 0;
    }

  return static_cast<int> (markupsNode->GetNthControlPoint(n)->IntermediatePositions.size());
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetIntermediatePointWorldPosition(int n,
                                                                       int idx,
                                                                       double point[3])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return 0;
  }

  if (!this->NodeExists(n))
    {
    return 0;
    }

  if (idx < 0 ||
       static_cast<unsigned int>(idx) >= this->GetNthNode(n)->IntermediatePositions.size())
    {
    return 0;
    }

  vtkVector3d intermediatePosition = this->GetNthNode(n)->IntermediatePositions[static_cast<unsigned int> (idx)];
  markupsNode->TransformPointToWorld(intermediatePosition.GetData(), point);

  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetIntermediatePointDisplayPosition(int n,
                                                                         int idx,
                                                                         double displayPos[2])
{
  double pos[4] = {0.0, 0.0, 0.0, 1.0};
  if (!vtkSlicerAbstractRepresentation::GetIntermediatePointWorldPosition(n, idx, pos))
    {
    return 0;
    }

  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);

  displayPos[0] = pos[0];
  displayPos[1] = pos[1];
  return 1;
}

//----------------------------------------------------------------------
// The display position for a given world position must be re-computed
// from the world positions... It should not be queried from the renderer
// whose camera position may have changed
int vtkSlicerAbstractRepresentation::GetNthNodeDisplayPosition(int n, double displayPos[2])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return 0;
  }

  if (!this->NodeExists(n))
    {
    return 0;
    }

  double pos[4] = { 0.0, 0.0, 0.0, 1.0 };
  markupsNode->TransformPointToWorld(this->GetNthNode(n)->Position.GetData(), pos);

  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);

  displayPos[0] = pos[0];
  displayPos[1] = pos[1];
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetNthNodeWorldPosition(int n, double worldPos[3])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return 0;
  }

  if (!this->NodeExists(n))
    {
    return 0;
    }

  markupsNode->GetNthControlPointPositionWorld(n, worldPos);

  return 1;
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetNthNodeVisibility(int n)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return false;
  }

  if (!this->NodeExists(n))
    {
    return false;
    }

  return markupsNode->GetNthControlPointVisibility(n);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeVisibility(int n, bool visibility)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }

  if (!this->NodeExists(n))
    {
    return;
    }

  return markupsNode->SetNthControlPointVisibility(n, visibility);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetNthNodeSelected(int n)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    return false;
    }
  if (!this->NodeExists(n))
    {
    return false;
    }

  return markupsNode->GetNthControlPointSelected(n);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeSelected(int n, bool selected)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    return;
    }
  if (!this->NodeExists(n))
    {
    return;
    }

  return markupsNode->SetNthControlPointSelected(n, selected);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::GetNthNodeLocked(int n)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return false;
  }
  if (!this->NodeExists(n))
  {
    return false;
  }

  return markupsNode->GetNthControlPointLocked(n);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeLocked(int n, bool locked)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }
  if (!this->NodeExists(n))
  {
    return;
  }

  return markupsNode->SetNthControlPointLocked(n, locked);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeOrientation(int n, double orientation[4])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }
  if (!this->NodeExists(n))
  {
    return;
  }

  markupsNode->SetNthControlPointOrientationFromArray(n, orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::GetNthNodeOrientation(int n, double orientation[4])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }
  if (!this->NodeExists(n))
  {
    return;
  }

  markupsNode->GetNthControlPointOrientation(n, orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeLabel(int n, vtkStdString label)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }
  if (!this->NodeExists(n))
  {
    return;
  }

  markupsNode->SetNthControlPointLabel(n, label);
}

//----------------------------------------------------------------------
vtkStdString vtkSlicerAbstractRepresentation::GetNthNodeLabel(int n)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return nullptr;
  }
  if (!this->NodeExists(n))
  {
    return nullptr;
  }

  return markupsNode->GetNthControlPointLabel(n);
}

//----------------------------------------------------------------------
ControlPoint* vtkSlicerAbstractRepresentation::GetNthNode(int n)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return nullptr;
  }
  if (!this->NodeExists(n))
  {
    return nullptr;
  }

  return markupsNode->GetNthControlPoint(n);
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractRepresentation::NodeExists(int n)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return false;
  }
  if (!this->NodeExists(n))
  {
    return false;
  }

  return markupsNode->ControlPointExists(n);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetNthNodeWorldPositionInternal(int n, const double worldPos[3])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
  }
  if (!this->NodeExists(n))
  {
    return;
  }

  markupsNode->SetNthControlPointPositionWorldFromArray(n, worldPos);

  this->UpdateLines(n);
  this->NeedToRender = 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::FromWorldOrientToOrientationQuaternion(const double worldOrient[9], double orientation[4])
{
  if (!worldOrient || !orientation)
    {
    return;
    }

  double worldOrientMatrix[3][3];
  worldOrientMatrix[0][0] = worldOrient[0];
  worldOrientMatrix[0][1] = worldOrient[1];
  worldOrientMatrix[0][2] = worldOrient[2];
  worldOrientMatrix[1][0] = worldOrient[3];
  worldOrientMatrix[1][1] = worldOrient[4];
  worldOrientMatrix[1][2] = worldOrient[5];
  worldOrientMatrix[2][0] = worldOrient[6];
  worldOrientMatrix[2][1] = worldOrient[7];
  worldOrientMatrix[2][2] = worldOrient[8];

  vtkMath::Matrix3x3ToQuaternion(worldOrientMatrix, orientation);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::FromOrientationQuaternionToWorldOrient(const double orientation[4], double worldOrient[9])
{
  if (!worldOrient || !orientation)
    {
    return;
    }

  double worldOrientMatrix[3][3];
  vtkMath::QuaternionToMatrix3x3(orientation, worldOrientMatrix);

  worldOrient[0] = worldOrientMatrix[0][0];
  worldOrient[1] = worldOrientMatrix[0][1];
  worldOrient[2] = worldOrientMatrix[0][2];
  worldOrient[3] = worldOrientMatrix[1][0];
  worldOrient[4] = worldOrientMatrix[1][1];
  worldOrient[5] = worldOrientMatrix[1][2];
  worldOrient[6] = worldOrientMatrix[2][0];
  worldOrient[7] = worldOrientMatrix[2][1];
  worldOrient[8] = worldOrientMatrix[2][2];
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetNthNodeWorldPosition(int n, const double worldPos[3])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  // Check if this is a valid location
  if (!this->PointPlacer->ValidateWorldPosition((double*)worldPos))
    {
    return 0;
    }

  this->SetNthNodeWorldPositionInternal(n, worldPos);
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetNthNodeDisplayPosition(int n, const double displayPos[2])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  double worldPos[3], worldOrient[9], orientation[4];
  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                                 (double*)displayPos, worldPos,
                                                 worldOrient))
    {
    return 0;
    }

  this->FromWorldOrientToOrientationQuaternion(worldOrient, orientation);
  this->SetNthNodeOrientation(n,  orientation);
  return this->SetNthNodeWorldPosition(n, worldPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetNthNodeDisplayPosition(int n, const int displayPos[2])
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->SetNthNodeDisplayPosition(n, doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::SetNthNodeDisplayPosition(int n, int X, int Y)
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = X;
  doubleDisplayPos[1] = Y;
  return this->SetNthNodeDisplayPosition(n, doubleDisplayPos);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::FindClosestPointOnWidget(int X, int Y,
                                                              double closestWorldPos[3],
                                                              int *idx)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return 0;
  }

  // Make a line out of this viewing ray
  double p1[4], p2[4];

  double tmp1[4], tmp2[4];
  tmp1[0] = X;
  tmp1[1] = Y;
  tmp1[2] = 0.0;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(p1);

  tmp1[2] = 1.0;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(p2);

  double closestDistance2 = VTK_DOUBLE_MAX;
  int closestNode=0;

  // compute a world tolerance based on pixel
  // tolerance on the focal plane
  double fp[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  fp[3] = 1.0;
  this->Renderer->SetWorldPoint(fp);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(tmp1);

  tmp1[0] = 0;
  tmp1[1] = 0;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(tmp2);

  tmp1[0] = this->PixelTolerance;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(tmp1);

  double wt2 = vtkMath::Distance2BetweenPoints(tmp1, tmp2);

  vtkNew<vtkGeneralTransform> nodeToWorldTransform;
  vtkMRMLTransformNode::GetTransformBetweenNodes(markupsNode->GetParentTransformNode(), NULL, nodeToWorldTransform.GetPointer());

  // Now loop through all lines and look for closest one within tolerance
  double p3[4] = {0.0, 0.0, 0.0, 1.0};
  double p4[4] = {0.0, 0.0, 0.0, 1.0};
  for(int i = 0; i < this->GetNumberOfNodes(); i++)
    {
    if (!this->NodeExists(i))
      {
      continue;
      }
    for (unsigned int j = 0; j <= this->GetNthNode(i)->IntermediatePositions.size(); j++)
      {
      if (j == 0)
        {
        nodeToWorldTransform->TransformPoint(this->GetNthNode(i)->Position.GetData(), p3);
        if (!this->GetNthNode(i)->IntermediatePositions.empty())
          {
          nodeToWorldTransform->TransformPoint(this->GetNthNode(i)->IntermediatePositions[j].GetData(), p4);
          }
        else
          {
          if (i < this->GetNumberOfNodes() - 1)
            {
            nodeToWorldTransform->TransformPoint(this->GetNthNode(i + 1)->Position.GetData(), p4);
            }
          else if (this->ClosedLoop)
            {
            nodeToWorldTransform->TransformPoint(this->GetNthNode(0)->Position.GetData(), p4);
            }
          }
        }
      else if (j == this->GetNthNode(i)->IntermediatePositions.size())
        {
        nodeToWorldTransform->TransformPoint(this->GetNthNode(i)->IntermediatePositions[j-1].GetData(), p3);
        if (i < this->GetNumberOfNodes() - 1)
          {
          nodeToWorldTransform->TransformPoint(this->GetNthNode(i + 1)->Position.GetData(), p4);
          }
        else if (this->ClosedLoop)
          {
          nodeToWorldTransform->TransformPoint(this->GetNthNode(0)->Position.GetData(), p4);
          }
        else
          {
          // Shouldn't be able to get here (only if we don't have
          // a closed loop but we do have intermediate points after
          // the last node - contradictary conditions)
          continue;
          }
        }
      else
        {
        nodeToWorldTransform->TransformPoint(this->GetNthNode(i)->IntermediatePositions[j-1].GetData(), p3);
        nodeToWorldTransform->TransformPoint(this->GetNthNode(i)->IntermediatePositions[j].GetData(), p4);
        }

      // Now we have the four points - check closest intersection
      double u, v;

      if (vtkLine::Intersection(p1, p2, p3, p4, u, v))
        {
        double p5[3], p6[3];
        p5[0] = p1[0] + u*(p2[0]-p1[0]);
        p5[1] = p1[1] + u*(p2[1]-p1[1]);
        p5[2] = p1[2] + u*(p2[2]-p1[2]);

        p6[0] = p3[0] + v*(p4[0]-p3[0]);
        p6[1] = p3[1] + v*(p4[1]-p3[1]);
        p6[2] = p3[2] + v*(p4[2]-p3[2]);

        double d = vtkMath::Distance2BetweenPoints(p5, p6);

        if (d < wt2 && d < closestDistance2)
          {
          closestWorldPos[0] = p6[0];
          closestWorldPos[1] = p6[1];
          closestWorldPos[2] = p6[2];
          closestDistance2 = d;
          closestNode = static_cast<int>(i);
          }
        }
      else
        {
        double d = vtkLine::DistanceToLine(p3, p1, p2);
        if (d < wt2 && d < closestDistance2)
          {
          closestWorldPos[0] = p3[0];
          closestWorldPos[1] = p3[1];
          closestWorldPos[2] = p3[2];
          closestDistance2 = d;
          closestNode = static_cast<int>(i);
          }

        d = vtkLine::DistanceToLine(p4, p1, p2);
        if (d < wt2 && d < closestDistance2)
          {
          closestWorldPos[0] = p4[0];
          closestWorldPos[1] = p4[1];
          closestWorldPos[2] = p4[2];
          closestDistance2 = d;
          closestNode = static_cast<int>(i);
          }
        }
      }
    }

  if (closestDistance2 < VTK_DOUBLE_MAX)
    {
    if (closestNode < this->GetNumberOfNodes() -1)
      {
      *idx = closestNode+1;
      return 1;
      }
    else if (this->ClosedLoop)
      {
      *idx = 0;
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddNodeOnWidget(int X, int Y)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    return 0;
    }

  int idx;
  double worldPos[3], worldOrient[9], displayPos[2], orientation[4];
  displayPos[0] = X;
  displayPos[1] = Y;

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, worldPos,
                                               worldOrient))
    {
    return 0;
    }

  double pos[3];
  if (!this->FindClosestPointOnWidget(X, Y, pos, &idx))
    {
    return 0;
    }

  if (!this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos,
                                               pos,
                                               worldPos,
                                               worldOrient))
    {
    return 0;
    }

  // Add a new point at this position
  ControlPoint *node = new ControlPoint;
  markupsNode->InitControlPoint(node);
  markupsNode->TransformPointFromWorld(vtkVector3d(worldPos), node->Position);

  markupsNode->DisableModifiedEventOn();
  markupsNode->InsertControlPoint(node, idx);
  this->FromWorldOrientToOrientationQuaternion(worldOrient, orientation);
  this->SetNthNodeOrientation(idx,  orientation);
  markupsNode->DisableModifiedEventOff();

  this->UpdateLines(idx);
  this->NeedToRender = 1;

  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::DeleteNthNode(int n)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    return 0;
    }
  if (!this->NodeExists(n))
    {
    return 0;
    }

  markupsNode->DisableModifiedEventOn();
  markupsNode->RemoveNthControlPoint(n);
  markupsNode->DisableModifiedEventOff();

  this->UpdateLines(n - 1);

  this->NeedToRender = 1;
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::DeleteActiveNode()
{
  return this->DeleteNthNode(this->GetActiveNode());
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::DeleteLastNode()
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return 0;
  }

  markupsNode->DisableModifiedEventOn();
  markupsNode->RemoveLastControlPoint();
  markupsNode->DisableModifiedEventOff();

  this->UpdateLines(this->GetNumberOfNodes() - 1);

  this->NeedToRender = 1;
  return 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::UpdateLines(int index)
{
  int indices[2];

  if (this->LineInterpolator)
    {
    vtkIntArray *arr = vtkIntArray::New();
    this->LineInterpolator->GetSpan(index, arr, this);

    int nNodes = static_cast<int> (arr->GetNumberOfTuples());
    for (int i = 0; i < nNodes; i++)
      {
      arr->GetTypedTuple(i, indices);
      this->UpdateLine(indices[0], indices[1]);
      }
    arr->Delete();
    }

  // A check to make sure that we have no line segments in
  // the last node if the loop is not closed
  if (!this->ClosedLoop && this->GetNumberOfNodes() > 0)
    {
    int idx = this->GetNumberOfNodes() - 1;
    this->GetNthNode(idx)->IntermediatePositions.clear();
    }

  this->BuildLines();
  this->RebuildLocator = true;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::AddIntermediatePointWorldPosition(int n,
                                                                       const double posWorld[3])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    return 0;
    }
  if (!this->NodeExists(n))
    {
    return 0;
    }

  double pos[3];
  markupsNode->TransformPointFromWorld(posWorld, pos);
  this->GetNthNode(n)->IntermediatePositions.push_back(vtkVector3d(pos));
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::GetNthNodeSlope(int n, double slope[3])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  int idx1, idx2;

  if (n == 0 && !this->ClosedLoop)
    {
    idx1 = 0;
    idx2 = 1;
    }
  else if (n == this->GetNumberOfNodes() - 1 && !this->ClosedLoop)
    {
    idx1 = this->GetNumberOfNodes() - 2;
    idx2 = idx1+1;
    }
  else
    {
    idx1 = n - 1;
    idx2 = n + 1;

    if (idx1 < 0)
      {
      idx1 += this->GetNumberOfNodes();
      }
    if (idx2 >= this->GetNumberOfNodes())
      {
      idx2 -= this->GetNumberOfNodes();
      }
    }

  slope[0] =
    this->GetNthNode(idx2)->Position.GetX() -
    this->GetNthNode(idx1)->Position.GetX();
  slope[1] =
    this->GetNthNode(idx2)->Position.GetY() -
    this->GetNthNode(idx1)->Position.GetY();
  slope[2] =
    this->GetNthNode(idx2)->Position.GetZ() -
    this->GetNthNode(idx1)->Position.GetZ();

  vtkMath::Normalize(slope);
  return 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::UpdateLine(int idx1, int idx2)
{
  if (!this->LineInterpolator || !this->NodeExists(idx1))
    {
    return;
    }

  // Clear all the points at idx1
  this->GetNthNode(idx1)->IntermediatePositions.clear();

  this->LineInterpolator->InterpolateLine(this, idx1, idx2);
}

//---------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::UpdateWidget(bool force /*=false*/)
{
  if (!this->Locator || !this->PointPlacer)
    {
    return 0;
    }

  this->PointPlacer->UpdateInternalState();

  //even if just the camera has moved we need to mark the locator
  //as needing to be rebuilt
  if (this->Locator->GetMTime() < this->Renderer->GetActiveCamera()->GetMTime())
    {
    this->RebuildLocator = true;
    }

  if (this->WidgetBuildTime > this->PointPlacer->GetMTime() && !force)
    {
    // Widget does not need to be rebuilt
    return 0;
    }

  for(int i = 0; (i + 1) < this->GetNumberOfNodes(); i++)
    {
    this->UpdateLine(i, i + 1);
    }

  if (this->ClosedLoop)
    {
    this->UpdateLine(this->GetNumberOfNodes() - 1, 0);
    }
  this->BuildLines();
  this->RebuildLocator = true;

  this->WidgetBuildTime.Modified();

  return 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation
::GetRendererComputedDisplayPositionFromWorldPosition(const double worldPos[3],
                                                      double displayPos[2])
{
  double pos[4];
  pos[0] = worldPos[0];
  pos[1] = worldPos[1];
  pos[2] = worldPos[2];
  pos[3] = 1.0;

  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);

  displayPos[0] = static_cast<int>(pos[0]);
  displayPos[1] = static_cast<int>(pos[1]);
}

/*
//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::Initialize(vtkPolyData * pd)
{
  if (!markupsNode)
    {
    return;
    }

  vtkPoints *points   = pd->GetPoints();
  vtkIdType nPoints = points->GetNumberOfPoints();
  if (nPoints <= 0)
    {
    return; // Yeah right.. build from nothing !
    }

  // Clear all existing nodes.
  markupsNode->DisableModifiedEventOn();
  markupsNode->RemoveAllControlPoints();
  markupsNode->DisableModifiedEventOff();

  vtkPolyData *tmpPoints = vtkPolyData::New();
  tmpPoints->DeepCopy(pd);
  this->Locator->SetDataSet(tmpPoints);
  tmpPoints->Delete();

  //reserver space in memory to speed up vector push_back
  markupsNode->GetControlPoints()->reserve(static_cast<unsigned int> (nPoints));
  vtkIdList *pointIds = pd->GetCell(0)->GetPointIds();

  // Get the worldOrient from the point placer
  double ref[3], displayPos[2], worldPos[3], worldOrient[9], orientation[4];
  ref[0] = 0.0; ref[1] = 0.0; ref[2] = 0.0;
  displayPos[0] = 0.0; displayPos[1] = 0.0;
  this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                          displayPos, ref, worldPos, worldOrient);

  // Add nodes without calling rebuild lines
  // to improve performance dramatically(~15x) on large datasets
  double *pos;
  for (vtkIdType i=0; i < nPoints; i++)
    {
    pos = points->GetPoint(i);
    // Check if this is a valid location
    if (!this->PointPlacer->ValidateWorldPosition(pos))
      {
      continue;
      }

    this->GetRendererComputedDisplayPositionFromWorldPosition(pos, displayPos);

    // Add a new point at this position

    markupsNode->DisableModifiedEventOn();
    vtkVector3d controlPointPos(pos[0], pos[1], pos[2]);
    int pointIndex = markupsNode->AddControlPoint(controlPointPos);
    this->FromWorldOrientToOrientationQuaternion(worldOrient, orientation);
    this->SetNthNodeOrientation(pointIndex,  orientation);
    markupsNode->DisableModifiedEventOff();
    }

  if (pointIds->GetNumberOfIds() > nPoints)
    {
    this->ClosedLoopOn();
    }

  // Update the widget representation from the nodes using the line interpolator
  for (int i = 1; i <= nPoints; i++)
    {
    this->UpdateLines(i);
    }
  this->BuildRepresentation();

  // Show the widget.
  this->VisibilityOn();
}
*/

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::BuildLocator()
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    return;
    }
  if (!this->RebuildLocator && !this->NeedToRender)
    {
    return;
    }

  int size = this->GetNumberOfNodes();
  if (size < 1)
    {
    return;
    }

  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(size);

  //setup up the matrixes needed to transform
  //world to display. We are going to do this manually
  // as calling the renderer will create a new matrix for each call
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  matrix->DeepCopy(this->Renderer->GetActiveCamera()
    ->GetCompositeProjectionTransformMatrix(
    this->Renderer->GetTiledAspectRatio(),0,1));

  //viewport info
  double viewPortRatio[2];
  int sizex,sizey;

  /* get physical window dimensions */
  if (this->Renderer->GetVTKWindow())
    {
    double *viewPort = this->Renderer->GetViewport();
    sizex = this->Renderer->GetVTKWindow()->GetSize()[0];
    sizey = this->Renderer->GetVTKWindow()->GetSize()[1];
    viewPortRatio[0] = (sizex * (viewPort[2] - viewPort[0])) / 2.0 +
        sizex*viewPort[0];
    viewPortRatio[1] = (sizey * (viewPort[3] - viewPort[1])) / 2.0 +
        sizey*viewPort[1];
    }
  else
    {
    //can't compute the locator without a vtk window
    return;
    }

  double view[4];
  double pos[3] = {0,0,0};
  double *wp;
  for (int i = 0; i < size; i++)
    {
    if (!markupsNode->ControlPointExists(i))
      {
      continue;
      }
    vtkVector3d worldPos;
    markupsNode->TransformPointToWorld(this->GetNthNode(i)->Position, worldPos);
    double* wp = worldPos.GetData();

    //convert from world to view
    view[0] = wp[0]*matrix->Element[0][0] + wp[1]*matrix->Element[0][1] +
      wp[2]*matrix->Element[0][2] + matrix->Element[0][3];
    view[1] = wp[0]*matrix->Element[1][0] + wp[1]*matrix->Element[1][1] +
      wp[2]*matrix->Element[1][2] + matrix->Element[1][3];
    view[2] = wp[0]*matrix->Element[2][0] + wp[1]*matrix->Element[2][1] +
      wp[2]*matrix->Element[2][2] + matrix->Element[2][3];
    view[3] = wp[0]*matrix->Element[3][0] + wp[1]*matrix->Element[3][1] +
      wp[2]*matrix->Element[3][2] + matrix->Element[3][3];
    if (view[3] != 0.0)
      {
      pos[0] = view[0] / view[3];
      pos[1] = view[1] / view[3];
      }

    //now from view to display
    pos[0] = (pos[0] + 1.0) * viewPortRatio[0];
    pos[1] = (pos[1] + 1.0) * viewPortRatio[1];
    pos[2] = 0;

    points->InsertPoint(i, pos);
    }

  matrix->Delete();
  vtkPolyData *tmp = vtkPolyData::New();
  tmp->SetPoints(points);
  this->Locator->SetDataSet(tmp);
  tmp->Delete();
  points->Delete();

  //we fully updated the display locator
  this->RebuildLocator = false;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetClosedLoop(vtkTypeBool val)
{
  if (this->ClosedLoop != val)
    {
    this->ClosedLoop = val;
    this->UpdateLines(this->GetNumberOfNodes() - 1);
    this->NeedToRender = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::UpdateCentroid()
{
  double p[3], centroidWorldPos[3];
  centroidWorldPos[0] = 0.;
  centroidWorldPos[1] = 0.;
  centroidWorldPos[2] = 0.;

  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    this->GetNthNodeWorldPosition(i, p);
    centroidWorldPos[0] += p[0];
    centroidWorldPos[1] += p[1];
    centroidWorldPos[2] += p[2];
  }
  double inv_N = 1. / static_cast< double >(this->GetNumberOfNodes());
  centroidWorldPos[0] *= inv_N;
  centroidWorldPos[1] *= inv_N;
  centroidWorldPos[2] *= inv_N;

  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (markupsNode)
    {
    markupsNode->SetCentroidPositionFromPointer(centroidWorldPos);
    }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetMarkupsDisplayNode(vtkMRMLMarkupsDisplayNode *markupsDisplayNode)
{
  if (this->MarkupsDisplayNode == markupsDisplayNode)
  {
    return;
  }

  this->MarkupsDisplayNode = markupsDisplayNode;
}

//----------------------------------------------------------------------
vtkMRMLMarkupsDisplayNode *vtkSlicerAbstractRepresentation::GetMarkupsDisplayNode()
{
  return this->MarkupsDisplayNode;
}


//----------------------------------------------------------------------
vtkMRMLMarkupsNode *vtkSlicerAbstractRepresentation::GetMarkupsNode()
{
  if (!this->MarkupsDisplayNode)
  {
    return nullptr;
  }
  return vtkMRMLMarkupsNode::SafeDownCast(this->MarkupsDisplayNode->GetDisplayableNode());
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetRenderer(vtkRenderer *ren)
{
  if ( ren == this->Renderer )
    {
    return;
    }

  // vtkPickingManager reduces perfomances, we don't use it
  this->UnRegisterPickers();
  this->Renderer = ren;
  // register with potentially new picker
  if (this->Renderer)
  {
    this->RegisterPickers();
  }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance <<"\n";
  os << indent << "Rebuild Locator: " <<
     (this->RebuildLocator ? "On" : "Off") << endl;

  os << indent << "Current Operation: ";

  os << indent << "Line Interpolator: " << this->LineInterpolator << "\n";
  os << indent << "Point Placer: " << this->PointPlacer << "\n";

  os << indent << "Always On Top: "
     << (this->AlwaysOnTop ? "On\n" : "Off\n");
}


//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::AddActorsBounds(vtkBoundingBox& boundingBox,
  const std::vector<vtkProp*> &actors, double* additionalBounds /*=nullptr*/)
{
  for (auto actor : actors)
    {
    if (!actor->GetVisibility())
      {
      continue;
      }
    double* bounds = actor->GetBounds();
    if (!bounds)
      {
      continue;
      }
    boundingBox.AddBounds(bounds);
    }
  if (additionalBounds)
    {
    boundingBox.AddBounds(additionalBounds);
    }
}

//-----------------------------------------------------------------------------
int vtkSlicerAbstractRepresentation::CanInteract(const int displayPosition[2], const double position[3], double &closestDistance2, int &itemIndex)
{
  return InteractNone;
}

//-----------------------------------------------------------------------------
vtkTextProperty* vtkSlicerAbstractRepresentation::GetUnselectedTextProperty()
{
  return this->ControlPoints[Unselected]->TextProperty;
}

//-----------------------------------------------------------------------------
vtkTextProperty* vtkSlicerAbstractRepresentation::GetSelectedTextProperty()
{
  return this->ControlPoints[Selected]->TextProperty;
}

//-----------------------------------------------------------------------------
vtkTextProperty* vtkSlicerAbstractRepresentation::GetActiveTextProperty()
{
  return this->ControlPoints[Active]->TextProperty;
}

//-----------------------------------------------------------------------------
vtkPointPlacer* vtkSlicerAbstractRepresentation::GetPointPlacer()
{
  return this->PointPlacer;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetPointPlacer(vtkPointPlacer* placer)
{
  if (this->PointPlacer == placer)
  {
    return;
  }
  this->PointPlacer = placer;
  this->Modified();
}


//-----------------------------------------------------------------------------
vtkSlicerLineInterpolator* vtkSlicerAbstractRepresentation::GetLineInterpolator()
{
  return this->LineInterpolator;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation::SetLineInterpolator(vtkSlicerLineInterpolator* interpolator)
{
  if (this->LineInterpolator == interpolator)
  {
    return;
  }
  this->LineInterpolator = interpolator;
  this->Modified();
}
