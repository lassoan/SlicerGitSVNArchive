/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Brigham and Women's Hospital

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Laurent Chauvin, Brigham and Women's
  Hospital. The project was supported by grants 5P01CA067165,
  5R01CA124377, 5R01CA138586, 2R44DE019322, 7R01CA124377,
  5R42CA137886, 5R42CA137886
  It was then updated for the Transforms module by Nicole Aucoin, BWH.

==============================================================================*/

// qMRML includes
#include "qMRMLTransformDisplayNodeWidget.h"
#include "ui_qMRMLTransformDisplayNodeWidget.h"

// MRML includes
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLTransformDisplayNode.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Transforms
class qMRMLTransformDisplayNodeWidgetPrivate
  : public Ui_qMRMLTransformDisplayNodeWidget
{
  Q_DECLARE_PUBLIC(qMRMLTransformDisplayNodeWidget);
protected:
  qMRMLTransformDisplayNodeWidget* const q_ptr;
public:
  qMRMLTransformDisplayNodeWidgetPrivate(qMRMLTransformDisplayNodeWidget& object);
  void init();

  vtkMRMLTransformDisplayNode* TransformDisplayNode;
};

//-----------------------------------------------------------------------------
// qMRMLTransformDisplayNodeWidgetPrivate methods

//-----------------------------------------------------------------------------
qMRMLTransformDisplayNodeWidgetPrivate
::qMRMLTransformDisplayNodeWidgetPrivate(qMRMLTransformDisplayNodeWidget& object)
  : q_ptr(&object)
{
  this->TransformDisplayNode = NULL;
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidgetPrivate
::init()
{
  Q_Q(qMRMLTransformDisplayNodeWidget);
  this->setupUi(q);

  QObject::connect(this->InputReferenceComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(referenceVolumeChanged(vtkMRMLNode*)));

  // Glyph Parameters
  QObject::connect(this->InputGlyphPointMax, SIGNAL(valueChanged(double)), q, SLOT(setGlyphPointMax(double)));
  QObject::connect(this->InputGlyphScale, SIGNAL(valueChanged(double)), q, SLOT(setGlyphScale(double)));
  QObject::connect(this->InputGlyphThreshold, SIGNAL(valuesChanged(double, double)), q, SLOT(setGlyphThreshold(double, double)));
  QObject::connect(this->GenerateSeedButton, SIGNAL(clicked()), q, SLOT(setSeed()));
  QObject::connect(this->InputGlyphSeed, SIGNAL(valueChanged(int)), q, SLOT(setGlyphSeed(int)));
  QObject::connect(this->GlyphSourceComboBox, SIGNAL(currentIndexChanged(int)), q, SLOT(setGlyphSourceOption(int)));
  // Arrow Parameters
  QObject::connect(this->InputGlyphArrowScaleDirectional, SIGNAL(toggled(bool)), q, SLOT(setGlyphArrowScaleDirectional(bool)));
  QObject::connect(this->InputGlyphArrowScaleIsotropic, SIGNAL(toggled(bool)), q, SLOT(setGlyphArrowScaleIsotropic(bool)));
  QObject::connect(this->InputGlyphArrowTipLength, SIGNAL(valueChanged(double)), q, SLOT(setGlyphArrowTipLength(double)));
  QObject::connect(this->InputGlyphArrowTipRadius, SIGNAL(valueChanged(double)), q, SLOT(setGlyphArrowTipRadius(double)));
  QObject::connect(this->InputGlyphArrowShaftRadius, SIGNAL(valueChanged(double)), q, SLOT(setGlyphArrowShaftRadius(double)));
  QObject::connect(this->InputGlyphArrowResolution, SIGNAL(valueChanged(double)), q, SLOT(setGlyphArrowResolution(double)));
  // Cone Parameters
  QObject::connect(this->InputGlyphConeScaleDirectional, SIGNAL(toggled(bool)), q, SLOT(setGlyphConeScaleDirectional(bool)));
  QObject::connect(this->InputGlyphConeScaleIsotropic, SIGNAL(toggled(bool)), q, SLOT(setGlyphConeScaleIsotropic(bool)));
  QObject::connect(this->InputGlyphConeHeight, SIGNAL(valueChanged(double)), q, SLOT(setGlyphConeHeight(double)));
  QObject::connect(this->InputGlyphConeRadius, SIGNAL(valueChanged(double)), q, SLOT(setGlyphConeRadius(double)));
  QObject::connect(this->InputGlyphConeResolution, SIGNAL(valueChanged(double)), q, SLOT(setGlyphConeResolution(double)));
  // Sphere Parameters
  QObject::connect(this->InputGlyphSphereResolution, SIGNAL(valueChanged(double)), q, SLOT(setGlyphSphereResolution(double)));

  // Grid Parameters
  QObject::connect(this->InputGridScale, SIGNAL(valueChanged(double)), q, SLOT(setGridScale(double)));
  QObject::connect(this->InputGridSpacing, SIGNAL(valueChanged(double)), q, SLOT(setGridSpacingMM(double)));

  // Block Parameters
  QObject::connect(this->InputBlockScale, SIGNAL(valueChanged(double)), q, SLOT(setBlockScale(double)));
  QObject::connect(this->InputBlockDisplacementCheck, SIGNAL(stateChanged(int)), q, SLOT(setBlockDisplacementCheck(int)));

  // Contour Parameters
  QRegExp rx("^(([0-9]+(.[0-9]+)?),+)*([0-9]+(.[0-9]+)?)$");
  this->InputContourValues->setValidator(new QRegExpValidator(rx,q));
  QObject::connect(this->InputContourValues, SIGNAL(textChanged(QString)), q, SLOT(setContourValues(QString)));
  //QObject::connect(this->InputContourNumber, SIGNAL(valueChanged(double)), q, SLOT(setContourNumber(double)));
  //QObject::connect(this->InputContourRange, SIGNAL(valuesChanged(double, double)), q, SLOT(setContourRange(double, double)));
  QObject::connect(this->InputContourDecimation, SIGNAL(valueChanged(double)), q, SLOT(setContourDecimation(double)));

  // Glyph Slice Parameters
  QObject::connect(this->GlyphSliceComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(setGlyphSliceNode(vtkMRMLNode*)));
  QObject::connect(this->InputGlyphSlicePointMax, SIGNAL(valueChanged(double)), q, SLOT(setGlyphSlicePointMax(double)));
  QObject::connect(this->InputGlyphSliceThreshold, SIGNAL(valuesChanged(double, double)), q, SLOT(setGlyphSliceThreshold(double, double)));
  QObject::connect(this->InputGlyphSliceScale, SIGNAL(valueChanged(double)), q, SLOT(setGlyphSliceScale(double)));
  QObject::connect(this->InputGlyphSliceSeed, SIGNAL(valueChanged(int)), q, SLOT(setGlyphSliceSeed(int)));
  QObject::connect(this->GenerateSeedButton2, SIGNAL(clicked()), q, SLOT(setSeed2()));

  // Grid Slice Parameters
  //QObject::connect(this->GridSliceComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(setGridSliceNode(vtkMRMLNode*)));
  QObject::connect(this->InputGridSliceScale, SIGNAL(valueChanged(double)), q, SLOT(setGridSliceScale(double)));
  QObject::connect(this->InputGridSliceSpacing, SIGNAL(valueChanged(double)), q, SLOT(setGridSliceSpacingMM(double)));

  q->updateWidgetFromDisplayNode();
}

//-----------------------------------------------------------------------------
// qMRMLTransformDisplayNodeWidget methods

//-----------------------------------------------------------------------------
qMRMLTransformDisplayNodeWidget
::qMRMLTransformDisplayNodeWidget(QWidget *newParent) :
    Superclass(newParent)
  , d_ptr(new qMRMLTransformDisplayNodeWidgetPrivate(*this))
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  d->init();
}

