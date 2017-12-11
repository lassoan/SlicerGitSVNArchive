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
#include <vtkMRMLMarkupsCurveNode.h>
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsCurveDisplayableManager.h"

// MarkupsModule/VTKWidgets includes
#include <vtkMarkupsCurveWidget.h>
#include <vtkMarkupsGlyphSource2D.h>

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
#include <vtkProperty2D.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkMarkupsCurveWidget.h>
#include <vtkSmartPointer.h>
#include <vtkMarkupsSplineRepresentation.h>
#include <vtkSphereSource.h>
#include <vtkSplineWidget2.h>
#include <vtkMarkupsSplineRepresentation.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsCurveDisplayableManager);

//---------------------------------------------------------------------------
// vtkMRMLMarkupsCurveDisplayableManager Callback
/// \ingroup Slicer_QtModules_Markups
class vtkMarkupsCurveWidgetCallback3D : public vtkCommand
{
public:
  static vtkMarkupsCurveWidgetCallback3D *New()
  { return new vtkMarkupsCurveWidgetCallback3D; }

  vtkMarkupsCurveWidgetCallback3D()
    : Widget(NULL)
    , Node(NULL)
    , DisplayableManager(NULL)
    , LastInteractionEventMarkupIndex(-1)
    , PointMovedSinceStartInteraction(false)
  {
  }

