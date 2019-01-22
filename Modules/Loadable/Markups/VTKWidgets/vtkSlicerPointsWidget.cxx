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

#include "vtkSlicerPointsWidget.h"
#include "vtkSlicerPointsRepresentation3D.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkSphereSource.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkSlicerPointsWidget);

//----------------------------------------------------------------------
vtkSlicerPointsWidget::vtkSlicerPointsWidget()
{
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSlicerPointsWidget::MoveAction);

}

//----------------------------------------------------------------------
vtkSlicerPointsWidget::~vtkSlicerPointsWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerPointsWidget::CreateDefaultRepresentation()
{
  vtkSlicerPointsRepresentation3D *rep = vtkSlicerPointsRepresentation3D::New();
  rep->SetRenderer(this->GetCurrentRenderer());
  this->SetRepresentation(rep);
}

//-------------------------------------------------------------------------
void vtkSlicerPointsWidget::AddPointToRepresentationFromWorldCoordinate(double worldCoordinates[3],
                                                                        bool persistence /*=false*/)
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if ( !rep )
    {
    return;
    }

  if ( persistence )
    {
    if ( this->FollowCursor == true )
      {
      rep->DeleteLastNode();
      }
    else
      {
      this->FollowCursor = true;
      }
    }
  else if ( this->FollowCursor == true )
    {
    rep->DeleteLastNode();
    this->FollowCursor = false;
    }

  if ( rep->AddNodeAtWorldPosition( worldCoordinates ) )
    {
    this->CurrentHandle = rep->GetActiveNode();
    if ( this->WidgetState == vtkSlicerPointsWidget::Start )
      {
      this->InvokeEvent( vtkCommand::StartInteractionEvent, &this->CurrentHandle );
      }
    this->WidgetState = vtkSlicerPointsWidget::Define;
    rep->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent( vtkCommand::PlacePointEvent, &this->CurrentHandle );
    this->ReleaseFocus();
    this->Render();
    if ( !this->FollowCursor )
      {
      this->WidgetState = vtkSlicerPointsWidget::Manipulate;
      this->InvokeEvent( vtkCommand::EndInteractionEvent, &this->CurrentHandle );
      this->Interactor->MouseWheelForwardEvent();
      this->Interactor->MouseWheelBackwardEvent();
      }
    }

  if ( this->FollowCursor )
    {
    rep->AddNodeAtWorldPosition( worldCoordinates );
    }
}

//-------------------------------------------------------------------------
void vtkSlicerPointsWidget::MoveAction( vtkAbstractWidget *w )
{
  vtkSlicerPointsWidget *self = reinterpret_cast<vtkSlicerPointsWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if ( self->WidgetState == vtkSlicerPointsWidget::Start ||
       !rep )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  if ( self->WidgetState == vtkSlicerPointsWidget::Define &&
       self->FollowCursor && rep->GetNumberOfNodes() > 0)
    {
    rep->SetNthNodeDisplayPosition( rep->GetNumberOfNodes()-1, X, Y );
    }

  if (self->WidgetState == vtkSlicerPointsWidget::Manipulate)
    {
    if ( rep->GetCurrentOperation() == vtkSlicerAbstractRepresentation::Inactive )
      {
      int active = rep->ActivateNode( X, Y );
      self->SetCursor( active );
      if ( active )
        {
        self->CurrentHandle = rep->GetActiveNode();
        self->InvokeEvent( vtkCommand::InteractionEvent, &self->CurrentHandle );
        }
      }
    else if ( rep->GetInteractionState() != vtkSlicerAbstractRepresentation::Outside )
      {
      double pos[2];
      pos[0] = X;
      pos[1] = Y;
      rep->WidgetInteraction( pos );
      if ( rep->GetCurrentOperation() != vtkSlicerAbstractRepresentation::Pick )
        {
        self->CurrentHandle = rep->GetActiveNode();
        self->InvokeEvent( vtkCommand::InteractionEvent, &self->CurrentHandle );
        }
      }
    }

  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}
