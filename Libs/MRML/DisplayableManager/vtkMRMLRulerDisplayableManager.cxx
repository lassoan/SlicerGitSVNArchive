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

  This file was originally developed by Andras Lasso, PerkLab, Queen's University.

==============================================================================*/

// MRMLDisplayableManager includes
#include "vtkMRMLRulerDisplayableManager.h"

// MRML includes
#include <vtkMRMLAbstractViewNode.h>
#include <vtkMRMLLogic.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkActor2D.h>
#include <vtkAxisActor2D.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
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
#include <vtksys/SystemTools.hxx>

// STD includes
#include <sstream>

// Constants
static const int RENDERER_LAYER = 1; // layer ID where the orientation marker will be displayed
static const char* AXES_LABELS[] = {"L", "R", "P", "A", "I", "S"};
static const int MINIMUM_RULER_LENGTH_PIXEL = 200;
static const int RULER_FONT_SIZE = 14;
static const int RULER_LINE_MARGIN = 10; // vertical distance of line from edge of view
static const int RULER_THICKNESS = 10;
static const int RULER_TEXT_MARGIN = 10; // horizontal distaace of ruler text from ruler line


//---------------------------------------------------------------------------
class vtkRulerRendererUpdateObserver : public vtkCommand
{
public:
  static vtkRulerRendererUpdateObserver *New()
    {
    return new vtkRulerRendererUpdateObserver;
    }
  vtkRulerRendererUpdateObserver()
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
  vtkWeakPointer<vtkMRMLRulerDisplayableManager> DisplayableManager;
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLRulerDisplayableManager );

//---------------------------------------------------------------------------
class vtkMRMLRulerDisplayableManager::vtkInternal
{
public:
  vtkInternal(vtkMRMLRulerDisplayableManager * external);
  ~vtkInternal();

  void CreateRuler();

  void UpdateRuler();

  void CreateMarkerRenderer();

  void AddRendererUpdateObserver(vtkRenderer* renderer);
  void RemoveRendererUpdateObserver();

  vtkSmartPointer<vtkRenderer> MarkerRenderer;
  vtkSmartPointer<vtkTextActor> RulerTextActor;
  //vtkSmartPointer<vtkActor2D> RulerLineActor;
  vtkSmartPointer<vtkAxisActor2D> RulerLineActor;

  // Ruler points are in normalized coordinates (ruler will set to the correct size by adjusting actor scaling):
  // X: -0.5 .. 0.5
  // Y: 0 .. 1
  // Z: 0
  vtkSmartPointer<vtkPoints> RulerLinePoints;

  vtkSmartPointer<vtkRulerRendererUpdateObserver> RendererUpdateObserver;
  int RendererUpdateObservationId;
  vtkWeakPointer<vtkRenderer> ObservedRenderer;

