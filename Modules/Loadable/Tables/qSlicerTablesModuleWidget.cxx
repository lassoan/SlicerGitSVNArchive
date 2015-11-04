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

// Qt includes
#include <QAction>
#include <QFileDialog>
#include <QApplication>
#include <QClipboard>
#include <QStringBuilder>

// C++ includes
#include <cmath>

// SlicerQt includes
#include "qSlicerTablesModuleWidget.h"
#include "ui_qSlicerTablesModuleWidget.h"

// vtkSlicerLogic includes
#include "vtkSlicerTablesLogic.h"

// MRMLWidgets includes
#include <qMRMLUtils.h>
#include <qMRMLTableModel.h>

// MRML includes
#include "vtkMRMLTableNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

//-----------------------------------------------------------------------------
class qSlicerTablesModuleWidgetPrivate: public Ui_qSlicerTablesModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerTablesModuleWidget);
protected:
  qSlicerTablesModuleWidget* const q_ptr;
public:
  qSlicerTablesModuleWidgetPrivate(qSlicerTablesModuleWidget& object);
//  static QList<vtkSmartPointer<vtkMRMLTransformableNode> > getSelectedNodes(qMRMLTreeView* tree);

  vtkSlicerTablesLogic*      logic()const;
  vtkTable* table()const;

  vtkWeakPointer<vtkMRMLTableNode> MRMLTableNode;
  QAction*                      CopyAction;
  QAction*                      PasteAction;
};

//-----------------------------------------------------------------------------
qSlicerTablesModuleWidgetPrivate::qSlicerTablesModuleWidgetPrivate(qSlicerTablesModuleWidget& object)
  : q_ptr(&object)
{
  this->MRMLTableNode = 0;
  this->CopyAction = 0;
  this->PasteAction = 0;
}
//-----------------------------------------------------------------------------
vtkSlicerTablesLogic* qSlicerTablesModuleWidgetPrivate::logic()const
{
  Q_Q(const qSlicerTablesModuleWidget);
  return vtkSlicerTablesLogic::SafeDownCast(q->logic());
}

//-----------------------------------------------------------------------------
vtkTable* qSlicerTablesModuleWidgetPrivate::table()const
{
  Q_Q(const qSlicerTablesModuleWidget);
  if (this->MRMLTableNode.GetPointer()==NULL)
    {
    return NULL;
    }
  return this->MRMLTableNode->GetTable();
}

//-----------------------------------------------------------------------------
/*
QList<vtkSmartPointer<vtkMRMLTransformableNode> > qSlicerTablesModuleWidgetPrivate::getSelectedNodes(qMRMLTreeView* tree)
{
  QModelIndexList selectedIndexes =
    tree->selectionModel()->selectedRows();
  selectedIndexes = qMRMLTreeView::removeChildren(selectedIndexes);

  // Return the list of nodes
  QList<vtkSmartPointer<vtkMRMLTransformableNode> > selectedNodes;
  foreach(QModelIndex selectedIndex, selectedIndexes)
    {
    vtkMRMLTransformableNode* node = vtkMRMLTransformableNode::SafeDownCast(
      tree->sortFilterProxyModel()->
      mrmlNodeFromIndex( selectedIndex ));
    Q_ASSERT(node);
    selectedNodes << node;
    }
  return selectedNodes;
}
*/
//-----------------------------------------------------------------------------
qSlicerTablesModuleWidget::qSlicerTablesModuleWidget(QWidget* _parentWidget)
  : Superclass(_parentWidget)
  , d_ptr(new qSlicerTablesModuleWidgetPrivate(*this))
{
}

