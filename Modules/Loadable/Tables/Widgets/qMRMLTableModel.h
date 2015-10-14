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
  //Q_PROPERTY(bool noneEnabled READ noneEnabled WRITE setNoneEnabled)

public:
  typedef QAbstractItemModel Superclass;
  qMRMLTableModel(QObject *parent=0);
  virtual ~qMRMLTableModel();

  enum ItemDataRole{
    SortRole = Qt::UserRole + 1
  };

  /*
  /// The color column contains a Qt::DecorationRole with a pixmap of the color,
  /// the ColorRole with the color QColor, the colorName as Qt::TooltipRole and
  /// the colorName as Qt::DisplayRole only if LabelInColorColumn is true.
  enum Columns{
    ColorColumn = 0,
    LabelColumn = 1,
    OpacityColumn = 2
  };
  */

  void setMRMLTableNode(vtkMRMLTableNode* node);
  vtkMRMLTableNode* mrmlTableNode()const;

  /*
  /// Set/Get NoneEnabled flags
  /// An additional item is added into the menu list, where the user can select "None".
  void setNoneEnabled(bool enable);
  bool noneEnabled()const;
  */

  /*
  /// Control wether or not displaying the label in the color column
  void setLabelInColorColumn(bool enable);
  bool isLabelInColorColumn()const;

  /// Return the vtkMRMLNode associated to the node index.
  /// -1 if the node index is not a MRML node (i.e. vtkMRMLScene, extra item...)
  inline int colorFromIndex(const QModelIndex &nodeIndex)const;
  */
  /// Return the VTK table cell associated to the node index.
  void updateMRMLFromModel(QStandardItem* item)const;

  /// Update the entire table from the MRML node
  void updateModelFromMRML();
/*
  QStandardItem* itemFromColor(int color, int column = 0)const;
  QModelIndexList indexes(int color)const;

  inline QColor qcolorFromIndex(const QModelIndex& nodeIndex)const;
  inline QColor qcolorFromItem(QStandardItem* nodeItem)const;
  QColor qcolorFromColor(int color)const;

  /// Return the name of the color \a colorEntry
  QString nameFromColor(int colorEntry)const;

  /// Overload the header data method for the veritical header
  /// so that can return the color index rather than the row
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  */

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
