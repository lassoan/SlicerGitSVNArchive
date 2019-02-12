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
#include "vtkSlicerAbstractRepresentation2D.h"
#include "vtkCleanPolyData.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
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
#include "vtkSlicerLineInterpolator.h"
#include "vtkBezierSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkBox.h"
#include "vtkIntArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointPlacer.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWindow.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkCoordinate.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkLabelHierarchy.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"

vtkSlicerAbstractRepresentation2D::ControlPointsPipeline2D::ControlPointsPipeline2D()
{
  this->Glypher = vtkSmartPointer<vtkGlyph2D>::New();
  this->Glypher->SetInputData(this->ControlPointsPolyData);
  this->Glypher->SetScaleFactor(1.0);

  // By default the Points are rendered as spheres
  vtkNew<vtkSphereSource> ss;
  ss->SetRadius(0.5);
  ss->Update();
  this->PointMarkerShape = ss->GetOutput();
  this->Glypher->SetSourceData(this->PointMarkerShape);

  this->Property = vtkSmartPointer<vtkProperty2D>::New();
  this->Property->SetColor(0.4, 1.0, 1.0);
  this->Property->SetPointSize(10.);
  this->Property->SetLineWidth(2.);
  this->Property->SetOpacity(1.);

  this->Mapper = vtkSmartPointer<vtkOpenGLPolyDataMapper2D>::New();
  this->Mapper->SetInputConnection(this->Glypher->GetOutputPort());
  this->Mapper->ScalarVisibilityOff();
  vtkNew<vtkCoordinate> coordinate;
  //coordinate->SetViewport(this->Renderer); TODO: check this
  // World coordinates of the node can not be used to build the actors of the representation
  // Because the world coorinate in the node are the 3D ones.
  coordinate->SetCoordinateSystemToDisplay();
  this->Mapper->SetTransformCoordinate(coordinate);

  this->Actor = vtkSmartPointer<vtkActor2D>::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  // Labels
  this->LabelsMapper = vtkSmartPointer<vtkLabelPlacementMapper>::New();
  this->LabelsMapper->SetInputConnection(this->PointSetToLabelHierarchyFilter->GetOutputPort());
  // Here it will be the best to use Display coorinate system
  // in that way we would not need the addtional copy of the polydata in LabelFocalData (in Viewport coordinates)
  // However the LabelsMapper seems not working with the Display coordiantes.
  this->LabelsMapper->GetAnchorTransform()->SetCoordinateSystemToNormalizedViewport();
  this->LabelsActor = vtkActor2D::New();
  this->LabelsActor->SetMapper(this->LabelsMapper);
}


//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation2D::vtkSlicerAbstractRepresentation2D()
{
  for (int i = 0; i<NumberOfControlPointTypes; i++)
  {
    this->ControlPoints[i] = new ControlPointsPipeline2D;
  }

  this->ControlPoints[Selected]->TextProperty->SetColor(1.0, 0.5, 0.5);
  reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[Selected])->Property->SetColor(1.0, 0.5, 0.5);

  this->ControlPoints[Active]->TextProperty->SetColor(0.4, 1.0, 0.); // bright green
  reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[Selected])->Property->SetColor(0.4, 1.0, 0.);


  this->PointsVisibilityOnSlice = vtkSmartPointer<vtkIntArray>::New();
  this->PointsVisibilityOnSlice->SetName("pointsVisibilityOnSlice");
  this->PointsVisibilityOnSlice->Allocate(100);
  this->PointsVisibilityOnSlice->SetNumberOfValues(1);
  this->PointsVisibilityOnSlice->SetValue(0,0);

  this->SliceNode = nullptr;


  this->HandleSize = 3;
}

