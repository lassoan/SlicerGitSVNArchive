from __future__ import print_function
import os, copy
import qt
import vtk
import logging

from functools import cmp_to_key

from ctk import ctkDICOMObjectListWidget, ctkDICOMDatabase, ctkDICOMIndexer, ctkDICOMBrowser
import slicer
from slicer.util import VTKObservationMixin

from slicer.util import settingsValue, toBool
import DICOMLib


#########################################################
#
#
comment = """

DICOMWidgets are helper classes to build an interface
to manage DICOM data in the context of slicer.

This code is slicer-specific and relies on the slicer python module
for elements like slicer.dicomDatabase and slicer.mrmlScene

"""


#
#########################################################

def setDatabasePrecacheTags(dicomBrowser=None):
  """query each plugin for tags that should be cached on import
     and set them for the dicom app widget and slicer"""
  if not slicer.dicomDatabase:
    return
  tagsToPrecache = list(slicer.dicomDatabase.tagsToPrecache)
  for pluginClass in slicer.modules.dicomPlugins:
    plugin = slicer.modules.dicomPlugins[pluginClass]()
    tagsToPrecache += list(plugin.tags.values())
  tagsToPrecache = sorted(set(tagsToPrecache))  # remove duplicates
  slicer.dicomDatabase.tagsToPrecache = tagsToPrecache
  if dicomBrowser:
    dicomBrowser.tagsToPrecache = tagsToPrecache


class SizePositionSettingsMixin(object):

  def saveSizeAndPosition(self):

    self.settings.beginGroup('DICOM/{}'.format(self.objectName))
    self.settings.setValue("size", self.size)
    self.settings.setValue("pos", self.pos)
    self.settings.endGroup()

  def restoreSizeAndPosition(self):
    parent = self.parent() if self.parent() else slicer.util.mainWindow()
    screenPos = parent.pos

    self.settings.beginGroup('DICOM/{}'.format(self.objectName))
    self.resize(self.settings.value("size",
                                    qt.QSize(int(parent.width*3/4), int(parent.height*3/4))))
    self.move(self.settings.value("pos",
                                  qt.QPoint(int(screenPos.x() + (parent.width - self.width)/2), screenPos.y())))
    self.settings.endGroup()

    # If window position is no longer valid (for example because the window was
    # placed on a monitor that has been disconnected) then the window may simply not appear
    # for the user. Use geometry validation feature of restoreGeometry to make sure that
    # restored window appears.
    savedGeometry = self.saveGeometry()
    self.restoreGeometry(savedGeometry)


