/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkSlicerCropVolumeLogic.cxx,v $
  Date:      $Date: 2006/01/06 17:56:48 $
  Version:   $Revision: 1.58 $

=========================================================================auto=*/

// CLI invocation
#include <qSlicerCLIModule.h>
#include <vtkSlicerCLIModuleLogic.h>

// CropLogic includes
#include "vtkSlicerCLIModuleLogic.h"
#include "vtkSlicerCropVolumeLogic.h"
#include "vtkSlicerVolumesLogic.h"

// CropMRML includes

// MRML includes
#include <vtkMRMLAnnotationROINode.h>
#include <vtkMRMLCropVolumeParametersNode.h>
#include <vtkMRMLDiffusionTensorVolumeNode.h>
#include <vtkMRMLDiffusionWeightedVolumeNode.h>
#include <vtkMRMLDiffusionWeightedVolumeDisplayNode.h>
#include <vtkMRMLLabelMapVolumeNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLAnnotationROINode.h>

// VTK includes
#include <vtkImageData.h>
#include <vtkImageClip.h>
#include <vtkNew.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkVersion.h>

// STD includes
#include <cassert>
#include <iostream>

//----------------------------------------------------------------------------
class vtkSlicerCropVolumeLogic::vtkInternal
{
public:
  vtkInternal();

  vtkSlicerVolumesLogic* VolumesLogic;
  vtkSlicerCLIModuleLogic* ResampleLogic;
};

//----------------------------------------------------------------------------
vtkSlicerCropVolumeLogic::vtkInternal::vtkInternal()
{
  this->VolumesLogic = 0;
  this->ResampleLogic = 0;
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerCropVolumeLogic);

//----------------------------------------------------------------------------
vtkSlicerCropVolumeLogic::vtkSlicerCropVolumeLogic()
{
  this->Internal = new vtkInternal;
}

//----------------------------------------------------------------------------
vtkSlicerCropVolumeLogic::~vtkSlicerCropVolumeLogic()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSlicerCropVolumeLogic::SetVolumesLogic(vtkSlicerVolumesLogic* logic)
{
  this->Internal->VolumesLogic = logic;
}

//----------------------------------------------------------------------------
vtkSlicerVolumesLogic* vtkSlicerCropVolumeLogic::GetVolumesLogic()
{
  return this->Internal->VolumesLogic;
}

//----------------------------------------------------------------------------
void vtkSlicerCropVolumeLogic::SetResampleLogic(vtkSlicerCLIModuleLogic* logic)
{
  this->Internal->ResampleLogic = logic;
}

//----------------------------------------------------------------------------
vtkSlicerCLIModuleLogic* vtkSlicerCropVolumeLogic::GetResampleLogic()
{
  return this->Internal->ResampleLogic;
}

//----------------------------------------------------------------------------
void vtkSlicerCropVolumeLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  os << indent << "vtkSlicerCropVolumeLogic:             " << this->GetClassName() << "\n";
}

