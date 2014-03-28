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
    VIS_MODE_GLYPH,
    VIS_MODE_GRID,
    VIS_MODE_CONTOUR,
    VIS_MODE_LAST // this should be the last mode
  };

  enum GlyphTypes
  {
    GLYPH_TYPE_ARROW,
    GLYPH_TYPE_CONE,
    GLYPH_TYPE_SPHERE,
    GLYPH_TYPE_LAST // this should be the last glyph type
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

  vtkSetMacro(VisualizationMode, int);
  vtkGetMacro(VisualizationMode, int);
  /// Convert visualization mode index to a string for serialization.
  /// Returns an empty string if the index is unknown.
  static const char* ConvertVisualizationModeToString(int modeIndex);
  /// Convert visualization mode string to an index that can be set in VisualizationMode.
  /// Returns -1 if the string is unknown.
  static int ConvertVisualizationModeFromString(const char* modeString);

  // Glyph Parameters
  vtkSetMacro(GlyphMaxNumberOfPoints, int);
  vtkGetMacro(GlyphMaxNumberOfPoints, int);
  vtkSetMacro(GlyphSpacingMm, double);
  vtkGetMacro(GlyphSpacingMm, double);
  vtkSetMacro(GlyphScalePercent, double);
  vtkGetMacro(GlyphScalePercent, double);
  vtkSetMacro(GlyphDisplayRangeMaxMm, double);
  vtkGetMacro(GlyphDisplayRangeMaxMm, double);
  vtkSetMacro(GlyphDisplayRangeMinMm, double);
  vtkGetMacro(GlyphDisplayRangeMinMm, double);
  vtkSetMacro(GlyphRandomSeed, int);
  vtkGetMacro(GlyphRandomSeed, int);
  vtkSetMacro(GlyphType, int);
  vtkGetMacro(GlyphType, int);
  /// Convert glyph type index to a string for serialization.
  /// Returns an empty string if the index is unknown.
  static const char* ConvertGlyphTypeToString(int typeIndex);
  /// Convert glyph type string to an index that can be set in GlyphType.
  /// Returns -1 if the string is unknown.
  static int ConvertGlyphTypeFromString(const char* typeString);
  // 3d parameters
  vtkSetMacro(GlyphScaleDirectional, bool);
  vtkGetMacro(GlyphScaleDirectional, bool);
  vtkSetMacro(GlyphTipLengthPercent, double);
  vtkGetMacro(GlyphTipLengthPercent, double);
  vtkSetMacro(GlyphDiameterPercent, double);
  vtkGetMacro(GlyphDiameterPercent, double);
  vtkSetMacro(GlyphDiameterMm, double);
  vtkGetMacro(GlyphDiameterMm, double);
  vtkSetMacro(GlyphShaftDiameterPercent, double);
  vtkGetMacro(GlyphShaftDiameterPercent, double);
  vtkSetMacro(GlyphResolution, int);
  vtkGetMacro(GlyphResolution, int);

  // Grid Parameters
  vtkSetMacro(GridScalePercent, double);
  vtkGetMacro(GridScalePercent, double);
  vtkSetMacro(GridSpacingMm, double);
  vtkGetMacro(GridSpacingMm, double);

  // Contour Parameters
  unsigned int GetNumberOfContourLevels();
  void SetContourLevelsMm(double*, int size);
  double* GetContourLevelsMm();
  std::string GetContourLevelsMmAsString();
  void SetContourLevelsMmFromString(const char* str, const char separator=' ');

  vtkSetMacro(ContourResolutionMm, double);
  vtkGetMacro(ContourResolutionMm, double);


protected:

  int VisualizationMode;

  // Glyph Parameters
  double GlyphSpacingMm;
  double GlyphScalePercent;
  double GlyphDisplayRangeMaxMm;
  double GlyphDisplayRangeMinMm;
  int GlyphMaxNumberOfPoints;
  int GlyphRandomSeed;
  int GlyphType;
  // 3d parameters
  bool GlyphScaleDirectional;
  double GlyphTipLengthPercent;
  double GlyphDiameterPercent;
  double GlyphDiameterMm;
  double GlyphShaftDiameterPercent;
  int GlyphResolution;

  // Grid Parameters
  double GridScalePercent;
  double GridSpacingMm;

  // Contour Parameters
  std::vector<double> ContourLevelsMm;
  double ContourResolutionMm;

 protected:
  vtkMRMLTransformDisplayNode ( );
  ~vtkMRMLTransformDisplayNode ( );
  vtkMRMLTransformDisplayNode ( const vtkMRMLTransformDisplayNode& );
  void operator= ( const vtkMRMLTransformDisplayNode& );

};

#endif