  virtual void Execute (vtkObject *vtkNotUsed(caller), unsigned long event, void *callData)
  {
    if (event ==  vtkCommand::PlacePointEvent)
      {
      // std::cout << "Warning: PlacePointEvent not supported" << std::endl;
      }
    else if ((event == vtkCommand::StartInteractionEvent) || (event == vtkCommand::EndInteractionEvent) || (event == vtkCommand::InteractionEvent))
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
      }
    if (event == vtkCommand::StartInteractionEvent)
      {
      // If calldata is NULL, invoking an event may cause a crash (e.g., Python observer
      // tries to dereference the NULL pointer), therefore it's important to always pass a valid pointer
      // and indicate invalidity with value (-1).
      this->LastInteractionEventMarkupIndex = (callData ? *(reinterpret_cast<int *>(callData)) : -1);
      this->PointMovedSinceStartInteraction = false;
      this->Node->InvokeEvent(vtkMRMLMarkupsNode::PointStartInteractionEvent, &this->LastInteractionEventMarkupIndex);
      // no need to propagate to MRML, just notify external observers that the user selected a markup
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
    else if (event == vtkCommand::InteractionEvent)
      {
      this->PointMovedSinceStartInteraction = true;
      }
    // the interaction with the widget ended, now propagate the changes to MRML
    this->DisplayableManager->PropagateWidgetToMRML(this->Widget, this->Node);
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
// vtkMRMLMarkupsCurveDisplayableManager methods

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  //this->Helper->PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
/// Create a new widget.
vtkMarkupsWidget * vtkMRMLMarkupsCurveDisplayableManager::CreateWidget(vtkMRMLMarkupsNode* node)
{
  if (!node)
    {
    vtkErrorMacro("CreateWidget: Node not set!")
    return 0;
    }

  vtkMRMLMarkupsCurveNode* curveNode = vtkMRMLMarkupsCurveNode::SafeDownCast(node);

  if (!curveNode)
    {
    vtkErrorMacro("CreateWidget: Could not get fiducial node!")
    return 0;
    }

  vtkMRMLMarkupsDisplayNode *displayNode = curveNode->GetMarkupsDisplayNode();

  if (!displayNode)
    {
     //std::cout<<"No DisplayNode!"<<std::endl;
    }

  vtkNew<vtkMarkupsSplineRepresentation> rep;
  vtkNew<vtkOrientedPolygonalHandleRepresentation3D> handle;

  // default to a starburst glyph, update in propagate mrml to widget
  vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
  glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::StarBurst2D);
  glyphSource->Update();
  glyphSource->SetScale(1.0);
  handle->SetHandle(glyphSource->GetOutput());

  //rep->SetHandleRepresentation(handle.GetPointer());

  vtkMarkupsCurveWidget * curveWidget = vtkMarkupsCurveWidget::New();
  curveWidget->CreateDefaultRepresentation();

  curveWidget->SetRepresentation(rep.GetPointer());

  if (this->GetInteractor()->GetPickingManager())
  {
    if (!(this->GetInteractor()->GetPickingManager()->GetEnabled()))
    {
      // Managed picking is on by default on the curve widget, but the interactor
      // will need to have it's picking manager turned on once curve widgets are
      // going to be used to avoid dragging handles that are behind others.
      // Enabling it before setting the interactor on the curve widget seems to
      // work better with tests of two fiducial lists.
      this->GetInteractor()->GetPickingManager()->EnabledOn();
    }
  }
  curveWidget->SetInteractor(this->GetInteractor());
  curveWidget->SetCurrentRenderer(this->GetRenderer());

  vtkDebugMacro("Fids CreateWidget: Created widget for node " << curveNode->GetID() << " with a representation");

  return curveWidget;

  }

//---------------------------------------------------------------------------
/// Tear down the widget creation
void vtkMRMLMarkupsCurveDisplayableManager::OnWidgetCreated(vtkMarkupsWidget * widget, vtkMRMLMarkupsNode * node)
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
  vtkMarkupsCurveWidgetCallback3D *myCallback = vtkMarkupsCurveWidgetCallback3D::New();
  myCallback->SetNode(node);
  myCallback->SetWidget(widget);
  myCallback->SetDisplayableManager(this);
  widget->AddObserver(vtkCommand::StartInteractionEvent,myCallback);
  widget->AddObserver(vtkCommand::EndInteractionEvent, myCallback);
  widget->AddObserver(vtkCommand::InteractionEvent, myCallback);
  myCallback->Delete();
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsCurveDisplayableManager::UpdateNthHandlePositionFromMRML(int n, vtkMarkupsWidget *widget, vtkMRMLMarkupsNode *pointsNode)
{
  if (!pointsNode || !widget)
    {
    return false;
    }
  if (n > pointsNode->GetNumberOfMarkups())
    {
    return false;
    }
  vtkMarkupsCurveWidget *curveWidget = vtkMarkupsCurveWidget::SafeDownCast(widget);
  if (!curveWidget)
    {
    return false;
    }
  vtkMarkupsSplineRepresentation * splineRepresentation = vtkMarkupsSplineRepresentation::SafeDownCast(curveWidget->GetRepresentation());
  if (!splineRepresentation)
    {
    return false;
    }
  bool positionChanged = false;

  // transform fiducial point using parent transforms
  double fidWorldCoord[4];
  pointsNode->GetMarkupPointWorld(n, 0, fidWorldCoord);

  // for 3d managers, compare world positions
  double handleWorldCoord[4];
  splineRepresentation->GetHandlePosition(n, handleWorldCoord);

  if (this->GetWorldCoordinatesChanged(handleWorldCoord, fidWorldCoord))
    {
    vtkDebugMacro("UpdateNthHandlePositionFromMRML: 3D:"
                  << " world coordinates changed:\n\thandle = "
                  << handleWorldCoord[0] << ", "
                  << handleWorldCoord[1] << ", "
                  << handleWorldCoord[2] << "\n\tfid =  "
                  << fidWorldCoord[0] << ", "
                  << fidWorldCoord[1] << ", "
                  << fidWorldCoord[2]);
    splineRepresentation->SetHandlePosition(n, fidWorldCoord);
    positionChanged = true;
    }
  else
    {
    vtkDebugMacro("UpdateNthHandlePositionFromMRML: 3D: world coordinates unchanged!");
    }

  return positionChanged;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveDisplayableManager::SetNthHandle(int n, vtkMRMLMarkupsCurveNode* curveNode, vtkMarkupsCurveWidget *curveWidget)
{
  vtkMarkupsSplineRepresentation * curveRepresentation = vtkMarkupsSplineRepresentation::SafeDownCast(curveWidget->GetRepresentation());

  if (!curveRepresentation)
    {
    vtkErrorMacro("SetNthHandle: no handle representation in widget!");
    return;
    }

  vtkMRMLMarkupsDisplayNode *displayNode = curveNode->GetMarkupsDisplayNode();
  if (!displayNode)
    {
    vtkDebugMacro("SetNthHandle: Could not get display node for node " << (curveNode->GetID() ? curveNode->GetID() : "null id"));
    return;
    }

  int numberOfHandles = curveRepresentation->GetNumberOfHandles();
  vtkDebugMacro("SetNthHandle, n = " << n << ", number of handles = " << numberOfHandles);

  // does this handle need to be created?
  bool createdNewHandle = false;
  if (n >= numberOfHandles)
    {
    // create a new handle
    double fidWorldCoord[4];
    curveNode->GetMarkupPointWorld(n, 0, fidWorldCoord);
    vtkHandleWidget* newhandle = curveWidget->CreateNewHandle(fidWorldCoord);
    if (!newhandle)
      {
      vtkErrorMacro("SetNthHandle: error creaing a new handle!");
      }
    else
      {
      // std::cout << "SetNthHandle: created a new handle,number of handles = " << curveRepresentation->GetNumberOfHandles() << std::endl;
      createdNewHandle = true;
      newhandle->ManagesCursorOff();
      if (newhandle->GetEnabled() == 0)
        {
        vtkDebugMacro("turning on the new handle");
        newhandle->EnabledOn();
        }
      }
    }
  else
  {
    // update the postion
    bool positionChanged = this->UpdateNthHandlePositionFromMRML(n, curveWidget, curveNode);
    if (!positionChanged)
    {
      vtkDebugMacro("Position did not change");
    }
  }

  //vtkOrientedPolygonalHandleRepresentation3D *handleRep =
  //  vtkOrientedPolygonalHandleRepresentation3D::SafeDownCast(curveRepresentation->GetHandleRepresentation(n));
  //if (!handleRep)
  //  {
  //  vtkErrorMacro("Failed to get an oriented polygonal handle rep for n = "
  //        << n << ", number of handles = "
  //        << seedRepresentation->GetNumberOfSeeds()
  //        << ", handle rep = "
  //        << (seedRepresentation->GetHandleRepresentation(n) ? seedRepresentation->GetHandleRepresentation(n)->GetClassName() : "null"));
  //  return;
  //  }

  // update the text
  //std::string textString = curveNode->GetNthFiducialLabel(n);
  //if (textString.compare(handleRep->GetLabelText()) != 0)
  //  {
  //  handleRep->SetLabelText(textString.c_str());
  //  }
  //// visibility for this handle, hide it if the whole list is invisible,
  //// this fid is invisible, or the list isn't visible in this view
  //bool fidVisible = true;
  //vtkMRMLViewNode *viewNode = this->GetMRMLViewNode();
  //if ((viewNode && displayNode->GetVisibility(viewNode->GetID()) == 0) ||
  //    displayNode->GetVisibility() == 0 ||
  //    curveNode->GetNthFiducialVisibility(n) == 0)
  //  {
  //  fidVisible = false;
  //  }
  //if (fidVisible)
  //  {
  //  handleRep->VisibilityOn();
  //  handleRep->HandleVisibilityOn();
  //  handleRep->EnablePicking();
  //  if (textString.compare("") != 0)
  //    {
  //    handleRep->LabelVisibilityOn();
  //    }
  //  curveWidget->GetSeed(n)->EnabledOn();
  //  }
  //else
  //  {
  //  handleRep->VisibilityOff();
  //  handleRep->HandleVisibilityOff();
  //  handleRep->DisablePicking();
  //  handleRep->LabelVisibilityOff();
  //  curveWidget->GetSeed(n)->EnabledOff();
  //  }

  // update locked
  int listLocked = curveNode->GetLocked();
  int handleLocked = curveNode->GetNthMarkupLocked(n);
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
  //vtkHandleWidget *handle = curveWidget->GetHandle(n);
  //if (listLocked || persistentPlaceMode)
  //  {
  //  handle->ProcessEventsOff();
  //  }
  //else
  //  {
  //  handle->ProcessEventsOn();
  //  handle->SetEnableTranslation(!handleLocked);
  //  }

  //// set the glyph type if a new handle was created, or the glyph type changed
  //int oldGlyphType = this->Helper->GetNodeGlyphType(displayNode, n);
  //if (createdNewHandle ||
  //    oldGlyphType != displayNode->GetGlyphType())
  //  {
  //  vtkDebugMacro("3D: DisplayNode glyph type = "
  //        << displayNode->GetGlyphType()
  //        << " = " << displayNode->GetGlyphTypeAsString()
  //        << ", is 3d glyph = "
  //        << (displayNode->GlyphTypeIs3D() ? "true" : "false"));
  //  if (displayNode->GlyphTypeIs3D())
  //    {
  //    if (displayNode->GetGlyphType() == vtkMRMLMarkupsDisplayNode::Sphere3D)
  //      {
  //      // std::cout << "3d sphere" << std::endl;
  //      vtkNew<vtkSphereSource> sphereSource;
  //      sphereSource->SetRadius(0.5);
  //      sphereSource->SetPhiResolution(10);
  //      sphereSource->SetThetaResolution(10);
  //      sphereSource->Update();
  //      handleRep->SetHandle(sphereSource->GetOutput());
  //      }
  //    else
  //      {
  //      // the 3d diamond isn't supported yet, use a 2d diamond for now
  //      vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
  //      glyphSource->SetGlyphType(vtkMRMLMarkupsDisplayNode::Diamond2D);
  //      glyphSource->Update();
  //      glyphSource->SetScale(1.0);
  //      handleRep->SetHandle(glyphSource->GetOutput());
  //      }
  //    }//if (displayNode->GlyphTypeIs3D())
  //  else
  //    {
  //    // 2D
  //    vtkNew<vtkMarkupsGlyphSource2D> glyphSource;
  //    glyphSource->SetGlyphType(displayNode->GetGlyphType());
  //    glyphSource->Update();
  //    glyphSource->SetScale(1.0);
  //    handleRep->SetHandle(glyphSource->GetOutput());
  //    }
  //  // TBD: keep with the assumption of one glyph type per markups node,
  //  // but they may have different glyphs during update
  //  this->Helper->SetNodeGlyphType(displayNode, displayNode->GetGlyphType(), n);
  //  }  // end of glyph type

  //// update the text display properties if there is text
  //if (textString.compare("") != 0)
  //  {
  //  // scale the text
  //  double textscale[3] = {displayNode->GetTextScale(), displayNode->GetTextScale(), displayNode->GetTextScale()};
  //  handleRep->SetLabelTextScale(textscale);
  //  if (handleRep->GetLabelTextActor())
  //    {
  //    // set the colours
  //    if (curveNode->GetNthFiducialSelected(n))
  //      {
  //      double *color = displayNode->GetSelectedColor();
  //      handleRep->GetLabelTextActor()->GetProperty()->SetColor(color);
  //      // std::cout << "Set label text actor property color to selected col " << color[0] << ", " << color[1] << ", " << color[2] << std::endl;
  //      }
  //    else
  //      {
  //      double *color = displayNode->GetColor();
  //      handleRep->GetLabelTextActor()->GetProperty()->SetColor(color);
  //      // std::cout << "Set label text actor property color to col " << color[0] << ", " << color[1] << ", " << color[2] << std::endl;
  //      }

  //    handleRep->GetLabelTextActor()->GetProperty()->SetOpacity(displayNode->GetOpacity());
  //    }
  //  }

  //vtkProperty *prop = NULL;
  //prop = handleRep->GetProperty();
  //if (prop)
  //  {
  //  if (curveNode->GetNthFiducialSelected(n))
  //    {
  //    // use the selected color
  //    double *color = displayNode->GetSelectedColor();
  //    prop->SetColor(color);
  //    // std::cout << "Set glyph property color to selected col " << color[0] << ", " << color[1] << ", " << color[2] << std::endl;
  //    }
  //  else
  //    {
  //    // use the unselected color
  //    double *color = displayNode->GetColor();
  //    prop->SetColor(color);
  //    // std::cout << "Set glyph property color to col " << color[0] << ", " << color[1] << ", " << color[2] << std::endl;
  //    }

  //  // material properties
  //  prop->SetOpacity(displayNode->GetOpacity());
  //  prop->SetAmbient(displayNode->GetAmbient());
  //  prop->SetDiffuse(displayNode->GetDiffuse());
  //  prop->SetSpecular(displayNode->GetSpecular());
  //  }

  //handleRep->SetUniformScale(displayNode->GetGlyphScale());
}

//---------------------------------------------------------------------------
/// Propagate properties of MRML node to widget.
void vtkMRMLMarkupsCurveDisplayableManager::PropagateMRMLToWidget(vtkMRMLMarkupsNode* node, vtkMarkupsWidget * widget)
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
  vtkMarkupsCurveWidget* curveWidget = vtkMarkupsCurveWidget::SafeDownCast(widget);

  if (!curveWidget)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Could not get curve widget")
    return;
    }

