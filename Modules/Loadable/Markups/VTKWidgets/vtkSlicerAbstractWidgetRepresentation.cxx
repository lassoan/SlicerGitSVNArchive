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
#include "vtkCleanPolyData.h"
#include "vtkCommand.h"
#include "vtkGeneralTransform.h"
#include "vtkMarkupsGlyphSource2D.h"
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

vtkSlicerAbstractWidgetRepresentation::ControlPointsPipeline::ControlPointsPipeline()
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

  this->GlyphSource2D = vtkSmartPointer<vtkMarkupsGlyphSource2D>::New();

  this->GlyphSourceSphere = vtkSmartPointer<vtkSphereSource>::New();
  this->GlyphSourceSphere->SetRadius(0.5);

};

//----------------------------------------------------------------------
vtkSlicerAbstractWidgetRepresentation::vtkSlicerAbstractWidgetRepresentation()
{
  this->ControlPointSize = 3.0;
  this->Tolerance                = 0.4;
  this->PixelTolerance           = 1;
  this->Locator                  = nullptr;
  this->NeedToRender             = false;
  this->ClosedLoop               = 0;

  this->Locator = vtkSmartPointer<vtkIncrementalOctreePointLocator>::New();
  this->Locator->SetBuildCubicOctree(1);
  this->RebuildLocator = true;

  this->PointPlacer = vtkSmartPointer<vtkFocalPlanePointPlacer>::New();

  for (int i = 0; i<NumberOfControlPointTypes; i++)
  {
    this->ControlPoints[i] = nullptr;
  }

  this->AlwaysOnTop = 0;

}

//----------------------------------------------------------------------
vtkSlicerAbstractWidgetRepresentation::~vtkSlicerAbstractWidgetRepresentation()
{
  for (int i=0; i<NumberOfControlPointTypes; i++)
  {
    delete this->ControlPoints[i];
    this->ControlPoints[i] = nullptr;
  }
  // Force deleting variables to prevent circular dependency keeping objects alive
  this->Locator = NULL;
  this->PointPlacer = NULL;
}

//----------------------------------------------------------------------
double vtkSlicerAbstractWidgetRepresentation::CalculateViewScaleFactor()
{
  if (!this->Renderer || !this->Renderer->GetActiveCamera())
  {
    return 1.0;
  }

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
// The display position for a given world position must be re-computed
// from the world positions... It should not be queried from the renderer
// whose camera position may have changed
int vtkSlicerAbstractWidgetRepresentation::GetNthNodeDisplayPosition(int n, double displayPos[2])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode || n < 0 || n >= markupsNode->GetNumberOfControlPoints())
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
vtkMRMLMarkupsNode::ControlPoint* vtkSlicerAbstractWidgetRepresentation::GetNthNode(int n)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode || n < 0 || n >= markupsNode->GetNumberOfControlPoints())
  {
    return nullptr;
  }
  return markupsNode->GetNthControlPoint(n);
}

/*
//----------------------------------------------------------------------
int vtkSlicerAbstractWidgetRepresentation::SetNthNodeWorldPosition(int n, const double worldPos[3])
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
*/

