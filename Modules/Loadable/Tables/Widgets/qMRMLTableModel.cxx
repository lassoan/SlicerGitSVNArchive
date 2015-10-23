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

// Qt includes
#include <QApplication>
#include <QPalette>

// qMRML includes
#include "qMRMLUtils.h"
#include "qMRMLTableModel.h"

// MRML includes
#include <vtkMRMLTableNode.h>

// VTK includes
#include <vtkCallbackCommand.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

//------------------------------------------------------------------------------
// qMRMLTableModelPrivate
//------------------------------------------------------------------------------
class qMRMLTableModelPrivate
{
  Q_DECLARE_PUBLIC(qMRMLTableModel);
protected:
  qMRMLTableModel* const q_ptr;
public:
  qMRMLTableModelPrivate(qMRMLTableModel& object);
  virtual ~qMRMLTableModelPrivate();
  void init();

  vtkSmartPointer<vtkCallbackCommand> CallBack;
  vtkSmartPointer<vtkMRMLTableNode>   MRMLTableNode;
  bool Transposed;
};

//------------------------------------------------------------------------------
qMRMLTableModelPrivate::qMRMLTableModelPrivate(qMRMLTableModel& object)
  : q_ptr(&object)
{
  this->CallBack = vtkSmartPointer<vtkCallbackCommand>::New();
  this->Transposed = false;
}

//------------------------------------------------------------------------------
qMRMLTableModelPrivate::~qMRMLTableModelPrivate()
{
  if (this->MRMLTableNode)
    {
    this->MRMLTableNode->RemoveObserver(this->CallBack);
    }
}

//------------------------------------------------------------------------------
void qMRMLTableModelPrivate::init()
{
  Q_Q(qMRMLTableModel);
  this->CallBack->SetClientData(q);
  this->CallBack->SetCallback(qMRMLTableModel::onMRMLNodeEvent);
  q->setColumnCount(0);
  QObject::connect(q, SIGNAL(itemChanged(QStandardItem*)), q, SLOT(onItemChanged(QStandardItem*)), Qt::UniqueConnection);
}

//------------------------------------------------------------------------------
// qMRMLTableModel
//------------------------------------------------------------------------------
qMRMLTableModel::qMRMLTableModel(QObject *_parent)
  : QStandardItemModel(_parent)
  , d_ptr(new qMRMLTableModelPrivate(*this))
{
  Q_D(qMRMLTableModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLTableModel::qMRMLTableModel(qMRMLTableModelPrivate* pimpl, QObject *parentObject)
  : QStandardItemModel(parentObject)
  , d_ptr(pimpl)
{
  Q_D(qMRMLTableModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLTableModel::~qMRMLTableModel()
{
}

//------------------------------------------------------------------------------
void qMRMLTableModel::setMRMLTableNode(vtkMRMLTableNode* tableNode)
{
  Q_D(qMRMLTableModel);
  if (d->MRMLTableNode)
    {
    d->MRMLTableNode->RemoveObserver(d->CallBack);
    }
  if (tableNode)
    {
    tableNode->AddObserver(vtkCommand::ModifiedEvent, d->CallBack);
    }
  d->MRMLTableNode = tableNode;
  this->updateModelFromMRML();
}

//------------------------------------------------------------------------------
vtkMRMLTableNode* qMRMLTableModel::mrmlTableNode()const
{
  Q_D(const qMRMLTableModel);
  return d->MRMLTableNode;
}

// --------------------------------------------------------------------------
void qMRMLTableModel::updateModelFromMRML()
{
  Q_D(qMRMLTableModel);

  QObject::disconnect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(onItemChanged(QStandardItem*)));

  vtkMRMLTableNode* tableNode = vtkMRMLTableNode::SafeDownCast(d->MRMLTableNode);
  vtkTable* table = (tableNode ? tableNode->GetTable() : NULL);
  if (table==NULL || table->GetNumberOfColumns()==0)
    {
    // setRowCount and setColumnCount to 0 would not be enough, it's necesary to remove the header as well
    this->reset();
    QObject::connect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(onItemChanged(QStandardItem*)), Qt::UniqueConnection);
    return;
    }

  bool locked = tableNode->GetLocked();
  bool sortable = tableNode->GetSortable();
  bool labelInFirstTableColumn = tableNode->GetLabelInFirstColumn();

  vtkIdType numberOfTableColumns = table->GetNumberOfColumns();
  vtkIdType numberOfTableRows = table->GetNumberOfRows();
  vtkIdType tableColOffset = labelInFirstTableColumn ? 1 : 0;
  if (d->Transposed)
    {
    setRowCount(static_cast<int>(numberOfTableColumns-tableColOffset));
    setColumnCount(static_cast<int>(numberOfTableRows));
    }
  else
    {
    setRowCount(static_cast<int>(numberOfTableRows));
    setColumnCount(static_cast<int>(numberOfTableColumns-tableColOffset));
    }

  for (vtkIdType tableCol = tableColOffset; tableCol < numberOfTableColumns; ++tableCol)
    {
    int modelCol = static_cast<int>(tableCol - tableColOffset);

    QString columnName(table->GetColumnName(tableCol));
    // unkown is used to encode columns with no name
    if (columnName == "unknown")
      {
      columnName = "";
      }
    setHeaderData(modelCol, d->Transposed ? Qt::Vertical : Qt::Horizontal, columnName);

    for (vtkIdType tableRow = 0; tableRow < numberOfTableRows; ++tableRow)
      {
      QStandardItem* item = new QStandardItem();
      vtkVariant variant = table->GetValue(tableRow, tableCol);
      item->setText(QString(variant.ToString()));
      if (sortable)
        {
        if (variant.IsNumeric())
          {
          item->setData(variant.ToDouble(), SortRole);
          }
        else
          {
          item->setData(variant.ToString().c_str(), SortRole);
          }
        }
      if (locked)
        {
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable); // Item is view-only
        }
      if (d->Transposed)
        {
        setItem(modelCol, static_cast<int>(tableRow), item);
        }
      else
        {
        setItem(static_cast<int>(tableRow), modelCol, item);
        }
      }
    }
  if(labelInFirstTableColumn)
    {
    for (vtkIdType tableRow = 0; tableRow < numberOfTableRows; ++tableRow)
      {
      setHeaderData(static_cast<int>(tableRow), d->Transposed ? Qt::Horizontal : Qt::Vertical, QString(table->GetValue(tableRow, 0).ToString()));
      }
    }

  QObject::connect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(onItemChanged(QStandardItem*)), Qt::UniqueConnection);
}

