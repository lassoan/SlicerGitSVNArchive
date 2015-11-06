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
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

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
  this->UseColumnNameAsColumnHeader = true;
  this->UseFirstColumnAsRowHeader = false;
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
  of << indent << " useFirstColumnAsRowHeader=\"" << (this->GetUseFirstColumnAsRowHeader() ? "true" : "false") << "\"";
  of << indent << " useColumnNameAsColumnHeader=\"" << (this->GetUseColumnNameAsColumnHeader() ? "true" : "false") << "\"";
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
      this->SetLocked(strcmp(attValue,"true")?0:1);
      }
    else if (!strcmp(attName, "sortable"))
      {
      this->SetSortable(strcmp(attValue,"true")?0:1);
      }
    else if (!strcmp(attName, "useColumnNameAsColumnHeader"))
      {
      this->SetUseColumnNameAsColumnHeader(strcmp(attValue,"true")?0:1);
      }
    else if (!strcmp(attName, "useFirstColumnAsRowHeader"))
      {
      this->SetUseFirstColumnAsRowHeader(strcmp(attValue,"true")?0:1);
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
    this->SetLocked(node->GetLocked());
    this->SetSortable(node->GetSortable());
    this->SetUseColumnNameAsColumnHeader(node->GetUseColumnNameAsColumnHeader());
    this->SetUseFirstColumnAsRowHeader(node->GetUseFirstColumnAsRowHeader());
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
  os << indent << "\nLocked: " << this->GetLocked();
  os << indent << "\nSortable: " << this->GetSortable();
  os << indent << "\nUseColumnNameAsColumnHeader: " << this->GetUseColumnNameAsColumnHeader();
  os << indent << "\nUseFirstColumnAsRowHeader: " << this->GetUseFirstColumnAsRowHeader();
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

//----------------------------------------------------------------------------
vtkAbstractArray* vtkMRMLTableNode::AddColumn(vtkAbstractArray* column)
{
  // Create table (if not available already)
  if (!this->Table)
    {
    this->SetAndObserveTable(vtkSmartPointer<vtkTable>::New());
    if (!this->Table)
      {
      vtkErrorMacro("vtkMRMLTableNode::AddColumn failed: failed to add VTK table");
      return 0;
      }
    }
  int tableWasModified = this->StartModify();
  // Create new column (if not provided)
  vtkSmartPointer<vtkAbstractArray> newColumn;
  if (column)
    {
    newColumn = column;
    if (this->Table->GetNumberOfColumns()>0)
      {
      // There are columns in the table already, so we need to make sure the number of rows is matching
      int numberOfColumnsToAddToTable = newColumn->GetNumberOfTuples()-this->Table->GetNumberOfRows();
      if (numberOfColumnsToAddToTable>0)
        {
        // Table is shorter than the array, add empty rows to the table.
        for (int i=0; i<numberOfColumnsToAddToTable; i++)
          {
          this->Table->InsertNextBlankRow();
          }
        }
      else if (numberOfColumnsToAddToTable<0)
        {
        // Need to add more items to the array to match the table size.
        // To make sure that augmentation of the array is consistent, we create a dummy vtkTable
        // and use its InsertNextBlankRow() method.
        vtkNew<vtkTable> augmentingTable;
        augmentingTable->AddColumn(newColumn);
        int numberOfColumnsToAddToArray = -numberOfColumnsToAddToTable;
        for (int i=0; i<numberOfColumnsToAddToArray; i++)
          {
          augmentingTable->InsertNextBlankRow();
          }
        }
      }
    }
  else
    {
    int numberOfRows = this->Table->GetNumberOfRows();
    if (numberOfRows==0)
      {
      // add at least one element
      numberOfRows=1;
      }
    newColumn = vtkSmartPointer<vtkStringArray>::New();
    newColumn->SetNumberOfTuples(numberOfRows);
    vtkVariant emptyCell("");
    for (int i=0; i<numberOfRows; i++)
      {
      newColumn->SetVariantValue(i, emptyCell);
      }
    }

  // Generate a new unique column name (if not provided)
  if (!newColumn->GetName())
    {
    std::string newColumnName;
    int i=1;
    do
      {
      std::stringstream ss;
      ss << "Column " << i;
      newColumnName = ss.str();
      i++;
      }
    while (this->Table->GetColumnByName(newColumnName.c_str())!=0);
    newColumn->SetName(newColumnName.c_str());
    }

  this->Table->AddColumn(newColumn);
  this->Modified();
  this->EndModify(tableWasModified);
  return newColumn;
}

//----------------------------------------------------------------------------
bool vtkMRMLTableNode::RemoveColumn(int columnIndex)
{
  if (!this->Table)
    {
    vtkErrorMacro("vtkMRMLTableNode::RemoveColumn failed: invalid table");
    return false;
    }
  if (columnIndex<0 || columnIndex>=this->Table->GetNumberOfColumns())
    {
    vtkErrorMacro("vtkMRMLTableNode::RemoveColumn failed: invalid column index: "<<columnIndex);
    return false;
    }
  this->Table->RemoveColumn(columnIndex);
  this->Modified();
  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLTableNode::AddEmptyRow()
{
  if (!this->Table)
    {
    vtkErrorMacro("vtkMRMLTableNode::AddEmptyRow failed: invalid table");
    return -1;
    }
  if (this->Table->GetNumberOfColumns()==0)
    {
    vtkErrorMacro("vtkMRMLTableNode::AddEmptyRow failed: no columns are defined");
    return -1;
    }
  vtkIdType rowIndex = this->Table->InsertNextBlankRow();
  this->Modified();
  return rowIndex;
}

//----------------------------------------------------------------------------
bool vtkMRMLTableNode::RemoveRow(int rowIndex)
{
  if (!this->Table)
    {
    vtkErrorMacro("vtkMRMLTableNode::RemoveRow failed: invalid table");
    return false;
    }
  if (this->Table->GetNumberOfColumns()==0)
    {
    vtkErrorMacro("vtkMRMLTableNode::RemoveRow failed: no columns are defined");
    return false;
    }
    if (rowIndex<0 || rowIndex>=this->Table->GetNumberOfRows())
    {
    vtkErrorMacro("vtkMRMLTableNode::RemoveRow failed: invalid row index: "<<rowIndex);
    return false;
    }
  this->Table->RemoveRow(rowIndex);
  this->Modified();
  return true;
}