//-----------------------------------------------------------------------------
qSlicerTablesModuleWidget::~qSlicerTablesModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::setup()
{
  Q_D(qSlicerTablesModuleWidget);
  d->setupUi(this);

  // Add coordinate reference button to a button group
  d->CopyAction = new QAction(this);
  d->CopyAction->setIcon(QIcon(":Icons/Medium/SlicerEditCopy.png"));
  d->CopyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  d->CopyAction->setShortcuts(QKeySequence::Copy);
  d->CopyAction->setToolTip(tr("Copy"));
  this->addAction(d->CopyAction);
  d->PasteAction = new QAction(this);
  d->PasteAction->setIcon(QIcon(":Icons/Medium/SlicerEditPaste.png"));
  d->PasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  d->PasteAction->setShortcuts(QKeySequence::Paste);
  d->PasteAction->setToolTip(tr("Paste"));
  this->addAction(d->PasteAction);

  // Connect node selector with module itself
  this->connect(d->TableNodeSelector,
                SIGNAL(currentNodeChanged(vtkMRMLNode*)),
                SLOT(onNodeSelected(vtkMRMLNode*)));

  this->connect(d->ColumnInsertButton, SIGNAL(clicked()), SLOT(insertColumn()));
  this->connect(d->ColumnDeleteButton, SIGNAL(clicked()), SLOT(deleteColumn()));
  this->connect(d->RowInsertButton, SIGNAL(clicked()), SLOT(insertRow()));
  this->connect(d->RowDeleteButton, SIGNAL(clicked()), SLOT(deleteRow()));
  this->connect(d->LabelInFirstColumn, SIGNAL(clicked()), SLOT(toggleLabelInFirstColumn()));




  // Connect copy and paste actions
  d->CopyButton->setDefaultAction(d->CopyAction);
  this->connect(d->CopyAction,
                SIGNAL(triggered()),
                SLOT(copySelection()));

  d->PasteButton->setDefaultAction(d->PasteAction);
  this->connect(d->PasteAction,
                SIGNAL(triggered()),
                SLOT(pasteSelection()));

  this->onNodeSelected(0);
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::onNodeSelected(vtkMRMLNode* node)
{
  Q_D(qSlicerTablesModuleWidget);

  vtkMRMLTableNode* tableNode = vtkMRMLTableNode::SafeDownCast(node);

  // Enable/Disable CoordinateReference, identity, split buttons, MatrixViewGroupBox, and
  // Min/Max translation inputs

  /*
  d->InvertPushButton->setEnabled(tableNode != 0);

  d->TranslateFirstToolButton->setEnabled(isLinearTransform);
  d->IdentityPushButton->setEnabled(isLinearTransform);
  d->MatrixViewGroupBox->setEnabled(isLinearTransform);

  d->TranslateFirstToolButton->setVisible(isLinearTransform);
  d->MatrixViewGroupBox->setVisible(isLinearTransform);
  d->TranslationSliders->setVisible(isLinearTransform);
  d->RotationSliders->setVisible(isLinearTransform);

  d->copySelectionToolButton->setVisible(isLinearTransform);
  d->pasteSelectionToolButton->setVisible(isLinearTransform);

  d->SplitPushButton->setVisible(isCompositeTransform);

  QStringList nodeTypes;
  // If no transform node, it would show the entire scene, lets shown none
  // instead.
  if (tableNode == 0)
    {
    nodeTypes << QString("vtkMRMLNotANode");
    }
  d->TransformedTreeView->setNodeTypes(nodeTypes);

  // Filter the current node in the transformed tree view
  d->TransformedTreeView->setRootNode(tableNode);

  // Hide the current node in the transformable tree view
  QStringList hiddenNodeIDs;
  if (tableNode)
    {
    hiddenNodeIDs << QString(tableNode->GetID());
    }
  d->TransformableTreeView->sortFilterProxyModel()
    ->setHiddenNodeIDs(hiddenNodeIDs);

    */
  this->qvtkReconnect(d->MRMLTableNode,
    tableNode, vtkCommand::ModifiedEvent,
    this, SLOT(onMRMLTableNodeModified(vtkObject*)));

  d->MRMLTableNode = tableNode;

}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::onMRMLTableNodeModified(vtkObject* caller)
{
  Q_D(qSlicerTablesModuleWidget);

  vtkMRMLTableNode* tableNode = vtkMRMLTableNode::SafeDownCast(caller);
  if (!tableNode)
    {
    return;
    }
  Q_ASSERT(d->MRMLTableNode == tableNode);

  /*
  bool isLinearTransform = tableNode->IsLinear();
  bool isCompositeTransform = tableNode->IsComposite();

  d->TranslateFirstToolButton->setEnabled(isLinearTransform);
  d->IdentityPushButton->setEnabled(isLinearTransform);
  d->MatrixViewGroupBox->setEnabled(isLinearTransform);

  // This method may be called very frequently (when transform is changing
  // in real time). Due to some reason setVisible calls take time,
  // even if the visibility state does not change.
  // To save time, only call the set function if the visibility has to be changed.
  if (isLinearTransform!=d->TranslateFirstToolButton->isVisible())
    {
    d->TranslateFirstToolButton->setVisible(isLinearTransform);
    }
  if (isLinearTransform!=d->MatrixViewGroupBox->isVisible())
    {
    d->MatrixViewGroupBox->setVisible(isLinearTransform);
    }
  if (isLinearTransform!=d->TranslationSliders->isVisible())
    {
    d->TranslationSliders->setVisible(isLinearTransform);
    }
  if (isLinearTransform!=d->RotationSliders->isVisible())
    {
    d->RotationSliders->setVisible(isLinearTransform);
    }
  if (isLinearTransform!=d->copySelectionToolButton->isVisible())
    {
    d->copySelectionToolButton->setVisible(isLinearTransform);
    }
  if (isLinearTransform!=d->pasteSelectionToolButton->isVisible())
    {
    d->pasteSelectionToolButton->setVisible(isLinearTransform);
    }

  d->SplitPushButton->setVisible(isCompositeTransform);
*/
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::copySelection()
{
  Q_D(qSlicerTablesModuleWidget);

  if (!d->TableView->selectionModel()->hasSelection())
    {
    return;
    }

  qMRMLTableModel* tableModel = d->TableView->tableModel();
  if (tableModel == NULL)
    {
    qWarning("qSlicerTablesModuleWidget::copySelection failed: invalid table model");
    return;
    }

  QModelIndexList selection = d->TableView->selectionModel()->selectedIndexes();
  QString textToCopy;
  bool firstLine = true;
  for (int rowIndex=0; rowIndex<tableModel->rowCount(); rowIndex++)
    {
    if (!d->TableView->selectionModel()->rowIntersectsSelection(rowIndex, QModelIndex()))
      {
      // no items are selected in this entire row, skip it
      continue;
      }
    if (firstLine)
      {
      firstLine = false;
      }
    else
      {
      textToCopy.append('\n');
      }
    bool firstItemInLine = true;
    for (int columnIndex=0; columnIndex<tableModel->columnCount(); columnIndex++)
      {
      if (!d->TableView->selectionModel()->columnIntersectsSelection(columnIndex, QModelIndex()))
        {
        // no items are selected in this entire column, skip it
        continue;
        }
      if (firstItemInLine)
        {
        firstItemInLine = false;
        }
      else
        {
        textToCopy.append('\t');
        }
      textToCopy.append(tableModel->item(rowIndex,columnIndex)->text());
      }
    }

  QApplication::clipboard()->setText(textToCopy);
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::pasteSelection()
{
  Q_D(qSlicerTablesModuleWidget);

  QString text = QApplication::clipboard()->text();
  if (text.isEmpty())
    {
    return;
    }

  qMRMLTableModel* tableModel = d->TableView->tableModel();
  if (tableModel == NULL)
    {
    qWarning("qSlicerTablesModuleWidget::copySelection failed: invalid table model");
    return;
    }

  int rowIndex = d->TableView->currentIndex().row();
  int startColumnIndex = d->TableView->currentIndex().column();
  QStringList lines = text.split('\n');
  foreach(QString line, lines)
    {
    if (rowIndex>=tableModel->rowCount())
        {
        // reached last row in the table, ignore subsequent rows
        break;
        }
    int columnIndex = startColumnIndex;
    QStringList cells = line.split('\t');
    foreach(QString cell, cells)
      {
      if (columnIndex>=tableModel->columnCount())
        {
        // reached last column in the table, ignore subsequent columns
        break;
        }
      tableModel->item(rowIndex,columnIndex)->setText(cell);
      columnIndex++;
      }
    rowIndex++;
  }
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::insertColumn()
{
  Q_D(qSlicerTablesModuleWidget);
  vtkMRMLTableNode* tableNode = d->MRMLTableNode;
  if (tableNode==NULL)
    {
    qWarning("qSlicerTablesModuleWidget::insertColumn failed: invalid table node");
    return;
    }
  if (d->TableView->tableModel()->transposed())
    {
    tableNode->AddEmptyRow();
    }
  else
    {
    tableNode->AddColumn();
    }
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::deleteColumn()
{
  Q_D(qSlicerTablesModuleWidget);
  vtkMRMLTableNode* tableNode = d->MRMLTableNode;
  if (tableNode==NULL)
    {
    qWarning("qSlicerTablesModuleWidget::deleteColumn failed: invalid table node");
    return;
    }
  QModelIndexList selection = d->TableView->selectionModel()->selectedIndexes();
  d->TableView->tableModel()->removeSelectionFromMRML(selection, false);
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::insertRow()
{
  Q_D(qSlicerTablesModuleWidget);
  vtkMRMLTableNode* tableNode = d->MRMLTableNode;
  if (tableNode==NULL)
    {
    qWarning("qSlicerTablesModuleWidget::insertRow failed: invalid table node");
    return;
    }
  if (d->TableView->tableModel()->transposed())
    {
    tableNode->AddColumn();
    }
  else
    {
    tableNode->AddEmptyRow();
    }
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::deleteRow()
{
  Q_D(qSlicerTablesModuleWidget);
  vtkMRMLTableNode* tableNode = d->MRMLTableNode;
  if (tableNode==NULL)
    {
    qWarning("qSlicerTablesModuleWidget::deleteRow failed: invalid table node");
    return;
    }
  QModelIndexList selection = d->TableView->selectionModel()->selectedIndexes();
  d->TableView->tableModel()->removeSelectionFromMRML(selection, true);
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::toggleLabelInFirstColumn()
{
  Q_D(qSlicerTablesModuleWidget);
  vtkMRMLTableNode* tableNode = d->MRMLTableNode;
  if (tableNode==NULL)
    {
    qWarning("qSlicerTablesModuleWidget::toggleLabelInFirstColumn failed: invalid table node");
    return;
    }
  tableNode->SetLabelInFirstColumn(!tableNode->GetLabelInFirstColumn());
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerTablesModuleWidget);
  this->Superclass::setMRMLScene(scene);
}
