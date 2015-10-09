/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, PMH.
  and was partially funded by OCAIRO and Sparkit.

==============================================================================*/

// MRML includes
#include "vtkMRMLTableNode.h"
#include "vtkMRMLTableStorageNode.h"

// VTK includes
#include <vtkTable.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTableNode);

//----------------------------------------------------------------------------
vtkMRMLTableNode::vtkMRMLTableNode()
{
  this->Table = vtkTable::New();
  this->Locked = false;
  this->Sortable = false;
  this->HideFromEditorsOff();
}

//----------------------------------------------------------------------------
vtkMRMLTableNode::~vtkMRMLTableNode()
{
  if (this->Table)
    {
    this->Table->Delete();
    this->Table = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkMRMLTableNode::WriteXML(ostream& of, int nIndent)
{
  // Start by having the superclass write its information
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);
  of << indent << " locked=\"" << (this->GetLocked() ? "true" : "false") << "\"";
  of << indent << " sortable=\"" << (this->GetSortable() ? "true" : "false") << "\"";
}


//----------------------------------------------------------------------------
void vtkMRMLTableNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  vtkMRMLNode::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "locked"))
      {
      if (!strcmp(attValue,"true"))
        {
        this->SetLocked(1);
        }
      else
        {
        this->SetLocked(0);
        }
      }
    else if (!strcmp(attName, "sortable"))
      {
      if (!strcmp(attValue,"true"))
        {
        this->SetSortable(1);
        }
      else
        {
        this->SetSortable(0);
        }
      }
    }

  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
//
void vtkMRMLTableNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  vtkMRMLTableNode *node = vtkMRMLTableNode::SafeDownCast(anode);
  if (node)
    {
    this->SetAndObserveTable(node->GetTable());
    }
  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLTableNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  //if (this->TargetPlanList && this->TargetPlanList == vtkMRMLFiducialListNode::SafeDownCast(caller) &&
  //  event ==  vtkCommand::ModifiedEvent)
  //  {
  //  //this->InvokeEvent(vtkMRMLVolumeNode::ImageDataModifiedEvent, NULL);
  //  //this->UpdateFromMRML();
  //  return;
  //  }
  //
  //if (this->TargetCompletedList && this->TargetCompletedList == vtkMRMLFiducialListNode::SafeDownCast(caller) &&
  //  event ==  vtkCommand::ModifiedEvent)
  //  {
  //  //this->InvokeEvent(vtkMRMLVolumeNode::ImageDataModifiedEvent, NULL);
  //  //this->UpdateFromMRML();
  //  return;
  //  }

  return;
}


//----------------------------------------------------------------------------
void vtkMRMLTableNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "\nTable Data:";
  if (this->GetTable())
    {
    os << "\n";
    this->GetTable()->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "none\n";
    }
}

//---------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLTableNode::CreateDefaultStorageNode()
{
  return vtkMRMLStorageNode::SafeDownCast(vtkMRMLTableStorageNode::New());
}

//----------------------------------------------------------------------------
void vtkMRMLTableNode::SetAndObserveTable(vtkTable* table)
{
  if (table==this->Table)
    {
    return;
    }
  vtkSetAndObserveMRMLObjectMacro(this->Table, table);
  this->Modified(); // TODO: check if modified event is invoked by vtkSetAndObserveMRMLObjectMacro; if yes then don't invoke it again here
}
