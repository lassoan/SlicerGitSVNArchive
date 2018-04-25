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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// MRMLDisplayableManager includes
#include "vtkMRMLCrosshairDisplayableManager.h"

// MRML includes
#include <vtkMRMLApplicationLogic.h>
#include <vtkMRMLColorNode.h>
#include <vtkMRMLCrosshairNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLLightBoxRendererManagerProxy.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLSliceNode.h>
#include <vtkSliceIntersectionWidget.h>
#include <vtkSliceIntersectionRepresentation2D.h>

// VTK includes
#include <vtkActor2D.h>
#include <vtkCallbackCommand.h>
#include <vtkCellArray.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPickingManager.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProp.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>
#include <vtkVersion.h>

// STD includes
#include <algorithm>
#include <cassert>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLCrosshairDisplayableManager );

//---------------------------------------------------------------------------
class vtkMRMLCrosshairDisplayableManager::vtkInternal
{
public:
  vtkInternal(vtkMRMLCrosshairDisplayableManager * external);
  ~vtkInternal();

  vtkObserverManager* GetMRMLNodesObserverManager();
  void Modified();

  // Slice
  vtkMRMLSliceNode* GetSliceNode();
  void UpdateSliceNode();
  void UpdateIntersectingSliceNodes();
  // Slice Composite
  vtkMRMLSliceCompositeNode* FindSliceCompositeNode();
  void SetSliceCompositeNode(vtkMRMLSliceCompositeNode* compositeNode);
  // Crosshair

  void SetCrosshairNode(vtkMRMLCrosshairNode* crosshairNode);

  // Actors
  void SetActor(vtkActor2D* prop) {Actor = prop;};

  // Build the crosshair representation
  void BuildCrosshair();

  // Add a line to the crosshair in display coordinates (needs to be
  // passed the points and cellArray to manipulate).
  void AddCrosshairLine(vtkPoints *pts, vtkCellArray *cellArray,
                        int p1x, int p1y, int p2x, int p2y);

  // Did crosshair position change?
  bool DidCrosshairPositionChange();

  // Did crosshair property change?
  bool DidCrosshairPropertyChange();

  vtkMRMLCrosshairDisplayableManager*        External;
  int                                        PickState;
  int                                        ActionState;
  vtkWeakPointer<vtkMRMLSliceCompositeNode>  SliceCompositeNode;
  vtkSmartPointer<vtkActor2D>                Actor;
  vtkWeakPointer<vtkRenderer>                LightBoxRenderer;

  vtkWeakPointer<vtkMRMLCrosshairNode>       CrosshairNode;
  int CrosshairMode;
  int CrosshairThickness;
  double CrosshairPosition[3];

  vtkSmartPointer<vtkSliceIntersectionWidget> SliceIntersectionWidget;
};


//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLCrosshairDisplayableManager::vtkInternal
::vtkInternal(vtkMRMLCrosshairDisplayableManager * external)
{
  this->External = external;
  this->SliceCompositeNode = 0;
  this->CrosshairNode = 0;
  this->Actor = 0;
  this->LightBoxRenderer = 0;
  this->CrosshairMode = -1;
  this->CrosshairThickness = -1;
  this->CrosshairPosition[0] = 0.0;
  this->CrosshairPosition[1] = 0.0;
  this->CrosshairPosition[2] = 0.0;
  this->SliceIntersectionWidget = vtkSmartPointer<vtkSliceIntersectionWidget>::New();
}

//---------------------------------------------------------------------------
vtkMRMLCrosshairDisplayableManager::vtkInternal::~vtkInternal()
{
  this->SetSliceCompositeNode(0);
  this->SetCrosshairNode(0);
  this->LightBoxRenderer = 0;
  // everything should be empty
  assert(this->SliceCompositeNode == 0);
  assert(this->CrosshairNode == 0);
}