//----------------------------------------------------------------------
vtkSlicerAbstractRepresentation2D::~vtkSlicerAbstractRepresentation2D()
{
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::GetSliceToWorldCoordinates(const double slicePos[2],
                                                                   double worldPos[3])
{
  if (this->Renderer == nullptr ||
      this->SliceNode == nullptr)
    {
    return;
    }

  double rasw[4], xyzw[4];
  xyzw[0] = slicePos[0] - this->Renderer->GetOrigin()[0];
  xyzw[1] = slicePos[1] - this->Renderer->GetOrigin()[1];
  xyzw[2] = 0.;
  xyzw[3] = 1.0;

  this->SliceNode->GetXYToRAS()->MultiplyPoint(xyzw, rasw);

  worldPos[0] = rasw[0]/rasw[3];
  worldPos[1] = rasw[1]/rasw[3];
  worldPos[2] = rasw[2]/rasw[3];
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::GetWorldToSliceCoordinates(const double worldPos[3], double slicePos[2])
{
  if (this->Renderer == nullptr ||
      this->SliceNode == nullptr)
    {
    return;
    }

  double sliceCoordinates[4], worldCoordinates[4];
  worldCoordinates[0] = worldPos[0];
  worldCoordinates[1] = worldPos[1];
  worldCoordinates[2] = worldPos[2];
  worldCoordinates[3] = 1.0;

  vtkNew<vtkMatrix4x4> rasToxyMatrix;
  this->SliceNode->GetXYToRAS()->Invert(this->SliceNode->GetXYToRAS(), rasToxyMatrix.GetPointer());

  rasToxyMatrix->MultiplyPoint(worldCoordinates, sliceCoordinates);

  slicePos[0] = sliceCoordinates[0];
  slicePos[1] = sliceCoordinates[1];
}


//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::BuildRepresentationPointsAndLabels(double labelsOffset)
{
  int numPoints = this->GetNumberOfNodes();

  for (int controlPointType = 0; controlPointType < NumberOfControlPointTypes; ++controlPointType)
  {
    ControlPointsPipeline2D* controlPoints = reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[controlPointType]);

    controlPoints->ControlPoints->SetNumberOfPoints(0);
    controlPoints->ControlPointsPolyData->GetPointData()->GetNormals()->SetNumberOfTuples(0);

    controlPoints->LabelControlPoints->SetNumberOfPoints(0);
    controlPoints->LabelControlPointsPolyData->GetPointData()->GetNormals()->SetNumberOfTuples(0);

    controlPoints->Labels->SetNumberOfValues(0);
    controlPoints->LabelsPriority->SetNumberOfValues(0);

    int startIndex = 0;
    int stopIndex = numPoints - 1;
    if (controlPointType == Active)
    {
      if (this->GetActiveNode() >= 0 && this->GetActiveNode() < numPoints &&
        this->GetNthNodeVisibility(this->GetActiveNode()))
      {
        startIndex = this->GetActiveNode();
        stopIndex = startIndex;
      }
      else
      {
        controlPoints->Actor->VisibilityOff();
        controlPoints->LabelsActor->VisibilityOff();
        continue;
      }
    }

    for (int pointIndex = startIndex; pointIndex <= stopIndex; pointIndex++)
      {

      if (controlPointType != Active
        && (!this->GetNthNodeVisibility(pointIndex) || pointIndex == this->GetActiveNode() ||
          ( (controlPointType == Selected) != (this->GetNthNodeSelected(pointIndex)!=0) ) ) )
        {
        continue;
        }

      double slicePos[2] = { 0.0 };
      double worldOrient[9] = { 0.0 };
      double orientation[4] = { 0.0 };
      this->GetNthNodeDisplayPosition(pointIndex, slicePos);
      bool skipPoint = false;

      // TODO: skipping points at the same position is probably this not a good idea.
      // For example, only one of them may have label.
      for (int jj = 0; jj < controlPoints->ControlPoints->GetNumberOfPoints(); jj++)
        {
        double* pos = controlPoints->ControlPoints->GetPoint(jj);
        double eps = 0.001;
        if (fabs(pos[0] - slicePos[0]) < eps &&
            fabs(pos[1] - slicePos[1]))
          {
          skipPoint = true;
          break;
          }
        }
      if (skipPoint)
        {
        continue;
        }

      controlPoints->ControlPoints->InsertNextPoint(slicePos);

      slicePos[0] += labelsOffset;
      slicePos[1] += labelsOffset;
      this->Renderer->DisplayToView();
      double viewPos[3];
      this->Renderer->GetViewPoint(viewPos);
      this->Renderer->ViewToNormalizedViewport(viewPos[0], viewPos[1], viewPos[2]);
      controlPoints->LabelControlPoints->InsertNextPoint(viewPos);

      this->GetNthNodeOrientation(pointIndex, orientation);
      this->FromOrientationQuaternionToWorldOrient(orientation, worldOrient);
      controlPoints->ControlPointsPolyData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);
      controlPoints->LabelControlPointsPolyData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);

      controlPoints->Labels->InsertNextValue(this->GetNthNodeLabel(pointIndex));
      controlPoints->LabelsPriority->InsertNextValue(std::to_string(pointIndex));
      }

    controlPoints->ControlPoints->Modified();
    controlPoints->ControlPointsPolyData->GetPointData()->GetNormals()->Modified();
    controlPoints->ControlPointsPolyData->Modified();

    controlPoints->LabelControlPoints->Modified();
    controlPoints->LabelControlPointsPolyData->GetPointData()->GetNormals()->Modified();
    controlPoints->LabelControlPointsPolyData->Modified();

    if (controlPointType == Active)
      {
      controlPoints->Actor->VisibilityOn();
      controlPoints->LabelsActor->VisibilityOn();
      }

    }

}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::SetSliceNode(vtkMRMLSliceNode *sliceNode)
{
  if (sliceNode == nullptr || this->SliceNode == sliceNode)
    {
    return;
    }

  this->SliceNode = sliceNode;
}

