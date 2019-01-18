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
#include "vtkBezierSlicerLineInterpolator.h"
#include "vtkSphereSource.h"
#include "vtkPropPicker.h"
#include "vtkPickingManager.h"
#include "vtkAppendPolyData.h"

vtkStandardNewMacro(vtkSlicerLineRepresentation2D);

//----------------------------------------------------------------------
vtkSlicerLineRepresentation2D::vtkSlicerLineRepresentation2D()
{
  this->Lines = vtkPolyData::New();
  this->LinesMapper = vtkOpenGLPolyDataMapper2D::New();
  this->LinesMapper->SetInputData(this->Lines);

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->LinesActor = vtkActor2D::New();
  this->LinesActor->SetMapper(this->LinesMapper);
  this->LinesActor->SetProperty(this->LinesProperty);

  this->CursorPicker->AddPickList(this->LinesActor);

  this->appendActors = vtkAppendPolyData::New();
  this->appendActors->AddInputData(this->CursorShape);
  this->appendActors->AddInputData(this->SelectedCursorShape);
  this->appendActors->AddInputData(this->ActiveCursorShape);
  this->appendActors->AddInputData(this->Lines);
}

//----------------------------------------------------------------------
vtkSlicerLineRepresentation2D::~vtkSlicerLineRepresentation2D()
{
  this->Lines->Delete();
  this->LinesMapper->Delete();
  this->LinesActor->Delete();
  this->LinesProperty->Delete();
  this->ActiveLinesProperty->Delete();
  this->appendActors->Delete();
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::CreateDefaultProperties()
{
  Superclass::CreateDefaultProperties();

  this->LinesProperty = vtkProperty2D::New();
  this->LinesProperty->SetColor( 0.4, 1.0, 1.0 );
  this->LinesProperty->SetLineWidth( 2. );
  this->LinesProperty->SetOpacity( 1. );

  this->ActiveLinesProperty = vtkProperty2D::New();
  this->ActiveLinesProperty->SetColor( 0.4, 1.0, 0. );
  this->ActiveLinesProperty->SetLineWidth( 2. );
  this->ActiveLinesProperty->SetOpacity( 1. );
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::Highlight(int highlight)
{
  if ( !this->MarkupsNode )
  {
    return;
  }

  if ( highlight && this->MarkupsNode->GetActiveControlPoint() == -1 &&
       this->InteractionState == vtkSlicerAbstractRepresentation::Nearby )
  {
    this->LinesActor->SetProperty(this->ActiveLinesProperty);
  }
  else
  {
    this->LinesActor->SetProperty(this->LinesProperty);
  }

  this->NeedToRenderOn();
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::BuildLines()
{
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();

  int i, j;
  vtkIdType index = 0;

  int numberOfNodes = this->GetNumberOfNodes();
  int count = numberOfNodes;
  for (i = 0; i < numberOfNodes; i++)
  {
    count += this->GetNumberOfIntermediatePoints(i);
  }

  points->SetNumberOfPoints(count);
  vtkIdType numLines = count;
  if (numLines > 0)
  {
    vtkIdType *lineIndices = new vtkIdType[numLines];

    double pos[3];
    for (i = 0; i < numberOfNodes; i++)
    {
      // Add the node
      this->GetNthNodeWorldPosition(i, pos);
      points->InsertPoint(index, pos);
      lineIndices[index] = index;
      index++;

      int numIntermediatePoints = this->GetNumberOfIntermediatePoints(i);

      for (j = 0; j < numIntermediatePoints; j++)
      {
        this->GetIntermediatePointWorldPosition(i, j, pos);
        points->InsertPoint(index, pos);
        lineIndices[index] = index;
        index++;
      }
    }

    lines->InsertNextCell(numLines, lineIndices);
    delete [] lineIndices;
  }

  this->Lines->SetPoints(points);
  this->Lines->SetLines(lines);

  points->Delete();
  lines->Delete();
}

//----------------------------------------------------------------------
vtkPolyData *vtkSlicerLineRepresentation2D::GetWidgetRepresentationAsPolyData()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput();
}


//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::GetActors(vtkPropCollection *pc)
{
  Superclass::GetActors(pc);
  this->LinesActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  Superclass::ReleaseGraphicsResources(win);
  this->LinesActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  count = Superclass::RenderOverlay(viewport);
  if (this->LinesActor->GetVisibility())
  {
    count +=  this->LinesActor->RenderOverlay(viewport);
  }
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
  count = Superclass::RenderOpaqueGeometry(viewport);
  if (this->LinesActor->GetVisibility())
  {
    count += this->LinesActor->RenderOpaqueGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkSlicerLineRepresentation2D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  count = Superclass::RenderTranslucentPolygonalGeometry(viewport);
  if (this->LinesActor->GetVisibility())
  {
    count += this->LinesActor->RenderTranslucentPolygonalGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkSlicerLineRepresentation2D::HasTranslucentPolygonalGeometry()
{
  int result=0;
  result |= Superclass::HasTranslucentPolygonalGeometry();
  if (this->LinesActor->GetVisibility())
  {
    result |= this->LinesActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}

//----------------------------------------------------------------------
double *vtkSlicerLineRepresentation2D::GetBounds()
{
  this->appendActors->Update();
  return this->appendActors->GetOutput()->GetPoints() ?
              this->appendActors->GetOutput()->GetBounds() : nullptr;
}

//-----------------------------------------------------------------------------
void vtkSlicerLineRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

   os << indent << "Line Visibility: " << this->LinesActor->GetVisibility() << "\n";

  if (this->LinesProperty)
  {
    os << indent << "Lines Property: " << this->LinesProperty << "\n";
  }
  else
  {
    os << indent << "Lines Property: (none)\n";
  }

  if (this->ActiveLinesProperty)
  {
    os << indent << "Active Lines Property: " << this->ActiveLinesProperty << "\n";
  }
  else
  {
    os << indent << "Active Lines Property: (none)\n";
  }

}