class DICOMDetailsBase(VTKObservationMixin, SizePositionSettingsMixin):
  """Implement the Qt window showing details and possible
  operations to perform on the selected dicom list item.
  This is a helper used in the DICOMWidget class.
  """

  widgetType = None
  closed = qt.Signal() # Invoked when the dicom widget is closed using the close method

  def __init__(self, dicomBrowser=None):
    VTKObservationMixin.__init__(self)
    self.settings = qt.QSettings()

    self.dicomBrowser = dicomBrowser if dicomBrowser is not None else ctkDICOMBrowser()

    self.browserPersistent = settingsValue('DICOM/BrowserPersistent', False, converter=toBool)
    self.advancedView = settingsValue('DICOM/advancedView', 0, converter=int)
    self.horizontalTables = settingsValue('DICOM/horizontalTables', 0, converter=int)

    self.pluginInstances = {}
    self.fileLists = []

    setDatabasePrecacheTags(self.dicomBrowser)

    self.dicomBrowser.connect('databaseDirectoryChanged(QString)', self.onDatabaseDirectoryChanged)
    self.extensionCheckPending = False
    self.dicomBrowser.connect('directoryImported()', self.onDirectoryImported)

  def onSendActionTriggered(self, triggered):
    if len(self.fileLists):
      sendDialog = DICOMSendDialog([dcmFile for sublist in self.fileLists for dcmFile in sublist], self)

  def _findChildren(self, name):
    """Since the ctkDICOMBrowser widgets stolen by the Slicer DICOM browser
    loses their objectName when they are re-parented, this convenience function
    will search in both the ``self`` and ``self.dicomBrowser``.
    """
    try:
      return slicer.util.findChildren(self, name=name)[0]
    except IndexError:
      return slicer.util.findChildren(self.dicomBrowser, name=name)[0]

  def setup(self, showPreview=False):
    """
    main window is a frame with widgets from the app
    widget repacked into it along with slicer-specific
    extra widgets
    """

    self.dicomBrowser.databaseDirectorySelectorVisible = False
    self.dicomBrowser.toolbarVisible = False
    self.dicomBrowser.databaseDirectorySettingsKey = slicer.dicomDatabaseDirectorySettingsKey

    # If DICOM module has not created slicer.dicomDatabase already then it means that
    # no database exists yet and it is up to the DICOM browser to create it.
    if not slicer.dicomDatabase:
      if slicer.app.commandOptions().testingEnabled:
        databaseDirectory = os.path.join(slicer.app.temporaryPath, 'tempDICOMDatabase_'+ctk.ctkDICOMDatabase().schemaVersion())
      else:
        databaseDirectory = self.settings.value(slicer.dicomDatabaseDirectorySettingsKey)
        if not databaseDirectory:
          documentsLocation = qt.QStandardPaths.DocumentsLocation
          documents = qt.QStandardPaths.writableLocation(documentsLocation)
          databaseDirectory = os.path.join(documents, slicer.app.applicationName+"DICOMDatabase")
          self.dicomBrowser = databaseDirectory  # this updates slicer.dicomDatabase

    #
    # create and configure the Slicer browser widget - this involves
    # reaching inside and manipulating the widget hierarchy
    # - TODO: this configuration should be exposed more natively
    #   in the CTK code to avoid the findChildren calls
    #
    self.tables = slicer.util.findChildren(self.dicomBrowser, 'dicomTableManager')[0]
    patientTable = slicer.util.findChildren(self.tables, 'patientsTable')[0]
    self.patientTableView = slicer.util.findChildren(patientTable, 'tblDicomDatabaseView')[0]
    patientSearchBox = slicer.util.findChildren(patientTable, 'leSearchBox')[0]
    studyTable = slicer.util.findChildren(self.tables, 'studiesTable')[0]
    self.studyTableView = slicer.util.findChildren(studyTable, 'tblDicomDatabaseView')[0]
    studySearchBox = slicer.util.findChildren(studyTable, 'leSearchBox')[0]
    seriesTable = slicer.util.findChildren(self.tables, 'seriesTable')[0]
    self.seriesTableView = slicer.util.findChildren(seriesTable, 'tblDicomDatabaseView')[0]
    seriesSearchBox = slicer.util.findChildren(seriesTable, 'leSearchBox')[0]
    self.tableSplitter = qt.QSplitter()
    self.tableSplitter.addWidget(self.patientTableView)
    self.tableSplitter.addWidget(self.studyTableView)
    self.tableSplitter.addWidget(self.seriesTableView)

    self.userFrame = qt.QWidget()
    self.preview = qt.QWidget()

    self.objectName = 'SlicerDICOMBrowser'

    self.setWindowTitle('DICOM Browser')

    self.setLayout(qt.QVBoxLayout())

    # enable export button and make new connection
    self.actionExport = self.dicomBrowser.findChildren('QAction', 'ActionExport')[0]
    self.actionExport.enabled = 1
    self.actionExport.connect('triggered()', self.onExportAction)

    # Move all search boxes to one row to save space
    self.searchFrame = qt.QWidget()
    self.searchFrame.setMaximumHeight(40)
    self.searchLayout = qt.QHBoxLayout(self.searchFrame)
    self.layout().addWidget(self.searchFrame)
    patientsLabel = qt.QLabel('Patients: ')
    self.searchLayout.addWidget(patientsLabel)
    self.searchLayout.addWidget(patientSearchBox)
    studiesLabel = qt.QLabel('Studies: ')
    self.searchLayout.addWidget(studiesLabel)
    self.searchLayout.addWidget(studySearchBox)
    seriesLabel = qt.QLabel('Series: ')
    self.searchLayout.addWidget(seriesLabel)
    self.searchLayout.addWidget(seriesSearchBox)

    # tables goes next, spread across 1 row, 2 columns
    if self.horizontalTables:
      self.tableSplitter.setOrientation(1)
    else:
      self.tableSplitter.setOrientation(0)
    self.layout().addWidget(self.tableSplitter)

    #
    # preview related column
    #
    self.previewLayout = qt.QVBoxLayout()
    if showPreview:
      self.previewLayout.addWidget(self.preview)
    else:
      self.preview.hide()

    #
    # action related column (interacting with slicer)
    #
    self.loadableTableFrame = qt.QWidget()
    self.loadableTableFrame.setMaximumHeight(200)
    self.loadableTableLayout = qt.QFormLayout(self.loadableTableFrame)
    self.layout().addWidget(self.loadableTableFrame)

    self.loadableTableLayout.addWidget(self.userFrame)
    self.userFrame.hide()

    self.loadableTable = DICOMLoadableTable(self.userFrame)
    self.loadableTable.itemChanged.connect(self.onLoadableTableItemChanged)

    #
    # button row for action column
    #
    self.actionButtonsFrame = qt.QWidget()
    self.actionButtonsFrame.setMaximumHeight(40)
    self.actionButtonsFrame.objectName = 'ActionButtonsFrame'
    self.layout().addWidget(self.actionButtonsFrame)

    self.actionButtonLayout = qt.QHBoxLayout()
    self.actionButtonsFrame.setLayout(self.actionButtonLayout)

    self.loadButton = qt.QPushButton('Load')
    self.loadButton.toolTip = 'Load selected items into the scene'
    self.actionButtonLayout.addWidget(self.loadButton)
    self.loadButton.connect('clicked()', self.loadCheckedLoadables)

    self.actionButtonLayout.addStretch(1)

    self.examineButton = qt.QPushButton('Examine')
    self.actionButtonLayout.addWidget(self.examineButton)
    self.examineButton.enabled = False
    self.examineButton.connect('clicked()', self.examineForLoading)

    self.uncheckAllButton = qt.QPushButton('Uncheck All')
    self.actionButtonLayout.addWidget(self.uncheckAllButton)
    self.uncheckAllButton.connect('clicked()', self.uncheckAllLoadables)
    self.actionButtonLayout.addStretch(1)

    self.advancedViewButton = qt.QCheckBox('Advanced')
    self.advancedViewButton.objectName = 'AdvancedViewCheckBox'
    self.actionButtonLayout.addWidget(self.advancedViewButton)
    self.advancedViewButton.enabled = True
    self.advancedViewButton.checked = self.advancedView
    self.advancedViewButton.toggled.connect(self.onAdvancedViewButton)

    self.horizontalViewCheckBox = qt.QCheckBox('Horizontal')
    self.horizontalViewCheckBox.objectName = 'HorizontalViewCheckBox'
    self.horizontalViewCheckBox.checked = self.horizontalTables
    self.horizontalViewCheckBox.connect('clicked()', self.onHorizontalViewCheckBox)
    self.actionButtonLayout.addWidget(self.horizontalViewCheckBox)
    self.toolLayout.addStretch(1)

    self.browserPersistentButton = qt.QCheckBox('Browser Persistent')
    self.browserPersistentButton.objectName = 'BrowserPersistentCheckBox'
    self.browserPersistentButton.toolTip = 'When enabled, DICOM Browser remains open after loading data or switching to another module'
    self.browserPersistentButton.checked = self.browserPersistent
    self.actionButtonLayout.addWidget(self.browserPersistentButton)
    self.browserPersistentButton.connect('stateChanged(int)', self.setBrowserPersistence)

    if self.advancedView:
      self.loadableTableFrame.visible = True
    else:
      self.loadableTableFrame.visible = False
      self.examineButton.visible = False
      self.uncheckAllButton.visible = False

    #
    # Series selection
    #
    self.tables.connect('seriesSelectionChanged(QStringList)', self.onSeriesSelected)

    #
    # Plugin selection widget
    #
    self.pluginSelector = DICOMPluginSelector(self)
    self.loadableTableLayout.addRow(self.pluginSelector, self.loadableTable)
    self.checkBoxByPlugins = []

    for pluginClass in slicer.modules.dicomPlugins:
      self.checkBox = self.pluginSelector.checkBoxByPlugin[pluginClass]
      self.checkBox.connect('stateChanged(int)', self.onPluginStateChanged)
      self.checkBoxByPlugins.append(self.checkBox)

    self.loadButton.enabled = self.seriesTableView.selectedIndexes()

    self.updateVisibility()

  def updateVisibility(self):
    for name in [
      'ActionImport', 'ActionExport', 'ActionQuery', 'ActionSend', 'ActionRemove', 'ActionRepair', 'ActionViewMetadata',
      'AdvancedViewCheckBox', 'HorizontalViewCheckBox', 'BrowserPersistentCheckBox'
    ]:
      visible = settingsValue('DICOM/%s.visible' % name, True, converter=toBool)
      control = self._findChildren(name)
      control.visible = visible

    self.sendControl = self._findChildren('ActionSend')
    self.sendControl.triggered.connect(self.onSendActionTriggered)

  def onDatabaseDirectoryChanged(self, databaseDirectory):
    if not hasattr(slicer, 'dicomDatabase') or not slicer.dicomDatabase:
      slicer.dicomDatabase = ctkDICOMDatabase()
      slicer.app.setDICOMDatabase(slicer.dicomDatabase)
    slicer.dicomDatabase.openDatabase(os.path.join(databaseDirectory, "ctkDICOM.sql"))

  def onDirectoryImported(self):
    """The dicom browser will emit multiple directoryImported
    signals during the same operation, so we collapse them
    into a single check for compatible extensions."""
    if not hasattr(slicer.app, 'extensionsManagerModel'):
      # Slicer may not be built with extension manager support
      return
    if not self.extensionCheckPending:
      self.extensionCheckPending = True
      def timerCallback():
        # Prompting for extension may be undesirable in custom applications.
        # DICOM/PromptForExtensions key can be used to disable this feature.
        promptForExtensionsEnabled = settingsValue('DICOM/PromptForExtensions', True, converter=toBool)
        if promptForExtensionsEnabled:
          self.promptForExtensions()
        self.extensionCheckPending = False
      qt.QTimer.singleShot(0, timerCallback)

  def promptForExtensions(self):
    extensionsToOffer = self.checkForExtensions()
    if len(extensionsToOffer) != 0:
      if len(extensionsToOffer) == 1:
        pluralOrNot = " is"
      else:
        pluralOrNot = "s are"
      message = "The following data type%s in your database:\n\n" % pluralOrNot
      displayedTypeDescriptions = []
      for extension in extensionsToOffer:
        typeDescription = extension['typeDescription']
        if not typeDescription in displayedTypeDescriptions:
          # only display each data type only once
          message += '  ' + typeDescription + '\n'
          displayedTypeDescriptions.append(typeDescription)
      message += "\nThe following extension%s not installed, but may help you work with this data:\n\n" % pluralOrNot
      displayedExtensionNames = []
      for extension in extensionsToOffer:
        extensionName = extension['name']
        if not extensionName in displayedExtensionNames:
          # only display each extension name only once
          message += '  ' + extensionName + '\n'
          displayedExtensionNames.append(extensionName)
      message += "\nYou can install extensions using the Extension Manager option from the View menu."
      slicer.util.infoDisplay(message, parent=self, windowTitle='DICOM')

  def checkForExtensions(self):
    """Check to see if there
    are any registered extensions that might be available to
    help the user work with data in the database.

    1) load extension json description
    2) load info for each series
    3) check if data matches

    then return matches

    See
    http://www.na-mic.org/Bug/view.php?id=4146
    """

    # 1 - load json
    import logging, os, json
    logging.info('Imported a DICOM directory, checking for extensions')
    modulePath = os.path.dirname(slicer.modules.dicom.path)
    extensionDescriptorPath = os.path.join(modulePath, 'DICOMExtensions.json')
    try:
      with open(extensionDescriptorPath, 'r') as extensionDescriptorFP:
        extensionDescriptor = extensionDescriptorFP.read()
        dicomExtensions = json.loads(extensionDescriptor)
    except:
      logging.error('Cannot access DICOMExtensions.json file')
      return

    # 2 - get series info
    #  - iterate though metadata - should be fast even with large database
    #  - the fileValue call checks the tag cache so it's fast
    modalityTag = "0008,0060"
    sopClassUIDTag = "0008,0016"
    sopClassUIDs = set()
    modalities = set()
    for patient in slicer.dicomDatabase.patients():
      for study in slicer.dicomDatabase.studiesForPatient(patient):
        for series in slicer.dicomDatabase.seriesForStudy(study):
          instance0 = slicer.dicomDatabase.filesForSeries(series)[0]
          modality = slicer.dicomDatabase.fileValue(instance0, modalityTag)
          sopClassUID = slicer.dicomDatabase.fileValue(instance0, sopClassUIDTag)
          modalities.add(modality)
          sopClassUIDs.add(sopClassUID)

    # 3 - check if data matches
    extensionsManagerModel = slicer.app.extensionsManagerModel()
    installedExtensions = extensionsManagerModel.installedExtensions
    extensionsToOffer = []
    for extension in dicomExtensions['extensions']:
      extensionName = extension['name']
      if extensionName not in installedExtensions:
        tagValues = extension['tagValues']
        if 'Modality' in tagValues:
          for modality in tagValues['Modality']:
            if modality in modalities:
              extensionsToOffer.append(extension)
        if 'SOPClassUID' in tagValues:
          for sopClassUID in tagValues['SOPClassUID']:
            if sopClassUID in sopClassUIDs:
              extensionsToOffer.append(extension)
    return extensionsToOffer

  def onPluginStateChanged(self, state):
    self.settings.beginWriteArray('DICOM/disabledPlugins')

    for key in self.settings.allKeys():
      self.settings.remove(key)

    plugins = self.pluginSelector.selectedPlugins()
    arrayIndex = 0
    for pluginClass in slicer.modules.dicomPlugins:
      if pluginClass not in plugins:
        self.settings.setArrayIndex(arrayIndex)
        self.settings.setValue(pluginClass, 'disabled')
        arrayIndex += 1

    self.settings.endArray()

  def setBrowserPersistence(self, state):
    self.browserPersistent = state
    self.settings.setValue('DICOM/BrowserPersistent', bool(self.browserPersistent))

  def onAdvancedViewButton(self, checked):
    self.advancedView = checked
    advancedWidgets = [self.loadableTableFrame, self.examineButton, self.uncheckAllButton]
    for widget in advancedWidgets:
      widget.visible = self.advancedView
    if self.advancedView:
      self.loadButton.enabled = self.loadableTable.getNumberOfCheckedItems() > 0
    else:
      self.loadButton.enabled = self.seriesTableView.selectedIndexes()

    self.settings.setValue('DICOM/advancedView', int(self.advancedView))

  def onHorizontalViewCheckBox(self):
    self.tableSplitter.setOrientation(self.horizontalViewCheckBox.checked)
    self.settings.setValue('DICOM/horizontalTables', int(self.horizontalViewCheckBox.checked))

  def onExportAction(self):
    self.exportDialog = slicer.qSlicerDICOMExportDialog()
    self.exportDialog.setMRMLScene(slicer.mrmlScene)
    self.close()
    self.exportDialog.execDialog()

  def open(self):
    self.show()

  def close(self):
    self.hide()
    self.closed.emit()

  def organizeLoadables(self):
    """Review the selected state and confidence of the loadables
    across plugins so that the options the user is most likely
    to want are listed at the top of the table and are selected
    by default. Only offer one pre-selected loadable per series
    unless both plugins mark it as selected and they have equal
    confidence."""

    # first, get all loadables corresponding to a series
    seriesUIDTag = "0020,000E"
    loadablesBySeries = {}
    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        seriesUID = slicer.dicomDatabase.fileValue(loadable.files[0], seriesUIDTag)
        if seriesUID not in loadablesBySeries:
          loadablesBySeries[seriesUID] = [loadable]
        else:
          loadablesBySeries[seriesUID].append(loadable)

    # now for each series, find the highest confidence selected loadables
    # and set all others to be unselected
    for series in loadablesBySeries:
      highestConfidenceValue = -1
      for loadable in loadablesBySeries[series]:
        if loadable.confidence > highestConfidenceValue:
          highestConfidenceValue = loadable.confidence
      for loadable in loadablesBySeries[series]:
        if loadable.confidence < highestConfidenceValue:
          loadable.selected = False

  def onSeriesSelected(self, seriesUIDList):
    self.loadButton.enabled = self.seriesTableView.selectedIndexes() and not self.advancedView
    self.offerLoadables(seriesUIDList, "SeriesUIDList")
    self.sendControl.enabled = len(self.fileLists)

  def offerLoadables(self, uidArgument, role):
    """Get all the loadable options at the currently selected level
    and present them in the loadable table"""
    self.loadableTable.setLoadables([])
    if self.advancedViewButton.checkState() == 2:
      self.loadButton.enabled = False
    self.fileLists = self.getFileListsForRole(uidArgument, role)
    self.examineButton.enabled = len(self.fileLists) != 0
    self.viewMetadataButton.enabled = len(self.fileLists) != 0

  def getFileListsForRole(self, uidArgument, role):
    fileLists = []
    if role == "Series":
      fileLists.append(slicer.dicomDatabase.filesForSeries(uidArgument))
    if role == "SeriesUIDList":
      for uid in uidArgument:
        uid = uid.replace("'", "")
        fileLists.append(slicer.dicomDatabase.filesForSeries(uid))
    if role == "Study":
      series = slicer.dicomDatabase.seriesForStudy(uidArgument)
      for serie in series:
        fileLists.append(slicer.dicomDatabase.filesForSeries(serie))
    if role == "Patient":
      studies = slicer.dicomDatabase.studiesForPatient(uidArgument)
      for study in studies:
        series = slicer.dicomDatabase.seriesForStudy(study)
        for serie in series:
          fileList = slicer.dicomDatabase.filesForSeries(serie)
          fileLists.append(fileList)
    return fileLists

  def uncheckAllLoadables(self):
    self.loadableTable.uncheckAll()

  def onLoadableTableItemChanged(self, item):
    self.loadButton.enabled = self.loadableTable.getNumberOfCheckedItems() > 0

  def examineForLoading(self):
    """For selected plugins, give user the option
    of what to load"""

    self.loadablesByPlugin, loadEnabled = self.getLoadablesFromFileLists(self.fileLists)

    self.loadButton.enabled = loadEnabled
    # self.viewMetadataButton.enabled = loadEnabled
    self.organizeLoadables()
    self.loadableTable.setLoadables(self.loadablesByPlugin)

  def getLoadablesFromFileLists(self, fileLists):
    """Take list of file lists, return loadables by plugin dictionary
    """
    loadablesByPlugin = {}
    loadEnabled = False
    if not isinstance(fileLists, list) or len(fileLists) == 0 or not type(fileLists[0]) in [tuple, list]:
      logging.error('File lists must contain a non-empty list of tuples/lists')
      return loadablesByPlugin, loadEnabled

    allFileCount = missingFileCount = 0
    for fileList in self.fileLists:
      for filePath in fileList:
        allFileCount += 1
        if not os.path.exists(filePath):
          missingFileCount += 1

    if missingFileCount > 0:
      slicer.util.warningDisplay("Warning: %d of %d selected files listed in the database cannot be found on disk."
                                 % (missingFileCount, allFileCount), windowTitle="DICOM")

    if missingFileCount == allFileCount:
      return loadablesByPlugin, loadEnabled

    plugins = self.pluginSelector.selectedPlugins()

    progress = slicer.util.createProgressDialog(parent=self, value=0, maximum=len(plugins))

    for step, pluginClass in enumerate(plugins):
      if pluginClass not in self.pluginInstances:
        self.pluginInstances[pluginClass] = slicer.modules.dicomPlugins[pluginClass]()
      plugin = self.pluginInstances[pluginClass]
      if progress.wasCanceled:
        break
      progress.labelText = '\nChecking %s' % pluginClass
      slicer.app.processEvents()
      progress.setValue(step)
      slicer.app.processEvents()
      try:
        loadablesByPlugin[plugin] = plugin.examineForImport(fileLists)
        # If regular method is not overridden (so returns empty list), try old function
        # Ensuring backwards compatibility: examineForImport used to be called examine
        if not loadablesByPlugin[plugin]:
          loadablesByPlugin[plugin] = plugin.examine(fileLists)
        loadEnabled = loadEnabled or loadablesByPlugin[plugin] != []
      except Exception as e:
        import traceback
        traceback.print_exc()
        slicer.util.warningDisplay("Warning: Plugin failed: %s\n\nSee python console for error message." % pluginClass,
                                   windowTitle="DICOM", parent=self)
        print("DICOM Plugin failed: %s" % str(e))

    progress.close()

    return loadablesByPlugin, loadEnabled

  def isFileListInCheckedLoadables(self, fileList):
    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        if len(loadable.files) != len(fileList) or len(loadable.files) == 0:
          continue
        inputFileListCopy = copy.deepcopy(fileList)
        loadableFileListCopy = copy.deepcopy(loadable.files)
        try:
          inputFileListCopy.sort()
          loadableFileListCopy.sort()
        except Exception:
          pass
        isEqual = True
        for pair in zip(inputFileListCopy, loadableFileListCopy):
          if pair[0] != pair[1]:
            print("{} != {}".format(pair[0], pair[1]))
            isEqual = False
            break
        if not isEqual:
          continue
        return True
    return False

  def loadCheckedLoadables(self):
    """Invoke the load method on each plugin for the loadable
    (DICOMLoadable or qSlicerDICOMLoadable) instances that are selected"""
    if self.advancedViewButton.checkState() == 0:
      self.examineForLoading()

    self.loadableTable.updateSelectedFromCheckstate()

    # TODO: add check that disables all referenced stuff to be considered?
    # get all the references from the checked loadables
    referencedFileLists = []
    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        if hasattr(loadable, 'referencedInstanceUIDs'):
          instanceFileList = []
          for instance in loadable.referencedInstanceUIDs:
            instanceFile = slicer.dicomDatabase.fileForInstance(instance)
            if instanceFile != '':
              instanceFileList.append(instanceFile)
          if len(instanceFileList) and not self.isFileListInCheckedLoadables(instanceFileList):
            referencedFileLists.append(instanceFileList)

    # if applicable, find all loadables from the file lists
    loadEnabled = False
    if len(referencedFileLists):
      (self.referencedLoadables, loadEnabled) = self.getLoadablesFromFileLists(referencedFileLists)

    automaticallyLoadReferences = int(slicer.util.settingsValue('DICOM/automaticallyLoadReferences', qt.QMessageBox.InvalidRole))
    if slicer.app.commandOptions().testingEnabled:
      automaticallyLoadReferences = qt.QMessageBox.No
    if loadEnabled and automaticallyLoadReferences == qt.QMessageBox.InvalidRole:
      self.showReferenceDialogAndProceed()
    elif loadEnabled and automaticallyLoadReferences == qt.QMessageBox.Yes:
      self.addReferencesAndProceed()
    else:
      self.proceedWithReferencedLoadablesSelection()

    return

  def showReferenceDialogAndProceed(self):
    referencesDialog = DICOMReferencesDialog(self, loadables=self.referencedLoadables)
    answer = referencesDialog.exec_()
    if referencesDialog.rememberChoiceAndStopAskingCheckbox.checked == True:
      if answer == qt.QMessageBox.Yes:
        qt.QSettings().setValue('DICOM/automaticallyLoadReferences', qt.QMessageBox.Yes)
      if answer == qt.QMessageBox.No:
        qt.QSettings().setValue('DICOM/automaticallyLoadReferences', qt.QMessageBox.No)
    if answer == qt.QMessageBox.Yes:
      # each check box corresponds to a referenced loadable that was selected by examine;
      # if the user confirmed that reference should be loaded, add it to the self.loadablesByPlugin dictionary
      for plugin in self.referencedLoadables:
        for loadable in [l for l in self.referencedLoadables[plugin] if l.selected]:
          if referencesDialog.checkboxes[loadable].checked:
            self.loadablesByPlugin[plugin].append(loadable)
        self.loadablesByPlugin[plugin] = list(set(self.loadablesByPlugin[plugin]))
      self.proceedWithReferencedLoadablesSelection()
    elif answer == qt.QMessageBox.No:
      self.proceedWithReferencedLoadablesSelection()

  def addReferencesAndProceed(self):
    for plugin in self.referencedLoadables:
      for loadable in [l for l in self.referencedLoadables[plugin] if l.selected]:
        self.loadablesByPlugin[plugin].append(loadable)
      self.loadablesByPlugin[plugin] = list(set(self.loadablesByPlugin[plugin]))
    self.proceedWithReferencedLoadablesSelection()

  def proceedWithReferencedLoadablesSelection(self):
    if not self.warnUserIfLoadableWarningsAndProceed():
      return

    selectedLoadables = self.getAllSelectedLoadables()
    progress = slicer.util.createProgressDialog(parent=self, value=0, maximum=len(selectedLoadables))
    loadingResult = ''

    loadedNodeIDs = []

    @vtk.calldata_type(vtk.VTK_OBJECT)
    def onNodeAdded(caller, event, calldata):
      node = calldata
      if isinstance(node, slicer.vtkMRMLVolumeNode):
        loadedNodeIDs.append(node.GetID())

    def updateProgress(value=None, text=None):
      if value:
        progress.setValue(step)
      if text:
        progress.labelText = text
      slicer.app.processEvents()

    self.addObserver(slicer.mrmlScene, slicer.vtkMRMLScene.NodeAddedEvent, onNodeAdded)

    for step, (loadable, plugin) in enumerate(selectedLoadables.items(), start=1):
      if progress.wasCanceled:
        break
      updateProgress(value=step, text='\nLoading %s' % loadable.name)
      try:
        loadSuccess = plugin.load(loadable)
      except:
        loadSuccess = False
        import traceback
        logging.error("DICOM plugin failed to load '"
          + loadable.name + "' as a '" + plugin.loadType + "'.\n"
          + traceback.format_exc())
      if not loadSuccess:
        loadingResult = '%s\nCould not load: %s as a %s' % (loadingResult, loadable.name, plugin.loadType)
      try:
        for derivedItem in loadable.derivedItems:
          indexer = ctkDICOMIndexer()
          updateProgress(text='\nIndexing %s' % derivedItem)
          indexer.addFile(slicer.dicomDatabase, derivedItem)
      except AttributeError:
        # no derived items or some other attribute error
        pass

    self.removeObserver(slicer.mrmlScene, slicer.vtkMRMLScene.NodeAddedEvent, onNodeAdded)

    loadedFileParameters = {}
    loadedFileParameters['nodeIDs'] = loadedNodeIDs
    slicer.app.ioManager().emitNewFileLoaded(loadedFileParameters)

    progress.close()
    if loadingResult:
      slicer.util.warningDisplay(loadingResult, windowTitle='DICOM loading')

    self.onLoadingFinished()

  def warnUserIfLoadableWarningsAndProceed(self):
    warningsInSelectedLoadables = False
    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        if loadable.selected and loadable.warning != "":
          warningsInSelectedLoadables = True
          logging.warning('Warning in DICOM plugin ' + plugin.loadType + ' when examining loadable ' + loadable.name +
                          ': ' + loadable.warning)
    if warningsInSelectedLoadables:
      warning = "Warnings detected during load.  Examine data in Advanced mode for details.  Load anyway?"
      if not slicer.util.confirmOkCancelDisplay(warning, parent=self):
        return False
    return True

  def getAllSelectedLoadables(self):
    loadables = {}
    for plugin in self.loadablesByPlugin:
      for loadable in self.loadablesByPlugin[plugin]:
        if loadable.selected:
          loadables[loadable] = plugin
    return loadables

  def onLoadingFinished(self):
    if not self.browserPersistent:
      self.close()


