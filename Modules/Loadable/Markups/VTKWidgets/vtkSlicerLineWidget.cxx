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

#include "vtkSlicerLineWidget.h"
#include "vtkSlicerLineRepresentation3D.h"
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

vtkStandardNewMacro(vtkSlicerLineWidget);

//----------------------------------------------------------------------
vtkSlicerLineWidget::vtkSlicerLineWidget()
{
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSlicerLineWidget::MoveAction);

}

//----------------------------------------------------------------------
vtkSlicerLineWidget::~vtkSlicerLineWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerLineWidget::CreateDefaultRepresentation()
{
  vtkSlicerLineRepresentation3D *rep = vtkSlicerLineRepresentation3D::New();
  rep->SetRenderer(this->GetCurrentRenderer());
  this->SetRepresentation(rep);
}

//-------------------------------------------------------------------------
void vtkSlicerLineWidget::AddPointToRepresentationFromWorldCoordinate(double worldCoordinates[3],
                                                                      bool persistence /*=true*/)
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
    if ( this->WidgetState == vtkSlicerLineWidget::Start )
      {
      this->InvokeEvent( vtkCommand::StartInteractionEvent, &this->CurrentHandle );
      }
    this->WidgetState = vtkSlicerLineWidget::Define;
    rep->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent( vtkCommand::PlacePointEvent, &this->CurrentHandle );
    this->ReleaseFocus();
    this->Render();
    if ( rep->GetNumberOfNodes() > 1 )
      {
      this->FollowCursor = false;
      this->WidgetState = vtkSlicerLineWidget::Manipulate;
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
void vtkSlicerLineWidget::MoveAction( vtkAbstractWidget *w )
{
  vtkSlicerLineWidget *self = reinterpret_cast<vtkSlicerLineWidget*>(w);
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(self->WidgetRep);

  if ( self->WidgetState == vtkSlicerLineWidget::Start ||
       !rep )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  if ( self->WidgetState == vtkSlicerLineWidget::Define &&
       self->FollowCursor && rep->GetNumberOfNodes() > 0)
    {
    rep->SetNthNodeDisplayPosition( rep->GetNumberOfNodes()-1, X, Y );
    }

  if (self->WidgetState == vtkSlicerLineWidget::Manipulate)
    {
    if ( rep->GetCurrentOperation() == vtkSlicerAbstractRepresentation::Inactive )
      {
      // on 3D views compute the interaction state is expensive
      // it would be the best to solve the rendering flickering of 3D view when using a vtkPropPicker
      // and use the vtkPropPicker (it is hardware accelerated) instaed of the vtkCellPicker.
      // 2D views already use the vtkPropPicker.
      int state = rep->ComputeInteractionState( X, Y );
      rep->ActivateNode( X, Y );
      self->SetCursor( state > 0 );
      if ( state == vtkSlicerAbstractRepresentation::OnControlPoint )
        {
        self->CurrentHandle = rep->GetActiveNode();
        self->InvokeEvent( vtkCommand::InteractionEvent, &self->CurrentHandle );
        }
      else if ( state == vtkSlicerAbstractRepresentation::OnLine )
        {
        self->CurrentHandle = -1;
        self->InvokeEvent( vtkCommand::InteractionEvent, &self->CurrentHandle );
        }
      else
        {
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
        if ( rep->GetInteractionState() == vtkSlicerAbstractRepresentation::OnControlPoint )
          {
          self->CurrentHandle = rep->GetActiveNode();
          }
        else if (rep->GetInteractionState() == vtkSlicerAbstractRepresentation::OnLine)
          {
          self->CurrentHandle = -1;
          }
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

