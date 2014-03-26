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
#include "qMRMLTransformsFiducialProjectionPropertyWidget.h"
#include "ui_qMRMLTransformsFiducialProjectionPropertyWidget.h"

// MRML includes
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLTransformDisplayNode.h>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Transforms
class qMRMLTransformsFiducialProjectionPropertyWidgetPrivate
  : public Ui_qMRMLTransformsFiducialProjectionPropertyWidget
{
  Q_DECLARE_PUBLIC(qMRMLTransformsFiducialProjectionPropertyWidget);
protected:
  qMRMLTransformsFiducialProjectionPropertyWidget* const q_ptr;
public:
  qMRMLTransformsFiducialProjectionPropertyWidgetPrivate(qMRMLTransformsFiducialProjectionPropertyWidget& object);
  void init();

  vtkMRMLTransformDisplayNode* FiducialDisplayNode;
};

//-----------------------------------------------------------------------------
// qMRMLTransformsFiducialProjectionPropertyWidgetPrivate methods

//-----------------------------------------------------------------------------
qMRMLTransformsFiducialProjectionPropertyWidgetPrivate
::qMRMLTransformsFiducialProjectionPropertyWidgetPrivate(qMRMLTransformsFiducialProjectionPropertyWidget& object)
  : q_ptr(&object)
{
  this->FiducialDisplayNode = NULL;
}

//-----------------------------------------------------------------------------
void qMRMLTransformsFiducialProjectionPropertyWidgetPrivate
::init()
{
  Q_Q(qMRMLTransformsFiducialProjectionPropertyWidget);
  this->setupUi(q);
  QObject::connect(this->point2DProjectionCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setProjectionVisibility(bool)));
  QObject::connect(this->pointProjectionColorPickerButton, SIGNAL(colorChanged(QColor)),
                   q, SLOT(setProjectionColor(QColor)));
  QObject::connect(this->pointUseFiducialColorCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setUseFiducialColor(bool)));
  QObject::connect(this->pointOutlinedBehindSlicePlaneCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setOutlinedBehindSlicePlane(bool)));
  QObject::connect(this->projectionOpacitySliderWidget, SIGNAL(valueChanged(double)),
                   q, SLOT(setProjectionOpacity(double)));
  q->updateWidgetFromDisplayNode();
}

//-----------------------------------------------------------------------------
// qMRMLTransformsFiducialProjectionPropertyWidget methods

//-----------------------------------------------------------------------------
qMRMLTransformsFiducialProjectionPropertyWidget
::qMRMLTransformsFiducialProjectionPropertyWidget(QWidget *newParent) :
    Superclass(newParent)
  , d_ptr(new qMRMLTransformsFiducialProjectionPropertyWidgetPrivate(*this))
{
  Q_D(qMRMLTransformsFiducialProjectionPropertyWidget);
  d->init();
}

//-----------------------------------------------------------------------------
qMRMLTransformsFiducialProjectionPropertyWidget
::~qMRMLTransformsFiducialProjectionPropertyWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLTransformsFiducialProjectionPropertyWidget
::setMRMLFiducialNode(vtkMRMLTransformNode* fiducialNode)
{
  Q_D(qMRMLTransformsFiducialProjectionPropertyWidget);
  vtkMRMLTransformDisplayNode* displayNode = vtkMRMLTransformDisplayNode::SafeDownCast(fiducialNode->GetDisplayNode());

  qvtkReconnect(d->FiducialDisplayNode, displayNode, vtkCommand::ModifiedEvent,
                this, SLOT(updateWidgetFromDisplayNode()));

  d->FiducialDisplayNode = displayNode;
  this->updateWidgetFromDisplayNode();
}

//-----------------------------------------------------------------------------
void qMRMLTransformsFiducialProjectionPropertyWidget
::setProjectionVisibility(bool showProjection)
{
  Q_D(qMRMLTransformsFiducialProjectionPropertyWidget);
  if (!d->FiducialDisplayNode)
    {
    return;
    }
  if (showProjection)
    {
    //d->FiducialDisplayNode->SliceProjectionOn();
    }
  else
    {
    //d->FiducialDisplayNode->SliceProjectionOff();
    }
}

