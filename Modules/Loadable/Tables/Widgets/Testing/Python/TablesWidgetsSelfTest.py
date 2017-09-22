import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *

#
# TablesWidgetsSelfTest
#

class TablesWidgetsSelfTest(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "TablesWidgetsSelfTest"
    self.parent.categories = ["Testing.TestCases"]
    self.parent.dependencies = ["Tables"]
    self.parent.contributors = ["Andras Lasso (PerkLab, Queen's)"]
    self.parent.helpText = """This is a self test for Tables widgets."""
    self.parent.acknowledgementText = """This file was originally developed by Andras Lasso,
PerkLab, Queen's University and was supported through the Applied Cancer Research Unit program
of Cancer Care Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care"""

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['TablesWidgetsSelfTest'] = self.runTest

  def runTest(self):
    tester = TablesWidgetsSelfTestTest()
    tester.runTest()

#
# TablesWidgetsSelfTestWidget
#

class TablesWidgetsSelfTestWidget(ScriptedLoadableModuleWidget):
  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

#
# TablesWidgetsSelfTestLogic
#

class TablesWidgetsSelfTestLogic(ScriptedLoadableModuleLogic):
  """This class should implement all the actual
  computation done by your module.  The interface
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget
  """
  def __init__(self):
    pass


class TablesWidgetsSelfTestTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

    self.delayMs = 700

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_TablesWidgetsSelfTest_FullTest1()

  # ------------------------------------------------------------------------------
  def test_TablesWidgetsSelfTest_FullTest1(self):
    # Check for Tables module
    self.assertTrue( slicer.modules.tables )

    self.section_SetupPathsAndNames()
    self.section_CreateTables()
    self.section_SimpleTablesWidget()
    self.section_TablesPlaceWidget()
    self.delayDisplay("Test passed",self.delayMs)

  # ------------------------------------------------------------------------------
  def section_SetupPathsAndNames(self):
    # Set constants
    self.sampleTablesNodeName1 = 'SampleTables1'
    self.sampleTablesNodeName2 = 'SampleTables2'

  # ------------------------------------------------------------------------------
  def section_CreateTables(self):
    self.delayDisplay("Create table nodes",self.delayMs)

    self.TablesLogic = slicer.modules.Tables.logic()

    # Create sample Tables node
    self.TablesNode1 = slicer.mrmlScene.GetNodeByID(self.TablesLogic.AddNewFiducialNode())
    self.TablesNode1.SetName(self.sampleTablesNodeName1)

    self.TablesNode2 = slicer.mrmlScene.GetNodeByID(self.TablesLogic.AddNewFiducialNode())
    self.TablesNode2.SetName(self.sampleTablesNodeName2)

  # ------------------------------------------------------------------------------
  def section_SimpleTablesWidget(self):
    self.delayDisplay("Test SimpleTablesWidget",self.delayMs)

    simpleTablesWidget = slicer.qSlicerTableColumnPropertiesWidget()
    nodeSelector = slicer.util.findChildren(simpleTablesWidget,"TablesFiducialNodeComboBox")[0]

    simpleTablesWidget.setMRMLScene(slicer.mrmlScene)
    simpleTablesWidget.show()

    placeWidget = simpleTablesWidget.TablesPlaceWidget()
    self.assertIsNotNone(placeWidget)

    simpleTablesWidget.setCurrentNode(None)
    simpleTablesWidget.enterPlaceModeOnNodeChange = False
    placeWidget.placeModeEnabled = False
    nodeSelector.setCurrentNode(self.TablesNode1)
    self.assertFalse(placeWidget.placeModeEnabled)

    simpleTablesWidget.enterPlaceModeOnNodeChange = True
    nodeSelector.setCurrentNode(self.TablesNode2)
    self.assertTrue(placeWidget.placeModeEnabled)

    simpleTablesWidget.jumpToSliceEnabled = True
    self.assertTrue(simpleTablesWidget.jumpToSliceEnabled)
    simpleTablesWidget.jumpToSliceEnabled = False
    self.assertFalse(simpleTablesWidget.jumpToSliceEnabled)

    simpleTablesWidget.nodeSelectorVisible = False
    self.assertFalse(simpleTablesWidget.nodeSelectorVisible)
    simpleTablesWidget.nodeSelectorVisible = True
    self.assertTrue(simpleTablesWidget.nodeSelectorVisible)

    simpleTablesWidget.optionsVisible = False
    self.assertFalse(simpleTablesWidget.optionsVisible)
    simpleTablesWidget.optionsVisible = True
    self.assertTrue(simpleTablesWidget.optionsVisible)

    defaultColor = qt.QColor(0,255,0)
    simpleTablesWidget.defaultNodeColor = defaultColor
    self.assertEqual(simpleTablesWidget.defaultNodeColor, defaultColor)

    self.TablesNode3 = nodeSelector.addNode()
    displayNode3 = self.TablesNode3.GetDisplayNode()
    color3 = displayNode3.GetColor()
    self.assertEqual(color3[0]*255, defaultColor.red())
    self.assertEqual(color3[1]*255, defaultColor.green())
    self.assertEqual(color3[2]*255, defaultColor.blue())

    numberOfFiducialsAdded = 5
    for i in range(numberOfFiducialsAdded):
      self.TablesLogic.AddFiducial()

    tableWidget = simpleTablesWidget.tableWidget()
    self.assertEqual(tableWidget.rowCount, numberOfFiducialsAdded)

  # ------------------------------------------------------------------------------
  def section_TablesPlaceWidget(self):
    self.delayDisplay("Test TablesPlaceWidget",self.delayMs)

    placeWidget = slicer.qSlicerTablesPlaceWidget()
    placeWidget.setMRMLScene(slicer.mrmlScene)
    placeWidget.setCurrentNode(self.TablesNode1)
    placeWidget.show()

    placeWidget.buttonsVisible = False
    self.assertFalse(placeWidget.buttonsVisible)
    placeWidget.buttonsVisible = True
    self.assertTrue(placeWidget.buttonsVisible)

    placeWidget.deleteAllTablesOptionVisible = False
    self.assertFalse(placeWidget.deleteAllTablesOptionVisible)
    placeWidget.deleteAllTablesOptionVisible = True
    self.assertTrue(placeWidget.deleteAllTablesOptionVisible)

    placeWidget.deleteAllTablesOptionVisible = False
    self.assertFalse(placeWidget.deleteAllTablesOptionVisible)
    placeWidget.deleteAllTablesOptionVisible = True
    self.assertTrue(placeWidget.deleteAllTablesOptionVisible)

    placeWidget.placeMultipleTables = slicer.qSlicerTablesPlaceWidget.ForcePlaceSingleMarkup
    placeWidget.placeModeEnabled = True
    self.assertFalse(placeWidget.placeModePersistency)

    placeWidget.placeMultipleTables = slicer.qSlicerTablesPlaceWidget.ForcePlaceMultipleTables
    placeWidget.placeModeEnabled = False
    placeWidget.placeModeEnabled = True
    self.assertTrue(placeWidget.placeModePersistency)