//------------------------------------------------------------------------------
void qMRMLTableModel::updateMRMLFromModel(QStandardItem* item)const
{
  Q_D(const qMRMLTableModel);
  if (item == NULL)
    {
    qCritical("qMRMLTableModel::updateMRMLFromModel failed: item is invalid");
    return;
    }
  vtkMRMLTableNode* tableNode = vtkMRMLTableNode::SafeDownCast(d->MRMLTableNode);
  if (tableNode==NULL)
    {
    qCritical("qMRMLTableModel::updateMRMLFromModel failed: tableNode is invalid");
    return;
    }
  vtkTable* table = tableNode->GetTable();
  if (table==NULL)
    {
    qCritical("qMRMLTableModel::updateMRMLFromModel failed: table is invalid");
    return;
    }

  vtkVariant itemText(item->text().toLatin1().constData()); // the vtkVariant constructor makes a copy of the input buffer, so using constData is safe
  int tableRow=0;
  int tableCol=0;
  if (d->Transposed)
    {
    tableRow = item->column();
    tableCol = tableNode->GetLabelInFirstColumn() ? item->row()+1 : item->row();
    }
  else
    {
    tableRow = item->row();
    tableCol = tableNode->GetLabelInFirstColumn() ? item->column()+1 : item->column();
    }

  vtkVariant valueInTableBefore = table->GetValue(tableRow, tableCol);
  table->SetValue(tableRow, tableCol, itemText);
  vtkVariant valueInTableAfter = table->GetValue(tableRow, tableCol);
  if (valueInTableBefore == valueInTableAfter)
    {
    // the value is not changed, this means that the table cannot store this value - revert the value in the table
    QObject::disconnect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(onItemChanged(QStandardItem*)));
    item->setText(QString(valueInTableBefore.ToString()));
    QObject::connect(this, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(onItemChanged(QStandardItem*)), Qt::UniqueConnection);
    }
  else
    {
    tableNode->Modified();
    }
}

//-----------------------------------------------------------------------------
void qMRMLTableModel::onMRMLNodeEvent(vtkObject* vtk_obj, unsigned long event,
                                      void* client_data, void* vtkNotUsed(call_data))
{
  vtkMRMLTableNode* tableNode = reinterpret_cast<vtkMRMLTableNode*>(vtk_obj);
  qMRMLTableModel* tableModel = reinterpret_cast<qMRMLTableModel*>(client_data);
  Q_ASSERT(tableNode);
  Q_ASSERT(tableModel);
  switch(event)
    {
    default:
    case vtkCommand::ModifiedEvent:
      tableModel->onMRMLTableNodeModified(tableNode);
      break;
    }
}

//------------------------------------------------------------------------------
void qMRMLTableModel::onMRMLTableNodeModified(vtkObject* node)
{
  Q_D(qMRMLTableModel);
  vtkMRMLTableNode* tableNode = vtkMRMLTableNode::SafeDownCast(node);
  Q_UNUSED(tableNode);
  Q_UNUSED(d);
  Q_ASSERT(tableNode == d->MRMLTableNode);
  this->updateModelFromMRML();
}

//------------------------------------------------------------------------------
void qMRMLTableModel::onItemChanged(QStandardItem * item)
{
  if (item == this->invisibleRootItem())
    {
    return;
    }
  this->updateMRMLFromModel(item);
}

//------------------------------------------------------------------------------
void qMRMLTableModel::setTransposed(bool transposed)
{
  Q_D(qMRMLTableModel);
  if (d->Transposed == transposed)
    {
    return;
    }
  d->Transposed = transposed;
  this->updateModelFromMRML();
}

//------------------------------------------------------------------------------
bool qMRMLTableModel::transposed()const
{
  Q_D(const qMRMLTableModel);
  return d->Transposed;
}