//-----------------------------------------------------------------------------
qMRMLTransformDisplayNodeWidget
::~qMRMLTransformDisplayNodeWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget
::setMRMLTransformNode(vtkMRMLNode* transformNode)
{
  setMRMLTransformNode(vtkMRMLTransformNode::SafeDownCast(transformNode));
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget
::setMRMLTransformNode(vtkMRMLTransformNode* transformNode)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  vtkMRMLTransformDisplayNode* displayNode = NULL;
  if (transformNode!=NULL)
  {
    displayNode=vtkMRMLTransformDisplayNode::SafeDownCast(transformNode->GetDisplayNode());
  }

  qvtkReconnect(d->TransformDisplayNode, displayNode, vtkCommand::ModifiedEvent,
                this, SLOT(updateWidgetFromDisplayNode()));

  d->TransformDisplayNode = displayNode;
  this->updateWidgetFromDisplayNode();
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget
::updateWidgetFromDisplayNode()
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  this->setEnabled(d->TransformDisplayNode != 0);

  if (!d->TransformDisplayNode)
    {
    return;
    }

  // Update widget if different from MRML node

  //d->InputReferenceComboBox->setCurrentNode(d->TransformDisplayNode->GetReferenceVolumeNode());

  // Update Visualization Parameters
  // Glyph Parameters
  d->InputGlyphPointMax->setValue(d->TransformDisplayNode->GetGlyphPointMax());
  d->InputGlyphSeed->setValue(d->TransformDisplayNode->GetGlyphSeed());
  d->InputGlyphScale->setValue(d->TransformDisplayNode->GetGlyphScale());
  d->InputGlyphThreshold->setMaximumValue(d->TransformDisplayNode->GetGlyphThresholdMax());
  d->InputGlyphThreshold->setMinimumValue(d->TransformDisplayNode->GetGlyphThresholdMin());
  d->GlyphSourceComboBox->setCurrentIndex(d->TransformDisplayNode->GetGlyphSourceOption());
  // Arrow Parameters
  d->InputGlyphArrowScaleDirectional->setChecked(d->TransformDisplayNode->GetGlyphArrowScaleDirectional());
  d->InputGlyphArrowScaleIsotropic->setChecked(d->TransformDisplayNode->GetGlyphArrowScaleIsotropic());
  d->InputGlyphArrowTipLength->setValue(d->TransformDisplayNode->GetGlyphArrowTipLength());
  d->InputGlyphArrowTipRadius->setValue(d->TransformDisplayNode->GetGlyphArrowTipRadius());
  d->InputGlyphArrowShaftRadius->setValue(d->TransformDisplayNode->GetGlyphArrowShaftRadius());
  d->InputGlyphArrowResolution->setValue(d->TransformDisplayNode->GetGlyphArrowResolution());
  // Cone Parameters
  d->InputGlyphConeScaleDirectional->setChecked(d->TransformDisplayNode->GetGlyphConeScaleDirectional());
  d->InputGlyphConeScaleIsotropic->setChecked(d->TransformDisplayNode->GetGlyphConeScaleIsotropic());
  d->InputGlyphConeHeight->setValue(d->TransformDisplayNode->GetGlyphConeHeight());
  d->InputGlyphConeRadius->setValue(d->TransformDisplayNode->GetGlyphConeRadius());
  d->InputGlyphConeResolution->setValue(d->TransformDisplayNode->GetGlyphConeResolution());
  // Sphere Parameters
  d->InputGlyphSphereResolution->setValue(d->TransformDisplayNode->GetGlyphSphereResolution());

  // Grid Parameters
  d->InputGridScale->setValue(d->TransformDisplayNode->GetGridScale());
  d->InputGridSpacing->setValue(d->TransformDisplayNode->GetGridSpacingMM());

  // Block Parameters
  d->InputBlockScale->setValue(d->TransformDisplayNode->GetBlockScale());
  d->InputBlockDisplacementCheck->setChecked(d->TransformDisplayNode->GetBlockDisplacementCheck());

  // Contour Parameters
  //d->InputContourNumber->setValue(d->TransformDisplayNode->GetContourNumber());
  //d->InputContourRange->setMaximumValue(d->TransformDisplayNode->GetContourMax());
  //d->InputContourRange->setMinimumValue(d->TransformDisplayNode->GetContourMin());
  d->InputContourDecimation->setValue(d->TransformDisplayNode->GetContourDecimation());

  d->InputGlyphSlicePointMax->setValue(d->TransformDisplayNode->GetGlyphSlicePointMax());
  d->InputGlyphSliceThreshold->setMaximumValue(d->TransformDisplayNode->GetGlyphSliceThresholdMax());
  d->InputGlyphSliceThreshold->setMinimumValue(d->TransformDisplayNode->GetGlyphSliceThresholdMin());
  d->InputGlyphSliceScale->setValue(d->TransformDisplayNode->GetGlyphSliceScale());
  d->InputGlyphSliceSeed->setValue(d->TransformDisplayNode->GetGlyphSliceSeed());

  // Grid Slice Parameters
  d->InputGridSliceScale->setValue(d->TransformDisplayNode->GetGridSliceScale());
  d->InputGridSliceSpacing->setValue(d->TransformDisplayNode->GetGridSliceSpacingMM());

  this->updateLabels();
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::updateLabels()
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  // update button states and show/hide static labels

}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::updateGlyphSourceOptions(int sourceOption)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (sourceOption == vtkMRMLTransformDisplayNode::GLYPH_ARROW_3D)
  {
    d->ArrowSourceOptions->setEnabled(true);
    d->ArrowSourceOptions->setVisible(true);
    d->ConeSourceOptions->setEnabled(false);
    d->ConeSourceOptions->setVisible(false);
    d->SphereSourceOptions->setEnabled(false);
    d->SphereSourceOptions->setVisible(false);
  }
  else if (sourceOption == vtkMRMLTransformDisplayNode::GLYPH_CONE_3D)
  {
    d->ArrowSourceOptions->setEnabled(false);
    d->ArrowSourceOptions->setVisible(false);
    d->ConeSourceOptions->setEnabled(true);
    d->ConeSourceOptions->setVisible(true);
    d->SphereSourceOptions->setEnabled(false);
    d->SphereSourceOptions->setVisible(false);
  }
  else if (sourceOption == vtkMRMLTransformDisplayNode::GLYPH_SPHERE_3D)
  {
    d->ArrowSourceOptions->setEnabled(false);
    d->ArrowSourceOptions->setVisible(false);
    d->ConeSourceOptions->setEnabled(false);
    d->ConeSourceOptions->setVisible(false);
    d->SphereSourceOptions->setEnabled(true);
    d->SphereSourceOptions->setVisible(true);
  }
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::referenceVolumeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
    {
    return;
    }

  d->TransformDisplayNode->SetAndObserveReferenceVolumeNode(node);
  this->updateLabels();
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphPointMax(double pointMax)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
    {
    return;
    }
  d->TransformDisplayNode->SetGlyphPointMax(pointMax);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setSeed()
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSeed(rand());
  d->InputGlyphSeed->setValue(d->TransformDisplayNode->GetGlyphSeed());
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphSeed(int seed)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSeed(seed);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphScale(double scale)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphScale(scale);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphThreshold(double min, double max)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphThresholdMin(min);
  d->TransformDisplayNode->SetGlyphThresholdMax(max);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphSourceOption(int option)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSourceOption(option);
  this->updateGlyphSourceOptions(option);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphArrowScaleDirectional(bool state)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphArrowScaleDirectional(state);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphArrowScaleIsotropic(bool state)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphArrowScaleIsotropic(state);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphArrowTipLength(double length)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphArrowTipLength(length);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphArrowTipRadius(double radius)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphArrowTipRadius(radius);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphArrowShaftRadius(double radius)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphArrowShaftRadius(radius);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphArrowResolution(double resolution)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphArrowResolution(resolution);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphConeScaleDirectional(bool state)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphConeScaleDirectional(state);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphConeScaleIsotropic(bool state)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphConeScaleIsotropic(state);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphConeHeight(double height)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphConeHeight(height);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphConeRadius(double radius)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphConeRadius(radius);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphConeResolution(double resolution)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphConeResolution(resolution);
}

