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
#include <vtkMRMLMarkupsAngleNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsAngleDisplayableManager2D.h"

// MarkupsModule/VTKWidgets includes
#include <vtkMarkupsGlyphSource2D.h>

// MRMLDisplayableManager includes
#include <vtkSliceViewInteractorStyle.h>

// MRML includes
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>

// VTK includes
#include <vtkAbstractWidget.h>
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
#include <vtkAngleWidget.h>
#include <vtkAngleRepresentation2D.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsAngleDisplayableManager2D);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsAngleDisplayableManager2D Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsAngleWidgetCallback2D : public vtkCommand
{
public:
  static vtkMarkupsAngleWidgetCallback2D *New()
  { return new vtkMarkupsAngleWidgetCallback2D; }

  vtkMarkupsAngleWidgetCallback2D()
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

    //
    // mark the Node with an attribute to indicate if it is currently being interacted with
    // so that other code can respond to changes only when it is not moving
    // Markups.MovingInSliceView will be set to the layout name of
    // our slice node while it is being actively manipulated
    vtkAngleWidget *widget = vtkAngleWidget::SafeDownCast(this->Widget);
    if (widget && this->DisplayableManager && this->Node)
      {
      vtkMRMLSliceNode *sliceNode = this->DisplayableManager->GetMRMLSliceNode();
      if (sliceNode)
        {
        int modifiedWasDisabled = this->Node->GetDisableModifiedEvent();
        this->Node->DisableModifiedEventOn();
        if (widget->GetWidgetState() == vtkAngleWidget::MovingSeed)
          {
          this->Node->SetAttribute("Markups.MovingInSliceView", sliceNode->GetLayoutName());
          std::ostringstream seedNumber;
          int *n =  reinterpret_cast<int *>(callData);
          if (n != NULL)
            {
            seedNumber << *n;
            }
          else
            {
            seedNumber << "-1";
            }
          this->Node->SetAttribute("Markups.MovingMarkupIndex", seedNumber.str().c_str());
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

    // check which event it is
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
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      // save the state of the node when done moving
      if (this->Node->GetScene())
        {
        this->Node->GetScene()->SaveStateForUndo(this->Node);
        }
      if (callData)
        {
        // Most of the time vtkCommand::EndInteractionEvent does not provide
        // seed index, but in case we get a value then update the markup index.
        this->LastInteractionEventMarkupIndex = *(reinterpret_cast<int *>(callData));
        }
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointEndInteractionEvent, &this->LastInteractionEventMarkupIndex);
      if (!this->PointMovedSinceStartInteraction)
        {
        this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointClickedEvent, &this->LastInteractionEventMarkupIndex);
        }
      this->LastInteractionEventMarkupIndex = -1;
      }
    else if (event == vtkCommand::InteractionEvent)
      {
      // restrict the widget to the renderer

      // we need the widgetRepresentation
      vtkAngleRepresentation2D * representation = vtkAngleRepresentation2D::SafeDownCast(this->Widget->GetRepresentation());
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
      else if (representation->GetNumberOfSeeds() == 1)
        {
        n = 0;
        }
      if ( n >= 0 )
        {
        // have a single seed that moved
        // first, we get the current displayCoordinates of the points
        double displayCoordinates1[4] = { 0, 0, 0, 1 };
        representation->GetSeedDisplayPosition(n, displayCoordinates1);

        // second, we copy these to restrictedDisplayCoordinates
        double restrictedDisplayCoordinates1[4] = {displayCoordinates1[0], displayCoordinates1[1], displayCoordinates1[2], displayCoordinates1[3]};

        // modify restrictedDisplayCoordinates 1 and 2, if these are outside the viewport of the current renderer
        bool changed = this->DisplayableManager->RestrictDisplayCoordinatesToViewport(restrictedDisplayCoordinates1);

        // only if we had to restrict the coordinates aka. if the coordinates changed, we update the positions
        if (changed ||
            this->DisplayableManager->GetDisplayCoordinatesChanged(displayCoordinates1,restrictedDisplayCoordinates1))
          {
          representation->SetSeedDisplayPosition(n,restrictedDisplayCoordinates1);
          }

        // propagate the changes to MRML
        this->DisplayableManager->UpdateNthMarkupPositionFromWidget(n, this->Node, this->Widget);
        this->PointMovedSinceStartInteraction = true;
        }
      else
        {
        std::cout << "Had an interaction event without the seed index!" << std::endl;
        }
      }
  }

  void SetWidget(vtkAbstractWidget *w)
    {
    this->Widget = w;
    }
  void SetNode(vtkMRMLMarkupsNode *n)
    {
    this->Node = n;
    }
  void SetDisplayableManager(vtkMRMLMarkupsDisplayableManager2D * dm)
    {
    this->DisplayableManager = dm;
    }

  vtkAbstractWidget * Widget;
  vtkMRMLMarkupsNode * Node;
  vtkMRMLMarkupsDisplayableManager2D * DisplayableManager;
  int LastInteractionEventMarkupIndex;
  bool PointMovedSinceStartInteraction;
};