//----------------------------------------------------------------------
int vtkSlicerAbstractWidgetRepresentation::FindClosestPointOnWidget(
  const int displayPos[2], double closestWorldPos[3], int *idx)
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return 0;
  }

  // Make a line out of this viewing ray
  double p1[4] = { 0.0, 0.0, 0.0, 1.0 };
  double tmp1[4] = { static_cast<double>(displayPos[0]), static_cast<double>(displayPos[1]), 0.0, 1.0 };
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(p1);

  double p2[4] = { 0.0, 0.0, 0.0, 1.0 };
  tmp1[2] = 1.0;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(p2);

  double closestDistance2 = VTK_DOUBLE_MAX;
  int closestNode = 0;

  // compute a world tolerance based on pixel
  // tolerance on the focal plane
  double fp[4] = { 0.0, 0.0, 0.0, 1.0 };
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  this->Renderer->SetWorldPoint(fp);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(tmp1);

  tmp1[0] = 0;
  tmp1[1] = 0;
  double tmp2[4] = { 0.0, 0.0, 0.0, 1.0 };
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(tmp2);

  tmp1[0] = this->PixelTolerance;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(tmp1);

  double wt2 = vtkMath::Distance2BetweenPoints(tmp1, tmp2);


  // Now loop through all lines and look for closest one within tolerance
  double p3[4] = {0.0, 0.0, 0.0, 1.0};
  double p4[4] = {0.0, 0.0, 0.0, 1.0};
  vtkPoints* curvePointsWorld = this->GetMarkupsNode()->GetCurvePointsWorld();
  vtkIdType numberOfPoints = curvePointsWorld->GetNumberOfPoints();
  for(vtkIdType i = 0; i < numberOfPoints; i++)
    {
    curvePointsWorld->GetPoint(i, p3);
    if (i + 1 < numberOfPoints)
      {
      curvePointsWorld->GetPoint(i + 1, p4);
      }
    else
      {
      if (!this->ClosedLoop)
        {
        continue;
        }
      curvePointsWorld->GetPoint(0, p4);
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

  if (closestDistance2 < VTK_DOUBLE_MAX)
    {
    if (closestNode < markupsNode->GetNumberOfControlPoints() -1)
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
void vtkSlicerAbstractWidgetRepresentation
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

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::BuildLocator()
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

  int size = markupsNode->GetNumberOfControlPoints();
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
void vtkSlicerAbstractWidgetRepresentation::UpdateCenter()
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode || markupsNode->GetNumberOfControlPoints() < 1)
    {
    return;
    }
  double centerWorldPos[3] = { 0.0 };

  for (int i = 0; i < markupsNode->GetNumberOfControlPoints(); i++)
  {
    double p[4];
    markupsNode->GetNthControlPointPositionWorld(i, p);
    centerWorldPos[0] += p[0];
    centerWorldPos[1] += p[1];
    centerWorldPos[2] += p[2];
  }
  double inv_N = 1. / static_cast< double >(markupsNode->GetNumberOfControlPoints());
  centerWorldPos[0] *= inv_N;
  centerWorldPos[1] *= inv_N;
  centerWorldPos[2] *= inv_N;

  markupsNode->SetCenterPositionFromPointer(centerWorldPos);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::SetMarkupsDisplayNode(vtkMRMLMarkupsDisplayNode *markupsDisplayNode)
{
  if (this->MarkupsDisplayNode == markupsDisplayNode)
  {
    return;
  }

  this->MarkupsDisplayNode = markupsDisplayNode;

  vtkMRMLMarkupsNode* markupsNode = nullptr;
  if (this->MarkupsDisplayNode)
    {
    markupsNode = vtkMRMLMarkupsNode::SafeDownCast(this->MarkupsDisplayNode->GetDisplayableNode());
    }
  this->SetMarkupsNode(markupsNode);
}

//----------------------------------------------------------------------
vtkMRMLMarkupsDisplayNode *vtkSlicerAbstractWidgetRepresentation::GetMarkupsDisplayNode()
{
  return this->MarkupsDisplayNode;
}

//----------------------------------------------------------------------
vtkMRMLMarkupsNode *vtkSlicerAbstractWidgetRepresentation::GetMarkupsNode()
{
  if (!this->MarkupsDisplayNode)
  {
    return nullptr;
  }
  return vtkMRMLMarkupsNode::SafeDownCast(this->MarkupsDisplayNode->GetDisplayableNode());
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::SetMarkupsNode(vtkMRMLMarkupsNode *markupsNode)
{
  this->MarkupsNode = markupsNode;
}


//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::SetRenderer(vtkRenderer *ren)
{
  if ( ren == this->Renderer )
    {
    return;
    }
  this->Renderer = ren;
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkRenderer* vtkSlicerAbstractWidgetRepresentation::GetRenderer()
{
  return this->Renderer;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::SetViewNode(vtkMRMLAbstractViewNode* viewNode)
{
  if (viewNode == this->ViewNode)
  {
    return;
  }
  this->ViewNode = viewNode;
  this->Modified();
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractViewNode* vtkSlicerAbstractWidgetRepresentation::GetViewNode()
{
  return this->ViewNode;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance <<"\n";
  os << indent << "Rebuild Locator: " <<
     (this->RebuildLocator ? "On" : "Off") << endl;

  os << indent << "Current Operation: ";

  os << indent << "Point Placer: " << this->PointPlacer << "\n";

  os << indent << "Always On Top: "
     << (this->AlwaysOnTop ? "On\n" : "Off\n");
}


//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::AddActorsBounds(vtkBoundingBox& boundingBox,
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
int vtkSlicerAbstractWidgetRepresentation::CanInteract(const int displayPosition[2], const double position[3], double &closestDistance2, int &itemIndex)
{
  return vtkMRMLMarkupsDisplayNode::ComponentNone;
}

//-----------------------------------------------------------------------------
vtkPointPlacer* vtkSlicerAbstractWidgetRepresentation::GetPointPlacer()
{
  return this->PointPlacer;
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::SetPointPlacer(vtkPointPlacer* placer)
{
  if (this->PointPlacer == placer)
  {
    return;
  }
  this->PointPlacer = placer;
  this->Modified();
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractWidgetRepresentation::GetTransformationReferencePoint(double referencePointWorld[3])
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return false;
  }
  this->UpdateCenter();
  markupsNode->GetCenterPosition(referencePointWorld);
  return true;
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::BuildLine(vtkPolyData* linePolyData, bool displayPosition)
{
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> line;

  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    linePolyData->SetPoints(points);
    linePolyData->SetLines(line);
    return;
    }
  int numberOfControlPoints = markupsNode->GetNumberOfControlPoints();
  vtkIdType numberOfLines = numberOfControlPoints - 1;
  bool loop = (markupsNode->GetCurveClosed() && numberOfControlPoints > 2);
  if (loop)
    {
    numberOfLines++;
    }
  if (numberOfLines <= 0)
    {
    return;
    }

  double pos[3] = { 0.0 };
  vtkIdType index = 0;
  line->InsertNextCell(numberOfLines+1);

  for (int i = 0; i < numberOfControlPoints; i++)
    {
    // Add the node
    if (displayPosition)
      {
      this->GetNthNodeDisplayPosition(i, pos);
      }
    else
      {
      markupsNode->GetNthControlPointPositionWorld(i, pos);
      }
    points->InsertNextPoint(pos);
    line->InsertCellPoint(i);
    index++;
    }

  if (loop)
    {
    if (displayPosition)
      {
      this->GetNthNodeDisplayPosition(0, pos);
      }
    else
      {
      markupsNode->GetNthControlPointPositionWorld(0, pos);
      }
    points->InsertPoint(index, pos);
    line->InsertCellPoint(0);
    }

  linePolyData->SetPoints(points);
  linePolyData->SetLines(line);
}

    /*

    for (vtkIdType j = 0; j < numIntermediatePoints; j++)
    {
    this->GetIntermediatePointWorldPosition(i, j, pos);

    if (displayPosition)
    {
    if (3D view)
    {
    this->Renderer->SetWorldPoint(pos);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(pos);
    }
    else
    {
    this->GetWorldToSliceCoordinates(worldPos.GetData(), displayPos);
    }
    }

    points->InsertPoint(index, pos);
    lineIndices[index] = index;
    index++;
    }


    */

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::ShallowCopy(vtkProp *prop)
{
  vtkSlicerAbstractWidgetRepresentation *rep = vtkSlicerAbstractWidgetRepresentation::SafeDownCast(prop);
  if (rep)
  {
    // TODO: copy class members
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::UpdateFromMRML(vtkMRMLNode* caller, unsigned long event, void *callData)
{
  if (!event || event == vtkMRMLTransformableNode::TransformModifiedEvent)
  {
    this->MarkupsTransformModifiedTime.Modified();
  }

  if (!event || event == vtkMRMLDisplayableNode::DisplayModifiedEvent)
    {
    // Update MRML data node from display node
    vtkMRMLMarkupsNode* markupsNode = nullptr;
    if (this->MarkupsDisplayNode)
      {
      markupsNode = vtkMRMLMarkupsNode::SafeDownCast(this->MarkupsDisplayNode->GetDisplayableNode());
      }
    this->SetMarkupsNode(markupsNode);
    }

  this->NeedToRenderOn(); // TODO: call this only if actually needed to improve performance
}

//-----------------------------------------------------------------------------
void vtkSlicerAbstractWidgetRepresentation::UpdateRelativeCoincidentTopologyOffsets(vtkMapper* mapper)
{
  if (this->AlwaysOnTop)
  {
    // max value 65536 so we subtract 66000 to make sure we are
    // zero or negative
    mapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
  }
  else
  {
    mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
    mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
  }
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractWidgetRepresentation::GetAllControlPointsVisible()
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
    {
    return false;
    }

  for (int controlPointIndex = 0; controlPointIndex < markupsNode->GetNumberOfControlPoints(); controlPointIndex++)
  {
    if (!markupsNode->GetNthControlPointVisibility(controlPointIndex))
    {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------
bool vtkSlicerAbstractWidgetRepresentation::GetAllControlPointsSelected()
{
  vtkMRMLMarkupsNode* markupsNode = this->GetMarkupsNode();
  if (!markupsNode)
  {
    return false;
  }

  for (int controlPointIndex = 0; controlPointIndex < markupsNode->GetNumberOfControlPoints(); controlPointIndex++)
  {
    if (!markupsNode->GetNthControlPointSelected(controlPointIndex))
    {
      return false;
    }
  }
  return true;
}