/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLClipModelsNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.3 $

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLClipModelsNode.h"
#include "vtkMRMLSliceNode.h"

// VTK includes
#include <vtkCommand.h>
#include <vtkImplicitBoolean.h>
#include <vtkImplicitFunctionCollection.h>
#include <vtkIntArray.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLClipModelsNode);

static const char* RedClipSliceNodeReferenceRole = "redClipSlice";
static const char* YellowClipSliceNodeReferenceRole = "yellowClipSlice";
static const char* GreenClipSliceNodeReferenceRole = "greenClipSlice";

//----------------------------------------------------------------------------
vtkMRMLClipModelsNode::vtkMRMLClipModelsNode()
{
  this->SetSingletonTag("vtkMRMLClipModelsNode");
  this->HideFromEditors = true;
  this->ClippingMethod = vtkMRMLClipModelsNode::Straight;
  for (int sliceIndex = 0; sliceIndex < NUMBER_OF_SLICE_PLANES; ++sliceIndex)
    {
    this->SliceClipStates[sliceIndex] = vtkMRMLClipModelsNode::ClipOff;
    this->SlicePlanes[sliceIndex] = vtkPlane::New();
    }

  this->SlicePlanesFunction = vtkImplicitBoolean::New();
  this->SlicePlanesFunction->SetOperationTypeToIntersection();
  this->ClipType = vtkMRMLClipModelsNode::ClipIntersection;

  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkCommand::ModifiedEvent);
  this->AddNodeReferenceRole(RedClipSliceNodeReferenceRole, NULL, events.GetPointer());
  this->AddNodeReferenceRole(YellowClipSliceNodeReferenceRole, NULL, events.GetPointer());
  this->AddNodeReferenceRole(GreenClipSliceNodeReferenceRole, NULL, events.GetPointer());

  // Set default clip slice nodes
  this->SetAndObserveRedSliceNodeID("vtkMRMLSliceNodeRed");
  this->SetAndObserveYellowSliceNodeID("vtkMRMLSliceNodeYellow");
  this->SetAndObserveGreenSliceNodeID("vtkMRMLSliceNodeGreen");
}

//----------------------------------------------------------------------------
vtkMRMLClipModelsNode::~vtkMRMLClipModelsNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLClipModelsNode::WriteXML(ostream& of, int nIndent)
{
  // Write all attributes not equal to their defaults

  Superclass::WriteXML(of, nIndent);

  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLIntMacro(redSliceClipState, RedSliceClipState);
  vtkMRMLWriteXMLIntMacro(yellowSliceClipState, YellowSliceClipState);
  vtkMRMLWriteXMLIntMacro(greenSliceClipState, GreenSliceClipState);
  vtkMRMLWriteXMLIntMacro(clipType, ClipType);
  if (this->ClippingMethod != vtkMRMLClipModelsNode::Straight)
    {
    vtkMRMLWriteXMLTypedEnumMacro(clippingMethod, ClippingMethod, ClippingMethodType);
    }
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLClipModelsNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLIntMacro(redSliceClipState, RedSliceClipState);
  vtkMRMLReadXMLIntMacro(yellowSliceClipState, YellowSliceClipState);
  vtkMRMLReadXMLIntMacro(greenSliceClipState, GreenSliceClipState);
  vtkMRMLReadXMLIntMacro(clipType, ClipType);
  vtkMRMLReadXMLTypedEnumMacro(clippingMethod, ClippingMethod, ClippingMethodType);
  vtkMRMLReadXMLEndMacro();

  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLClipModelsNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyIntMacro(RedSliceClipState);
  vtkMRMLCopyIntMacro(YellowSliceClipState);
  vtkMRMLCopyIntMacro(GreenSliceClipState);
  vtkMRMLCopyIntMacro(ClipType);
  vtkMRMLCopyTypedEnumMacro(ClippingMethod, ClippingMethodType);
  vtkMRMLCopyEndMacro();

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLClipModelsNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintIntMacro(ClipType);
  vtkMRMLPrintTypedEnumMacro(ClippingMethod, ClippingMethodType);
  vtkMRMLPrintIntMacro(RedSliceClipState);
  vtkMRMLPrintIntMacro(YellowSliceClipState);
  vtkMRMLPrintIntMacro(GreenSliceClipState);
  vtkMRMLPrintEndMacro();
}

