/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// MRMLDisplayableManager includes
#include "vtkMRMLOrientationMarkerDisplayableManager.h"

// MRML includes
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkMatrix3x3.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>

// STD includes


class vtkRendererUpdateObserver : public vtkCommand
{
public:
  static vtkRendererUpdateObserver *New()
    {
    return new vtkRendererUpdateObserver;
    }
  vtkRendererUpdateObserver()
    {
    this->DisplayableManager = 0;
    }
  virtual void Execute(vtkObject* vtkNotUsed(wdg), unsigned long vtkNotUsed(event), void* vtkNotUsed(calldata))
    {
    if (this->DisplayableManager)
      {
      this->DisplayableManager->UpdateFromRenderer();
      }
  }
  vtkWeakPointer<vtkMRMLOrientationMarkerDisplayableManager> DisplayableManager;
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLOrientationMarkerDisplayableManager );

//---------------------------------------------------------------------------
class vtkMRMLOrientationMarkerDisplayableManager::vtkInternal
{
public:
  vtkInternal(vtkMRMLOrientationMarkerDisplayableManager * external);
  ~vtkInternal();

  void UpdateMarkerVisibility();
  void UpdateMarkerType();
  void UpdateMarkerSize();
  void UpdateMarkerOrientation();

  void CreateCubeOrientationModel();
  void CreateHumanOrientationModel(const char* modelFilename);
  void CreateAxesOrientationModel();

  void AddRendererObserver(vtkRenderer* renderer);
  void RemoveRendererObserver();

  vtkSmartPointer<vtkRenderer> MarkerRenderer;
  vtkSmartPointer<vtkAnnotatedCubeActor> CubeActor;
  vtkSmartPointer<vtkActor> HumanActor;
  vtkSmartPointer<vtkAxesActor> AxesActor;

  vtkSmartPointer<vtkRendererUpdateObserver> RendererObserver;
  int RendererObservationId;
  vtkWeakPointer<vtkRenderer> ObservedRenderer;

