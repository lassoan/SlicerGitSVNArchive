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
    self.methodSelectorComboBox.addItem("Fill between slices", MORPHOLOGICAL_SLICE_INTERPOLATION)
    self.methodSelectorComboBox.addItem("Expand segments", GROWCUT)
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

  def onCancel(self):
    previewNode = self.scriptedEffect.parameterSetNode().GetNodeReference(ResultPreviewNodeReferenceRole)
    if previewNode:
      self.scriptedEffect.parameterSetNode().SetNodeReferenceID(ResultPreviewNodeReferenceRole, None)
      slicer.mrmlScene.RemoveNode(previewNode)
    self.updateGUIFromMRML()

  def onApply(self):
    previewNode = self.scriptedEffect.parameterSetNode().GetNodeReference(ResultPreviewNodeReferenceRole)
    # TODO: move segments into current segmentation
    if previewNode:
      self.scriptedEffect.parameterSetNode().SetNodeReferenceID(ResultPreviewNodeReferenceRole, None)
      slicer.mrmlScene.RemoveNode(previewNode)
    self.updateGUIFromMRML()

  def preview(self):
    # Get master volume image data
    import vtkSegmentationCorePython as vtkSegmentationCore
    masterImageData = self.scriptedEffect.masterVolumeImageData()
    # Get segmentation
    segmentationNode = self.scriptedEffect.parameterSetNode().GetSegmentationNode()

    # Generate merged labelmap as input to AutoComplete
    mergedImage = vtkSegmentationCore.vtkOrientedImageData()
    segmentationNode.GenerateMergedLabelmapForAllSegments(mergedImage, vtkSegmentationCore.vtkSegmentation.EXTENT_UNION_OF_SEGMENTS, masterImageData)

    # Make a zero-valued volume for the output
    outputLabelmap = vtkSegmentationCore.vtkOrientedImageData()
    thresh = vtk.vtkImageThreshold()
    thresh.ReplaceInOn()
    thresh.ReplaceOutOn()
    thresh.SetInValue(0)
    thresh.SetOutValue(0)
    thresh.SetOutputScalarType( vtk.VTK_SHORT )
    thresh.SetInputData( mergedImage )
    thresh.SetOutput( outputLabelmap )
    thresh.Update()
    outputLabelmap.DeepCopy( mergedImage ) #TODO: It was thresholded just above, why deep copy now?

    method = self.scriptedEffect.parameter("AutoCompleteMethod")

    if method == MORPHOLOGICAL_SLICE_INTERPOLATION:
        import vtkITK
        interpolator = vtkITK.vtkITKMorphologicalContourInterpolator()
        interpolator.SetInputData(mergedImage)
        interpolator.Update()
        outputLabelmap.DeepCopy(interpolator.GetOutput())

    elif method == GROWCUT:
      # Cast master image if not short
      if masterImageData.GetScalarType() != vtk.VTK_SHORT:
        imageCast = vtk.vtkImageCast()
        imageCast.SetInputData(masterImageData)
        imageCast.SetOutputScalarTypeToShort()
        imageCast.ClampOverflowOn()
        imageCast.Update()
        masterImageDataShort = vtkSegmentationCore.vtkOrientedImageData()
        masterImageDataShort.DeepCopy(imageCast.GetOutput()) # Copy image data
        masterImageDataShort.CopyDirections(masterImageData) # Copy geometry
        masterImageData = masterImageDataShort

      # Perform grow cut
      import vtkITK
      growCutFilter = vtkITK.vtkITKGrowCutSegmentationImageFilter()
      growCutFilter.SetInputData( 0, masterImageData )
      growCutFilter.SetInputData( 1, mergedImage )
      #TODO: This call sets an empty image for the optional "previous segmentation", and
      #      is apparently needed for the first segmentation too. Why?
      growCutFilter.SetInputConnection( 2, thresh.GetOutputPort() )

      #TODO: These are magic numbers inherited from EditorLib/AutoComplete.py
      objectSize = 5.
      contrastNoiseRatio = 0.8
      priorStrength = 0.003
      segmented = 2
      conversion = 1000

      spacing = mergedImage.GetSpacing()
      voxelVolume = reduce(lambda x,y: x*y, spacing)
      voxelAmount = objectSize / voxelVolume
      voxelNumber = round(voxelAmount) * conversion

      cubeRoot = 1./3.
      oSize = int(round(pow(voxelNumber,cubeRoot)))

      growCutFilter.SetObjectSize( oSize )
      growCutFilter.SetContrastNoiseRatio( contrastNoiseRatio )
      growCutFilter.SetPriorSegmentConfidence( priorStrength )
      growCutFilter.Update()

      outputLabelmap.DeepCopy( growCutFilter.GetOutput() )
    else:
      logging.error("Invalid auto-complete method {0}".format(smoothingMethod))
      return

    previewNode = self.scriptedEffect.parameterSetNode().GetNodeReference(ResultPreviewNodeReferenceRole)
    if not previewNode:
      previewNode = slicer.mrmlScene.CreateNodeByClass('vtkMRMLSegmentationNode')
      #referenceImageGeometry = segmentationNode.GetSegmentation().GetConversionParameter(
      #  vtkSegmentationCore.vtkSegmentationConverter.GetReferenceImageGeometryParameterName())
      #previewNode.GetSegmentation().SetConversionParameter(
      #  vtkSegmentationCore.vtkSegmentationConverter.GetReferenceImageGeometryParameterName(), referenceImageGeometry)
      previewNode = slicer.mrmlScene.AddNode(previewNode)
      previewNode.CreateDefaultDisplayNodes()
      if segmentationNode.GetParentTransformNode():
        previewNode.SetAndObserveTransformNodeID(segmentationNode.GetParentTransformNode().GetID())
      self.scriptedEffect.parameterSetNode().SetNodeReferenceID(ResultPreviewNodeReferenceRole, previewNode.GetID())
    else:
      previewNode.GetSegmentation().RemoveAllSegments()

    # Write output segmentation results in segments
    segmentIDs = vtk.vtkStringArray()
    segmentationNode.GetSegmentation().GetSegmentIDs(segmentIDs)
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
      #slicer.vtkSlicerSegmentationsModuleLogic.SetBinaryLabelmapToSegment(newSegmentLabelmap,
      #  segmentationNode, segmentID, slicer.vtkSlicerSegmentationsModuleLogic.MODE_REPLACE, newSegmentLabelmap.GetExtent())
      newSegment = vtkSegmentationCore.vtkSegment()
      maybe set name to avoid crash when hovering over?
      previewNode.GetSegmentation().AddSegment(newSegment, segmentID)
      slicer.vtkSlicerSegmentationsModuleLogic.SetBinaryLabelmapToSegment(newSegmentLabelmap, previewNode, segmentID)

    self.updateGUIFromMRML()


MORPHOLOGICAL_SLICE_INTERPOLATION = 'MORPHOLOGICAL_SLICE_INTERPOLATION'
GROWCUT = 'GROWCUT'

ResultPreviewNodeReferenceRole = "AutoCompleteSegmentationResultPreview"