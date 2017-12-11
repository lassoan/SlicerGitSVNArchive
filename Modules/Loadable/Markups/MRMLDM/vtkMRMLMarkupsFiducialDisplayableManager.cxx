/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// MarkupsModule/MRML includes
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsFiducialDisplayableManager.h"

// MarkupsModule/VTKWidgets includes
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMarkupsGlyphSource2D.h>
#include <vtkMarkupsPointListWidget.h>
#include <vtkMarkupsPointListRepresentation.h>

// MRMLDisplayableManager includes
#include <vtkSliceViewInteractorStyle.h>

// MRML includes
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkMarkupsWidget.h>
#include <vtkFollower.h>
#include <vtkHandleRepresentation.h>
#include <vtkInteractorStyle.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOrientedPolygonalHandleRepresentation3D.h>
#include <vtkPickingManager.h>
#include <vtkPointHandleRepresentation2D.h>
#include <vtkProperty2D.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsFiducialDisplayableManager);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsFiducialDisplayableManager Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsFiducialWidgetCallback : public vtkCommand
{
public:
  static vtkMarkupsFiducialWidgetCallback *New()
  { return new vtkMarkupsFiducialWidgetCallback; }

  vtkMarkupsFiducialWidgetCallback()
    : Widget(NULL)
    , Node(NULL)
    , DisplayableManager(NULL)
    , LastInteractionEventMarkupIndex(-1)
    , PointMovedSinceStartInteraction(false)
  {
  }

  virtual void Execute (vtkObject *vtkNotUsed(caller), unsigned long event, void *callData)
  {

    // sanity checks
    if (!this->DisplayableManager)
      {
      return;
      }
    if (!this->Node)
      {
      return;
      }
    if (!this->Widget)
      {
      return;
      }
    // sanity checks end

    if (this->DisplayableManager->Is2DDisplayableManager())
    {
    // mark the Node with an attribute to indicate if it is currently being interacted with
    // so that other code can respond to changes only when it is not moving
    // Markups.MovingInSliceView will be set to the layout name of
    // our slice node while it is being actively manipulated
    vtkMarkupsWidget *widget = vtkMarkupsWidget::SafeDownCast(this->Widget);
    if (widget && this->DisplayableManager && this->Node)
      {
      vtkMRMLSliceNode *sliceNode = this->DisplayableManager->GetMRMLSliceNode();
      if (sliceNode)
        {
        int modifiedWasDisabled = this->Node->GetDisableModifiedEvent();
        this->Node->DisableModifiedEventOn();
        if (widget->GetWidgetState() == vtkMarkupsWidget::MovingHandle)
          {
          this->Node->SetAttribute("Markups.MovingInSliceView", sliceNode->GetLayoutName());
          std::ostringstream handleNumber;
          int *n =  reinterpret_cast<int *>(callData);
          if (n != NULL)
            {
            handleNumber << *n;
            }
          else
            {
            handleNumber << "-1";
            }
          this->Node->SetAttribute("Markups.MovingMarkupIndex", handleNumber.str().c_str());
          }
        else
          {
          const char *movingView = this->Node->GetAttribute("Markups.MovingInSliceView");
          if (movingView && !strcmp(movingView, sliceNode->GetLayoutName()))
            {
            this->Node->RemoveAttribute("Markups.MovingInSliceView");
            }
          }
        this->Node->SetDisableModifiedEvent(modifiedWasDisabled);
        }
      }
    }
    if (event ==  vtkCommand::PlacePointEvent)
      {
      // std::cout << "Warning: PlacePointEvent not supported" << std::endl;
      }
    else if (event == vtkCommand::StartInteractionEvent)
      {
      // If calldata is NULL, invoking an event may cause a crash (e.g., Python observer
      // tries to dereference the NULL pointer), therefore it's important to always pass a valid pointer
      // and indicate invalidity with value (-1).
      this->LastInteractionEventMarkupIndex = (callData ? *(reinterpret_cast<int *>(callData)) : -1);
      this->PointMovedSinceStartInteraction = false;
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointStartInteractionEvent, &this->LastInteractionEventMarkupIndex);

      // no need to propagate to MRML, just notify external observers that the user selected a markup
      // TODO: this return was not here in 2D DM
      return;
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      // save the state of the node when done moving, then call
      // PropagateWidgetToMRML to update the node one last time
      if (this->Node->GetScene())
        {
        this->Node->GetScene()->SaveStateForUndo(this->Node);
        }
      if (callData)
        {
        // Most of the time vtkCommand::EndInteractionEvent does not provide
        // handle index, but in case we get a value then update the markup index.
        this->LastInteractionEventMarkupIndex = *(reinterpret_cast<int *>(callData));
        }
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointEndInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (!this->PointMovedSinceStartInteraction)
        {
        this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointClickedEvent, &this->LastInteractionEventMarkupIndex);
        }
      this->LastInteractionEventMarkupIndex = -1;
      }

    if (this->DisplayableManager->Is2DDisplayableManager())
    {


    if (event == vtkCommand::InteractionEvent)
      {
      // restrict the widget to the renderer

      // we need the widgetRepresentation
      vtkMarkupsPointListRepresentation * representation = vtkMarkupsPointListRepresentation::SafeDownCast(this->Widget->GetRepresentation());
      if (!representation)
        {
        vtkErrorWithObjectMacro(this->Widget, "Representation is null.");
        return;
        }

      // It might be possible that in response to a single point interaction event, other points are adjusted as well,
      // so treat this callData separately from start/end interaction's callData and don't update this->LastInteractionEventMarkupIndex.
      int n = -1;
      int* nPtr =  reinterpret_cast<int *>(callData);
      if (nPtr)
        {
        n = *nPtr;
        }
      else if (representation->GetNumberOfHandles() == 1)
        {
        n = 0;
        }
      if ( n >= 0 )
        {
        // have a single handle that moved
        // first, we get the current displayCoordinates of the points
        double displayCoordinates1[4] = { 0, 0, 0, 1 };
        representation->GetHandleDisplayPosition(n, displayCoordinates1);

        // second, we copy these to restrictedDisplayCoordinates
        double restrictedDisplayCoordinates1[4] = {displayCoordinates1[0], displayCoordinates1[1], displayCoordinates1[2], displayCoordinates1[3]};

        // modify restrictedDisplayCoordinates 1 and 2, if these are outside the viewport of the current renderer
        bool changed = this->DisplayableManager->RestrictDisplayCoordinatesToViewport(restrictedDisplayCoordinates1);

        // only if we had to restrict the coordinates aka. if the coordinates changed, we update the positions
        if (changed ||
            this->DisplayableManager->GetDisplayCoordinatesChanged(displayCoordinates1,restrictedDisplayCoordinates1))
          {
          representation->SetHandleDisplayPosition(n,restrictedDisplayCoordinates1);
          }

        // propagate the changes to MRML
        this->DisplayableManager->UpdateNthMarkupPositionFromWidget(n, this->Node, this->Widget);
        this->PointMovedSinceStartInteraction = true;
        }
      else
        {
        std::cout << "Had an interaction event without the handle index!" << std::endl;
        }
      }
    }
    else
    {
    if (event == vtkCommand::InteractionEvent)
      {
      this->PointMovedSinceStartInteraction = true;
      }
    // the interaction with the widget ended, now propagate the changes to MRML
    this->DisplayableManager->PropagateWidgetToMRML(this->Widget, this->Node);
    }
  }

  void SetWidget(vtkMarkupsWidget *w)
    {
    this->Widget = w;
    }
  void SetNode(vtkMRMLMarkupsNode *n)
    {
    this->Node = n;
    }
  void SetDisplayableManager(vtkMRMLMarkupsDisplayableManager * dm)
    {
    this->DisplayableManager = dm;
    }

  vtkMarkupsWidget * Widget;
  vtkMRMLMarkupsNode * Node;
  vtkMRMLMarkupsDisplayableManager * DisplayableManager;
  int LastInteractionEventMarkupIndex;
  bool PointMovedSinceStartInteraction;
};