  // cast to the specific mrml node
  vtkMRMLMarkupsCurveNode* curveNode = vtkMRMLMarkupsCurveNode::SafeDownCast(node);

  if (!curveNode)
    {
    vtkErrorMacro("PropagateMRMLToWidget: Could not get fiducial node!")
    return;
    }

  // disable processing of modified events
  this->Updating = 1;

  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
  vtkMRMLMarkupsDisplayNode *displayNode = curveNode->GetMarkupsDisplayNode();

  if (!displayNode)
    {
    vtkDebugMacro("PropagateMRMLToWidget: Could not get display node for node " << (curveNode->GetID() ? curveNode->GetID() : "null id"));
    }

  // iterate over the fiducials in this markup
  int numberOfFiducials = curveNode->GetNumberOfMarkups();

  vtkDebugMacro("Fids PropagateMRMLToWidget, node num markups = " << numberOfFiducials);

  for (int n = 0; n < numberOfFiducials; n++)
    {
    // std::cout << "Fids PropagateMRMLToWidget: n = " << n << std::endl;
    this->SetNthHandle(n, curveNode, curveWidget);
    }

  // update lock status
  this->Helper->UpdateLocked(node, this->GetInteractionNode());

  // update visibility of widget as a whole
  // std::cout << "PropagateMRMLToWidget: calling UpdateWidgetVisibility" << std::endl;
  this->UpdateWidgetVisibility(node);