//----------------------------------------------------------------------------
int vtkSlicerCropVolumeLogic::Apply(vtkMRMLCropVolumeParametersNode* pnode)
{
  vtkMRMLScene *scene = this->GetMRMLScene();
  if (!scene)
    {
    vtkErrorMacro("CropVolume: Invalid scene");
    return -1;
    }

  // Check inputs
  if (!pnode)
    {
    vtkErrorMacro("CropVolume: Invalid parameter node");
    return -1;
    }
  vtkMRMLVolumeNode *inputVolume =
    vtkMRMLVolumeNode::SafeDownCast(scene->GetNodeByID(pnode->GetInputVolumeNodeID()));
  vtkMRMLAnnotationROINode *inputROI =
    vtkMRMLAnnotationROINode::SafeDownCast(scene->GetNodeByID(pnode->GetROINodeID()));
  if (!inputVolume || !inputROI)
    {
    vtkErrorMacro("CropVolume: Invalid input volume or ROI");
    return -1;
    }
  if (vtkMRMLDiffusionTensorVolumeNode::SafeDownCast(inputVolume))
    {
    vtkErrorMacro("CropVolume: Diffusion tensor volumes are not supported by this module");
    return -2;
    }

  // Check/create output volume
  vtkMRMLVolumeNode *outputVolume =
    vtkMRMLVolumeNode::SafeDownCast(scene->GetNodeByID(pnode->GetOutputVolumeNodeID()));
  if (outputVolume)
    {
    // Output volume is provided, use that (if compatible)
    if (outputVolume->GetClassName() != inputVolume->GetClassName())
      {
      vtkErrorMacro("CropVolume: output volume (" << outputVolume->GetClassName() <<
        ") is not compatible with input volume (" << inputVolume->GetClassName() << ")");
      return -1;
      }
    }
  else
    {
    // Create compatible output volume
    if (!this->Internal->VolumesLogic)
      {
      vtkErrorMacro("CropVolume: invalid Volumes logic");
      return -2;
      }
    std::ostringstream outSS;
    outSS << (inputVolume->GetName() ? inputVolume->GetName() : "Volume") << " cropped";
    outputVolume = this->Internal->VolumesLogic->CloneVolume(this->GetMRMLScene(), inputVolume, outSS.str().c_str());
    if (!outputVolume)
      {
      vtkErrorMacro("CropVolume: failed to create output volume");
      return -2;
      }
    }

  int errorCode = 0;
  if(pnode->GetVoxelBased()) // voxel based cropping selected
    {
    errorCode = this->CropVoxelBased(inputROI, inputVolume, outputVolume);
    }
  else  // interpolated cropping selected
    {
    errorCode = this->CropInterpolated(inputROI, inputVolume, outputVolume,
      pnode->GetIsotropicResampling(), pnode->GetSpacingScalingConst(), pnode->GetInterpolationMode());
    }
  if (errorCode != 0)
    {
    return errorCode;
    }
  // no errors
  outputVolume->SetAndObserveTransformNodeID(NULL);
  pnode->SetOutputVolumeNodeID(outputVolume->GetID());
  return 0;
}

