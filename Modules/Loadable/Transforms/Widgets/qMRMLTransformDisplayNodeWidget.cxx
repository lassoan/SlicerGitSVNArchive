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

  QObject::connect(this->Visible2dCheckBox, SIGNAL(toggled(bool)), q, SLOT(setVisible2d(bool)));
  QObject::connect(this->Visible3dCheckBox, SIGNAL(toggled(bool)), q, SLOT(setVisible3d(bool)));

  QObject::connect(this->RegionNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), q, SLOT(regionNodeChanged(vtkMRMLNode*)));

  QObject::connect(this->GlyphToggle, SIGNAL(toggled(bool)), q, SLOT(setGlyphVisualizationMode(bool)));
  QObject::connect(this->GridToggle, SIGNAL(toggled(bool)), q, SLOT(setGridVisualizationMode(bool)));
  QObject::connect(this->ContourToggle, SIGNAL(toggled(bool)), q, SLOT(setContourVisualizationMode(bool)));

  // Glyph Parameters
  QObject::connect(this->GlyphSpacingMm, SIGNAL(valueChanged(double)), q, SLOT(setGlyphSpacingMm(double)));
  QObject::connect(this->GlyphMaxNumberOfPoints, SIGNAL(valueChanged(double)), q, SLOT(setGlyphMaxNumberOfPoints(double)));
  QObject::connect(this->GlyphScalePercent, SIGNAL(valueChanged(double)), q, SLOT(setGlyphScalePercent(double)));
  QObject::connect(this->GlyphDisplayRangeMm, SIGNAL(valuesChanged(double, double)), q, SLOT(setGlyphDisplayRangeMm(double, double)));
  QObject::connect(this->GlyphGenerateRandomSeedButton, SIGNAL(clicked()), q, SLOT(generateGlyphRandomSeed()));
  QObject::connect(this->GlyphRandomSeed, SIGNAL(valueChanged(int)), q, SLOT(setGlyphRandomSeed(int)));
  QObject::connect(this->GlyphTypeComboBox, SIGNAL(currentIndexChanged(int)), q, SLOT(setGlyphType(int)));
  // 3D Glyph Parameters
  QObject::connect(this->GlyphScaleDirectionalCheckBox, SIGNAL(toggled(bool)), q, SLOT(setGlyphScaleDirectional(bool)));
  QObject::connect(this->GlyphDiameterMm, SIGNAL(valueChanged(double)), q, SLOT(setGlyphDiameterMm(double)));
  QObject::connect(this->GlyphDiameterPercent, SIGNAL(valueChanged(double)), q, SLOT(setGlyphDiameterPercent(double)));
  QObject::connect(this->GlyphTipLengthPercent, SIGNAL(valueChanged(double)), q, SLOT(setGlyphTipLengthPercent(double)));
  QObject::connect(this->GlyphShaftDiameterPercent, SIGNAL(valueChanged(double)), q, SLOT(setGlyphShaftDiameterPercent(double)));
  QObject::connect(this->GlyphResolution, SIGNAL(valueChanged(double)), q, SLOT(setGlyphResolution(double)));

  // Grid Parameters
  QObject::connect(this->GridScalePercent, SIGNAL(valueChanged(double)), q, SLOT(setGridScalePercent(double)));
  QObject::connect(this->GridSpacingMm, SIGNAL(valueChanged(double)), q, SLOT(setGridSpacingMm(double)));
  QObject::connect(this->GridLineDiameterMm, SIGNAL(valueChanged(double)), q, SLOT(setGridLineDiameterMm(double)));

  // Contour Parameters
  QRegExp rx("^(([0-9]+(.[0-9]+)?)[ ]?)*([0-9]+(.[0-9]+)?)[ ]?$");
  this->ContourLevelsMm->setValidator(new QRegExpValidator(rx,q));
  QObject::connect(this->ContourLevelsMm, SIGNAL(textChanged(QString)), q, SLOT(setContourLevelsMm(QString)));
  QObject::connect(this->ContourResolutionMm, SIGNAL(valueChanged(double)), q, SLOT(setContourResolutionMm(double)));

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

  d->Visible2dCheckBox->setChecked(d->TransformDisplayNode->GetSliceIntersectionVisibility());
  d->Visible3dCheckBox->setChecked(d->TransformDisplayNode->GetVisibility());

  switch (d->TransformDisplayNode->GetVisualizationMode())
  {
    case vtkMRMLTransformDisplayNode::VIS_MODE_GLYPH: d->GlyphToggle->setChecked(true); break;
    case vtkMRMLTransformDisplayNode::VIS_MODE_GRID: d->GridToggle->setChecked(true); break;
    case vtkMRMLTransformDisplayNode::VIS_MODE_CONTOUR: d->ContourToggle->setChecked(true); break;
  }

  d->RegionNodeComboBox->setCurrentNode(d->TransformDisplayNode->GetRegionNode());

  // Update Visualization Parameters
  // Glyph Parameters
  d->GlyphSpacingMm->setValue(d->TransformDisplayNode->GetGlyphSpacingMm());
  d->GlyphMaxNumberOfPoints->setValue(d->TransformDisplayNode->GetGlyphMaxNumberOfPoints());
  d->GlyphRandomSeed->setValue(d->TransformDisplayNode->GetGlyphRandomSeed());
  d->GlyphScalePercent->setValue(d->TransformDisplayNode->GetGlyphScalePercent());
  d->GlyphDisplayRangeMm->setMaximumValue(d->TransformDisplayNode->GetGlyphDisplayRangeMaxMm());
  d->GlyphDisplayRangeMm->setMinimumValue(d->TransformDisplayNode->GetGlyphDisplayRangeMinMm());
  d->GlyphTypeComboBox->setCurrentIndex(d->TransformDisplayNode->GetGlyphType());
  // 3D Glyph Parameters
  d->GlyphScaleDirectionalCheckBox->setChecked(d->TransformDisplayNode->GetGlyphScaleDirectional());
  d->GlyphDiameterMm->setValue(d->TransformDisplayNode->GetGlyphDiameterMm());
  d->GlyphDiameterPercent->setValue(d->TransformDisplayNode->GetGlyphDiameterPercent());
  d->GlyphTipLengthPercent->setValue(d->TransformDisplayNode->GetGlyphTipLengthPercent());
  d->GlyphShaftDiameterPercent->setValue(d->TransformDisplayNode->GetGlyphShaftDiameterPercent());
  d->GlyphResolution->setValue(d->TransformDisplayNode->GetGlyphResolution());

  // Grid Parameters
  d->GridScalePercent->setValue(d->TransformDisplayNode->GetGridScalePercent());
  d->GridSpacingMm->setValue(d->TransformDisplayNode->GetGridSpacingMm());
  d->GridLineDiameterMm->setValue(d->TransformDisplayNode->GetGridLineDiameterMm());

  // Contour Parameters

  // Only update the text in the editbox if it is changed (to not interfere with editing of the values)
  std::vector<double> levelsInWidget=vtkMRMLTransformDisplayNode::ConvertContourLevelsFromString(d->ContourLevelsMm->text().toLatin1());
  std::vector<double> levelsInMRML;
  d->TransformDisplayNode->GetContourLevelsMm(levelsInMRML);
  if (!vtkMRMLTransformDisplayNode::ContourLevelsEqual(levelsInWidget,levelsInMRML))
  {
    d->ContourLevelsMm->setText(QLatin1String(d->TransformDisplayNode->GetContourLevelsMmAsString().c_str()));
  }

  d->ContourResolutionMm->setValue(d->TransformDisplayNode->GetContourResolutionMm());

  this->updateLabels();
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::updateLabels()
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  // update button states and show/hide static labels

}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::updateGlyphSourceOptions(int glyphType)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  bool glyphScaleDirectional=true;
  if (d->TransformDisplayNode)
  {
    glyphScaleDirectional=d->TransformDisplayNode->GetGlyphScaleDirectional();
  }

  if (glyphType == vtkMRMLTransformDisplayNode::GLYPH_TYPE_ARROW)
  {
    d->GlyphDiameterMmLabel->setVisible(glyphScaleDirectional);
    d->GlyphDiameterMm->setVisible(glyphScaleDirectional);
    d->GlyphDiameterPercentLabel->setVisible(!glyphScaleDirectional);
    d->GlyphDiameterPercent->setVisible(!glyphScaleDirectional);
    d->GlyphShaftDiameterLabel->setVisible(true);
    d->GlyphShaftDiameterPercent->setVisible(true);
    d->GlyphTipLengthLabel->setVisible(true);
    d->GlyphTipLengthPercent->setVisible(true);
  }
  else if (glyphType == vtkMRMLTransformDisplayNode::GLYPH_TYPE_CONE)
  {
    d->GlyphDiameterMmLabel->setVisible(glyphScaleDirectional);
    d->GlyphDiameterMm->setVisible(glyphScaleDirectional);
    d->GlyphDiameterPercentLabel->setVisible(!glyphScaleDirectional);
    d->GlyphDiameterPercent->setVisible(!glyphScaleDirectional);
    d->GlyphShaftDiameterLabel->setVisible(false);
    d->GlyphShaftDiameterPercent->setVisible(false);
    d->GlyphTipLengthLabel->setVisible(true);
    d->GlyphTipLengthPercent->setVisible(true);
  }
  else if (glyphType == vtkMRMLTransformDisplayNode::GLYPH_TYPE_SPHERE)
  {
    d->GlyphDiameterMmLabel->setVisible(false);
    d->GlyphDiameterMm->setVisible(false);
    d->GlyphDiameterPercentLabel->setVisible(false);
    d->GlyphDiameterPercent->setVisible(false);
    d->GlyphShaftDiameterLabel->setVisible(false);
    d->GlyphShaftDiameterPercent->setVisible(false);
    d->GlyphTipLengthLabel->setVisible(false);
    d->GlyphTipLengthPercent->setVisible(false);
  }
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::regionNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
    {
    return;
    }
  d->TransformDisplayNode->SetAndObserveRegionNode(node);
  this->updateLabels();
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphSpacingMm(double spacing)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphSpacingMm(spacing);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphMaxNumberOfPoints(double pointMax)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
    {
    return;
    }
  d->TransformDisplayNode->SetGlyphMaxNumberOfPoints(pointMax);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::generateGlyphRandomSeed()
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphRandomSeed(rand());
  d->GlyphRandomSeed->setValue(d->TransformDisplayNode->GetGlyphRandomSeed());
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphRandomSeed(int seed)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphRandomSeed(seed);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphScalePercent(double scale)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphScalePercent(scale);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphDisplayRangeMm(double min, double max)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphDisplayRangeMinMm(min);
  d->TransformDisplayNode->SetGlyphDisplayRangeMaxMm(max);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphType(int glyphType)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphType(glyphType);
  this->updateGlyphSourceOptions(glyphType);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphScaleDirectional(bool state)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphScaleDirectional(state);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphTipLengthPercent(double length)
{
  Q_D(qMRMLTransformDisplayNodeWidget);

  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphTipLengthPercent(length);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphDiameterMm(double diameterMm)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphDiameterMm(diameterMm);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphDiameterPercent(double diameterPercent)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphDiameterPercent(diameterPercent);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphShaftDiameterPercent(double diameterPercent)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphShaftDiameterPercent(diameterPercent);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphResolution(double resolution)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGlyphResolution(resolution);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGridScalePercent(double scale)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGridScalePercent(scale);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGridSpacingMm(double spacing)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGridSpacingMm(spacing);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGridLineDiameterMm(double diameterMm)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetGridLineDiameterMm(diameterMm);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setContourLevelsMm(QString values_str)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetContourLevelsMmFromString(values_str.toLatin1());
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setContourResolutionMm(double resolutionMm)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetContourResolutionMm(resolutionMm);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGlyphVisualizationMode(bool activate)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!activate)
    {
    return;
    }
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetVisualizationMode(vtkMRMLTransformDisplayNode::VIS_MODE_GLYPH);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setGridVisualizationMode(bool activate)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!activate)
    {
    return;
    }
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetVisualizationMode(vtkMRMLTransformDisplayNode::VIS_MODE_GRID);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setContourVisualizationMode(bool activate)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!activate)
    {
    return;
    }
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetVisualizationMode(vtkMRMLTransformDisplayNode::VIS_MODE_CONTOUR);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setVisible2d(bool visible)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetSliceIntersectionVisibility(visible);
}

//-----------------------------------------------------------------------------
void qMRMLTransformDisplayNodeWidget::setVisible3d(bool visible)
{
  Q_D(qMRMLTransformDisplayNodeWidget);
  if (!d->TransformDisplayNode)
  {
    return;
  }
  d->TransformDisplayNode->SetVisibility(visible);
}