  vtkMRMLOrientationMarkerDisplayableManager* External;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::vtkInternal(vtkMRMLOrientationMarkerDisplayableManager * external)
{
  this->External = external;
  this->RendererObserver = vtkSmartPointer<vtkRendererUpdateObserver>::New();
  this->RendererObserver->DisplayableManager = this->External;
  this->RendererObservationId = 0;
}

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::~vtkInternal()
{
  RemoveRendererObserver();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::AddRendererObserver(vtkRenderer* renderer)
{
  RemoveRendererObserver();
  if (renderer)
    {
    this->ObservedRenderer = renderer;
    this->RendererObservationId = renderer->AddObserver(vtkCommand::StartEvent, this->RendererObserver);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::RemoveRendererObserver()
{
  if (this->ObservedRenderer)
    {
    this->ObservedRenderer->RemoveObserver(this->RendererObservationId);
    this->RendererObservationId = 0;
    this->ObservedRenderer = NULL;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::CreateCubeOrientationModel()
{
  this->CubeActor = vtkSmartPointer<vtkAnnotatedCubeActor>::New();
  this->CubeActor->SetXPlusFaceText("R");
  this->CubeActor->SetXMinusFaceText("L");
  this->CubeActor->SetYMinusFaceText("P");
  this->CubeActor->SetYPlusFaceText("A");
  this->CubeActor->SetZMinusFaceText("I");
  this->CubeActor->SetZPlusFaceText("S");
  this->CubeActor->SetZFaceTextRotation(90);
  this->CubeActor->GetTextEdgesProperty()->SetColor(0.95,0.95,0.95);
  this->CubeActor->GetTextEdgesProperty()->SetLineWidth(2);
  this->CubeActor->GetCubeProperty()->SetColor(0.15,0.15,0.15);
  this->CubeActor->PickableOff();
  this->CubeActor->DragableOff();
  this->CubeActor->SetVisibility(0);
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::CreateHumanOrientationModel(const char* modelFilename)
{
  vtkNew<vtkXMLPolyDataReader> polyDataReader;
  polyDataReader->SetFileName(modelFilename);
  polyDataReader->Update();
  polyDataReader->GetOutput()->GetPointData()->SetActiveScalars("Color");

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(polyDataReader->GetOutputPort());
  mapper->SetColorModeToDirectScalars();

  this->HumanActor = vtkSmartPointer<vtkActor>::New();
  this->HumanActor->SetMapper(mapper.GetPointer());
  const double scale = 0.01;
  this->HumanActor->SetScale(scale,scale,scale);
  this->HumanActor->PickableOff();
  this->HumanActor->DragableOff();
  this->HumanActor->SetVisibility(0);
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::CreateAxesOrientationModel()
{
  this->AxesActor = vtkSmartPointer<vtkAxesActor>::New();
  this->AxesActor->SetXAxisLabelText("R");
  this->AxesActor->SetYAxisLabelText("A");
  this->AxesActor->SetZAxisLabelText("S");
  this->AxesActor->PickableOff();
  this->AxesActor->DragableOff();
  this->AxesActor->SetVisibility(0);
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerVisibility()
{
  bool visible = false;
  int representation = -1;

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (sliceNode)
    {
    visible = sliceNode->GetOrientationMarkerVisibility();
    representation = sliceNode->GetOrientationMarkerRepresentation();
    }

  if (this->CubeActor)
    {
    this->CubeActor->SetVisibility(visible && representation==vtkMRMLSliceNode::OrientationMarkerCube);
    }
  if (this->HumanActor)
    {
    this->HumanActor->SetVisibility(visible && representation==vtkMRMLSliceNode::OrientationMarkerHuman);
    }
  if (this->AxesActor)
    {
    this->AxesActor->SetVisibility(visible && representation==vtkMRMLSliceNode::OrientationMarkerAxes);
    }

  //this->External->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerOrientation()
{
  vtkRenderer* renderer = this->External->GetRenderer();
  if (renderer==NULL)
    {
    vtkErrorWithObjectMacro(this->External, "vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerOrientation() failed: renderer is invalid");
    return;
    }

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (sliceNode && this->MarkerRenderer)
    {
    // Calculate the camera position and viewup based on XYToRAS matrix
    vtkMatrix4x4* sliceToRas = sliceNode->GetSliceToRAS();
    vtkNew<vtkMatrix3x3> sliceToRasOrientation;
    for (int r=0; r<3; r++)
      {
      for (int c=0; c<3; c++)
        {
        sliceToRasOrientation->SetElement(r,c,sliceToRas->GetElement(r,c));
        }
      }
    double det = sliceToRasOrientation->Determinant();
    double cameraPositionMultiplier = 5.0;
    double y[3] = {0,0, det>0 ? -cameraPositionMultiplier : cameraPositionMultiplier};
    // Calculating position
    double position[3] = {0};
    sliceToRasOrientation->MultiplyPoint(y,position);
    // Calculating viewUp
    double n[3] = {0,1,0};
    double viewUp[3] = {0};
    sliceToRasOrientation->MultiplyPoint(n,viewUp);

    vtkCamera* camera = this->MarkerRenderer->GetActiveCamera();
    camera->SetPosition(-position[0],-position[1],-position[2]);
    camera->SetViewUp(viewUp[0],viewUp[1],viewUp[2]);

    this->MarkerRenderer->ResetCamera();

    return;
    }

  vtkMRMLViewNode* threeDViewNode = vtkMRMLViewNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (threeDViewNode && this->ObservedRenderer)
    {
    vtkCamera *cam = this->ObservedRenderer->GetActiveCamera();
    double pos[3], fp[3], viewup[3];
    cam->GetPosition( pos );
    cam->GetFocalPoint( fp );
    cam->GetViewUp( viewup );

    cam = this->MarkerRenderer->GetActiveCamera();
    cam->SetPosition( pos );
    cam->SetFocalPoint( fp );
    cam->SetViewUp( viewup );
    this->MarkerRenderer->ResetCamera();

    return;
    }

  vtkErrorWithObjectMacro(this->External, "vtkMRMLOrientationMarkerDisplayableManager::UpdateMarkerOrientation() failed: displayable node is invalid");
}


//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerSize()
{
  if (this->MarkerRenderer==NULL)
    {
    vtkErrorWithObjectMacro(this->External, "vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerSize() failed: MarkerRenderer is invalid");
    return;
    }
  int sizePercent = -1;

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (sliceNode)
    {
    sizePercent = sliceNode->GetOrientationMarkerSize();
    }

  if (sizePercent>=0)
    {
    // Viewport: xmin, ymin, xmax, ymax; range: 0.0-1.0; origin is bottom left
    double* viewport= this->MarkerRenderer->GetViewport();
    // Determine the available renderer size in pixels
    double minX=0;
    double minY=0;
    this->MarkerRenderer->NormalizedDisplayToDisplay(minX,minY);
    double maxX=1;
    double maxY=1;
    this->MarkerRenderer->NormalizedDisplayToDisplay(maxX,maxY);
    int rendererSizeInPixels[2] = {maxX-minX, maxY-minY};
    if (rendererSizeInPixels[0]>0 && rendererSizeInPixels[1]>0)
      {
      // Compute normalized size for a square-shaped viewport. Square size is defined a percentage of renderer width.
      double viewPortSizeInPixels = double(rendererSizeInPixels[0])*(0.01*sizePercent);
      double newViewport[4] = {1.0-viewPortSizeInPixels/double(rendererSizeInPixels[0]), 0.0,
        1.0, viewPortSizeInPixels/double(rendererSizeInPixels[1])};
      if (newViewport[0]<0.0) newViewport[0]=0.0;
      if (newViewport[1]<0.0) newViewport[1]=0.0;
      if (newViewport[2]>1.0) newViewport[1]=1.0;
      if (newViewport[3]>1.0) newViewport[3]=1.0;
      // Update the viewport
      if (newViewport[0] != viewport[0] || newViewport[1] != viewport[1]
        || newViewport[2] != viewport[2] || newViewport[3] != viewport[3])
        {
        this->MarkerRenderer->SetViewport(newViewport);
        }
      }
    }
}

//---------------------------------------------------------------------------
// vtkMRMLOrientationMarkerDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::vtkMRMLOrientationMarkerDisplayableManager()
{
  this->Internal = new vtkInternal(this);
}

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::~vtkMRMLOrientationMarkerDisplayableManager()
{
  delete this->Internal;
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::Create()
{
  this->Superclass::Create();

  vtkRenderer* renderer = this->GetRenderer();
  if (renderer==NULL)
    {
    vtkErrorMacro("vtkMRMLOrientationMarkerDisplayableManager::Create() failed: renderer is invalid");
    return;
    }

  this->Internal->MarkerRenderer = vtkSmartPointer<vtkRenderer>::New();
  this->Internal->MarkerRenderer->InteractiveOff();

  vtkRenderWindow* renderWindow = renderer->GetRenderWindow();
  if (renderWindow->GetNumberOfLayers() < 2)
    {
    renderWindow->SetNumberOfLayers( 2 );
    }
  this->Internal->MarkerRenderer->SetLayer(1);
  renderWindow->AddRenderer(this->Internal->MarkerRenderer);

  vtkMRMLViewNode* threeDViewNode = vtkMRMLViewNode::SafeDownCast(this->GetMRMLDisplayableNode());
  if (threeDViewNode)
    {
    this->Internal->AddRendererObserver(renderer);
    }

  this->Internal->CreateCubeOrientationModel();
  this->Internal->CreateAxesOrientationModel();
  this->Internal->CreateHumanOrientationModel("c:\\S4\\Modules\\Loadable\\ViewAnnotations\\Resources\\Models\\OrientationMarkerHuman.vtp");

  this->Internal->MarkerRenderer->AddViewProp(this->Internal->CubeActor);
  this->Internal->MarkerRenderer->AddViewProp(this->Internal->HumanActor);
  this->Internal->MarkerRenderer->AddViewProp(this->Internal->AxesActor);

  this->UpdateFromViewNode();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::OnMRMLDisplayableNodeModifiedEvent(vtkObject* vtkNotUsed(caller))
{
  this->UpdateFromViewNode();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::UpdateFromViewNode()
{
  this->Internal->UpdateMarkerVisibility();
  this->Internal->UpdateMarkerSize();
  this->Internal->UpdateMarkerOrientation();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::UpdateFromRenderer()
{
  this->Internal->UpdateMarkerOrientation();
}