//---------------------------------------------------------------------------
// vtkMRMLMarkupsFiducialDisplayableManager methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkMarkupsWidget * vtkMRMLMarkupsFiducialDisplayableManager::CreateWidget(vtkMRMLMarkupsNode* markupsNode)
{
  if (!markupsNode)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return 0;
    }

  if (this->Is2DDisplayableManager())
    {
    // 2d glyphs and text need to be scaled by 1/60 to show up properly in the 2d slice windows
    this->SetScaleFactor2D(0.01667);
    }

  vtkMRMLMarkupsDisplayNode *displayNode = markupsNode->GetMarkupsDisplayNode();
  if (!displayNode)
    {
    // std::cout<<"No DisplayNode!"<<std::endl;
    }

  vtkNew<vtkMarkupsPointListRepresentation> rep;

  if (this->Is2DDisplayableManager())
    {
    // unset the glyph type which can be necessary when recreating a widget due to 2d/3d swap
    int oldGlyphType = this->Helper->GetNodeGlyphType(displayNode, 0);
    if (oldGlyphType != -1)
      {
      vtkDebugMacro("CreateWidget: found a glyph type already defined for this node: " << oldGlyphType);
      this->Helper->SetNodeGlyphType(displayNode, vtkMRMLMarkupsDisplayNode::GlyphMin - 1, 0);
      }

    vtkNew<vtkOrientedPolygonalHandleRepresentation3D> handle;

    // default to a starburst glyph, update in propagate mrml to widget
    vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
    glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::StarBurst2D);
    glyphSource->Update();
    glyphSource->SetScale(1.0);
    handle->SetHandle(glyphSource->GetOutput());

    rep->SetHandleRepresentation(handle.GetPointer());

    vtkDebugMacro("making handle for markupsNode " << markupsNode->GetName());
    vtkDebugMacro(" for sliceNode " << this->GetMRMLSliceNode()->GetName());

    if (!this->IsInLightboxMode())
      {
      vtkDebugMacro("CreateWidget: not in light box mode, making a 3d handle");
      vtkNew<vtkOrientedPolygonalHandleRepresentation3D> handle;


      // default to a sphere glyph, update in propagate mrml to widget
      vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
      glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::Sphere3D);
      glyphSource->Update();
      glyphSource->SetScale(1.0);
      handle->SetHandle(glyphSource->GetOutput());
      rep->SetHandleRepresentation(handle.GetPointer());
      }
    else
      {
      vtkDebugMacro("CreateWidget: in light box mode, making a 2d handle");
      vtkNew<vtkPointHandleRepresentation2D> handle;
      rep->SetHandleRepresentation(handle.GetPointer());
      }
    }
  else
    {
    // 3D displayable manager
    vtkNew<vtkOrientedPolygonalHandleRepresentation3D> handle;

    // default to a starburst glyph, update in propagate mrml to widget
    vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
    glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::StarBurst2D);
    glyphSource->Update();
    glyphSource->SetScale(1.0);
    handle->SetHandle(glyphSource->GetOutput());

    rep->SetHandleRepresentation(handle.GetPointer());
    }

  // widget
  vtkMarkupsPointListWidget * markupsWidget = vtkMarkupsPointListWidget::New();
  //markupsWidget->CreateDefaultRepresentation();

  markupsWidget->SetRepresentation(rep.GetPointer());

  if (this->GetInteractor()->GetPickingManager())
    {
    if (!(this->GetInteractor()->GetPickingManager()->GetEnabled()))
      {
      // Managed picking is on by default on the markups widget, but the interactor
      // will need to have it's picking manager turned on once makrups widgets are
      // going to be used to avoid dragging seeds that are behind others.
      // Enabling it before setting the interactor on the markups widget seems to
      // work better with tests of two fiducial lists.
      this->GetInteractor()->GetPickingManager()->EnabledOn();
      }
    }
  markupsWidget->SetInteractor(this->GetInteractor());

  // set the renderer on the widget and representation
  if (this->Is2DDisplayableManager())
    {
    if (!this->IsInLightboxMode())
      {
      markupsWidget->SetCurrentRenderer(this->GetRenderer());
      markupsWidget->GetRepresentation()->SetRenderer(this->GetRenderer());
      }
    else
      {
      int lightboxIndex = this->GetLightboxIndex(markupsNode, 0, 0);
      markupsWidget->SetCurrentRenderer(this->GetRenderer(lightboxIndex));
      markupsWidget->GetRepresentation()->SetRenderer(this->GetRenderer(lightboxIndex));
      }
    }
  else
    {
    markupsWidget->SetCurrentRenderer(this->GetRenderer());
    }

  vtkDebugMacro("Fids CreateWidget: Created widget for node " << markupsNode->GetID() << " with a representation");

  return markupsWidget;
  }