//---------------------------------------------------------------------------
// vtkMRMLMarkupsAngleDisplayableManager2D methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
/// Create a new seed widget.
vtkAbstractWidget * vtkMRMLMarkupsAngleDisplayableManager2D::CreateWidget(vtkMRMLMarkupsNode* node)
{

  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return 0;
    }

  // 2d glyphs and text need to be scaled by 1/60 to show up properly in the 2d slice windows
  this->SetScaleFactor2D(0.01667);

  vtkMRMLMarkupsAngleNode* angleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(node);

  if (!angleNode)
    {
    vtkErrorMacro("CreateWidget: Could not get fiducial node!")
    return 0;
    }

  vtkMRMLMarkupsDisplayNode *displayNode = angleNode->GetMarkupsDisplayNode();

  if (!displayNode)
    {
    // std::cout<<"No DisplayNode!"<<std::endl;
    }

  // unset the glyph type which can be necessary when recreating a widget due to 2d/3d swap
  int oldGlyphType = this->Helper->GetNodeGlyphType(displayNode, 0);
  if (oldGlyphType != -1)
    {
    vtkDebugMacro("CreateWidget: found a glyph type already defined for this node: " << oldGlyphType);
    this->Helper->SetNodeGlyphType(displayNode, vtkMRMLMarkupsDisplayNode::GlyphMin - 1, 0);
    }

  vtkNew<vtkAngleRepresentation2D> rep;

  vtkDebugMacro("making handle for angleNode " << angleNode->GetName());
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

  //seed widget
  vtkAngleWidget * angleWidget = vtkAngleWidget::New();
  angleWidget->CreateDefaultRepresentation();

  angleWidget->SetRepresentation(rep.GetPointer());

  if (this->GetInteractor()->GetPickingManager())
    {
    if (!(this->GetInteractor()->GetPickingManager()->GetEnabled()))
      {
      // if the picking manager is not already turned on for this
      // interactor, enable it
      this->GetInteractor()->GetPickingManager()->EnabledOn();
      }
    }

  angleWidget->SetInteractor(this->GetInteractor());
  // set the renderer on the widget and representation
  if (!this->IsInLightboxMode())
    {
    angleWidget->SetCurrentRenderer(this->GetRenderer());
    angleWidget->GetRepresentation()->SetRenderer(this->GetRenderer());
    }
  else
    {
    int lightboxIndex = this->GetLightboxIndex(angleNode, 0,0);
    angleWidget->SetCurrentRenderer(this->GetRenderer(lightboxIndex));
    angleWidget->GetRepresentation()->SetRenderer(this->GetRenderer(lightboxIndex));
    }

  vtkDebugMacro("Fids CreateWidget: Created widget for node " << angleNode->GetID() << " with a representation");

  angleWidget->CompleteInteraction();

  return angleWidget;

  }

