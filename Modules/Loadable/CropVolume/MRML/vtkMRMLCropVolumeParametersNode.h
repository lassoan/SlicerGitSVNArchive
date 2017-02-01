/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLVolumeRenderingParametersNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
// .NAME vtkMRMLVolumeRenderingParametersNode - MRML node for storing a slice through RAS space
// .SECTION Description
// This node stores the information about the currently selected volume
//
//

#ifndef __vtkMRMLCropVolumeParametersNode_h
#define __vtkMRMLCropVolumeParametersNode_h

#include "vtkMRML.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLNode.h"
#include "vtkSlicerCropVolumeModuleMRMLExport.h"

class vtkMRMLAnnotationROINode;
class vtkMRMLVolumeNode;

/// \ingroup Slicer_QtModules_CropVolume
class VTK_SLICER_CROPVOLUME_MODULE_MRML_EXPORT vtkMRMLCropVolumeParametersNode : public vtkMRMLNode
{
public:
  enum
    {
    InterpolationNearestNeighbor = 1,
    InterpolationLinear = 2,
    InterpolationWindowedSinc = 3,
    InterpolationBSpline = 4
    };

  static vtkMRMLCropVolumeParametersNode *New();
  vtkTypeMacro(vtkMRMLCropVolumeParametersNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from XML attributes
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "CropVolumeParameters";};

  /// Set volume node to be cropped
  void SetInputVolumeNodeID(const char *nodeID);
  /// Get volume node to be cropped
  const char *GetInputVolumeNodeID();
  vtkMRMLVolumeNode* GetInputVolumeNode();

  /// Set resulting cropped volume node
  void SetOutputVolumeNodeID(const char *nodeID);
  /// Get resulting cropped volume node
  const char* GetOutputVolumeNodeID();
  vtkMRMLVolumeNode* GetOutputVolumeNode();

  /// Set cropping region of interest
  void SetROINodeID(const char *nodeID);
  /// Get cropping region of interest
  const char* GetROINodeID();
  vtkMRMLAnnotationROINode* GetROINode();

  vtkSetMacro(IsotropicResampling,bool);
  vtkGetMacro(IsotropicResampling,bool);
  vtkBooleanMacro(IsotropicResampling,bool);

  vtkSetMacro(ROIVisibility,bool);
  vtkGetMacro(ROIVisibility,bool);
  vtkBooleanMacro(ROIVisibility,bool);

  vtkSetMacro(VoxelBased,bool);
  vtkGetMacro(VoxelBased,bool);
  vtkBooleanMacro(VoxelBased,bool);

  vtkSetMacro(InterpolationMode, int);
  vtkGetMacro(InterpolationMode, int);

  vtkSetMacro(SpacingScalingConst, double);
  vtkGetMacro(SpacingScalingConst, double);

protected:
  vtkMRMLCropVolumeParametersNode();
  ~vtkMRMLCropVolumeParametersNode();

  vtkMRMLCropVolumeParametersNode(const vtkMRMLCropVolumeParametersNode&);
  void operator=(const vtkMRMLCropVolumeParametersNode&);

  bool ROIVisibility;
  bool VoxelBased;
  int InterpolationMode;
  bool IsotropicResampling;
  double SpacingScalingConst;
};

#endif

