/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLClipModelsNode.h,v $
  Date:      $Date: 2006/03/19 17:12:28 $
  Version:   $Revision: 1.6 $

=========================================================================auto=*/

#ifndef __vtkMRMLClipModelsNode_h
#define __vtkMRMLClipModelsNode_h

#include "vtkMRMLNode.h"

class vtkImplicitBoolean;
class vtkMatrix4x4;
class vtkPlane;
class vtkMRMLSliceNode;

/// \brief MRML node to represent three clipping planes.
///
/// The vtkMRMLClipModelsNode MRML node stores
/// the direction of clipping for each of the three clipping planes.
/// It also stores the type of combined clipping operation as either an
/// intersection or union.
class VTK_MRML_EXPORT vtkMRMLClipModelsNode : public vtkMRMLNode
{
public:
  static vtkMRMLClipModelsNode *New();
  vtkTypeMacro(vtkMRMLClipModelsNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  ///
  /// TableModifiedEvent is send when the parent table is modified
  enum
  {
    ClipPlanePoseModifiedEvent = 19000
  };

  //--------------------------------------------------------------------------
  /// MRMLNode methods
  //--------------------------------------------------------------------------

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  ///
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "ClipModels";}

  ///
  /// Indicates the type of clipping
  /// "Intersection" or "Union"
  vtkGetMacro(ClipType, int);
  void SetClipType(int clipType);

  enum
    {
      ClipIntersection = 0,
      ClipUnion = 1
    };

  ///
  /// Indicates if the Red slice clipping is Off,
  /// Positive space, or Negative space
  int GetRedSliceClipState();
  void SetRedSliceClipState(int state);

  ///
  /// Indicates if the Yellow slice clipping is Off,
  /// Positive space, or Negative space
  int GetYellowSliceClipState();
  void SetYellowSliceClipState(int state);

  ///
  /// Indicates if the Green slice clipping is Off,
  /// Positive space, or Negative space
  int GetGreenSliceClipState();
  void SetGreenSliceClipState(int state);

  enum
    {
      ClipOff = 0,
      ClipPositiveSpace = 1,
      ClipNegativeSpace = 2,
    };

  ///
  ///Indicates what clipping method should be used
  ///Straight cut, whole cell extraction, or whole cell extraction with boundary cells
  typedef enum
  {
    Straight = 0,
    WholeCells,
    WholeCellsWithBoundary,
  } ClippingMethodType;

  vtkGetMacro(ClippingMethod, ClippingMethodType);
  vtkSetMacro(ClippingMethod, ClippingMethodType);

  //Convert between enum and string
  static int GetClippingMethodFromString(const char* name);
  static const char* GetClippingMethodAsString(ClippingMethodType id);

  void SetAndObserveRedSliceNodeID(const char* nodeID);
  void SetAndObserveYellowSliceNodeID(const char* nodeID);
  void SetAndObserveGreenSliceNodeID(const char* nodeID);

  const char* GetRedSliceNodeID();
  const char* GetYellowSliceNodeID();
  const char* GetGreenSliceNodeID();

  vtkMRMLSliceNode* GetRedSliceNode();
  vtkMRMLSliceNode* GetYellowSliceNode();
  vtkMRMLSliceNode* GetGreenSliceNode();

  vtkImplicitBoolean* GetClippingFunction();
  void GetTransformedClippingFunction(vtkMatrix4x4* transformToWorld, vtkImplicitBoolean* clippingFunction);

  /// Returns true if any clipping is enabled.
  bool IsClipping();

  void ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData) VTK_OVERRIDE;

protected:
  vtkMRMLClipModelsNode();
  ~vtkMRMLClipModelsNode();
  vtkMRMLClipModelsNode(const vtkMRMLClipModelsNode&);
  void operator=(const vtkMRMLClipModelsNode&);

  void SetSliceClipState(int sliceIndex, int state);
  void UpdateClipPlanePose(vtkPlane* slicePlane, vtkMatrix4x4* sliceMatrix, int clipState);
  vtkMRMLSliceNode* GetSliceNode(int sliceIndex);

  int ClipType;
  ClippingMethodType ClippingMethod;

  enum
  {
    RED_SLICE,
    YELLOW_SLICE,
    GREEN_SLICE,
    NUMBER_OF_SLICE_PLANES // this line must be last
  };
  int SliceClipStates[NUMBER_OF_SLICE_PLANES];
  vtkPlane* SlicePlanes[NUMBER_OF_SLICE_PLANES];
  vtkImplicitBoolean* SlicePlanesFunction;
};

#endif
