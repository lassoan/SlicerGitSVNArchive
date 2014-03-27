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

const char InputReferenceRole[]="inputNode";
const char ReferenceVolumeReferenceRole[] = "referenceVolume";
const char OutputModelReferenceRole[] = "outputModel";


//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTransformDisplayNode);

//----------------------------------------------------------------------------
vtkMRMLTransformDisplayNode::vtkMRMLTransformDisplayNode()
  :vtkMRMLDisplayNode()
{

  this->VisualizationMode=0;

  //Glyph Parameters
  this->GlyphSpacingMm = 1;
  this->GlyphScale = 1;
  this->GlyphThresholdMax = 1000;
  this->GlyphThresholdMin = 0;
  this->GlyphPointMax = 2000;
  this->GlyphSeed = 687848400;
  this->GlyphSourceOption = 0;
  //Arrow Parameters
  this->GlyphArrowScaleDirectional = true;
  this->GlyphArrowScaleIsotropic = false;
  this->GlyphArrowTipLength = 0.35;
  this->GlyphArrowTipRadius = 0.5;
  this->GlyphArrowShaftRadius = 0.15;
  this->GlyphArrowResolution = 6;
  //Cone Parameters
  this->GlyphConeScaleDirectional = true;
  this->GlyphConeScaleIsotropic = false;
  this->GlyphConeHeight = 1.0;
  this->GlyphConeRadius = 0.6;
  this->GlyphConeResolution = 6;
  //Sphere Parameters
  this->GlyphSphereResolution = 6;

  //Grid Parameters
  this->GridScale = 1;
  this->GridSpacingMm = 12;

  //Contour Parameters
  this->ContourValues.clear();
  this->ContourValues.push_back(1);
  this->ContourValues.push_back(2);
  this->ContourValues.push_back(3);
  this->ContourValues.push_back(4);
  this->ContourDecimation = 0.25;
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

  of << indent << " Show2dGlyph=\""<< (this->VisualizationMode&VIS_MODE_2D_GLYPH?1:0) << "\"";
  of << indent << " Show2dGrid=\""<< (this->VisualizationMode&VIS_MODE_2D_GRID?1:0) << "\"";
  of << indent << " Show2dContour=\""<< (this->VisualizationMode&VIS_MODE_2D_CONTOUR?1:0) << "\"";
  of << indent << " Show3dGlyph=\""<< (this->VisualizationMode&VIS_MODE_3D_GLYPH?1:0) << "\"";
  of << indent << " Show3dGrid=\""<< (this->VisualizationMode&VIS_MODE_3D_GRID?1:0) << "\"";
  of << indent << " Show3dContour=\""<< (this->VisualizationMode&VIS_MODE_3D_CONTOUR?1:0) << "\"";

  of << indent << " GlyphSpacingMm=\""<< this->GlyphSpacingMm << "\"";
  of << indent << " GlyphPointMax=\""<< this->GlyphPointMax << "\"";
  of << indent << " GlyphScale=\""<< this->GlyphScale << "\"";
  of << indent << " GlyphThresholdMax=\""<< this->GlyphThresholdMax << "\"";
  of << indent << " GlyphThresholdMin=\""<< this->GlyphThresholdMin << "\"";
  of << indent << " GlyphSeed=\""<< this->GlyphSeed << "\"";
  of << indent << " GlyphSourceOption=\""<< this->GlyphSourceOption << "\"";
    of << indent << " GlyphArrowScaleDirectional=\"" << this->GlyphArrowScaleDirectional << "\"";
    of << indent << " GlyphArrowScaleIsotropic=\"" << this->GlyphArrowScaleIsotropic << "\"";
    of << indent << " GlyphArrowTipLength=\"" << this->GlyphArrowTipLength << "\"";
    of << indent << " GlyphArrowTipRadius=\""<< this->GlyphArrowTipRadius << "\"";
    of << indent << " GlyphArrowShaftRadius=\"" << this->GlyphArrowShaftRadius << "\"";
    of << indent << " GlyphArrowResolution=\"" << this->GlyphArrowResolution << "\"";
    of << indent << " GlyphConeScaleDirectional=\"" << this->GlyphConeScaleDirectional << "\"";
    of << indent << " GlyphConeScaleIsotropic=\"" << this->GlyphConeScaleIsotropic << "\"";
    of << indent << " GlyphConeHeight=\"" << this->GlyphConeHeight << "\"";
    of << indent << " GlyphConeRadius=\"" << this->GlyphConeRadius << "\"";
    of << indent << " GlyphConeResolution=\"" << this->GlyphConeResolution << "\"";
    of << indent << " GlyphSphereResolution=\"" << this->GlyphSphereResolution << "\"";

  of << indent << " GridScale=\""<< this->GridScale << "\"";
  of << indent << " GridSpacingMm=\""<< this->GridSpacingMm << "\"";

  of << indent << " ContourValues=\"";
  for (int i=0; i<this->ContourValues.size(); i++)
  {
    if (i>0)
    {
      of << " "; //separate numbers by a space
    }
    of << this->ContourValues[i];
  }
  of << "\"";
  of << indent << " ContourDecimation=\""<< this->ContourDecimation << "\"";
}


