import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *

#
# TablesSelfTest
#

class TablesSelfTest(ScriptedLoadableModule):
  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    parent.title = "TablesSelfTest"
    parent.categories = ["Testing.TestCases"]
    parent.dependencies = ["Tables"]
    parent.contributors = ["Andras Lasso (PerkLab, Queen's)"]
    parent.helpText = """
    This is a self test for the Subject hierarchy core plugins.
    """
    parent.acknowledgementText = """This file was originally developed by Andras Lasso, PerkLab, Queen's University and was supported through the Applied Cancer Research Unit program of Cancer Care Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care""" # replace with organization, grant and thanks.
    self.parent = parent

    # Add this test to the SelfTest module's list for discovery when the module
    # is created.  Since this module may be discovered before SelfTests itself,
    # create the list if it doesn't already exist.
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['TablesSelfTest'] = self.runTest

  def runTest(self):
    tester = TablesSelfTestTest()
    tester.runTest()

#
# TablesSelfTestWidget
#

class TablesSelfTestWidget(ScriptedLoadableModuleWidget):
  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

#
# TablesSelfTestLogic
#

class TablesSelfTestLogic(ScriptedLoadableModuleLogic):
  """This class should implement all the actual
  computation done by your module.  The interface
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget
  """
  def __init__(self):
    pass


