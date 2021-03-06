/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLTableViewNode.h"

#include "vtkMRMLScene.h"
#include "vtkMRMLTableNode.h"

// VTK includes
#include <vtkCommand.h> // for vtkCommand::ModifiedEvent
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

const char* vtkMRMLTableViewNode::TableNodeReferenceRole = "table";
const char* vtkMRMLTableViewNode::TableNodeReferenceMRMLAttributeName = "tableNodeRef";

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTableViewNode);

//----------------------------------------------------------------------------
vtkMRMLTableViewNode::vtkMRMLTableViewNode()
: DoPropagateTableSelection(true)
{
  this->AddNodeReferenceRole(this->GetTableNodeReferenceRole(),
                             this->GetTableNodeReferenceMRMLAttributeName());
}

//----------------------------------------------------------------------------
vtkMRMLTableViewNode::~vtkMRMLTableViewNode()
{
}

//----------------------------------------------------------------------------
const char* vtkMRMLTableViewNode::GetNodeTagName()
{
  return "TableView";
}


//----------------------------------------------------------------------------
void vtkMRMLTableViewNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);
  of << indent << " doPropagateTableSelection=\"" << (int)this->DoPropagateTableSelection << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLTableViewNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  Superclass::ReadXMLAttributes(atts);
  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if(!strcmp (attName, "doPropagateTableSelection" ))
      {
      this->SetDoPropagateTableSelection(atoi(attValue)?true:false);
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
// Copy the node\"s attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, SliceID
void vtkMRMLTableViewNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  vtkMRMLTableViewNode *node = vtkMRMLTableViewNode::SafeDownCast(anode);
  if (node)
    {
    this->SetDoPropagateTableSelection (node->GetDoPropagateTableSelection());
    }
  else
    {
    vtkErrorMacro("vtkMRMLTableViewNode::Copy failed: invalid input node");
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTableViewNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  os << indent << "DoPropagateTableSelection: " << this->DoPropagateTableSelection << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLTableViewNode::SetTableNodeID(const char* tableNodeId)
{
  this->SetNodeReferenceID(this->GetTableNodeReferenceRole(), tableNodeId);
  // Instead of calling Modified() here, we let OnNodeReference* methods to call
  // modified (this the behavior is correct, even if the references are modified
  // directly, without calling SetTableNodeID)
}

//----------------------------------------------------------------------------
const char* vtkMRMLTableViewNode::GetTableNodeID()
{
  return this->GetNodeReferenceID(this->GetTableNodeReferenceRole());
}

//----------------------------------------------------------------------------
vtkMRMLTableNode* vtkMRMLTableViewNode::GetTableNode()
{
  return vtkMRMLTableNode::SafeDownCast(this->GetNodeReference(this->GetTableNodeReferenceRole()));
}

//----------------------------------------------------------------------------
const char* vtkMRMLTableViewNode::GetTableNodeReferenceRole()
{
  return vtkMRMLTableViewNode::TableNodeReferenceRole;
}

//----------------------------------------------------------------------------
const char* vtkMRMLTableViewNode::GetTableNodeReferenceMRMLAttributeName()
{
  return vtkMRMLTableViewNode::TableNodeReferenceMRMLAttributeName;
}

//----------------------------------------------------------------------------
void vtkMRMLTableViewNode::OnNodeReferenceAdded(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceAdded(reference);
  if (std::string(reference->GetReferenceRole()) == this->GetTableNodeReferenceRole())
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkMRMLTableViewNode::OnNodeReferenceModified(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceModified(reference);
  if (std::string(reference->GetReferenceRole()) == this->GetTableNodeReferenceRole())
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkMRMLTableViewNode::OnNodeReferenceRemoved(vtkMRMLNodeReference *reference)
{
  this->Superclass::OnNodeReferenceRemoved(reference);
  if (std::string(reference->GetReferenceRole()) == this->GetTableNodeReferenceRole())
    {
    this->Modified();
    }
}
