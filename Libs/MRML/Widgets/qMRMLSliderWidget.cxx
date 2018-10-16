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

  This file was originally developed by Johan Andruejol, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#include "qMRMLSliderWidget.h"

// Qt includes
#include <QHBoxLayout>
#include <QMenu>
//#include <QStyleOptionSlider>
#include <QToolButton>
#include <QWidgetAction>

// CTK includes
#include <ctkLinearValueProxy.h>
#include <ctkDoubleSlider.h>
#include <ctkUtils.h>

// MRML includes
#include <vtkMRMLNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLUnitNode.h>

// qMRML includes
#include <qMRMLSpinBox.h>

// STD includes
#include <cmath>

// VTK includes
#include <vtkCommand.h>

// --------------------------------------------------------------------------
class qMRMLSliderWidgetPrivate
{
  Q_DECLARE_PUBLIC(qMRMLSliderWidget);
protected:
  qMRMLSliderWidget* const q_ptr;
public:
  qMRMLSliderWidgetPrivate(qMRMLSliderWidget& object);
  ~qMRMLSliderWidgetPrivate();

  void setAndObserveSelectionNode();
  void updateValueProxy(vtkMRMLUnitNode* unitNode);

  QString Quantity;
  vtkMRMLScene* MRMLScene;
  vtkMRMLSelectionNode* SelectionNode;
  qMRMLSliderWidget::UnitAwareProperties Flags;
  ctkLinearValueProxy* Proxy;

  QToolButton* OptionsButton;
  qMRMLSpinBox* MinSpinBox;
  qMRMLSpinBox* MaxSpinBox;
};

// --------------------------------------------------------------------------
qMRMLSliderWidgetPrivate::qMRMLSliderWidgetPrivate(qMRMLSliderWidget& object)
  : q_ptr(&object)
{
  this->Quantity = "";
  this->MRMLScene = 0;
  this->SelectionNode = 0;
  this->Flags = qMRMLSliderWidget::Prefix | qMRMLSliderWidget::Suffix
    | qMRMLSliderWidget::Precision | qMRMLSliderWidget::Scaling;
  this->Proxy = new ctkLinearValueProxy;
}

// --------------------------------------------------------------------------
qMRMLSliderWidgetPrivate::~qMRMLSliderWidgetPrivate()
{
  delete this->Proxy;
}

// --------------------------------------------------------------------------
void qMRMLSliderWidgetPrivate::setAndObserveSelectionNode()
{
  Q_Q(qMRMLSliderWidget);

  vtkMRMLSelectionNode* selectionNode = 0;
  if (this->MRMLScene)
    {
    selectionNode = vtkMRMLSelectionNode::SafeDownCast(
      this->MRMLScene->GetNodeByID("vtkMRMLSelectionNodeSingleton"));
    }

  q->qvtkReconnect(this->SelectionNode, selectionNode,
    vtkMRMLSelectionNode::UnitModifiedEvent,
    q, SLOT(updateWidgetFromUnitNode()));
  this->SelectionNode = selectionNode;
  q->updateWidgetFromUnitNode();
}

// --------------------------------------------------------------------------
void qMRMLSliderWidgetPrivate::updateValueProxy(vtkMRMLUnitNode* unitNode)
{
  Q_Q(qMRMLSliderWidget);
  if (!unitNode)
    {
    q->setValueProxy(0);
    this->Proxy->setCoefficient(1.0);
    this->Proxy->setOffset(0.0);
    return;
    }

  q->setValueProxy(this->Proxy);
  this->Proxy->setOffset(unitNode->GetDisplayOffset());
  this->Proxy->setCoefficient(unitNode->GetDisplayCoefficient());
}

// --------------------------------------------------------------------------
// qMRMLSliderWidget