class DICOMReferencesDialog(qt.QMessageBox):

  WINDOW_TITLE = "Referenced datasets found"
  WINDOW_TEXT = "The loaded DICOM objects contain references to other datasets you did not select for loading. Please " \
                "select Yes if you would like to load the following referenced datasets, No if you only want to load the " \
                "originally selected series, or Cancel to abort loading."

  def __init__(self, parent, loadables):
    super(DICOMReferencesDialog, self).__init__(parent)
    self.loadables = loadables
    self.checkboxes = dict()
    self.setup()

  def setup(self):
    self._setBasicProperties()
    self._addTextLabel()
    self._addLoadableCheckboxes()
    self.rememberChoiceAndStopAskingCheckbox = qt.QCheckBox('Remember choice and stop asking')
    self.rememberChoiceAndStopAskingCheckbox.toolTip = 'Can be changed later in Application Settings / DICOM'
    self.yesButton = self.addButton(self.Yes)
    self.yesButton.setSizePolicy(qt.QSizePolicy(qt.QSizePolicy.Expanding, qt.QSizePolicy.Preferred))
    self.noButton = self.addButton(self.No)
    self.noButton.setSizePolicy(qt.QSizePolicy(qt.QSizePolicy.Expanding, qt.QSizePolicy.Preferred))
    self.cancelButton = self.addButton(self.Cancel)
    self.cancelButton.setSizePolicy(qt.QSizePolicy(qt.QSizePolicy.Expanding, qt.QSizePolicy.Preferred))
    self.layout().addWidget(self.yesButton, 3, 0, 1, 1)
    self.layout().addWidget(self.noButton, 3, 1, 1, 1)
    self.layout().addWidget(self.cancelButton, 3, 2, 1, 1)
    self.layout().addWidget(self.rememberChoiceAndStopAskingCheckbox, 2, 0, 1, 3)

  def _setBasicProperties(self):
    self.layout().setSpacing(9)
    self.setWindowTitle(self.WINDOW_TITLE)
    self.fontMetrics = qt.QFontMetrics(qt.QApplication.font(self))
    self.setMinimumWidth(self.fontMetrics.width(self.WINDOW_TITLE))

  def _addTextLabel(self):
    label = qt.QLabel(self.WINDOW_TEXT)
    label.wordWrap = True
    self.layout().addWidget(label, 0, 0, 1, 3)

  def _addLoadableCheckboxes(self):
    self.checkBoxGroupBox = qt.QGroupBox("References")
    self.checkBoxGroupBox.setLayout(qt.QFormLayout())
    for plugin in self.loadables:
      for loadable in [l for l in self.loadables[plugin] if l.selected]:
        checkBoxText = loadable.name + ' (' + plugin.loadType + ') '
        cb = qt.QCheckBox(checkBoxText, self)
        cb.checked = True
        cb.setSizePolicy(qt.QSizePolicy(qt.QSizePolicy.Expanding, qt.QSizePolicy.Preferred))
        self.checkboxes[loadable] = cb
        self.checkBoxGroupBox.layout().addWidget(cb)
    self.layout().addWidget(self.checkBoxGroupBox, 1, 0, 1, 3)


