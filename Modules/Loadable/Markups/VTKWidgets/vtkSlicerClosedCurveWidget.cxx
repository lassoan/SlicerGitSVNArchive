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

#include "vtkSlicerClosedCurveWidget.h"
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

vtkStandardNewMacro(vtkSlicerClosedCurveWidget);

//----------------------------------------------------------------------
vtkSlicerClosedCurveWidget::vtkSlicerClosedCurveWidget()
{
  this->SetEventTranslation(vtkCommand::LeftButtonPressEvent, vtkEvent::AltModifier, WidgetRotateStart);
  this->SetEventTranslation(vtkCommand::LeftButtonReleaseEvent, vtkEvent::AnyModifier, WidgetRotateEnd);

  this->SetEventTranslation(vtkCommand::RightButtonPressEvent, vtkEvent::AltModifier, WidgetScaleStart);
  this->SetEventTranslation(vtkCommand::RightButtonReleaseEvent, vtkEvent::AnyModifier, WidgetScaleEnd);

  this->SetEventTranslation(vtkCommand::RightButtonPressEvent, vtkEvent::NoModifier, WidgetPick);

  this->SetEventTranslation(vtkCommand::LeftButtonPressEvent, vtkEvent::ControlModifier, WidgetControlPointInsert);
}

//----------------------------------------------------------------------
vtkSlicerClosedCurveWidget::~vtkSlicerClosedCurveWidget()
{
}

//----------------------------------------------------------------------
void vtkSlicerClosedCurveWidget::CreateDefaultRepresentation()
{
  vtkNew<vtkSlicerCurveRepresentation3D> rep;
  this->SetRepresentation(rep);
}

//----------------------------------------------------------------------
void vtkSlicerClosedCurveWidget::AddPointOnCurveAction(vtkAbstractWidget *w)
{
  /* TODO: implement this

  vtkSlicerClosedCurveWidget *self = reinterpret_cast<vtkSlicerClosedCurveWidget*>(w);
  if ( self->WidgetState != vtkSlicerClosedCurveWidget::Manipulate)
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
