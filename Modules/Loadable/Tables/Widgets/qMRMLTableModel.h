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

#ifndef __qMRMLTableModel_h
#define __qMRMLTableModel_h

// Qt includes
#include <QStandardItemModel>

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

// qMRML includes
#include "qSlicerTablesModuleWidgetsExport.h"

class vtkMRMLNode;
class vtkMRMLTableNode;
class QAction;

class qMRMLTableModelPrivate;

//------------------------------------------------------------------------------
class Q_SLICER_MODULE_TABLES_WIDGETS_EXPORT qMRMLTableModel : public QStandardItemModel
{
  Q_OBJECT
  QVTK_OBJECT
  Q_ENUMS(ItemDataRole)
  Q_PROPERTY(bool transposed READ transposed WRITE setTransposed)

public:
  typedef QAbstractItemModel Superclass;
  qMRMLTableModel(QObject *parent=0);
  virtual ~qMRMLTableModel();

  enum ItemDataRole{
    SortRole = Qt::UserRole + 1
  };

  void setMRMLTableNode(vtkMRMLTableNode* node);
  vtkMRMLTableNode* mrmlTableNode()const;

  /// Set/Get transposed flag
  /// If transposed is true then columns of the MRML table are added as rows in the model.
  void setTransposed(bool transposed);
  bool transposed()const;

  /// Return the VTK table cell associated to the node index.
  void updateMRMLFromModel(QStandardItem* item)const;

  /// Update the entire table from the MRML node
  void updateModelFromMRML();

protected slots:
  void onMRMLTableNodeModified(vtkObject* node);
  void onItemChanged(QStandardItem * item);

protected:

  qMRMLTableModel(qMRMLTableModelPrivate* pimpl, QObject *parent=0);

  static void onMRMLNodeEvent(vtkObject* vtk_obj, unsigned long event,
                              void* client_data, void* call_data);
protected:
  QScopedPointer<qMRMLTableModelPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLTableModel);
  Q_DISABLE_COPY(qMRMLTableModel);
};

#endif