//---------------------------------------------------------------------------
/// Tear down the widget creation
void vtkMRMLMarkupsFiducialDisplayableManager::OnWidgetCreated(vtkMarkupsWidget * widget, vtkMRMLMarkupsNode * node)
{
  if (!widget)
    {
    vtkErrorMacro("OnWidgetCreated: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("OnWidgetCreated: MRML node was null!")
    return;
    }

  // add the callback
  vtkMarkupsFiducialWidgetCallback *myCallback = vtkMarkupsFiducialWidgetCallback::New();
  myCallback->SetNode(node);
  myCallback->SetWidget(widget);
  myCallback->SetDisplayableManager(this);
  widget->AddObserver(vtkCommand::StartInteractionEvent,myCallback);
  widget->AddObserver(vtkCommand::EndInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::InteractionEvent, myCallback);
  myCallback->Delete();
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsFiducialDisplayableManager::UpdateNthHandlePositionFromMRML(int n, vtkMarkupsWidget *markupsWidget, vtkMRMLMarkupsNode *pointsNode)
{
  if (!pointsNode || !markupsWidget)
    {
    return false;
    }
  if (n > pointsNode->GetNumberOfMarkups())
    {
    return false;
    }
  vtkMarkupsRepresentation * markupsRepresentation = vtkMarkupsRepresentation::SafeDownCast(markupsWidget->GetRepresentation());
  if (!markupsRepresentation)
    {
    return false;
    }

  bool positionChanged = false;

  if (this->Is2DDisplayableManager())
  {
    // for 2d managers, compare the display positions
    double displayCoordinates1[4] = { 0, 0, 0, 1 };
    double displayCoordinatesBuffer1[4] = { 0, 0, 0, 1 };

    // get point in world coordinates using parent transforms
    double pointTransformed[4] = { 0, 0, 0, 1 };
    // always only one point in a fiducial
    pointsNode->GetMarkupPointWorld(n, 0, pointTransformed);

    this->GetWorldToDisplayCoordinates(pointTransformed,displayCoordinates1);

    //TODO TODO: this returns incorrect result, which breaks 2D views
    markupsRepresentation->GetHandleDisplayPosition(n,displayCoordinatesBuffer1);

    if (this->GetDisplayCoordinatesChanged(displayCoordinates1,displayCoordinatesBuffer1))
      {
      if (markupsRepresentation->GetRenderer() != NULL &&
        markupsRepresentation->GetRenderer()->IsActiveCameraCreated())
        {
          markupsRepresentation->SetHandleDisplayPosition(n, displayCoordinates1);
          positionChanged = true;
        }
      }
  }
  else
  {
    // transform fiducial point using parent transforms
    double fidWorldCoord[4] = { 0, 0, 0, 1 };
    pointsNode->GetMarkupPointWorld(n, 0, fidWorldCoord);

    // for 3d managers, compare world positions
    double handleWorldCoord[4] = { 0, 0, 0, 1 };
    markupsRepresentation->GetHandleWorldPosition(n,handleWorldCoord);

    if (this->GetWorldCoordinatesChanged(handleWorldCoord, fidWorldCoord))
      {
      markupsRepresentation->GetHandleRepresentation(n)->SetWorldPosition(fidWorldCoord);
      positionChanged = true;
      }
    else
      {
      vtkDebugMacro("UpdateNthHandlePositionFromMRML: 3D: world coordinates unchanged!");
      }
  }
  return positionChanged;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager::SetNthHandleGlyphType(int n,
  vtkMRMLMarkupsDisplayNode* displayNode, vtkOrientedPolygonalHandleRepresentation3D *handleRep)
{
  if (displayNode->GlyphTypeIs3D())
  {
    if (this->Is2DDisplayableManager())
    {
      // map the 3d sphere to a filled circle, the 3d diamond to a filled
      // diamond
      vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
      if (displayNode->GetGlyphType() == vtkMRMLMarkupsDisplayNode::Sphere3D)
      {
        // std::cout << "using circle 2d for sphere 3d" << std::endl;
        glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::Circle2D);
      }
      else if (displayNode->GetGlyphType() == vtkMRMLMarkupsDisplayNode::Diamond3D)
      {
        glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::Diamond2D);
      }
      else
      {
        glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::StarBurst2D);
        // std::cout << "2d starburst" << std::endl;
      }
      glyphSource->Update();
      glyphSource->SetScale(1.0);
      handleRep->SetHandle(glyphSource->GetOutput());
    }
    else
    {
      // 3D displayable manager
      if (displayNode->GetGlyphType() == vtkMRMLMarkupsDisplayNode::Sphere3D)
      {
        // std::cout << "3d sphere" << std::endl;
        vtkNew<vtkSphereSource> sphereSource;
        sphereSource->SetRadius(0.5);
        sphereSource->SetPhiResolution(10);
        sphereSource->SetThetaResolution(10);
        sphereSource->Update();
        handleRep->SetHandle(sphereSource->GetOutput());
      }
      else
      {
        // the 3d diamond isn't supported yet, use a 2d diamond for now
        vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
        glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::Diamond2D);
        glyphSource->Update();
        glyphSource->SetScale(1.0);
        handleRep->SetHandle(glyphSource->GetOutput());
      }
    }
  }//if (displayNode->GlyphTypeIs3D())
  else
  {
    // 2D glyph
    vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
    glyphSource->SetGlyphType(displayNode->GetGlyphType());
    glyphSource->Update();
    glyphSource->SetScale(1.0);
    handleRep->SetHandle(glyphSource->GetOutput());
  }
  // TBD: keep with the assumption of one glyph type per markups node,
  // but they may have different glyphs during update
  this->Helper->SetNodeGlyphType(displayNode, displayNode->GetGlyphType(), n);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager::SetNthHandle(int n, vtkMRMLMarkupsFiducialNode* markupsNode, vtkMarkupsWidget *markupsWidget)
{
  vtkMarkupsRepresentation * markupsRepresentation = vtkMarkupsRepresentation::SafeDownCast(markupsWidget->GetRepresentation());
  if (!markupsRepresentation)
    {
    vtkErrorMacro("SetNthHandle: no markups representation in widget!");
    return;
    }
  vtkMRMLMarkupsDisplayNode *displayNode = markupsNode->GetMarkupsDisplayNode();
  if (!displayNode)
    {
    vtkDebugMacro("SetNthHandle: Could not get display node for node " << (markupsNode->GetID() ? markupsNode->GetID() : "null id"));
    return;
    }
  int numberOfHandles = markupsRepresentation->GetNumberOfHandles();

  // does this handle need to be created?
  bool createdNewHandle = false;
  if (n >= numberOfHandles)
  {
    // create a new handle
    // transform fiducial point using parent transforms

    int newHandleIndex = -1;
    if (this->Is2DDisplayableManager())
    {
      // for 2d managers, compare the display positions
      double displayCoordinates[4] = { 0, 0, 0, 1 };
      // get point in world coordinates using parent transforms
      double pointTransformed[4] = { 0, 0, 0, 1 };
      // always only one point in a fiducial
      markupsNode->GetMarkupPointWorld(n, 0, pointTransformed);
      this->GetWorldToDisplayCoordinates(pointTransformed, displayCoordinates);
      newHandleIndex = markupsWidget->AddHandle(NULL, displayCoordinates);
    }
    else
    {
      double fidWorldCoord[4] = { 0., 0., 0., 1. };
      markupsNode->GetMarkupPointWorld(n, 0, fidWorldCoord);
      newHandleIndex = markupsWidget->AddHandle(fidWorldCoord, NULL);
    }
    vtkHandleWidget* newhandle = (newHandleIndex >= 0 ? markupsWidget->GetHandle(newHandleIndex) : NULL);
    if (newhandle)
    {
      // std::cout << "SetNthHandle: created a new handle,number of seeds = " << markupsRepresentation->GetNumberOfHandles() << std::endl;
      createdNewHandle = true;
      newhandle->ManagesCursorOff();
      if (newhandle->GetEnabled() == 0)
      {
        vtkDebugMacro("turning on the new handle");
        if (this->Is2DDisplayableManager())
        {
          // only enable the handle if there is an active camera
          if (newhandle->GetRepresentation() &&
            newhandle->GetRepresentation()->GetRenderer() &&
            newhandle->GetRepresentation()->GetRenderer()->IsActiveCameraCreated())
          {
            newhandle->EnabledOn();
          }
          else
          {
            vtkDebugMacro("SetNthHandle: no active camera, delaying enabling the handle");
          }
        }
      }
      else
      {
        newhandle->EnabledOn();
      }
    }
    else
    {
      vtkErrorMacro("SetNthHandle: error creaing a new handle!");
    }
  }
  else
  {
    // update the position
    bool positionChanged = this->UpdateNthHandlePositionFromMRML(n, markupsWidget, markupsNode);
    if (!positionChanged)
    {
      vtkDebugMacro("SetNthHandle: Position did not change");
    }
  }

  // visibility for this handle, hide it if the whole list is invisible,
  // this fid is invisible, or the list isn't visible in this view
  bool fidVisible = true;
  vtkMRMLViewNode *viewNode = this->GetMRMLViewNode();
  if (this->Is2DDisplayableManager())
  {
    if (displayNode->GetVisibility() == 0 ||
      markupsNode->GetNthFiducialVisibility(n) == 0 ||
      !this->IsWidgetDisplayableOnSlice(markupsNode, n))
    {
      fidVisible = false;
    }
  }
  else
  {
    if ((viewNode && displayNode->GetVisibility(viewNode->GetID()) == 0) ||
      displayNode->GetVisibility() == 0 ||
      markupsNode->GetNthFiducialVisibility(n) == 0)
    {
      fidVisible = false;
    }
  }

  // might be in lightbox mode where using a 2d point handle
  vtkPointHandleRepresentation2D *pointHandleRep =
    vtkPointHandleRepresentation2D::SafeDownCast(markupsRepresentation->GetHandleRepresentation(n));
  if (pointHandleRep)
  {
    // set the glyph type - TBD, swapping isn't working
    // set the color
    if (markupsNode->GetNthFiducialSelected(n))
    {
      // use the selected color
      pointHandleRep->GetProperty()->SetColor(displayNode->GetSelectedColor());
    }
    else
    {
      // use the unselected color
      pointHandleRep->GetProperty()->SetColor(displayNode->GetColor());
    }
    // update visibility and enabled (if the point handle is still enabled
    // while invisible, mousing near it will show it)
    pointHandleRep->SetVisibility(fidVisible);
    markupsWidget->GetHandle(n)->SetEnabled(fidVisible);
    return;
  }

  // can have a 3d or 2d handle depending on if in light box mode or not
  vtkOrientedPolygonalHandleRepresentation3D *handleRep =
    vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(markupsRepresentation->GetHandleRepresentation(n));

  if (!handleRep)
  {
    vtkErrorMacro("Failed to get an oriented polygonal handle rep for n = " << n << ", number of seeds = "
      << markupsRepresentation->GetNumberOfHandles() << ", handle rep = "
      << (markupsRepresentation->GetHandleRepresentation(n) ? markupsRepresentation->GetHandleRepresentation(n)->GetClassName() : "null"));
    return;
  }

  // update the text
  std::string textString = markupsNode->GetNthFiducialLabel(n);
  if (textString.compare(handleRep->GetLabelText()) != 0)
  {
    handleRep->SetLabelText(textString.c_str());
  }

  // set the glyph type if a new handle was created, or the glyph type changed
  int oldGlyphType = this->Helper->GetNodeGlyphType(displayNode, n);
  if (createdNewHandle || oldGlyphType != displayNode->GetGlyphType())
  {
    this->SetNthHandleGlyphType(n, displayNode, handleRep);
  }

  // set color and material properties
  vtkProperty *prop = NULL;
  prop = handleRep->GetProperty();
  if (prop)
  {
    double* color = markupsNode->GetNthFiducialSelected(n) ?
      displayNode->GetSelectedColor() : displayNode->GetColor();
    prop->SetColor(color);
    prop->SetOpacity(displayNode->GetOpacity());
    prop->SetAmbient(displayNode->GetAmbient());
    prop->SetDiffuse(displayNode->GetDiffuse());
    prop->SetSpecular(displayNode->GetSpecular());
  }

  // set the scaling
  // the following is only needed since we require a different uniform scale depending on 2D and 3D
  if (this->Is2DDisplayableManager())
  {
    handleRep->SetUniformScale(displayNode->GetGlyphScale()*this->GetScaleFactor2D());
  }
  else
  {
    handleRep->SetUniformScale(displayNode->GetGlyphScale());
  }

  // update the text display properties if there is text
  if (textString.compare("") != 0)
  {
    // scale the text
    double textscale[3] = { displayNode->GetTextScale(), displayNode->GetTextScale(), displayNode->GetTextScale() };
    if (this->Is2DDisplayableManager())
    {
      // scale it down for the 2d windows
      textscale[0] *= this->GetScaleFactor2D();
      textscale[1] *= this->GetScaleFactor2D();
      textscale[2] *= this->GetScaleFactor2D();
    }
    handleRep->SetLabelTextScale(textscale);
    // set the text colors
    if (handleRep->GetLabelTextActor())
    {
      double *color = (markupsNode->GetNthFiducialSelected(n) ?
        displayNode->GetSelectedColor() : displayNode->GetColor());
      handleRep->GetLabelTextActor()->GetProperty()->SetColor(color);
      handleRep->GetLabelTextActor()->GetProperty()->SetOpacity(displayNode->GetOpacity());
    }
  }
  // set the text and handle visibility
  if (fidVisible)
  {
    handleRep->VisibilityOn();
    handleRep->HandleVisibilityOn();
    handleRep->EnablePicking();
    if (textString.compare("") != 0)
    {
      handleRep->LabelVisibilityOn();
    }
    markupsWidget->GetHandle(n)->EnabledOn();

    if (this->Is2DDisplayableManager())
    {
      // if the fiducial is visible, turn off projection
      vtkMarkupsWidget* fiducialSeed = vtkMarkupsWidget::SafeDownCast(this->Helper->GetPointProjectionWidget(markupsNode->GetNthMarkupID(n)));
      if (fiducialSeed && fiducialSeed->GetHandle(0))
      {
        fiducialSeed->GetHandle(0)->Off();
      }
    }

  }
  else
  {
    handleRep->VisibilityOff();
    handleRep->HandleVisibilityOff();
    handleRep->LabelVisibilityOff();
    handleRep->DisablePicking();
    markupsWidget->GetHandle(n)->EnabledOff();
    if (this->Is2DDisplayableManager())
    {
      vtkMarkupsRepresentation *markupsRepresentation = vtkMarkupsRepresentation::SafeDownCast(markupsWidget->GetRepresentation());
      if (markupsRepresentation)
      {
        vtkOrientedPolygonalHandleRepresentation3D *orientedHandleRep =
          vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(
          markupsRepresentation->GetHandleRepresentation());
        if (orientedHandleRep)
        {
          orientedHandleRep->DisablePicking();
        }
      }

      // if the widget is not shown on the slice, show the intersection
      if (markupsNode &&
        markupsNode->GetDisplayNode())
      {
        double transformedP1[4];
        markupsNode->GetNthFiducialWorldCoordinates(n, transformedP1);

        double displayP1[4];
        this->GetWorldToDisplayCoordinates(transformedP1, displayP1);

        vtkMarkupsPointListWidget* projectionSeed =
          vtkMarkupsPointListWidget::SafeDownCast(this->Helper->GetPointProjectionWidget(markupsNode->GetNthMarkupID(n)));

        vtkMRMLMarkupsDisplayNode* pointDisplayNode = markupsNode->GetMarkupsDisplayNode();

        if ((pointDisplayNode->GetSliceProjection() & pointDisplayNode->ProjectionOn) &&
          pointDisplayNode->GetVisibility())
        {
          double glyphScale = pointDisplayNode->GetGlyphScale()*2.0;
          int glyphType = pointDisplayNode->GetGlyphType();
          if (glyphType == vtkMRMLMarkupsDisplayNode::Sphere3D)
          {
            // 3D Sphere glyph is represented in 2D by a Circle2D glyph
            glyphType = vtkMRMLMarkupsDisplayNode::Circle2D;
          }

          double projectionOpacity = pointDisplayNode->GetSliceProjectionOpacity();
          double projectionColor[3];

          if (pointDisplayNode->GetSliceProjectionUseFiducialColor())
          {
            if (markupsNode->GetNthMarkupSelected(n))
            {
              pointDisplayNode->GetSelectedColor(projectionColor);
            }
            else
            {
              pointDisplayNode->GetColor(projectionColor);
            }
          }
          else
          {
            pointDisplayNode->GetSliceProjectionColor(projectionColor);
          }

          if (!projectionSeed)
          {
            vtkNew<vtkMarkupsGlyphSource2D> glyph;
            glyph->SetGlyphType(glyphType);
            glyph->SetScale(glyphScale);
            glyph->SetColor(projectionColor);

            vtkNew<vtkPointHandleRepresentation2D> handle;
            handle->SetCursorShape(glyph->GetOutput());

            vtkNew<vtkMarkupsPointListRepresentation> rep;
            rep->SetHandleRepresentation(handle.GetPointer());

            projectionSeed = vtkMarkupsPointListWidget::New();
            projectionSeed->CreateDefaultRepresentation();
            projectionSeed->SetRepresentation(rep.GetPointer());
            projectionSeed->SetInteractor(this->GetInteractor());
            projectionSeed->SetCurrentRenderer(this->GetRenderer());
            projectionSeed->AddHandle(NULL, displayP1);
            projectionSeed->ProcessEventsOff();
            projectionSeed->ManagesCursorOff();
            projectionSeed->On();
            this->Helper->WidgetPointProjections[markupsNode->GetNthMarkupID(n)] = projectionSeed;
          }

          vtkMarkupsRepresentation* projectionSeedRep =
            vtkMarkupsRepresentation::SafeDownCast(projectionSeed->GetRepresentation());

          if (projectionSeedRep)
          {
            projectionSeed->Off();

            if (projectionSeed->GetHandle(0))
            {
              vtkPointHandleRepresentation2D* handleRep =
                vtkPointHandleRepresentation2D::SafeDownCast(projectionSeed->GetHandle(0)->GetRepresentation());
              if (!handleRep)
              {
                vtkWarningMacro("Must create new handle for projecting seed " << n);
              }

              if (handleRep)
              {
                vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
                glyphSource->SetGlyphType(glyphType);
                glyphSource->SetScale(glyphScale);
                glyphSource->SetScale2(glyphScale);

                if (pointDisplayNode->GetSliceProjectionOutlinedBehindSlicePlane())
                {
                  static const double threshold = 0.5;
                  static const double inPlaneOpacity = 1.0;
                  if (displayP1[2] < 0)
                  {
                    glyphSource->FilledOff();
                    if (displayP1[2] > -threshold)
                    {
                      projectionOpacity = inPlaneOpacity;
                    }
                  }
                  else if (displayP1[2] > 0)
                  {
                    glyphSource->FilledOn();
                    if (displayP1[2] < threshold)
                    {
                      projectionOpacity = inPlaneOpacity;
                    }
                  }
                }
                else
                {
                  glyphSource->FilledOn();
                }
                glyphSource->SetColor(projectionColor);
                handleRep->GetProperty()->SetColor(projectionColor);
                handleRep->GetProperty()->SetOpacity(projectionOpacity);
                // call update to update the points array and avoid a
                // null pointer crash
                glyphSource->Update();
                handleRep->SetCursorShape(glyphSource->GetOutput());
                handleRep->SetDisplayPosition(displayP1);
                projectionSeed->On();
              }
            }
          }
        }
        else
        {
          if (projectionSeed)
          {
            projectionSeed->Off();
          }
        }
      }


      // update locked
      int listLocked = markupsNode->GetLocked();
      int seedLocked = markupsNode->GetNthMarkupLocked(n);
      // if the user is placing lots of fiducials at once, add this one as locked
      // so that it can't be moved when placing the next fiducials. They will be
      // unlocked when the interaction node goes back into ViewTransform
      int persistentPlaceMode = 0;
      vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
      if (interactionNode)
      {
        persistentPlaceMode =
          (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
          && (interactionNode->GetPlaceModePersistence() == 1);
      }
      vtkHandleWidget *handleWidget = markupsWidget->GetHandle(n);
      if (listLocked || persistentPlaceMode)
      {
        handleWidget->ProcessEventsOff();
      }
      else
      {
        handleWidget->ProcessEventsOn();
        handleWidget->SetEnableTranslation(!seedLocked);
      }
    }
  }
}

//---------------------------------------------------------------------------
/// Propagate properties of MRML node to widget.
void vtkMRMLMarkupsFiducialDisplayableManager::PropagateMRMLToWidget(vtkMRMLMarkupsNode* node, vtkMarkupsWidget * markupsWidget)
{
  if (!markupsWidget)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("PropagateMRMLToWidget: MRML node was null!")
    return;
    }

  if (this->Is2DDisplayableManager())
    {
    if (markupsWidget->GetWidgetState() != vtkMarkupsWidget::MovingHandle)
      {
      // ignore events not caused by seed movement
      // return;
      // std::cout << "2D: Seed widget state is not moving: state = " << markupsWidget->GetWidgetState() << " != " << vtkMarkupsWidget::MovingSeed << std::endl;
      }
    }

  // cast to the specific mrml node
  vtkMRMLMarkupsFiducialNode* markupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(node);

  if (!markupsNode)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Could not get fiducial node!")
    return;
    }

  // disable processing of modified events
  this->Updating = 1;

  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
  vtkMRMLMarkupsDisplayNode *displayNode = markupsNode->GetMarkupsDisplayNode();

  if (!displayNode)
    {
    vtkDebugMacro("PropagateMRMLToWidget: Could not get display node for node " << (markupsNode->GetID() ? markupsNode->GetID() : "null id"));
    }

  vtkMarkupsRepresentation * markupsRepresentation = vtkMarkupsRepresentation::SafeDownCast(markupsWidget->GetRepresentation());
  if (!markupsRepresentation)
    {
    vtkErrorMacro("Unable to get the handle representation!");
    return;
    }
  // can have a 3d or 2d handle depending on if in light box mode or not
  vtkOrientedPolygonalHandleRepresentation3D *handleRep =
    vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(markupsRepresentation->GetHandleRepresentation());
  // might be in lightbox mode where using a 2d point handle
  vtkPointHandleRepresentation2D *pointHandleRep =
    vtkPointHandleRepresentation2D::SafeDownCast(markupsRepresentation->GetHandleRepresentation());

  if (this->Is2DDisplayableManager())
    {
    // double check that if switch in and out of light box mode, the handle rep
    // is updated
    bool updateHandleType = false;
    if (this->IsInLightboxMode() && handleRep)
      {
      vtkDebugMacro("PropagateMRMLToWidget: have a 3d handle representation in 2d light box, resetting it.");
      vtkNew<vtkPointHandleRepresentation2D> handle;
      markupsRepresentation->SetHandleRepresentation(handle.GetPointer());
      updateHandleType = true;
      handleRep = NULL;
      pointHandleRep = vtkPointHandleRepresentation2D::SafeDownCast(markupsRepresentation->GetHandleRepresentation());
      }
    else if (!this->IsInLightboxMode() && pointHandleRep)
      {
      vtkDebugMacro("PropagateMRMLToWidget: Not in light box, but have a point handle.");
      vtkNew<vtkOrientedPolygonalHandleRepresentation3D> handle;
      // default to a sphere glyph, update in propagate mrml to widget
      vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
      glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::Sphere3D);
      glyphSource->Update();
      glyphSource->SetScale(1.0);
      glyphSource->SetScale2(1.0);
      handle->SetHandle(glyphSource->GetOutput());
      markupsRepresentation->SetHandleRepresentation(handle.GetPointer());
      updateHandleType = true;
      pointHandleRep = NULL;
      handleRep = vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(markupsRepresentation->GetHandleRepresentation());
      }
    if (updateHandleType)
      {
      vtkDebugMacro("WARNING: updated the handle type");
      markupsWidget->CreateDefaultRepresentation();
      // need to reset any old handles
      int numSeeds = markupsRepresentation->GetNumberOfHandles();
      // remove seeds from the end of the list
      for (int n = numSeeds - 1; n >= 0; --n)
        {
        markupsWidget->RemoveHandle(n);
        }
      // set nth seed will recreate the handles
      }
    }

  // iterate over the fiducials in this markup
  int numberOfFiducials = markupsNode->GetNumberOfMarkups();

  if (this->Is2DDisplayableManager())
    {
    if (numberOfFiducials == 0)
      {
      if (handleRep)
        {
        handleRep->DisablePicking();
        }
      int handleIndex = 0;
      vtkHandleWidget *handleWidget;
      while ((handleWidget = markupsWidget->GetHandle(handleIndex)))
        {
        vtkHandleRepresentation *handleRepresentation = handleWidget->GetHandleRepresentation();
        if (handleRepresentation)
          {
          vtkOrientedPolygonalHandleRepresentation3D *orientedHandleRep =
            vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(handleRepresentation);
          if (orientedHandleRep)
            {
            orientedHandleRep->DisablePicking();
            }
          }
        handleIndex++;
        }
      }
    else
      {
      if (handleRep)
        {
        handleRep->EnablePicking();
        }
      }
    }

  for (int n = 0; n < numberOfFiducials; n++)
    {
    // std::cout << "Fids PropagateMRMLToWidget: n = " << n << std::endl;
    this->SetNthHandle(n, markupsNode, markupsWidget);
    }

  // now update the position of all the handles - done in SetNthHandle now
  //this->UpdatePosition(widget, node);

  // update lock status
  this->Helper->UpdateLocked(node, this->GetInteractionNode());

  // update visibility of widget as a whole
  // std::cout << "PropagateMRMLToWidget: calling UpdateWidgetVisibility" << std::endl;
  this->UpdateWidgetVisibility(node);

  markupsRepresentation->NeedToRenderOn();
  markupsWidget->Modified();