//----------------------------------------------------------------------------
bool vtkSlicerCropVolumeLogic::GetVoxelBasedCropOutputExtent(vtkMRMLAnnotationROINode* roi, vtkMRMLVolumeNode* inputVolume, int outputExtent[6])
{
  outputExtent[0] = outputExtent[2] = outputExtent[4] = 0;
  outputExtent[1] = outputExtent[3] = outputExtent[5] = -1;
  if (!roi || !inputVolume || !inputVolume->GetImageData())
    {
    return false;
    }

  int originalImageExtents[6] = { 0 };
  inputVolume->GetImageData()->GetExtent(originalImageExtents);

  vtkMRMLTransformNode* roiTransform = roi->GetParentTransformNode();
  if (roiTransform && !roiTransform->IsTransformToWorldLinear())
    {
    vtkWarningMacro("CropVolume: ROI is transformed using a non-linear transform. The transformation will be ignored");
    roiTransform = NULL;
    }

  if (inputVolume->GetParentTransformNode() && !inputVolume->GetParentTransformNode()->IsTransformToWorldLinear())
    {
    vtkWarningMacro("vtkSlicerCropVolumeLogic::CropVoxelBased: voxel-based cropping of non-linearly transformed input volume is not supported");
    return -1;
    }

  vtkNew<vtkMatrix4x4> roiToVolumeTransformMatrix;
  vtkMRMLTransformNode::GetMatrixTransformBetweenNodes(roiTransform, inputVolume->GetParentTransformNode(),
    roiToVolumeTransformMatrix.GetPointer());

  vtkNew<vtkMatrix4x4> rasToIJK;
  inputVolume->GetRASToIJKMatrix(rasToIJK.GetPointer());

  vtkNew<vtkMatrix4x4> roiToVolumeIJKTransformMatrix;
  vtkMatrix4x4::Multiply4x4(rasToIJK.GetPointer(), roiToVolumeTransformMatrix.GetPointer(),
    roiToVolumeIJKTransformMatrix.GetPointer());

  double roiXYZ[3] = { 0 };
  roi->GetXYZ(roiXYZ);
  double roiRadius[3] = { 0 };
  roi->GetRadiusXYZ(roiRadius);

  const int numberOfCorners = 8;
  double volumeCorners_ROI[numberOfCorners][4] =
    {
    { roiXYZ[0] - roiRadius[0], roiXYZ[1] - roiRadius[1], roiXYZ[2] - roiRadius[2], 1. },
    { roiXYZ[0] + roiRadius[0], roiXYZ[1] - roiRadius[1], roiXYZ[2] - roiRadius[2], 1. },
    { roiXYZ[0] - roiRadius[0], roiXYZ[1] + roiRadius[1], roiXYZ[2] - roiRadius[2], 1. },
    { roiXYZ[0] + roiRadius[0], roiXYZ[1] + roiRadius[1], roiXYZ[2] - roiRadius[2], 1. },
    { roiXYZ[0] - roiRadius[0], roiXYZ[1] - roiRadius[1], roiXYZ[2] + roiRadius[2], 1. },
    { roiXYZ[0] + roiRadius[0], roiXYZ[1] - roiRadius[1], roiXYZ[2] + roiRadius[2], 1. },
    { roiXYZ[0] - roiRadius[0], roiXYZ[1] + roiRadius[1], roiXYZ[2] + roiRadius[2], 1. },
    { roiXYZ[0] + roiRadius[0], roiXYZ[1] + roiRadius[1], roiXYZ[2] + roiRadius[2], 1. },
    };

  // Get ROI extent in IJK coordinate system
  double outputExtentDouble[6] = { 0 };
  for (int cornerPointIndex = 0; cornerPointIndex < numberOfCorners; cornerPointIndex++)
    {
    double volumeCorner_IJK[4] = { 0, 0, 0, 1 };
    roiToVolumeIJKTransformMatrix->MultiplyPoint(volumeCorners_ROI[cornerPointIndex], volumeCorner_IJK);
    for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
      {
      if (cornerPointIndex == 0 || volumeCorner_IJK[axisIndex] < outputExtentDouble[axisIndex * 2])
        {
        outputExtentDouble[axisIndex * 2] = volumeCorner_IJK[axisIndex];
        }
      if (cornerPointIndex == 0 || volumeCorner_IJK[axisIndex] > outputExtentDouble[axisIndex * 2 + 1])
        {
        outputExtentDouble[axisIndex * 2 + 1] = volumeCorner_IJK[axisIndex];
        }
      }
    }

  // Limit output extent to input extent
  int* inputExtent = inputVolume->GetImageData()->GetExtent();
  for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
    {
    outputExtent[axisIndex * 2] = std::max(inputExtent[axisIndex * 2], int(ceil(outputExtentDouble[axisIndex * 2])));
    // 0.5 for rounding purposes to make sure everything selected by roi is cropped
    outputExtent[axisIndex * 2 + 1] = std::min(inputExtent[axisIndex * 2 + 1], int(floor(outputExtentDouble[axisIndex * 2 + 1])));
    }

  return true;
}

//----------------------------------------------------------------------------
int vtkSlicerCropVolumeLogic::CropVoxelBased(vtkMRMLAnnotationROINode* roi, vtkMRMLVolumeNode* inputVolume, vtkMRMLVolumeNode* outputVolume)
{
  if(!roi || !inputVolume || !outputVolume)
    {
    return -1;
    }
  if (!inputVolume->GetImageData())
    {
    vtkWarningMacro("vtkSlicerCropVolumeLogic::CropVoxelBased: input image is empty")
    outputVolume->SetAndObserveImageData(NULL);
    return 0;
    }

  int outputExtent[6] = { 0, -1, 0, -1, 0, -1 };
  if (!this->GetVoxelBasedCropOutputExtent(roi, inputVolume, outputExtent))
    {
    vtkWarningMacro("vtkSlicerCropVolumeLogic::CropVoxelBased: failed to get output geometry")
    return -1;
    }

  vtkNew<vtkMatrix4x4> inputIJKToRAS;
  inputVolume->GetIJKToRASMatrix(inputIJKToRAS.GetPointer());

  vtkNew<vtkImageClip> imageClip;
  imageClip->SetInputData(inputVolume->GetImageData());
  imageClip->SetOutputWholeExtent(outputExtent);
  imageClip->SetClipData(true);
  imageClip->Update();

  int wasModified = outputVolume->StartModify();
  outputVolume->SetAndObserveImageData(imageClip->GetOutput());
  outputVolume->SetIJKToRASMatrix(inputIJKToRAS.GetPointer());
  outputVolume->ShiftImageDataExtentToZeroStart();
  outputVolume->EndModify(wasModified);

  return 0;
}

