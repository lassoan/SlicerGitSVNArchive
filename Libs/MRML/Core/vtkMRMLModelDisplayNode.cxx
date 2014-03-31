/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLModelDisplayNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.3 $

=========================================================================auto=*/
// STL includes
#include <sstream>

// MRML includes
#include "vtkMRMLModelDisplayNode.h"

// VTK includes
#include <vtkAlgorithmOutput.h>
#include <vtkAssignAttribute.h>
#include <vtkCommand.h>
#include <vtkObjectFactory.h>
#include <vtkPassThrough.h>
#include <vtkPolyData.h>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLModelDisplayNode);

//-----------------------------------------------------------------------------
vtkMRMLModelDisplayNode::vtkMRMLModelDisplayNode()
{
  this->OutputPolyDataRAS = false;
  this->PassThrough = vtkPassThrough::New();
  this->AssignAttribute = vtkAssignAttribute::New();
  // Be careful, virtualization doesn't work in constructors
  this->UpdatePolyDataPipeline();
}

//-----------------------------------------------------------------------------
vtkMRMLModelDisplayNode::~vtkMRMLModelDisplayNode()
{
  this->PassThrough->Delete();
  this->AssignAttribute->Delete();
}

//----------------------------------------------------------------------------
void vtkMRMLModelDisplayNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);
  of << indent << " outputPolyDataRAS=\"" << this->OutputPolyDataRAS << "\"";
}


//----------------------------------------------------------------------------
void vtkMRMLModelDisplayNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;

  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp (attName, "outputPolyDataRAS"))
      {
      std::stringstream ss;
      ss << attValue;
      ss >> this->OutputPolyDataRAS;
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLModelDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  os << indent << "OutputPolyDataRAS: " << this->OutputPolyDataRAS << "\n";
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, LabelText, ID
void vtkMRMLModelDisplayNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  vtkMRMLModelDisplayNode *node = vtkMRMLModelDisplayNode::SafeDownCast(anode);
  if (anode==NULL)
  {
    vtkErrorMacro("vtkMRMLModelDisplayNode::Copy failed: invalid source node type");
    return;
  }
  this->SetOutputPolyDataRAS(node->OutputPolyDataRAS);
  this->EndModify(disabledModify);
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayNode::ProcessMRMLEvents(vtkObject *caller,
                                                unsigned long event,
                                                void *callData )
{
  this->Superclass::ProcessMRMLEvents(caller, event, callData);

  if (event == vtkCommand::ModifiedEvent)
    {
    this->UpdatePolyDataPipeline();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayNode::SetInputPolyData(vtkPolyData* polyData)
{
   if (this->GetInputPolyData() == polyData)
     {
     return;
     }
   this->SetInputToPolyDataPipeline(polyData);
   this->Modified();
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayNode::SetInputToPolyDataPipeline(vtkPolyData* polyData)
{
  this->PassThrough->SetInput(polyData);
  this->AssignAttribute->SetInput(polyData);
}

//---------------------------------------------------------------------------
vtkPolyData* vtkMRMLModelDisplayNode::GetInputPolyData()
{
  return vtkPolyData::SafeDownCast(this->AssignAttribute->GetInput());
}

//---------------------------------------------------------------------------
vtkPolyData* vtkMRMLModelDisplayNode::GetOutputPolyData()
{
  if (!this->GetOutputPort())
    {
    return 0;
    }
  if (!this->GetInputPolyData())
    {
    return 0;
    }
  return vtkPolyData::SafeDownCast(
    this->GetOutputPort()->GetProducer()->GetOutputDataObject(
      this->GetOutputPort()->GetIndex()));
}

//---------------------------------------------------------------------------
vtkAlgorithmOutput* vtkMRMLModelDisplayNode::GetOutputPort()
{
  if (this->GetActiveScalarName())
    {
    return this->AssignAttribute->GetOutputPort();
    }
  else
    {
    return this->PassThrough->GetOutputPort();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayNode::SetActiveScalarName(const char *scalarName)
{
  int wasModifying = this->StartModify();
  this->Superclass::SetActiveScalarName(scalarName);
  this->UpdatePolyDataPipeline();
  this->EndModify(wasModifying);
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayNode::SetActiveAttributeLocation(int location)
{
  int wasModifying = this->StartModify();
  this->Superclass::SetActiveAttributeLocation(location);
  this->UpdatePolyDataPipeline();
  this->EndModify(wasModifying);
}

//---------------------------------------------------------------------------
void vtkMRMLModelDisplayNode::UpdatePolyDataPipeline()
{
  this->AssignAttribute->Assign(
    this->GetActiveScalarName(),
    this->GetActiveScalarName() ? vtkDataSetAttributes::SCALARS : -1,
    this->GetActiveAttributeLocation());
  if (this->GetAutoScalarRange() && this->GetOutputPolyData())
    {
    this->GetOutputPolyData()->Update();
    this->SetScalarRange(this->GetOutputPolyData()->GetScalarRange());
    }
}