class TablesSelfTestTest(ScriptedLoadableModuleTest):
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
    self.test_TablesSelfTest_FullTest1()

  # ------------------------------------------------------------------------------
  def test_TablesSelfTest_FullTest1(self):
    # Check for Tables module
    self.assertTrue( slicer.modules.tables )

    # TODO: Uncomment when #598 is fixed
    # slicer.util.selectModule('Tables')

    self.section_SetupPathsAndNames()
    self.section_MarkupRole()
    self.section_ChartRole()
    self.section_CloneNode()
    self.section_PluginAutoSearch()

  # ------------------------------------------------------------------------------
  def section_SetupPathsAndNames(self):
    # Make sure subject hierarchy auto-creation is on for this test
    tablesWidget = slicer.modules.tables.widgetRepresentation()
    self.assertTrue( tablesWidget != None )

    # Set constants
    self.sampleTableName = 'SampleTable'

  # ------------------------------------------------------------------------------
  def section_MarkupRole(self):
    self.delayDisplay("Markup role",self.delayMs)

    # Create sample markups node
    tableNode = slicer.vtkMRMLTableNode()
    slicer.mrmlScene.AddNode(tableNode)
    tableNode.SetName(self.sampleTableName)
    column = tableNode.AddColumn()
    self.assertTrue( column is not None )

    column.InsertNextValue("some")
    column.InsertNextValue("data")
    column.InsertNextValue("in this")
    column.InsertNextValue("column")

    self.assertTrue( markupsShNode != None )
    self.assertTrue( markupsShNode.GetParentNode() == studyNode )
    self.assertTrue( markupsShNode.GetOwnerPluginName() == 'Markups' )

  # ------------------------------------------------------------------------------
  def section_ChartRole(self):
    self.delayDisplay("Chart role",self.delayMs)

    # Create sample chart node
    chartNode = slicer.vtkMRMLChartNode()
    slicer.mrmlScene.AddNode(chartNode)
    chartNode.SetName(self.sampleChartName)

    # Add markups to subject hierarchy
    from vtkSlicerTablesModuleMRML import vtkMRMLTablesNode

    studyNode = slicer.util.getNode(self.studyName + slicer.vtkMRMLTablesConstants.GetTablesNodeNamePostfix())
    self.assertTrue( studyNode != None )

    chartShNode = vtkMRMLTablesNode.CreateTablesNode(slicer.mrmlScene, studyNode, slicer.vtkMRMLTablesConstants.GetDICOMLevelSeries(), self.sampleChartName, chartNode)

    self.assertTrue( chartShNode != None )
    self.assertTrue( chartShNode.GetParentNode() == studyNode )
    self.assertTrue( chartShNode.GetOwnerPluginName() == 'Charts' )

  # ------------------------------------------------------------------------------
  def section_CloneNode(self):
    self.delayDisplay("Clone node",self.delayMs)

    markupsShNode = slicer.util.getNode(self.sampleTableName + slicer.vtkMRMLTablesConstants.GetTablesNodeNamePostfix())
    self.assertTrue( markupsShNode != None )
    tableNode = markupsShNode.GetAssociatedNode()
    self.assertTrue( tableNode != None )

    # Add storage node for markups node to test cloning those
    markupsStorageNode = slicer.vtkMRMLMarkupsFiducialStorageNode()
    slicer.mrmlScene.AddNode(markupsStorageNode)
    tableNode.SetAndObserveStorageNodeID(markupsStorageNode.GetID())

    # Get clone node plugin
    import qSlicerTablesModuleWidgetsPythonQt
    tablesWidget = slicer.modules.subjecthierarchy.widgetRepresentation()
    self.assertTrue( tablesWidget != None )
    tablesPluginLogic = tablesWidget.pluginLogic()
    self.assertTrue( tablesPluginLogic != None )

    cloneNodePlugin = tablesPluginLogic.tablesPluginByName('CloneNode')
    self.assertTrue( cloneNodePlugin != None )

    # Set markup node as current (i.e. selected in the tree) for clone
    tablesPluginLogic.setCurrentTablesNode(markupsShNode)

    # Get clone node context menu action and trigger
    cloneNodePlugin.nodeContextMenuActions()[0].activate(qt.QAction.Trigger)

    self.assertTrue( slicer.mrmlScene.GetNumberOfNodesByClass('vtkMRMLMarkupsFiducialNode') == 2 )
    self.assertTrue( slicer.mrmlScene.GetNumberOfNodesByClass('vtkMRMLMarkupsDisplayNode') == 2 )
    self.assertTrue( slicer.mrmlScene.GetNumberOfNodesByClass('vtkMRMLMarkupsFiducialStorageNode') == 2 )

    clonedMarkupShNode = slicer.util.getNode(self.sampleTableName + ' Copy' + slicer.vtkMRMLTablesConstants.GetTablesNodeNamePostfix())
    self.assertTrue( clonedMarkupShNode != None )
    clonedMarkupNode = clonedMarkupShNode.GetAssociatedNode()
    self.assertTrue( clonedMarkupNode != None )
    self.assertTrue( clonedMarkupNode.GetName != self.sampleTableName + ' Copy' )
    self.assertTrue( clonedMarkupNode.GetDisplayNode() != None )
    self.assertTrue( clonedMarkupNode.GetStorageNode() != None )

    from vtkSlicerTablesModuleLogic import vtkSlicerTablesModuleLogic
    inSameStudy = vtkSlicerTablesModuleLogic.AreNodesInSameBranch(markupsShNode, clonedMarkupShNode, slicer.vtkMRMLTablesConstants.GetDICOMLevelStudy())
    self.assertTrue( inSameStudy )

  # ------------------------------------------------------------------------------
  def section_PluginAutoSearch(self):
    self.delayDisplay("Plugin auto search",self.delayMs)

    # Disable subject hierarchy auto-creation to be able to test plugin auto search
    tablesWidget = slicer.modules.subjecthierarchy.widgetRepresentation()
    tablesPluginLogic = tablesWidget.pluginLogic()
    self.assertTrue( tablesWidget != None )
    self.assertTrue( tablesPluginLogic != None )
    tablesPluginLogic.autoCreateTables = False

    # Test whether the owner plugin is automatically searched when the associated data node changes
    chartNode2 = slicer.vtkMRMLChartNode()
    chartNode2.SetName(self.sampleChartName + '2')
    slicer.mrmlScene.AddNode(chartNode2)

    clonedMarkupShNode = slicer.util.getNode(self.sampleTableName + ' Copy' + slicer.vtkMRMLTablesConstants.GetTablesNodeNamePostfix())
    clonedMarkupShNode.SetAssociatedNodeID(chartNode2.GetID())

    self.assertTrue( clonedMarkupShNode.GetOwnerPluginName() == 'Charts' )