//----------------------------------------------------------------------------
bool vtkSlicerCropVolumeLogic::GetInterpolatedCropOutputGeometry(vtkMRMLAnnotationROINode* roi, vtkMRMLVolumeNode* inputVolume,
  bool isotropicResampling, double spacingScale, int outputExtent[6], double outputSpacing[3])
{
  if (!roi || !inputVolume)
    {
    return false;
    }

  double* inputSpacing = inputVolume->GetSpacing();
  if (isotropicResampling)
    {
    double minSpacing = std::min(std::min(inputSpacing[0], inputSpacing[1]), inputSpacing[2]);
    outputSpacing[0] = minSpacing * spacingScale;
    outputSpacing[1] = minSpacing * spacingScale;
    outputSpacing[2] = minSpacing * spacingScale;
    }
  else
    {
    // TODO: find output spacing axis that is closest to the input spacing
    outputSpacing[0] = inputSpacing[0] * spacingScale;
    outputSpacing[1] = inputSpacing[1] * spacingScale;
    outputSpacing[2] = inputSpacing[2] * spacingScale;
    }

  // prepare the resampling reference volume
  double roiRadius[3] = { 0 };
  roi->GetRadiusXYZ(roiRadius);

  outputExtent[0] = outputExtent[2] = outputExtent[4] = 0;
  outputExtent[1] = int(roiRadius[0] / outputSpacing[0] * 2. + 0.5) - 1;
  outputExtent[3] = int(roiRadius[1] / outputSpacing[1] * 2. + 0.5) - 1;
  outputExtent[5] = int(roiRadius[2] / outputSpacing[2] * 2. + 0.5) - 1;

  return true;
}

