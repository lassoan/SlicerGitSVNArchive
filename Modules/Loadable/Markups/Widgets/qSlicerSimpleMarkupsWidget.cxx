/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Matthew Holden, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Markups Widgets includes
#include "qSlicerSimpleMarkupsWidget.h"

// Markups includes
#include <vtkSlicerMarkupsLogic.h>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerModuleManager.h"
#include "qSlicerAbstractCoreModule.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>

// Qt includes
#include <QtGui>

int FIDUCIAL_LABEL_COLUMN = 0;
int FIDUCIAL_X_COLUMN = 1;
int FIDUCIAL_Y_COLUMN = 2;
int FIDUCIAL_Z_COLUMN = 3;
int FIDUCIAL_COLUMNS = 4;


//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_CreateModels
class qSlicerSimpleMarkupsWidgetPrivate
  : public Ui_qSlicerSimpleMarkupsWidget
{
  Q_DECLARE_PUBLIC(qSlicerSimpleMarkupsWidget);
protected:
  qSlicerSimpleMarkupsWidget* const q_ptr;

public:
  qSlicerSimpleMarkupsWidgetPrivate( qSlicerSimpleMarkupsWidget& object);
  ~qSlicerSimpleMarkupsWidgetPrivate();
  virtual void setupUi(qSlicerSimpleMarkupsWidget*);

public:
  QColor DefaultNodeColor;

  bool EnterPlaceModeOnNodeChange;
  bool JumpToSliceEnabled;

  QList < QWidget* > OptionsWidgets;

  vtkWeakPointer<vtkSlicerMarkupsLogic> MarkupsLogic;
  vtkWeakPointer<vtkMRMLMarkupsNode> CurrentMarkupsNode;
};

// --------------------------------------------------------------------------
qSlicerSimpleMarkupsWidgetPrivate::qSlicerSimpleMarkupsWidgetPrivate( qSlicerSimpleMarkupsWidget& object)
  : q_ptr(&object)
  , EnterPlaceModeOnNodeChange(true)
  , JumpToSliceEnabled(false)
{
}

//-----------------------------------------------------------------------------
qSlicerSimpleMarkupsWidgetPrivate::~qSlicerSimpleMarkupsWidgetPrivate()
{
}

// --------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidgetPrivate::setupUi(qSlicerSimpleMarkupsWidget* widget)
{
  this->Ui_qSlicerSimpleMarkupsWidget::setupUi(widget);
}


//-----------------------------------------------------------------------------
// qSlicerSimpleMarkupsWidget methods

//-----------------------------------------------------------------------------
qSlicerSimpleMarkupsWidget::qSlicerSimpleMarkupsWidget(QWidget* parentWidget) : Superclass( parentWidget ) , d_ptr( new qSlicerSimpleMarkupsWidgetPrivate(*this) )
{
  Q_D(qSlicerSimpleMarkupsWidget);
  this->setup();
}

