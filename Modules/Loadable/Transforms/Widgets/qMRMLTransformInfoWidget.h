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

#ifndef __qMRMLTransformInfoWidget_h
#define __qMRMLTransformInfoWidget_h

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

// SlicerQt includes
#include "qMRMLWidget.h"

#include "qSlicerTransformsModuleWidgetsExport.h"

class qMRMLTransformInfoWidgetPrivate;
class vtkMRMLTransformNode;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_Transforms
class Q_SLICER_MODULE_TRANSFORMS_WIDGETS_EXPORT
qMRMLTransformInfoWidget
  : public qMRMLWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qMRMLWidget Superclass;
  qMRMLTransformInfoWidget(QWidget *parent=0);
  virtual ~qMRMLTransformInfoWidget();

  vtkMRMLTransformNode* mrmlTransformNode()const;

public slots:

  /// Set the MRML node of interest
  /// Note that setting transformNode to 0 will disable the widget
  void setMRMLTransformNode(vtkMRMLTransformNode* transformNode);

  /// Utility function that calls setMRMLTransformNode(vtkMRMLTransformNode* transformNode)
  /// It's useful to connect to vtkMRMLNode* signals
  void setMRMLTransformNode(vtkMRMLNode* node);

protected slots:
  void updateWidgetFromMRML();

protected:
  QScopedPointer<qMRMLTransformInfoWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLTransformInfoWidget);
  Q_DISABLE_COPY(qMRMLTransformInfoWidget);
};

#endif
