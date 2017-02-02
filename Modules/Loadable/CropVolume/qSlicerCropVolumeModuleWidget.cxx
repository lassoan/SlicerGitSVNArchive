// Qt includes
#include <QDebug>
#include <QMessageBox>

// CTK includes
#include <ctkFlowLayout.h>

// SlicerQt includes
#include <qSlicerAbstractCoreModule.h>

// CropVolume includes
#include "qSlicerCropVolumeModuleWidget.h"
#include "ui_qSlicerCropVolumeModuleWidget.h"

// CropVolume Logic includes
#include <vtkSlicerCropVolumeLogic.h>

// qMRML includes
#include <qMRMLNodeFactory.h>

// MRMLAnnotation includes
#include <vtkMRMLAnnotationROINode.h>

// MRMLLogic includes
#include <vtkMRMLApplicationLogic.h>

#include <vtkNew.h>
#include <vtkMatrix4x4.h>


// MRML includes
#include <vtkMRMLCropVolumeParametersNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLTransformNode.h>


//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_CropVolume
class qSlicerCropVolumeModuleWidgetPrivate: public Ui_qSlicerCropVolumeModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerCropVolumeModuleWidget);
protected:
  qSlicerCropVolumeModuleWidget* const q_ptr;
public:

  qSlicerCropVolumeModuleWidgetPrivate(qSlicerCropVolumeModuleWidget& object);
  ~qSlicerCropVolumeModuleWidgetPrivate();

  vtkSlicerCropVolumeLogic* logic() const;

  void performROIVoxelGridAlignment();
  bool isInputVolumeLinearTransformed() const;
  void showUnsupportedTransVolumeVoxelCroppingDialog() const;

  vtkWeakPointer<vtkMRMLCropVolumeParametersNode> ParametersNode;
  vtkWeakPointer<vtkMRMLVolumeNode> InputVolumeNode;
  vtkWeakPointer<vtkMRMLAnnotationROINode> InputROINode;
};

//-----------------------------------------------------------------------------
// qSlicerCropVolumeModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerCropVolumeModuleWidgetPrivate::qSlicerCropVolumeModuleWidgetPrivate(qSlicerCropVolumeModuleWidget& object) : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
qSlicerCropVolumeModuleWidgetPrivate::~qSlicerCropVolumeModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerCropVolumeLogic* qSlicerCropVolumeModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerCropVolumeModuleWidget);
  return vtkSlicerCropVolumeLogic::SafeDownCast(q->logic());
}


