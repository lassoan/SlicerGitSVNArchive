/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLTransformDisplayNode.cxx,v $
Date:      $Date: 2006/03/03 22:26:39 $
Version:   $Revision: 1.3 $

=========================================================================auto=*/

#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include <vtkImageData.h>

#include "vtkDiffusionTensorGlyph.h"

#include "vtkTransformPolyDataFilter.h"

#include "vtkMRMLDiffusionTensorDisplayPropertiesNode.h"
#include "vtkMRMLTransformDisplayNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLDisplayableNode.h"

#include "vtkMRMLVolumeNode.h"

#include <sstream>

const char RegionReferenceRole[] = "region";

const char CONTOUR_LEVEL_SEPARATOR=' ';

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTransformDisplayNode);

//----------------------------------------------------------------------------
vtkMRMLTransformDisplayNode::vtkMRMLTransformDisplayNode()
  :vtkMRMLDisplayNode()
{
  this->VisualizationMode=VIS_MODE_GLYPH;

  this->GlyphSpacingMm=10.0;
  this->GlyphScalePercent=100;
  this->GlyphDisplayRangeMaxMm=1000;
  this->GlyphDisplayRangeMinMm=0;
  this->GlyphMaxNumberOfPoints=2000;
  this->GlyphRandomSeed=687848400;
  this->GlyphType=GLYPH_TYPE_ARROW;
  this->GlyphScaleDirectional=true;
  this->GlyphTipLengthPercent=15;
  this->GlyphDiameterPercent=20;
  this->GlyphDiameterMm=0.5;
  this->GlyphShaftDiameterPercent=15;
  this->GlyphResolution=6;

  this->GridScalePercent=100;
  this->GridSpacingMm=15.0;

  this->ContourResolutionMm=5.0;
  this->ContourLevelsMm.clear();
  this->ContourLevelsMm.push_back(1.0);
  this->ContourLevelsMm.push_back(2.0);
  this->ContourLevelsMm.push_back(3.0);
  this->ContourLevelsMm.push_back(5.0);
}


//----------------------------------------------------------------------------
vtkMRMLTransformDisplayNode::~vtkMRMLTransformDisplayNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);

  of << indent << " VisualizationMode=\""<< ConvertVisualizationModeToString(this->VisualizationMode) << "\"";

  of << indent << " GlyphSpacingMm=\""<< this->GlyphSpacingMm << "\"";
  of << indent << " GlyphScalePercent=\""<< this->GlyphScalePercent << "\"";
  of << indent << " GlyphDisplayRangeMaxMm=\""<< this->GlyphDisplayRangeMaxMm << "\"";
  of << indent << " GlyphDisplayRangeMinMm=\""<< this->GlyphDisplayRangeMinMm << "\"";
  of << indent << " GlyphMaxNumberOfPoints=\""<< this->GlyphMaxNumberOfPoints << "\"";
  of << indent << " GlyphRandomSeed=\""<< this->GlyphRandomSeed << "\"";
  of << indent << " GlyphType=\""<< ConvertGlyphTypeToString(this->GlyphType) << "\"";
  of << indent << " GlyphScaleDirectional=\"" << this->GlyphScaleDirectional << "\"";
  of << indent << " GlyphTipLengthPercent=\"" << this->GlyphTipLengthPercent << "\"";
  of << indent << " GlyphDiameterPercent=\"" << this->GlyphDiameterPercent << "\"";
  of << indent << " GlyphDiameterMm=\""<< this->GlyphDiameterMm << "\"";
  of << indent << " GlyphShaftDiameterPercent=\"" << this->GlyphShaftDiameterPercent << "\"";
  of << indent << " GlyphResolution=\"" << this->GlyphResolution << "\"";

  of << indent << " GridScalePercent=\""<< this->GridScalePercent << "\"";
  of << indent << " GridSpacingMm=\""<< this->GridSpacingMm << "\"";

  of << indent << " ContourResolutionMm=\""<< this->ContourResolutionMm << "\"";
  of << indent << " ContourLevelsMm=\"" << GetContourLevelsMmAsString() << "\"";
}


