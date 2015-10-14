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

#ifndef __qMRMLTableView_h
#define __qMRMLTableView_h

// Qt includes
#include <QTableView>

// qMRML includes
#include "qSlicerTablesModuleWidgetsExport.h"

class QSortFilterProxyModel;
class qMRMLTableViewPrivate;
class qMRMLTableModel;
class vtkMRMLTableNode;
class vtkMRMLNode;

/// \brief Spreadsheet view for table nodes.
/// Allow view/edit of a vtkMRMLTableNode.
class Q_SLICER_MODULE_TABLES_WIDGETS_EXPORT qMRMLTableView : public QTableView
{
  Q_OBJECT
  //Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly)
public:
  qMRMLTableView(QWidget *parent=0);
  virtual ~qMRMLTableView();

  vtkMRMLTableNode* mrmlTableNode()const;
  qMRMLTableModel* tableModel()const;
  QSortFilterProxyModel* sortFilterProxyModel()const;

  //bool readOnly()const;

public slots:
  void setMRMLTableNode(vtkMRMLTableNode* tableNode);
  /// Utility function to simply connect signals/slots with Qt Designer
  void setMRMLTableNode(vtkMRMLNode* tableNode);

  //void setReadOnly(bool);

protected:
  QScopedPointer<qMRMLTableViewPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLTableView);
  Q_DISABLE_COPY(qMRMLTableView);
};

#endif
