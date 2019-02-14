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
#include "vtkMRMLMarkupsDisplayableManager.h"

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
vtkCxxSetObjectMacro(vtkMRMLMarkupsDisplayableManagerHelper, DisplayableManager, vtkMRMLMarkupsDisplayableManager);


//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManagerHelper::vtkMRMLMarkupsDisplayableManagerHelper()
{
  this->DisplayableManager = NULL;
  this->AddingMarkupsNode = false;
  this->ObservedMarkupNodeEvents.push_back(vtkCommand::ModifiedEvent);
  this->ObservedMarkupNodeEvents.push_back(vtkMRMLTransformableNode::TransformModifiedEvent);
  this->ObservedMarkupNodeEvents.push_back(vtkMRMLDisplayableNode::DisplayModifiedEvent);
  this->ObservedMarkupNodeEvents.push_back(vtkMRMLMarkupsNode::PointModifiedEvent);
  this->ObservedMarkupNodeEvents.push_back(vtkMRMLMarkupsNode::PointAddedEvent);
  this->ObservedMarkupNodeEvents.push_back(vtkMRMLMarkupsNode::PointRemovedEvent);
  this->ObservedMarkupNodeEvents.push_back(vtkMRMLMarkupsNode::AllPointsRemovedEvent);
  this->ObservedMarkupNodeEvents.push_back(vtkMRMLMarkupsNode::LockModifiedEvent);
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsDisplayableManagerHelper::~vtkMRMLMarkupsDisplayableManagerHelper()
{
  this->RemoveAllWidgetsAndNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MarkupsDisplayNodeList:" << std::endl;

  os << indent << "MarkupsDisplayNodesToWidgets map:" << std::endl;
  DisplayNodeToWidgetIt widgetIterator = this->MarkupsDisplayNodesToWidgets.begin();
  for (widgetIterator = this->MarkupsDisplayNodesToWidgets.begin();
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
};

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::UpdateLockedAllWidgetsFromNodes()
{
  // iterate through the node list
  for (DisplayNodeToWidgetIt it = this->MarkupsDisplayNodesToWidgets.begin();
    it != this->MarkupsDisplayNodesToWidgets.end();
    ++it)
    {
    // TODO: we could be more efficient by only updating lock state
    (it->second)->GetRepresentation()->BuildRepresentation();
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
  vtkMRMLMarkupsDisplayableManagerHelper::MarkupsNodesIt displayableIt =
    this->MarkupsNodes.find(markupsNode);
  if (displayableIt == this->MarkupsNodes.end())
  {
    // we do not manage this markup
    return nullptr;
  }

  // Return first widget found for a markups node
  for (vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt widgetIterator = this->MarkupsDisplayNodesToWidgets.begin();
    widgetIterator != this->MarkupsDisplayNodesToWidgets.end();
    ++widgetIterator)
  {
    vtkMRMLMarkupsDisplayNode *markupsDisplayNode = widgetIterator->first;
    if (markupsDisplayNode->GetDisplayableNode() == markupsNode)
    {
      return widgetIterator->second;
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
  if (!node)
  {
    return;
  }
  vtkMRMLAbstractViewNode* viewNode = vtkMRMLAbstractViewNode::SafeDownCast(this->DisplayableManager->GetMRMLDisplayableNode());
  if (!viewNode)
  {
    return;
  }

  if (this->AddingMarkupsNode)
  {
    return;
  }
  this->AddingMarkupsNode = true;

  this->AddObservations(node);
  this->MarkupsNodes.insert(node);

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

  vtkMRMLMarkupsDisplayableManagerHelper::MarkupsNodesIt displayableIt =
    this->MarkupsNodes.find(node);

  if (displayableIt == this->MarkupsNodes.end())
  {
    // we do not manage this markup
    return;
  }

  // Remove display nodes corresponding to this markups node
  for (vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt widgetIterator = this->MarkupsDisplayNodesToWidgets.begin();
    widgetIterator != this->MarkupsDisplayNodesToWidgets.end();
    /*upon deletion the increment is done already, so don't increment here*/)
  {
    vtkMRMLMarkupsDisplayNode *markupsDisplayNode = widgetIterator->first;
    if (markupsDisplayNode->GetDisplayableNode() != node)
    {
      ++widgetIterator;
    }
    else
    {
      // display node of the node that is being removed
      vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt widgetIteratorToRemove = widgetIterator;
      ++widgetIterator;
      vtkSlicerAbstractWidget* widgetToRemove = widgetIteratorToRemove->second;
      this->DeleteWidget(widgetToRemove);
      this->MarkupsDisplayNodesToWidgets.erase(widgetIteratorToRemove);
    }
  }

  this->RemoveObservations(node);
  this->MarkupsNodes.erase(displayableIt);
}


//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::AddDisplayNode(vtkMRMLMarkupsDisplayNode* markupsDisplayNode)
{
  if (!markupsDisplayNode)
  {
    return;
  }

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

  vtkSlicerAbstractWidget* newWidget = this->DisplayableManager->CreateWidget(markupsDisplayNode);
  if (!newWidget)
  {
    vtkErrorMacro("vtkMRMLMarkupsDisplayableManager2D: Failed to create widget");
    return;
  }

  //Set up widget
  vtkMRMLInteractionNode* interactionNode = this->DisplayableManager->GetInteractionNode();
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

  this->DisplayableManager->OnWidgetAdded(markupsDisplayNode, newWidget);
  this->DisplayableManager->RequestRender();

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
  this->DeleteWidget(widget);

  this->MarkupsDisplayNodesToWidgets.erase(markupsDisplayNode);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::DeleteWidget(vtkSlicerAbstractWidget* widget)
{
  if (!widget)
  {
    return;
  }
  // TODO: remove vtkMarkupsFiducialWidgetCallback2D observers
  widget->Off();
  widget->Delete();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::AddObservations(vtkMRMLMarkupsNode* node)
{
  vtkCallbackCommand* callbackCommand = this->DisplayableManager->GetMRMLNodesCallbackCommand();
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  for (auto observedMarkupNodeEvent : this->ObservedMarkupNodeEvents)
  {
    if (!broker->GetObservationExist(node, observedMarkupNodeEvent, this->DisplayableManager, callbackCommand))
    {
      broker->AddObservation(node, observedMarkupNodeEvent, this->DisplayableManager, callbackCommand);
    }
  }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::RemoveObservations(vtkMRMLMarkupsNode* node)
{
  vtkCallbackCommand* callbackCommand = this->DisplayableManager->GetMRMLNodesCallbackCommand();
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  for (auto observedMarkupNodeEvent : this->ObservedMarkupNodeEvents)
  {
    vtkEventBroker::ObservationVector observations;
    observations = broker->GetObservations(node, observedMarkupNodeEvent, this, callbackCommand);
    broker->RemoveObservations(observations);
  }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::UpdateAllWidgetsFromInteractionNode(vtkMRMLInteractionNode *interactionNode)
{
  // Sanity checks
  if (interactionNode == nullptr)
  {
    return;
  }

  // loop through all widgets and update the widget status
  for (vtkMRMLMarkupsDisplayableManagerHelper::DisplayNodeToWidgetIt widgetIterator = this->MarkupsDisplayNodesToWidgets.begin();
    widgetIterator != this->MarkupsDisplayNodesToWidgets.end(); ++widgetIterator)
  {
    vtkSlicerAbstractWidget* widget = widgetIterator->second;
    if (!widget)
    {
      continue;
    }
    widget->Leave();
  }
}
