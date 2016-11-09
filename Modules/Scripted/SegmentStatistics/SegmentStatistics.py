import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# SegmentStatistics
#

class SegmentStatistics(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    import string
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Segment Statistics"
    self.parent.categories = ["Quantification"]
    self.parent.dependencies = []
    self.parent.contributors = ["Andras Lasso (PerkLab), Steve Pieper (Isomics)"]
    self.parent.helpText = """
Use this module to calculate counts and volumes for segments plus statistics on the grayscale background volume.
"""
    self.parent.helpText += self.getDefaultModuleDocumentationLink()
    self.parent.acknowledgementText = """
Supported by NA-MIC, NAC, BIRN, NCIGT, and the Slicer Community. See http://www.slicer.org for details.
"""

#
# SegmentStatisticsWidget
#

class SegmentStatisticsWidget(ScriptedLoadableModuleWidget):
  """Uses ScriptedLoadableModuleWidget base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

    self.chartOptions = ("Count", "Volume mm^3", "Volume cc", "Min", "Max", "Mean", "StdDev")

    self.logic = None
    self.grayscaleNode = None
    self.labelNode = None
    self.fileName = None
    self.fileDialog = None

    # Instantiate and connect widgets ...
    #

    # Inputs
    inputsCollapsibleButton = ctk.ctkCollapsibleButton()
    inputsCollapsibleButton.text = "Inputs"
    self.layout.addWidget(inputsCollapsibleButton)
    inputsFormLayout = qt.QFormLayout(inputsCollapsibleButton)

    # Segmentation selector
    self.segmentationSelector = slicer.qMRMLNodeComboBox()
    self.segmentationSelector.nodeTypes = ["vtkMRMLSegmentationNode"]
    self.segmentationSelector.addEnabled = False
    self.segmentationSelector.removeEnabled = True
    self.segmentationSelector.renameEnabled = True
    self.segmentationSelector.setMRMLScene( slicer.mrmlScene )
    self.segmentationSelector.setToolTip( "Pick the segmentation to compute statistics for" )
    inputsFormLayout.addRow("Segmentation:", self.segmentationSelector)

    # Grayscale volume selector
    self.grayscaleSelector = slicer.qMRMLNodeComboBox()
    self.grayscaleSelector.nodeTypes = ["vtkMRMLScalarVolumeNode"]
    self.grayscaleSelector.addEnabled = False
    self.grayscaleSelector.removeEnabled = True
    self.grayscaleSelector.renameEnabled = True
    self.grayscaleSelector.noneEnabled = True
    self.grayscaleSelector.showChildNodeTypes = False
    self.grayscaleSelector.setMRMLScene( slicer.mrmlScene )
    self.grayscaleSelector.setToolTip( "Select the grayscale volume for intensity statistics calculations")
    inputsFormLayout.addRow("Grayscale volume:", self.grayscaleSelector)

    # Output table selector
    outputCollapsibleButton = ctk.ctkCollapsibleButton()
    outputCollapsibleButton.text = "Output"
    self.layout.addWidget(outputCollapsibleButton)
    outputFormLayout = qt.QFormLayout(outputCollapsibleButton)

    self.outputTableSelector = slicer.qMRMLNodeComboBox()
    self.outputTableSelector.nodeTypes = ["vtkMRMLTableNode"]
    self.outputTableSelector.addEnabled = True
    self.outputTableSelector.selectNodeUponCreation = True
    self.outputTableSelector.renameEnabled = True
    self.outputTableSelector.removeEnabled = True
    self.outputTableSelector.noneEnabled = False
    self.outputTableSelector.setMRMLScene( slicer.mrmlScene )
    self.outputTableSelector.setToolTip( "Select the table where statistics will be saved into")
    outputFormLayout.addRow("Output table:", self.outputTableSelector)

    # Apply Button
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Calculate Statistics."
    self.applyButton.enabled = False
    self.parent.layout().addWidget(self.applyButton)

    # Add vertical spacer
    self.parent.layout().addStretch(1)

    # connections
    self.applyButton.connect('clicked()', self.onApply)
    self.grayscaleSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.onNodeSelectionChanged)
    self.segmentationSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.onNodeSelectionChanged)
    self.outputTableSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.onNodeSelectionChanged)

    self.onNodeSelectionChanged()

  def onNodeSelectionChanged(self):
    self.applyButton.enabled = (self.segmentationSelector.currentNode() is not None) and (self.outputTableSelector.currentNode() is not None)
    if self.segmentationSelector.currentNode():
      self.outputTableSelector.baseName = self.segmentationSelector.currentNode().GetName() + ' statistics'

  def onApply(self):
    """Calculate the label statistics
    """

    self.applyButton.text = "Working..."
    self.applyButton.setEnabled(False)
    slicer.app.processEvents()
    self.logic = SegmentStatisticsLogic(self.outputTableSelector.currentNode(), self.segmentationSelector.currentNode(), self.grayscaleSelector.currentNode())
    self.applyButton.setEnabled(True)
    self.applyButton.text = "Apply"

    currentLayout = slicer.app.layoutManager().layout
    layoutWithTable = slicer.modules.tables.logic().GetLayoutWithTable(currentLayout)
    slicer.app.layoutManager().setLayout(layoutWithTable)
    slicer.app.applicationLogic().GetSelectionNode().SetReferenceActiveTableID(self.outputTableSelector.currentNode().GetID())
    slicer.app.applicationLogic().PropagateTableSelection()

#
# SegmentStatisticsLogic
#

class SegmentStatisticsLogic(ScriptedLoadableModuleLogic):
  """Implement the logic to calculate label statistics.
  Nodes are passed in as arguments.
  Results are stored as 'statistics' instance variable.
  Uses ScriptedLoadableModuleLogic base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, outputTableNode, segmentationNode, grayscaleNode):
    import vtkSegmentationCorePython as vtkSegmentationCore

    self.keys = ("Segment", "Count", "Volume mm^3", "Volume cc", "Min", "Max", "Mean", "StdDev")

    self.labelStats = {}
    self.labelStats["SegmentIDs"] = []

    segmentation = segmentationNode.GetSegmentation()

    # Generate merged labelmap of all visible segments
    visibleSegmentIds = vtk.vtkStringArray()
    segmentationNode.GetDisplayNode().GetVisibleSegmentIDs(visibleSegmentIds)
    if visibleSegmentIds.GetNumberOfValues() == 0:
      logging.info("Smoothing operation skipped: there are no visible segments")
      return

    for segmentIndex in range(visibleSegmentIds.GetNumberOfValues()):
      segmentID = visibleSegmentIds.GetValue(segmentIndex)
      segment = segmentation.GetSegment(segmentID)
      segmentLabelmap = segment.GetRepresentation(vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName())

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

      #  use vtk's statistics class with the binary labelmap as a stencil
      stencil = vtk.vtkImageToImageStencil()
      stencil.SetInputData(thresh.GetOutput())
      stencil.ThresholdByUpper(labelValue)
      stencil.Update()

      stat = vtk.vtkImageAccumulate()
      stat.SetInputData(thresh.GetOutput())
      stat.SetStencilData(stencil.GetOutput())
      stat.Update()


      cubicMMPerVoxel = reduce(lambda x,y: x*y, segmentLabelmap.GetSpacing())
      ccPerCubicMM = 0.001

      # add an entry to the LabelStats list
      self.labelStats["SegmentIDs"].append(segmentID)
      self.labelStats[segmentID,"Segment"] = segment.GetName()
      self.labelStats[segmentID,"Count"] = stat.GetVoxelCount()
      self.labelStats[segmentID,"Volume mm^3"] = stat.GetVoxelCount() * cubicMMPerVoxel
      self.labelStats[segmentID,"Volume cc"] = stat.GetVoxelCount() * cubicMMPerVoxel * ccPerCubicMM

      if grayscaleNode and grayscaleNode.GetImageData():
        self.addGrayscaleVolumeStats(segmentID, segmentationNode, segmentLabelmap, grayscaleNode)

      # TODO: add closed surface statistics as well

      self.exportToTable(outputTableNode)

  def addGrayscaleVolumeStats(self, segmentID, segmentationNode, segmentLabelmap, grayscaleNode):
    import vtkSegmentationCorePython as vtkSegmentationCore

    # Get reference geometry in the segmentation node's coordinate system
    # Create (non-allocated) image data that matches reference geometry
    referenceGeometry_Reference = vtkSegmentationCore.vtkOrientedImageData() # reference geometry in reference node coordinate system
    referenceGeometry_Reference.SetExtent(grayscaleNode.GetImageData().GetExtent())
    ijkToRasMatrix = vtk.vtkMatrix4x4()
    grayscaleNode.GetIJKToRASMatrix(ijkToRasMatrix)
    referenceGeometry_Reference.SetGeometryFromImageToWorldMatrix(ijkToRasMatrix)
    # # Transform it to the segmentation node coordinate system
    # referenceGeometry_Segmentation = vtkSegmentationCore.vtkOrientedImageData() # reference geometry in segmentation coordinate system
    # referenceGeometry_Segmentation.DeepCopy(referenceGeometry_Reference)

    # referenceGeometryToSegmentationTransform = vtk.vtkGeneralTransform()
    # slicer.vtkMRMLTransformNode.GetTransformBetweenNodes(grayscaleNode.GetParentTransformNode(),
      # segmentationNode.GetParentTransformNode(), referenceGeometryToSegmentationTransform)
    # vtkSegmentationCore.vtkOrientedImageDataResample.TransformOrientedImage(referenceGeometry_Segmentation, referenceGeometryToSegmentationTransform, True)
    # segmentationToReferenceGeometryTransform = referenceGeometryToSegmentationTransform.GetInverse()
    # segmentationToReferenceGeometryTransform.Update()

    segmentationToReferenceGeometryTransform = vtk.vtkGeneralTransform()
    slicer.vtkMRMLTransformNode.GetTransformBetweenNodes(segmentationNode.GetParentTransformNode(),
      grayscaleNode.GetParentTransformNode(), segmentationToReferenceGeometryTransform)

    segmentLabelmap_Reference = vtkSegmentationCore.vtkOrientedImageData()
    vtkSegmentationCore.vtkOrientedImageDataResample.ResampleOrientedImageToReferenceOrientedImage(
      segmentLabelmap, referenceGeometry_Reference, segmentLabelmap_Reference,
      False, # nearest neighbor interpolation
      False, # no padding
      segmentationToReferenceGeometryTransform)

    # We need to know exactly the value of the segment voxels, apply threshold to make force the selected label value
    labelValue = 1
    backgroundValue = 0
    thresh = vtk.vtkImageThreshold()
    thresh.SetInputData(segmentLabelmap_Reference)
    thresh.ThresholdByLower(0)
    thresh.SetInValue(backgroundValue)
    thresh.SetOutValue(labelValue)
    thresh.SetOutputScalarType(vtk.VTK_UNSIGNED_CHAR)
    thresh.Update()

    #  use vtk's statistics class with the binary labelmap as a stencil
    stencil = vtk.vtkImageToImageStencil()
    stencil.SetInputData(thresh.GetOutput())
    stencil.ThresholdByUpper(labelValue)
    stencil.Update()

    stat = vtk.vtkImageAccumulate()
    stat.SetInputData(grayscaleNode.GetImageData())
    stat.SetStencilData(stencil.GetOutput())
    stat.Update()
    if stat.GetVoxelCount()>0:
      self.labelStats[segmentID,"Min"] = stat.GetMin()[0]
      self.labelStats[segmentID,"Max"] = stat.GetMax()[0]
      self.labelStats[segmentID,"Mean"] = stat.GetMean()[0]
      self.labelStats[segmentID,"StdDev"] = stat.GetStandardDeviation()[0]

  def exportToTable(self, table):
    """
    Export statistics to table node
    """

    tableWasModified = table.StartModify()

    table.RemoveAllColumns()

    # Define table columns
    for k in self.keys:
      col = table.AddColumn()
      col.SetName(k)

    # Fill columns
    for segmentID in self.labelStats["SegmentIDs"]:
      rowIndex = table.AddEmptyRow()
      columnIndex = 0
      for k in self.keys:
        value = self.labelStats[segmentID, k] if self.labelStats.has_key((segmentID, k)) else ""
        table.SetCellText(rowIndex, columnIndex, str(value))
        columnIndex += 1

    table.Modified()
    table.EndModify(tableWasModified)

  def statsAsCSV(self):
    """
    print comma separated value file with header keys in quotes
    """

    colorNode = self.getColorNode()

    csv = ""
    header = ""
    if colorNode:
      header += "\"%s\"" % "Type" + ","
    for k in self.keys[:-1]:
      header += "\"%s\"" % k + ","
    header += "\"%s\"" % self.keys[-1] + "\n"
    csv = header
    for i in self.labelStats["Labels"]:
      line = ""
      if colorNode:
        line += colorNode.GetColorName(i) + ","
      for k in self.keys[:-1]:
        line += str(self.labelStats[i,k]) + ","
      line += str(self.labelStats[i,self.keys[-1]]) + "\n"
      csv += line
    return csv

  def saveStats(self,fileName):
    fp = open(fileName, "w")
    fp.write(self.statsAsCSV())
    fp.close()

class SegmentStatisticsTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  Uses ScriptedLoadableModuleTest base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self,scenario=None):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_SegmentStatisticsBasic()

  def test_SegmentStatisticsBasic(self):
    """
    This tests some aspects of the label statistics
    """

    self.delayDisplay("Starting test_SegmentStatisticsBasic")
    #
    # first, get some data
    #
    import SampleData
    sampleDataLogic = SampleData.SampleDataLogic()
    mrHead = sampleDataLogic.downloadMRHead()
    ctChest = sampleDataLogic.downloadCTChest()
    self.delayDisplay('Two data sets loaded')

    volumesLogic = slicer.modules.volumes.logic()

    mrHeadLabel = volumesLogic.CreateAndAddLabelVolume( slicer.mrmlScene, mrHead, "mrHead-label" )

    warnings = volumesLogic.CheckForLabelVolumeValidity(ctChest, mrHeadLabel)

    self.delayDisplay("Warnings for mismatch:\n%s" % warnings)

    self.assertNotEqual( warnings, "" )

    warnings = volumesLogic.CheckForLabelVolumeValidity(mrHead, mrHeadLabel)

    self.delayDisplay("Warnings for match:\n%s" % warnings)

    self.assertEqual( warnings, "" )

    self.delayDisplay('test_SegmentStatisticsBasic passed!')

