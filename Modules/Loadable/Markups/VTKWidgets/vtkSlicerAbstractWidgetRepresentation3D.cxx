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

#include "vtkSlicerAbstractWidgetRepresentation.h"
#include "vtkSlicerAbstractWidgetRepresentation3D.h"
#include "vtkCleanPolyData.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLActor.h"
#include "vtkAssemblyPath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
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
#include "vtkSlicerLineInterpolator.h"
#include "vtkSlicerBezierLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkBox.h"
#include "vtkIntArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointPlacer.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkWindow.h"
#include "vtkProperty.h"
#include "vtkTextProperty.h"
#include "vtkActor2D.h"
#include "vtkLabelPlacementMapper.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkStringArray.h"
#include "vtkLabelHierarchy.h"
#include "vtkMRMLMarkupsDisplayNode.h"

vtkSlicerAbstractWidgetRepresentation3D::ControlPointsPipeline3D::ControlPointsPipeline3D()
{
  this->Glypher = vtkSmartPointer<vtkGlyph3D>::New();
  this->Glypher->SetInputData(this->ControlPointsPolyData);
  this->Glypher->SetVectorModeToUseNormal();
  this->Glypher->OrientOn();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(1.0);

  // By default the Points are rendered as spheres
  this->Glypher->SetSourceConnection(this->GlyphSourceSphere->GetOutputPort());

  this->Property = vtkSmartPointer<vtkProperty>::New();
  this->Property->SetRepresentationToSurface();
  this->Property->SetColor(0.4, 1.0, 1.0);
  this->Property->SetAmbient(0.0);
  this->Property->SetDiffuse(1.0);
  this->Property->SetSpecular(0.0);
  this->Property->SetShading(true);
  this->Property->SetSpecularPower(1.0);
  this->Property->SetPointSize(10.);
  this->Property->SetLineWidth(2.);
  this->Property->SetOpacity(1.);

  this->Mapper = vtkSmartPointer<vtkOpenGLPolyDataMapper>::New();
  this->Mapper->SetInputConnection(this->Glypher->GetOutputPort());
  // This turns on resolve coincident topology for everything
  // as it is a class static on the mapper
  this->Mapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->Mapper->ScalarVisibilityOff();
  // Put this on top of other objects
  this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
  this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
  this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->Actor = vtkSmartPointer<vtkOpenGLActor>::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  // Labels
  this->LabelsMapper = vtkSmartPointer<vtkLabelPlacementMapper>::New();
  this->LabelsMapper->SetInputConnection(this->PointSetToLabelHierarchyFilter->GetOutputPort());
  this->LabelsActor = vtkSmartPointer<vtkActor2D>::New();
  this->LabelsActor->SetMapper(this->LabelsMapper);
};

//----------------------------------------------------------------------
vtkSlicerAbstractWidgetRepresentation3D::vtkSlicerAbstractWidgetRepresentation3D()
{
  for (int i = 0; i<NumberOfControlPointTypes; i++)
  {
    this->ControlPoints[i] = new ControlPointsPipeline3D;
  }

  this->ControlPoints[Selected]->TextProperty->SetColor(1.0, 0.5, 0.5);
  reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[Selected])->Property->SetColor(1.0, 0.5, 0.5);

  this->ControlPoints[Active]->TextProperty->SetColor(0.4, 1.0, 0.); // bright green
  reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[Active])->Property->SetColor(0.4, 1.0, 0.);

  this->ControlPointSize = 3;
}