//  markupsWidget->CompleteInteraction();

  // enable processing of modified events
  this->Updating = 0;
}

//---------------------------------------------------------------------------
/// Propagate properties of widget to MRML node.
void vtkMRMLMarkupsFiducialDisplayableManager::PropagateWidgetToMRML(vtkMarkupsWidget * markupsWidget, vtkMRMLMarkupsNode* markupsNode)
{
  if (!markupsNode)
    {
    vtkErrorMacro("PropagateWidgetToMRML: MRML node was null!");
    return;
    }

  if (!markupsWidget)
  {
    vtkErrorMacro("PropagateWidgetToMRML: Could not get markups widget!");
    return;
  }

  if (markupsWidget->GetWidgetState() != vtkMarkupsWidget::MovingHandle)
  {
    // only process markup move events here
    return;
  }

  // disable processing of modified events
  this->Updating = 1;

  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
  vtkMarkupsRepresentation * markupsRepresentation = vtkMarkupsRepresentation::SafeDownCast(markupsWidget->GetRepresentation());
  int numberOfHandles = markupsRepresentation->GetNumberOfHandles();

  bool atLeastOnePositionChanged = false;
  for (int n = 0; n < numberOfHandles; n++)
    {
    double newCoords[4] = { 0, 0, 0, 1};
    if (this->Is2DDisplayableManager())
      {
      double displayCoordinates[3] = {0, 0, 0};
      markupsRepresentation->GetHandleDisplayPosition(n, displayCoordinates);
      this->GetDisplayToWorldCoordinates(displayCoordinates, newCoords);
      }
    else
      {
      markupsRepresentation->GetHandleWorldPosition(n, newCoords);
      }

    // was there a change?
    double currentCoords[4] = { 0, 0, 0, 1 };
    markupsNode->GetMarkupPointWorld(n, 0, currentCoords);
    if (this->GetWorldCoordinatesChanged(currentCoords, newCoords))
      {
      atLeastOnePositionChanged = true;
      markupsNode->SetMarkupPointWorld(n, 0, newCoords[0], newCoords[1], newCoords[2]);
      }
    }

  // did any of the positions change?
  if (atLeastOnePositionChanged)
    {
    markupsNode->Modified();
    markupsNode->GetScene()->InvokeEvent(vtkMRMLMarkupsNode::PointModifiedEvent, markupsNode);
    }
  // This displayableManager should now consider ModifiedEvent again
  this->Updating = 0;
}

