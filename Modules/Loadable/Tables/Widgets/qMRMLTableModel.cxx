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
};

//------------------------------------------------------------------------------
qMRMLTableModelPrivate::qMRMLTableModelPrivate(qMRMLTableModel& object)
  : q_ptr(&object)
{
  this->CallBack = vtkSmartPointer<vtkCallbackCommand>::New();
  //this->NoneEnabled = false;
  //this->LabelInColor = false;
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
  q->setColumnCount(1);
  /*
  if (this->LabelInColor)
    {
    q->setHorizontalHeaderLabels(QStringList() << "Color" << "Label" << "Opacity");
    }
  else
    {
    q->setHorizontalHeaderLabels(QStringList() << "" << "Label" << "Opacity");
    }
  */
  QObject::connect(q, SIGNAL(itemChanged(QStandardItem*)),
                   q, SLOT(onItemChanged(QStandardItem*)),
                   Qt::UniqueConnection);
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

  QObject::disconnect(this, SIGNAL(itemChanged(QStandardItem*)),
                    this, SLOT(onItemChanged(QStandardItem*)));

  vtkMRMLTableNode* tableNode = vtkMRMLTableNode::SafeDownCast(d->MRMLTableNode);
  if (tableNode==NULL)
    {
    this->setRowCount(0);
    return;
    }
  vtkTable* table = tableNode->GetTable();
  if (table==NULL)
    {
    this->setRowCount(0);
    return;
    }

  //TODO: bool sortable = tableNode->GetSortable();
  bool sortable = false;

  vtkIdType numberOfRows = table->GetNumberOfRows();
  vtkIdType numberOfColumns = table->GetNumberOfColumns();
  vtkIdType colOffset = sortable ? 0 : 1;

  setRowCount(static_cast<int>(numberOfRows));
  setColumnCount(static_cast<int>(numberOfColumns - colOffset));
  for (vtkIdType dataCol = colOffset; dataCol < numberOfColumns; ++dataCol)
    {
    int modelCol = static_cast<int>(dataCol - colOffset);

    QString columnName(table->GetColumnName(dataCol));
    // unkown is used to encode columns with no name
    if (columnName == "unknown")
    {
      columnName = "";
    }
    setHeaderData(modelCol, Qt::Horizontal, columnName);

    for (vtkIdType r = 0; r < numberOfRows; ++r)
      {
      QStandardItem* item = new QStandardItem();
      vtkVariant variant = table->GetValue(r, dataCol);
      item->setText(QString(variant.ToString()));
      if (sortable)
        {
        item->setData(variant.ToDouble(), SortRole);
        }
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable); // Item is view-only
      setItem(static_cast<int>(r), modelCol, item);
      }
    }
  for (vtkIdType r = 0; r < numberOfRows; ++r)
    {
    if(sortable)
      {
      QStandardItem * item = this->item(r, 0);
      item->setBackground(QPalette().color(QPalette::Window));
      item->setData(r, SortRole);
      setHeaderData(static_cast<int>(r), Qt::Vertical, QString());
      }
    else
      {
      setHeaderData(static_cast<int>(r), Qt::Vertical, QString(table->GetValue(r, 0).ToString()));
      }
    }

  QObject::connect(this, SIGNAL(itemChanged(QStandardItem*)),
               this, SLOT(onItemChanged(QStandardItem*)),
               Qt::UniqueConnection);

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

  vtkVariant itemText(item->text().toLatin1().constData()); // the vtkVariant constructor makes a copy of the input buffer
  table->SetValue(item->row(), item->column(), itemText);
  // TODO: if sortable then index may nneed to be shifted and
  //  item->setData(variant.ToDouble(), SortRole);
  // is probably needed

  /*
  switch(item->column())
    {
    case qMRMLTableModel::ColorColumn:
      {
      QColor rgba(item->data(qMRMLTableModel::ColorRole).value<QColor>());
      tableNode->SetColor(color, rgba.redF(), rgba.greenF(), rgba.blueF(), rgba.alphaF());
      break;
      }
    case qMRMLTableModel::LabelColumn:
      tableNode->SetColorName(color, item->text().toLatin1());
      break;
    case qMRMLTableModel::OpacityColumn:
      tableNode->SetOpacity(color, item->data(Qt::DisplayRole).toDouble());
      break;
    default:
      break;
    }
  */
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
