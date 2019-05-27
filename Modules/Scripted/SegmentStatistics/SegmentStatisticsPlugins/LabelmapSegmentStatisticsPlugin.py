import vtk
import slicer
from SegmentStatisticsPlugins import SegmentStatisticsPluginBase
from functools import reduce

class LabelmapSegmentStatisticsPlugin(SegmentStatisticsPluginBase):
  """Statistical plugin for Labelmaps"""

  def __init__(self):
    super(LabelmapSegmentStatisticsPlugin,self).__init__()
    self.name = "Labelmap"
    self.keys = ["voxel_count", "volume_mm3", "volume_cm3", "diameter_x", "diameter_y", "diameter_z"]
    self.defaultKeys = ["voxel_count", "volume_mm3", "volume_cm3"]
    #... developer may add extra options to configure other parameters

  def computeStatistics(self, segmentID):
    import vtkSegmentationCorePython as vtkSegmentationCore
    requestedKeys = self.getRequestedKeys()

    segmentationNode = slicer.mrmlScene.GetNodeByID(self.getParameterNode().GetParameter("Segmentation"))

    if len(requestedKeys)==0:
      return {}

    containsLabelmapRepresentation = segmentationNode.GetSegmentation().ContainsRepresentation(
      vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName())
    if not containsLabelmapRepresentation:
      return {}

    segment = segmentationNode.GetSegmentation().GetSegment(segmentID)
    segBinaryLabelName = vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName()
    segmentLabelmap = segment.GetRepresentation(segBinaryLabelName)

    if (not segmentLabelmap
      or not segmentLabelmap.GetPointData()
      or not segmentLabelmap.GetPointData().GetScalars()):
      # No input label data
      return {}

    # We need to know exactly the value of the segment voxels, apply threshold to make force the selected label value
    labelValue = 1
    backgroundValue = 0
    thresh = vtk.vtkImageThreshold()
    thresh.SetInputData(segmentLabelmap)
    thresh.ThresholdByLower(0)
    thresh.SetInValue(backgroundValue)
    thresh.SetOutValue(labelValue)
    thresh.SetOutputScalarType(vtk.VTK_UNSIGNED_CHAR)
    thresh.Update()

    #  Use binary labelmap as a stencil
    stencil = vtk.vtkImageToImageStencil()
    stencil.SetInputData(thresh.GetOutput())
    stencil.ThresholdByUpper(labelValue)
    stencil.Update()

    stat = vtk.vtkImageAccumulate()
    stat.SetInputData(thresh.GetOutput())
    stat.SetStencilData(stencil.GetOutput())
    stat.Update()

    bounds = [0] * 6
    segment.GetBounds(bounds)

    # Add data to statistics list
    cubicMMPerVoxel = reduce(lambda x,y: x*y, segmentLabelmap.GetSpacing())
    ccPerCubicMM = 0.001
    stats = {}
    if "voxel_count" in requestedKeys:
      stats["voxel_count"] = stat.GetVoxelCount()
    if "volume_mm3" in requestedKeys:
      stats["volume_mm3"] = stat.GetVoxelCount() * cubicMMPerVoxel
    if "volume_cm3" in requestedKeys:
      stats["volume_cm3"] = stat.GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM
    if "diameter_x" in requestedKeys:
      stats["diameter_x"] = abs(bounds[1]-bounds[0])
    if "diameter_y" in requestedKeys:
      stats["diameter_y"] = abs(bounds[3] - bounds[2])
    if "diameter_z" in requestedKeys:
      stats["diameter_z"] = abs(bounds[5] - bounds[4])
    return stats

  def getMeasurementInfo(self, key):
    """Get information (name, description, units, ...) about the measurement for the given key"""
    info = {}

    # @fedorov could not find any suitable DICOM quantity code for "number of voxels".
    # DCM has "Number of needles" etc., so probably "Number of voxels"
    # should be added too. Need to discuss with @dclunie. For now, a
    # QIICR private scheme placeholder.
    info["voxel_count"] = \
      self.createMeasurementInfo(name="Voxel count", description="Number of voxels", units="voxels",
                                   quantityDicomCode=self.createCodedEntry("nvoxels", "99QIICR", "Number of voxels", True),
                                   unitsDicomCode=self.createCodedEntry("voxels", "UCUM", "voxels", True))

    info["volume_mm3"] = \
      self.createMeasurementInfo(name="Volume mm3", description="Volume in mm3", units="mm3",
                                   quantityDicomCode=self.createCodedEntry("G-D705", "SRT", "Volume", True),
                                   unitsDicomCode=self.createCodedEntry("mm3", "UCUM", "cubic millimeter", True))

    info["volume_cm3"] = \
      self.createMeasurementInfo(name="Volume cm3", description="Volume in cm3", units="cm3",
                                   quantityDicomCode=self.createCodedEntry("G-D705", "SRT", "Volume", True),
                                   unitsDicomCode=self.createCodedEntry("cm3", "UCUM", "cubic centimeter", True),
                                   measurementMethodDicomCode=self.createCodedEntry("126030", "DCM",
                                                                             "Sum of segmented voxel volumes", True))

    info["diameter_x"] = \
      self.createMeasurementInfo(name="Diameter X mm", description="Diameter along the first axis of the segmentation in mm", units="mm",
                                   quantityDicomCode=self.createCodedEntry("M-02550", "SRT", "Diameter", True),
                                   unitsDicomCode=self.createCodedEntry("mm", "UCUM", "millimeter", True),
                                   derivationDicomCode=self.createCodedEntry("G-A220","SRT","Width", True))
    info["diameter_y"] = \
      self.createMeasurementInfo(name="Diameter Y mm", description="Diameter along the second axis of the segmentation in mm", units="mm",
                                   quantityDicomCode=self.createCodedEntry("M-02550", "SRT", "Diameter", True),
                                   unitsDicomCode=self.createCodedEntry("mm", "UCUM", "millimeter", True),
                                   derivationDicomCode=self.createCodedEntry("G-D7FE","SRT","Length", True))
    info["diameter_z"] = \
      self.createMeasurementInfo(name="Diameter Z mm", description="Diameter along the third axis of the segmentation in mm", units="mm",
                                   quantityDicomCode=self.createCodedEntry("M-02550", "SRT", "Diameter", True),
                                   unitsDicomCode=self.createCodedEntry("mm", "UCUM", "millimeter", True),
                                   derivationDicomCode=self.createCodedEntry("G-D785","SRT","Depth", True))

    return info[key] if key in info else None
