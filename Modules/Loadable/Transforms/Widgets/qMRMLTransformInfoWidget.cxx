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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// qMRML includes
#include "qMRMLTransformInfoWidget.h"
#include "ui_qMRMLTransformInfoWidget.h"

// MRML includes
#include <vtkMRMLTransformNode.h>

//------------------------------------------------------------------------------
class qMRMLTransformInfoWidgetPrivate: public Ui_qMRMLTransformInfoWidget
{
  Q_DECLARE_PUBLIC(qMRMLTransformInfoWidget);

protected:
  qMRMLTransformInfoWidget* const q_ptr;

public:
  qMRMLTransformInfoWidgetPrivate(qMRMLTransformInfoWidget& object);
  void init();

  vtkMRMLTransformNode* MRMLTransformNode;
};

//------------------------------------------------------------------------------
qMRMLTransformInfoWidgetPrivate::qMRMLTransformInfoWidgetPrivate(qMRMLTransformInfoWidget& object)
  : q_ptr(&object)
{
  this->MRMLTransformNode = 0;
}

//------------------------------------------------------------------------------
void qMRMLTransformInfoWidgetPrivate::init()
{
  Q_Q(qMRMLTransformInfoWidget);
  this->setupUi(q);
  q->setEnabled(this->MRMLTransformNode != 0);
}

//------------------------------------------------------------------------------
qMRMLTransformInfoWidget::qMRMLTransformInfoWidget(QWidget *_parent)
  : Superclass(_parent)
  , d_ptr(new qMRMLTransformInfoWidgetPrivate(*this))
{
  Q_D(qMRMLTransformInfoWidget);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLTransformInfoWidget::~qMRMLTransformInfoWidget()
{
}

//------------------------------------------------------------------------------
vtkMRMLTransformNode* qMRMLTransformInfoWidget::mrmlTransformNode()const
{
  Q_D(const qMRMLTransformInfoWidget);
  return d->MRMLTransformNode;
}

//------------------------------------------------------------------------------
void qMRMLTransformInfoWidget::setMRMLTransformNode(vtkMRMLNode* node)
{
  this->setMRMLTransformNode(vtkMRMLTransformNode::SafeDownCast(node));
}

//------------------------------------------------------------------------------
void qMRMLTransformInfoWidget::setMRMLTransformNode(vtkMRMLTransformNode* transformNode)
{
  Q_D(qMRMLTransformInfoWidget);
  qvtkReconnect(d->MRMLTransformNode, transformNode, vtkCommand::ModifiedEvent,
                this, SLOT(updateWidgetFromMRML()));
  d->MRMLTransformNode = transformNode;
  this->updateWidgetFromMRML();
}

//------------------------------------------------------------------------------
void qMRMLTransformInfoWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLTransformInfoWidget);
  if (d->MRMLTransformNode)
  {
    d->TransformToParentInfoTextBrowser->setText(d->MRMLTransformNode->GetTransformToParentInfo());
    d->TransformFromParentInfoTextBrowser->setText(d->MRMLTransformNode->GetTransformFromParentInfo());
  }
  else
  {
    d->TransformToParentInfoTextBrowser->setText("");
    d->TransformFromParentInfoTextBrowser->setText("");
  }

  this->setEnabled(d->MRMLTransformNode != 0);
}