//---------------------------------------------------------------------------
vtkObserverManager* vtkMRMLCrosshairDisplayableManager::vtkInternal::GetMRMLNodesObserverManager()
{
  return this->External->GetMRMLNodesObserverManager();
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::vtkInternal::Modified()
{
  return this->External->Modified();
}

//---------------------------------------------------------------------------
bool vtkMRMLCrosshairDisplayableManager::vtkInternal::DidCrosshairPositionChange()
{
  if (this->CrosshairNode.GetPointer() == 0)
    {
    return false;
    }

  // update the position of the actor
  double *ras = this->CrosshairNode->GetCrosshairRAS();
  double *lastRas = this->CrosshairPosition;
  double eps = 1.0e-12;
  if (fabs(lastRas[0] - ras[0]) > eps
    || fabs(lastRas[1] - ras[1]) > eps
    || fabs(lastRas[2] - ras[2]) > eps)
    {
    return true;
    }
  else
    {
    return false;
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLCrosshairDisplayableManager::vtkInternal::DidCrosshairPropertyChange()
{
  if (this->CrosshairNode.GetPointer() == 0)
    {
    return false;
    }

  if (this->CrosshairMode != this->CrosshairNode->GetCrosshairMode()
    || this->CrosshairThickness != this->CrosshairNode->GetCrosshairThickness())
    {
    return true;
    }
  else
    {
    return false;
    }
}

//---------------------------------------------------------------------------
vtkMRMLSliceNode* vtkMRMLCrosshairDisplayableManager::vtkInternal
::GetSliceNode()
{
  return this->External->GetMRMLSliceNode();
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::vtkInternal::UpdateSliceNode()
{
  assert(!this->GetSliceNode() || this->GetSliceNode()->GetLayoutName());
  // search the scene for a matching slice composite node
  if (!this->SliceCompositeNode.GetPointer() || // the slice composite has been deleted
      !this->SliceCompositeNode->GetLayoutName() || // the slice composite points to a diff slice node
      strcmp(this->SliceCompositeNode->GetLayoutName(),
             this->GetSliceNode()->GetLayoutName()))
    {
    vtkMRMLSliceCompositeNode* sliceCompositeNode =
      this->FindSliceCompositeNode();
    this->SetSliceCompositeNode(sliceCompositeNode);
    }

  // search for the Crosshair node
  vtkMRMLCrosshairNode* crosshairNode = vtkMRMLCrosshairDisplayableManager::FindCrosshairNode(this->External->GetMRMLScene());
  this->SetCrosshairNode(crosshairNode);
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::vtkInternal::UpdateIntersectingSliceNodes()
{
  if (this->External->GetMRMLScene() == 0)
  {
    this->SliceIntersectionWidget->GetSliceIntersectionRepresentation()->SetSliceNode(NULL);
    this->SliceIntersectionWidget->GetSliceIntersectionRepresentation()->RemoveAllIntersectingSliceNodes();
    return;
  }


  if (!this->SliceIntersectionWidget->GetRepresentation())
  {
    this->SliceIntersectionWidget->CreateDefaultRepresentation();
    if (this->External->GetInteractor()->GetPickingManager())
    {
      if (!(this->External->GetInteractor()->GetPickingManager()->GetEnabled()))
      {
        // Managed picking is on by default on the seed widget, but the interactor
        // will need to have it's picking manager turned on once seed widgets are
        // going to be used to avoid dragging seeds that are behind others.
        // Enabling it before setting the interactor on the seed widget seems to
        // work better with tests of two fiducial lists.
        this->External->GetInteractor()->GetPickingManager()->EnabledOn();
      }
    }
    this->SliceIntersectionWidget->SetInteractor(this->External->GetInteractor());
    this->SliceIntersectionWidget->SetCurrentRenderer(this->External->GetRenderer());
  }

  this->SliceIntersectionWidget->GetSliceIntersectionRepresentation()->SetSliceNode(this->GetSliceNode());

  if (this->GetSliceNode())
  {
    vtkMRMLNode* node;
    vtkCollectionSimpleIterator it;
    vtkCollection* scene = this->External->GetMRMLScene()->GetNodes();
    for (scene->InitTraversal(it);
      (node = vtkMRMLNode::SafeDownCast(scene->GetNextItemAsObject(it)));)
    {
      vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(node);
      if (sliceNode)
      {
        this->SliceIntersectionWidget->GetSliceIntersectionRepresentation()->AddIntersectingSliceNode(sliceNode);
      }
    }
  }
}

//---------------------------------------------------------------------------
vtkMRMLSliceCompositeNode* vtkMRMLCrosshairDisplayableManager::vtkInternal
::FindSliceCompositeNode()
{
  if (this->GetSliceNode() == 0 ||
      this->External->GetMRMLApplicationLogic() == 0)
    {
    return 0;
    }

  vtkMRMLSliceLogic *sliceLogic = NULL;
  vtkMRMLApplicationLogic *mrmlAppLogic = this->External->GetMRMLApplicationLogic();
  if (mrmlAppLogic)
    {
    sliceLogic = mrmlAppLogic->GetSliceLogic(this->GetSliceNode());
    }
  if (sliceLogic)
    {
    return sliceLogic->GetSliceCompositeNode(this->GetSliceNode());
    }
  // no matching slice composite node is found
  return 0;
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::vtkInternal
::SetSliceCompositeNode(vtkMRMLSliceCompositeNode* compositeNode)
{
  if (this->SliceCompositeNode == compositeNode)
    {
    return;
    }
  vtkSetAndObserveMRMLNodeMacro(this->SliceCompositeNode, compositeNode);
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::vtkInternal
::SetCrosshairNode(vtkMRMLCrosshairNode* crosshairNode)
{
  if (this->CrosshairNode == crosshairNode)
    {
    return;
    }
  vtkSetAndObserveMRMLNodeMacro(this->CrosshairNode, crosshairNode);
}

//---------------------------------------------------------------------------
vtkMRMLCrosshairNode* vtkMRMLCrosshairDisplayableManager::FindCrosshairNode(vtkMRMLScene* scene)
{
  if (scene == 0)
    {
    return 0;
    }

  vtkMRMLNode* node;
  vtkCollectionSimpleIterator it;
  vtkSmartPointer<vtkCollection> crosshairs;
  crosshairs.TakeReference(scene->GetNodesByClass("vtkMRMLCrosshairNode"));
  for (crosshairs->InitTraversal(it);
       (node = (vtkMRMLNode*)crosshairs->GetNextItemAsObject(it)) ;)
    {
    vtkMRMLCrosshairNode* crosshairNode =
      vtkMRMLCrosshairNode::SafeDownCast(node);
    if (crosshairNode
        && crosshairNode->GetCrosshairName() == std::string("default"))
      {
      return crosshairNode;
      }
    }
  // no matching crosshair node is found
  //assert(0);
  return 0;
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::vtkInternal::BuildCrosshair()
{
  // Remove the old actor is any
  if (this->Actor.GetPointer())
    {
    if (this->LightBoxRenderer)
      {
      this->LightBoxRenderer->RemoveActor(this->Actor);
      }
    this->Actor = 0;
    }

  if (!this->CrosshairNode.GetPointer())
    {
    this->SliceIntersectionWidget->Off();
    return;
    }

  // Get the size of the window
  int *screenSize = this->External->GetInteractor()->GetRenderWindow()->GetScreenSize();

  // Constants in display coordinates to define the crosshair
  int negW = -1.0*screenSize[0];
  int negWminus = -5;
  int negWminus2 = -10;
  int posWplus = 5;
  int posWplus2 = 10;
  int posW = screenSize[0];

  int negH = -1.0*screenSize[1];
  int negHminus = -5;
  int negHminus2 = -10;
  int posHplus = 5;
  int posHplus2 = 10;
  int posH = screenSize[1];

  // Set up the VTK data structures
  vtkNew<vtkPolyData> polyData;
  vtkNew<vtkCellArray> cellArray;
  vtkNew<vtkPoints> points;
  polyData->SetLines(cellArray.GetPointer());
  polyData->SetPoints(points.GetPointer());

  vtkNew<vtkPolyDataMapper2D> mapper;
  vtkNew<vtkActor2D> actor;
  mapper->SetInputData(polyData.GetPointer());
  actor->SetMapper(mapper.GetPointer());

  if (this->LightBoxRenderer)
    {
    this->LightBoxRenderer->AddActor(actor.GetPointer());
    }

  // Cache the actor
  this->SetActor(actor.GetPointer());

  // Define the geometry
  switch (this->CrosshairNode->GetCrosshairMode())
    {
    case vtkMRMLCrosshairNode::NoCrosshair:
      break;
    case vtkMRMLCrosshairNode::ShowBasic:
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             0, negH, 0, negHminus);
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             0, posHplus, 0, posH);
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             negW, 0, negWminus, 0);
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             posWplus, 0, posW, 0);
      break;
    case vtkMRMLCrosshairNode::ShowIntersection:
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             negW, 0, posW, 0);
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             0, negH, 0, posH);
      break;
    case vtkMRMLCrosshairNode::ShowSmallBasic:
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             0, negHminus2, 0, negHminus);
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             0, posHplus, 0, posHplus2);
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             negWminus2, 0, negWminus, 0);
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             posWplus, 0, posWplus2, 0);
      break;
    case vtkMRMLCrosshairNode::ShowSmallIntersection:
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             0, negHminus2, 0, posHplus2);
      this->AddCrosshairLine(points.GetPointer(), cellArray.GetPointer(),
                             negWminus2, 0, posWplus2, 0);
      break;
    default:
      break;
    }

  // Set the properties
  //

  // Line Width
  if (this->CrosshairNode->GetCrosshairThickness() == vtkMRMLCrosshairNode::Fine)
    {
    actor->GetProperty()->SetLineWidth(1);
    }
  else if (this->CrosshairNode->GetCrosshairThickness() == vtkMRMLCrosshairNode::Medium)
    {
    actor->GetProperty()->SetLineWidth(3);
    }
  else if (this->CrosshairNode->GetCrosshairThickness() == vtkMRMLCrosshairNode::Thick)
    {
    actor->GetProperty()->SetLineWidth(5);
    }

  // Color
  actor->GetProperty()->SetColor(1.0, 0.8, 0.1);
  actor->GetProperty()->SetOpacity(1.0);

  /*
  vtkSliceIntersectionRepresentation2D* rep = vtkSliceIntersectionRepresentation2D::SafeDownCast(this->SliceIntersectionWidget->GetRepresentation());
  if (rep)
  {
    double bounds[] = { -10, 10, -20, 20, -30, 30 };
    rep->PlaceWidget(bounds);
  }
  */

  this->SliceIntersectionWidget->SetInteractor(this->External->GetInteractor());

  // Set the visibility
  if (this->CrosshairNode->GetCrosshairMode() == vtkMRMLCrosshairNode::NoCrosshair)
    {
    actor->VisibilityOff();
    this->SliceIntersectionWidget->Off();
    }
  else
    {
    actor->VisibilityOn();
    this->SliceIntersectionWidget->On();
    }

  this->CrosshairMode = this->CrosshairNode->GetCrosshairMode();
  this->CrosshairThickness = this->CrosshairNode->GetCrosshairThickness();
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::vtkInternal::AddCrosshairLine(vtkPoints *pts, vtkCellArray *cellArray, int p1x, int p1y, int p2x, int p2y)
{
  vtkIdType p1 = pts->InsertNextPoint(p1x, p1y, 0);
  vtkIdType p2 = pts->InsertNextPoint(p2x, p2y, 0);

  cellArray->InsertNextCell(2);
  cellArray->InsertCellPoint(p1);
  cellArray->InsertCellPoint(p2);
}