//---------------------------------------------------------------------------
/// Create a markupsMRMLnode
void vtkMRMLMarkupsFiducialDisplayableManager::OnClickInRenderWindow(double x, double y, const char *associatedNodeID)
{
  if (!this->IsCorrectDisplayableManager())
    {
    // jump out
    vtkDebugMacro("OnClickInRenderWindow: x = " << x << ", y = " << y << ", incorrect displayable manager, focus = " << this->Focus << ", jumping out");
    return;
    }

  // place the handle where the user clicked
  vtkDebugMacro("OnClickInRenderWindow: placing handle at " << x << ", " << y);
  // switch to updating state to avoid events mess
  this->Updating = 1;

  double displayCoordinates1[2];
  displayCoordinates1[0] = x;
  displayCoordinates1[1] = y;

  double worldCoordinates1[4];

  this->GetDisplayToWorldCoordinates(displayCoordinates1,worldCoordinates1);

  // Is there an active markups node that's a fiducial node?
  vtkMRMLMarkupsFiducialNode *activeMarkupsNode = NULL;

  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (selectionNode)
    {
    const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
    if (mrmlNode &&
        mrmlNode->IsA("vtkMRMLMarkupsFiducialNode"))
      {
      activeMarkupsNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(mrmlNode);
      }
    else
      {
      vtkDebugMacro("OnClickInRenderWindow: active markup id = "
            << (activeMarkupsID ? activeMarkupsID : "null")
            << ", mrml node is "
            << (mrmlNode ? mrmlNode->GetID() : "null")
            << ", not a vtkMRMLMarkupsFiducialNode");
      }
    }

  bool newNode = false;
  if (!activeMarkupsNode)
    {
    newNode = true;
    // create the MRML node
    activeMarkupsNode = vtkMRMLMarkupsFiducialNode::New();
    activeMarkupsNode->SetName("F");
    }

  // add a fiducial: this will trigger an update of the widgets
  int fiducialIndex = activeMarkupsNode->AddPointWorldToNewMarkup(vtkVector3d(worldCoordinates1));
  if (fiducialIndex == -1)
    {
    vtkErrorMacro("OnClickInRenderWindow: unable to add a fiducial to active fiducial list!");
    if (newNode)
      {
      activeMarkupsNode->Delete();
      }
    return;
    }
  // std::cout << "OnClickInRenderWindow: Setting " << fiducialIndex << "th fiducial label from " << activeFiducialNode->GetNthFiducialLabel(fiducialIndex);

  // reset updating state
  this->Updating = 0;

  // if this was a one time place, go back to view transform mode
  vtkMRMLInteractionNode *interactionNode = this->GetInteractionNode();
  if (interactionNode && interactionNode->GetPlaceModePersistence() != 1)
    {
    vtkDebugMacro("End of one time place, place mode persistence = " << interactionNode->GetPlaceModePersistence());
    interactionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::ViewTransform);
    }

  // save for undo and add the node to the scene after any reset of the
  // interaction node so that don't end up back in place mode
  this->GetMRMLScene()->SaveStateForUndo();

  // is there a node associated with this?
  if (associatedNodeID)
    {
    activeMarkupsNode->SetNthFiducialAssociatedNodeID(fiducialIndex, associatedNodeID);
    }

  if (newNode)
    {
    // create a display node and add node and display node to scene
    vtkMRMLMarkupsDisplayNode *displayNode = vtkMRMLMarkupsDisplayNode::New();
    this->GetMRMLScene()->AddNode(displayNode);
    // let the logic know that it needs to set it to defaults
    displayNode->InvokeEvent(vtkMRMLMarkupsDisplayNode::ResetToDefaultsEvent);

    activeMarkupsNode->AddAndObserveDisplayNodeID(displayNode->GetID());
    this->GetMRMLScene()->AddNode(activeMarkupsNode);

    // have to reset the fid id since the fiducial node generates a scene
    // unique id only if the node was in the scene when the point was added
    if (!activeMarkupsNode->ResetNthMarkupID(0))
      {
      vtkWarningMacro("Failed to reset the unique ID on the first fiducial in a new list: " << activeMarkupsNode->GetNthMarkupID(0));
      }

    // save it as the active markups list
    if (selectionNode)
      {
      selectionNode->SetActivePlaceNodeID(activeMarkupsNode->GetID());
      }
    // clean up
    displayNode->Delete();
    activeMarkupsNode->Delete();
    }
}

