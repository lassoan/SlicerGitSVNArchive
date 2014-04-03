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

#include "vtkMRMLModelDisplayNode.h"

class vtkColorTransferFunction;
class vtkPointSet;
class vtkMatrix4x4;
class vtkMRMLProceduralColorNode;
class vtkMRMLTransformNode;
class vtkMRMLVolumeNode;


/// \brief MRML node to represent display properties for transforms visualization in the slice and 3D viewers.
///
/// vtkMRMLTransformDisplayNode nodes store display properties of transforms.
class VTK_MRML_EXPORT vtkMRMLTransformDisplayNode : public vtkMRMLModelDisplayNode
{
 public:
  static vtkMRMLTransformDisplayNode *New (  );
  vtkTypeMacro ( vtkMRMLTransformDisplayNode,vtkMRMLModelDisplayNode );
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

  /// A node that defines the region of interest where the transform should be
  /// displayed.
  vtkMRMLNode* GetRegionNode();
  void SetAndObserveRegionNode(vtkMRMLNode* node);

  vtkSetMacro(VisualizationMode, int);
  vtkGetMacro(VisualizationMode, int);
  /// Convert visualization mode index to a string for serialization.
  /// Returns an empty string if the index is unknown.
  static const char* ConvertVisualizationModeToString(int modeIndex);
  /// Convert visualization mode string to an index that can be set in VisualizationMode.
  /// Returns -1 if the string is unknown.
  static int ConvertVisualizationModeFromString(const char* modeString);

  // Glyph Parameters
  vtkSetMacro(GlyphSpacingMm, double);
  vtkGetMacro(GlyphSpacingMm, double);
  vtkSetMacro(GlyphScalePercent, double);
  vtkGetMacro(GlyphScalePercent, double);
  vtkSetMacro(GlyphDisplayRangeMaxMm, double);
  vtkGetMacro(GlyphDisplayRangeMaxMm, double);
  vtkSetMacro(GlyphDisplayRangeMinMm, double);
  vtkGetMacro(GlyphDisplayRangeMinMm, double);
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
  vtkSetMacro(GridLineDiameterMm, double);
  vtkGetMacro(GridLineDiameterMm, double);
  vtkSetMacro(GridResolutionMm, double);
  vtkGetMacro(GridResolutionMm, double);

  // Contour Parameters
  unsigned int GetNumberOfContourLevels();
  void SetContourLevelsMm(double*, int size);
  double* GetContourLevelsMm();
  void GetContourLevelsMm(std::vector<double> &levels);
  std::string GetContourLevelsMmAsString();
  void SetContourLevelsMmFromString(const char* str);
  static std::vector<double> ConvertContourLevelsFromString(const char* str);
  static std::string ConvertContourLevelsToString(const std::vector<double>& levels);
  static bool IsContourLevelEqual(const std::vector<double>& levels1, const std::vector<double>& levels2);

  vtkSetMacro(ContourResolutionMm, double);
  vtkGetMacro(ContourResolutionMm, double);
  vtkSetMacro(ContourOpacity, double);
  vtkGetMacro(ContourOpacity, double);

  /// Return the glyph polydata for the input slice image.
  /// This is the polydata to use in a 3D view.
  /// Reimplemented to by-pass the check on the input polydata.
  virtual vtkPolyData* GetOutputPolyData();

  /// Generate polydata for 2D transform visualization
  /// sliceToRAS, fieldOfViewOrigin, and fieldOfViewSize should be set from the slice node
  void GetVisualization2d(vtkPolyData* output, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize);

  /// Set the default color table
  /// Create and a procedural color node with default colors and use it for visualization.
  void SetDefaultColors();

  vtkColorTransferFunction* GetColorMap();
  void SetColorMap(vtkColorTransferFunction* newColorMap);
  //std::string GetColorMapAsString();
  //void SetColorMapFromString(const char* str);

protected:

  void GetGlyphVisualization2d(vtkPolyData* output, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize);
  void GetGridVisualization2d(vtkPolyData* output, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize);
  void GetContourVisualization2d(vtkPolyData* output, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize);