//----------------------------------------------------------------------------
int vtkSlicerCropVolumeLogic::CropInterpolated(vtkMRMLAnnotationROINode* roi, vtkMRMLVolumeNode* inputVolume, vtkMRMLVolumeNode* outputVolume,
  bool isotropicResampling, double spacingScale, int interpolationMode)
{
  if (!roi || !inputVolume || !outputVolume)
    {
    return -1;
    }

  if (this->Internal->ResampleLogic == 0)
    {
    vtkErrorMacro("CropVolume: resample logic is not set");
    return -3;
    }

  int outputExtent[6] = { 0, -1, 0, -1, 0, -1 };
  double outputSpacing[3] = { 0 };
  this->GetInterpolatedCropOutputGeometry(roi, inputVolume, isotropicResampling, spacingScale, outputExtent, outputSpacing);

  double roiXYZ[3] = { 0 };
  roi->GetXYZ(roiXYZ);
  double roiRadius[3] = { 0 };
  roi->GetRadiusXYZ(roiRadius);

  double outputOrigin[3] =
  {
    roiXYZ[0] - roiRadius[0] + outputSpacing[0] * .5, // offset in LPS/IJK space?
    roiXYZ[1] - roiRadius[1] + outputSpacing[1] * .5,
    roiXYZ[2] - roiRadius[2] + outputSpacing[2] * .5
  };

  /*
  vtkNew<vtkMatrix4x4> outputIJKToRAS;
  outputIJKToRAS->SetElement(0, 0, outputSpacing[0]);
  outputIJKToRAS->SetElement(1, 1, outputSpacing[1]);
  outputIJKToRAS->SetElement(2, 2, outputSpacing[2]);
  outputIJKToRAS->SetElement(0, 3, outputOrigin[0]);
  outputIJKToRAS->SetElement(1, 3, outputOrigin[0]);
  outputIJKToRAS->SetElement(2, 3, outputOrigin[0]);

  // account for the ROI parent transform, if present
  vtkMRMLTransformNode *roiTransform = roi->GetParentTransformNode();
  // If ROI is transformed with a warping transform then we ignore the transformation
  if (roiTransform && roiTransform->IsTransformToWorldLinear())
    {
    vtkNew<vtkMatrix4x4> roiMatrix;
    roiTransform->GetMatrixTransformToWorld(roiMatrix.GetPointer());
    outputIJKToRAS->Multiply4x4(roiMatrix.GetPointer(), outputIJKToRAS.GetPointer(),
      outputIJKToRAS.GetPointer());
    }

  */

  vtkMRMLCommandLineModuleNode* cmdNode = this->Internal->ResampleLogic->CreateNodeInScene();
  if (cmdNode == NULL)
    {
    vtkErrorMacro("CropVolume: failed to create resample node");
    return -4;
    }

  /*
  vtkNew<vtkMRMLLabelMapVolumeNode> refVolume;
  refVolume->HideFromEditorsOn();
  vtkNew<vtkImageData> refImageData;
  refImageData->SetExtent(outputExtent);
  // Note: it is a waste of memory to allocate reference image
  // (as it is only used for defining geometry)
  refImageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  refVolume->SetAndObserveImageData(refImageData.GetPointer());
  refVolume->SetIJKToRASMatrix(outputIJKToRAS.GetPointer());
  this->GetMRMLScene()->AddNode(refVolume.GetPointer());
  */

  cmdNode->SetParameterAsString("inputVolume", inputVolume->GetID());
  //cmdNode->SetParameterAsString("referenceVolume", refVolume->GetID());
  cmdNode->SetParameterAsString("outputVolume", outputVolume->GetID());

  std::stringstream spacingStream;
  spacingStream << std::setprecision(15) << outputSpacing[0] << "," << outputSpacing[1] << "," << outputSpacing[2];
  cmdNode->SetParameterAsString("outputImageSpacing", spacingStream.str());

  std::stringstream sizeStream;
  sizeStream << outputExtent[1] - outputExtent[0] + 1  << ","
    << outputExtent[3] - outputExtent[2] + 1 << ","
    << outputExtent[5] - outputExtent[4] + 1;
  cmdNode->SetParameterAsString("outputImageSize", sizeStream.str());

  cmdNode->SetParameterAsString("directionMatrix", "-1,0,0,0,-1,0,0,0,1");

  /*
  std::stringstream originStream;
  originStream << std::setprecision(15) << -outputOrigin[0] << "," << -outputOrigin[1] << "," << outputOrigin[2];
  cmdNode->SetParameterAsString("outputImageOrigin", originStream.str());
  */
  vtkNew<vtkMRMLMarkupsFiducialNode> originMarkupNode;
  originMarkupNode->AddFiducial(outputOrigin[0], outputOrigin[1], outputOrigin[2]);
  this->GetMRMLScene()->AddNode(originMarkupNode.GetPointer());
  cmdNode->SetParameterAsString("outputImageOrigin", originMarkupNode->GetID());

  vtkMRMLTransformNode *inputToReferenceVolumeTransform = inputVolume->GetParentTransformNode();

  // TODO: check if it works with non-linear transform - it should!
  // TODO: we should use the relative transform between the input and output volume node
  // (put that transform in a temporary transform node)
  if (inputToReferenceVolumeTransform != NULL && inputToReferenceVolumeTransform->IsLinear())
    {
    cmdNode->SetParameterAsString("transformationFile",
      inputToReferenceVolumeTransform->GetID());
    }

  std::string interp = "linear";
  switch (interpolationMode)
    {
    case vtkMRMLCropVolumeParametersNode::InterpolationNearestNeighbor:
      interp = "nn";
      break;
    case vtkMRMLCropVolumeParametersNode::InterpolationLinear:
      interp = "linear";
      break;
    case vtkMRMLCropVolumeParametersNode::InterpolationWindowedSinc:
      interp = "ws";
      break;
    case vtkMRMLCropVolumeParametersNode::InterpolationBSpline:
      interp = "bs";
      break;
    }

  cmdNode->SetParameterAsString("interpolationType", interp.c_str());
  this->Internal->ResampleLogic->ApplyAndWait(cmdNode, false);

  //this->GetMRMLScene()->RemoveNode(refVolume.GetPointer());
  this->GetMRMLScene()->RemoveNode(cmdNode);
  this->GetMRMLScene()->RemoveNode(originMarkupNode.GetPointer());

  // success
  return 0;
}

