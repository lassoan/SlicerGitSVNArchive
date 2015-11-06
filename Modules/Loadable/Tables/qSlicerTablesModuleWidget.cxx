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

  this->connect(d->ColumnInsertButton, SIGNAL(clicked()), d->TableView, SLOT(insertColumn()));
  this->connect(d->ColumnDeleteButton, SIGNAL(clicked()), d->TableView, SLOT(deleteColumn()));
  this->connect(d->RowInsertButton, SIGNAL(clicked()), d->TableView, SLOT(insertRow()));
  this->connect(d->RowDeleteButton, SIGNAL(clicked()), d->TableView, SLOT(deleteRow()));
  this->connect(d->LockFirstRowButton, SIGNAL(toggled(bool)), d->TableView, SLOT(setFirstRowLocked(bool)));
  this->connect(d->LockFirstColumnButton, SIGNAL(toggled(bool)), d->TableView, SLOT(setFirstColumnLocked(bool)));

  // Connect copy and paste actions
  d->CopyButton->setDefaultAction(d->CopyAction);
  this->connect(d->CopyAction, SIGNAL(triggered()), d->TableView, SLOT(copySelection()));
  d->PasteButton->setDefaultAction(d->PasteAction);
  this->connect(d->PasteAction, SIGNAL(triggered()), d->TableView, SLOT(pasteSelection()));

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
void qSlicerTablesModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerTablesModuleWidget);
  this->Superclass::setMRMLScene(scene);
}
