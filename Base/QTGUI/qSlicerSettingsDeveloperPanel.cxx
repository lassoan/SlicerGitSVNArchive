/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <QUrl>

// QtGUI includes
#include "qSlicerApplication.h"
#include "qSlicerSettingsDeveloperPanel.h"
#include "ui_qSlicerSettingsDeveloperPanel.h"

// --------------------------------------------------------------------------
// qSlicerSettingsDeveloperPanelPrivate

//-----------------------------------------------------------------------------
class qSlicerSettingsDeveloperPanelPrivate: public Ui_qSlicerSettingsDeveloperPanel
{
  Q_DECLARE_PUBLIC(qSlicerSettingsDeveloperPanel);
protected:
  qSlicerSettingsDeveloperPanel* const q_ptr;

public:
  qSlicerSettingsDeveloperPanelPrivate(qSlicerSettingsDeveloperPanel& object);
  void init();
};

// --------------------------------------------------------------------------
// qSlicerSettingsDeveloperPanelPrivate methods

// --------------------------------------------------------------------------
qSlicerSettingsDeveloperPanelPrivate
::qSlicerSettingsDeveloperPanelPrivate(qSlicerSettingsDeveloperPanel& object)
  :q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerSettingsDeveloperPanelPrivate::init()
{
  Q_Q(qSlicerSettingsDeveloperPanel);

  this->setupUi(q);

  // Default values
  this->DeveloperModeEnabledCheckBox->setChecked(false);
  this->QtTestingEnabledCheckBox->setChecked(false);
#ifndef Slicer_USE_QtTesting
  this->QtTestingEnabledCheckBox->hide();
  this->QtTestingEnabledLabel->hide();
#endif

  QStringList logFilePaths = qSlicerApplication::application()->recentLogFiles();
  this->RecentLogFilesComboBox->addItems(logFilePaths);

  // Register settings
  q->registerProperty("Developer/DeveloperMode", this->DeveloperModeEnabledCheckBox,
                      "checked", SIGNAL(toggled(bool)),
                      "Enable/Disable developer mode", ctkSettingsPanel::OptionRequireRestart);

  // Register settings
  q->registerProperty("QtTesting/Enabled", this->QtTestingEnabledCheckBox,
                      "checked", SIGNAL(toggled(bool)),
                      "Enable/Disable QtTesting", ctkSettingsPanel::OptionRequireRestart);

  // Actions to propagate to the application when settings are changed
  QObject::connect(this->DeveloperModeEnabledCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(enableDeveloperMode(bool)));
  QObject::connect(this->QtTestingEnabledCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(enableQtTesting(bool)));

  QObject::connect(this->RecentLogFilesCopyPushButton, SIGNAL(clicked()),
                   q, SLOT(onRecentLogFilePathCopy()));

}

// --------------------------------------------------------------------------
// qSlicerSettingsDeveloperPanel methods

// --------------------------------------------------------------------------
qSlicerSettingsDeveloperPanel::qSlicerSettingsDeveloperPanel(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSettingsDeveloperPanelPrivate(*this))
{
  Q_D(qSlicerSettingsDeveloperPanel);
  d->init();
}

// --------------------------------------------------------------------------
qSlicerSettingsDeveloperPanel::~qSlicerSettingsDeveloperPanel()
{
}

// --------------------------------------------------------------------------
void qSlicerSettingsDeveloperPanel::enableDeveloperMode(bool value)
{
  Q_UNUSED(value);
}

// --------------------------------------------------------------------------
void qSlicerSettingsDeveloperPanel::enableQtTesting(bool value)
{
  Q_UNUSED(value);
}

// --------------------------------------------------------------------------
void qSlicerSettingsDeveloperPanel::onRecentLogFilePathCopy()
{
  Q_D(qSlicerSettingsDeveloperPanel);

  // Alternative actions that we could do here:
  // - Copy filename to clipboard:
  //    QApplication::clipboard()->setText(d->RecentLogFilesComboBox->currentText());
  // - Open file in editor: (a problem is that on Windows .log files are opened in Notepad and in the log file the
  //   end of line is not CR/LF and so the log is displayed as one very long line)
  //    QDesktopServices::openUrl(QUrl("file:///"+d->RecentLogFilesComboBox->currentText(), QUrl::TolerantMode));

  QFile f(d->RecentLogFilesComboBox->currentText());
  if (f.open(QFile::ReadOnly | QFile::Text))
  {
    QTextStream in(&f);
    QString logText = in.readAll();
    QApplication::clipboard()->setText(logText);
  }
}