//---------------------------------------------------------------------------
/// observe key press events
void vtkMRMLMarkupsFiducialDisplayableManager::AdditionnalInitializeStep()
{
  // don't add the key press event, as it triggers a crash on start up
  //vtkDebugMacro("Adding an observer on the key press event");
  this->AddInteractorStyleObservableEvent(vtkCommand::KeyPressEvent);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager::OnInteractorStyleEvent(int eventid)
{
  this->Superclass::OnInteractorStyleEvent(eventid);

  if (this->GetDisableInteractorStyleEventsProcessing())
    {
    vtkWarningMacro("OnInteractorStyleEvent: Processing of events was disabled.")
    return;
    }

  if (eventid == vtkCommand::KeyPressEvent)
    {
    char *keySym = this->GetInteractor()->GetKeySym();
    vtkDebugMacro("OnInteractorStyleEvent: key press event position = "
              << this->GetInteractor()->GetEventPosition()[0] << ", "
              << this->GetInteractor()->GetEventPosition()[1]
              << ", key sym = " << (keySym == NULL ? "null" : keySym));
    if (!keySym)
      {
      return;
      }
    if (strcmp(keySym, "p") == 0)
      {
      if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
        {
        this->OnClickInRenderWindowGetCoordinates();
        }
      else
        {
        vtkDebugMacro("Fiducial DisplayableManager: key press p, but not in Place mode! Returning.");
        return;
        }
      }
    }
  else if (eventid == vtkCommand::KeyReleaseEvent)
    {
    vtkDebugMacro("Got a key release event");
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager::UpdatePosition(vtkMarkupsWidget *widget, vtkMRMLMarkupsNode *markupsNode)
{
//  vtkWarningMacro("UpdatePosition, node is " << (node == NULL ? "null" : node->GetID()));
  if (!markupsNode)
    {
    vtkErrorMacro("UpdatePosition - invalid node");
    return;
    }
  // get the widget
  if (!widget)
    {
    vtkErrorMacro("UpdatePosition: no widget associated with points node " << markupsNode->GetID());
    return;
    }

  vtkMarkupsWidget* markupsWidget = vtkMarkupsWidget::SafeDownCast(widget);
  if (!markupsWidget)
   {
   vtkErrorMacro("UpdatePosition: Could not get markups widget!")
   return;
   }

  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
  bool positionChanged = false;
  int numberOfFiducials = markupsNode->GetNumberOfMarkups();
  for (int n = 0; n < numberOfFiducials; n++)
    {
    if (this->UpdateNthHandlePositionFromMRML(n, markupsWidget, markupsNode))
      {
      positionChanged = true;
      }
    }
  // did any of the positions change?
  if (positionChanged && this->Updating == 0)
    {
    // not already updating from propagate mrml to widget, so trigger a render
    vtkMarkupsRepresentation * markupsRepresentation = vtkMarkupsRepresentation::SafeDownCast(markupsWidget->GetRepresentation());
    markupsRepresentation->NeedToRenderOn();
    markupsWidget->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager::OnMRMLSceneEndClose()
{
  // make sure to delete widgets and projections
  this->Superclass::OnMRMLSceneEndClose();

  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager::OnMRMLMarkupsNodeNthMarkupModifiedEvent(vtkMRMLMarkupsNode* node, int n)
{
  int numberOfMarkups = node->GetNumberOfMarkups();
  if (n < 0 || n >= numberOfMarkups)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: n = " << n << " is out of range 0-" << numberOfMarkups);
    return;
    }

  vtkMarkupsWidget *widget = this->Helper->GetWidget(node);
  if (!widget)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: a markup was added to a node that doesn't already have a widget! Returning..");
    return;
    }

  vtkMarkupsWidget* markupsWidget = vtkMarkupsWidget::SafeDownCast(widget);
  if (!markupsWidget)
   {
   vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: Could not get markups widget!")
   return;
   }
  this->SetNthHandle(n, vtkMRMLMarkupsFiducialNode::SafeDownCast(node), markupsWidget);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager::OnMRMLMarkupsNodeMarkupAddedEvent(vtkMRMLMarkupsNode * markupsNode, int n)
{
  vtkDebugMacro("OnMRMLMarkupsNodeMarkupAddedEvent");

  if (!markupsNode)
    {
    return;
    }
  vtkMarkupsWidget *widget = this->Helper->GetWidget(markupsNode);
  if (!widget)
    {
    // TBD: create a widget?
    vtkErrorMacro("OnMRMLMarkupsNodeMarkupAddedEvent: a markup was added to a node that doesn't already have a widget! Returning..");
    return;
    }
  if (n < 0)
    {
    // batch update, recreate the widget
    this->Helper->RemoveWidgetAndNode(markupsNode);
    this->AddWidget(markupsNode);
    return;
    }

  vtkMarkupsWidget* markupsWidget = vtkMarkupsWidget::SafeDownCast(widget);
  if (!markupsWidget)
   {
   vtkErrorMacro("OnMRMLMarkupsNodeMarkupAddedEvent: Could not get markups widget!")
   return;
   }

  // this call will create a new handle and set it
  // std::cout << "OnMRMLMarkupsNodeMarkupAddedEvent: adding to markups node that currently has " << markupsNode->GetNumberOfMarkups() << std::endl;
  this->SetNthHandle(n, vtkMRMLMarkupsFiducialNode::SafeDownCast(markupsNode), markupsWidget);

  vtkMarkupsRepresentation * markupsRepresentation = vtkMarkupsRepresentation::SafeDownCast(markupsWidget->GetRepresentation());
  markupsRepresentation->NeedToRenderOn();
  markupsWidget->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsFiducialDisplayableManager::OnMRMLMarkupsNodeMarkupRemovedEvent(vtkMRMLMarkupsNode * markupsNode, int vtkNotUsed(n))
{
  vtkDebugMacro("OnMRMLMarkupsNodeMarkupRemovedEvent");

  if (!markupsNode)
    {
    return;
    }
  vtkMarkupsWidget *widget = this->Helper->GetWidget(markupsNode);
  if (!widget)
    {
    // TBD: create a widget?
    vtkErrorMacro("OnMRMLMarkupsNodeMarkupRemovedEvent: a markup was removed from a node that doesn't already have a widget! Returning..");
    return;
    }

  // for now, recreate the widget
  this->Helper->RemoveWidgetAndNode(markupsNode);
  this->AddWidget(markupsNode);
}