//-----------------------------------------------------------------------------
// Sphere Parameters
//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphSphereResolution(double resolution)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSphereResolution(resolution);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGridScale(double scale)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGridScale(scale);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGridSpacingMM(double spacing)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGridSpacingMM(spacing);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setBlockScale(double scale)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetBlockScale(scale);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setBlockDisplacementCheck(int state)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetBlockDisplacementCheck(state);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setContourValues(QString values_str)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  QStringList values_strlist = values_str.split(",");
  QList<double> values_qlist;

  int valuesSize = values_strlist.size();

  for (int i=0; i<valuesSize; i++)
  {
    values_qlist.append(values_strlist[i].toDouble());
  }

  double* values_array = new double[valuesSize];

  for (int j=0; j<valuesSize; j++)
  {
    values_array[j] = values_qlist[j];
  }

  d->TransformDisplayNode->SetContourValues(values_array, valuesSize);
  d->TransformDisplayNode->SetContourNumber(valuesSize);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setContourDecimation(double reduction)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetContourDecimation(reduction);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphSlicePointMax(double pointMax)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSlicePointMax(pointMax);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphSliceThreshold(double min, double max)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSliceThresholdMin(min);
  d->TransformDisplayNode->SetGlyphSliceThresholdMax(max);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphSliceScale(double scale)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSliceScale(scale);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphSliceSeed(int seed)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSliceSeed(seed);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setSeed2()
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSliceSeed(rand());
  d->InputGlyphSliceSeed->setValue(d->TransformDisplayNode->GetGlyphSliceSeed());
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGridSliceScale(double scale)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGridSliceScale(scale);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGridSliceSpacingMM(double spacing)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGridSliceSpacingMM(spacing);
}
