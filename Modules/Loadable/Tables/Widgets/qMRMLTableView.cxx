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
#include <QHeaderView>
#include <QSortFilterProxyModel>

// qMRML includes
#include "qMRMLTableView.h"
#include "qMRMLTableModel.h"
#include "qMRMLItemDelegate.h"

// MRML includes
#include <vtkMRMLTableNode.h>

//------------------------------------------------------------------------------
class qMRMLTableViewPrivate
{
  Q_DECLARE_PUBLIC(qMRMLTableView);
protected:
  qMRMLTableView* const q_ptr;
public:
  qMRMLTableViewPrivate(qMRMLTableView& object);
  void init();
};

//------------------------------------------------------------------------------
qMRMLTableViewPrivate::qMRMLTableViewPrivate(qMRMLTableView& object)
  : q_ptr(&object)
{
}

//------------------------------------------------------------------------------
void qMRMLTableViewPrivate::init()
{
  Q_Q(qMRMLTableView);

  qMRMLTableModel* tableModel = new qMRMLTableModel(q);
  QSortFilterProxyModel* sortFilterModel = new QSortFilterProxyModel(q);
  //sortFilterModel->setFilterKeyColumn(qMRMLTableModel::LabelColumn);
  sortFilterModel->setSourceModel(tableModel);
  q->setModel(sortFilterModel);

  //q->setSelectionBehavior(QAbstractItemView::SelectRows);
  q->horizontalHeader()->setStretchLastSection(false);
  //q->horizontalHeader()->setResizeMode(qMRMLTableModel::ColorColumn, QHeaderView::ResizeToContents);
  //q->horizontalHeader()->setResizeMode(qMRMLTableModel::LabelColumn, QHeaderView::Stretch);
  //q->horizontalHeader()->setResizeMode(qMRMLTableModel::OpacityColumn, QHeaderView::ResizeToContents);

  q->setItemDelegate(new qMRMLItemDelegate(q));
}

//------------------------------------------------------------------------------
qMRMLTableView::qMRMLTableView(QWidget *_parent)
  : QTableView(_parent)
  , d_ptr(new qMRMLTableViewPrivate(*this))
{
  Q_D(qMRMLTableView);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLTableView::~qMRMLTableView()
{
}

//------------------------------------------------------------------------------
qMRMLTableModel* qMRMLTableView::tableModel()const
{
  return qobject_cast<qMRMLTableModel*>(this->sortFilterProxyModel()->sourceModel());
}

//------------------------------------------------------------------------------
QSortFilterProxyModel* qMRMLTableView::sortFilterProxyModel()const
{
  return qobject_cast<QSortFilterProxyModel*>(this->model());
}

//------------------------------------------------------------------------------
void qMRMLTableView::setMRMLTableNode(vtkMRMLNode* node)
{
  this->setMRMLTableNode(vtkMRMLTableNode::SafeDownCast(node));
}

//------------------------------------------------------------------------------
void qMRMLTableView::setMRMLTableNode(vtkMRMLTableNode* node)
{
  qMRMLTableModel* mrmlModel = this->tableModel();
  Q_ASSERT(mrmlModel);

  mrmlModel->setMRMLTableNode(node);
  this->sortFilterProxyModel()->invalidate();

  bool readOnly = (node==NULL) || (node->GetLocked());
  this->setEditTriggers( readOnly ? QAbstractItemView::NoEditTriggers : QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed );

  this->horizontalHeader()->setMinimumSectionSize(120);
  this->resizeColumnsToContents();

  //TODO:
  /*
  this->setSortingEnabled(sortable);
  if (mrmlModel)
    {
    mrmlModel->setSortRole(SortRole);
    }
  */

}

//------------------------------------------------------------------------------
vtkMRMLTableNode* qMRMLTableView::mrmlTableNode()const
{
  qMRMLTableModel* mrmlModel = this->tableModel();
  Q_ASSERT(mrmlModel);
  return mrmlModel->mrmlTableNode();
}

/*
//------------------------------------------------------------------------------
void qMRMLTableView::setReadOnly(bool readOnly)
{
  if (readOnly)
    {
    this->setEditTriggers( this->readOnly ? QAbstractItemView::NoEditTriggers : QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed );
    }
  else
    {
    this->sortFilterProxyModel()->setFilterRegExp(QRegExp());
    }
}

//------------------------------------------------------------------------------
bool qMRMLTableView::showOnlyNamedColors()const
{
  return this->sortFilterProxyModel()->filterRegExp().isEmpty();
}
*/