//----------------------------------------------------------------------
vtkSlicerAbstractWidgetRepresentation3D::~vtkSlicerAbstractWidgetRepresentation3D()
{
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation3D::BuildRepresentationPointsAndLabels()
{
  int numPoints = this->GetNumberOfNodes();

  for (int controlPointType = 0; controlPointType < NumberOfControlPointTypes; ++controlPointType)
  {
    ControlPointsPipeline3D* controlPoints = reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[controlPointType]);

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
        ((controlPointType == Selected) != (this->GetNthNodeSelected(pointIndex) != 0))))
        {
        continue;
        }

      double worldPos[3], worldOrient[9] = {0}, orientation[4] = {0};
      this->GetNthNodeWorldPosition(pointIndex, worldPos);

      // TODO: skipping points at the same position is probably this not a good idea.
      // For example, only one of them may have label.
      bool skipPoint = false;
      for (int jj = 0; jj < controlPoints->ControlPoints->GetNumberOfPoints(); jj++)
        {
        double* pos = controlPoints->ControlPoints->GetPoint(jj);
        double eps = 0.001;
        if (fabs(pos[0] - worldPos[0]) < eps &&
            fabs(pos[1] - worldPos[1]) < eps &&
            fabs(pos[2] - worldPos[2]) < eps)
          {
          skipPoint = true;
          break;
          }
        }
      if (skipPoint)
        {
        continue;
        }

      controlPoints->ControlPoints->InsertNextPoint(worldPos);

      worldPos[0] += this->ControlPointSize;
      worldPos[1] += this->ControlPointSize;
      worldPos[2] += this->ControlPointSize;
      controlPoints->LabelControlPoints->InsertNextPoint(worldPos);

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
int vtkSlicerAbstractWidgetRepresentation3D::CanInteract(const int displayPosition[2],
  const double worldPosition[3], double &closestDistance2, int &componentIndex)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode || markupsNode->GetLocked() || this->GetNumberOfNodes() < 1)
  {
    return vtkMRMLMarkupsDisplayNode::ComponentNone;
  }

  int foundComponentType = vtkMRMLMarkupsDisplayNode::ComponentNone;

  double displayPosition3[3] = { static_cast<double>(displayPosition[0]), static_cast<double>(displayPosition[1]), 0.0 };


  this->PixelTolerance = this->ControlPointSize + this->ControlPointSize * this->Tolerance;
  double scale = this->CalculateViewScaleFactor();
  this->PixelTolerance *= scale;
  double pixelTolerance2 = this->PixelTolerance * this->PixelTolerance;

  closestDistance2 = VTK_DOUBLE_MAX; // in display coordinate system
  componentIndex = -1;
  if (this->GetNumberOfNodes() > 2 && this->ClosedLoop && markupsNode)
  {
    // Check if centroid is selected
    double centroidPosWorld[3], centroidPosDisplay[3];
    markupsNode->GetCentroidPosition(centroidPosWorld);
    this->Renderer->SetWorldPoint(centroidPosWorld);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(centroidPosDisplay);
    double dist2 = vtkMath::Distance2BetweenPoints(centroidPosDisplay, displayPosition3);
    if (dist2 < pixelTolerance2)
    {
      closestDistance2 = dist2;
      foundComponentType = vtkMRMLMarkupsDisplayNode::ComponentCentroid;
      componentIndex = 0;
    }
  }

  vtkIdType numberOfPoints = this->GetNumberOfNodes();
  double displayCoordinates[4], worldCoordinates[4];

  double pointDisplayPos[4] = { 0.0, 0.0, 0.0, 1.0 };
  double pointWorldPos[4] = { 0.0, 0.0, 0.0, 1.0 };

  for (int i = 0; i < numberOfPoints; i++)
  {
    if (!markupsNode->GetNthControlPointVisibility(i))
    {
      continue;
    }
    double centroidPosWorld[3], centroidPosDisplay[3];
    markupsNode->GetNthControlPointPositionWorld(i, centroidPosWorld);
    this->Renderer->SetWorldPoint(centroidPosWorld);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(centroidPosDisplay);
    double dist2 = vtkMath::Distance2BetweenPoints(centroidPosDisplay, displayPosition3);
    if (dist2 < pixelTolerance2)
    {
      closestDistance2 = dist2;
      foundComponentType = vtkMRMLMarkupsDisplayNode::ComponentControlPoint;
      componentIndex = i;
    }
  }

  /* This would probably faster for many points:

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
  */

  return foundComponentType;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation3D::CanInteractWithLine(int &foundComponentType,
  const int displayPosition[2], const double worldPosition[3], double &closestDistance2, int &componentIndex)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode || markupsNode->GetLocked() || this->GetNumberOfNodes() < 1)
  {
    return;
  }

  vtkIdType numberOfPoints = this->GetNumberOfNodes();

  double pointWorldPos1[4] = { 0.0, 0.0, 0.0, 1.0 };
  double pointWorldPos2[4] = { 0.0, 0.0, 0.0, 1.0 };

  double toleranceWorld = this->ControlPointSize * this->ControlPointSize;

  //this->PixelTolerance = this->ControlPointSize + this->ControlPointSize * this->Tolerance;
  //double scale = this->CalculateViewScaleFactor();
  //this->PixelTolerance *= scale;

  for (int i = 0; i < numberOfPoints - 1; i++)
  {
    markupsNode->GetNthControlPointPositionWorld(i, pointWorldPos1);
    markupsNode->GetNthControlPointPositionWorld(i + 1, pointWorldPos2);

    double relativePositionAlongLine = -1.0; // between 0.0-1.0 if between the endpoints of the line segment
    double distance2 = vtkLine::DistanceToLine(worldPosition, pointWorldPos1, pointWorldPos2, relativePositionAlongLine);
    if (distance2 < toleranceWorld && distance2 < closestDistance2 && relativePositionAlongLine >= 0 && relativePositionAlongLine <= 1)
    {
      closestDistance2 = distance2;
      foundComponentType = vtkMRMLMarkupsDisplayNode::ComponentLine;
      componentIndex = i;
    }
  }
}