//-----------------------------------------------------------------------------
bool vtkSlicerCropVolumeLogic::FitROIToInputVolume(vtkMRMLCropVolumeParametersNode* parametersNode)
{
  if (!parametersNode)
    {
    return false;
    }
  vtkMRMLAnnotationROINode* roiNode = parametersNode->GetROINode();
  vtkMRMLVolumeNode* volumeNode = parametersNode->GetInputVolumeNode();
  if (!roiNode || !volumeNode)
    {
    return false;
    }

  double volumeBounds_ROI[6] = { 0 }; // volume bounds in ROI's coordinate system

  vtkMRMLTransformNode* roiTransform = roiNode->GetParentTransformNode();
  if (roiTransform && !roiTransform->IsTransformToWorldLinear())
    {
    vtkGenericWarningMacro("CropVolume: ROI is transformed using a non-linear transform. The transformation will be ignored");
    roiTransform = NULL;
    }
  vtkNew<vtkMatrix4x4> worldToROI;
  vtkMRMLTransformNode::GetMatrixTransformBetweenNodes(NULL, roiTransform, worldToROI.GetPointer());

  volumeNode->GetSliceBounds(volumeBounds_ROI, worldToROI.GetPointer());

  double roiCenter[3] = { 0 };
  double roiRadius[3] = { 0 };
  for (int i = 0; i < 3; i++)
    {
    roiCenter[i] = (volumeBounds_ROI[i * 2 + 1] + volumeBounds_ROI[i * 2]) / 2;
    roiRadius[i] = (volumeBounds_ROI[i * 2 + 1] - volumeBounds_ROI[i * 2]) / 2;
    }
  roiNode->SetXYZ(roiCenter);
  roiNode->SetRadiusXYZ(roiRadius);

  return true;
}

//----------------------------------------------------------------------------
void vtkSlicerCropVolumeLogic::RegisterNodes()
{
  if(!this->GetMRMLScene())
    {
    return;
    }
  vtkMRMLCropVolumeParametersNode* pNode = vtkMRMLCropVolumeParametersNode::New();
  this->GetMRMLScene()->RegisterNodeClass(pNode);
  pNode->Delete();
}


void vtkSlicerCropVolumeLogic::SnapROIToVoxelGrid(vtkMRMLAnnotationROINode* inputROI, vtkMRMLVolumeNode* inputVolume)
{
  if (!inputROI || !inputVolume)
    {
      vtkDebugMacro(
          "vtkSlicerCropVolumeLogic: Snapping ROI to voxel grid failed (input ROI and/or input Volume are invalid)");
      return;
    }

  vtkSmartPointer<vtkMRMLScene> scene = this->GetMRMLScene();

  if (!scene)
    {
      vtkDebugMacro(
          "vtkSlicerCropVolumeLogic: Snapping ROI to voxel grid failed (MRML Scene is not available)");
      return;
    }

  vtkNew<vtkMatrix4x4> rotationMat;

  bool ok = vtkSlicerCropVolumeLogic::ComputeIJKToRASRotationOnlyMatrix(
      inputVolume, rotationMat.GetPointer());


  if (!ok)
    {
      vtkDebugMacro(
          "vtkSlicerCropVolumeLogic: Snapping ROI to voxel grid failed (IJK to RAS rotation only matrix computation failed)");
      return;
    }

  vtkNew<vtkMRMLLinearTransformNode> roiTransformNode;
  roiTransformNode->ApplyTransformMatrix(rotationMat.GetPointer());
  roiTransformNode->SetScene(this->GetMRMLScene());

  this->GetMRMLScene()->AddNode(roiTransformNode.GetPointer());

  inputROI->SetAndObserveTransformNodeID(roiTransformNode->GetID());
}



