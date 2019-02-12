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
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceNode.h>

// STD includes
#include <algorithm>
#include <map>
#include <vector>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsDisplayableManagerHelper);

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MarkupsDisplayNodeList:" << std::endl;

  for (unsigned int nodesIt = 0;
       nodesIt < this->MarkupsDisplayNodeList.size();
       ++nodesIt)
    {
    os << indent.GetNextIndent() << this->MarkupsDisplayNodeList[nodesIt]->GetID() << std::endl;
    }

  os << indent << "Widgets map:" << std::endl;
  WidgetsIt widgetIterator = this->Widgets.begin();
  for (widgetIterator =  this->Widgets.begin();
       widgetIterator != this->Widgets.end();
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
  for (unsigned int i = 0; i < this->MarkupsDisplayNodeList.size(); i++)
    {
    vtkMRMLMarkupsDisplayNode *markupsDisplayNode = this->MarkupsDisplayNodeList[i];
    this->UpdateLocked(markupsDisplayNode);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::UpdateLockedAllWidgets(bool locked)
{
  // loop through all widgets and update lock status
  vtkDebugMacro("UpdateLockedAllWidgets: locked = " << locked);
  for (WidgetsIt it = this->Widgets.begin();
       it !=  this->Widgets.end();
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
vtkSlicerAbstractWidget * vtkMRMLMarkupsDisplayableManagerHelper::GetWidget(vtkMRMLMarkupsDisplayNode * node)
{
  if (!node)
    {
    return nullptr;
    }

  // Make sure the map contains a vtkWidget associated with this node
  WidgetsIt it = this->Widgets.find(node);
  if (it == this->Widgets.end())
    {
    return nullptr;
    }

  return it->second;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::RemoveAllWidgetsAndNodes()
{
  WidgetsIt widgetIterator = this->Widgets.begin();
  for (widgetIterator =  this->Widgets.begin();
       widgetIterator != this->Widgets.end();
       ++widgetIterator)
    {
    widgetIterator->second->Off();
    widgetIterator->second->Delete();
    }
  this->Widgets.clear();

  this->MarkupsDisplayNodeList.clear();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::RemoveWidgetAndNode(
    vtkMRMLMarkupsDisplayNode *node)
{
  if (!node)
    {
    return;
    }

  // Make sure the map contains a vtkWidget associated with this node
  WidgetsIt widgetIterator = this->Widgets.find(node);
  if (widgetIterator != this->Widgets.end())
    {
    // Delete and Remove vtkWidget from the map
    if (this->Widgets[node])
      {
      this->Widgets[node]->Off();
      this->Widgets[node]->Delete();
      }
    this->Widgets.erase(node);
    }

  // go through the list and remove the projection points for it
  // this can get called after a markup has been removed from the list,
  // so turn it around and iterate through all the markups in all the lists,
  // then delete the widget point projections for those that don't match up to
  // points in the scene
  vtkCollection *col = nullptr;
  if (node->GetScene() != nullptr)
    {
    col = node->GetScene()->GetNodesByClass("vtkMRMLMarkupsNode");
    }
  else
    {
    // without a scene to get the other nodes from, default to just using this node
    col = vtkCollection::New();
    col->AddItem(node);
    }

 int numberOfMarkupsNodes = col->GetNumberOfItems();
 // use a map so can use find, it's markup id: list node id
 std::map<std::string, std::string> currentMarkupIDs;
 for (int i = 0; i < numberOfMarkupsNodes; ++i)
   {
   vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(col->GetItemAsObject(i));
   if (!markupsNode)
     {
     continue;
     }
   for (int n = 0; n < markupsNode->GetNumberOfControlPoints(); n++)
    {
    std::string pointID = markupsNode->GetNthControlPointID(n);
    currentMarkupIDs[pointID] = std::string(markupsNode->GetID());
    }
   }
 col->RemoveAllItems();
 col->Delete();

  vtkMRMLMarkupsDisplayableManagerHelper::MarkupsNodeListIt nodeIterator = std::find(
      this->MarkupsDisplayNodeList.begin(),
      this->MarkupsDisplayNodeList.end(),
      node);

  // Make sure the map contains the markupsNode
  if (nodeIterator != this->MarkupsDisplayNodeList.end())
    {
    //vtkMRMLMarkupsNode* markupsNode = vtkMRMLMarkupsNode::SafeDownCast(*nodeIterator);
    //if (markupsNode)
     // {
      //markupsNode->Delete();
     // }
    this->MarkupsDisplayNodeList.erase(nodeIterator);
    }

  // remove the entry for the node glyph types
  //this->RemoveNodeGlyphType(node);

}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsDisplayableManagerHelper::RecordWidgetForNode(vtkSlicerAbstractWidget* widget, vtkMRMLMarkupsDisplayNode *node)
{
  if (!widget)
    {
    vtkErrorMacro("RecordWidgetForNode: no widget!");
    return;
    }
  if (!node)
    {
    vtkErrorMacro("RecordWidgetForNode: no node!");
    return;
    }
  // save the widget to the map indexed by the node
  this->Widgets[node] = widget;
  // save the node
  this->MarkupsDisplayNodeList.push_back(node);
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