//-----------------------------------------------------------------------------
void qMRMLTransformsFiducialProjectionPropertyWidget
::setProjectionColor(QColor newColor)
{
  Q_D(qMRMLTransformsFiducialProjectionPropertyWidget);
  if (!d->FiducialDisplayNode)
    {
    return;
    }
  //d->FiducialDisplayNode->SetSliceProjectionColor(newColor.redF(), newColor.greenF(), newColor.blueF());
}

//-----------------------------------------------------------------------------
void qMRMLTransformsFiducialProjectionPropertyWidget
::setUseFiducialColor(bool useFiducialColor)
{
  Q_D(qMRMLTransformsFiducialProjectionPropertyWidget);
  if (!d->FiducialDisplayNode)
    {
    return;
    }
  if (useFiducialColor)
    {
    //d->FiducialDisplayNode->SliceProjectionUseFiducialColorOn();
    //d->pointProjectionColorLabel->setEnabled(false);
    //d->pointProjectionColorPickerButton->setEnabled(false);
    }
  else
    {
    //d->FiducialDisplayNode->SliceProjectionUseFiducialColorOff();
    //d->pointProjectionColorLabel->setEnabled(true);
    //d->pointProjectionColorPickerButton->setEnabled(true);
    }
}

//-----------------------------------------------------------------------------
void qMRMLTransformsFiducialProjectionPropertyWidget
::setOutlinedBehindSlicePlane(bool outlinedBehind)
{
  Q_D(qMRMLTransformsFiducialProjectionPropertyWidget);
  if (!d->FiducialDisplayNode)
    {
    return;
    }
  if (outlinedBehind)
    {
    //d->FiducialDisplayNode->SliceProjectionOutlinedBehindSlicePlaneOn();
    }
  else
    {
    //d->FiducialDisplayNode->SliceProjectionOutlinedBehindSlicePlaneOff();
    }
}

//-----------------------------------------------------------------------------
void qMRMLTransformsFiducialProjectionPropertyWidget
::setProjectionOpacity(double opacity)
{
  Q_D(qMRMLTransformsFiducialProjectionPropertyWidget);
  if (!d->FiducialDisplayNode)
    {
    return;
    }
  //d->FiducialDisplayNode->SetSliceProjectionOpacity(opacity);
}

//-----------------------------------------------------------------------------
void qMRMLTransformsFiducialProjectionPropertyWidget
::updateWidgetFromDisplayNode()
{
  Q_D(qMRMLTransformsFiducialProjectionPropertyWidget);

  this->setEnabled(d->FiducialDisplayNode != 0);

  if (!d->FiducialDisplayNode)
    {
    return;
    }

  // Update widget if different from MRML node
  // -- 2D Projection Visibility
  /*
  d->point2DProjectionCheckBox->setChecked(
    d->FiducialDisplayNode->GetSliceProjection() &
    vtkMRMLTransformDisplayNode::ProjectionOn);
  */

  // -- Projection Color
  double pColor[3]={0};
  //d->FiducialDisplayNode->GetSliceProjectionColor(pColor);
  QColor displayColor = QColor(pColor[0]*255, pColor[1]*255, pColor[2]*255);
  d->pointProjectionColorPickerButton->setColor(displayColor);

  // -- Use Fiducial Color
  //bool useFiducialColor = d->FiducialDisplayNode->GetSliceProjectionUseFiducialColor();
  bool useFiducialColor = false;

  d->pointUseFiducialColorCheckBox->setChecked(useFiducialColor);
  d->pointProjectionColorLabel->setEnabled(!useFiducialColor);
  d->pointProjectionColorPickerButton->setEnabled(!useFiducialColor);

  // -- Outlined Behind Slice Plane
  //d->pointOutlinedBehindSlicePlaneCheckBox->setChecked(d->FiducialDisplayNode->GetSliceProjectionOutlinedBehindSlicePlane());

  // -- Opacity
  //d->projectionOpacitySliderWidget->setValue(d->FiducialDisplayNode->GetSliceProjectionOpacity());
}
