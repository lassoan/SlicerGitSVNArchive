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

#include "vtkMRMLTransformDisplayNode.h"

#include "vtkMRMLModelNode.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLVolumeNode.h"

#include "vtkTransformVisualizerGlyph3D.h"

#include "vtkAbstractTransform.h"
#include "vtkArrowSource.h"
#include "vtkConeSource.h"
#include "vtkContourFilter.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkGeneralTransform.h"
#include "vtkGlyphSource2D.h"
#include "vtkImageData.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRibbonFilter.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTubeFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVectorNorm.h"
#include "vtkWarpVector.h"

#include <sstream>

const char RegionReferenceRole[] = "region";

const char CONTOUR_LEVEL_SEPARATOR=' ';


//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTransformDisplayNode);

//----------------------------------------------------------------------------
vtkMRMLTransformDisplayNode::vtkMRMLTransformDisplayNode()
  :vtkMRMLModelDisplayNode()
{
  // Don't show transform nodes by default
  // to allow the users to adjust visualization parameters first
  this->Visibility = 0;
  this->SliceIntersectionVisibility = 0;

  this->OutputPolyDataRAS = true;

  this->VisualizationMode=VIS_MODE_GLYPH;

  this->GlyphSpacingMm=10.0;
  this->GlyphScalePercent=100;
  this->GlyphDisplayRangeMaxMm=100;
  this->GlyphDisplayRangeMinMm=0.01;
  this->GlyphType=GLYPH_TYPE_ARROW;
  this->GlyphScaleDirectional=true;
  this->GlyphTipLengthPercent=30;
  this->GlyphDiameterPercent=20;
  this->GlyphDiameterMm=5.0;
  this->GlyphShaftDiameterPercent=40;
  this->GlyphResolution=6;

  this->GridScalePercent=100;
  this->GridSpacingMm=15.0;
  this->GridLineDiameterMm=1.0;

  this->ContourResolutionMm=5.0;

  this->ContourLevelsMm.clear();
  for (double level=2.0; level<20.0; level+=2.0)
  {
    this->ContourLevelsMm.push_back(level);
  }

  this->CachedPolyData3d=vtkPolyData::New();
}