class Slicelet(object):
  """A slicer slicelet is a module widget that comes up in stand alone mode
  implemented as a python class.
  This class provides common wrapper functionality used by all slicer modlets.
  """
  # TODO: put this in a SliceletLib
  # TODO: parse command line arge


  def __init__(self, widgetClass=None):
    self.parent = qt.QFrame()
    self.parent.setLayout( qt.QVBoxLayout() )

    # TODO: should have way to pop up python interactor
    self.buttons = qt.QFrame()
    self.buttons.setLayout( qt.QHBoxLayout() )
    self.parent.layout().addWidget(self.buttons)
    self.addDataButton = qt.QPushButton("Add Data")
    self.buttons.layout().addWidget(self.addDataButton)
    self.addDataButton.connect("clicked()",slicer.app.ioManager().openAddDataDialog)
    self.loadSceneButton = qt.QPushButton("Load Scene")
    self.buttons.layout().addWidget(self.loadSceneButton)
    self.loadSceneButton.connect("clicked()",slicer.app.ioManager().openLoadSceneDialog)

    if widgetClass:
      self.widget = widgetClass(self.parent)
      self.widget.setup()
    self.parent.show()

class SegmentStatisticsSlicelet(Slicelet):
  """ Creates the interface when module is run as a stand alone gui app.
  """

  def __init__(self):
    super(SegmentStatisticsSlicelet,self).__init__(SegmentStatisticsWidget)


if __name__ == "__main__":
  # TODO: need a way to access and parse command line arguments
  # TODO: ideally command line args should handle --xml

  import sys
  print( sys.argv )

  slicelet = SegmentStatisticsSlicelet()