  vtkMarkupsSplineRepresentation * curveRepresentation = vtkMarkupsSplineRepresentation::SafeDownCast(curveWidget->GetRepresentation());
  curveRepresentation->NeedToRenderOn();
  curveWidget->Modified();

  // enable processing of modified events
  this->Updating = 0;
}

//---------------------------------------------------------------------------
/// Propagate properties of widget to MRML node.
void vtkMRMLMarkupsCurveDisplayableManager::PropagateWidgetToMRML(vtkMarkupsWidget * widget, vtkMRMLMarkupsNode* node)
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
  vtkMarkupsCurveWidget* curveWidget = vtkMarkupsCurveWidget::SafeDownCast(widget);

  if (!curveWidget)
   {
   vtkErrorMacro("PropagateWidgetToMRML: Could not get curve widget")
   return;
   }

  if (vtkMarkupsSplineRepresentation::SafeDownCast(curveWidget->GetRepresentation())->GetInteractionState() != vtkMarkupsSplineRepresentation::Moving)
    {
    // ignore events not caused by handle movement
    return;
    }

  // cast to the specific mrml node
  vtkMRMLMarkupsCurveNode* curveNode = vtkMRMLMarkupsCurveNode::SafeDownCast(node);

  if (!curveNode)
   {
   vtkErrorMacro("PropagateWidgetToMRML: Could not get fiducial node!")
   return;
   }

  // disable processing of modified events
  this->Updating = 1;

  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
  vtkMarkupsSplineRepresentation * curveRepresentation = vtkMarkupsSplineRepresentation::SafeDownCast(curveWidget->GetRepresentation());
  int numberOfHandles = curveRepresentation->GetNumberOfHandles();

  bool positionChanged = false;
  for (int n = 0; n < numberOfHandles; n++)
    {
    double worldCoordinates1[4];
    curveRepresentation->GetHandlePosition(n,worldCoordinates1);
    vtkDebugMacro("PropagateWidgetToMRML: 3d: widget handle " << n
          << " world coords = " << worldCoordinates1[0] << ", "
          << worldCoordinates1[1] << ", "<< worldCoordinates1[2]);

    // was there a change?
    double currentCoordinates[4];
    curveNode->GetNthFiducialWorldCoordinates(n,currentCoordinates);
    vtkDebugMacro("PropagateWidgetToMRML: fiducial " << n
          << " current world coordinates = " << currentCoordinates[0]
          << ", " << currentCoordinates[1] << ", "
          << currentCoordinates[2]);

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
      positionChanged = true;
      vtkDebugMacro("PropagateWidgetToMRML: position changed, setting fiducial coordinates");
      curveNode->SetNthFiducialWorldCoordinates(n,worldCoordinates1);
      }
    }

  // did any of the positions change?
  if (positionChanged)
    {
    vtkDebugMacro("PropagateWidgetToMRML: position changed, calling point modified on the fiducial node");
    curveNode->Modified();
    curveNode->GetScene()->InvokeEvent(vtkMRMLMarkupsNode::PointModifiedEvent,curveNode);
    }
  // This displayableManager should now consider ModifiedEvent again
  this->Updating = 0;
}