  /// Generate polydata for 3D transform visualization
  /// roiToRAS defines the ROI origin and direction
  /// roiSize defines the ROI size (in the ROI coordinate system spacing)
  void GetVisualization3d(vtkPolyData* output, vtkMatrix4x4* roiToRAS, int* roiSize);
  void GetGlyphVisualization3d(vtkPolyData* output, vtkMatrix4x4* roiToRAS, int* roiSize);
  void GetGridVisualization3d(vtkPolyData* output, vtkMatrix4x4* roiToRAS, int* roiSize);
  void GetContourVisualization3d(vtkPolyData* output, vtkMatrix4x4* roiToRAS, int* roiSize);

  /// Takes samples from the displacement field specified by the transformation on a 3D ROI
  /// and stores it in an unstructured grid.
  /// pointGroupSize: the number of points will be N*pointGroupSize (the actual number will be returned in numGridPoints[3])
  void GetTransformedPointSamplesOnRoi(vtkPointSet* pointSet, vtkMatrix4x4* roiToRAS, int* roiSize, double pointSpacingMm, int pointGroupSize=1, int* numGridPoints=NULL);

  /// Takes samples from the displacement field specified by the transformation on a slice
  /// and stores it in an unstructured grid.
  /// pointGroupSize: the number of points will be N*pointGroupSize (the actual number will be returned in numGridPoints[3])
  void GetTransformedPointSamplesOnSlice(vtkPointSet* outputPointSet, vtkMatrix4x4* sliceToRAS, double* fieldOfViewOrigin, double* fieldOfViewSize, double pointSpacing, int pointGroupSize=1, int* numGridPoints=NULL);

  /// Takes samples from the displacement field mganitude, specified by the transformation on a uniform grid
  /// and stores it in an image volume.
  void GetTransformedPointSamplesAsImage(vtkImageData* magnitudeImage, vtkMatrix4x4* ijkToRAS, int* imageSize);

  /// Takes samples from the displacement field specified by the transformation on a uniform grid
  /// and stores it in an unstructured grid.
  /// gridToRAS specifies the grid origin, direction, and spacing
  /// gridSize is a 3-component int array specifying the dimension of the grid
  void GetTransformedPointSamples(vtkPointSet* outputPointSet, vtkMatrix4x4* gridToRAS, int* gridSize);

  /// Extends the gridPolyData points to a grid. If warpedGrid is specified then a warped grid is generated, too.
  void CreateGrid(vtkPolyData* gridPolyData, int numGridPoints[3], vtkPolyData* warpedGrid=NULL);

  virtual vtkMRMLTransformNode* GetTransformNode();

  static std::vector<double> StringToDoubleVector(const char* sourceStr);
  static std::string DoubleVectorToString(const double* values, int numberOfValues);

  int VisualizationMode;

  // Glyph Parameters
  double GlyphSpacingMm;
  double GlyphScalePercent;
  double GlyphDisplayRangeMaxMm;
  double GlyphDisplayRangeMinMm;
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
  double GridLineDiameterMm;
  /// Determines how densely the grid is sampled. Higher value results in more faithful representation of the
  /// deformed lines, but needs more computation time.
  double GridResolutionMm;

  // Contour Parameters
  double ContourResolutionMm;
  /// Opacity of the 3D contour. Between 0 and 1.
  double ContourOpacity;
  std::vector<double> ContourLevelsMm;

  vtkMRMLProceduralColorNode* ColorMapNode;

  // 3D model of the visualized transform to be used in all 3D views
  vtkPolyData* CachedPolyData3d;

 protected:
  vtkMRMLTransformDisplayNode ( );
  ~vtkMRMLTransformDisplayNode ( );
  vtkMRMLTransformDisplayNode ( const vtkMRMLTransformDisplayNode& );
  void operator= ( const vtkMRMLTransformDisplayNode& );

  // Return the number of samples in each grid
  int GetGridSubdivision();

};

#endif