//---------------------------------------------------------------------------
/// Tear down the widget creation
void vtkMRMLMarkupsAngleDisplayableManager2D::OnWidgetCreated(vtkAbstractWidget * widget, vtkMRMLMarkupsNode * node)
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
  vtkMarkupsAngleWidgetCallback2D *myCallback = vtkMarkupsAngleWidgetCallback2D::New();
  myCallback->SetNode(node);
  myCallback->SetWidget(widget);
  myCallback->SetDisplayableManager(this);
  widget->AddObserver(vtkCommand::StartInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::EndInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::InteractionEvent,myCallback);
  myCallback->Delete();

}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsAngleDisplayableManager2D::UpdateNthMarkupPositionFromWidget(int n, vtkMRMLMarkupsNode* pointsNode, vtkAbstractWidget * widget)
{
//  std::cout << "UpdateNthMarkupPositionFromWidget: n = " << n << std::endl;

  if (!pointsNode || !widget)
    {
    return false;
    }
  if (n > pointsNode->GetNumberOfMarkups())
    {
    return false;
    }
  vtkAngleWidget *angleWidget = vtkAngleWidget::SafeDownCast(widget);
  if (!angleWidget)
    {
    return false;
    }
  vtkAngleRepresentation2D * angleRepresentation = vtkAngleRepresentation2D::SafeDownCast(angleWidget->GetRepresentation());
  if (!angleRepresentation)
    {
    return false;
    }

  bool positionChanged = false;

  // for 2d managers, compare the display positions
  double displayCoordinates1[4];
  double displayCoordinatesBuffer1[4];

  // get point in world coordinates using parent transforms
  double pointTransformed[4];
  // always only one point in a fiducial
  pointsNode->GetMarkupPointWorld(n, 0, pointTransformed);

  this->GetWorldToDisplayCoordinates(pointTransformed,displayCoordinates1);

  angleRepresentation->GetSeedDisplayPosition(n,displayCoordinatesBuffer1);

  if (this->GetDisplayCoordinatesChanged(displayCoordinates1,displayCoordinatesBuffer1))
    {
    positionChanged = true;
    double worldCoordinates[4] = {0.0, 0.0, 0.0, 0.0};
    this->GetDisplayToWorldCoordinates(displayCoordinatesBuffer1, worldCoordinates);
    pointsNode->SetMarkupPointWorld(n, 0, worldCoordinates[0], worldCoordinates[1], worldCoordinates[2]);
    }

  return positionChanged;
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsAngleDisplayableManager2D::UpdateNthSeedPositionFromMRML(int n, vtkAbstractWidget *widget, vtkMRMLMarkupsNode *pointsNode)
{
//  vtkWarningMacro("UpdateNthSeedPositionFromMRML: n = " << n);

  if (!pointsNode || !widget)
    {
    return false;
    }
  if (n > pointsNode->GetNumberOfMarkups())
    {
    return false;
    }
  vtkAngleWidget *angleWidget = vtkAngleWidget::SafeDownCast(widget);
  if (!angleWidget)
    {
    return false;
    }
  vtkAngleRepresentation2D * angleRepresentation = vtkAngleRepresentation2D::SafeDownCast(angleWidget->GetRepresentation());
  if (!angleRepresentation)
    {
    return false;
    }
  bool positionChanged = false;

//  std::cout << "UpdateNthSeedPositionFromMRML: n = " << n << std::endl;

  // for 2d managers, compare the display positions
  double displayCoordinates1[4];
  double displayCoordinatesBuffer1[4];

  // get point in world coordinates using parent transforms
  double pointTransformed[4];
  // always only one point in a fiducial
  pointsNode->GetMarkupPointWorld(n, 0, pointTransformed);

  this->GetWorldToDisplayCoordinates(pointTransformed,displayCoordinates1);

  angleRepresentation->GetSeedDisplayPosition(n,displayCoordinatesBuffer1);

  if (this->GetDisplayCoordinatesChanged(displayCoordinates1,displayCoordinatesBuffer1))
    {
    // only update when really changed
    vtkDebugMacro("UpdateNthSeedPositionFromMRML: " << n << ": "
                  << this->GetMRMLSliceNode()->GetName()
                  << ": display coordinates changed:\n\tseed display = "
                  << displayCoordinatesBuffer1[0] << ", " << displayCoordinatesBuffer1[1]
                  << "\n\tfid display =  " << displayCoordinates1[0] << ", " << displayCoordinates1[1] );
    if (angleRepresentation->GetRenderer() != NULL &&
        angleRepresentation->GetRenderer()->IsActiveCameraCreated())
      {
      angleRepresentation->SetSeedDisplayPosition(n,displayCoordinates1);
      positionChanged = true;
      }
    else
      {
      vtkDebugMacro("UpdateNthSeedPositionFromMRML: " <<
                    "No active camera on seed representation, " <<
                    "delaying updating position");
      }
    }
  else
    {
    vtkDebugMacro("UpdateNthSeedPositionFromMRML: " <<  this->GetMRMLSliceNode()->GetName() << ": display coordinates unchanged!");
    }
  return positionChanged;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::SetNthSeed(int n, vtkMRMLMarkupsAngleNode* angleNode, vtkAngleWidget *angleWidget)
{
  if (!angleWidget->GetRepresentation())
    {
    vtkErrorMacro("SetNthSeed: no representation in seed widget!");
    return;
    }
  vtkAngleRepresentation2D * angleRepresentation = vtkAngleRepresentation2D::SafeDownCast(angleWidget->GetRepresentation());

  if (!angleRepresentation)
    {
    vtkErrorMacro("SetNthSeed: no seed representation in widget!");
    return;
    }

  vtkMRMLMarkupsDisplayNode *displayNode = angleNode->GetMarkupsDisplayNode();
  if (!displayNode)
    {
    vtkDebugMacro("SetNthSeed: Could not get display node for node " << (angleNode->GetID() ? angleNode->GetID() : "null id"));
    return;
    }

  int numberOfHandles = angleRepresentation->GetNumberOfSeeds();
  vtkDebugMacro("SetNthSeed, n = " << n << ", number of handles = " << numberOfHandles);

  // does this handle need to be created?
  bool createdNewHandle = false;
  if (n >= numberOfHandles)
    {
    // create a new handle
    vtkHandleWidget* newhandle = angleWidget->CreateNewHandle();
    if (!newhandle)
      {
      vtkErrorMacro("SetNthSeed: error creaing a new handle!");
      }
    else
      {
      // std::cout << "SetNthSeed: created a new handle,number of seeds = " << angleRepresentation->GetNumberOfSeeds() << std::endl;
      createdNewHandle = true;
      newhandle->ManagesCursorOff();
      if (newhandle->GetEnabled() == 0)
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
          vtkDebugMacro("SetNthSeed: no active camera, delaying enabling the handle");
          }
        }
      }
    }

  // can have a 3d or 2d handle depending on if in light box mode or not
  vtkOrientedPolygonalHandleRepresentation3D *handleRep =
    vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(angleRepresentation->GetHandleRepresentation(n));
  // might be in lightbox mode where using a 2d point handle
  vtkPointHandleRepresentation2D *pointHandleRep =
    vtkPointHandleRepresentation2D::SafeDownCast(angleRepresentation->GetHandleRepresentation(n));

  // update the postion
  bool positionChanged = this->UpdateNthSeedPositionFromMRML(n, angleWidget, angleNode);
  if (!positionChanged)
    {
    vtkDebugMacro("SetNthSeed: Position did not change");
    }

  if (!handleRep && !pointHandleRep)
    {
    vtkErrorMacro("Failed to get a handle rep for n = " << n
              << ", number of seeds = "
              <<  angleRepresentation->GetNumberOfSeeds()
              << ", handle rep = "
              << (angleRepresentation->GetHandleRepresentation(n) ? angleRepresentation->GetHandleRepresentation(n)->GetClassName() : "null"));
    return;
    }

  // visibility for this handle, hide it if the whole list is invisible,
  // this fid is invisible, or the fid isn't visible on this slice
  bool fidVisible = true;
  if (displayNode->GetVisibility() == 0 ||
      angleNode->GetNthFiducialVisibility(n) == 0 ||
      !this->IsWidgetDisplayableOnSlice(angleNode, n))
    {
    fidVisible = false;
    }

  if (handleRep)
    {
    // set the glyph type if a new handle was created, or the glyph type changed
    int oldGlyphType = this->Helper->GetNodeGlyphType(displayNode, n);
    if (createdNewHandle ||
        oldGlyphType != displayNode->GetGlyphType())
      {
      vtkDebugMacro("DisplayNode glyph type = " << displayNode->GetGlyphType()
            << " = " << displayNode->GetGlyphTypeAsString()
            << ", is 3d glyph = "
            << (displayNode->GlyphTypeIs3D() ? "true" : "false")
            << ", is 2d disp manager.");
      if (displayNode->GlyphTypeIs3D())
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
        }//if (displayNode->GlyphTypeIs3D())
      else
        {
        // 2D
        vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
        glyphSource->SetGlyphType(displayNode->GetGlyphType());
        glyphSource->Update();
        glyphSource->SetScale(1.0);
        handleRep->SetHandle(glyphSource->GetOutput());
        }
      // TBD: keep with the assumption of one glyph type per markups node,
      // that each seed has to have the same type, but update if necessary
      this->Helper->SetNodeGlyphType(displayNode, displayNode->GetGlyphType(), n);
      }  // end of glyph type

    // set the color
    vtkProperty *prop = NULL;
    prop = handleRep->GetProperty();
    if (prop)
      {
      if (angleNode->GetNthFiducialSelected(n))
        {
        // use the selected color
        prop->SetColor( displayNode->GetSelectedColor());
        }
      else
        {
        // use the unselected color
        prop->SetColor(displayNode->GetColor());
        }

      // set the material properties
      prop->SetOpacity(displayNode->GetOpacity());
      prop->SetAmbient(displayNode->GetAmbient());
      prop->SetDiffuse(displayNode->GetDiffuse());
      prop->SetSpecular(displayNode->GetSpecular());
      }

    // set the scaling
    // the following is only needed since we require a different uniform scale depending on 2D and 3D
    handleRep->SetUniformScale(displayNode->GetGlyphScale()*this->GetScaleFactor2D());

    /// if the text is visible
    std::string textString = angleNode->GetNthFiducialLabel(n);
    // update the text string

    if (textString.compare(handleRep->GetLabelText()) != 0)
      {
      handleRep->SetLabelText(textString.c_str());
      }
    // scale the text
    if (textString.compare("") != 0)
      {
      // scale the text
      double textscale[3] = {displayNode->GetTextScale(), displayNode->GetTextScale(), displayNode->GetTextScale()};

      // scale it down for the 2d windows
      textscale[0] *= this->GetScaleFactor2D();
      textscale[1] *= this->GetScaleFactor2D();
      textscale[2] *= this->GetScaleFactor2D();

      handleRep->SetLabelTextScale(textscale);
      }
    // set the text colors
    if (handleRep->GetLabelTextActor())
      {
      // set the colours
      if (angleNode->GetNthFiducialSelected(n))
        {
        handleRep->GetLabelTextActor()->GetProperty()->SetColor(displayNode->GetSelectedColor());
        }
      else
        {
        handleRep->GetLabelTextActor()->GetProperty()->SetColor(displayNode->GetColor());
        }
      handleRep->GetLabelTextActor()->GetProperty()->SetOpacity(displayNode->GetOpacity());
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
      angleWidget->GetSeed(n)->EnabledOn();
      // if the fiducial is visible, turn off projection
      vtkAngleWidget* fiducialSeed = vtkAngleWidget::SafeDownCast(this->Helper->GetPointProjectionWidget(angleNode->GetNthMarkupID(n)));
      if (fiducialSeed && fiducialSeed->GetSeed(0))
        {
        fiducialSeed->GetSeed(0)->Off();
        }
      }
    else
      {
      handleRep->VisibilityOff();
      handleRep->HandleVisibilityOff();
      handleRep->LabelVisibilityOff();
      handleRep->DisablePicking();
      vtkAngleRepresentation2D *angleRepresentation = vtkAngleRepresentation2D::SafeDownCast(angleWidget->GetRepresentation());
      if (angleRepresentation)
        {
        vtkOrientedPolygonalHandleRepresentation3D *orientedHandleRep =
          vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(
            angleRepresentation->GetHandleRepresentation());
        if (orientedHandleRep)
          {
          orientedHandleRep->DisablePicking();
          }
        }

      // if the widget is not shown on the slice, show the intersection
      if (angleNode &&
          angleNode->GetDisplayNode())
        {
        double transformedP1[4];
        angleNode->GetNthFiducialWorldCoordinates(n, transformedP1);

        double displayP1[4];
        this->GetWorldToDisplayCoordinates(transformedP1, displayP1);

        vtkAngleWidget* projectionSeed =
          vtkAngleWidget::SafeDownCast(this->Helper->GetPointProjectionWidget(angleNode->GetNthMarkupID(n)));

        vtkMRMLMarkupsDisplayNode* pointDisplayNode = angleNode->GetMarkupsDisplayNode();

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
            if (angleNode->GetNthMarkupSelected(n))
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

            vtkNew<vtkAngleRepresentation2D> rep;
            rep->SetHandleRepresentation(handle.GetPointer());

            projectionSeed = vtkAngleWidget::New();
            projectionSeed->CreateDefaultRepresentation();
            projectionSeed->SetRepresentation(rep.GetPointer());
            projectionSeed->SetInteractor(this->GetInteractor());
            projectionSeed->SetCurrentRenderer(this->GetRenderer());
            projectionSeed->CreateNewHandle();
            projectionSeed->ProcessEventsOff();
            projectionSeed->ManagesCursorOff();
            projectionSeed->On();
            projectionSeed->CompleteInteraction();
            this->Helper->WidgetPointProjections[angleNode->GetNthMarkupID(n)] = projectionSeed;
            }

          vtkAngleRepresentation2D* projectionSeedRep =
            vtkAngleRepresentation2D::SafeDownCast(projectionSeed->GetRepresentation());

          if (projectionSeedRep)
            {
            projectionSeed->Off();

            if (projectionSeed->GetSeed(0))
              {
              vtkPointHandleRepresentation2D* handleRep =
                vtkPointHandleRepresentation2D::SafeDownCast(projectionSeed->GetSeed(0)->GetRepresentation());
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
                projectionSeed->CompleteInteraction();
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
      }
    // update locked
    int listLocked = angleNode->GetLocked();
    int seedLocked = angleNode->GetNthMarkupLocked(n);
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
    vtkHandleWidget *seed = angleWidget->GetSeed(n);
    if (listLocked || persistentPlaceMode)
      {
      seed->ProcessEventsOff();
      }
    else
      {
      seed->ProcessEventsOn();
      seed->SetEnableTranslation(!seedLocked);
      }
    }
  else if (pointHandleRep)
    {
    // set the glyph type - TBD, swapping isn't working
    // set the color
    if (angleNode->GetNthFiducialSelected(n))
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
    angleWidget->GetSeed(n)->SetEnabled(fidVisible);
    }
}

//---------------------------------------------------------------------------
/// Propagate properties of MRML node to widget.
void vtkMRMLMarkupsAngleDisplayableManager2D::PropagateMRMLToWidget(vtkMRMLMarkupsNode* node, vtkAbstractWidget * widget)
{

  if (!widget)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("PropagateMRMLToWidget: MRML node was null!")
    return;
    }

  // cast to the specific widget
  vtkAngleWidget* angleWidget = vtkAngleWidget::SafeDownCast(widget);

  if (!angleWidget)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Could not get seed widget!")
    return;
    }

  if (angleWidget->GetWidgetState() != vtkAngleWidget::MovingSeed)
    {
    // ignore events not caused by seed movement
    // return;
    // std::cout << "2D: Seed widget state is not moving: state = " << angleWidget->GetWidgetState() << " != " << vtkAngleWidget::MovingSeed << std::endl;
    }

  // cast to the specific mrml node
  vtkMRMLMarkupsAngleNode* angleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(node);

  if (!angleNode)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Could not get fiducial node!")
    return;
    }

  // disable processing of modified events
  this->Updating = 1;


  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes


  vtkMRMLMarkupsDisplayNode *displayNode = angleNode->GetMarkupsDisplayNode();

  if (!displayNode)
    {
    vtkDebugMacro("PropagateMRMLToWidget: Could not get display node for node " << (angleNode->GetID() ? angleNode->GetID() : "null id"));
    }

  vtkAngleRepresentation2D * angleRepresentation = vtkAngleRepresentation2D::SafeDownCast(angleWidget->GetRepresentation());
  if (!angleRepresentation)
    {
    vtkErrorMacro("Unable to get the seed representation!");
    return;
    }

  // can have a 3d or 2d handle depending on if in light box mode or not
  vtkOrientedPolygonalHandleRepresentation3D *handleRep =
    vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(angleRepresentation->GetHandleRepresentation());
  // might be in lightbox mode where using a 2d point handle
  vtkPointHandleRepresentation2D *pointHandleRep =
    vtkPointHandleRepresentation2D::SafeDownCast(angleRepresentation->GetHandleRepresentation());
  // double check that if switch in and out of light box mode, the handle rep
  // is updated
  bool updateHandleType = false;
  if (this->IsInLightboxMode())
    {
    if (handleRep)
      {
      vtkDebugMacro("PropagateMRMLToWidget: have a 3d handle representation in 2d light box, resetting it.");
      vtkNew<vtkPointHandleRepresentation2D> handle;
      angleRepresentation->SetHandleRepresentation(handle.GetPointer());
      updateHandleType = true;
      handleRep = NULL;
      pointHandleRep =
        vtkPointHandleRepresentation2D::SafeDownCast(angleRepresentation->GetHandleRepresentation());
      }
    }
  else
    {
    if (pointHandleRep)
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
      angleRepresentation->SetHandleRepresentation(handle.GetPointer());
      updateHandleType = true;
      pointHandleRep = NULL;
      handleRep =
        vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(angleRepresentation->GetHandleRepresentation());
      }
    }
  if (updateHandleType)
    {
    vtkDebugMacro("WARNING: updated the handle type");
    angleWidget->CreateDefaultRepresentation();
    // need to reset any old handles
    int numSeeds = angleRepresentation->GetNumberOfSeeds();
    // remove seeds from the end of the list
    for (int n = numSeeds - 1; n >= 0; --n)
      {
      angleWidget->DeleteSeed(n);
      }
    // set nth seed will recreate the handles
    }

  // iterate over the fiducials in this markup
  int numberOfFiducials = angleNode->GetNumberOfMarkups();

  vtkDebugMacro("Fids PropagateMRMLToWidget, node num markups = " << numberOfFiducials);

  if (numberOfFiducials == 0)
    {
    if (handleRep)
      {
      handleRep->DisablePicking();
      }
    int seed = 0;
    vtkHandleWidget *handleWidget;
    while ( (handleWidget = angleWidget->GetSeed(seed)) )
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
      seed++;
      }
    }
  else
    {
    if (handleRep)
      {
      handleRep->EnablePicking();
      }
    }

  for (int n = 0; n < numberOfFiducials; n++)
    {
    // std::cout << "Fids PropagateMRMLToWidget: n = " << n << std::endl;
    this->SetNthSeed(n, angleNode, angleWidget);
    }


  // now update the position of all the seeds - done in SetNthSeed now
  //this->UpdatePosition(widget, node);

  // update lock status
  this->Helper->UpdateLocked(node, this->GetInteractionNode());

  // update visibility of widget as a whole
  // std::cout << "PropagateMRMLToWidget: calling UpdateWidgetVisibility" << std::endl;
  this->UpdateWidgetVisibility(node);

  if (angleRepresentation)
    {
    angleRepresentation->NeedToRenderOn();
    }
  angleWidget->Modified();