//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidgetPrivate::showUnsupportedTransVolumeVoxelCroppingDialog() const
{
  QMessageBox::information(NULL,"Crop Volume","The selected volume is under a transform. Voxel based cropping is only supported for non transformed volumes!");
}
//-----------------------------------------------------------------------------
bool qSlicerCropVolumeModuleWidgetPrivate::isInputVolumeLinearTransformed() const
{
  Q_ASSERT(this->InputVolumeComboBox);

  vtkMRMLVolumeNode* inputVolume = vtkMRMLVolumeNode::SafeDownCast(this->InputVolumeComboBox->currentNode());
  if(!inputVolume)
    {
    return false;
    }
  vtkMRMLTransformNode* volTransform = inputVolume->GetParentTransformNode();
  if(!volTransform)
    {
    return false;
    }
  // we ignore non-linear transforms
  return volTransform->IsTransformToWorldLinear();
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidgetPrivate::performROIVoxelGridAlignment()
{
  Q_ASSERT(this->InputVolumeComboBox);
  Q_ASSERT(this->InputROIComboBox);

  vtkSmartPointer<vtkMRMLVolumeNode> inputVolume = vtkMRMLVolumeNode::SafeDownCast(this->InputVolumeComboBox->currentNode());
  vtkSmartPointer<vtkMRMLAnnotationROINode> inputROI = vtkMRMLAnnotationROINode::SafeDownCast(this->InputROIComboBox->currentNode());

  if (!inputVolume || !inputROI || this->InterpolationEnabledCheckBox->isChecked())
    {
    return;
    }

  vtkNew<vtkMatrix4x4> volRotMat;
  bool volumeTilted = vtkSlicerCropVolumeLogic::IsVolumeTiltedInRAS(inputVolume,volRotMat.GetPointer());

  vtkMRMLTransformNode* roiTransform = inputROI->GetParentTransformNode();
  if(roiTransform && roiTransform->IsTransformToWorldLinear())
    {
    vtkNew<vtkMatrix4x4> parentTransform;
    roiTransform->GetMatrixTransformToWorld(parentTransform.GetPointer());
    bool same = true;
    for(int i=0; i < 4 && same; ++i)
      {
      for(int j=0; j < 4; ++j)
        {
        if(parentTransform->GetElement(i,j) != volRotMat->GetElement(i,j))
          {
          same = false;
          break;
          }
        }
      }
    if(!same)
      {
      QString message = "The selected ROI has a transform which is neither an identity transform"
        " nor a Crop Volume voxelgrid alignment transform for the selected volume. In order to perform"
        " voxel based cropping a new transform will be applied to the ROI.\n\nDo you want to continue and reset the ROI's transform?";
      int ret = QMessageBox::information(NULL,"Crop Volume",message,QMessageBox::Yes,QMessageBox::No);

      if(ret == QMessageBox::Yes)
        {
        inputROI->SetAndObserveTransformNodeID(0);
        }
      else if(ret == QMessageBox::No)
        {
        this->InputROIComboBox->setCurrentNode(NULL);
        return;
        }
      }
    }

  if(volumeTilted)
    {
    vtkSlicerCropVolumeLogic* logic = this->logic();
    Q_ASSERT(logic);
    logic->SnapROIToVoxelGrid(inputROI,inputVolume);
    }

}
//-----------------------------------------------------------------------------
// qSlicerCropVolumeModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerCropVolumeModuleWidget::qSlicerCropVolumeModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerCropVolumeModuleWidgetPrivate(*this) )
{
}

//-----------------------------------------------------------------------------
qSlicerCropVolumeModuleWidget::~qSlicerCropVolumeModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::setup()
{
  Q_D(qSlicerCropVolumeModuleWidget);

  d->setupUi(this);
  //ctkFlowLayout* flowLayout = ctkFlowLayout::replaceLayout(d->InterpolatorWidget);
  //flowLayout->setPreferredExpandingDirections(Qt::Vertical);

  this->Superclass::setup();

  connect(d->CropButton, SIGNAL(clicked()),
          this, SLOT(onApply()) );

  connect(d->ParametersNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(setParametersNode(vtkMRMLNode*)));

  connect(d->InputVolumeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(setInputVolume(vtkMRMLNode*)));

  connect(d->InputROIComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
          this, SLOT(setInputROI(vtkMRMLNode*)));
  connect(d->InputROIComboBox->nodeFactory(), SIGNAL(nodeInitialized(vtkMRMLNode*)),
          this, SLOT(initializeInputROI(vtkMRMLNode*)));
  connect(d->InputROIComboBox, SIGNAL(nodeAdded(vtkMRMLNode*)),
          this, SLOT(onInputROIAdded(vtkMRMLNode*)));

  connect(d->OutputVolumeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(setOutputVolume(vtkMRMLNode*)));

  connect(d->VisibilityButton, SIGNAL(toggled(bool)),
          this, SLOT(onROIVisibilityChanged(bool)));
  connect(d->ROIFitPushButton, SIGNAL(clicked()),
    this, SLOT(onROIFit()));

  connect(d->InterpolationEnabledCheckBox, SIGNAL(toggled(bool)),
    this, SLOT(onInterpolationEnabled(bool)));
  connect(d->SpacingScalingSpinBox, SIGNAL(valueChanged(double)),
    this, SLOT(onSpacingScalingValueChanged(double)));
  connect(d->IsotropicCheckbox, SIGNAL(toggled(bool)),
    this, SLOT(onIsotropicModeChanged(bool)));

  connect(d->LinearRadioButton, SIGNAL(toggled(bool)),
          this, SLOT(onInterpolationModeChanged()));
  connect(d->NNRadioButton, SIGNAL(toggled(bool)),
          this, SLOT(onInterpolationModeChanged()));
  connect(d->WSRadioButton, SIGNAL(toggled(bool)),
          this, SLOT(onInterpolationModeChanged()));
  connect(d->BSRadioButton, SIGNAL(toggled(bool)),
          this, SLOT(onInterpolationModeChanged()));

}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::enter()
{
  Q_D(qSlicerCropVolumeModuleWidget);

  // For user's convenience, create a deafult ROI parameter node if
  // none exists yet.
  vtkMRMLScene* scene = this->mrmlScene();
  if (!scene)
    {
    qWarning("qSlicerCropVolumeModuleWidget::enter: invalid scene");
    return;
    }
  if (scene->GetNumberOfNodesByClass("vtkMRMLCropVolumeParametersNode") == 0)
    {
    vtkNew<vtkMRMLCropVolumeParametersNode> parametersNode;
    scene->AddNode(parametersNode.GetPointer());
    d->ParametersNodeComboBox->setCurrentNode(parametersNode.GetPointer());
    }

  //this->setInputVolume();
  //this->onInputROIChanged();

  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  this->Superclass::setMRMLScene(scene);
  // observe close event so can re-add a parameters node if necessary
  qvtkReconnect(this->mrmlScene(), vtkMRMLScene::EndImportEvent, this, SLOT(onMRMLSceneEndBatchProcessEvent()));
  qvtkReconnect(this->mrmlScene(), vtkMRMLScene::EndBatchProcessEvent, this, SLOT(onMRMLSceneEndBatchProcessEvent()));
  qvtkReconnect(this->mrmlScene(), vtkMRMLScene::EndCloseEvent, this, SLOT(onMRMLSceneEndBatchProcessEvent()));
  qvtkReconnect(this->mrmlScene(), vtkMRMLScene::EndRestoreEvent, this, SLOT(onMRMLSceneEndBatchProcessEvent()));
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::initializeInputROI(vtkMRMLNode *n)
{
  Q_D(const qSlicerCropVolumeModuleWidget);
  vtkMRMLScene* scene = qobject_cast<qMRMLNodeFactory*>(this->sender())->mrmlScene();
  vtkMRMLAnnotationROINode::SafeDownCast(n)->Initialize(scene);
  if (d->ParametersNode && d->ParametersNode->GetInputVolumeNode())
    {
    this->onROIFit();
    }
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::onApply()
{
  Q_D(const qSlicerCropVolumeModuleWidget);
  vtkSlicerCropVolumeLogic *logic = d->logic();

  if(!d->ParametersNode.GetPointer() ||
    !d->ParametersNode->GetInputVolumeNode() ||
    !d->ParametersNode->GetOutputVolumeNode())
    {
    qWarning() << Q_FUNC_INFO << ": invalid inputs";
    return;
    }

  d->ParametersNode->SetROINodeID(d->InputROIComboBox->currentNode()->GetID());

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
  vtkMRMLNode* oldOutputNode = d->OutputVolumeComboBox->currentNode();
  if (!logic->Apply(d->ParametersNode))
    {
    // no errors
    if (d->ParametersNode->GetOutputVolumeNode() != oldOutputNode)
      {
      // new volume is created
      //d->OutputVolumeComboBox->setCurrentNode(parametersNode->GetOutputVolumeNode());
      // Show cropped volume in slice viewers
      vtkSlicerApplicationLogic *appLogic = this->module()->appLogic();
      vtkMRMLSelectionNode *selectionNode = appLogic->GetSelectionNode();
      selectionNode->SetReferenceActiveVolumeID(d->ParametersNode->GetOutputVolumeNodeID());
      appLogic->PropagateVolumeSelection();
      }
    }
  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::setInputVolume(vtkMRMLNode* volumeNode)
{
  Q_D(qSlicerCropVolumeModuleWidget);

  if (!d->ParametersNode.GetPointer())
    {
    qWarning() << Q_FUNC_INFO << ": invalid parameter node";
    return;
    }

  qvtkReconnect(d->InputVolumeNode, volumeNode, vtkCommand::ModifiedEvent,
    this, SLOT(updateVolumeInfo()));
  d->InputVolumeNode = volumeNode;
  d->ParametersNode->SetInputVolumeNodeID(volumeNode ? volumeNode->GetID() : NULL);
  // parameter node change will trigger GUI update
  //this->updateWidgetFromMRML();

  if (volumeNode)
    {
    // Check if this volume can be cropped
    if (!d->InterpolationEnabledCheckBox->isChecked())
      {
      if (d->isInputVolumeLinearTransformed())
        {
        d->showUnsupportedTransVolumeVoxelCroppingDialog();
        d->InputVolumeComboBox->setCurrentNode(NULL);
        }
      else
        {
        d->performROIVoxelGridAlignment();
        }
      }
    /*
    // Show volume in slice viewer
    if (this->mrmlScene() &&
        !this->mrmlScene()->IsClosing() &&
        !this->mrmlScene()->IsBatchProcessing())
      {
      // set it to be active in the slice windows
      vtkSlicerApplicationLogic *appLogic = this->module()->appLogic();
      vtkMRMLSelectionNode *selectionNode = appLogic->GetSelectionNode();
      selectionNode->SetReferenceActiveVolumeID(parametersNode->GetInputVolumeNodeID());
      appLogic->PropagateVolumeSelection();
      }
    }
    */
    }
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::setOutputVolume(vtkMRMLNode* volumeNode)
{
  Q_D(qSlicerCropVolumeModuleWidget);

  vtkMRMLCropVolumeParametersNode *parametersNode = vtkMRMLCropVolumeParametersNode::SafeDownCast(d->ParametersNodeComboBox->currentNode());
  if (!parametersNode)
    {
    qWarning() << Q_FUNC_INFO << ": invalid parameter node";
    return;
    }

  parametersNode->SetOutputVolumeNodeID(d->OutputVolumeComboBox->currentNode() ? d->OutputVolumeComboBox->currentNode()->GetID() : NULL);
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::setInputROI(vtkMRMLNode* node)
{
  Q_D(qSlicerCropVolumeModuleWidget);

  if (!d->ParametersNode.GetPointer())
    {
    qWarning() << Q_FUNC_INFO << ": invalid parameter node";
    return;
    }
  vtkMRMLAnnotationROINode* roiNode = vtkMRMLAnnotationROINode::SafeDownCast(node);
  qvtkReconnect(d->InputROINode, roiNode, vtkCommand::ModifiedEvent,
    this, SLOT(updateVolumeInfo()));
  d->InputROINode = roiNode;
  d->ParametersNode->SetROINodeID(roiNode ? roiNode->GetID() : NULL);
  // parameter node change will trigger GUI update
  //this->updateWidgetFromMRML();

  if (roiNode)
    {
    if (!d->InterpolationEnabledCheckBox->isChecked())
      {
      d->performROIVoxelGridAlignment();
      }
    d->VisibilityButton->setChecked(roiNode->GetDisplayVisibility());
    }
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::onInputROIAdded(vtkMRMLNode* node)
{
  Q_D(qSlicerCropVolumeModuleWidget);

  if (!d->ParametersNode.GetPointer())
    {
    qWarning() << Q_FUNC_INFO << ": invalid parameter node";
    return;
    }
  vtkMRMLAnnotationROINode* roiNode = vtkMRMLAnnotationROINode::SafeDownCast(node);
  if (roiNode && d->ParametersNode->GetInputVolumeNode())
    {
    this->onROIFit();
    }
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::onROIVisibilityChanged(bool visible)
{
  Q_D(qSlicerCropVolumeModuleWidget);
  if (!d->ParametersNode || !d->ParametersNode->GetROINode())
    {
    return;
    }
  d->ParametersNode->GetROINode()->SetDisplayVisibility(visible);
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::onROIFit()
{
  Q_D(qSlicerCropVolumeModuleWidget);
  d->logic()->FitROIToInputVolume(vtkMRMLCropVolumeParametersNode::SafeDownCast(d->ParametersNodeComboBox->currentNode()));
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::onInterpolationModeChanged()
{
  Q_D(qSlicerCropVolumeModuleWidget);
  if (!d->ParametersNode)
    {
    return;
    }
  if(d->NNRadioButton->isChecked())
    {
    d->ParametersNode->SetInterpolationMode(vtkMRMLCropVolumeParametersNode::InterpolationNearestNeighbor);
    }
  if(d->LinearRadioButton->isChecked())
    {
    d->ParametersNode->SetInterpolationMode(vtkMRMLCropVolumeParametersNode::InterpolationLinear);
    }
  if(d->WSRadioButton->isChecked())
    {
    d->ParametersNode->SetInterpolationMode(vtkMRMLCropVolumeParametersNode::InterpolationWindowedSinc);
    }
  if(d->BSRadioButton->isChecked())
    {
    d->ParametersNode->SetInterpolationMode(vtkMRMLCropVolumeParametersNode::InterpolationBSpline);
    }
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::onSpacingScalingValueChanged(double s)
{
  Q_D(qSlicerCropVolumeModuleWidget);
  if (!d->ParametersNode)
    {
    return;
    }
  d->ParametersNode->SetSpacingScalingConst(s);
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::onIsotropicModeChanged(bool isotropic)
{
  Q_D(qSlicerCropVolumeModuleWidget);
  if (!d->ParametersNode)
    {
    return;
    }
  d->ParametersNode->SetIsotropicResampling(isotropic);
}

//-----------------------------------------------------------------------------
void
qSlicerCropVolumeModuleWidget::onInterpolationEnabled(bool interpolationEnabled)
{
  Q_D(qSlicerCropVolumeModuleWidget);

  if (!d->ParametersNode)
    {
    return;
    }
  d->ParametersNode->SetVoxelBased(!interpolationEnabled);

  if (d->isInputVolumeLinearTransformed())
    {
    d->showUnsupportedTransVolumeVoxelCroppingDialog();
    d->InputVolumeComboBox->setCurrentNode(NULL);
    }
  else
    {
    d->performROIVoxelGridAlignment();
    }
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::onMRMLSceneEndBatchProcessEvent()
{
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
    {
    return;
    }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerCropVolumeModuleWidget);
  if (!this->mrmlScene())
    {
    return;
    }

  if (!d->ParametersNode)
    {
    // reset widget to defaults from node class
    d->InputVolumeComboBox->setCurrentNode(NULL);
    d->InputROIComboBox->setCurrentNode(NULL);

    d->InterpolationEnabledCheckBox->setChecked(true);
    d->VisibilityButton->setChecked(true);

    d->IsotropicCheckbox->setChecked(false);
    d->SpacingScalingSpinBox->setValue(1.0);
    d->LinearRadioButton->setChecked(1);

    return;
    }

  const char *volumeNodeID = d->ParametersNode->GetInputVolumeNodeID();
  int nodeCount = d->InputVolumeComboBox->nodeCount();
  if (!volumeNodeID && nodeCount != 0)
    {
    // the parameters node doesn't have a volume set, but there is a volume in the
    // combo box, so use the last one
    int nodeIndex = nodeCount - 1;
    volumeNodeID = d->InputVolumeComboBox->nodeFromIndex(nodeIndex)->GetID();
    // then reset it on the param node?
    d->ParametersNode->SetInputVolumeNodeID(volumeNodeID);
    }
  vtkMRMLNode *volumeNode = this->mrmlScene()->GetNodeByID(volumeNodeID);
  d->InputVolumeComboBox->setCurrentNode(volumeNode);

  /*
  const char *roiNodeID = d->ParametersNode->GetROINodeID();
  if (!roiNodeID && d->InputROIComboBox->nodeCount() != 0)
    {
    int nodeIndex =  d->InputROIComboBox->nodeCount() - 1;
    roiNodeID =  d->InputROIComboBox->nodeFromIndex(nodeIndex)->GetID();
    parametersNode->SetROINodeID(roiNodeID);
    }
    */
  d->InputROIComboBox->setCurrentNode(d->ParametersNode->GetROINode());

  d->InterpolationEnabledCheckBox->setChecked(!d->ParametersNode->GetVoxelBased());

  switch (d->ParametersNode->GetInterpolationMode())
    {
    case vtkMRMLCropVolumeParametersNode::InterpolationNearestNeighbor: d->NNRadioButton->setChecked(1); break;
    case vtkMRMLCropVolumeParametersNode::InterpolationLinear: d->LinearRadioButton->setChecked(1); break;
    case vtkMRMLCropVolumeParametersNode::InterpolationWindowedSinc: d->WSRadioButton->setChecked(1); break;
    case vtkMRMLCropVolumeParametersNode::InterpolationBSpline: d->BSRadioButton->setChecked(1); break;
    }
  d->IsotropicCheckbox->setChecked(d->ParametersNode->GetIsotropicResampling());
  d->VisibilityButton->setChecked(d->ParametersNode->GetROINode() && (d->ParametersNode->GetROINode()->GetDisplayVisibility() != 0));

  d->SpacingScalingSpinBox->setValue(d->ParametersNode->GetSpacingScalingConst());
}

//-----------------------------------------------------------
bool qSlicerCropVolumeModuleWidget::setEditedNode(vtkMRMLNode* node,
                                                  QString role /* = QString()*/,
                                                  QString context /* = QString()*/)
{
  Q_D(qSlicerCropVolumeModuleWidget);
  Q_UNUSED(role);
  Q_UNUSED(context);

  if (vtkMRMLCropVolumeParametersNode::SafeDownCast(node))
    {
    d->ParametersNodeComboBox->setCurrentNode(node);
    return true;
    }
  return false;
}

//------------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::setParametersNode(vtkMRMLNode* node)
{
  vtkMRMLCropVolumeParametersNode* parametersNode = vtkMRMLCropVolumeParametersNode::SafeDownCast(node);
  Q_D(qSlicerCropVolumeModuleWidget);
  qvtkReconnect(d->ParametersNode, parametersNode, vtkCommand::ModifiedEvent,
    this, SLOT(updateWidgetFromMRML()));
  d->ParametersNode = parametersNode;
  this->updateWidgetFromMRML();
}

//------------------------------------------------------------------------------
void qSlicerCropVolumeModuleWidget::updateVolumeInfo()
{
  Q_D(qSlicerCropVolumeModuleWidget);
  if (!d->ParametersNode)
    {
    return;
    }
  // d->InputDimensionsWidget->setCoordinates()
}
