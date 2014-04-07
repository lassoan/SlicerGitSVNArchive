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

// QT includes
#include <QApplication>
#include <QTimer>

// qMRML includes
#include "qMRMLTransformInfoWidget.h"

// MRML includes
#include <vtkMRMLTransformNode.h>

// VTK includes
#include <vtkSmartPointer.h>

// STD includes

int qMRMLTransformInfoWidgetTest1(int argc, char * argv [] )
{
  QApplication app(argc, argv);

  vtkSmartPointer< vtkMRMLTransformNode > transformNode = vtkSmartPointer< vtkMRMLTransformNode >::New();

  qMRMLTransformInfoWidget transformInfo;
  transformInfo.setMRMLTransformNode(transformNode);
  transformInfo.show();

  if (argc < 2 || QString(argv[1]) != "-I" )
    {
    QTimer::singleShot(200, &app, SLOT(quit()));
    }
  return app.exec();
}