#define READ_FROM_ATT(varName)    \
  if (!strcmp(attName,#varName))  \
  {                               \
    std::stringstream ss;         \
    ss << attValue;               \
    ss >> this->varName;          \
    continue;                     \
  }


//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName,"VisualizationMode"))
    {
      this->VisualizationMode = ConvertVisualizationModeFromString(attValue);
      continue;
    }
    READ_FROM_ATT(GlyphSpacingMm);
    READ_FROM_ATT(GlyphScalePercent);
    READ_FROM_ATT(GlyphDisplayRangeMaxMm);
    READ_FROM_ATT(GlyphDisplayRangeMinMm);
    READ_FROM_ATT(GlyphMaxNumberOfPoints);
    READ_FROM_ATT(GlyphRandomSeed);
    if (!strcmp(attName,"GlyphType"))
    {
      this->VisualizationMode = ConvertGlyphTypeFromString(attValue);
      continue;
    }
    READ_FROM_ATT(GlyphScaleDirectional);
    READ_FROM_ATT(GlyphTipLengthPercent);
    READ_FROM_ATT(GlyphDiameterPercent);
    READ_FROM_ATT(GlyphDiameterMm);
    READ_FROM_ATT(GlyphShaftDiameterPercent);
    READ_FROM_ATT(GlyphResolution);
    READ_FROM_ATT(GridScalePercent);
    READ_FROM_ATT(GridSpacingMm);
    READ_FROM_ATT(ContourResolutionMm);
    if (!strcmp(attName,"ContourLevelsMm"))
    {
      SetContourLevelsMmFromString(attValue);
      continue;
    }
  }

  this->Modified();
  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLTransformDisplayNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  vtkMRMLTransformDisplayNode *node = vtkMRMLTransformDisplayNode::SafeDownCast(anode);

  this->VisualizationMode = node->VisualizationMode;

  this->GlyphSpacingMm = node->GlyphSpacingMm;
  this->GlyphScalePercent = node->GlyphScalePercent;
  this->GlyphDisplayRangeMaxMm = node->GlyphDisplayRangeMaxMm;
  this->GlyphDisplayRangeMinMm = node->GlyphDisplayRangeMinMm;
  this->GlyphMaxNumberOfPoints = node->GlyphMaxNumberOfPoints;
  this->GlyphRandomSeed = node->GlyphRandomSeed;
  this->GlyphType = node->GlyphType;
  this->GlyphScaleDirectional = node->GlyphScaleDirectional;
  this->GlyphTipLengthPercent = node->GlyphTipLengthPercent;
  this->GlyphDiameterPercent = node->GlyphDiameterPercent;
  this->GlyphDiameterMm = node->GlyphDiameterMm;
  this->GlyphShaftDiameterPercent = node->GlyphShaftDiameterPercent;
  this->GlyphResolution = node->GlyphResolution;

  this->GridScalePercent = node->GridScalePercent;
  this->GridSpacingMm = node->GridSpacingMm;

  this->ContourLevelsMm = node->ContourLevelsMm;
  this->ContourResolutionMm = node->ContourResolutionMm;

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  os << indent << "VisualizationMode = "<< ConvertVisualizationModeToString(this->VisualizationMode) << "\n";
  os << indent << "GlyphScalePercent = "<< this->GlyphScalePercent << "\n";
  os << indent << "GlyphDisplayRangeMaxMm = "<< this->GlyphDisplayRangeMaxMm << "\n";
  os << indent << "GlyphDisplayRangeMinMm = "<< this->GlyphDisplayRangeMinMm << "\n";
  os << indent << "GlyphMaxNumberOfPoints = "<< this->GlyphMaxNumberOfPoints << "\n";
  os << indent << "GlyphRandomSeed = "<< this->GlyphRandomSeed << "\n";
  os << indent << "GlyphType = "<< ConvertGlyphTypeToString(this->GlyphType) << "\n";
  os << indent << "GlyphScaleDirectional = "<< this->GlyphScaleDirectional << "\n";
  os << indent << "GlyphTipLengthPercent = " << this->GlyphTipLengthPercent << "\n";
  os << indent << "GlyphDiameterPercent = " << this->GlyphDiameterPercent << "\n";
  os << indent << "GlyphDiameterMm = " << this->GlyphDiameterMm << "\n";
  os << indent << "GlyphShaftDiameterPercent = " << this->GlyphShaftDiameterPercent << "\n";
  os << indent << "GlyphResolution = " << this->GlyphResolution << "\n";

  os << indent << "GridScalePercent = " << this->GridScalePercent << "\n";
  os << indent << "GridSpacingMm = " << this->GridSpacingMm << "\n";

  os << indent << " ContourResolutionMm = "<< this->ContourResolutionMm << "\n";
  os << indent << " ContourLevelsMm = " << GetContourLevelsMmAsString() << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData )
{
  // Calls "UpdatePolyDataPipeline"
  this->Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLTransformDisplayNode::GetRegionNode()
{
  return this->GetNodeReference(RegionReferenceRole);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::SetAndObserveRegionNode(vtkMRMLNode* node)
{
  if (node)
  {
    this->SetNthNodeReferenceID(RegionReferenceRole,0,node->GetID());
  }
  else
  {
    this->RemoveNthNodeReferenceID(RegionReferenceRole,0);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::SetContourLevelsMm(double* values, int size)
{
  this->ContourLevelsMm.clear();
  for (int i=0; i<size; i++)
  {
    this->ContourLevelsMm.push_back(values[i]);
  }
  Modified();
}

//----------------------------------------------------------------------------
double* vtkMRMLTransformDisplayNode::GetContourLevelsMm()
{
  if (this->ContourLevelsMm.size()==0)
  {
    return NULL;
  }
  // std::vector values are guaranteed to be stored in a continuous block in memory,
  // so we can return the address to the first one
  return &(this->ContourLevelsMm[0]);
}

//----------------------------------------------------------------------------
unsigned int vtkMRMLTransformDisplayNode::GetNumberOfContourLevels()
{
  return this->ContourLevelsMm.size();
}

//----------------------------------------------------------------------------
const char* vtkMRMLTransformDisplayNode::ConvertVisualizationModeToString(int modeIndex)
{
  switch (modeIndex)
  {
  case VIS_MODE_GLYPH: return "GLYPH";
  case VIS_MODE_GRID: return "GRID";
  case VIS_MODE_CONTOUR: return "CONTOUR";
  default: return "";
  }
}

//----------------------------------------------------------------------------
int vtkMRMLTransformDisplayNode::ConvertVisualizationModeFromString(const char* modeString)
{
  if (modeString==NULL)
  {
    return -1;
  }
  for (int modeIndex=0; modeIndex<VIS_MODE_LAST; modeIndex++)
  {
    if (strcmp(modeString,ConvertVisualizationModeToString(modeIndex))==0)
    {
      return modeIndex;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
const char* vtkMRMLTransformDisplayNode::ConvertGlyphTypeToString(int modeIndex)
{
  switch (modeIndex)
  {
  case GLYPH_TYPE_ARROW: return "ARROW";
  case GLYPH_TYPE_CONE: return "CONE";
  case GLYPH_TYPE_SPHERE: return "SPHERE";
  default: return "";
  }
}

//----------------------------------------------------------------------------
int vtkMRMLTransformDisplayNode::ConvertGlyphTypeFromString(const char* modeString)
{
  if (modeString==NULL)
  {
    return -1;
  }
  for (int modeIndex=0; modeIndex<GLYPH_TYPE_LAST; modeIndex++)
  {
    if (strcmp(modeString,ConvertGlyphTypeToString(modeIndex))==0)
    {
      return modeIndex;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
std::string vtkMRMLTransformDisplayNode::GetContourLevelsMmAsString()
{
  return ConvertContourLevelsToString(this->ContourLevelsMm);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::SetContourLevelsMmFromString(const char* str)
{
  std::vector<double> newLevels=ConvertContourLevelsFromString(str);
  if (ContourLevelsEqual(newLevels, this->ContourLevelsMm))
  {
    // no change
    return;
  }
  this->ContourLevelsMm=newLevels;
  Modified();
}

//----------------------------------------------------------------------------
std::vector<double> vtkMRMLTransformDisplayNode::ConvertContourLevelsFromString(const char* str)
{
  std::vector<double> contourLevels;
  std::stringstream ss(str);
  std::string itemString;
  double itemDouble;
  while (std::getline(ss, itemString, CONTOUR_LEVEL_SEPARATOR))
  {
    std::stringstream itemStream;
    itemStream << itemString;
    itemStream >> itemDouble;
    contourLevels.push_back(itemDouble);
  }
  return contourLevels;
}

//----------------------------------------------------------------------------
std::string vtkMRMLTransformDisplayNode::ConvertContourLevelsToString(const std::vector<double>& levels)
{
  std::stringstream ss;
  for (int i=0; i<levels.size(); i++)
  {
    if (i>0)
    {
      ss << CONTOUR_LEVEL_SEPARATOR;
    }
    ss << levels[i];
  }
  return ss.str();
}

//----------------------------------------------------------------------------
bool vtkMRMLTransformDisplayNode::ContourLevelsEqual(const std::vector<double>& levels1, const std::vector<double>& levels2)
{
  if (levels1.size()!=levels2.size())
  {
    return false;
  }
  const double COMPARISON_TOLERANCE=0.01;
  for (int i=0; i<levels1.size(); i++)
  {
    if (fabs(levels1[i]-levels2[i])>COMPARISON_TOLERANCE)
    {
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetContourLevelsMm(std::vector<double> &levels)
{
  levels=this->ContourLevelsMm;
}