//---------------------------------------------------------------------------
// vtkMRMLCrosshairDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLCrosshairDisplayableManager::vtkMRMLCrosshairDisplayableManager()
{
  this->Internal = new vtkInternal(this);
}

//---------------------------------------------------------------------------
vtkMRMLCrosshairDisplayableManager::~vtkMRMLCrosshairDisplayableManager()
{
  delete this->Internal;
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::ObserveMRMLScene()
{
  this->Internal->BuildCrosshair();
  this->Superclass::ObserveMRMLScene();
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::UpdateFromMRMLScene()
{
  this->Internal->UpdateSliceNode();

  this->Internal->UpdateIntersectingSliceNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::UnobserveMRMLScene()
{
  this->Internal->SliceIntersectionWidget->GetSliceIntersectionRepresentation()->SetSliceNode(NULL);
  this->Internal->SliceIntersectionWidget->GetSliceIntersectionRepresentation()->RemoveAllIntersectingSliceNodes();
  this->Internal->SetSliceCompositeNode(0);
  this->Internal->SetCrosshairNode(0);
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager
::OnMRMLSceneNodeAdded(vtkMRMLNode* nodeAdded)
{
  if (this->GetMRMLScene()->IsBatchProcessing() ||
    !nodeAdded->IsA("vtkMRMLSliceNode"))
  {
    return;
  }
  this->Internal->SliceIntersectionWidget->GetSliceIntersectionRepresentation()
    ->AddIntersectingSliceNode(vtkMRMLSliceNode::SafeDownCast(nodeAdded));
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager
::OnMRMLSceneNodeRemoved(vtkMRMLNode* nodeRemoved)
{
  if (!nodeRemoved->IsA("vtkMRMLSliceNode"))
  {
    return;
  }

  this->Internal->SliceIntersectionWidget->GetSliceIntersectionRepresentation()
    ->RemoveIntersectingSliceNode(vtkMRMLSliceNode::SafeDownCast(nodeRemoved));
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::OnMRMLNodeModified(
    vtkMRMLNode* vtkNotUsed(node))
{
  // update the properties and style of the crosshair
  bool builtCrosshair = false;
  if (this->Internal->DidCrosshairPropertyChange())
    {
    this->Internal->BuildCrosshair();
    builtCrosshair = true;
    }

  // update the position of the actor
  if ((this->Internal->DidCrosshairPositionChange() || builtCrosshair)
      && this->Internal->Actor)
    {
    double xyz[3];
    double *ras = this->Internal->CrosshairNode->GetCrosshairRAS();
    this->ConvertRASToXYZ(ras, xyz);

    this->Internal->Actor->SetPosition(xyz[0], xyz[1]);

    // put the actor in the right lightbox
    if (this->GetLightBoxRendererManagerProxy())
      {
      int id = (int) (floor(xyz[2] + 0.5)); // round to find the lightbox
      vtkRenderer *renderer
        = this->GetLightBoxRendererManagerProxy()->GetRenderer(id);
      if (renderer == NULL)
        {
        // crosshair must not be displayed in this view
        this->Internal->Actor->SetVisibility(false);
        }
      else
        {
        // crosshair must be displayed in this view
        if (this->Internal->LightBoxRenderer == renderer)
          {
          this->Internal->Actor->SetVisibility(true);
          }
        else
          {
          if (this->Internal->LightBoxRenderer)
            {
            this->Internal->LightBoxRenderer->RemoveActor(this->Internal->Actor);
            }
          this->Internal->Actor->SetVisibility(true);
          renderer->AddActor(this->Internal->Actor);
          this->Internal->LightBoxRenderer = renderer;
          }
        }
      }

    double *lastRas = this->Internal->CrosshairPosition;
    lastRas[0] = ras[0];
    lastRas[1] = ras[1];
    lastRas[2] = ras[2];
    }

  // Request a render
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::Create()
{
  // Setup the SliceNode, SliceCompositeNode, CrosshairNode
  this->Internal->UpdateSliceNode();
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::AdditionalInitializeStep()
{
  // Build the initial crosshair representation
  this->Internal->BuildCrosshair();
}

//---------------------------------------------------------------------------
void vtkMRMLCrosshairDisplayableManager::OnMRMLSliceNodeModifiedEvent()
{
  if (this->Internal->CrosshairNode)
    {
    // slice position may have changed,
    // change last crosshair position to force a position update
    this->Internal->CrosshairPosition[0] += 100;
    this->OnMRMLNodeModified(this->Internal->CrosshairNode);
    }
}
