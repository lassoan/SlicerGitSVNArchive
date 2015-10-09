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

// MRML includes
#include "vtkMRMLTableNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>
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

  // Set a static min/max range to let users freely enter values
//  d->MatrixWidget->setRange(-1e10, 1e10);

  // Transform nodes connection
  /*
  this->connect(d->TransformToolButton, SIGNAL(clicked()),
                SLOT(transformSelectedNodes()));
  this->connect(d->UntransformToolButton, SIGNAL(clicked()),
                SLOT(untransformSelectedNodes()));
  this->connect(d->HardenToolButton, SIGNAL(clicked()),
                SLOT(hardenSelectedNodes()));
                */

  // Connect copy and paste actions
/*
d->copyTableToolButton->setDefaultAction(d->CopyAction);
  this->connect(d->CopyAction,
                SIGNAL(triggered()),
                SLOT(copyTable()));

  d->pasteTableToolButton->setDefaultAction(d->PasteAction);
  this->connect(d->PasteAction,
                SIGNAL(triggered()),
                SLOT(pasteTable()));

  // Icons
  QIcon rightIcon =
    QApplication::style()->standardIcon(QStyle::SP_ArrowRight);
  d->TransformToolButton->setIcon(rightIcon);

  QIcon leftIcon =
    QApplication::style()->standardIcon(QStyle::SP_ArrowLeft);
  d->UntransformToolButton->setIcon(leftIcon);
*/
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

  d->copyTableToolButton->setVisible(isLinearTransform);
  d->pasteTableToolButton->setVisible(isLinearTransform);

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
  if (isLinearTransform!=d->copyTableToolButton->isVisible())
    {
    d->copyTableToolButton->setVisible(isLinearTransform);
    }
  if (isLinearTransform!=d->pasteTableToolButton->isVisible())
    {
    d->pasteTableToolButton->setVisible(isLinearTransform);
    }

  d->SplitPushButton->setVisible(isCompositeTransform);
*/
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::copyTable()
{
  Q_D(qSlicerTablesModuleWidget);
/*
  vtkLinearTransform* linearTransform =
      vtkLinearTransform::SafeDownCast(d->MRMLTableNode->GetTransformToParent());
  if (!linearTransform)
    {
    // Silent fail, no worries!
    qDebug() << "Unable to cast parent transform as a vtkLinearTransform";
    return;
    }

  vtkMatrix4x4* internalMatrix = linearTransform->GetMatrix();
  QString output;

  for (int rowIndex = 0; rowIndex < 4; ++rowIndex)
    {
    for (int columnIndex = 0; columnIndex < 4; ++columnIndex)
     {
      output.append(
            QString::number(internalMatrix->GetElement(rowIndex, columnIndex)));
      output.append(" ");
     }
    output.append("\n");
    }

  QApplication::clipboard()->setText(output);
  */
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::pasteTable()
{
  Q_D(qSlicerTablesModuleWidget);

  /*
  vtkNew<vtkMatrix4x4> tempMatrix;

  QString text = QApplication::clipboard()->text();
  QRegExp rx("(\\ |\\,|\\:|\\t|\\n|\\[|\\])");
  QStringList entries = text.split(rx, QString::SkipEmptyParts);

  if (entries.count() != 4
      && entries.count() != 9
      && entries.count() != 16)
    {
    // Silent fail, incompatible matrix size
    qDebug() << "qSlicerTablesModuleWidget::pasteTable -- "
                "Pasted matrix is not a 2x2 or 3x3 or 4x4 matrix.";
    return;
    }

  bool ok = true;
  int index = 0;
  foreach(const QString& entry, entries)
    {
    double numericEntry = entry.toDouble(&ok);
    if (!ok)
      {
      // Silent fail, no problem!
      qDebug() << "qSlicerTablesModuleWidget::pasteTable -- "
                  "Unable to cast string to double: " << entry;
      return;
      }
    tempMatrix->SetElement(index / static_cast<int>(sqrt(static_cast<double>(entries.count()))),
                           index % static_cast<int>(sqrt(static_cast<double>(entries.count()))),
                           numericEntry);
    ++index;
  }

  d->MRMLTableNode->SetMatrixTransformToParent(tempMatrix.GetPointer());
  */
}

//-----------------------------------------------------------------------------
void qSlicerTablesModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerTablesModuleWidget);
  this->Superclass::setMRMLScene(scene);
}