// --------------------------------------------------------------------------
qMRMLSliderWidget::qMRMLSliderWidget(QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new qMRMLSliderWidgetPrivate(*this))
{
  Q_D(qMRMLSliderWidget);
  QWidget* rangeWidget = new QWidget(this);
  QHBoxLayout* rangeLayout = new QHBoxLayout;
  rangeWidget->setLayout(rangeLayout);
  rangeLayout->setContentsMargins(0, 0, 0, 0);

  d->MinSpinBox = new qMRMLSpinBox(rangeWidget);
  d->MinSpinBox->setPrefix("Min: ");
  d->MinSpinBox->setRange(-1000000., 1000000.);
  d->MinSpinBox->setValue(this->minimum());
  connect(d->MinSpinBox, SIGNAL(valueChanged(double)),
    this, SLOT(updateRange()));
  rangeLayout->addWidget(d->MinSpinBox);

  d->MaxSpinBox = new qMRMLSpinBox(rangeWidget);
  d->MaxSpinBox->setPrefix("Max: ");
  d->MaxSpinBox->setRange(-1000000., 1000000.);
  d->MaxSpinBox->setValue(this->maximum());
  connect(d->MaxSpinBox, SIGNAL(valueChanged(double)),
    this, SLOT(updateRange()));
  rangeLayout->addWidget(d->MaxSpinBox);

  connect(this->slider(), SIGNAL(rangeChanged(double, double)),
    this, SLOT(updateSpinBoxRange(double, double)));

  QWidgetAction* rangeAction = new QWidgetAction(this);
  rangeAction->setDefaultWidget(rangeWidget);

  QMenu* optionsMenu = new QMenu(this);
  optionsMenu->addAction(rangeAction);

  d->OptionsButton = new QToolButton(this);
  d->OptionsButton->setIcon(QIcon(":Icons/SliceMoreOptions.png"));
  d->OptionsButton->setMenu(optionsMenu);
  d->OptionsButton->setPopupMode(QToolButton::InstantPopup);
  QHBoxLayout* hBoxLayout = qobject_cast<QHBoxLayout*>(this->layout());
  d->OptionsButton->setVisible(false);
  hBoxLayout->addWidget(d->OptionsButton);
}

// --------------------------------------------------------------------------
qMRMLSliderWidget::~qMRMLSliderWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLSliderWidget::setQuantity(const QString& quantity)
{
  Q_D(qMRMLSliderWidget);
  if (quantity == d->Quantity)
    {
    return;
    }

  d->Quantity = quantity;
  d->MinSpinBox->setQuantity(quantity);
  d->MaxSpinBox->setQuantity(quantity);
  this->updateWidgetFromUnitNode();
}

//-----------------------------------------------------------------------------
QString qMRMLSliderWidget::quantity()const
{
  Q_D(const qMRMLSliderWidget);
  return d->Quantity;
}

// --------------------------------------------------------------------------
vtkMRMLScene* qMRMLSliderWidget::mrmlScene()const
{
  Q_D(const qMRMLSliderWidget);
  return d->MRMLScene;
}

// --------------------------------------------------------------------------
void qMRMLSliderWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qMRMLSliderWidget);

  if (this->mrmlScene() == scene)
    {
    return;
    }

  d->MRMLScene = scene;
  d->MinSpinBox->setMRMLScene(scene);
  d->MaxSpinBox->setMRMLScene(scene);
  d->setAndObserveSelectionNode();
  this->setEnabled(scene != 0);
}

// --------------------------------------------------------------------------
qMRMLSliderWidget::UnitAwareProperties
qMRMLSliderWidget::unitAwareProperties()const
{
  Q_D(const qMRMLSliderWidget);
  return d->Flags;
}

// --------------------------------------------------------------------------
void qMRMLSliderWidget::setUnitAwareProperties(UnitAwareProperties newFlags)
{
  Q_D(qMRMLSliderWidget);
  if (newFlags == d->Flags)
    {
    return;
    }

  d->Flags = newFlags;
}