class DICOMDetailsDialog(DICOMDetailsBase, qt.QDialog):

  widgetType = "dialog"
  closed = qt.Signal()  # Invoked when the dicom widget is closed using the close method

  def __init__(self, dicomBrowser=None, parent="mainWindow"):
    DICOMDetailsBase.__init__(self, dicomBrowser)
    qt.QDialog.__init__(self, slicer.util.mainWindow() if parent == "mainWindow" else parent)
    self.modal = True
    self.setWindowFlags(
            qt.Qt.WindowMaximizeButtonHint |
            qt.Qt.WindowCloseButtonHint |
            qt.Qt.Window)
    self.setup()

  def open(self):
    if not self.isVisible():
      self.restoreSizeAndPosition()
    self.exec_()

  def closeEvent(self, event):
    qt.QDialog.closeEvent(self, event)

  def resizeEvent(self, event):
    qt.QDialog.resizeEvent(self, event)
    self.saveSizeAndPosition()

  def moveEvent(self, event):
    qt.QDialog.moveEvent(self, event)
    self.saveSizeAndPosition()


class DICOMDetailsWindow(DICOMDetailsDialog):

  widgetType = "window"

  def __init__(self, dicomBrowser=None, parent="mainWindow"):
    super(DICOMDetailsWindow, self).__init__(dicomBrowser, parent)
    self.modal=False

  def open(self):
    if not self.isVisible():
      self.restoreSizeAndPosition()
    self.show()
    self.activateWindow()