//  angleWidget->CompleteInteraction();

  // enable processing of modified events
  this->Updating = 0;


}

//---------------------------------------------------------------------------
/// Propagate properties of widget to MRML node.
void vtkMRMLMarkupsAngleDisplayableManager2D::PropagateWidgetToMRML(vtkAbstractWidget * widget, vtkMRMLMarkupsNode* node)
{
  if (!widget)
    {
    vtkErrorMacro("PropagateWidgetToMRML: Widget was null!")
    return;
    }

  if (!node)
    {
    vtkErrorMacro("PropagateWidgetToMRML: MRML node was null!")
    return;
    }

  // cast to the specific widget
  vtkAngleWidget* angleWidget = vtkAngleWidget::SafeDownCast(widget);

  if (!angleWidget)
   {
   vtkErrorMacro("PropagateWidgetToMRML: Could not get seed widget!")
   return;
   }

  // cast to the specific mrml node
  vtkMRMLMarkupsAngleNode* angleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(node);

  if (!angleNode)
   {
   vtkErrorMacro("PropagateWidgetToMRML: Could not get fiducial node!")
   return;
   }

  // disable processing of modified events
  this->Updating = 1;
  // this was stopping PointModifiedEvent from being invoked, need that to
  // update the GUI
//  int disabledModify = angleNode->StartModify();


  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
  vtkAngleRepresentation2D * angleRepresentation = vtkAngleRepresentation2D::SafeDownCast(angleWidget->GetRepresentation());
  int numberOfSeeds = angleRepresentation->GetNumberOfSeeds();

  bool atLeastOnePositionChanged = false;
  for (int n = 0; n < numberOfSeeds; n++)
    {
    double worldCoordinates1[4];
    bool thisPositionChanged = false;
    // 2D widget was changed

    double displayCoordinates1[4];
    angleRepresentation->GetSeedDisplayPosition(n,displayCoordinates1);
    vtkDebugMacro("PropagateWidgetToMRML: 2d DM: widget display coords = "
          << displayCoordinates1[0] << ", " << displayCoordinates1[1]
          << ", " << displayCoordinates1[2]);
    this->GetDisplayToWorldCoordinates(displayCoordinates1,worldCoordinates1);
    vtkDebugMacro("PropagateWidgetToMRML: 2d: widget seed " << n
          << " world coords = "
          << worldCoordinates1[0] << ", " << worldCoordinates1[1]
          << ", "<< worldCoordinates1[2]);

    // was there a change?
    double currentCoordinates[4];
    angleNode->GetNthFiducialWorldCoordinates(n,currentCoordinates);
    vtkDebugMacro("PropagateWidgetToMRML: fiducial " << n
          << " current world coordinates = "
          << currentCoordinates[0] << ", " << currentCoordinates[1]
          << ", " << currentCoordinates[2]);

    double currentCoords[3];
    currentCoords[0] = currentCoordinates[0];
    currentCoords[1] = currentCoordinates[1];
    currentCoords[2] = currentCoordinates[2];
    double newCoords[3];
    newCoords[0] = worldCoordinates1[0];
    newCoords[1] = worldCoordinates1[1];
    newCoords[2] = worldCoordinates1[2];
    if (this->GetWorldCoordinatesChanged(currentCoords, newCoords))
      {
      vtkDebugMacro("PropagateWidgetToMRML: position changed.");
      atLeastOnePositionChanged = true;
      thisPositionChanged = true;
      }

    if (thisPositionChanged)
      {
      vtkDebugMacro("PropagateWidgetToMRML: this position changed, setting fiducial coordinates");
      angleNode->SetNthFiducialWorldCoordinates(n,worldCoordinates1);
      }
    }

  // enable processing of modified events
//  angleNode->EndModify(disabledModify);

  // did any of the positions change?
  if (atLeastOnePositionChanged)
    {
    vtkDebugMacro("PropagateWidgetToMRML: position changed, calling point modified on the fiducial node");
    angleNode->Modified();
    //angleNode->GetScene()->InvokeEvent(vtkCommand::ModifiedEvent,angleNode);
    angleNode->GetScene()->InvokeEvent(vtkMRMLMarkupsNode::PointModifiedEvent,angleNode);
    }
  // This displayableManager should now consider ModifiedEvent again
  this->Updating = 0;
}

