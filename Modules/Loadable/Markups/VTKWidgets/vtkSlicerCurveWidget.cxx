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

#include "vtkSlicerCurveWidget.h"
#include "vtkSlicerCurveRepresentation3D.h"
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

vtkStandardNewMacro(vtkSlicerCurveWidget);

//----------------------------------------------------------------------
vtkSlicerCurveWidget::vtkSlicerCurveWidget()
{
  this->SetEventTranslation(vtkCommand::LeftButtonPressEvent, vtkEvent::AltModifier, WidgetRotateStart);
  this->SetEventTranslation(vtkCommand::LeftButtonReleaseEvent, vtkEvent::AnyModifier, WidgetRotateEnd);

  this->SetEventTranslation(vtkCommand::RightButtonPressEvent, vtkEvent::AltModifier, WidgetScaleStart);
  this->SetEventTranslation(vtkCommand::RightButtonReleaseEvent, vtkEvent::AnyModifier, WidgetScaleEnd);

  this->SetEventTranslation(vtkCommand::RightButtonPressEvent, vtkEvent::NoModifier, WidgetPick);

  this->SetEventTranslation(vtkCommand::LeftButtonPressEvent, vtkEvent::ControlModifier, WidgetControlPointInsert);
}

//----------------------------------------------------------------------
vtkSlicerCurveWidget::~vtkSlicerCurveWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerCurveWidget::CreateDefaultRepresentation()
{
  vtkSlicerCurveRepresentation3D *rep = vtkSlicerCurveRepresentation3D::New();
  rep->SetRenderer(this->GetRenderer());
  this->SetRepresentation(rep);
}

//----------------------------------------------------------------------
void vtkSlicerCurveWidget::AddPointOnCurveAction(vtkAbstractWidget *w)
{
  /* TODO: implement this
  vtkSlicerCurveWidget *self = reinterpret_cast<vtkSlicerCurveWidget*>(w);
  if ( self->WidgetState != vtkSlicerCurveWidget::Manipulate)
    {
    return;
    }

  vtkSlicerAbstractWidgetRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractWidgetRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  if ( rep->AddNodeOnWidget( X, Y ) )
    {
    if ( rep->ActivateNode( X, Y ) )
      {
      self->GrabFocus(self->EventCallbackCommand);
      self->StartInteraction();
      rep->StartWidgetInteraction( pos );
      self->CurrentHandle = rep->GetActiveNode();
      rep->SetCurrentOperationToPick();
      self->InvokeEvent( vtkCommand::PlacePointEvent, &self->CurrentHandle );
      self->EventCallbackCommand->SetAbortFlag( 1 );
      }
    }

  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
    */
}