//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL){
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName,"GlyphSpacingMm")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->GridSpacingMm;
      continue;
    }
    if (!strcmp(attName,"GlyphPointMax")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->GlyphPointMax;
      continue;
    }
    if (!strcmp(attName,"GlyphScale")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->GlyphScale;
      continue;
    }
    if (!strcmp(attName,"GlyphThresholdMax")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->GlyphThresholdMax;
      continue;
    }
    if (!strcmp(attName,"GlyphThresholdMin")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->GlyphThresholdMin;
      continue;
    }
    if (!strcmp(attName,"GlyphSeed")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->GlyphSeed;
      continue;
    }
    if (!strcmp(attName,"GlyphSourceOption")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->GlyphSourceOption;
      continue;
    }
      if (!strcmp(attName,"GlyphArrowScaleDirectional")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphArrowScaleDirectional;
        continue;
      }
      if (!strcmp(attName,"GlyphArrowScaleIsotropic")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphArrowScaleIsotropic;
        continue;
      }
      if (!strcmp(attName,"GlyphArrowTipLength")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphArrowTipLength;
        continue;
      }
      if (!strcmp(attName,"GlyphArrowTipRadius")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphArrowTipRadius;
        continue;
      }
      if (!strcmp(attName,"GlyphArrowShaftRadius")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphArrowShaftRadius;
        continue;
      }
      if (!strcmp(attName,"GlyphArrowResolution")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphArrowResolution;
        continue;
      }

      if (!strcmp(attName,"GlyphConeScaleDirectional")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphConeScaleDirectional;
        continue;
      }
      if (!strcmp(attName,"GlyphConeScaleIsotropic")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphConeScaleIsotropic;
        continue;
      }
      if (!strcmp(attName,"GlyphConeHeight")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphConeHeight;
        continue;
      }
      if (!strcmp(attName,"GlyphConeRadius")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphConeRadius;
        continue;
      }
      if (!strcmp(attName,"GlyphConeResolution")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphConeResolution;
        continue;
      }

      if (!strcmp(attName,"GlyphSphereResolution")){
        std::stringstream ss;
        ss << attValue;
        ss >> this->GlyphSphereResolution;
        continue;
      }

    if (!strcmp(attName,"GridScale")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->GridScale;
      continue;
    }
    if (!strcmp(attName,"GridSpacingMm")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->GridSpacingMm;
      continue;
    }

    if (!strcmp(attName,"ContourValues")){
      std::stringstream ss(attValue);
      std::string itemString;
      double itemDouble;
      const char delim=' ';
      this->ContourValues.clear();
      while (std::getline(ss, itemString, delim))
      {
        std::stringstream itemStream;
        itemStream << itemString;
        itemStream >> itemDouble;
        this->ContourValues.push_back(itemDouble);
      }
      continue;
    }
    if (!strcmp(attName,"ContourDecimation")){
      std::stringstream ss;
      ss << attValue;
      ss >> this->ContourDecimation;
      continue;
    }
  }

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
  this->GlyphPointMax = node->GlyphPointMax;
  this->GlyphThresholdMax = node->GlyphThresholdMax;
  this->GlyphThresholdMin = node->GlyphThresholdMin;
  this->GlyphSeed = node->GlyphSeed;
  this->GlyphSourceOption = node->GlyphSourceOption;
  this->GlyphArrowScaleDirectional = node->GlyphArrowScaleDirectional;
  this->GlyphArrowScaleIsotropic = node->GlyphArrowScaleIsotropic;
  this->GlyphArrowTipLength = node->GlyphArrowTipLength;
  this->GlyphArrowTipRadius = node->GlyphArrowTipRadius;
  this->GlyphArrowShaftRadius = node->GlyphArrowShaftRadius;
  this->GlyphArrowResolution = node->GlyphArrowResolution;
  this->GlyphConeScaleDirectional = node->GlyphConeScaleDirectional;
  this->GlyphConeScaleIsotropic = node->GlyphConeScaleIsotropic;
  this->GlyphConeHeight = node->GlyphConeHeight;
  this->GlyphConeRadius = node->GlyphConeRadius;
  this->GlyphConeResolution = node->GlyphConeResolution;
  this->GlyphSphereResolution = node->GlyphSphereResolution;

  this->GridScale = node->GridScale;
  this->GridSpacingMm = node->GridSpacingMm;

  this->ContourValues = node->ContourValues;
  this->ContourDecimation = node->ContourDecimation;

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  vtkIndent nextIndent=indent.GetNextIndent();

  os << indent << " GlyphSpacingMm = "<< this->GlyphSpacingMm << "\n";
  os << indent << " GlyphPointMax = "<< this->GlyphPointMax << "\n";
  os << indent << " GlyphScale = "<< this->GlyphScale << "\n";
  os << indent << " GlyphThresholdMax = "<< this->GlyphThresholdMax << "\n";
  os << indent << " GlyphThresholdMin = "<< this->GlyphThresholdMin << "\n";
  os << indent << " GlyphSeed = "<< this->GlyphSeed << "\n";
  os << indent << " GlyphSourceOption = "<< this->GlyphSourceOption << "\n";
  os << nextIndent << "GlyphArrowScaleDirectional = " << this->GlyphArrowScaleDirectional << "\n";
  os << nextIndent << "GlyphArrowScaleIsotropic =  " << this->GlyphArrowScaleIsotropic << "\n";
  os << nextIndent << "GlyphArrowTipLength = " << this->GlyphArrowTipLength << "\n";
  os << nextIndent << "GlyphArrowTipRadius = "<< this->GlyphArrowTipRadius << "\n";
  os << nextIndent << "GlyphArrowShaftRadius =  " << this->GlyphArrowShaftRadius << "\n";
  os << nextIndent << "GlyphArrowResolution = " << this->GlyphArrowResolution << "\n";
  os << nextIndent << "GlyphConeScaleDirectional = " << this->GlyphConeScaleDirectional << "\n";
  os << nextIndent << "GlyphConeScaleIsotropic =  " << this->GlyphConeScaleIsotropic << "\n";
  os << nextIndent << "GlyphConeHeight = " << this->GlyphConeHeight << "\n";
  os << nextIndent << "GlyphConeRadius = " << this->GlyphConeRadius << "\n";
  os << nextIndent << "GlyphConeResolution = " << this->GlyphConeResolution << "\n";
  os << nextIndent << "GlyphSphereResolution = " << this->GlyphSphereResolution << "\n";

  os << indent << " GridScale = "<< this->GridScale << "\n";
  os << indent << " GridSpacingMm = "<< this->GridSpacingMm << "\n";

  os << indent << " ContourValues = \"";
  for (int i=0; i<this->ContourValues.size(); i++)
  {
    if (i>0)
    {
      os << " "; //separate numbers by a space
    }
    os << this->ContourValues[i];
  }
  os << "\"";

  os << indent << " ContourDecimation = "<< this->ContourDecimation << "\n";

}

//---------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData )
{
  // Calls "UpdatePolyDataPipeline"
  this->Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
vtkMRMLVolumeNode* vtkMRMLTransformDisplayNode::GetReferenceVolumeNode()
{
  return vtkMRMLVolumeNode::SafeDownCast(this->GetNodeReference(ReferenceVolumeReferenceRole));
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::SetAndObserveReferenceVolumeNode(vtkMRMLNode* node)
{
  this->SetNthNodeReferenceID(ReferenceVolumeReferenceRole,0,node->GetID());
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::SetContourValues(double* values, int size)
{
  this->ContourValues.clear();
  for (int i=0; i<size; i++)
  {
    this->ContourValues.push_back(values[i]);
  }
}

//----------------------------------------------------------------------------
double* vtkMRMLTransformDisplayNode::GetContourValues()
{
  // std::vector values are guaranteed to be stored in a continuous block in memory,
  // so we can return the address to the first one
  return &(this->ContourValues[0]);
}

//----------------------------------------------------------------------------
unsigned int vtkMRMLTransformDisplayNode::GetNumberOfContourValues()
{
  return this->ContourValues.size();
}