//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation3D::UpdateFromMRML()
{
  // Make sure we are up to date with any changes made in the placer
  this->UpdateWidget();

  vtkMRMLMarkupsDisplayNode* display = this->MarkupsDisplayNode;
  if (display == nullptr)
    {
    return;
    }

  for (int i = 0; i<NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline3D* controlPoints = reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[i]);
    controlPoints->LabelsActor->SetVisibility(display->GetTextVisibility());

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

    controlPoints->Glypher->SetScaleFactor(this->ControlPointSize);
    }
  this->BuildRepresentationPointsAndLabels();
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation3D::GetActors(vtkPropCollection *pc)
{
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline3D* controlPoints = reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[i]);
    controlPoints->Actor->GetActors(pc);
    controlPoints->LabelsActor->GetActors(pc);
  }
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation3D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline3D* controlPoints = reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[i]);
    controlPoints->Actor->ReleaseGraphicsResources(win);
    controlPoints->LabelsActor->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------
int vtkSlicerAbstractWidgetRepresentation3D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline3D* controlPoints = reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[i]);
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
int vtkSlicerAbstractWidgetRepresentation3D::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->UpdateFromMRML();

  int count=0;
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline3D* controlPoints = reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[i]);
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
int vtkSlicerAbstractWidgetRepresentation3D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline3D* controlPoints = reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[i]);
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
vtkTypeBool vtkSlicerAbstractWidgetRepresentation3D::HasTranslucentPolygonalGeometry()
{
  for (int i = 0; i < NumberOfControlPointTypes; i++)
  {
    ControlPointsPipeline3D* controlPoints = reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[i]);
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
double *vtkSlicerAbstractWidgetRepresentation3D::GetBounds()
{
  vtkBoundingBox boundingBox;
  const std::vector<vtkProp*> actors(
    {
    reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[Unselected])->Actor,
    reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[Selected])->Actor,
    reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[Active])->Actor
    });
  this->AddActorsBounds(boundingBox, actors, Superclass::GetBounds());
  boundingBox.GetBounds(this->Bounds);
  return this->Bounds;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation3D::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  for (int i = 0; i < NumberOfControlPointTypes; i++)
    {
    ControlPointsPipeline3D* controlPoints = reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[i]);
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

//-----------------------------------------------------------------------------
vtkSlicerAbstractWidgetRepresentation3D::ControlPointsPipeline3D* vtkSlicerAbstractWidgetRepresentation3D::GetControlPointsPipeline(int controlPointType)
{
  return reinterpret_cast<ControlPointsPipeline3D*>(this->ControlPoints[controlPointType]);
}