//----------------------------------------------------------------------------
vtkMRMLTransformDisplayNode::~vtkMRMLTransformDisplayNode()
{
  this->CachedPolyData3d->Delete();
  this->CachedPolyData3d=NULL;
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
  of << indent << " GlyphType=\""<< ConvertGlyphTypeToString(this->GlyphType) << "\"";
  of << indent << " GlyphScaleDirectional=\"" << this->GlyphScaleDirectional << "\"";
  of << indent << " GlyphTipLengthPercent=\"" << this->GlyphTipLengthPercent << "\"";
  of << indent << " GlyphDiameterPercent=\"" << this->GlyphDiameterPercent << "\"";
  of << indent << " GlyphDiameterMm=\""<< this->GlyphDiameterMm << "\"";
  of << indent << " GlyphShaftDiameterPercent=\"" << this->GlyphShaftDiameterPercent << "\"";
  of << indent << " GlyphResolution=\"" << this->GlyphResolution << "\"";

  of << indent << " GridScalePercent=\""<< this->GridScalePercent << "\"";
  of << indent << " GridSpacingMm=\""<< this->GridSpacingMm << "\"";
  of << indent << " GridLineDiameterMm=\""<< this->GridLineDiameterMm << "\"";

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
    if (!strcmp(attName,"GlyphType"))
    {
      this->GlyphType = ConvertGlyphTypeFromString(attValue);
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
    READ_FROM_ATT(GridLineDiameterMm);
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
  this->GlyphType = node->GlyphType;
  this->GlyphScaleDirectional = node->GlyphScaleDirectional;
  this->GlyphTipLengthPercent = node->GlyphTipLengthPercent;
  this->GlyphDiameterPercent = node->GlyphDiameterPercent;
  this->GlyphDiameterMm = node->GlyphDiameterMm;
  this->GlyphShaftDiameterPercent = node->GlyphShaftDiameterPercent;
  this->GlyphResolution = node->GlyphResolution;

  this->GridScalePercent = node->GridScalePercent;
  this->GridSpacingMm = node->GridSpacingMm;
  this->GridLineDiameterMm = node->GridLineDiameterMm;

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
  os << indent << "GlyphType = "<< ConvertGlyphTypeToString(this->GlyphType) << "\n";
  os << indent << "GlyphScaleDirectional = "<< this->GlyphScaleDirectional << "\n";
  os << indent << "GlyphTipLengthPercent = " << this->GlyphTipLengthPercent << "\n";
  os << indent << "GlyphDiameterPercent = " << this->GlyphDiameterPercent << "\n";
  os << indent << "GlyphDiameterMm = " << this->GlyphDiameterMm << "\n";
  os << indent << "GlyphShaftDiameterPercent = " << this->GlyphShaftDiameterPercent << "\n";
  os << indent << "GlyphResolution = " << this->GlyphResolution << "\n";

  os << indent << "GridScalePercent = " << this->GridScalePercent << "\n";
  os << indent << "GridSpacingMm = " << this->GridSpacingMm << "\n";
  os << indent << "GridLineDiameterMm = " << this->GridLineDiameterMm << "\n";

  os << indent << " ContourResolutionMm = "<< this->ContourResolutionMm << "\n";
  os << indent << " ContourLevelsMm = " << GetContourLevelsMmAsString() << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData )
{
  if (caller!=NULL
    && (event==vtkCommand::ModifiedEvent || event==vtkMRMLTransformableNode::TransformModifiedEvent)
    && caller==GetRegionNode())
  {
    // update visualization if the region node is changed
    // Note: this updates all the 2D views as well, so instead of a generic modified event a separate
    // even for 2D and 3D views could be useful
    this->Modified();
  }
  else this->Superclass::ProcessMRMLEvents(caller, event, callData);
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
    vtkNew<vtkIntArray> events;
    events->InsertNextValue(vtkCommand::ModifiedEvent);
    events->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    this->SetAndObserveNthNodeReferenceID(RegionReferenceRole,0,node->GetID(),events.GetPointer());
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

//----------------------------------------------------------------------------
vtkPolyData* vtkMRMLTransformDisplayNode::GetOutputPolyData()
{
  vtkMRMLTransformNode* transformNode=GetTransformNode();
  if (transformNode==NULL)
  {
    return NULL;
  }
  vtkMRMLNode* regionNode=this->GetRegionNode();
  if (regionNode==NULL)
  {
    return NULL;
  }

  if (this->CachedPolyData3d->GetMTime()<this->GetMTime()
    || this->CachedPolyData3d->GetMTime()<transformNode->GetMTime()
    || this->CachedPolyData3d->GetMTime()<transformNode->GetTransformToWorldMTime()
    || this->CachedPolyData3d->GetMTime()<regionNode->GetMTime())
  {
    // cached polydata is obsolete, recompute it now
    vtkNew<vtkMatrix4x4> ijkToRAS;
    int regionSize_IJK[3]={0};
    vtkMRMLSliceNode* sliceNode=vtkMRMLSliceNode::SafeDownCast(regionNode);
    vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(regionNode);
    if (sliceNode!=NULL)
    {
      double pointSpacing=this->GetGlyphSpacingMm();

      vtkMatrix4x4* sliceToRAS=sliceNode->GetSliceToRAS();
      double* fieldOfViewSize=sliceNode->GetFieldOfView();
      double* fieldOfViewOrigin=sliceNode->GetXYZOrigin();

      int numOfPointsX=floor(fieldOfViewSize[0]/pointSpacing+0.5);
      int numOfPointsY=floor(fieldOfViewSize[1]/pointSpacing+0.5);
      double xOfs = -fieldOfViewSize[0]/2+fieldOfViewOrigin[0];
      double yOfs = -fieldOfViewSize[1]/2+fieldOfViewOrigin[1];

      ijkToRAS->DeepCopy(sliceToRAS);
      vtkNew<vtkMatrix4x4> ijkOffset;
      ijkOffset->Element[0][3]=xOfs;
      ijkOffset->Element[1][3]=yOfs;
      vtkMatrix4x4::Multiply4x4(ijkToRAS.GetPointer(),ijkOffset.GetPointer(),ijkToRAS.GetPointer());
      vtkNew<vtkMatrix4x4> voxelSpacing;
      voxelSpacing->Element[0][0]=pointSpacing;
      voxelSpacing->Element[1][1]=pointSpacing;
      voxelSpacing->Element[2][2]=pointSpacing;
      vtkMatrix4x4::Multiply4x4(ijkToRAS.GetPointer(),voxelSpacing.GetPointer(),ijkToRAS.GetPointer());

      regionSize_IJK[0]=numOfPointsX;
      regionSize_IJK[1]=numOfPointsY;
      regionSize_IJK[2]=0;
    }
    else if (displayableNode!=NULL)
    {
      double bounds_RAS[6]={0};
      displayableNode->GetRASBounds(bounds_RAS);
      ijkToRAS->SetElement(0,3,bounds_RAS[0]);
      ijkToRAS->SetElement(1,3,bounds_RAS[2]);
      ijkToRAS->SetElement(2,3,bounds_RAS[4]);
      regionSize_IJK[0]=floor(bounds_RAS[1]-bounds_RAS[0]);
      regionSize_IJK[1]=floor(bounds_RAS[3]-bounds_RAS[2]);
      regionSize_IJK[2]=floor(bounds_RAS[5]-bounds_RAS[4]);
    }
    else
    {
      vtkWarningMacro("Failed to show transform in 3D: unsupported ROI type");
      return NULL;
    }
    GetVisualization3d(this->CachedPolyData3d, ijkToRAS.GetPointer(), regionSize_IJK);
  }
  else
  {
    //vtkDebugMacro("Update was not needed"); TODO: this can be removed, used only for testing
  }

  return this->CachedPolyData3d;
}


//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetTransformedPointSamples(vtkPointSet* outputPointSet, vtkMatrix4x4* gridToRAS, int* gridSize)
{
  vtkMRMLTransformNode* inputTransformNode=GetTransformNode();
  if (!inputTransformNode)
  {
    return;
  }

  int numOfSamples=gridSize[0]*gridSize[1]*gridSize[2];

  //Will contain all the points that are to be rendered
  vtkSmartPointer<vtkPoints> samplePositions_RAS = vtkSmartPointer<vtkPoints>::New();
  samplePositions_RAS->SetNumberOfPoints(numOfSamples);

  //Will contain the corresponding vectors for outputPointSet
  vtkSmartPointer<vtkDoubleArray> sampleVectors_RAS = vtkSmartPointer<vtkDoubleArray>::New();
  sampleVectors_RAS->Initialize();
  sampleVectors_RAS->SetNumberOfComponents(3);
  sampleVectors_RAS->SetNumberOfTuples(numOfSamples);
  sampleVectors_RAS->SetName("DisplacementVector");

  vtkSmartPointer<vtkGeneralTransform> inputTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  inputTransformNode->GetTransformToWorld(inputTransform);

  double point_RAS[4] = {0,0,0,1};
  double transformedPoint_RAS[4] = {0,0,0,1};
  double pointDislocationVector_RAS[4] = {0,0,0,1};
  double point_Grid[4]={0,0,0,1};
  int sampleIndex=0;
  for (point_Grid[2]=0; point_Grid[2]<gridSize[2]; point_Grid[2]++)
  {
    for (point_Grid[1]=0; point_Grid[1]<gridSize[1]; point_Grid[1]++)
    {
      for (point_Grid[0]=0; point_Grid[0]<gridSize[0]; point_Grid[0]++)
      {
        gridToRAS->MultiplyPoint(point_Grid, point_RAS);

        inputTransform->TransformPoint(point_RAS, transformedPoint_RAS);

        pointDislocationVector_RAS[0] = transformedPoint_RAS[0] - point_RAS[0];
        pointDislocationVector_RAS[1] = transformedPoint_RAS[1] - point_RAS[1];
        pointDislocationVector_RAS[2] = transformedPoint_RAS[2] - point_RAS[2];

        samplePositions_RAS->SetPoint(sampleIndex, point_RAS[0], point_RAS[1], point_RAS[2]);
        sampleVectors_RAS->SetTuple3(sampleIndex,pointDislocationVector_RAS[0], pointDislocationVector_RAS[1], pointDislocationVector_RAS[2]);
        sampleIndex++;
      }
    }
  }

  outputPointSet->SetPoints(samplePositions_RAS);
  vtkPointData* pointData = outputPointSet->GetPointData();
  pointData->SetVectors(sampleVectors_RAS);

  // Compute vector magnitude and add to the data set
  vtkSmartPointer<vtkVectorNorm> norm = vtkSmartPointer<vtkVectorNorm>::New();
  norm->SetInput(outputPointSet);
  norm->Update();
  vtkDataArray* vectorMagnitude = norm->GetOutput()->GetPointData()->GetScalars();
  vectorMagnitude->SetName("DisplacementMagnitude");
  outputPointSet->GetPointData()->AddArray(vectorMagnitude);
}

//----------------------------------------------------------------------------
/*
  pointGroupSize: the number of points will be N*pointGroupSize (the actual number will be returned in numGridPoints[3])
*/
void vtkMRMLTransformDisplayNode::GetTransformedPointSamplesOnSlice(vtkPointSet* outputPointSet, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize, double pointSpacing, int pointGroupSize/*=1*/, int* numGridPoints/*=NULL*/)
{
  int numOfPointsX=floor(fieldOfViewSize[0]/(pointSpacing*pointGroupSize))*pointGroupSize;
  int numOfPointsY=floor(fieldOfViewSize[1]/(pointSpacing*pointGroupSize))*pointGroupSize;
  double xOfs = (fieldOfViewSize[0]-(numOfPointsX*pointSpacing))/2 -fieldOfViewSize[0]/2+fieldOfViewOrigin[0];
  double yOfs = (fieldOfViewSize[1]-(numOfPointsY*pointSpacing))/2 -fieldOfViewSize[1]/2+fieldOfViewOrigin[1];

  int gridSize[3]={numOfPointsX+1,numOfPointsY+1,1};

  vtkNew<vtkMatrix4x4> gridToRAS;
  gridToRAS->DeepCopy(sliceToRAS);
  vtkNew<vtkMatrix4x4> gridOffset;
  gridOffset->Element[0][3]=xOfs;
  gridOffset->Element[1][3]=yOfs;
  vtkMatrix4x4::Multiply4x4(gridToRAS.GetPointer(),gridOffset.GetPointer(),gridToRAS.GetPointer());
  vtkNew<vtkMatrix4x4> gridScaling;
  gridScaling->Element[0][0]=pointSpacing;
  gridScaling->Element[1][1]=pointSpacing;
  gridScaling->Element[2][2]=pointSpacing;
  vtkMatrix4x4::Multiply4x4(gridToRAS.GetPointer(),gridScaling.GetPointer(),gridToRAS.GetPointer());

  if (numGridPoints)
  {
    numGridPoints[0]=gridSize[0];
    numGridPoints[1]=gridSize[1];
    numGridPoints[2]=1;
  }

  GetTransformedPointSamples(outputPointSet, gridToRAS.GetPointer(), gridSize);

  float sliceNormal_RAS[3] = {0,0,0};
  sliceNormal_RAS[0] = sliceToRAS->GetElement(0,2);
  sliceNormal_RAS[1] = sliceToRAS->GetElement(1,2);
  sliceNormal_RAS[2] = sliceToRAS->GetElement(2,2);

  // Project vectors to the slice plane
  float dot = 0;
  vtkDataArray* projectedVectors = outputPointSet->GetPointData()->GetVectors();
  double* chosenVector = NULL;
  int numOfTuples=projectedVectors->GetNumberOfTuples();
  for(int i = 0; i < numOfTuples; i++)
  {
    chosenVector = projectedVectors->GetTuple3(i);
    dot = chosenVector[0]*sliceNormal_RAS[0] + chosenVector[1]*sliceNormal_RAS[1] + chosenVector[2]*sliceNormal_RAS[2];
    projectedVectors->SetTuple3(i, chosenVector[0]-dot*sliceNormal_RAS[0], chosenVector[1]-dot*sliceNormal_RAS[1], chosenVector[2]-dot*sliceNormal_RAS[2]);
  }
  projectedVectors->SetName("ProjectedDisplacementVector");
}


//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetTransformedPointSamplesAsImage(vtkImageData* magnitudeImage, vtkMatrix4x4* ijkToRAS, int* imageSize)
{
 vtkMRMLTransformNode* inputTransformNode=GetTransformNode();
  if (!inputTransformNode)
  {
    return;
  }
  vtkSmartPointer<vtkGeneralTransform> inputTransform=vtkSmartPointer<vtkGeneralTransform>::New();
  inputTransformNode->GetTransformToWorld(inputTransform);

  magnitudeImage->SetExtent(0,imageSize[0]-1,0,imageSize[1]-1,0,imageSize[2]-1);
  double spacing[3]=
  {
    sqrt(ijkToRAS->Element[0][0]*ijkToRAS->Element[0][0]+ijkToRAS->Element[1][0]*ijkToRAS->Element[1][0]+ijkToRAS->Element[2][0]*ijkToRAS->Element[2][0]),
    sqrt(ijkToRAS->Element[0][1]*ijkToRAS->Element[0][1]+ijkToRAS->Element[1][1]*ijkToRAS->Element[1][1]+ijkToRAS->Element[2][1]*ijkToRAS->Element[2][1]),
    sqrt(ijkToRAS->Element[0][2]*ijkToRAS->Element[0][2]+ijkToRAS->Element[1][2]*ijkToRAS->Element[1][2]+ijkToRAS->Element[2][2]*ijkToRAS->Element[2][2])
  };
  double origin[3]=
  {
    ijkToRAS->Element[0][3],
    ijkToRAS->Element[1][3],
    ijkToRAS->Element[2][3]
  };
  // The orientation of the volume cannot be set in the image
  // therefore the volume will not appear in the correct position
  // if the direction matrix is not identity.
  //magnitudeImage->SetSpacing(spacing);
  //magnitudeImage->SetOrigin(origin);
  magnitudeImage->SetScalarTypeToFloat();
  magnitudeImage->AllocateScalars();

  double point_RAS[4] = {0,0,0,1};
  double transformedPoint_RAS[4] = {0,0,0,1};
  double pointDislocationVector_RAS[4] = {0,0,0,1};
  double point_IJK[4]={0,0,0,1};
  float* voxelPtr=static_cast<float*>(magnitudeImage->GetScalarPointer());
  for (point_IJK[2]=0; point_IJK[2]<imageSize[2]; point_IJK[2]++)
  {
    for (point_IJK[1]=0; point_IJK[1]<imageSize[1]; point_IJK[1]++)
    {
      for (point_IJK[0]=0; point_IJK[0]<imageSize[0]; point_IJK[0]++)
      {
        ijkToRAS->MultiplyPoint(point_IJK, point_RAS);

        inputTransform->TransformPoint(point_RAS, transformedPoint_RAS);

        pointDislocationVector_RAS[0] = transformedPoint_RAS[0] - point_RAS[0];
        pointDislocationVector_RAS[1] = transformedPoint_RAS[1] - point_RAS[1];
        pointDislocationVector_RAS[2] = transformedPoint_RAS[2] - point_RAS[2];

        float mag=sqrt(
          pointDislocationVector_RAS[0]*pointDislocationVector_RAS[0]+
          pointDislocationVector_RAS[1]*pointDislocationVector_RAS[1]+
          pointDislocationVector_RAS[2]*pointDislocationVector_RAS[2]);

        *(voxelPtr++)=mag;
      }
    }
  }

}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetTransformedPointSamplesOnRoi(vtkPointSet* pointSet, vtkMatrix4x4* roiToRAS, int* roiSize, double pointSpacingMm, int pointGroupSize/*=1*/, int* numGridPoints/*=NULL*/)
{
  double roiSpacing[3]=
  {
    sqrt(roiToRAS->Element[0][0]*roiToRAS->Element[0][0]+roiToRAS->Element[1][0]*roiToRAS->Element[1][0]+roiToRAS->Element[2][0]*roiToRAS->Element[2][0]),
    sqrt(roiToRAS->Element[0][1]*roiToRAS->Element[0][1]+roiToRAS->Element[1][1]*roiToRAS->Element[1][1]+roiToRAS->Element[2][1]*roiToRAS->Element[2][1]),
    sqrt(roiToRAS->Element[0][2]*roiToRAS->Element[0][2]+roiToRAS->Element[1][2]*roiToRAS->Element[1][2]+roiToRAS->Element[2][2]*roiToRAS->Element[2][2])
  };

  double roiSizeMm[3]={roiSize[0]*roiSpacing[0], roiSize[1]*roiSpacing[1], roiSize[2]*roiSpacing[2]};

  int numOfPointsX=floor(roiSizeMm[0]/(pointSpacingMm*pointGroupSize)+0.5)*pointGroupSize;
  int numOfPointsY=floor(roiSizeMm[1]/(pointSpacingMm*pointGroupSize)+0.5)*pointGroupSize;
  int numOfPointsZ=floor(roiSizeMm[2]/(pointSpacingMm*pointGroupSize)+0.5)*pointGroupSize;

  double xOfs = (roiSizeMm[0]-(numOfPointsX*pointSpacingMm))/2.0/roiSpacing[0];
  double yOfs = (roiSizeMm[1]-(numOfPointsY*pointSpacingMm))/2.0/roiSpacing[1];
  double zOfs = (roiSizeMm[2]-(numOfPointsZ*pointSpacingMm))/2.0/roiSpacing[2];

  int gridSize[3]={numOfPointsX+1,numOfPointsY+1,numOfPointsZ+1};

  vtkNew<vtkMatrix4x4> gridToRAS;
  gridToRAS->DeepCopy(roiToRAS);
  vtkNew<vtkMatrix4x4> gridOffset;
  gridOffset->Element[0][3]=xOfs;
  gridOffset->Element[1][3]=yOfs;
  gridOffset->Element[2][3]=zOfs;
  vtkMatrix4x4::Multiply4x4(gridToRAS.GetPointer(),gridOffset.GetPointer(),gridToRAS.GetPointer());
  vtkNew<vtkMatrix4x4> gridScaling;
  gridScaling->Element[0][0]=pointSpacingMm/roiSpacing[0];
  gridScaling->Element[1][1]=pointSpacingMm/roiSpacing[1];
  gridScaling->Element[2][2]=pointSpacingMm/roiSpacing[2];
  vtkMatrix4x4::Multiply4x4(gridToRAS.GetPointer(),gridScaling.GetPointer(),gridToRAS.GetPointer());

  // We need rounding (floor(...+0.5)) because otherwise due to minor numerical differences
  // we could have one more or one less grid size when the roiSpacing does not match exactly the
  // glyph spacing.

  GetTransformedPointSamples(pointSet, gridToRAS.GetPointer(), gridSize);

  if (numGridPoints!=NULL)
  {
    numGridPoints[0]=gridSize[0];
    numGridPoints[1]=gridSize[1];
    numGridPoints[2]=gridSize[2];
  }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetGlyphVisualization3d(vtkPolyData* output, vtkMatrix4x4* roiToRAS, int* roiSize)
{
  vtkSmartPointer<vtkUnstructuredGrid> pointSet = vtkSmartPointer<vtkUnstructuredGrid>::New();
  GetTransformedPointSamplesOnRoi(pointSet, roiToRAS, roiSize, this->GlyphSpacingMm);

  vtkSmartPointer<vtkTransformVisualizerGlyph3D> glyphFilter = vtkSmartPointer<vtkTransformVisualizerGlyph3D>::New();
  glyphFilter->SetScaleModeToScaleByVector();
  glyphFilter->SetScaleFactor(this->GetGlyphScalePercent()*0.01);
  glyphFilter->SetScaleDirectional(false);
  glyphFilter->SetColorModeToColorByVector();
  glyphFilter->SetScaleDirectional(this->GetGlyphScaleDirectional());
  glyphFilter->OrientOn();
  glyphFilter->SetInput(pointSet);

  glyphFilter->SetMagnitudeThresholdLower(this->GlyphDisplayRangeMinMm);
  glyphFilter->SetMagnitudeThresholdUpper(this->GlyphDisplayRangeMaxMm);
  glyphFilter->SetMagnitudeThresholding(true);

  switch (this->GetGlyphType())
  {
    //Arrows
    case vtkMRMLTransformDisplayNode::GLYPH_TYPE_ARROW:
    {
      vtkSmartPointer<vtkArrowSource> arrowSource = vtkSmartPointer<vtkArrowSource>::New();
      arrowSource->SetTipLength(this->GetGlyphTipLengthPercent()*0.01);
      arrowSource->SetTipRadius(this->GetGlyphDiameterMm()*0.5);
      arrowSource->SetTipResolution(this->GetGlyphResolution());
      arrowSource->SetShaftRadius(arrowSource->GetTipRadius()*0.01*this->GetGlyphShaftDiameterPercent());
      arrowSource->SetShaftResolution(this->GetGlyphResolution());
      glyphFilter->SetSourceConnection(arrowSource->GetOutputPort());
      break;
    }
    //Cones
    case vtkMRMLTransformDisplayNode::GLYPH_TYPE_CONE:
    {
      vtkSmartPointer<vtkConeSource> coneSource = vtkSmartPointer<vtkConeSource>::New();
      coneSource->SetHeight(this->GetGlyphTipLengthPercent()*0.01);
      coneSource->SetRadius(this->GetGlyphDiameterMm()*0.5);
      coneSource->SetResolution(this->GetGlyphResolution());
      glyphFilter->SetSourceConnection(coneSource->GetOutputPort());
      break;
    }
    //Spheres
    case vtkMRMLTransformDisplayNode::GLYPH_TYPE_SPHERE:
    {
      vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
      sphereSource->SetRadius(0.5);
      sphereSource->SetThetaResolution(this->GetGlyphResolution());
      sphereSource->SetPhiResolution(this->GetGlyphResolution());
      glyphFilter->SetSourceConnection(sphereSource->GetOutputPort());
      break;
    }
  }

  glyphFilter->Update();
  output->ShallowCopy(glyphFilter->GetOutput());
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetGlyphVisualization2d(vtkPolyData* output, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize)
{
  //Pre-processing
  vtkSmartPointer<vtkUnstructuredGrid> pointSet = vtkSmartPointer<vtkUnstructuredGrid>::New();
  pointSet->Initialize();

  GetTransformedPointSamplesOnSlice(pointSet, sliceToRAS, fieldOfViewOrigin, fieldOfViewSize, this->GetGlyphSpacingMm());

  float sliceNormal_RAS[3] = {0,0,0};
  sliceNormal_RAS[0] = sliceToRAS->GetElement(0,2);
  sliceNormal_RAS[1] = sliceToRAS->GetElement(1,2);
  sliceNormal_RAS[2] = sliceToRAS->GetElement(2,2);

  vtkSmartPointer<vtkTransform> rotateArrow = vtkSmartPointer<vtkTransform>::New();
  bool useNewMethod=false;
  if (useNewMethod)
  {
    // the arrow tips are oriented correctly, but it seems that the direction of the shaft is not always correct
    vtkSmartPointer<vtkMatrix4x4> glyphOrientation=vtkSmartPointer<vtkMatrix4x4>::New();
    glyphOrientation->DeepCopy(sliceToRAS);
    glyphOrientation->Element[0][3]=0;
    glyphOrientation->Element[1][3]=0;
    glyphOrientation->Element[2][3]=0;
    rotateArrow->SetMatrix(glyphOrientation);
    rotateArrow->Inverse();
  }
  else
  {
    // the arrow tips are note always oriented correctly, but the direction of the shaft looks correct
    rotateArrow->RotateX(vtkMath::DegreesFromRadians(acos(abs(sliceNormal_RAS[2])))); // TODO: check this, it might not be correct for an arbitrarily oriented slice normal
  }

  vtkSmartPointer<vtkGlyphSource2D> glyph2DSource = vtkSmartPointer<vtkGlyphSource2D>::New();
  switch (this->GetGlyphType())
  {
    case vtkMRMLTransformDisplayNode::GLYPH_TYPE_ARROW: glyph2DSource->SetGlyphTypeToArrow(); break;
    case vtkMRMLTransformDisplayNode::GLYPH_TYPE_CONE: glyph2DSource->SetGlyphTypeToEdgeArrow(); break;
    case vtkMRMLTransformDisplayNode::GLYPH_TYPE_SPHERE: glyph2DSource->SetGlyphTypeToCircle(); break;
    default: glyph2DSource->SetGlyphTypeToNone();
  }
  glyph2DSource->SetScale(1);
  glyph2DSource->SetFilled(0);

  vtkSmartPointer<vtkTransformVisualizerGlyph3D> glyphFilter = vtkSmartPointer<vtkTransformVisualizerGlyph3D>::New();
  glyphFilter->SetScaleModeToScaleByVector();
  glyphFilter->SetScaleFactor(this->GetGlyphScalePercent()*0.01);
  glyphFilter->SetScaleDirectional(false);
  glyphFilter->SetColorModeToColorByVector();
  glyphFilter->SetSourceTransform(rotateArrow);
  glyphFilter->SetSourceConnection(glyph2DSource->GetOutputPort());
  glyphFilter->SetInput(pointSet);

  glyphFilter->SetMagnitudeThresholdLower(this->GlyphDisplayRangeMinMm);
  glyphFilter->SetMagnitudeThresholdUpper(this->GlyphDisplayRangeMaxMm);
  glyphFilter->SetMagnitudeThresholding(true);

  glyphFilter->Update();

  output->ShallowCopy(glyphFilter->GetOutput());
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::CreateGrid(vtkPolyData* gridPolyData, int numGridPoints[3], int gridSubdivision, vtkPolyData* warpedGrid/*=NULL*/)
{
  vtkSmartPointer<vtkCellArray> grid = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();

  // Create lines along i
  for (int k = 0; k < numGridPoints[2]; k+=gridSubdivision)
  {
    for (int j = 0; j < numGridPoints[1]; j+=gridSubdivision)
    {
      for (int i = 0; i < numGridPoints[0]-1; i++)
      {
        line->GetPointIds()->SetId(0, (i) + (j*numGridPoints[0]) + (k*numGridPoints[0]*numGridPoints[1]));
        line->GetPointIds()->SetId(1, (i+1) + (j*numGridPoints[0]) + (k*numGridPoints[0]*numGridPoints[1]));
        grid->InsertNextCell(line);
      }
    }
  }

  // Create lines along j
  for (int k = 0; k < numGridPoints[2]; k+=gridSubdivision)
  {
    for (int j = 0; j < numGridPoints[1]-1; j++)
    {
      for (int i = 0; i < numGridPoints[0]; i+=gridSubdivision)
      {
        line->GetPointIds()->SetId(0, (i) + ((j)*numGridPoints[0]) + (k*numGridPoints[0]*numGridPoints[1]));
        line->GetPointIds()->SetId(1, (i) + ((j+1)*numGridPoints[0]) + (k*numGridPoints[0]*numGridPoints[1]));
        grid->InsertNextCell(line);
      }
    }
  }

  // Create lines along k
  for (int k = 0; k < numGridPoints[2]-1; k++)
  {
    for (int j = 0; j < numGridPoints[1]; j+=gridSubdivision)
    {
      for (int i = 0; i < numGridPoints[0]; i+=gridSubdivision)
      {
        line->GetPointIds()->SetId(0, (i) + ((j)*numGridPoints[0]) + ((k)*numGridPoints[0]*numGridPoints[1]));
        line->GetPointIds()->SetId(1, (i) + ((j)*numGridPoints[0]) + ((k+1)*numGridPoints[0]*numGridPoints[1]));
        grid->InsertNextCell(line);
      }
    }
  }

  gridPolyData->SetLines(grid);

  if (warpedGrid)
  {
    vtkSmartPointer<vtkWarpVector> warp = vtkSmartPointer<vtkWarpVector>::New();
    warp->SetInput(gridPolyData);
    warp->SetScaleFactor(this->GetGridScalePercent()*0.01);
    warp->Update();
    vtkPolyData* polyoutput = warp->GetPolyDataOutput();
    polyoutput->GetPointData()->AddArray(gridPolyData->GetPointData()->GetArray("VectorMagnitude"));
    warpedGrid->ShallowCopy(warp->GetPolyDataOutput());
    warpedGrid->Update();
  }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetGridVisualization2d(vtkPolyData* output, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize)
{
  int gridSubdivision=2;
  double pointSpacing=this->GetGridSpacingMm()/gridSubdivision;
  int numGridPoints[3]={0};

  vtkSmartPointer<vtkPolyData> gridPolyData=vtkSmartPointer<vtkPolyData>::New();
  GetTransformedPointSamplesOnSlice(gridPolyData, sliceToRAS, fieldOfViewOrigin, fieldOfViewSize, pointSpacing, gridSubdivision, numGridPoints);

  CreateGrid(gridPolyData, numGridPoints, gridSubdivision, output);
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetGridVisualization3d(vtkPolyData* output, vtkMatrix4x4* roiToRAS, int* roiSize)
{
  int gridSubdivision=2;
  double pointSpacing=this->GetGridSpacingMm()/gridSubdivision;
  int numGridPoints[3]={0};

  vtkNew<vtkPolyData> gridPolyData;
  GetTransformedPointSamplesOnRoi(gridPolyData.GetPointer(), roiToRAS, roiSize, pointSpacing, gridSubdivision, numGridPoints);

//  SetEdgeVisibility(1);
  vtkNew<vtkPolyData> warpedGridPolyData;
  CreateGrid(gridPolyData.GetPointer(), numGridPoints, gridSubdivision, warpedGridPolyData.GetPointer());

  vtkNew<vtkTubeFilter> tubeFilter;
  tubeFilter->SetInput(warpedGridPolyData.GetPointer());
  tubeFilter->SetRadius(this->GetGridLineDiameterMm()*0.5);
  tubeFilter->Update();

  output->ShallowCopy(tubeFilter->GetOutput());
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetContourVisualization2d(vtkPolyData* output, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize)
{
  vtkNew<vtkImageData> magnitudeImage;
  double pointSpacing=this->GetContourResolutionMm();

  vtkNew<vtkMatrix4x4> ijkToRAS;

  int numOfPointsX=ceil(fieldOfViewSize[0]/pointSpacing);
  int numOfPointsY=ceil(fieldOfViewSize[1]/pointSpacing);
  double xOfs = -fieldOfViewSize[0]/2+fieldOfViewOrigin[0];
  double yOfs = -fieldOfViewSize[1]/2+fieldOfViewOrigin[1];

  ijkToRAS->DeepCopy(sliceToRAS);
  vtkNew<vtkMatrix4x4> ijkOffset;
  ijkOffset->Element[0][3]=xOfs;
  ijkOffset->Element[1][3]=yOfs;
  vtkMatrix4x4::Multiply4x4(ijkToRAS.GetPointer(),ijkOffset.GetPointer(),ijkToRAS.GetPointer());
  vtkNew<vtkMatrix4x4> voxelSpacing;
  voxelSpacing->Element[0][0]=pointSpacing;
  voxelSpacing->Element[1][1]=pointSpacing;
  voxelSpacing->Element[2][2]=pointSpacing;
  vtkMatrix4x4::Multiply4x4(ijkToRAS.GetPointer(),voxelSpacing.GetPointer(),ijkToRAS.GetPointer());

  int imageSize[3]={numOfPointsX, numOfPointsY,1};

  GetTransformedPointSamplesAsImage(magnitudeImage.GetPointer(), ijkToRAS.GetPointer(), imageSize);

  vtkNew<vtkContourFilter> contourFilter;
  double* levels=this->GetContourLevelsMm();
  for (int i=0; i<this->GetNumberOfContourLevels(); i++)
  {
    contourFilter->SetValue(i, levels[i]);
  }
  contourFilter->SetInput(magnitudeImage.GetPointer());
  contourFilter->Update();

  vtkNew<vtkTransformPolyDataFilter> transformSliceToRas;
  vtkNew<vtkTransform> sliceToRasTransform;
  sliceToRasTransform->SetMatrix(ijkToRAS.GetPointer());
  transformSliceToRas->SetTransform(sliceToRasTransform.GetPointer());
  transformSliceToRas->SetInputConnection(contourFilter->GetOutputPort());
  transformSliceToRas->Update();
  output->ShallowCopy(transformSliceToRas->GetOutput());
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetContourVisualization3d(vtkPolyData* output, vtkMatrix4x4* roiToRAS, int* roiSize)
{
  vtkNew<vtkImageData> magnitudeImage;
  double pointSpacing=this->GetContourResolutionMm();

  vtkNew<vtkMatrix4x4> ijkToRAS;

  int numOfPointsX=ceil(roiSize[0]/pointSpacing);
  int numOfPointsY=ceil(roiSize[1]/pointSpacing);
  int numOfPointsZ=ceil(roiSize[2]/pointSpacing);
  double xOfs = 0;
  double yOfs = 0;
  double zOfs = 0;

  ijkToRAS->DeepCopy(roiToRAS);
  vtkNew<vtkMatrix4x4> ijkOffset;
  ijkOffset->Element[0][3]=xOfs;
  ijkOffset->Element[1][3]=yOfs;
  ijkOffset->Element[2][3]=zOfs;
  vtkMatrix4x4::Multiply4x4(ijkToRAS.GetPointer(),ijkOffset.GetPointer(),ijkToRAS.GetPointer());
  vtkNew<vtkMatrix4x4> voxelSpacing;
  voxelSpacing->Element[0][0]=pointSpacing;
  voxelSpacing->Element[1][1]=pointSpacing;
  voxelSpacing->Element[2][2]=pointSpacing;
  vtkMatrix4x4::Multiply4x4(ijkToRAS.GetPointer(),voxelSpacing.GetPointer(),ijkToRAS.GetPointer());

  int imageSize[3]={numOfPointsX, numOfPointsY, numOfPointsZ};

  GetTransformedPointSamplesAsImage(magnitudeImage.GetPointer(), ijkToRAS.GetPointer(), imageSize);

  vtkNew<vtkContourFilter> contourFilter;
  double* levels=this->GetContourLevelsMm();
  for (int i=0; i<this->GetNumberOfContourLevels(); i++)
  {
    contourFilter->SetValue(i, levels[i]);
  }
  contourFilter->SetInput(magnitudeImage.GetPointer());
  contourFilter->Update();

  //  output->ShallowCopy(contourFilter->GetOutput());

  vtkNew<vtkTransformPolyDataFilter> transformSliceToRas;
  vtkNew<vtkTransform> sliceToRasTransform;
  sliceToRasTransform->SetMatrix(ijkToRAS.GetPointer());
  transformSliceToRas->SetTransform(sliceToRasTransform.GetPointer());
  transformSliceToRas->SetInputConnection(contourFilter->GetOutputPort());
  transformSliceToRas->Update();
  output->ShallowCopy(transformSliceToRas->GetOutput());

}


//----------------------------------------------------------------------------
vtkMRMLTransformNode* vtkMRMLTransformDisplayNode::GetTransformNode()
{
  return vtkMRMLTransformNode::SafeDownCast(this->GetDisplayableNode());
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetVisualization2d(vtkPolyData* output, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize)
{
  switch (this->GetVisualizationMode())
  {
  case vtkMRMLTransformDisplayNode::VIS_MODE_GLYPH:
    GetGlyphVisualization2d(output, sliceToRAS, fieldOfViewOrigin, fieldOfViewSize);
    break;
  case vtkMRMLTransformDisplayNode::VIS_MODE_GRID:
    GetGridVisualization2d(output, sliceToRAS, fieldOfViewOrigin, fieldOfViewSize);
    break;
  case vtkMRMLTransformDisplayNode::VIS_MODE_CONTOUR:
    GetContourVisualization2d(output, sliceToRAS, fieldOfViewOrigin, fieldOfViewSize);
    break;
  }
}

//----------------------------------------------------------------------------
void vtkMRMLTransformDisplayNode::GetVisualization3d(vtkPolyData* output, vtkMatrix4x4* roiToRAS, int* roiSize)
{
  switch (this->GetVisualizationMode())
  {
  case vtkMRMLTransformDisplayNode::VIS_MODE_GLYPH:
    this->BackfaceCulling=1;
    GetGlyphVisualization3d(output, roiToRAS, roiSize);
    break;
  case vtkMRMLTransformDisplayNode::VIS_MODE_GRID:
    this->BackfaceCulling=1;
    GetGridVisualization3d(output, roiToRAS, roiSize);
    break;
  case vtkMRMLTransformDisplayNode::VIS_MODE_CONTOUR:
    this->BackfaceCulling=0;
    GetContourVisualization3d(output, roiToRAS, roiSize);
    break;
  }
}

/*

CreateDefaultColorTableNode()

call it when the model is generated and there is no selected color table yet

if (outputModelNode->GetModelDisplayNode()->GetColorNode()==NULL)
  {
    vtkSmartPointer<vtkMRMLColorTableNode> colorTableNode = vtkSmartPointer<vtkMRMLColorTableNode>::New();
    this->GetMRMLScene()->AddNode(colorTableNode);

    colorTableNode->SetName("Deformation Field Colors");
    colorTableNode->SetAttribute("Category", "User Generated");
    colorTableNode->SetTypeToUser();
    colorTableNode->SetNumberOfColors(4);
    colorTableNode->GetLookupTable();
    colorTableNode->AddColor("negligible", 0.0, 0.0, 0.5, 1.0);
    colorTableNode->AddColor(       "low", 0.0, 1.0, 0.0, 1.0);
    colorTableNode->AddColor(    "medium", 1.0, 1.0, 0.0, 1.0);
    colorTableNode->AddColor(      "high", 1.0, 0.0, 0.0, 1.0);

    outputModelNode->GetModelDisplayNode()->SetAndObserveColorNodeID(colorTableNode->GetID());
  }

  vtkMRMLColorTableNode *colorNode = vtkMRMLColorTableNode::SafeDownCast(outputModelNode->GetModelDisplayNode()->GetColorNode());



      vtkDebugMacro("Color by mean fiber orientation");
    this->ScalarVisibilityOn( );
    this->ColorLinesByOrientation->SetColorMode(
      this->ColorLinesByOrientation->colorModeMeanFiberOrientation);
    this->TubeFilter->SetInputConnection(this->ColorLinesByOrientation->GetOutputPort());
    vtkMRMLNode* ColorNode = this->GetScene()->GetNodeByID("vtkMRMLColorTableNodeFullRainbow");
    if (ColorNode)
      {
      this->SetAndObserveColorNodeID(ColorNode->GetID());
      }

*/