class DICOMDetailsDock(DICOMDetailsBase, qt.QFrame):

  widgetType = "dock"
  closed = qt.Signal()  # Invoked when the dicom widget is closed using the close method

  def __init__(self, dicomBrowser=None):
    DICOMDetailsBase.__init__(self, dicomBrowser)
    qt.QFrame.__init__(self)
    self.dock = qt.QDockWidget(slicer.util.mainWindow())
    self.dock.setFeatures(qt.QDockWidget.DockWidgetFloatable |
                          qt.QDockWidget.DockWidgetMovable |
                          qt.QDockWidget.DockWidgetClosable)
    slicer.util.mainWindow().addDockWidget(qt.Qt.TopDockWidgetArea, self.dock)
    self.dock.setWidget(self)
    self.dock.visibilityChanged.connect(self.onVisibilityChanged)
    self.setup()

  def __del__(self):
    self.dock.visibilityChanged.disconnect(self.onVisibilityChanged)

  def open(self):
    if not self.dock.visible:
      self.dock.show()

  def close(self):
    self.dock.hide()
    self.closed.emit()

  def onVisibilityChanged(self, visible):
    if not visible:
      self.close()


class DICOMDetailsWidget(DICOMDetailsBase, qt.QWidget):

  widgetType = "widget"
  closed = qt.Signal()  # Invoked when the dicom widget is closed using the close method

  def __init__(self, dicomBrowser=None, parent=None):
    DICOMDetailsBase.__init__(self, dicomBrowser)
    qt.QWidget.__init__(self, parent)
    self.setup()


