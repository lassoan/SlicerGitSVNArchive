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
#include "vtkMRMLCameraDisplayableManager.h"
#include "vtkMRMLDisplayableManagerGroup.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLViewNode.h>
#include <vtkMRMLCameraNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLDisplayableNode.h>

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkFollower.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOutlineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkVectorText.h>

// STD includes

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

  vtkSmartPointer<vtkRenderer> OrientationMarkerRenderers;
  vtkSmartPointer<vtkActor> CubeActor;
  vtkSmartPointer<vtkActor> HumanActor;
  vtkSmartPointer<vtkActor> AxesActor;

  vtkActor* CurrentActor;

  vtkMRMLOrientationMarkerDisplayableManager* External;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::vtkInternal(vtkMRMLOrientationMarkerDisplayableManager * external)
{
  this->External = external;
  this->CurrentActor = NULL;
}

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::~vtkInternal()
{
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::CreateCubeOrientationModel()
{
  this->CubeActor = vtkSmartPointer<vtkActor>;
  this->CubeActor->SetXPlusFaceText('R');
  this->CubeActor->SetXMinusFaceText('L');
  this->CubeActor->SetYMinusFaceText('P');
  this->CubeActor->SetYPlusFaceText('A');
  this->CubeActor->SetZMinusFaceText('I');
  this->CubeActor->SetZPlusFaceText('S');
  this->CubeActor->SetZFaceTextRotation(90);
  this->CubeActor->GetTextEdgesProperty().SetColor(0.95,0.95,0.95);
  this->CubeActor->GetTextEdgesProperty().SetLineWidth(2);
  this->CubeActor->GetCubeProperty().SetColor(0.15,0.15,0.15);
  this->CubeActor->SetPickable(0);
  this->CubeActor->SetVisibility(0);
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::CreateHumanOrientationModel(const char* modelFilename)
{
  vtkNew<vtkXMLPolyDataReader> polyDataReader;
  polyDataReader->SetFileName(modelFilename);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(polyDataReader->GetOutputPort());
  mapper->SetColorModeToDirectScalars ();

  this->HumanActor = vtkSmartPointer<vtkActor>::New();
  this->HumanActor->SetMapper(mapper.GetPointer());
  const double scale = 0.01;
  this->HumanActor->SetScale(scale,scale,scale);
  this->HumanActor->SetPickable(0);
  this->HumanActor->SetVisibility(0);
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::CreateAxesOrientationModel()
{
  this->AxesActor = vtkSmartPointer<vtkActor>::New();
  this->AxesActor->SetXAxisLabelText('R')
  this->AxesActor->SetYAxisLabelText('A')
  this->AxesActor->SetZAxisLabelText('S')
  vtkNew<vtkTransform> transform;
  transform->Translate(0,0,0)
  this->AxesActor->SetUserTransform(transform.GetPointer());
  this->AxesActor->SetPickable(0);
  this->AxesActor->SetVisibility(0);
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerVisibility()
{
  bool visible = this->External->GetMRMLViewNode()->GetBoxVisible();
  // TODO: switch types
  /*
  int stereoType = this->External->GetMRMLViewNode()->GetStereoType();
  if (stereoType == vtkMRMLViewNode::RedBlue)
    {
    renderWindow->SetStereoTypeToRedBlue();
    }
  */
  this->CubeActor->SetVisibility(visible);
  this->External->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerSize()
{
    sliceNode = sliceLogic.GetBackgroundLayer().GetSliceNode()
    sliceViewName = sliceNode.GetLayoutName()

    if self.sliceViews[sliceViewName]:
      ren = self.orientationMarkerRenderers[sliceViewName]
      rw = self.sliceViews[sliceViewName].renderWindow()
      ren.SetViewport(self.viewPortStartWidth,0,1,self.viewPortFinishHeight)
      ren.SetLayer(1)

        # Calculate the camera position and viewup based on XYToRAS matrix
        camera = vtk.vtkCamera()
        m = sliceNode.GetSliceToRAS()
        v = np.array([[m.GetElement(0,0),m.GetElement(0,1),m.GetElement(0,2)],
            [m.GetElement(1,0),m.GetElement(1,1),m.GetElement(1,2)],
            [m.GetElement(2,0),m.GetElement(2,1),m.GetElement(2,2)]])
        det = np.linalg.det(v)
        cameraPositionMultiplier = 100/self.zoomValue
        y = np.array([0,0,-cameraPositionMultiplier]) # right hand
        if det < 0: # left hand
          y = np.array([0,0,cameraPositionMultiplier])

        x = np.matrix([[m.GetElement(0,0),m.GetElement(0,1),m.GetElement(0,2)],
            [m.GetElement(1,0),m.GetElement(1,1),m.GetElement(1,2)],
            [m.GetElement(2,0),m.GetElement(2,1),m.GetElement(2,2)]])

        # Calculating position
        position = np.inner(x,y)
        camera.SetPosition(-position[0,0],-position[0,1],-position[0,2])
        # Calculating viewUp
        n = np.array([0,1,0])
        viewUp = np.inner(x,n)
        camera.SetViewUp(viewUp[0,0],viewUp[0,1],viewUp[0,2])

        #ren.PreserveDepthBufferOff()
        ren.SetActiveCamera(camera)
        rw.AddRenderer(ren)
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
void vtkMRMLOrientationMarkerDisplayableManager::AdditionalInitializeStep()
{
  // TODO: Listen to ModifiedEvent and update the box coords if needed
  //this->AddMRMLDisplayableManagerEvent(vtkMRMLViewNode::ResetFocalPointRequestedEvent);
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::Create()
{
  this->Superclass::Create();

  vtkRenderer* renderer = this->GetRenderer();
  if (renderer==NULL || this->GetMRMLViewNode()==NULL)
    {
    vtkErrorMacro("vtkMRMLOrientationMarkerDisplayableManager::Create() failed");
    return;
    }

  this->Internal->CreateCubeOrientationModel();
  this->Internal->CreateAxesOrientationModel();
  this->Internal->CreateHumanOrientationModel("c:\\S4\\Modules\\Loadable\\ViewAnnotations\\Resources\\Models\\OrientationMarkerHuman.vtp");

  renderer->AddViewProp(this->Internal->CubeActor);
  renderer->AddViewProp(this->Internal->HumanActor);
  renderer->AddViewProp(this->Internal->AxesActor);

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
}