//---------------------------------------------------------------------------
/// Create a markupsMRMLnode
void vtkMRMLMarkupsAngleDisplayableManager2D::OnClickInRenderWindow(double x, double y, const char *associatedNodeID)
{
  if (!this->IsCorrectDisplayableManager())
    {
    // jump out
    vtkDebugMacro("OnClickInRenderWindow: x = " << x << ", y = " << y << ", incorrect displayable manager, focus = " << this->Focus << ", jumping out");
    return;
    }

  // place the seed where the user clicked
  vtkDebugMacro("OnClickInRenderWindow: placing seed at " << x << ", " << y);
  // switch to updating state to avoid events mess
  this->Updating = 1;

  double displayCoordinates1[2];
  displayCoordinates1[0] = x;
  displayCoordinates1[1] = y;


  double worldCoordinates1[4];

  this->GetDisplayToWorldCoordinates(displayCoordinates1,worldCoordinates1);

  // Is there an active markups node that's a fiducial node?
  vtkMRMLMarkupsAngleNode *activeAngleNode = NULL;

  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (selectionNode)
    {
    const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
    if (mrmlNode &&
        mrmlNode->IsA("vtkMRMLMarkupsAngleNode"))
      {
      activeAngleNode = vtkMRMLMarkupsAngleNode::SafeDownCast(mrmlNode);
      }
    else
      {
      vtkDebugMacro("OnClickInRenderWindow: active markup id = "
            << (activeMarkupsID ? activeMarkupsID : "null")
            << ", mrml node is "
            << (mrmlNode ? mrmlNode->GetID() : "null")
            << ", not a vtkMRMLMarkupsAngleNode");
      }
    }

  bool newNode = false;
  if (!activeAngleNode)
    {
    newNode = true;
    // create the MRML node
    activeAngleNode = vtkMRMLMarkupsAngleNode::New();
    activeAngleNode->SetName("F");
    }

  // add a fiducial: this will trigger an update of the widgets
  int fiducialIndex = activeAngleNode->AddPointWorldToNewMarkup(vtkVector3d(worldCoordinates1));
  if (fiducialIndex == -1)
    {
    vtkErrorMacro("OnClickInRenderWindow: unable to add a fiducial to active fiducial list!");
    if (newNode)
      {
      activeAngleNode->Delete();
      }
    return;
    }
  // std::cout << "OnClickInRenderWindow: Setting " << fiducialIndex << "th fiducial label from " << activeAngleNode->GetNthFiducialLabel(fiducialIndex);

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
    activeAngleNode->SetNthFiducialAssociatedNodeID(fiducialIndex, associatedNodeID);
    }

  if (newNode)
    {
    // create a display node and add node and display node to scene
    vtkMRMLMarkupsDisplayNode *displayNode = vtkMRMLMarkupsDisplayNode::New();
    this->GetMRMLScene()->AddNode(displayNode);
    // let the logic know that it needs to set it to defaults
    displayNode->InvokeEvent(vtkMRMLMarkupsDisplayNode::ResetToDefaultsEvent);

    activeAngleNode->AddAndObserveDisplayNodeID(displayNode->GetID());
    this->GetMRMLScene()->AddNode(activeAngleNode);

    // have to reset the fid id since the fiducial node generates a scene
    // unique id only if the node was in the scene when the point was added
    if (!activeAngleNode->ResetNthMarkupID(0))
      {
      vtkWarningMacro("Failed to reset the unique ID on the first fiducial in a new list: " << activeAngleNode->GetNthMarkupID(0));
      }

    // save it as the active markups list
    if (selectionNode)
      {
      selectionNode->SetActivePlaceNodeID(activeAngleNode->GetID());
      }
    // clean up
    displayNode->Delete();
    activeAngleNode->Delete();
    }
}