class DICOMLoadableTable(qt.QTableWidget):
  """Implement the Qt code for a table of
  selectable slicer data to be made from
  the given dicom files
  """

  def __init__(self, parent, width=350, height=100):
    super(DICOMLoadableTable, self).__init__(parent)
    self.setMinimumHeight(height)
    self.setMinimumWidth(width)
    self.loadables = {}
    self.setLoadables([])
    self.configure()
    slicer.app.connect('aboutToQuit()', self.deleteLater)

  def getNumberOfCheckedItems(self):
    return sum(1 for row in range(self.rowCount) if self.item(row, 0).checkState() == qt.Qt.Checked)

  def configure(self):
    self.setColumnCount(3)
    self.setHorizontalHeaderLabels(['DICOM Data', 'Reader', 'Warnings'])
    self.setSelectionBehavior(qt.QTableView.SelectRows)
    self.horizontalHeader().setSectionResizeMode(qt.QHeaderView.Stretch)
    self.horizontalHeader().setSectionResizeMode(0, qt.QHeaderView.ResizeToContents)
    self.horizontalHeader().setSectionResizeMode(1, qt.QHeaderView.Stretch)
    self.horizontalHeader().setSectionResizeMode(2, qt.QHeaderView.Stretch)

  def addLoadableRow(self, loadable, row, reader):
    self.insertRow(row)
    self.loadables[row] = loadable
    item = qt.QTableWidgetItem(loadable.name)
    self.setItem(row, 0, item)
    self.setCheckState(item, loadable)
    self.addReaderColumn(item, reader, row)
    self.addWarningColumn(item, loadable, row)

  def setCheckState(self, item, loadable):
    item.setCheckState(qt.Qt.Checked if loadable.selected else qt.Qt.Unchecked)
    item.setToolTip(loadable.tooltip)

  def addReaderColumn(self, item, reader, row):
    if not reader:
      return
    readerItem = qt.QTableWidgetItem(reader)
    readerItem.setFlags(readerItem.flags() ^ qt.Qt.ItemIsEditable)
    self.setItem(row, 1, readerItem)
    readerItem.setToolTip(item.toolTip())

  def addWarningColumn(self, item, loadable, row):
    warning = loadable.warning if loadable.warning else ''
    warnItem = qt.QTableWidgetItem(warning)
    warnItem.setFlags(warnItem.flags() ^ qt.Qt.ItemIsEditable)
    self.setItem(row, 2, warnItem)
    item.setToolTip(item.toolTip() + "\n" + warning)
    warnItem.setToolTip(item.toolTip())

  def setLoadables(self, loadablesByPlugin):
    """Load the table widget with a list
    of volume options (of class DICOMVolume)
    """
    self.clearContents()
    self.setRowCount(0)
    self.loadables = {}

    # For each plugin, keep only a single loadable selected for the same file set
    # to prevent loading same data multiple times.
    for plugin in loadablesByPlugin:
      for thisLoadableId in range(len(loadablesByPlugin[plugin])):
        for prevLoadableId in range(0, thisLoadableId):
          thisLoadable = loadablesByPlugin[plugin][thisLoadableId]
          prevLoadable = loadablesByPlugin[plugin][prevLoadableId]
          # fileDifferences will contain all the files that only present in one or the other list (or tuple)
          fileDifferences = set(thisLoadable.files).symmetric_difference(set(prevLoadable.files))
          if (not fileDifferences) and (prevLoadable.selected):
            thisLoadable.selected = False
            break

    row = 0
    for selectState in (True, False):
      for plugin in loadablesByPlugin:
        for loadable in loadablesByPlugin[plugin]:
          if loadable.selected == selectState:
            self.addLoadableRow(loadable, row, plugin.loadType)
            row += 1

    self.setVerticalHeaderLabels(row * [""])

  def uncheckAll(self):
    for row in range(self.rowCount):
      item = self.item(row, 0)
      item.setCheckState(False)

  def updateSelectedFromCheckstate(self):
    for row in range(self.rowCount):
      item = self.item(row, 0)
      self.loadables[row].selected = (item.checkState() != 0)
      # updating the names
      self.loadables[row].name = item.text()