//---------------------------------------------------------------------------
/// Create a markupsMRMLnode
void vtkMRMLMarkupsCurveDisplayableManager::OnClickInRenderWindow(double x, double y, const char *associatedNodeID)
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
  vtkMRMLMarkupsCurveNode *activeCurveNode = NULL;

  vtkMRMLSelectionNode *selectionNode = this->GetSelectionNode();
  if (selectionNode)
    {
    const char *activeMarkupsID = selectionNode->GetActivePlaceNodeID();
    vtkMRMLNode *mrmlNode = this->GetMRMLScene()->GetNodeByID(activeMarkupsID);
    if (mrmlNode &&
        mrmlNode->IsA("vtkMRMLMarkupsCurveNode"))
      {
      activeCurveNode = vtkMRMLMarkupsCurveNode::SafeDownCast(mrmlNode);
      }
    else
      {
      vtkDebugMacro("OnClickInRenderWindow: active markup id = "
            << (activeMarkupsID ? activeMarkupsID : "null")
            << ", mrml node is "
            << (mrmlNode ? mrmlNode->GetID() : "null")
            << ", not a vtkMRMLMarkupsCurveNode");
      }
    }

  bool newNode = false;
  if (!activeCurveNode)
    {
    newNode = true;
    // create the MRML node
    activeCurveNode = vtkMRMLMarkupsCurveNode::New();
    activeCurveNode->SetName("C");
    }

  // add a fiducial: this will trigger an update of the widgets
  int fiducialIndex = activeCurveNode->AddPointWorldToNewMarkup(vtkVector3d(worldCoordinates1));
  if (fiducialIndex == -1)
    {
    vtkErrorMacro("OnClickInRenderWindow: unable to add a fiducial to active fiducial list!");
    if (newNode)
      {
        activeCurveNode->Delete();
      }
    return;
    }
  // std::cout << "OnClickInRenderWindow: Setting " << fiducialIndex << "th fiducial label from " << activecurveNode->GetNthFiducialLabel(fiducialIndex);

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
      activeCurveNode->SetNthFiducialAssociatedNodeID(fiducialIndex, associatedNodeID);
    }

  if (newNode)
    {
    // create a display node and add node and display node to scene
    vtkMRMLMarkupsDisplayNode *displayNode = vtkMRMLMarkupsDisplayNode::New();
    this->GetMRMLScene()->AddNode(displayNode);
    // let the logic know that it needs to set it to defaults
    displayNode->InvokeEvent(vtkMRMLMarkupsDisplayNode::ResetToDefaultsEvent);

    activeCurveNode->AddAndObserveDisplayNodeID(displayNode->GetID());
    this->GetMRMLScene()->AddNode(activeCurveNode);

    // have to reset the fid id since the fiducial node generates a scene
    // unique id only if the node was in the scene when the point was added
    if (!activeCurveNode->ResetNthMarkupID(0))
      {
      vtkWarningMacro("Failed to reset the unique ID on the first fiducial in a new list: " << activeCurveNode->GetNthMarkupID(0));
      }

    // save it as the active markups list
    if (selectionNode)
      {
        selectionNode->SetActivePlaceNodeID(activeCurveNode->GetID());
      }
    // clean up
    displayNode->Delete();
    activeCurveNode->Delete();
    }
}

