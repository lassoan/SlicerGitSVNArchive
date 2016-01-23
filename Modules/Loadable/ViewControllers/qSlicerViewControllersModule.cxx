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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QtPlugin>
#include <QSettings>

// SlicerQt includes

// Slices QTModule includes
#include "qSlicerViewControllersModule.h"
#include "qSlicerViewControllersModuleWidget.h"

#include "vtkSlicerViewControllersLogic.h"

#include <vtkMRMLSliceNode.h>
#include <vtkMRMLViewNode.h>

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerViewControllersModule, qSlicerViewControllersModule);

//-----------------------------------------------------------------------------
class qSlicerViewControllersModulePrivate
{
public:
};

//-----------------------------------------------------------------------------
qSlicerViewControllersModule::qSlicerViewControllersModule(QObject* _parent)
  :Superclass(_parent)
  , d_ptr(new qSlicerViewControllersModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerViewControllersModule::~qSlicerViewControllersModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerViewControllersModule::acknowledgementText()const
{
  return "This module was developed by Jean-Christophe Fillion-Robin, Kitware Inc. "
         "This work was supported by NIH grant 3P41RR013218-12S1, "
         "NA-MIC, NAC and Slicer community.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerViewControllersModule::categories() const
{
  return QStringList() << "";
}

//-----------------------------------------------------------------------------
QIcon qSlicerViewControllersModule::icon() const
{
  return QIcon(":Icons/ViewControllers.png");
}

//-----------------------------------------------------------------------------
void qSlicerViewControllersModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
void qSlicerViewControllersModule::setMRMLScene(vtkMRMLScene* scene)
{
  this->Superclass::setMRMLScene(scene);
  vtkSlicerViewControllersLogic* logic = vtkSlicerViewControllersLogic::SafeDownCast(this->logic());
  if (!logic)
    {
    qCritical() << Q_FUNC_INFO << " failed: logic is invalid";
    return;
    }
  // Update default view nodes from settints
  this->readDefaultOrientationMarkerSettings(logic->GetDefaultSliceViewNode(), "DefaultSliceView");
  this->readDefaultOrientationMarkerSettings(logic->GetDefaultThreeDViewNode(), "Default3DView");
  this->writeDefaultOrientationMarkerSettings(logic->GetDefaultSliceViewNode(), "DefaultSliceView");
  this->writeDefaultOrientationMarkerSettings(logic->GetDefaultThreeDViewNode(), "Default3DView");
  // Update all existing view nodes to default
  logic->ResetAllViewNodesToDefault();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerViewControllersModule::createWidgetRepresentation()
{
  return new qSlicerViewControllersModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerViewControllersModule::createLogic()
{
  return vtkSlicerViewControllersLogic::New();
}

//-----------------------------------------------------------------------------
void qSlicerViewControllersModule::readDefaultOrientationMarkerSettings(vtkMRMLAbstractViewNode* defaultViewNode, QString groupName)
{
  if (!defaultViewNode)
    {
    qCritical() << Q_FUNC_INFO << " failed: defaultViewNode is invalid";
    return;
    }
  QSettings settings;
  settings.beginGroup(groupName);
  if (settings.contains("OrientationMarkerVisibility"))
    {
    defaultViewNode->SetOrientationMarkerVisibility(settings.value("OrientationMarkerVisibility").toBool());
    }
  if (settings.contains("OrientationMarkerRepresentation"))
    {
    defaultViewNode->SetOrientationMarkerRepresentation(
      vtkMRMLAbstractViewNode::GetOrientationMarkerRepresentationFromString(
      settings.value("OrientationMarkerRepresentation").toString().toLatin1()));
    }
  if (settings.contains("OrientationMarkerSize"))
    {
    defaultViewNode->SetOrientationMarkerSize(settings.value("OrientationMarkerSize").toInt());
    }

  /*
  QString orientationMarkerHumanModelFile;
  if (settings.contains("OrientationMarkerHumanModelFile"))
    {
    orientationMarkerHumanModelFile = settings.value("OrientationMarkerHumanModelFile").toString();
    QFileInfo checkFile(orientationMarkerHumanModelFile);
    // check if file exists and if yes: Is it really a file and no directory?
    if (!checkFile.exists() || !checkFile.isFile())
      {
      orientationMarkerHumanModelFile.clear();
      }
    }
  if (orientationMarkerHumanModelFile.isEmpty())
    {

    }
  vtkNew<vtkXMLPolyDataReader> polyDataReader;
  polyDataReader->SetFileName(modelFilename);
  polyDataReader->Update();
  polyDataReader->GetOutput()->GetPointData()->SetActiveScalars("Color");
  */
}

//-----------------------------------------------------------------------------
void qSlicerViewControllersModule::writeDefaultOrientationMarkerSettings(vtkMRMLAbstractViewNode* defaultViewNode, QString groupName)
{
  QSettings settings;
  settings.beginGroup(groupName);
  settings.setValue("OrientationMarkerVisibility", defaultViewNode->GetOrientationMarkerVisibility());
  settings.setValue("OrientationMarkerRepresentation",
    vtkMRMLAbstractViewNode::GetOrientationMarkerRepresentationAsString(
    defaultViewNode->GetOrientationMarkerRepresentation()));
  settings.setValue("OrientationMarkerSize", defaultViewNode->GetOrientationMarkerSize());
}

//-----------------------------------------------------------------------------
QStringList qSlicerViewControllersModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Wendy Plesniak (SPL, BWH)");
  moduleContributors << QString("Jim Miller (GE)");
  moduleContributors << QString("Steve Pieper (Isomics)");
  moduleContributors << QString("Ron Kikinis (SPL, BWH)");
  moduleContributors << QString("Jean-Christophe Fillion-Robin (Kitware)");
  return moduleContributors;
}
