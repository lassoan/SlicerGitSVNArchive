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
                                                                      bool persistence /*=false*/)
{
  vtkSlicerAbstractRepresentation *rep =
    reinterpret_cast<vtkSlicerAbstractRepresentation*>(this->WidgetRep);

  if ( !rep )
    {
    return;
    }

  this->CurrentHandle = rep->GetActiveNode();

  if ( rep->AddNodeAtWorldPosition( worldCoordinates ) )
    {
    this->CurrentHandle = rep->GetActiveNode();
    if ( this->WidgetState == vtkSlicerLineWidget::Start )
      {
      this->InvokeEvent( vtkCommand::StartInteractionEvent, &this->CurrentHandle );
      }
    this->WidgetState = vtkSlicerLineWidget::Define;
    rep->VisibilityOn();
    this->ReleaseFocus();
    this->Render();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent( vtkCommand::PlacePointEvent, &this->CurrentHandle );
    if ( !persistence )
      {
      this->WidgetState = vtkSlicerLineWidget::Manipulate;
      this->EventCallbackCommand->SetAbortFlag( 1 );
      this->InvokeEvent( vtkCommand::EndInteractionEvent, &this->CurrentHandle );
      this->Interactor->MouseWheelForwardEvent();
      this->Interactor->MouseWheelBackwardEvent();
      }
    }
}