//---------------------------------------------------------------------------
/// observe key press events
void vtkMRMLMarkupsCurveDisplayableManager::AdditionnalInitializeStep()
{
  // don't add the key press event, as it triggers a crash on start up
  //vtkDebugMacro("Adding an observer on the key press event");
  this->AddInteractorStyleObservableEvent(vtkCommand::KeyPressEvent);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveDisplayableManager::OnInteractorStyleEvent(int eventid)
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
    vtkDebugMacro("OnInteractorStyleEvent 3D: key press event position = "
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
void vtkMRMLMarkupsCurveDisplayableManager::UpdatePosition(vtkMarkupsWidget *widget, vtkMRMLMarkupsNode *node)
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
  vtkMarkupsCurveWidget* curveWidget = vtkMarkupsCurveWidget::SafeDownCast(widget);
  if (!curveWidget)
   {
   vtkErrorMacro("UpdatePosition: Could not get curve widget!")
   return;
   }

  // now get the widget properties (coordinates, measurement etc.) and if the mrml node has changed, propagate the changes
  bool positionChanged = false;
  int numberOfFiducials = pointsNode->GetNumberOfMarkups();
  for (int n = 0; n < numberOfFiducials; n++)
    {
    if (this->UpdateNthHandlePositionFromMRML(n, curveWidget, pointsNode))
      {
      positionChanged = true;
      }
    }
  // did any of the positions change?
  if (positionChanged && this->Updating == 0)
    {
    // not already updating from propagate mrml to widget, so trigger a render
    vtkMarkupsSplineRepresentation * curveRepresentation = vtkMarkupsSplineRepresentation::SafeDownCast(curveWidget->GetRepresentation());
    curveRepresentation->NeedToRenderOn();
    curveWidget->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveDisplayableManager::OnMRMLSceneEndClose()
{
  // clear out the map of glyph types
  this->Helper->ClearNodeGlyphTypes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveDisplayableManager::OnMRMLMarkupsNodeNthMarkupModifiedEvent(vtkMRMLMarkupsNode* node, int n)
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

  vtkMarkupsCurveWidget* curveWidget = vtkMarkupsCurveWidget::SafeDownCast(widget);
  if (!curveWidget)
   {
   vtkErrorMacro("OnMRMLMarkupsNodeNthMarkupModifiedEvent: Could not get curve widget")
   return;
   }
  this->SetNthHandle(n, vtkMRMLMarkupsCurveNode::SafeDownCast(node), curveWidget);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveDisplayableManager::OnMRMLMarkupsNodeMarkupAddedEvent(vtkMRMLMarkupsNode * markupsNode, int n)
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

  vtkMarkupsCurveWidget* curveWidget = vtkMarkupsCurveWidget::SafeDownCast(widget);
  if (!curveWidget)
   {
   vtkErrorMacro("OnMRMLMarkupsNodeMarkupAddedEvent: Could not get curve widget")
   return;
   }

  // this call will create a new handle and set it
  this->SetNthHandle(n, vtkMRMLMarkupsCurveNode::SafeDownCast(markupsNode), curveWidget);
  vtkMarkupsSplineRepresentation * curveRepresentation = vtkMarkupsSplineRepresentation::SafeDownCast(curveWidget->GetRepresentation());
  curveRepresentation->NeedToRenderOn();
  curveWidget->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveDisplayableManager::OnMRMLMarkupsNodeMarkupRemovedEvent(vtkMRMLMarkupsNode * markupsNode, int vtkNotUsed(n))
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