// --------------------------------------------------------------------------
void qMRMLSliderWidget::updateWidgetFromUnitNode()
{
  Q_D(qMRMLSliderWidget);

  if (d->SelectionNode)
    {
    vtkMRMLUnitNode* unitNode =
      vtkMRMLUnitNode::SafeDownCast(d->MRMLScene->GetNodeByID(
        d->SelectionNode->GetUnitNodeID(d->Quantity.toLatin1())));

    if (unitNode)
      {
      if (d->Flags.testFlag(qMRMLSliderWidget::Precision))
        {
        // setDecimals overwrites values therefore it is important
        // to call it only when it is necessary (without this check,
        // for example a setValue call may be ineffective if the min/max
        // value is changing at the same time)
        if (this->decimals()!=unitNode->GetPrecision())
          {
          this->setDecimals(unitNode->GetPrecision());
          }
        }
      if (d->Flags.testFlag(qMRMLSliderWidget::Prefix))
        {
        this->setPrefix(unitNode->GetPrefix());
        }
      if (d->Flags.testFlag(qMRMLSliderWidget::Suffix))
        {
        this->setSuffix(unitNode->GetSuffix());
        }
      if (d->Flags.testFlag(qMRMLSliderWidget::MinimumValue))
        {
        this->setMinimum(unitNode->GetMinimumValue());
        }
      if (d->Flags.testFlag(qMRMLSliderWidget::MaximumValue))
        {
        this->setMaximum(unitNode->GetMaximumValue());
        }
      if (d->Flags.testFlag(qMRMLSliderWidget::Scaling))
        {
        d->updateValueProxy(unitNode);
        }
      if (d->Flags.testFlag(qMRMLSliderWidget::Precision))
        {
        double range = this->maximum() - this->minimum();
        if (d->Flags.testFlag(qMRMLSliderWidget::Scaling))
          {
          range = unitNode->GetDisplayValueFromValue(this->maximum()) -
                  unitNode->GetDisplayValueFromValue(this->minimum());
          }
        double powerOfTen = ctk::closestPowerOfTen(range);
        if (powerOfTen != 0.)
          {
          this->setSingleStep(powerOfTen / 100);
          }
        }
      }
    }
}

// --------------------------------------------------------------------------
void qMRMLSliderWidget::setMinimum(double newMinimumValue)
{
  this->Superclass::setMinimum(newMinimumValue);
  if (this->unitAwareProperties().testFlag(qMRMLSliderWidget::Precision))
    {
    this->updateWidgetFromUnitNode();
    }
}

// --------------------------------------------------------------------------
void qMRMLSliderWidget::setMaximum(double newMaximumValue)
{
  this->Superclass::setMaximum(newMaximumValue);
  if (this->unitAwareProperties().testFlag(qMRMLSliderWidget::Precision))
    {
    this->updateWidgetFromUnitNode();
    }
}

// --------------------------------------------------------------------------
void qMRMLSliderWidget::setRange(double newMinimumValue, double newMaximumValue)
{
  this->Superclass::setRange(newMinimumValue, newMaximumValue);
  if (this->unitAwareProperties().testFlag(qMRMLSliderWidget::Precision))
    {
    this->updateWidgetFromUnitNode();
    }
}

// --------------------------------------------------------------------------
void qMRMLSliderWidget::updateSpinBoxRange(double min, double max)
{
  Q_D(qMRMLSliderWidget);
  // We must set the values at the same time and update the pipeline
  // when the MinSpinBox is set but not the MaxSpinBox. This could generate
  // infinite loop
  bool minSpinBoxBlocked = d->MinSpinBox->blockSignals(true);
  bool maxSpinBoxBlocked = d->MaxSpinBox->blockSignals(true);
  d->MinSpinBox->setValue(min);
  d->MaxSpinBox->setValue(max);
  d->MinSpinBox->blockSignals(minSpinBoxBlocked);
  d->MaxSpinBox->blockSignals(maxSpinBoxBlocked);
  this->updateRange();
}

// --------------------------------------------------------------------------
void qMRMLSliderWidget::updateRange()
{
  Q_D(qMRMLSliderWidget);
  this->setRange(d->MinSpinBox->value(),
    d->MaxSpinBox->value());
}
