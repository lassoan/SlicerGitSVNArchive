/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLTransformDisplayNode.h,v $
  Date:      $Date: 2006/03/19 17:12:28 $
  Version:   $Revision: 1.6 $

  =========================================================================auto=*/

#ifndef __vtkMRMLTransformDisplayNode_h
#define __vtkMRMLTransformDisplayNode_h

#include "vtkMRMLDisplayNode.h"

class vtkMRMLVolumeNode;

/// \brief MRML node to represent display properties for transforms visualization in the slice and 3D viewers.
///
/// vtkMRMLTransformDisplayNode nodes store display properties of transforms.
class VTK_MRML_EXPORT vtkMRMLTransformDisplayNode : public vtkMRMLDisplayNode
{
 public:
  static vtkMRMLTransformDisplayNode *New (  );
  vtkTypeMacro ( vtkMRMLTransformDisplayNode,vtkMRMLDisplayNode );
  void PrintSelf ( ostream& os, vtkIndent indent );

  enum VisualizationModes
  {
    VIS_MODE_2D_GLYPH=1,
    VIS_MODE_2D_GRID=2,
    VIS_MODE_2D_CONTOUR=4,
    VIS_MODE_3D_GLYPH=8,
    VIS_MODE_3D_GRID=16,
    VIS_MODE_3D_CONTOUR=32,
    VIS_MODE_3D_BLOCK=64
  };

  enum GlyphSources
  {
    GLYPH_ARROW_3D = 0,
    GLYPH_CONE_3D,
    GLYPH_SPHERE_3D,
  };

  //--------------------------------------------------------------------------
  /// MRMLNode methods
  //--------------------------------------------------------------------------

  virtual vtkMRMLNode* CreateNodeInstance (  );

  ///
  /// Read node attributes from XML (MRML) file
  virtual void ReadXMLAttributes ( const char** atts );

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML ( ostream& of, int indent );


  ///
  /// Copy the node's attributes to this object
  virtual void Copy ( vtkMRMLNode *node );

  ///
  /// Get node XML tag name (like Volume, UnstructuredGrid)
  virtual const char* GetNodeTagName ( ) {return "TransformDisplayNode";};

  ///
  /// alternative method to propagate events generated in Display nodes
  virtual void ProcessMRMLEvents ( vtkObject * /*caller*/,
                                   unsigned long /*event*/,
                                   void * /*callData*/ );

  //--------------------------------------------------------------------------
  /// Display options
  //--------------------------------------------------------------------------

  vtkMRMLVolumeNode* GetReferenceVolumeNode();
  void SetAndObserveReferenceVolumeNode(vtkMRMLNode* node);

  vtkSetMacro(VisualizationMode, unsigned int);
  vtkGetMacro(VisualizationMode, unsigned int);

  // Glyph Parameters
  vtkSetMacro(GlyphPointMax, int);
  vtkGetMacro(GlyphPointMax, int);
  vtkSetMacro(GlyphScale, float);
  vtkGetMacro(GlyphScale, float);
  vtkSetMacro(GlyphThresholdMax, double);
  vtkGetMacro(GlyphThresholdMax, double);
  vtkSetMacro(GlyphThresholdMin, double);
  vtkGetMacro(GlyphThresholdMin, double);
  vtkSetMacro(GlyphSeed, int);
  vtkGetMacro(GlyphSeed, int);
  vtkSetMacro(GlyphSourceOption, int);
  vtkGetMacro(GlyphSourceOption, int);
  // Arrow Parameters
  vtkSetMacro(GlyphArrowScaleDirectional, bool);
  vtkGetMacro(GlyphArrowScaleDirectional, bool);
  vtkSetMacro(GlyphArrowScaleIsotropic, bool);
  vtkGetMacro(GlyphArrowScaleIsotropic, bool);
  vtkSetMacro(GlyphArrowTipLength, float);
  vtkGetMacro(GlyphArrowTipLength, float);
  vtkSetMacro(GlyphArrowTipRadius, float);
  vtkGetMacro(GlyphArrowTipRadius, float);
  vtkSetMacro(GlyphArrowShaftRadius, float);
  vtkGetMacro(GlyphArrowShaftRadius, float);
  vtkSetMacro(GlyphArrowResolution, int);
  vtkGetMacro(GlyphArrowResolution, int);
  // Cone Parameters
  vtkSetMacro(GlyphConeScaleDirectional, bool);
  vtkGetMacro(GlyphConeScaleDirectional, bool);
  vtkSetMacro(GlyphConeScaleIsotropic, bool);
  vtkGetMacro(GlyphConeScaleIsotropic, bool);
  vtkSetMacro(GlyphConeHeight, float);
  vtkGetMacro(GlyphConeHeight, float);
  vtkSetMacro(GlyphConeRadius, float);
  vtkGetMacro(GlyphConeRadius, float);
  vtkSetMacro(GlyphConeResolution, int);
  vtkGetMacro(GlyphConeResolution, int);
  // Sphere Parameters
  vtkSetMacro(GlyphSphereResolution, float);
  vtkGetMacro(GlyphSphereResolution, float);