//-----------------------------------------------------------------------------
void vtkMRMLClipModelsNode::SetClipType(int clipType)
{
  if (clipType == this->ClipType)
    {
    // no change
    return;
    }
  if (this->ClipType == vtkMRMLClipModelsNode::ClipIntersection)
    {
    this->SlicePlanesFunction->SetOperationTypeToIntersection();
    }
  else if (this->ClipType == vtkMRMLClipModelsNode::ClipUnion)
    {
    this->SlicePlanesFunction->SetOperationTypeToUnion();
    }
  else
    {
    vtkErrorMacro("vtkMRMLClipModelsNode::SetClipType failed: invalid clipType: " << clipType);
    return;
    }
  this->ClipType = clipType;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkMRMLClipModelsNode::GetClippingMethodFromString(const char* name)
{
  if (name == NULL)
    {
    // invalid name
    return -1;
    }
  if (strcmp(name, "Straight"))
    {
    return (int)Straight;
    }
  else if (strcmp(name, "Whole Cells"))
    {
    return (int)WholeCells;
    }
  else if (strcmp(name, "Whole Cells With Boundary"))
    {
    return (int)WholeCellsWithBoundary;
    }
  // unknown name
  return -1;
}

//-----------------------------------------------------------------------------
const char* vtkMRMLClipModelsNode::GetClippingMethodAsString(ClippingMethodType id)
{
 switch (id)
    {
    case Straight: return "Straight";
    case WholeCells: return "Whole Cells";
    case WholeCellsWithBoundary: return "Whole Cells With Boundary";
    default:
      // invalid id
      return "";
    }
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::SetAndObserveRedSliceNodeID(const char* nodeID)
{
  this->SetAndObserveNodeReferenceID(RedClipSliceNodeReferenceRole, nodeID);
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::SetAndObserveYellowSliceNodeID(const char* nodeID)
{
  this->SetAndObserveNodeReferenceID(YellowClipSliceNodeReferenceRole, nodeID);
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::SetAndObserveGreenSliceNodeID(const char* nodeID)
{
  this->SetAndObserveNodeReferenceID(GreenClipSliceNodeReferenceRole, nodeID);
}

//---------------------------------------------------------------------------
const char* vtkMRMLClipModelsNode::GetRedSliceNodeID()
{
  return this->GetNodeReferenceID(RedClipSliceNodeReferenceRole);
}

//---------------------------------------------------------------------------
const char* vtkMRMLClipModelsNode::GetYellowSliceNodeID()
{
  return this->GetNodeReferenceID(YellowClipSliceNodeReferenceRole);
}

//---------------------------------------------------------------------------
const char* vtkMRMLClipModelsNode::GetGreenSliceNodeID()
{
  return this->GetNodeReferenceID(GreenClipSliceNodeReferenceRole);
}

//---------------------------------------------------------------------------
vtkMRMLSliceNode* vtkMRMLClipModelsNode::GetRedSliceNode()
{
  return vtkMRMLSliceNode::SafeDownCast(this->GetNodeReference(RedClipSliceNodeReferenceRole));
}

//---------------------------------------------------------------------------
vtkMRMLSliceNode* vtkMRMLClipModelsNode::GetGreenSliceNode()
{
  return vtkMRMLSliceNode::SafeDownCast(this->GetNodeReference(GreenClipSliceNodeReferenceRole));
}

//---------------------------------------------------------------------------
vtkMRMLSliceNode*  vtkMRMLClipModelsNode::GetYellowSliceNode()
{
  return vtkMRMLSliceNode::SafeDownCast(this->GetNodeReference(YellowClipSliceNodeReferenceRole));
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::SetSliceClipState(int sliceIndex, int state)
{
  if (state == this->SliceClipStates[sliceIndex])
    {
    // no change
    return;
    }
  this->SliceClipStates[sliceIndex] = state;
  vtkPlane* slicePlane = this->SlicePlanes[sliceIndex];
  vtkImplicitFunctionCollection* planes = this->SlicePlanesFunction->GetFunction();
  if (state == vtkMRMLClipModelsNode::ClipOff)
    {
    if (planes->IsItemPresent(slicePlane))
      {
      this->SlicePlanesFunction->RemoveFunction(slicePlane);
      }
    }
  else
    {
    if (!planes->IsItemPresent(slicePlane))
      {
      this->SlicePlanesFunction->AddFunction(slicePlane);
      }
    }
  this->Modified();
}

//---------------------------------------------------------------------------
int vtkMRMLClipModelsNode::GetRedSliceClipState()
{
  return this->SliceClipStates[vtkMRMLClipModelsNode::RED_SLICE];
}

//---------------------------------------------------------------------------
int vtkMRMLClipModelsNode::GetYellowSliceClipState()
{
  return this->SliceClipStates[vtkMRMLClipModelsNode::YELLOW_SLICE];
}

//---------------------------------------------------------------------------
int vtkMRMLClipModelsNode::GetGreenSliceClipState()
{
  return this->SliceClipStates[vtkMRMLClipModelsNode::GREEN_SLICE];
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::SetRedSliceClipState(int state)
{
  this->SetSliceClipState(vtkMRMLClipModelsNode::RED_SLICE, state);
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::SetYellowSliceClipState(int state)
{
  this->SetSliceClipState(vtkMRMLClipModelsNode::YELLOW_SLICE, state);
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::SetGreenSliceClipState(int state)
{
  this->SetSliceClipState(vtkMRMLClipModelsNode::GREEN_SLICE, state);
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::UpdateClipPlanePose(vtkPlane* slicePlane, vtkMatrix4x4* sliceMatrix, int clipState)
{
  slicePlane->SetOrigin(sliceMatrix->GetElement(0, 3),
    sliceMatrix->GetElement(1, 3),
    sliceMatrix->GetElement(2, 3));
  if (clipState == vtkMRMLClipModelsNode::ClipNegativeSpace)
    {
    slicePlane->SetNormal(-sliceMatrix->GetElement(0, 2),
      -sliceMatrix->GetElement(1, 2),
      -sliceMatrix->GetElement(2, 2));
    }
  else
    {
    slicePlane->SetNormal(sliceMatrix->GetElement(0, 2),
      sliceMatrix->GetElement(1, 2),
      sliceMatrix->GetElement(2, 2));
    }
}

//---------------------------------------------------------------------------
bool vtkMRMLClipModelsNode::IsClipping()
{
  for (int sliceIndex = 0; sliceIndex < vtkMRMLClipModelsNode::NUMBER_OF_SLICE_PLANES; ++sliceIndex)
    {
    if (this->SliceClipStates[sliceIndex] != vtkMRMLClipModelsNode::ClipOff)
      {
      return true;
      }
    }
  return false;
}

//---------------------------------------------------------------------------
vtkMRMLSliceNode* vtkMRMLClipModelsNode::GetSliceNode(int sliceIndex)
{
  switch (sliceIndex)
    {
    case vtkMRMLClipModelsNode::RED_SLICE:
      return this->GetRedSliceNode();
    case vtkMRMLClipModelsNode::YELLOW_SLICE:
      return this->GetYellowSliceNode();
    case vtkMRMLClipModelsNode::GREEN_SLICE:
      return this->GetGreenSliceNode();
    }
  return NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData)
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  // Emit a node modified event if the lookup table object is modified
  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(caller);
  if (sliceNode != NULL && event == vtkCommand::ModifiedEvent)
    {
    for (int sliceIndex = 0; sliceIndex < vtkMRMLClipModelsNode::NUMBER_OF_SLICE_PLANES; ++sliceIndex)
      {
      if (this->GetSliceNode(sliceIndex) != sliceNode)
        {
        continue;
        }
      if (this->SliceClipStates[sliceIndex] != vtkMRMLClipModelsNode::ClipOff)
        {
        vtkPlane* slicePlane = this->SlicePlanes[sliceIndex];
        vtkMatrix4x4* sliceMatrix = sliceNode->GetSliceToRAS();
        this->UpdateClipPlanePose(slicePlane, sliceMatrix, this->SliceClipStates[sliceIndex]);
        this->InvokeCustomModifiedEvent(vtkMRMLClipModelsNode::ClipPlanePoseModifiedEvent);
        }
      return;
      }
    }
}

//---------------------------------------------------------------------------
vtkImplicitBoolean* vtkMRMLClipModelsNode::GetClippingFunction()
{
  return this->SlicePlanesFunction;
}

//---------------------------------------------------------------------------
void vtkMRMLClipModelsNode::GetTransformedClippingFunction(vtkMatrix4x4* transformToWorld, vtkImplicitBoolean* clippingFunction)
{
  if (!clippingFunction || !transformToWorld)
    {
    return;
    }

  if (this->ClipType == vtkMRMLClipModelsNode::ClipIntersection)
    {
    clippingFunction->SetOperationTypeToIntersection();
    }
  else if (this->ClipType == vtkMRMLClipModelsNode::ClipUnion)
    {
    clippingFunction->SetOperationTypeToUnion();
    }

  vtkNew<vtkMatrix4x4> transformFromWorld;
  vtkMatrix4x4::Invert(transformToWorld, transformFromWorld.GetPointer());
  for (int sliceIndex = 0; sliceIndex < NUMBER_OF_SLICE_PLANES; ++sliceIndex)
    {
    if (this->SliceClipStates[sliceIndex] == vtkMRMLClipModelsNode::ClipOff)
      {
      continue;
      }
    vtkNew<vtkPlane> plane;
    vtkMRMLSliceNode* sliceNode = this->GetSliceNode(sliceIndex);
    vtkMatrix4x4* sliceMatrix = sliceNode->GetSliceToRAS();
    vtkNew<vtkMatrix4x4> sliceToNode;
    vtkMatrix4x4::Multiply4x4(transformFromWorld.GetPointer(), sliceMatrix, sliceToNode.GetPointer());
    this->UpdateClipPlanePose(plane.GetPointer(), sliceToNode.GetPointer(), this->SliceClipStates[sliceIndex]);
    clippingFunction->AddFunction(plane.GetPointer());
    }
}