  vtkMRMLRulerDisplayableManager* External;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLRulerDisplayableManager::vtkInternal::vtkInternal(vtkMRMLRulerDisplayableManager * external)
{
  this->External = external;
  this->RendererUpdateObserver = vtkSmartPointer<vtkRulerRendererUpdateObserver>::New();
  this->RendererUpdateObserver->DisplayableManager = this->External;
  this->RendererUpdateObservationId = 0;
}

//---------------------------------------------------------------------------
vtkMRMLRulerDisplayableManager::vtkInternal::~vtkInternal()
{
  RemoveRendererUpdateObserver();
}

//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::vtkInternal::AddRendererUpdateObserver(vtkRenderer* renderer)
{
  RemoveRendererUpdateObserver();
  if (renderer)
    {
    this->ObservedRenderer = renderer;
    this->RendererUpdateObservationId = renderer->AddObserver(vtkCommand::StartEvent, this->RendererUpdateObserver);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::vtkInternal::RemoveRendererUpdateObserver()
{
  if (this->ObservedRenderer)
    {
    this->ObservedRenderer->RemoveObserver(this->RendererUpdateObservationId);
    this->RendererUpdateObservationId = 0;
    this->ObservedRenderer = NULL;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::vtkInternal::CreateMarkerRenderer()
{
  vtkRenderer* renderer = this->External->GetRenderer();
  if (renderer==NULL)
    {
    vtkErrorWithObjectMacro(this->External, "vtkMRMLRulerDisplayableManager::vtkInternal::CreateMarkerRenderer() failed: renderer is invalid");
    return;
    }

  //this->MarkerRenderer = renderer;

  this->MarkerRenderer = vtkSmartPointer<vtkRenderer>::New();
  this->MarkerRenderer->InteractiveOff();

  vtkRenderWindow* renderWindow = renderer->GetRenderWindow();
  if (renderWindow->GetNumberOfLayers() < RENDERER_LAYER+1)
    {
    renderWindow->SetNumberOfLayers( RENDERER_LAYER+1 );
    }
  this->MarkerRenderer->SetLayer(RENDERER_LAYER);
  renderWindow->AddRenderer(this->MarkerRenderer);

  // In 3D viewers we need to follow the renderer and update the orientation marker accordingly
  vtkMRMLViewNode* threeDViewNode = vtkMRMLViewNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (threeDViewNode)
    {
    this->AddRendererUpdateObserver(renderer);
    }

  this->CreateRuler();

  this->MarkerRenderer->AddViewProp(this->RulerTextActor);
  this->MarkerRenderer->AddViewProp(this->RulerLineActor);
}


//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::vtkInternal::CreateRuler()
{
  const int numberOfTickLines = 11;
/*
  // Points
  this->RulerLinePoints = vtkSmartPointer<vtkPoints>::New();
  this->RulerLinePoints->SetNumberOfPoints(numberOfTickLines*2);
  double normalizedTickLineDistance = 1.0/double(numberOfTickLines-1);
  for (int lineIndex=0; lineIndex<numberOfTickLines; lineIndex++)
    {
    double tickLinePosition = -0.5+lineIndex*normalizedTickLineDistance;
    this->RulerLinePoints.SetPoint(0, tickLinePosition, 0, 0);
    this->RulerLinePoints.SetPoint(1, tickLinePosition, 1, 0);
    }

  // Lines
  vtkNew<vtkCellArray> linesArray;
  // Vertical tick lines
  for (int lineIndex=0; lineIndex<numberOfTickLines; lineIndex++)
    {
    vtkNew<vtkLine> tickLine;
    tickLine->GetPointIds().SetId(0,lineIndex*2)
    tickLine->GetPointIds().SetId(1,lineIndex*2+1)
    linesArray->InsertNextCell(tickLine.GetPointer())
    }
  // Long horizontal line
  vtkNew<vtkLine> horizontalLine;
  horizontalLine->GetPointIds().SetId(0,0)
  horizontalLine->GetPointIds().SetId(1,numberOfTickLines*2)
  linesArray->InsertNextCell(horizontalLine.GetPointer())

  // PolyData
  vtkNew<vtkPolyData> rulerLinePolyData;
  rulerLinePolyData->SetPoints(this->RulerLinePoints.GetPointer())
  rulerLinePolyData.SetLines(linesArray.GetPointer())

  vtkNew<vtkPolyDataMapper2D> mapper;
  mapper->SetInputData(rulerLinePolyData.GetPointer());

  this->RulerLineActor = vtkSmartPointer<vtkActor2D>::New();
  this->RulerLineActor->SetMapper(mapper.GetPointer());
  //const double scale = 0.01;
  //this->RulerLineActor->SetScale(scale,scale,scale);
  */

  this->RulerLineActor = vtkSmartPointer<vtkAxisActor2D>::New();
  this->RulerLineActor->GetPoint1Coordinate()->SetCoordinateSystemToDisplay();
  this->RulerLineActor->GetPoint2Coordinate()->SetCoordinateSystemToDisplay();

  this->RulerLineActor->PickableOff();
  this->RulerLineActor->DragableOff();
  this->RulerLineActor->VisibilityOff();
  //this->RulerLineActor->GetProperty()->SetLineWidth(1);
  //this->RulerLineActor->GetProperty()->SetColor(1,1,1);

  this->RulerTextActor = vtkSmartPointer<vtkTextActor>::New();
  vtkTextProperty* textProperty = this->RulerTextActor->GetTextProperty();
  textProperty->SetFontSize(RULER_FONT_SIZE);
  textProperty->SetFontFamilyToArial();
}

//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::vtkInternal::UpdateRuler()
{
  vtkMRMLAbstractViewNode* viewNode = vtkMRMLAbstractViewNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (!viewNode || !viewNode->GetRulerEnabled())
    {
    return;
    }

  if (!this->RulerTextActor || !this->RulerLineActor)
    {
    return;
    }

  int type = viewNode->GetRulerType();
  if (type==vtkMRMLAbstractViewNode::RulerTypeNone)
    {
    // ruler not visible, no updates are needed
    this->RulerTextActor->SetVisibility(false);
    this->RulerLineActor->SetVisibility(false);
    return;
    }

  int viewWidthPixel = 0;
  double scalingFactorPixelPerMm = 0;

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(viewNode);
  vtkMRMLViewNode* threeDViewNode = vtkMRMLViewNode::SafeDownCast(viewNode);
  if (sliceNode)
    {
    viewWidthPixel = sliceNode->GetDimensions()[0];

    vtkNew<vtkMatrix4x4> rasToXY;
    vtkMatrix4x4::Invert(sliceNode->GetXYToRAS(), rasToXY.GetPointer());

    // TODO: The current logic only supports rulers from 1mm to 10cm
    // add support for other ranges.
    scalingFactorPixelPerMm = sqrt(
      rasToXY->GetElement(0,0)*rasToXY->GetElement(0,0) +
      rasToXY->GetElement(0,1)*rasToXY->GetElement(0,1));

    }
  else if (threeDViewNode && this->ObservedRenderer)
    {
    vtkCamera *cam = this->ObservedRenderer->GetActiveCamera();
    if (cam->GetParallelProjection())
      {
      // Viewport: xmin, ymin, xmax, ymax; range: 0.0-1.0; origin is bottom left
      double* viewport = this->MarkerRenderer->GetViewport();
      // Determine the available renderer size in pixels
      double minX = 0;
      double minY = 0;
      this->MarkerRenderer->NormalizedDisplayToDisplay(minX, minY);
      double maxX = 1;
      double maxY = 1;
      this->MarkerRenderer->NormalizedDisplayToDisplay(maxX, maxY);
      int rendererSizeInPixels[2] = {maxX-minX, maxY-minY};

      viewWidthPixel = rendererSizeInPixels[0];

      // Parallel scale: height of the viewport in world-coordinate distances.
      // Larger numbers produce smaller images.
      static double adjust=1.0;
      scalingFactorPixelPerMm = double(rendererSizeInPixels[1])/cam->GetParallelScale()/adjust;
      }
    }
  else
    {
    vtkErrorWithObjectMacro(this->External, "vtkMRMLRulerDisplayableManager::UpdateMarkerOrientation() failed: displayable node is invalid");
    }

  double rulerMaxLengthMm = 0;
    if (scalingFactorPixelPerMm != 0)
      {
      rulerMaxLengthMm = viewWidthPixel/scalingFactorPixelPerMm/4; // TODO: rename
      }

  const int rulerAllowedLengthsMm[] = {1,5,10,50,100};
  int rulerSizesArraySize = sizeof(rulerAllowedLengthsMm)/sizeof(rulerAllowedLengthsMm[0]);
  if (viewWidthPixel < MINIMUM_RULER_LENGTH_PIXEL
    || rulerMaxLengthMm < rulerAllowedLengthsMm[0]*0.5 || rulerMaxLengthMm > rulerAllowedLengthsMm[rulerSizesArraySize-1]*5)
    {
    // ruler is too small or too big to display
    this->RulerTextActor->SetVisibility(false);
    this->RulerLineActor->SetVisibility(false);
    return;
    }

  // Find the value in rulerAllowedLengthsMm that is closest to rulerMaxLengthMm
  int bestMatchingRulerLengthIndex = 0;
  double bestMatchingRulerLengthDifferenceMm = fabs(rulerAllowedLengthsMm[0]-rulerMaxLengthMm);
  for (int i=1; i<rulerSizesArraySize; i++)
    {
    if (fabs(rulerAllowedLengthsMm[i]-rulerMaxLengthMm)<bestMatchingRulerLengthDifferenceMm)
      {
      bestMatchingRulerLengthIndex = i;
      bestMatchingRulerLengthDifferenceMm = fabs(rulerAllowedLengthsMm[i]-rulerMaxLengthMm);
      }
    else
      {
      // list is ordered, therefore if the difference has not got smaller
      // then it will not, so we can stop searching
      break;
      }
    }

  std::stringstream scalingFactorString;
  int rulerLengthMm = rulerAllowedLengthsMm[bestMatchingRulerLengthIndex];
  if (rulerLengthMm>10)
    {
    scalingFactorString << int(rulerLengthMm/10) << " cm";
    }
  else
    {
    scalingFactorString << rulerLengthMm << " mm";
    }

  double pointScale[3] = {scalingFactorPixelPerMm, RULER_THICKNESS, 1};
  double pointOrigin[3] = {double(viewWidthPixel)/2.0, RULER_LINE_MARGIN, 0};

  this->RulerLineActor->SetPoint2(pointOrigin[0]-double(rulerLengthMm)*scalingFactorPixelPerMm/2.0, RULER_LINE_MARGIN);
  this->RulerLineActor->SetPoint1(pointOrigin[0]+double(rulerLengthMm)*scalingFactorPixelPerMm/2.0, RULER_LINE_MARGIN);


  this->RulerTextActor->SetInput(scalingFactorString.str().c_str());
  //# set ruler text actor position
  this->RulerTextActor->SetDisplayPosition(int((viewWidthPixel+rulerLengthMm*scalingFactorPixelPerMm)/2)+RULER_TEXT_MARGIN,RULER_LINE_MARGIN);

  this->RulerTextActor->SetVisibility(type!=vtkMRMLAbstractViewNode::RulerTypeNone);
  this->RulerLineActor->SetVisibility(type!=vtkMRMLAbstractViewNode::RulerTypeNone);

  //this->External->RequestRender();

}

//---------------------------------------------------------------------------
// vtkMRMLRulerDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLRulerDisplayableManager::vtkMRMLRulerDisplayableManager()
{
  this->Internal = new vtkInternal(this);
}

//---------------------------------------------------------------------------
vtkMRMLRulerDisplayableManager::~vtkMRMLRulerDisplayableManager()
{
  delete this->Internal;
}

//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::Create()
{
  this->Internal->CreateMarkerRenderer();
  this->Superclass::Create();
//  this->UpdateFromViewNode();
}

//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::UpdateFromViewNode()
{
  // View node is changed, which may mean that either the marker type (visibility), size, or orientation is changed
  this->Internal->UpdateRuler();
}

//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::OnMRMLDisplayableNodeModifiedEvent(vtkObject* vtkNotUsed(caller))
{
  // view node is changed
  this->UpdateFromViewNode();
}

//---------------------------------------------------------------------------
void vtkMRMLRulerDisplayableManager::UpdateFromRenderer()
{
  // Rendering is performed, so let's re-render the marker with up-to-date orientation
  this->Internal->UpdateRuler();
}
