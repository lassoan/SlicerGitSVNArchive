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
#include <QKeyEvent>
#include <QSortFilterProxyModel>

// qMRML includes
#include "qMRMLTableView.h"
#include "qMRMLTableModel.h"

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
  sortFilterModel->setSourceModel(tableModel);
  q->setModel(sortFilterModel);

  q->horizontalHeader()->setStretchLastSection(false);
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

  this->horizontalHeader()->setMinimumSectionSize(120);
  this->resizeColumnsToContents();
}

//------------------------------------------------------------------------------
vtkMRMLTableNode* qMRMLTableView::mrmlTableNode()const
{
  qMRMLTableModel* mrmlModel = this->tableModel();
  Q_ASSERT(mrmlModel);
  return mrmlModel->mrmlTableNode();
}

//------------------------------------------------------------------------------
void qMRMLTableView::setTransposed(bool transposed)
{
  qMRMLTableModel* tableModel = this->tableModel();
  if (tableModel == NULL)
    {
    qCritical("qMRMLTableView::setTransposed failed: tableModel is invalid");
    return;
    }
  tableModel->setTransposed(transposed);
}

//------------------------------------------------------------------------------
bool qMRMLTableView::transposed()const
{
  qMRMLTableModel* tableModel = this->tableModel();
  if (tableModel == NULL)
    {
    qCritical("qMRMLTableView::transposed failed: tableModel is invalid");
    return false;
    }
  return tableModel->transposed();
}

//------------------------------------------------------------------------------
void qMRMLTableView::keyPressEvent(QKeyEvent *event)
{
  if (model())
    {
    // Prevent giving the focus to the previous/next widget if arrow keys are used
    // at the edge of the table (without this: if the current cell is in the top
    // row and user press the Up key, the focus goes from the table to the previous
    // widget in the tab order)
    if (event->key() == Qt::Key_Left && currentIndex().column() == 0
      || event->key() == Qt::Key_Up && currentIndex().row() == 0
      || event->key() == Qt::Key_Right && currentIndex().column() == model()->columnCount()-1
      || event->key() == Qt::Key_Down && currentIndex().row() == model()->rowCount()-1)
      {
      return;
      }
    }
  QTableView::keyPressEvent(event);
}
