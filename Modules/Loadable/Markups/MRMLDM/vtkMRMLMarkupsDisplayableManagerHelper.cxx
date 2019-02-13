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
#include <vtkMRMLMarkupsNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsDisplayableManagerHelper.h"
#include "vtkMRMLMarkupsDisplayableManager2D.h"
#include "vtkMRMLMarkupsDisplayableManager3D.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>
#include <vtkPickingManager.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSlicerAbstractRepresentation.h>
#include <vtkSlicerAbstractWidget.h>
#include <vtkSlicerPointsWidget.h>
#include <vtkSlicerLineWidget.h>
#include <vtkSlicerAngleWidget.h>
#include <vtkSmartPointer.h>
#include <vtkSphereHandleRepresentation.h>

// MRML includes
#include <vtkEventBroker.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLAbstractDisplayableManager.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLViewNode.h>

// STD includes
#include <algorithm>
#include <map>
#include <vector>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsDisplayableManagerHelper);
vtkCxxSetObjectMacro(vtkMRMLMarkupsDisplayableManagerHelper, DisplayableManager, vtkMRMLAbstractDisplayableManager);

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MarkupsDisplayNodeList:" << std::endl;

  os << indent << "MarkupsDisplayNodesToWidgets map:" << std::endl;
  DisplayNodeToWidgetIt widgetIterator = this->MarkupsDisplayNodesToWidgets.begin();
  for (widgetIterator =  this->MarkupsDisplayNodesToWidgets.begin();
       widgetIterator != this->MarkupsDisplayNodesToWidgets.end();
       ++widgetIterator)
    {
    os << indent.GetNextIndent() << widgetIterator->first->GetID() << " : widget is " << (widgetIterator->second ? "not null" : "null") << std::endl;
    if (widgetIterator->second &&
        widgetIterator->second->GetRepresentation())
      {
      vtkSlicerAbstractRepresentation * abstractRepresentation = vtkSlicerAbstractRepresentation::SafeDownCast(widgetIterator->second->GetRepresentation());
      int numberOfNodes = 0;
      if (abstractRepresentation)
        {
        numberOfNodes = abstractRepresentation->GetNumberOfNodes();
        }
      else
        {
        vtkWarningMacro("PrintSelf: no representation for widget assoc with markups node " << widgetIterator->first->GetID());
        }
      os << indent.GetNextIndent().GetNextIndent() << "enabled = "
        << widgetIterator->second->GetEnabled()
        << ", number of nodes = " << numberOfNodes << std::endl;
      }
    }
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManagerHelper::vtkMRMLMarkupsDisplayableManagerHelper()
{
  this->DisplayableManager = NULL;
  this->AddingMarkupsNode = false;
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManagerHelper::~vtkMRMLMarkupsDisplayableManagerHelper()
{
  this->RemoveAllWidgetsAndNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::UpdateLockedAllWidgetsFromNodes()
{
  // iterate through the node list
  for (DisplayNodeToWidgetIt it = this->MarkupsDisplayNodesToWidgets.begin();
    it != this->MarkupsDisplayNodesToWidgets.end();
    ++it)
    {
    vtkMRMLMarkupsDisplayNode *markupsDisplayNode = (it->first);
    this->UpdateLocked(markupsDisplayNode);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::UpdateLockedAllWidgets(bool locked)
{
  // loop through all widgets and update lock status
  vtkDebugMacro("UpdateLockedAllWidgets: locked = " << locked);
  for (DisplayNodeToWidgetIt it = this->MarkupsDisplayNodesToWidgets.begin();
       it !=  this->MarkupsDisplayNodesToWidgets.end();
       ++it)
    {
    if (it->second)
      {
      if (locked)
        {
        it->second->ProcessEventsOff();
        }
      else
        {
        it->second->ProcessEventsOn();
        }
      }
    }
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget* vtkMRMLMarkupsDisplayableManagerHelper::GetWidget(vtkMRMLMarkupsNode * markupsNode)
{
  if (!markupsNode)
  {
    return nullptr;
  }
  // Make sure the map contains a vtkWidget associated with this node
  MarkupsToDisplayIt it = this->MarkupsToDisplayNodes.find(markupsNode);
  if (it == this->MarkupsToDisplayNodes.end())
  {
    return nullptr;
  }

  // Return first widget found for a display node
  std::set< vtkMRMLMarkupsDisplayNode* >::iterator displayNodeIterator;
  for (displayNodeIterator = (it->second).begin();
    displayNodeIterator != (it->second).begin();
    ++displayNodeIterator)
  {
    vtkSlicerAbstractWidget* widgetFound = this->GetWidget(*displayNodeIterator);
    if (widgetFound)
    {
      return widgetFound;
    }
  }
  return nullptr;
}

//---------------------------------------------------------------------------
vtkSlicerAbstractWidget * vtkMRMLMarkupsDisplayableManagerHelper::GetWidget(vtkMRMLMarkupsDisplayNode * node)
{
  if (!node)
    {
    return nullptr;
    }

  // Make sure the map contains a vtkWidget associated with this node
  DisplayNodeToWidgetIt it = this->MarkupsDisplayNodesToWidgets.find(node);
  if (it == this->MarkupsDisplayNodesToWidgets.end())
    {
    return nullptr;
    }

  return it->second;
}


//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::RemoveAllWidgetsAndNodes()
{
  DisplayNodeToWidgetIt widgetIterator = this->MarkupsDisplayNodesToWidgets.begin();
  for (widgetIterator =  this->MarkupsDisplayNodesToWidgets.begin();
       widgetIterator != this->MarkupsDisplayNodesToWidgets.end();
       ++widgetIterator)
    {
    widgetIterator->second->Off();
    widgetIterator->second->Delete();
    }
  this->MarkupsDisplayNodesToWidgets.clear();
  // TODO: remove observers and MarkupsToDisplayNodes
}

/*
//---------------------------------------------------------------------------
int vtkMRMLMarkupsDisplayableManagerHelper::GetNodeGlyphType(vtkMRMLNode *displayNode, int index)
{
  std::map<vtkMRMLNode*, std::vector<int> >::iterator iter  = this->NodeGlyphTypes.find(displayNode);
  if (iter == this->NodeGlyphTypes.end())
    {
    // no record for this node
    return -1;
    }
  if (index < 0 || iter->second.size() <= static_cast<unsigned int> (index))
    {
    // no entry for this index
    return -1;
    }
  return iter->second[static_cast<size_t> (index)];
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::SetNodeGlyphType(vtkMRMLNode *displayNode, int glyphType, int index)
{
  if (!displayNode)
    {
    return;
    }
  // is there already an entry for this node?
  std::map<vtkMRMLNode*, std::vector<int> >::iterator iter  = this->NodeGlyphTypes.find(displayNode);
  if (iter == this->NodeGlyphTypes.end())
    {
    // no? add one
    std::vector<int> newEntry;
    newEntry.resize(static_cast<size_t> (index + 1));
    newEntry.at(static_cast<size_t> (index)) = glyphType;
    this->NodeGlyphTypes[displayNode] = newEntry;
    return;
    }

  // is there already an entry at this index?
  if (iter->second.size() <= static_cast<unsigned int> (index))
    {
    // no? add space
    this->NodeGlyphTypes[displayNode].resize(static_cast<size_t> (index + 1));
    }
  // set it
  this->NodeGlyphTypes[displayNode].at(static_cast<size_t> (index)) = glyphType;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::RemoveNodeGlyphType(vtkMRMLNode *displayNode)
{
  if (!displayNode)
    {
    return;
    }
  // is there an entry for this node?
  std::map<vtkMRMLNode*, std::vector<int> >::iterator iter  = this->NodeGlyphTypes.find(displayNode);
  if (iter == this->NodeGlyphTypes.end())
    {
    return;
    }
  // erase it
  this->NodeGlyphTypes.erase(iter);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::ClearNodeGlyphTypes()
{
  this->NodeGlyphTypes.clear();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::PrintNodeGlyphTypes()
{
  std::cout << "Node Glyph Types:" << std::endl;
  for (std::map<vtkMRMLNode*, std::vector<int> >::iterator iter  = this->NodeGlyphTypes.begin();
       iter !=  this->NodeGlyphTypes.end();
       iter++)
    {
    std::cout << "\tDisplay node " << iter->first->GetID() << ": " << iter->first->GetName() << std::endl;
    for (unsigned int i = 0; i < iter->second.size(); i++)
      {
      std::cout << "\t\t" << i << " glyph type = " << iter->second[i] << std::endl;
      }
    }
}
*/


//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::AddMarkupsNode(vtkMRMLMarkupsNode* node)
{
  if (this->AddingMarkupsNode)
  {
    return;
  }
  this->AddingMarkupsNode = true;

  if (!node)
  {
    return;
  }

  vtkMRMLAbstractViewNode* viewNode = nullptr;
  vtkMRMLMarkupsDisplayableManager2D* displayableManager2d = vtkMRMLMarkupsDisplayableManager2D::SafeDownCast(this->DisplayableManager);
  vtkMRMLMarkupsDisplayableManager3D* displayableManager3d = vtkMRMLMarkupsDisplayableManager3D::SafeDownCast(this->DisplayableManager);
  if (displayableManager2d)
  {
    viewNode = displayableManager2d->GetMRMLSliceNode();
  }
  else if (displayableManager3d)
  {
    viewNode = displayableManager3d->GetMRMLViewNode();
  }
  if (!viewNode)
  {
    return;
  }

  this->AddObservations(node);

  // Add Display Nodes
  int nnodes = node->GetNumberOfDisplayNodes();
  for (int i = 0; i<nnodes; i++)
  {
    vtkMRMLMarkupsDisplayNode *displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(node->GetNthDisplayNode(i));

    // Check whether DisplayNode should be shown in this view
    if (!displayNode
      || !displayNode->IsA("vtkMRMLMarkupsDisplayNode")
      || !displayNode->IsDisplayableInView(viewNode->GetID()))
    {
      continue;
    }

    this->MarkupsToDisplayNodes[node].insert(displayNode);
    this->AddDisplayNode(displayNode);
  }

  this->AddingMarkupsNode = false;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::RemoveMarkupsNode(vtkMRMLMarkupsNode* node)
{
  if (!node)
  {
    return;
  }
  vtkMRMLMarkupsDisplayableManagerHelper::MarkupsToDisplayIt displayableIt =
    this->MarkupsToDisplayNodes.find(node);

  if (displayableIt == this->MarkupsToDisplayNodes.end())
  {
    // we do not manage this markup
    return;
  }

  std::set< vtkMRMLMarkupsDisplayNode* > dnodes = displayableIt->second;
  std::set< vtkMRMLMarkupsDisplayNode* >::iterator diter;
  for (diter = dnodes.begin(); diter != dnodes.end(); ++diter)
  {
    this->RemoveDisplayNode(*diter);
  }
  this->RemoveObservations(node);
  this->MarkupsToDisplayNodes.erase(displayableIt);
}


//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::AddDisplayNode(vtkMRMLMarkupsDisplayNode* markupsDisplayNode)
{
  if (!markupsDisplayNode)
  {
    return;
  }

  vtkMRMLMarkupsDisplayableManager2D* displayableManager2d = vtkMRMLMarkupsDisplayableManager2D::SafeDownCast(this->DisplayableManager);
  vtkMRMLMarkupsDisplayableManager3D* displayableManager3d = vtkMRMLMarkupsDisplayableManager3D::SafeDownCast(this->DisplayableManager);

  // Do not add the display node if displayNodeIt is already associated with a widget object.
  // This happens when a segmentation node already associated with a display node
  // is copied into an other (using vtkMRMLNode::Copy()) and is added to the scene afterward.
  // Related issue are #3428 and #2608
  vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt displayNodeIt
    = this->MarkupsDisplayNodesToWidgets.find(markupsDisplayNode);
  if (displayNodeIt != this->MarkupsDisplayNodesToWidgets.end())
  {
    return;
  }

  // There should not be a widget for the new node
  if (this->GetWidget(markupsDisplayNode) != nullptr)
  {
    vtkErrorMacro("vtkMRMLMarkupsDisplayableManager2D: A widget is already associated to this node");
    return;
  }

  vtkSlicerAbstractWidget* newWidget = NULL;
  vtkMRMLInteractionNode *interactionNode = NULL;
  if (displayableManager2d)
  {
    newWidget = displayableManager2d->CreateWidget(markupsDisplayNode);
    interactionNode = displayableManager2d->GetInteractionNode();
  }
  else if (displayableManager3d)
  {
    newWidget = displayableManager3d->CreateWidget(markupsDisplayNode);
    interactionNode = displayableManager3d->GetInteractionNode();
  }
  if (!newWidget)
  {
    vtkErrorMacro("vtkMRMLMarkupsDisplayableManager2D: Failed to create widget");
    return;
  }

  //Set up widget

  if (interactionNode)
  {
    if (interactionNode->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
    {
      newWidget->SetManagesCursor(false);
    }
    else
    {
      newWidget->SetManagesCursor(true);
    }
  }
  //vtkDebugMacro("Fids CreateWidget: Created widget for node " << markupsNode->GetID() << " with a representation");

  // record the mapping between node and widget in the helper
  this->MarkupsDisplayNodesToWidgets[markupsDisplayNode] = newWidget;

  // Build representation
  newWidget->BuildRepresentation();

  if (displayableManager2d)
  {
    displayableManager2d->OnWidgetAdded(markupsDisplayNode, newWidget);
    displayableManager2d->RequestRender();

  }
  else if (displayableManager3d)
  {
    displayableManager2d->OnWidgetAdded(markupsDisplayNode, newWidget);
    displayableManager3d->RequestRender();
  }

  // Update cached matrices. Calls UpdateWidget
  //this->UpdateDisplayableTransforms(mNode);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::RemoveDisplayNode(vtkMRMLMarkupsDisplayNode* markupsDisplayNode)
{
  if (!markupsDisplayNode)
  {
    return;
  }

  vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt displayNodeIt
    = this->MarkupsDisplayNodesToWidgets.find(markupsDisplayNode);
  if (displayNodeIt == this->MarkupsDisplayNodesToWidgets.end())
  {
    // no widget found for this display node
    return;
  }

  vtkSlicerAbstractWidget* widget = (displayNodeIt->second);
  if (widget)
  {
    widget->Off();
    widget->Delete();
  }

  // TODO: remove vtkMarkupsFiducialWidgetCallback2D observers

  this->MarkupsDisplayNodesToWidgets.erase(markupsDisplayNode);
}


//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::AddObservations(vtkMRMLMarkupsNode* node)
{
  vtkCallbackCommand* callbackCommand = nullptr;
  vtkMRMLMarkupsDisplayableManager2D* displayableManager2d = vtkMRMLMarkupsDisplayableManager2D::SafeDownCast(this->DisplayableManager);
  vtkMRMLMarkupsDisplayableManager3D* displayableManager3d = vtkMRMLMarkupsDisplayableManager3D::SafeDownCast(this->DisplayableManager);
  if (displayableManager2d)
  {
    callbackCommand = displayableManager2d->GetMRMLNodesCallbackCommand();
  }
  else if (displayableManager3d)
  {
    callbackCommand = displayableManager3d->GetMRMLNodesCallbackCommand();
  }

  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  if (!broker->GetObservationExist(node, vtkCommand::ModifiedEvent, this, callbackCommand))
  {
    broker->AddObservation(node, vtkCommand::ModifiedEvent, this, callbackCommand);
  }
  if (!broker->GetObservationExist(node, vtkMRMLTransformableNode::TransformModifiedEvent, this, callbackCommand))
  {
    broker->AddObservation(node, vtkMRMLTransformableNode::TransformModifiedEvent, this, callbackCommand);
  }
  if (!broker->GetObservationExist(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this, callbackCommand))
  {
    broker->AddObservation(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this, callbackCommand);
  }
  if (!broker->GetObservationExist(node, vtkMRMLMarkupsNode::PointModifiedEvent, this, callbackCommand))
  {
    broker->AddObservation(node, vtkMRMLMarkupsNode::PointModifiedEvent, this, callbackCommand);
  }
  if (!broker->GetObservationExist(node, vtkMRMLMarkupsNode::PointAddedEvent, this, callbackCommand))
  {
    broker->AddObservation(node, vtkMRMLMarkupsNode::PointAddedEvent, this, callbackCommand);
  }
  if (!broker->GetObservationExist(node, vtkMRMLMarkupsNode::PointRemovedEvent, this, callbackCommand))
  {
    broker->AddObservation(node, vtkMRMLMarkupsNode::PointRemovedEvent, this, callbackCommand);
  }
  if (!broker->GetObservationExist(node, vtkMRMLMarkupsNode::AllPointsRemovedEvent, this, callbackCommand))
  {
    broker->AddObservation(node, vtkMRMLMarkupsNode::AllPointsRemovedEvent, this, callbackCommand);
  }
  if (!broker->GetObservationExist(node, vtkMRMLMarkupsNode::LockModifiedEvent, this, callbackCommand))
  {
    broker->AddObservation(node, vtkMRMLMarkupsNode::LockModifiedEvent, this, callbackCommand);
  }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::RemoveObservations(vtkMRMLMarkupsNode* node)
{
  vtkCallbackCommand* callbackCommand = nullptr;
  vtkMRMLMarkupsDisplayableManager2D* displayableManager2d = vtkMRMLMarkupsDisplayableManager2D::SafeDownCast(this->DisplayableManager);
  vtkMRMLMarkupsDisplayableManager3D* displayableManager3d = vtkMRMLMarkupsDisplayableManager3D::SafeDownCast(this->DisplayableManager);
  if (displayableManager2d)
  {
    callbackCommand = displayableManager2d->GetMRMLNodesCallbackCommand();
  }
  else if (displayableManager3d)
  {
    callbackCommand = displayableManager3d->GetMRMLNodesCallbackCommand();
  }

  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  observations = broker->GetObservations(node, vtkCommand::ModifiedEvent, this, callbackCommand);
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLTransformableNode::TransformModifiedEvent, this, callbackCommand);
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLDisplayableNode::DisplayModifiedEvent, this, callbackCommand);
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLMarkupsNode::PointModifiedEvent, this, callbackCommand);
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLMarkupsNode::PointAddedEvent, this, callbackCommand);
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLMarkupsNode::PointRemovedEvent, this, callbackCommand);
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLMarkupsNode::AllPointsRemovedEvent, this, callbackCommand);
  broker->RemoveObservations(observations);
  observations = broker->GetObservations(node, vtkMRMLMarkupsNode::LockModifiedEvent, this, callbackCommand);
  broker->RemoveObservations(observations);
}