//----------------------------------------------------------------------------
bool vtkSlicerCropVolumeLogic::ComputeIJKToRASRotationOnlyMatrix(vtkMRMLVolumeNode* inputVolume, vtkMatrix4x4* outputMatrix)
{
  if(inputVolume == NULL || outputMatrix == NULL)
    return false;

  vtkNew<vtkMatrix4x4> rotationMat;
  rotationMat->Identity();

  vtkNew<vtkMatrix4x4> inputIJKToRASMat;
  inputVolume->GetIJKToRASMatrix(inputIJKToRASMat.GetPointer());
  const char* scanOrder = vtkMRMLVolumeNode::ComputeScanOrderFromIJKToRAS(inputIJKToRASMat.GetPointer());

  vtkNew<vtkMatrix4x4> orientMat;
  orientMat->Identity();

  bool orientation = vtkSlicerCropVolumeLogic::ComputeOrientationMatrixFromScanOrder(scanOrder,orientMat.GetPointer());

  if(!orientation)
    return false;

  orientMat->Invert();

  vtkNew<vtkMatrix4x4> directionsMat;
  directionsMat->Identity();

  inputVolume->GetIJKToRASDirectionMatrix(directionsMat.GetPointer());

  vtkMatrix4x4::Multiply4x4(directionsMat.GetPointer(),orientMat.GetPointer(),rotationMat.GetPointer());

  outputMatrix->DeepCopy(rotationMat.GetPointer());

  return true;
}


//----------------------------------------------------------------------------
bool vtkSlicerCropVolumeLogic::IsVolumeTiltedInRAS( vtkMRMLVolumeNode* inputVolume, vtkMatrix4x4* rotationMatrix)
{
  assert(inputVolume);

  vtkNew<vtkMatrix4x4> iJKToRASMat;
  vtkNew<vtkMatrix4x4> directionMat;

  inputVolume->GetIJKToRASMatrix(iJKToRASMat.GetPointer());
  inputVolume->GetIJKToRASDirectionMatrix(directionMat.GetPointer());

  const char* scanOrder = vtkMRMLVolumeNode::ComputeScanOrderFromIJKToRAS(iJKToRASMat.GetPointer());

  vtkNew<vtkMatrix4x4> orientMat;
  vtkSlicerCropVolumeLogic::ComputeOrientationMatrixFromScanOrder(scanOrder,orientMat.GetPointer());

  bool same = true;

  for(int i=0; i < 4; ++i)
    {
      for(int j=0; j < 4; ++j)
        {
          if(directionMat->GetElement(i,j) != orientMat->GetElement(i,j))
            {
              same = false;
              break;
            }
        }
      if(same == false)
        break;
    }

  if(!rotationMatrix)
    rotationMatrix = vtkSmartPointer<vtkMatrix4x4>::New();

  if(same)
    {
      rotationMatrix->Identity();
    }
  else
    {
      vtkSlicerCropVolumeLogic::ComputeIJKToRASRotationOnlyMatrix(inputVolume,rotationMatrix);
      return true;
    }



  return false;
}

//----------------------------------------------------------------------------
bool
vtkSlicerCropVolumeLogic::ComputeOrientationMatrixFromScanOrder(
    const char *order, vtkMatrix4x4 *outputMatrix)
{
  vtkNew<vtkMatrix4x4> orientMat;
  orientMat->Identity();

  if (!strcmp(order, "IS") || !strcmp(order, "Axial IS")
      || !strcmp(order, "Axial"))
    {
      double elems[] =
        { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
      orientMat->DeepCopy(elems);
    }
  else if (!strcmp(order, "SI") || !strcmp(order, "Axial SI"))
    {
      double elems[] =
        { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1 };
      orientMat->DeepCopy(elems);
    }
  else if (!strcmp(order, "RL") || !strcmp(order, "Sagittal RL")
      || !strcmp(order, "Sagittal"))
    {
      double elems[] =
        { 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 1 };
      orientMat->DeepCopy(elems);
    }
  else if (!strcmp(order, "LR") || !strcmp(order, "Sagittal LR"))
    {
      double elems[] =
        { 0, 0, 1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 1 };
      orientMat->DeepCopy(elems);
    }
  else if (!strcmp(order, "PA") || !strcmp(order, "Coronal PA")
      || !strcmp(order, "Coronal"))
    {
      double elems[] =
        { -1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1 };
      orientMat->DeepCopy(elems);
    }
  else if (!strcmp(order, "AP") || !strcmp(order, "Coronal AP"))
    {
      double elems[] =
        { -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 1 };
      orientMat->DeepCopy(elems);
    }
  else
    {
      return false;
    }


  outputMatrix->DeepCopy(orientMat.GetPointer());
  return true;
}
