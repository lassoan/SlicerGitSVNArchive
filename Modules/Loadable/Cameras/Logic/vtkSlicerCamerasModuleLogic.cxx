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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Cameras Logic includes
#include "vtkSlicerCamerasModuleLogic.h"

// MRML includes
#include <vtkMRMLCameraNode.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerCamerasModuleLogic);

//----------------------------------------------------------------------------
vtkSlicerCamerasModuleLogic::vtkSlicerCamerasModuleLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerCamerasModuleLogic::~vtkSlicerCamerasModuleLogic()
= default;

//----------------------------------------------------------------------------
void vtkSlicerCamerasModuleLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkMRMLCameraNode* vtkSlicerCamerasModuleLogic
::GetViewActiveCameraNode(vtkMRMLViewNode* viewNode)
{
  if (!this->GetMRMLScene())
    {
    return nullptr;
    }
  vtkMRMLCameraNode* cameraNode = vtkMRMLCameraNode::SafeDownCast(
    this->GetMRMLScene()->GetSingletonNode(viewNode->GetLayoutName(), "vtkMRMLCameraNode"));
  return cameraNode;
}

//---------------------------------------------------------------------------
void vtkSlicerCamerasModuleLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  // List of events the slice logics should listen
  vtkNew<vtkIntArray> events;
  vtkNew<vtkFloatArray> priorities;

  float normalPriority = 0.0;
  //float lowPriority = -0.5;
  float highPriority = 0.5;

  // Events that use the default priority.  Don't care the order they
  // are triggered
  events->InsertNextValue(vtkMRMLScene::StartImportEvent);
  priorities->InsertNextValue(highPriority); // need to get to this event before camera displayable manager clears the active tag
  events->InsertNextValue(vtkMRMLScene::NodeAboutToBeAddedEvent);
  priorities->InsertNextValue(normalPriority);

  // Events that need to a lower priority than normal, in other words,
  // guaranteed to be be called before other triggers.
  // We use high priority here so that modules that observe scene imports
  // will not have to process imported camera nodes that will be deleted
  // right away.
  events->InsertNextValue(vtkMRMLScene::EndImportEvent);
  priorities->InsertNextValue(highPriority);

  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer(), priorities.GetPointer());

  this->ProcessMRMLSceneEvents(newScene, vtkCommand::ModifiedEvent, nullptr);
}

//---------------------------------------------------------------------------
void vtkSlicerCamerasModuleLogic::ProcessMRMLSceneEvents(vtkObject *caller, unsigned long event, void *callData)
{
  this->Superclass::ProcessMRMLSceneEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkSlicerCamerasModuleLogic::OnMRMLSceneStartImport()
{
}

//----------------------------------------------------------------------------
void vtkSlicerCamerasModuleLogic::OnMRMLSceneEndImport()
{
}