//-----------------------------------------------------------------------------
qSlicerSimpleMarkupsWidget::~qSlicerSimpleMarkupsWidget()
{
  this->setCurrentNode(NULL);
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setup()
{
  Q_D(qSlicerSimpleMarkupsWidget);

  // This cannot be called by the constructor, because Slicer may not exist when the constructor is called
  d->MarkupsLogic = NULL;
  if (qSlicerApplication::application() != NULL && qSlicerApplication::application()->moduleManager() != NULL)
    {
    qSlicerAbstractCoreModule* markupsModule = qSlicerApplication::application()->moduleManager()->module( "Markups" );
    if ( markupsModule != NULL )
      {
      d->MarkupsLogic = vtkSlicerMarkupsLogic::SafeDownCast( markupsModule->logic() );
      }
    }
  if (d->MarkupsLogic == NULL)
    {
    qCritical("qSlicerSimpleMarkupsWidget::setup: Markups module is not found, some markup manipulation features will not be available");
    }
  d->setupUi(this);

  d->OptionsWidgets << d->ColorButton << d->MarkupsPlaceWidget << d->MoreButton;

  d->DefaultNodeColor.setRgb(0.0,1.0,0.0);

  connect( d->MarkupsFiducialNodeComboBox, SIGNAL( currentNodeChanged( vtkMRMLNode* ) ), this, SLOT( onMarkupsFiducialNodeChanged() ) );
  connect( d->MarkupsFiducialNodeComboBox, SIGNAL( nodeAddedByUser( vtkMRMLNode* ) ), this, SLOT( onMarkupsFiducialNodeAdded( vtkMRMLNode* ) ) );

  // Use the pressed signal (otherwise we can unpress buttons without clicking them)
  connect( d->ColorButton, SIGNAL( colorChanged( QColor ) ), this, SLOT( onColorButtonChanged( QColor ) ) );

  connect( d->MarkupsPlaceWidget, SIGNAL( activeMarkupsFiducialPlaceModeChanged(bool) ), this, SIGNAL( activeMarkupsFiducialPlaceModeChanged(bool) ) );

  QMenu* moreMenu = new QMenu(tr("More options"), d->MoreButton);
  moreMenu->setObjectName("moreMenu");
  moreMenu->addAction(d->ActionVisibility);
  moreMenu->addAction(d->ActionLocked);
  QObject::connect(d->ActionVisibility, SIGNAL(triggered()), this, SLOT(onVisibilityButtonClicked()));
  QObject::connect(d->ActionLocked, SIGNAL(triggered()), this, SLOT(onLockedButtonClicked()));
  d->MoreButton->setMenu(moreMenu);

  d->MarkupsFiducialTableWidget->setColumnCount( FIDUCIAL_COLUMNS );
  d->MarkupsFiducialTableWidget->setHorizontalHeaderLabels( QStringList() << "Label" << "X" << "Y" << "Z" );
  d->MarkupsFiducialTableWidget->horizontalHeader()->setResizeMode( QHeaderView::Stretch );
  d->MarkupsFiducialTableWidget->setContextMenuPolicy( Qt::CustomContextMenu );
  // only select rows rather than cells
  d->MarkupsFiducialTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

  connect(d->MarkupsFiducialTableWidget, SIGNAL( customContextMenuRequested(const QPoint&) ), this, SLOT( onMarkupsFiducialTableContextMenu(const QPoint&) ) );
  connect(d->MarkupsFiducialTableWidget, SIGNAL( cellChanged( int, int ) ), this, SLOT( onMarkupsFiducialEdited( int, int ) ) );
  // listen for click on a markup
  connect(d->MarkupsFiducialTableWidget, SIGNAL(cellClicked(int,int)), this, SLOT(onMarkupsFiducialSelected(int,int)));
  // listen for the current cell selection change (happens when arrows are used to navigate)
  connect(d->MarkupsFiducialTableWidget, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(onMarkupsFiducialSelected(int, int)));

}

//-----------------------------------------------------------------------------
vtkMRMLNode* qSlicerSimpleMarkupsWidget::currentNode() const
{
  Q_D(const qSlicerSimpleMarkupsWidget);
  return d->MarkupsFiducialNodeComboBox->currentNode();
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qSlicerSimpleMarkupsWidget::getCurrentNode()
{
  qWarning("qSlicerSimpleMarkupsWidget::getCurrentNode() method is deprecated. Use qSlicerSimpleMarkupsWidget::currentNode() method instead");
  return this->currentNode();
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setCurrentNode(vtkMRMLNode* currentNode)
{
  Q_D(qSlicerSimpleMarkupsWidget);

  vtkMRMLMarkupsNode* currentMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( currentNode );
  if (currentMarkupsNode==d->CurrentMarkupsNode)
    {
    // not changed
    return;
    }

  // Don't change the active fiducial list if the current node is changed programmatically
  bool wasBlocked = d->MarkupsFiducialNodeComboBox->blockSignals(true);
  d->MarkupsFiducialNodeComboBox->setCurrentNode( currentMarkupsNode );
  d->MarkupsFiducialNodeComboBox->blockSignals(wasBlocked);

  d->MarkupsPlaceWidget->setCurrentNode( currentMarkupsNode );

  // Reconnect the appropriate nodes
  this->qvtkReconnect(d->CurrentMarkupsNode, currentMarkupsNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidget()));
  this->qvtkReconnect(d->CurrentMarkupsNode, currentMarkupsNode, vtkMRMLDisplayableNode::DisplayModifiedEvent, this, SLOT(updateWidget()));
  this->qvtkReconnect(d->CurrentMarkupsNode, currentMarkupsNode, vtkMRMLMarkupsNode::MarkupAddedEvent, d->MarkupsFiducialTableWidget, SLOT(scrollToBottom()));
  d->CurrentMarkupsNode = currentMarkupsNode;

  this->updateWidget();

  emit markupsFiducialNodeChanged();
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setNodeBaseName(QString newNodeBaseName)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  d->MarkupsFiducialNodeComboBox->setBaseName(newNodeBaseName);
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setDefaultNodeColor(QColor color)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  d->DefaultNodeColor = color;
}

//-----------------------------------------------------------------------------
bool qSlicerSimpleMarkupsWidget::enterPlaceModeOnNodeChange() const
{
  Q_D(const qSlicerSimpleMarkupsWidget);
  return d->EnterPlaceModeOnNodeChange;
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setEnterPlaceModeOnNodeChange(bool enterPlaceMode)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  d->EnterPlaceModeOnNodeChange = enterPlaceMode;
}

//-----------------------------------------------------------------------------
bool qSlicerSimpleMarkupsWidget::jumpToSliceEnabled() const
{
  Q_D(const qSlicerSimpleMarkupsWidget);
  return d->JumpToSliceEnabled;
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setJumpToSliceEnabled(bool enable)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  d->JumpToSliceEnabled = enable;
}

//-----------------------------------------------------------------------------
bool qSlicerSimpleMarkupsWidget::nodeSelectorVisible() const
{
  Q_D(const qSlicerSimpleMarkupsWidget);
  return d->MarkupsFiducialNodeComboBox->isVisible();
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setNodeSelectorVisible(bool visible)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  d->MarkupsFiducialNodeComboBox->setVisible(visible);
}

//-----------------------------------------------------------------------------
bool qSlicerSimpleMarkupsWidget::optionsVisible() const
{
  Q_D(const qSlicerSimpleMarkupsWidget);
  foreach( QWidget *w, d->OptionsWidgets )
    {
    if (!w->isVisible())
      {
      return false;
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setOptionsVisible(bool visible)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  foreach( QWidget *w, d->OptionsWidgets )
    {
    w->setVisible(visible);
    }
}

//-----------------------------------------------------------------------------
QTableWidget* qSlicerSimpleMarkupsWidget::tableWidget() const
{
  Q_D(const qSlicerSimpleMarkupsWidget);
  return d->MarkupsFiducialTableWidget;
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setNodeColor(QColor color)
{
  Q_D(qSlicerSimpleMarkupsWidget);

  vtkMRMLMarkupsNode* currentMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( d->MarkupsFiducialNodeComboBox->currentNode() );
  if ( currentMarkupsNode == NULL )
    {
    return;
    }

  vtkMRMLDisplayNode* currentMarkupsDisplayNode = currentMarkupsNode->GetDisplayNode();
  if ( currentMarkupsDisplayNode == NULL )
    {
    return;
    }

  double rgbDoubleVector[3] = {color.redF(),color.greenF(),color.blueF()};
  currentMarkupsDisplayNode->SetColor( rgbDoubleVector );
  currentMarkupsDisplayNode->SetSelectedColor( rgbDoubleVector );
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::getNodeColor(QColor color)
{
  Q_D(qSlicerSimpleMarkupsWidget);

  vtkMRMLMarkupsNode* currentMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( d->MarkupsFiducialNodeComboBox->currentNode() );
  if ( currentMarkupsNode == NULL )
    {
    return;
    }

  vtkMRMLDisplayNode* currentMarkupsDisplayNode = currentMarkupsNode->GetDisplayNode();
  if ( currentMarkupsDisplayNode == NULL )
    {
    return;
    }

  double rgbDoubleVector[3] = {0.0,0.0,0.0};
  currentMarkupsDisplayNode->GetSelectedColor(rgbDoubleVector);
  color.setRgb(rgbDoubleVector[0], rgbDoubleVector[1], rgbDoubleVector[2]);
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::highlightNthFiducial(int n)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  if ( n >= 0 && n < d->MarkupsFiducialTableWidget->rowCount() )
    {
    d->MarkupsFiducialTableWidget->selectRow(n);
    d->MarkupsFiducialTableWidget->setCurrentCell(n,0);
    d->MarkupsFiducialTableWidget->scrollTo(d->MarkupsFiducialTableWidget->currentIndex());
    }
  else
    {
    d->MarkupsFiducialTableWidget->clearSelection();
    }
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::onColorButtonChanged(QColor color)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  vtkMRMLMarkupsNode* currentMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( d->MarkupsFiducialNodeComboBox->currentNode() );
  if ( currentMarkupsNode == NULL && currentMarkupsNode->GetDisplayNode() == NULL )
    {
    return;
    }
  double colorDoubleVector[3] = {0.0,0.0,0.0};
  qMRMLUtils::qColorToColor( color, colorDoubleVector );
  currentMarkupsNode->GetDisplayNode()->SetSelectedColor( colorDoubleVector );
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::onVisibilityButtonClicked()
{
  Q_D(qSlicerSimpleMarkupsWidget);
  vtkMRMLMarkupsNode* currentMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( d->MarkupsFiducialNodeComboBox->currentNode() );
  if ( currentMarkupsNode == NULL || currentMarkupsNode->GetDisplayNode() == NULL )
    {
    return;
    }
  currentMarkupsNode->GetDisplayNode()->SetVisibility( ! currentMarkupsNode->GetDisplayNode()->GetVisibility() );
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::onLockedButtonClicked()
{
  Q_D(qSlicerSimpleMarkupsWidget);
  vtkMRMLMarkupsNode* currentMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( d->MarkupsFiducialNodeComboBox->currentNode() );
  if ( currentMarkupsNode == NULL )
    {
    return;
    }
  currentMarkupsNode->SetLocked( ! currentMarkupsNode->GetLocked() );
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::onDeleteButtonClicked()
{
  Q_D(qSlicerSimpleMarkupsWidget);
  vtkMRMLMarkupsNode* currentMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( d->MarkupsFiducialNodeComboBox->currentNode() );
  if ( currentMarkupsNode  == NULL )
    {
    return;
    }

  QItemSelectionModel* selectionModel = d->MarkupsFiducialTableWidget->selectionModel();
  std::vector< int > deleteFiducials;
  // Need to find selected before removing because removing automatically refreshes the table
  for ( int i = 0; i < d->MarkupsFiducialTableWidget->rowCount(); i++ )
    {
    if ( selectionModel->rowIntersectsSelection( i, d->MarkupsFiducialTableWidget->rootIndex() ) )
      {
      deleteFiducials.push_back( i );
      }
    }
  // Traversing this way should be more efficient and correct
  for ( int i = deleteFiducials.size() - 1; i >= 0; i-- )
    {
    currentMarkupsNode->RemoveMarkup( deleteFiducials.at( i ) );
    }

  this->updateWidget();
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::activate()
{
  Q_D(qSlicerSimpleMarkupsWidget);
  d->MarkupsPlaceWidget->setCurrentNodeActive(true);
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::placeActive(bool place)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  d->MarkupsPlaceWidget->setPlaceModeEnabled(place);
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::onMarkupsFiducialNodeChanged()
{
  Q_D(qSlicerSimpleMarkupsWidget);
  vtkMRMLMarkupsNode* currentMarkupsNode = vtkMRMLMarkupsNode::SafeDownCast( d->MarkupsFiducialNodeComboBox->currentNode() );

  if (d->MarkupsLogic != NULL && this->mrmlScene() != NULL)
    {
    // Depending to the current state, change the activeness and placeness for the current markups node
    vtkMRMLInteractionNode *interactionNode = vtkMRMLInteractionNode::SafeDownCast( this->mrmlScene()->GetNodeByID( "vtkMRMLInteractionNodeSingleton" ) );

    if ( currentMarkupsNode != NULL )
      {
      d->MarkupsLogic->SetActiveListID( currentMarkupsNode ); // If there are other widgets, they are responsible for updating themselves
      if( d->EnterPlaceModeOnNodeChange && interactionNode != NULL)
        {
        interactionNode->SetCurrentInteractionMode( vtkMRMLInteractionNode::Place );
        }
      }
    else if( d->EnterPlaceModeOnNodeChange && interactionNode != NULL)
      {
      interactionNode->SetCurrentInteractionMode( vtkMRMLInteractionNode::ViewTransform );
      }
    }

  this->setCurrentNode(currentMarkupsNode);
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::onMarkupsFiducialNodeAdded( vtkMRMLNode* newNode )
{
  Q_D(qSlicerSimpleMarkupsWidget);

  if (d->MarkupsLogic == NULL)
    {
    qCritical("qSlicerSimpleMarkupsWidget::onMarkupsFiducialNodeAdded failed: Markups module logic is invalid");
    return;
    }

  vtkMRMLMarkupsFiducialNode* newMarkupsFiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( newNode );
  d->MarkupsLogic->AddNewDisplayNodeForMarkupsNode( newMarkupsFiducialNode ); // Make sure there is an associated display node
  d->MarkupsFiducialNodeComboBox->setCurrentNode( newMarkupsFiducialNode );
  this->setNodeColor( d->DefaultNodeColor );
  this->onMarkupsFiducialNodeChanged();
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::onMarkupsFiducialTableContextMenu(const QPoint& position)
{
  Q_D(qSlicerSimpleMarkupsWidget);

  if (d->MarkupsLogic == NULL)
    {
    qCritical("qSlicerSimpleMarkupsWidget::onMarkupsFiducialTableContextMenu failed: Markups module logic is invalid");
    return;
    }

  QPoint globalPosition = d->MarkupsFiducialTableWidget->viewport()->mapToGlobal( position );

  QMenu* fiducialsMenu = new QMenu( d->MarkupsFiducialTableWidget );
  QAction* deleteAction = new QAction( "Delete highlighted fiducials", fiducialsMenu );
  QAction* upAction = new QAction( "Move current fiducial up", fiducialsMenu );
  QAction* downAction = new QAction( "Move current fiducial down", fiducialsMenu );
  QAction* jumpAction = new QAction( "Jump slices to fiducial", fiducialsMenu );

  fiducialsMenu->addAction( deleteAction );
  fiducialsMenu->addAction( upAction );
  fiducialsMenu->addAction( downAction );
  fiducialsMenu->addAction( jumpAction );

  QAction* selectedAction = fiducialsMenu->exec( globalPosition );

  int currentFiducial = d->MarkupsFiducialTableWidget->currentRow();
  vtkMRMLMarkupsFiducialNode* currentNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( d->MarkupsFiducialNodeComboBox->currentNode() );

  if ( currentNode == NULL )
    {
    return;
    }

  // Only do this for non-null node
  if ( selectedAction == deleteAction )
    {
    QItemSelectionModel* selectionModel = d->MarkupsFiducialTableWidget->selectionModel();
    std::vector< int > deleteFiducials;
    // Need to find selected before removing because removing automatically refreshes the table
    for ( int i = 0; i < d->MarkupsFiducialTableWidget->rowCount(); i++ )
      {
      if ( selectionModel->rowIntersectsSelection( i, d->MarkupsFiducialTableWidget->rootIndex() ) )
        {
        deleteFiducials.push_back( i );
        }
      }
    //Traversing this way should be more efficient and correct
    for ( int i = deleteFiducials.size() - 1; i >= 0; i-- )
      {
      currentNode->RemoveMarkup( deleteFiducials.at( i ) );
      }
    }


  if ( selectedAction == upAction )
    {
    if ( currentFiducial > 0 )
      {
      currentNode->SwapMarkups( currentFiducial, currentFiducial - 1 );
      }
    }

  if ( selectedAction == downAction )
    {
    if ( currentFiducial < currentNode->GetNumberOfFiducials() - 1 )
      {
      currentNode->SwapMarkups( currentFiducial, currentFiducial + 1 );
      }
    }

  if ( selectedAction == jumpAction )
    {
    d->MarkupsLogic->JumpSlicesToNthPointInMarkup( this->currentNode()->GetID(), currentFiducial, true /* centered */ );
    }

  this->updateWidget();
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::onMarkupsFiducialSelected(int row, int column)
{
  Q_UNUSED(column)
  Q_D(qSlicerSimpleMarkupsWidget);

  if (d->JumpToSliceEnabled)
    {
    vtkMRMLMarkupsFiducialNode* currentMarkupsFiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( this->currentNode() );
    if ( currentMarkupsFiducialNode == NULL )
      {
      return;
      }
    if (d->MarkupsLogic == NULL)
      {
      qCritical("qSlicerSimpleMarkupsWidget::onMarkupsFiducialSelected failed: Cannot jump, markups module logic is invalid");
      return;
      }
    d->MarkupsLogic->JumpSlicesToNthPointInMarkup(currentMarkupsFiducialNode->GetID(), row, true /* centered */);
    }

  emit currentMarkupsFiducialSelectionChanged(row);
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::onMarkupsFiducialEdited(int row, int column)
{
  Q_D(qSlicerSimpleMarkupsWidget);

  vtkMRMLMarkupsFiducialNode* currentMarkupsFiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( this->currentNode() );

  if ( currentMarkupsFiducialNode == NULL )
    {
    return;
    }

  // Find the fiducial's current properties
  double currentFiducialPosition[3] = { 0, 0, 0 };
  currentMarkupsFiducialNode->GetNthFiducialPosition( row, currentFiducialPosition );
  std::string currentFiducialLabel = currentMarkupsFiducialNode->GetNthFiducialLabel( row );

  // Find the entry that we changed
  QTableWidgetItem* qItem = d->MarkupsFiducialTableWidget->item( row, column );
  QString qText = qItem->text();

  if ( column == FIDUCIAL_LABEL_COLUMN )
    {
    currentMarkupsFiducialNode->SetNthFiducialLabel( row, qText.toStdString() );
    }

  // Check if the value can be converted to double is already performed implicitly
  double newFiducialPosition = qText.toDouble();

  // Change the position values
  if ( column == FIDUCIAL_X_COLUMN )
    {
    currentFiducialPosition[ 0 ] = newFiducialPosition;
    }
  if ( column == FIDUCIAL_Y_COLUMN )
    {
    currentFiducialPosition[ 1 ] = newFiducialPosition;
    }
  if ( column == FIDUCIAL_Z_COLUMN )
    {
    currentFiducialPosition[ 2 ] = newFiducialPosition;
    }

  currentMarkupsFiducialNode->SetNthFiducialPositionFromArray( row, currentFiducialPosition );

  this->updateWidget(); // This may not be necessary the widget is updated whenever a fiducial is changed
}

//-----------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::updateWidget()
{
  Q_D(qSlicerSimpleMarkupsWidget);

  if (d->MarkupsLogic == NULL || this->mrmlScene() == NULL)
    {
    qCritical("qSlicerSimpleMarkupsWidget::updateWidget failed: Markups module logic or scene is invalid");
    }

  vtkMRMLMarkupsFiducialNode* currentMarkupsFiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast( d->MarkupsFiducialNodeComboBox->currentNode() );
  if ( currentMarkupsFiducialNode == NULL || d->MarkupsLogic == NULL)
    {
    d->MarkupsFiducialTableWidget->clear();
    d->MarkupsFiducialTableWidget->setRowCount( 0 );
    d->MarkupsFiducialTableWidget->setColumnCount( 0 );
    d->ColorButton->setEnabled(false);
    d->MarkupsPlaceWidget->setEnabled(false);
    d->MoreButton->setEnabled(false);
    emit updateFinished();
    return;
    }

  d->ColorButton->setEnabled(true);
  d->MarkupsPlaceWidget->setEnabled(true);
  d->MoreButton->setEnabled(true);

  // Set the button indicating if this list is active
  bool wasBlockedColorButton = d->ColorButton->blockSignals( true );
  bool wasBlockedVisibilityButton = d->ActionVisibility->blockSignals( true );
  bool wasBlockedLockButton = d->ActionLocked->blockSignals( true );

  // Depending to the current state, change the activeness and placeness for the current markups node
  vtkMRMLInteractionNode *interactionNode = vtkMRMLInteractionNode::SafeDownCast( this->mrmlScene()->GetNodeByID( "vtkMRMLInteractionNodeSingleton" ) );

  if ( currentMarkupsFiducialNode->GetDisplayNode() != NULL  )
    {
    double* color = currentMarkupsFiducialNode->GetDisplayNode()->GetSelectedColor();
    QColor qColor;
    qMRMLUtils::colorToQColor( color, qColor );
    d->ColorButton->setColor( qColor );
    }

  if ( currentMarkupsFiducialNode->GetLocked() )
    {
    d->ActionLocked->setIcon( QIcon( ":/Icons/Small/SlicerLock.png" ) );
    }
  else
    {
    d->ActionLocked->setIcon( QIcon( ":/Icons/Small/SlicerUnlock.png" ) );
    }

  d->ActionVisibility->setEnabled(currentMarkupsFiducialNode->GetDisplayNode() != NULL);
  if (currentMarkupsFiducialNode->GetDisplayNode() != NULL)
    {
    if (currentMarkupsFiducialNode->GetDisplayNode()->GetVisibility() )
      {
      d->ActionVisibility->setIcon( QIcon( ":/Icons/Small/SlicerVisible.png" ) );
      }
    else
      {
      d->ActionVisibility->setIcon( QIcon( ":/Icons/Small/SlicerInvisible.png" ) );
      }
    }

  d->ColorButton->blockSignals( wasBlockedColorButton );
  d->ActionVisibility->blockSignals( wasBlockedVisibilityButton);
  d->ActionLocked->blockSignals( wasBlockedLockButton );

  // Update the fiducials table
  bool wasBlockedTableWidget = d->MarkupsFiducialTableWidget->blockSignals( true );

  if (d->MarkupsFiducialTableWidget->rowCount()==currentMarkupsFiducialNode->GetNumberOfFiducials())
    {
    // don't recreate the table if the number of items is not changed to preserve selection state
    double fiducialPosition[ 3 ] = { 0, 0, 0 };
    std::string fiducialLabel;
    for ( int i = 0; i < currentMarkupsFiducialNode->GetNumberOfFiducials(); i++ )
      {
      fiducialLabel = currentMarkupsFiducialNode->GetNthFiducialLabel( i );
      currentMarkupsFiducialNode->GetNthFiducialPosition( i, fiducialPosition );
      d->MarkupsFiducialTableWidget->item(i, FIDUCIAL_LABEL_COLUMN)->setText(QString::fromStdString( fiducialLabel ));
      d->MarkupsFiducialTableWidget->item(i, FIDUCIAL_X_COLUMN)->setText(QString::number( fiducialPosition[0], 'f', 3 ));
      d->MarkupsFiducialTableWidget->item(i, FIDUCIAL_Y_COLUMN)->setText(QString::number( fiducialPosition[1], 'f', 3 ));
      d->MarkupsFiducialTableWidget->item(i, FIDUCIAL_Z_COLUMN)->setText(QString::number( fiducialPosition[2], 'f', 3 ));
      }
    }
  else
    {
    d->MarkupsFiducialTableWidget->clear();
    d->MarkupsFiducialTableWidget->setRowCount( currentMarkupsFiducialNode->GetNumberOfFiducials() );
    d->MarkupsFiducialTableWidget->setColumnCount( FIDUCIAL_COLUMNS );
    d->MarkupsFiducialTableWidget->setHorizontalHeaderLabels( QStringList() << "Label" << "X" << "Y" << "Z" );

    double fiducialPosition[ 3 ] = { 0, 0, 0 };
    std::string fiducialLabel;
    for ( int i = 0; i < currentMarkupsFiducialNode->GetNumberOfFiducials(); i++ )
      {
      fiducialLabel = currentMarkupsFiducialNode->GetNthFiducialLabel( i );
      currentMarkupsFiducialNode->GetNthFiducialPosition( i, fiducialPosition );

      QTableWidgetItem* labelItem = new QTableWidgetItem( QString::fromStdString( fiducialLabel ) );
      QTableWidgetItem* xItem = new QTableWidgetItem( QString::number( fiducialPosition[0], 'f', 3 ) );
      QTableWidgetItem* yItem = new QTableWidgetItem( QString::number( fiducialPosition[1], 'f', 3 ) );
      QTableWidgetItem* zItem = new QTableWidgetItem( QString::number( fiducialPosition[2], 'f', 3 ) );

      d->MarkupsFiducialTableWidget->setItem( i, FIDUCIAL_LABEL_COLUMN, labelItem );
      d->MarkupsFiducialTableWidget->setItem( i, FIDUCIAL_X_COLUMN, xItem );
      d->MarkupsFiducialTableWidget->setItem( i, FIDUCIAL_Y_COLUMN, yItem );
      d->MarkupsFiducialTableWidget->setItem( i, FIDUCIAL_Z_COLUMN, zItem );
      }
    }

  d->MarkupsFiducialTableWidget->blockSignals( wasBlockedTableWidget );

  emit updateFinished();
}

//------------------------------------------------------------------------------
void qSlicerSimpleMarkupsWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerSimpleMarkupsWidget);
  Superclass::setMRMLScene(scene);
  this->updateWidget();
}

//-----------------------------------------------------------------------------
qSlicerMarkupsPlaceWidget* qSlicerSimpleMarkupsWidget::markupsPlaceWidget() const
{
  Q_D(const qSlicerSimpleMarkupsWidget);
  return d->MarkupsPlaceWidget;
}
