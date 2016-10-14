import os
import vtk, qt, ctk, slicer
import logging
from SegmentEditorEffects import *

class SegmentEditorAutoCompleteEffect(AbstractScriptedSegmentEditorEffect):
  """ AutoCompleteEffect is an effect that can create a full segmentation
      from a partial segmentation (not all slices are segmented or only
      part of the target structures are painted).
  """

  def __init__(self, scriptedEffect):
    scriptedEffect.name = 'Auto-complete'
    # Indicates that effect does not operate on one segment, but the whole segmentation.
    # This means that while this effect is active, no segment can be selected
    scriptedEffect.perSegment = False
    AbstractScriptedSegmentEditorEffect.__init__(self, scriptedEffect)
    # Stores merged labelmap image geometry (voxel data is not allocated)
    self.mergedLabelmapGeometryImage = None
    self.selectedSegmentIds = None
    self.clippedMasterImageData = None

  def clone(self):
    import qSlicerSegmentationsEditorEffectsPythonQt as effects
    clonedEffect = effects.qSlicerSegmentEditorScriptedEffect(None)
    clonedEffect.setPythonSource(__file__.replace('\\','/'))
    return clonedEffect

  def icon(self):
    iconPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons/AutoComplete.png')
    if os.path.exists(iconPath):
      return qt.QIcon(iconPath)
    return qt.QIcon()

  def helpText(self):
    return """Create an incomplete segmentation using any effects, click Preview to see \
complete segmentation. To refine results: edit segmentation using any effects and click Preview to see updated results. \
Click Apply to finalize results.
Masking settings are bypassed. Minimum two segments are required. If segments overlap, segment higher in the segments table will have priority.
"""

  def setupOptionsFrame(self):
    self.methodSelectorComboBox = qt.QComboBox()
    self.methodSelectorComboBox.addItem("Fill between parallel slices", MORPHOLOGICAL_SLICE_INTERPOLATION)
    self.methodSelectorComboBox.addItem("Grow from seeds", GROWCUT)
    self.methodSelectorComboBox.setToolTip("""<html>Auto-complete methods:<ul style="margin: 0">
<li><b>Fill between slices:</b> Perform complete segmentation on selected slices using any editor effect.
The complete segmentation will be created by interpolating segmentations on slices that were skipped.</li>
<li><b>Expand segments:</b> Create segments using any editor effect: one segment inside
each each region that should belong to a separate segment. Segments will be expanded to create
a complete segmentation, taking into account the master volume content.
(see http://insight-journal.org/browse/publication/977)</li>
</ul></html>""")
    self.scriptedEffect.addLabeledOptionsWidget("Method:", self.methodSelectorComboBox)

    self.previewButton = qt.QPushButton("Preview")
    self.previewButton.objectName = self.__class__.__name__ + 'Preview'
    self.previewButton.setToolTip("Preview complete segmentation")
    self.scriptedEffect.addOptionsWidget(self.previewButton)

    self.cancelButton = qt.QPushButton("Cancel")
    self.cancelButton.objectName = self.__class__.__name__ + 'Cancel'
    self.cancelButton.setToolTip("Clear preview and cancel auto-complete")

    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.objectName = self.__class__.__name__ + 'Apply'
    self.applyButton.setToolTip("Replace segments by previewed result")

    finishFrame = qt.QHBoxLayout()
    finishFrame.addWidget(self.cancelButton)
    finishFrame.addWidget(self.applyButton)
    self.scriptedEffect.addOptionsWidget(finishFrame)

    self.methodSelectorComboBox.connect("currentIndexChanged(int)", self.updateMRMLFromGUI)
    self.previewButton.connect('clicked()', self.onPreview)
    self.cancelButton.connect('clicked()', self.onCancel)
    self.applyButton.connect('clicked()', self.onApply)

  def createCursor(self, widget):
    # Turn off effect-specific cursor for this effect
    return slicer.util.mainWindow().cursor

  def setMRMLDefaults(self):
    self.scriptedEffect.setParameterDefault("AutoCompleteMethod", MORPHOLOGICAL_SLICE_INTERPOLATION)

  def updateGUIFromMRML(self):
    methodIndex = self.methodSelectorComboBox.findData(self.scriptedEffect.parameter("AutoCompleteMethod"))
    wasBlocked = self.methodSelectorComboBox.blockSignals(True)
    self.methodSelectorComboBox.setCurrentIndex(methodIndex)
    self.methodSelectorComboBox.blockSignals(wasBlocked)

    previewNode = self.scriptedEffect.parameterSetNode().GetNodeReference(ResultPreviewNodeReferenceRole)
    self.cancelButton.setEnabled(previewNode is not None)
    self.applyButton.setEnabled(previewNode is not None)

  def updateMRMLFromGUI(self):
    methodIndex = self.methodSelectorComboBox.currentIndex
    method = self.methodSelectorComboBox.itemData(methodIndex)
    self.scriptedEffect.setParameter("AutoCompleteMethod", method)

  def onPreview(self):
    slicer.util.showStatusMessage("Running auto-complete...", 2000)
    self.scriptedEffect.saveStateForUndo()
    self.preview()

  def reset(self):
    previewNode = self.scriptedEffect.parameterSetNode().GetNodeReference(ResultPreviewNodeReferenceRole)
    if previewNode:
      self.scriptedEffect.parameterSetNode().SetNodeReferenceID(ResultPreviewNodeReferenceRole, None)
      slicer.mrmlScene.RemoveNode(previewNode)
    self.mergedLabelmapGeometryImage = None
    self.selectedSegmentIds = None
    self.clippedMasterImageData = None
    self.updateGUIFromMRML()

  def onCancel(self):
    self.reset()

  def onApply(self):
    import vtkSegmentationCorePython as vtkSegmentationCore
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()
    previewNode = self.scriptedEffect.parameterSetNode().GetNodeReference(ResultPreviewNodeReferenceRole)

    # Move segments from preview into current segmentation
    segmentIDs = vtk.vtkStringArray()
    previewNode.GetSegmentation().GetSegmentIDs(segmentIDs)
    for index in xrange(segmentIDs.GetNumberOfValues()):
      segmentID = segmentIDs.GetValue(index)
      previewSegment = previewNode.GetSegmentation().GetSegment(segmentID)
      previewSegmentLabelmap = previewSegment.GetRepresentation(vtkSegmentationCore.vtkSegmentationConverter.GetSegmentationBinaryLabelmapRepresentationName())
      slicer.vtkSlicerSegmentationsModuleLogic.SetBinaryLabelmapToSegment(previewSegmentLabelmap, segmentationNode, segmentID)
      previewNode.GetSegmentation().RemoveSegment(segmentID) # delete now to limit memory usage

    self.reset()

  def preview(self):
    # Get master volume image data
    import vtkSegmentationCorePython as vtkSegmentationCore
    masterImageData = self.scriptedEffect.masterVolumeImageData()
    # Get segmentation
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()

    previewNode = self.scriptedEffect.parameterSetNode().GetNodeReference(ResultPreviewNodeReferenceRole)
    if not previewNode:
      # Compute merged labelmap extent (effective extent slightly expanded)
      self.selectedSegmentIds = vtk.vtkStringArray()
      segmentationNode.GetDisplayNode().GetVisibleSegmentIDs(self.selectedSegmentIds)
      minimumNumberOfSegments = 2 if method == GROWCUT else 1
      if self.selectedSegmentIds.GetNumberOfValues() < minimumNumberOfSegments:
        logging.info("Auto-complete operation skipped: at least {0} visible segments are required".format(minimumNumberOfSegments))
        return
      if not self.mergedLabelmapGeometryImage:
        self.mergedLabelmapGeometryImage = vtkSegmentationCore.vtkOrientedImageData()
      commonGeometryString = segmentationNode.GetSegmentation().DetermineCommonLabelmapGeometry(
        vtkSegmentationCore.vtkSegmentation.EXTENT_UNION_OF_EFFECTIVE_SEGMENTS, self.selectedSegmentIds)
      if not commonGeometryString:
        logging.info("Auto-complete operation skipped: all visible segments are empty")
        return
      vtkSegmentationCore.vtkSegmentationConverter.DeserializeImageGeometry(commonGeometryString, self.mergedLabelmapGeometryImage)

      masterImageExtent = masterImageData.GetExtent()
      labelsEffectiveExtent = self.mergedLabelmapGeometryImage.GetExtent()
      margin = [17, 17, 17]
      labelsExpandedExtent = [
        max(masterImageExtent[0], labelsEffectiveExtent[0]-margin[0]),
        min(masterImageExtent[1], labelsEffectiveExtent[1]+margin[0]),
        max(masterImageExtent[2], labelsEffectiveExtent[2]-margin[1]),
        min(masterImageExtent[3], labelsEffectiveExtent[3]+margin[1]),
        max(masterImageExtent[4], labelsEffectiveExtent[4]-margin[2]),
        min(masterImageExtent[5], labelsEffectiveExtent[5]+margin[2]) ]
      self.mergedLabelmapGeometryImage.SetExtent(labelsExpandedExtent)

      previewNode = slicer.mrmlScene.CreateNodeByClass('vtkMRMLSegmentationNode')
      previewNode = slicer.mrmlScene.AddNode(previewNode)
      previewNode.CreateDefaultDisplayNodes()
      previewNode.GetDisplayNode().SetVisibility2DOutline(False)
      if segmentationNode.GetParentTransformNode():
        previewNode.SetAndObserveTransformNodeID(segmentationNode.GetParentTransformNode().GetID())
      self.scriptedEffect.parameterSetNode().SetNodeReferenceID(ResultPreviewNodeReferenceRole, previewNode.GetID())

      self.clippedMasterImageData = vtkSegmentationCore.vtkOrientedImageData()
      masterImageClipper = vtk.vtkImageConstantPad()
      masterImageClipper.SetInputData(masterImageData)
      masterImageClipper.SetOutputWholeExtent(self.mergedLabelmapGeometryImage.GetExtent())
      masterImageClipper.Update()
      self.clippedMasterImageData.ShallowCopy(masterImageClipper.GetOutput())
      self.clippedMasterImageData.CopyDirections(self.mergedLabelmapGeometryImage)

    previewNode.GetSegmentation().RemoveAllSegments()
    previewNode.SetName(segmentationNode.GetName()+" preview")

    mergedImage = vtkSegmentationCore.vtkOrientedImageData()
    segmentationNode.GenerateMergedLabelmapForAllSegments(mergedImage,
      vtkSegmentationCore.vtkSegmentation.EXTENT_UNION_OF_EFFECTIVE_SEGMENTS, self.mergedLabelmapGeometryImage, self.selectedSegmentIds)

    # Make a zero-valued volume for the output
    outputLabelmap = vtkSegmentationCore.vtkOrientedImageData()

    method = self.scriptedEffect.parameter("AutoCompleteMethod")

    if method == MORPHOLOGICAL_SLICE_INTERPOLATION:
        import vtkITK
        interpolator = vtkITK.vtkITKMorphologicalContourInterpolator()
        interpolator.SetInputData(mergedImage)
        interpolator.Update()
        outputLabelmap.DeepCopy(interpolator.GetOutput())

    elif method == GROWCUT:

      #TODO: get common effective extent from merged labelmap size, pad with zeros, clip master to padded extent

      import vtkSlicerSegmentationsModuleLogicPython as vtkSlicerSegmentationsModuleLogic
      growCutFilter = vtkSlicerSegmentationsModuleLogic.vtkFastGrowCutSeg()
      growCutFilter.SetSourceVol(self.clippedMasterImageData)
      growCutFilter.SetSeedVol(mergedImage)
      growCutFilter.Update()

      outputLabelmap.DeepCopy( growCutFilter.GetOutput() )
    else:
      logging.error("Invalid auto-complete method {0}".format(smoothingMethod))
      return

    # Write output segmentation results in segments
    segmentIDs = vtk.vtkStringArray()
    segmentationNode.GetDisplayNode().GetVisibleSegmentIDs(segmentIDs)
    for index in xrange(segmentIDs.GetNumberOfValues()):
      segmentID = segmentIDs.GetValue(index)
      segment = segmentationNode.GetSegmentation().GetSegment(segmentID)
      # Disable save with scene?

      # Get label corresponding to segment in merged labelmap (and so AutoComplete output)
      colorIndexStr = vtk.mutable("")
      tagFound = segment.GetTag(slicer.vtkMRMLSegmentationDisplayNode.GetColorIndexTag(), colorIndexStr);
      if not tagFound:
        logging.error('Failed to apply AutoComplete result on segment ' + segmentID)
        continue
      colorIndex = int(colorIndexStr.get())

      # Get only the label of the current segment from the output image
      thresh = vtk.vtkImageThreshold()
      thresh.ReplaceInOn()
      thresh.ReplaceOutOn()
      thresh.SetInValue(1)
      thresh.SetOutValue(0)
      thresh.ThresholdBetween(colorIndex, colorIndex);
      thresh.SetOutputScalarType(vtk.VTK_UNSIGNED_CHAR)
      thresh.SetInputData(outputLabelmap)
      thresh.Update()

      # Write label to segment
      newSegmentLabelmap = vtkSegmentationCore.vtkOrientedImageData()
      newSegmentLabelmap.ShallowCopy(thresh.GetOutput())
      newSegmentLabelmap.CopyDirections(mergedImage)
      newSegment = vtkSegmentationCore.vtkSegment()
      newSegment.SetName(segment.GetName())
      previewNode.GetSegmentation().AddSegment(newSegment, segmentID)
      previewNode.GetDisplayNode().SetSegmentColor(segmentID, segmentationNode.GetDisplayNode().GetSegmentColor(segmentID))
      slicer.vtkSlicerSegmentationsModuleLogic.SetBinaryLabelmapToSegment(newSegmentLabelmap, previewNode, segmentID)

    self.updateGUIFromMRML()


MORPHOLOGICAL_SLICE_INTERPOLATION = 'MORPHOLOGICAL_SLICE_INTERPOLATION'
GROWCUT = 'GROWCUT'

ResultPreviewNodeReferenceRole = "AutoCompleteSegmentationResultPreview"