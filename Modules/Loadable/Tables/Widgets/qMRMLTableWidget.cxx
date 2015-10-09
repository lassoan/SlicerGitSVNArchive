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

// FooBar Widgets includes
#include "qMRMLTableWidget.h"

#include <QtGui>


//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_CreateModels
class qMRMLTableWidgetPrivate
  : public Ui_qMRMLTableWidget
{
  Q_DECLARE_PUBLIC(qMRMLTableWidget);
protected:
  qMRMLTableWidget* const q_ptr;

public:
  qMRMLTableWidgetPrivate( qMRMLTableWidget& object);
  ~qMRMLTableWidgetPrivate();
  virtual void setupUi(qMRMLTableWidget*);
};

// --------------------------------------------------------------------------
qMRMLTableWidgetPrivate
::qMRMLTableWidgetPrivate( qMRMLTableWidget& object) : q_ptr(&object)
{
}

qMRMLTableWidgetPrivate
::~qMRMLTableWidgetPrivate()
{
}


// --------------------------------------------------------------------------
void qMRMLTableWidgetPrivate
::setupUi(qMRMLTableWidget* widget)
{
  this->Ui_qMRMLTableWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qMRMLTableWidget methods

//-----------------------------------------------------------------------------
qMRMLTableWidget
::qMRMLTableWidget(QWidget* parentWidget) : Superclass( parentWidget ) , d_ptr( new qMRMLTableWidgetPrivate(*this) )
{
  this->MetricsTableNode = NULL;
  this->setup();
}


qMRMLTableWidget
::~qMRMLTableWidget()
{
}


void qMRMLTableWidget
::setup()
{
  Q_D(qMRMLTableWidget);

  d->setupUi(this);

  connect( d->MetricsTableNodeComboBox, SIGNAL( currentNodeChanged( vtkMRMLNode* ) ), this, SLOT( onMetricsTableNodeChanged( vtkMRMLNode* ) ) );

  connect( d->ClipboardButton, SIGNAL( clicked() ), this, SLOT( onClipboardButtonClicked() ) );
  d->ClipboardButton->setIcon( QIcon( ":/Icons/Small/SlicerEditCopy.png" ) );

  this->updateWidget();
}


vtkMRMLTableNode* qMRMLTableWidget
::getMetricsTableNode()
{
  Q_D(qMRMLTableWidget);

  return this->MetricsTableNode;
}


void qMRMLTableWidget
::setMetricsTableNode( vtkMRMLNode* newMetricsTableNode )
{
  Q_D(qMRMLTableWidget);

  d->MetricsTableNodeComboBox->setCurrentNode( newMetricsTableNode );
  // If it is a new table node, then the onTransformBufferNodeChanged will be called automatically
}


vtkMRMLTableNode* qMRMLTableWidget
::addMetricsTableNode()
{
  Q_D(qMRMLTableWidget);

  return vtkMRMLTableNode::SafeDownCast( d->MetricsTableNodeComboBox->addNode() ); // Automatically calls "onMetricsTableNodeChanged" function
}


void qMRMLTableWidget
::onMetricsTableNodeChanged( vtkMRMLNode* newMetricsTableNode )
{
  Q_D(qMRMLTableWidget);

  this->qvtkDisconnectAll();

  this->MetricsTableNode = vtkMRMLTableNode::SafeDownCast( newMetricsTableNode );

  this->qvtkConnect( this->MetricsTableNode, vtkCommand::ModifiedEvent, this, SLOT( onMetricsTableNodeModified() ) );

  this->updateWidget();

  emit metricsTableNodeChanged( this->MetricsTableNode );
}


void qMRMLTableWidget
::onMetricsTableNodeModified()
{
  this->updateWidget();
  emit metricsTableNodeModified(); // This should allows parent widgets to update themselves
}


void qMRMLTableWidget
::onClipboardButtonClicked()
{
  Q_D( qMRMLTableWidget );

  // Grab all of the contents from whatever is currently on the metrics table
  QString clipString = QString( "" );

  for ( int i = 0; i < this->MetricsTableNode->GetTable()->GetNumberOfRows(); i++ )
  {
    clipString.append( this->MetricsTableNode->GetTable()->GetValueByName( i, "TransformName" ).ToString() );
    clipString.append( " " );
    clipString.append( this->MetricsTableNode->GetTable()->GetValueByName( i, "MetricName" ).ToString() );
    clipString.append( " (" );
    clipString.append( this->MetricsTableNode->GetTable()->GetValueByName( i, "MetricUnit" ).ToString() );
    clipString.append( ") " );

    clipString.append( "\t" );
    clipString.append( this->MetricsTableNode->GetTable()->GetValueByName( i, "MetricValue" ).ToString() );
    clipString.append( "\n" );
  }

  QApplication::clipboard()->setText( clipString );
}


void qMRMLTableWidget
::updateWidget()
{
  Q_D(qMRMLTableWidget);

  d->MetricsTableNodeComboBox->setCurrentNode( this->MetricsTableNode );

  // Set up the table
  d->MetricsTable->clear();
  d->MetricsTable->setRowCount( 0 );
  d->MetricsTable->setColumnCount( 0 );

  if ( this->MetricsTableNode == NULL )
  {
    return;
  }

  QStringList MetricsTableHeaders;
  MetricsTableHeaders << "Metric" << "Value";
  d->MetricsTable->setRowCount( this->MetricsTableNode->GetTable()->GetNumberOfRows() );
  d->MetricsTable->setColumnCount( 2 );
  d->MetricsTable->setHorizontalHeaderLabels( MetricsTableHeaders );
  d->MetricsTable->horizontalHeader()->setResizeMode( QHeaderView::Stretch );

  // Add the computed values to the table
  for ( int i = 0; i < this->MetricsTableNode->GetTable()->GetNumberOfRows(); i++ )
  {
    QString nameString;
    nameString.append( this->MetricsTableNode->GetTable()->GetValueByName( i, "TransformName" ).ToString() );
    nameString.append( " " );
    nameString.append( this->MetricsTableNode->GetTable()->GetValueByName( i, "MetricName" ).ToString() );
    QTableWidgetItem* nameItem = new QTableWidgetItem( nameString );
    d->MetricsTable->setItem( i, 0, nameItem );

    QString valueString;
    valueString.append( this->MetricsTableNode->GetTable()->GetValueByName( i, "MetricValue" ).ToString() );
    valueString.append( " " );
    valueString.append( this->MetricsTableNode->GetTable()->GetValueByName( i, "MetricUnit" ).ToString() );
    QTableWidgetItem* valueItem = new QTableWidgetItem( valueString );
    d->MetricsTable->setItem( i, 1, valueItem );
  }
}
