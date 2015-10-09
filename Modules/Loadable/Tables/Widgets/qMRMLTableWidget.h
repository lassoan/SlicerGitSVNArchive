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

#ifndef __qMRMLTableWidget_h
#define __qMRMLTableWidget_h

// Qt includes
#include "qSlicerWidget.h"

// VTK includes
#include <vtkTable.h>

#include "vtkSlicerModuleLogic.h"
#include "vtkMRMLTableNode.h"
#include "vtkSlicerTablesLogic.h"

// FooBar Widgets includes
#include "qSlicerTablesModuleWidgetsExport.h"
#include "ui_qMRMLTableWidget.h"

class qMRMLTableWidgetPrivate;

/// \ingroup Slicer_QtModules_CreateModels
class Q_SLICER_MODULE_TABLES_WIDGETS_EXPORT
qMRMLTableWidget : public qSlicerWidget
{
  Q_OBJECT
public:
  typedef qSlicerWidget Superclass;
  qMRMLTableWidget(QWidget *parent=0);
  virtual ~qMRMLTableWidget();

  virtual void setMetricsTableNode( vtkMRMLNode* newMetricsTableNode );
  virtual vtkMRMLTableNode* getMetricsTableNode();
  virtual vtkMRMLTableNode* addMetricsTableNode();

protected slots:

  virtual void onMetricsTableNodeChanged( vtkMRMLNode* newMetricsTableNode );
  void onMetricsTableNodeModified();

  void onClipboardButtonClicked();

  void updateWidget();

signals:

  void metricsTableNodeChanged( vtkMRMLNode* newMetricsTableNode );
  void metricsTableNodeModified();

protected:

  QScopedPointer<qMRMLTableWidgetPrivate> d_ptr;

  vtkMRMLTableNode* MetricsTableNode;
  vtkSlicerTablesLogic* PerkEvaluatorLogic;

  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qMRMLTableWidget);
  Q_DISABLE_COPY(qMRMLTableWidget);

};

#endif