  // Grid Parameters
  vtkSetMacro(GridScale, float);
  vtkGetMacro(GridScale, float);
  vtkSetMacro(GridSpacingMM, int);
  vtkGetMacro(GridSpacingMM, int);

  // Block Parameters
  vtkSetMacro(BlockScale, float);
  vtkGetMacro(BlockScale, float);
  vtkSetMacro(BlockDisplacementCheck, int);
  vtkGetMacro(BlockDisplacementCheck, int);

  // Contour Parameters
  unsigned int GetNumberOfContourValues();
  void SetContourValues(double*, int size);
  double* GetContourValues();
  vtkSetMacro(ContourDecimation, float);
  vtkGetMacro(ContourDecimation, float);

  // Glyph Slice Parameters
  vtkSetMacro(GlyphSlicePointMax, int);
  vtkGetMacro(GlyphSlicePointMax, int);
  vtkSetMacro(GlyphSliceThresholdMax, double);
  vtkGetMacro(GlyphSliceThresholdMax, double);
  vtkSetMacro(GlyphSliceThresholdMin, double);
  vtkGetMacro(GlyphSliceThresholdMin, double);
  vtkSetMacro(GlyphSliceScale, float);
  vtkGetMacro(GlyphSliceScale, float);
  vtkSetMacro(GlyphSliceSeed, int);
  vtkGetMacro(GlyphSliceSeed, int);

  //Grid Slice Parameters
  vtkSetMacro(GridSliceScale, float);
  vtkGetMacro(GridSliceScale, float);
  vtkSetMacro(GridSliceSpacingMM, int);
  vtkGetMacro(GridSliceSpacingMM, int);

//Parameters
protected:

  // Bitfield specifying which visualization modes are enabled.
  // Bits are specified by the VisualizationModes enum.
  unsigned int VisualizationMode;

  // Glyph Parameters
  int GlyphPointMax;
  float GlyphScale;
  double GlyphThresholdMax;
  double GlyphThresholdMin;
  int GlyphSeed;
  int GlyphSourceOption; //0 - Arrow, 1 - Cone, 2 - Sphere
  // Arrow Parameters
  bool GlyphArrowScaleDirectional;
  bool GlyphArrowScaleIsotropic;
  float GlyphArrowTipLength;
  float GlyphArrowTipRadius;
  float GlyphArrowShaftRadius;
  int GlyphArrowResolution;

  // Cone Parameters
  bool GlyphConeScaleDirectional;
  bool GlyphConeScaleIsotropic;
  float GlyphConeHeight;
  float GlyphConeRadius;
  int GlyphConeResolution;

  // Sphere Parameters
  float GlyphSphereResolution;

  // Grid Parameters
  float GridScale;
  int GridSpacingMM;

  // Block Parameters
  float BlockScale;
  int BlockDisplacementCheck;

  // Contour Parameters
  std::vector<double> ContourValues;
  float ContourDecimation;

  // Glyph Slice Parameters
  int GlyphSlicePointMax;
  double GlyphSliceThresholdMax;
  double GlyphSliceThresholdMin;
  float GlyphSliceScale;
  int GlyphSliceSeed;

  // Grid Slice Parameters
  float GridSliceScale;
  int GridSliceSpacingMM;

 protected:
  vtkMRMLTransformDisplayNode ( );
  ~vtkMRMLTransformDisplayNode ( );
  vtkMRMLTransformDisplayNode ( const vtkMRMLTransformDisplayNode& );
  void operator= ( const vtkMRMLTransformDisplayNode& );

};

#endif
