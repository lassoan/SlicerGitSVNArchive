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

// Tables Widgets includes
#include "qSlicerTableColumnPropertiesWidget.h"

// Markups includes
#include <vtkSlicerTablesLogic.h>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerModuleManager.h"
#include "qSlicerAbstractCoreModule.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLTableNode.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_CreateModels
class qSlicerTableColumnPropertiesWidgetPrivate
  : public Ui_qSlicerTableColumnPropertiesWidget
{
  Q_DECLARE_PUBLIC(qSlicerTableColumnPropertiesWidget);
protected:
  qSlicerTableColumnPropertiesWidget* const q_ptr;

public:
  qSlicerTableColumnPropertiesWidgetPrivate( qSlicerTableColumnPropertiesWidget& object);
  ~qSlicerTableColumnPropertiesWidgetPrivate();
  virtual void setupUi(qSlicerTableColumnPropertiesWidget*);

public:
  QStringList ColumnNames;

  vtkWeakPointer<vtkSlicerTablesLogic> TablesLogic;
  vtkWeakPointer<vtkMRMLTableNode> CurrentTableNode;
};

// --------------------------------------------------------------------------
qSlicerTableColumnPropertiesWidgetPrivate::qSlicerTableColumnPropertiesWidgetPrivate( qSlicerTableColumnPropertiesWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerTableColumnPropertiesWidgetPrivate::~qSlicerTableColumnPropertiesWidgetPrivate()
{
}

// --------------------------------------------------------------------------
void qSlicerTableColumnPropertiesWidgetPrivate::setupUi(qSlicerTableColumnPropertiesWidget* widget)
{
  this->Ui_qSlicerTableColumnPropertiesWidget::setupUi(widget);
}


//-----------------------------------------------------------------------------
// qSlicerTableColumnPropertiesWidget methods

//-----------------------------------------------------------------------------
qSlicerTableColumnPropertiesWidget::qSlicerTableColumnPropertiesWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr(new qSlicerTableColumnPropertiesWidgetPrivate(*this))
{
  this->setup();
}

//-----------------------------------------------------------------------------
qSlicerTableColumnPropertiesWidget::~qSlicerTableColumnPropertiesWidget()
{
  this->setCurrentNode(NULL);
}

//-----------------------------------------------------------------------------
void qSlicerTableColumnPropertiesWidget::setup()
{
  Q_D(qSlicerTableColumnPropertiesWidget);

  // This cannot be called by the constructor, because Slicer may not exist when the constructor is called
  d->TablesLogic = NULL;
  if (qSlicerApplication::application() != NULL && qSlicerApplication::application()->moduleManager() != NULL)
    {
    qSlicerAbstractCoreModule* tablesModule = qSlicerApplication::application()->moduleManager()->module( "Tables" );
    if (tablesModule != NULL)
      {
      d->TablesLogic = vtkSlicerTablesLogic::SafeDownCast(tablesModule->logic());
      }
    }
  if (d->TablesLogic == NULL)
    {
    qCritical("qSlicerTableColumnPropertiesWidget::setup: Tables module is not found, some manipulation features will not be available");
    }
  d->setupUi(this);

/*  connect( d->MarkupsFiducialNodeComboBox, SIGNAL( currentNodeChanged( vtkMRMLNode* ) ), this, SLOT( onMarkupsFiducialNodeChanged() ) );
  connect( d->MarkupsFiducialNodeComboBox, SIGNAL( nodeAddedByUser( vtkMRMLNode* ) ), this, SLOT( onMarkupsFiducialNodeAdded( vtkMRMLNode* ) ) );
  connect( d->MarkupsPlaceWidget, SIGNAL( activeMarkupsFiducialPlaceModeChanged(bool) ), this, SIGNAL( activeMarkupsFiducialPlaceModeChanged(bool) ) );


  d->MarkupsFiducialTableWidget->setColumnCount( FIDUCIAL_COLUMNS );
  d->MarkupsFiducialTableWidget->setHorizontalHeaderLabels( QStringList() << "Label" << "X" << "Y" << "Z" );
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  d->MarkupsFiducialTableWidget->horizontalHeader()->setResizeMode( QHeaderView::Stretch );
#else
  d->MarkupsFiducialTableWidget->horizontalHeader()->setSectionResizeMode( QHeaderView::Stretch );
#endif
  d->MarkupsFiducialTableWidget->setContextMenuPolicy( Qt::CustomContextMenu );
  d->MarkupsFiducialTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows); // only select rows rather than cells

  connect(d->MarkupsFiducialTableWidget, SIGNAL( customContextMenuRequested(const QPoint&) ), this, SLOT( onMarkupsFiducialTableContextMenu(const QPoint&) ) );
  connect(d->MarkupsFiducialTableWidget, SIGNAL( cellChanged( int, int ) ), this, SLOT( onMarkupsFiducialEdited( int, int ) ) );
  // listen for click on a markup
  connect(d->MarkupsFiducialTableWidget, SIGNAL(cellClicked(int,int)), this, SLOT(onMarkupsFiducialSelected(int,int)));
  // listen for the current cell selection change (happens when arrows are used to navigate)
  connect(d->MarkupsFiducialTableWidget, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(onMarkupsFiducialSelected(int, int)));
  */
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qSlicerTableColumnPropertiesWidget::currentNode() const
{
  Q_D(const qSlicerTableColumnPropertiesWidget);
  return d->CurrentTableNode;
}

//-----------------------------------------------------------------------------
void qSlicerTableColumnPropertiesWidget::setCurrentNode(vtkMRMLNode* currentNode)
{
  Q_D(qSlicerTableColumnPropertiesWidget);

  vtkMRMLTableNode* currentTableNode = vtkMRMLTableNode::SafeDownCast( currentNode );
  if (currentTableNode == d->CurrentTableNode)
    {
    // not changed
    return;
    }

  // Reconnect the appropriate nodes
  this->qvtkReconnect(d->CurrentTableNode, currentTableNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidget()));
  d->CurrentTableNode = currentTableNode;

  this->updateWidget();
}

//-----------------------------------------------------------------------------
void qSlicerTableColumnPropertiesWidget::setColumnProperty(QString propertyName, QString propertyValue)
{
  Q_D(qSlicerTableColumnPropertiesWidget);
  if (d->CurrentTableNode == NULL)
    {
    qWarning() << Q_FUNC_INFO << " failed: table node is not selected";
    return;
    }
  if (d->ColumnNames.empty())
    {
    qWarning() << Q_FUNC_INFO << " failed: table column names are not specified";
    return;
    }
  foreach(const QString& columnName, d->ColumnNames)
    {
      d->CurrentTableNode->SetColumnProperty(columnName.toLatin1().constData(), propertyName.toLatin1().constData(), propertyValue.toLatin1().constData());
    }
}

//-----------------------------------------------------------------------------
QString qSlicerTableColumnPropertiesWidget::columnProperty(QString propertyName) const
{
  Q_D(const qSlicerTableColumnPropertiesWidget);
  if (d->CurrentTableNode == NULL)
  {
    qWarning() << Q_FUNC_INFO << " failed: table node is not selected";
    return "";
  }
  if (d->ColumnNames.empty())
  {
    qWarning() << Q_FUNC_INFO << " failed: table column names are not specified";
    return "";
  }
  std::string commonPropertyValue = d->CurrentTableNode->GetColumnProperty(d->ColumnNames[0].toLatin1().constData(), propertyName.toLatin1().constData());
  foreach(const QString& columnName, d->ColumnNames)
    {
    std::string currentPropertyValue = d->CurrentTableNode->GetColumnProperty(columnName.toLatin1().constData(), propertyName.toLatin1().constData());
    if (currentPropertyValue != commonPropertyValue)
      {
      // not all column types are the same
      return QString();
      }
    }
  return commonPropertyValue.c_str();
}

//-----------------------------------------------------------------------------
void qSlicerTableColumnPropertiesWidget::updateWidget()
{
  Q_D(qSlicerTableColumnPropertiesWidget);

  if (d->TablesLogic == NULL || this->mrmlScene() == NULL)
    {
    qCritical("qSlicerTableColumnPropertiesWidget::updateWidget failed: Tables module logic or scene is invalid");
    }

  if ( d->CurrentTableNode == NULL)
    {
    /*d->MarkupsFiducialTableWidget->clear();
    d->MarkupsFiducialTableWidget->setRowCount( 0 );
    d->MarkupsFiducialTableWidget->setColumnCount( 0 );
    d->MarkupsPlaceWidget->setEnabled(false);
    emit updateFinished();*/
    return;
    }

/*  d->MarkupsPlaceWidget->setEnabled(true);

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

  emit updateFinished();*/
}

//------------------------------------------------------------------------------
void qSlicerTableColumnPropertiesWidget::setMRMLScene(vtkMRMLScene* scene)
{
  this->Superclass::setMRMLScene(scene);
  this->updateWidget();
}