//----------------------------------------------------------------------
vtkMRMLSliceNode *vtkSlicerAbstractRepresentation2D::GetSliceNode()
{
  return this->SliceNode;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::GetNthNodeDisplayPosition(int n, double slicePos[2])
{
  if (!this->NodeExists(n) || !this->SliceNode)
    {
    return 0;
    }

  double worldPos[3];

  this->GetNthNodeWorldPosition(n, worldPos);
  this->GetWorldToSliceCoordinates(worldPos, slicePos);

  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::GetIntermediatePointDisplayPosition(int n, int idx, double displayPos[2])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode || !this->NodeExists(n))
    {
    return 0;
    }

  if (idx < 0 ||
      static_cast<unsigned int>(idx) >= this->GetNthNode(n)->IntermediatePositions.size())
    {
    return 0;
    }

  vtkVector3d intermediatePosition = this->GetNthNode(n)->IntermediatePositions[static_cast<unsigned int> (idx)];
  vtkVector3d worldPos;
  markupsNode->TransformPointToWorld(intermediatePosition, worldPos);

  this->GetWorldToSliceCoordinates(worldPos.GetData(), displayPos);
  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::SetNthNodeDisplayPosition(int n, const double slicePos[2])
{
  if (!this->NodeExists(n))
    {
    return 0;
    }

  double worldPos[3];

  this->GetSliceToWorldCoordinates(slicePos, worldPos);
  this->SetNthNodeWorldPositionInternal(n, worldPos);

  return 1;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::AddNodeAtDisplayPosition(const double slicePos[2])
{
  double worldPos[3];

  this->GetSliceToWorldCoordinates(slicePos, worldPos);
  this->AddNodeAtPositionInternal(worldPos);

  return 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::SetNthPointSliceVisibility(int n, bool visibility)
{
  this->PointsVisibilityOnSlice->InsertValue(n, visibility);
  this->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::SetCentroidSliceVisibility(bool visibility)
{
  this->CentroidVisibilityOnSlice = visibility;
  this->Modified();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::BuildLocator()
{
  if (!this->RebuildLocator && !this->NeedToRender)
    {
    return;
    }

  vtkIdType size = this->GetNumberOfNodes();
  if (size < 1)
    {
    return;
    }

  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(size);

  double pos[3] = {0,0,0};
  for(int i = 0; i < size; i++)
    {
    this->GetNthNodeDisplayPosition(i, pos);
    points->InsertPoint(i,pos);
    }

  vtkPolyData *tmp = vtkPolyData::New();
  tmp->SetPoints(points);
  this->Locator->SetDataSet(tmp);
  tmp->FastDelete();
  points->FastDelete();

  //we fully updated the display locator
  this->RebuildLocator = false;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::AddNodeAtPositionInternal(const double worldPos[3])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return;
    }

  // Add a new point at this position
  vtkVector3d pos(worldPos[0], worldPos[1], worldPos[2]);
  markupsNode->DisableModifiedEventOn();
  markupsNode->AddControlPointWorld(pos);
  markupsNode->DisableModifiedEventOff();

  this->UpdateLines(this->GetNumberOfNodes() - 1);

  if (this->GetNumberOfNodes() - 1 >= this->PointsVisibilityOnSlice->GetNumberOfValues())
    {
    this->PointsVisibilityOnSlice->InsertValue(this->GetNumberOfNodes() - 1, 1);
    }
  else
    {
    this->PointsVisibilityOnSlice->InsertNextValue(1);
    }

  this->NeedToRender = 1;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::BuildRepresentation()
{
  // Make sure we are up to date with any changes made in the placer
  this->UpdateWidget();

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

  for (int controlPointType = 0; controlPointType < NumberOfControlPointTypes; ++controlPointType)
  {
    ControlPointsPipeline2D* controlPoints = reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[controlPointType]);
    controlPoints->LabelsActor->SetVisibility(this->MarkupsDisplayNode->GetTextVisibility());
    controlPoints->Glypher->SetScaleFactor(scale * this->HandleSize);
  }

  this->BuildRepresentationPointsAndLabels(scale * this->HandleSize);
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::CanInteract(const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &itemIndex)
{
  int interactResult = InteractNone;
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    return interactResult;
    }
  if (this->GetNumberOfNodes() < 1)
    {
    return interactResult;
    }
  if (markupsNode->GetLocked())
    {
    return interactResult;
    }

  double displayPosition3[3] = { static_cast<double>(displayPosition[0]), static_cast<double>(displayPosition[1]), 0.0 };

  this->PixelTolerance = this->HandleSize * (1.0 + this->Tolerance) * this->CalculateViewScaleFactor();
  double pixelTolerance2 = this->PixelTolerance * this->PixelTolerance;

  closestDistance2 = VTK_DOUBLE_MAX; // in display coordinate system
  itemIndex = -1;
  bool canProcess = false;
  if (this->GetNumberOfNodes() > 2 && this->ClosedLoop && markupsNode && this->CentroidVisibilityOnSlice)
    {
    // Check if centroid is selected
    double centroidPosWorld[3], centroidPosDisplay[3];
    markupsNode->GetCentroidPositionWorld(centroidPosWorld);
    this->GetWorldToSliceCoordinates(centroidPosWorld, centroidPosDisplay);

    double dist2 = vtkMath::Distance2BetweenPoints(centroidPosDisplay, displayPosition3);
    if ( dist2 < pixelTolerance2)
      {
      closestDistance2 = dist2;
      canProcess = true;
      interactResult = InteractCentroid;
      }
    }

  vtkIdType numberOfPoints = this->GetNumberOfNodes();
  double sliceCoordinates[4], worldCoordinates[4];

  double pointDisplayPos[4] = { 0.0, 0.0, 0.0, 1.0 };
  double pointWorldPos[4] = { 0.0, 0.0, 0.0, 1.0 };

  vtkNew<vtkMatrix4x4> rasToxyMatrix;
  this->SliceNode->GetXYToRAS()->Invert(this->SliceNode->GetXYToRAS(), rasToxyMatrix.GetPointer());
  for (int i = 0; i < numberOfPoints; i++)
    {
    if (!this->PointsVisibilityOnSlice->GetValue(i))
      {
      continue;
      }
    markupsNode->GetNthControlPointPositionWorld(i, pointWorldPos);
    rasToxyMatrix->MultiplyPoint(pointWorldPos, pointDisplayPos);
    double dist2 = vtkMath::Distance2BetweenPoints(pointDisplayPos, displayPosition3);
    if (dist2 < pixelTolerance2)
      {
      closestDistance2 = dist2;
      canProcess = true;
      interactResult = InteractControlPoint;
      itemIndex = i;
      }
    }

  return canProcess;
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::ActivateNode(int X, int Y)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (this->GetNumberOfNodes() == 0)
    {
    this->SetActiveNode(-1);
    return 0;
    }

  double displayPos[3];
  displayPos[0] = static_cast<double>(X);
  displayPos[1] = static_cast<double>(Y);
  displayPos[2] = 0.;

  this->PixelTolerance = this->HandleSize + this->HandleSize * this->Tolerance;
  double scale = this->CalculateViewScaleFactor();
  this->PixelTolerance *= scale;

  if (this->GetNumberOfNodes() > 2 && this->ClosedLoop &&
      markupsNode)
    {
    // Check if centroid is selected
    double centroidPosWorld[3], centroidPosDisplay[3];
    markupsNode->GetCentroidPositionWorld(centroidPosWorld);
    this->GetWorldToSliceCoordinates(centroidPosWorld, centroidPosDisplay);

    if (vtkMath::Distance2BetweenPoints(centroidPosDisplay, displayPos) <
        this->PixelTolerance * this->PixelTolerance)
      {
      if (this->GetActiveNode() != -3)
        {
        this->SetActiveNode(-3);
        this->NeedToRender = 1;
        }
      return 1;
      }
    }

  this->BuildLocator();

  double closestDistance2 = VTK_DOUBLE_MAX;
  int closestNode = static_cast<int> (this->Locator->FindClosestPointWithinRadius(
    this->PixelTolerance, displayPos, closestDistance2));

  if (closestNode != this->GetActiveNode())
    {
    this->SetActiveNode(closestNode);
    this->NeedToRender = 1;
    }
  return (this->GetActiveNode() >= 0);
}


//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::GetActors(vtkPropCollection *pc)
{
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline2D* controlPoints = reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[i]);
    controlPoints->Actor->GetActors(pc);
    controlPoints->LabelsActor->GetActors(pc);
  }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline2D* controlPoints = reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[i]);
    controlPoints->Actor->ReleaseGraphicsResources(win);
    controlPoints->LabelsActor->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  int count = 0;
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline2D* controlPoints = reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[i]);
    if (controlPoints->Actor->GetVisibility())
    {
      count += controlPoints->Actor->RenderOverlay(viewport);
    }
    if (controlPoints->LabelsActor->GetVisibility())
    {
      count += controlPoints->LabelsActor->RenderOverlay(viewport);
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();

  int count = 0;
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline2D* controlPoints = reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[i]);
    if (controlPoints->Actor->GetVisibility())
    {
      count += controlPoints->Actor->RenderOpaqueGeometry(viewport);
    }
    if (controlPoints->LabelsActor->GetVisibility())
    {
      count += controlPoints->LabelsActor->RenderOpaqueGeometry(viewport);
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerAbstractRepresentation2D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count = 0;
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline2D* controlPoints = reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[i]);
    if (controlPoints->Actor->GetVisibility())
    {
      count += controlPoints->Actor->RenderTranslucentPolygonalGeometry(viewport);
    }
    if (controlPoints->LabelsActor->GetVisibility())
    {
      count += controlPoints->LabelsActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerAbstractRepresentation2D::HasTranslucentPolygonalGeometry()
{
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline2D* controlPoints = reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[i]);
    if (controlPoints->Actor->GetVisibility())
    {
      if (controlPoints->Actor->HasTranslucentPolygonalGeometry())
      {
        return true;
      }
    }
    if (controlPoints->LabelsActor->GetVisibility())
    {
      if (controlPoints->LabelsActor->HasTranslucentPolygonalGeometry())
      {
        return true;
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractRepresentation2D::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline2D* controlPoints = reinterpret_cast<ControlPointsPipeline2D*>(this->ControlPoints[i]);
    os << indent << "Pipeline " << i << "\n";
    if (controlPoints->Actor)
    {
      os << indent << "Points Visibility: " << controlPoints->Actor->GetVisibility() << "\n";
    }
    else
    {
      os << indent << "Points: (none)\n";
    }
    if (controlPoints->LabelsActor)
    {
      os << indent << "Labels Visibility: " << controlPoints->LabelsActor->GetVisibility() << "\n";
    }
    else
    {
      os << indent << "Labels Points: (none)\n";
    }
    if (controlPoints->Property)
    {
      os << indent << "Property: " << controlPoints->Property << "\n";
    }
    else
    {
      os << indent << "Property: (none)\n";
    }
  }
}
