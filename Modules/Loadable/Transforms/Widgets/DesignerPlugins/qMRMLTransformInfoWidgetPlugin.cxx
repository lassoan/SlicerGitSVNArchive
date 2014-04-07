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

#include "qMRMLTransformInfoWidgetPlugin.h"
#include "qMRMLTransformInfoWidget.h"

//------------------------------------------------------------------------------
qMRMLTransformInfoWidgetPlugin::qMRMLTransformInfoWidgetPlugin(QObject *_parent)
  : QObject(_parent)
{
}

//------------------------------------------------------------------------------
QWidget *qMRMLTransformInfoWidgetPlugin::createWidget(QWidget *_parent)
{
  qMRMLTransformInfoWidget* _widget = new qMRMLTransformInfoWidget(_parent);
  return _widget;
}

//------------------------------------------------------------------------------
QString qMRMLTransformInfoWidgetPlugin::domXml() const
{
  return "<widget class=\"qMRMLTransformInfoWidget\" \
          name=\"MRMLTransformInfoWidget\">\n"
          "</widget>\n";
}

//------------------------------------------------------------------------------
QString qMRMLTransformInfoWidgetPlugin::includeFile() const
{
  return "qMRMLTransformInfoWidget.h";
}

//------------------------------------------------------------------------------
bool qMRMLTransformInfoWidgetPlugin::isContainer() const
{
  return false;
}

//------------------------------------------------------------------------------
QString qMRMLTransformInfoWidgetPlugin::name() const
{
  return "qMRMLTransformInfoWidget";
}