//---------------------------------------------------------------------------
/// observe key press events
void vtkMRMLMarkupsAngleDisplayableManager2D::AdditionnalInitializeStep()
{
  // don't add the key press event, as it triggers a crash on start up
  //vtkDebugMacro("Adding an observer on the key press event");
  this->AddInteractorStyleObservableEvent(vtkCommand::KeyPressEvent);
}


//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnInteractorStyleEvent(int eventid)
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
    vtkDebugMacro("OnInteractorStyleEvent 2D: key press event position = "
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
void vtkMRMLMarkupsAngleDisplayableManager2D::UpdatePosition(vtkAbstractWidget *widget, vtkMRMLNode *node)
{
//  vtkWarningMacro("UpdatePosition, node is " << (node == NULL ? "null" : node->GetID()));
  if (!node)
    {
    return;
    }
  vtkMRMLMarkupsNode *pointsNode = vtkMRMLMarkupsNode::SafeDownCast(node);
  if (!pointsNode)
    {
    vtkErrorMacro("UpdatePosition - Can not access control points node from node with id " << node->GetID());
    return;
    }
  // get the widget
  if (!widget)
    {
    vtkErrorMacro("UpdatePosition: no widget associated with points node " << pointsNode->GetID());
    return;
    }
  // cast to a seed widget
  vtkAngleWidget* angleWidget = vtkAngleWidget::SafeDownCast(widget);

  if (!angleWidget)
   {
   vtkErrorMacro("UpdatePosition: Could not get seed widget!")
   return;
   }

  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes


  // disable processing of modified events
  //this->Updating = 1;
  bool positionChanged = false;
  int numberOfFiducials = pointsNode->GetNumberOfMarkups();
  for (int n = 0; n < numberOfFiducials; n++)
    {
    if (this->UpdateNthSeedPositionFromMRML(n, angleWidget, pointsNode))
      {
      positionChanged = true;
      }
    }
  // did any of the positions change?
  if (positionChanged && this->Updating == 0)
    {
    // not already updating from propagate mrml to widget, so trigger a render
    vtkAngleRepresentation2D * angleRepresentation = vtkAngleRepresentation2D::SafeDownCast(angleWidget->GetRepresentation());
    angleRepresentation->NeedToRenderOn();
    angleWidget->Modified();
//    angleWidget->CompleteInteraction();
    }
  // enable processing of modified events
  //this->Updating = 0;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnMRMLSceneEndClose()
{
  // make sure to delete widgets and projections
  this->Superclass::OnMRMLSceneEndClose();

  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnMRMLMarkupsNodeNthMarkupModifiedEvent(vtkMRMLMarkupsNode* node, int n)
{
  int numberOfMarkups = node->GetNumberOfMarkups();
  if (n < 0 || n >= numberOfMarkups)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: n = " << n << " is out of range 0-" << numberOfMarkups);
    return;
    }

  vtkAbstractWidget *widget = this->Helper->GetWidget(node);
  if (!widget)
    {
    vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: a markup was added to a node that doesn't already have a widget! Returning..");
    return;
    }

  vtkAngleWidget* angleWidget = vtkAngleWidget::SafeDownCast(widget);
  if (!angleWidget)
   {
   vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: Could not get seed widget!")
   return;
   }
  this->SetNthSeed(n, vtkMRMLMarkupsAngleNode::SafeDownCast(node), angleWidget);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnMRMLMarkupsNodeMarkupAddedEvent(vtkMRMLMarkupsNode * markupsNode, int n)
{
  vtkDebugMacro("OnMRMLMarkupsNodeMarkupAddedEvent");

  if (!markupsNode)
    {
    return;
    }
  vtkAbstractWidget *widget = this->Helper->GetWidget(markupsNode);
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

  vtkAngleWidget* angleWidget = vtkAngleWidget::SafeDownCast(widget);
  if (!angleWidget)
   {
   vtkErrorMacro("OnMRMLMarkupsNodeMarkupAddedEvent: Could not get seed widget!")
   return;
   }

  // this call will create a new handle and set it
  // std::cout << "OnMRMLMarkupsNodeMarkupAddedEvent: adding to markups node that currently has " << markupsNode->GetNumberOfMarkups() << std::endl;
  this->SetNthSeed(n, vtkMRMLMarkupsAngleNode::SafeDownCast(markupsNode), angleWidget);

  vtkAngleRepresentation2D * angleRepresentation = vtkAngleRepresentation2D::SafeDownCast(angleWidget->GetRepresentation());
  angleRepresentation->NeedToRenderOn();
  angleWidget->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsAngleDisplayableManager2D::OnMRMLMarkupsNodeMarkupRemovedEvent(vtkMRMLMarkupsNode * markupsNode, int vtkNotUsed(n))
{
  vtkDebugMacro("OnMRMLMarkupsNodeMarkupRemovedEvent");

  if (!markupsNode)
    {
    return;
    }
  vtkAbstractWidget *widget = this->Helper->GetWidget(markupsNode);
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
