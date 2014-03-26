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

#ifndef __qMRMLTransformsFiducialProjectionPropertyWidget_h
#define __qMRMLTransformsFiducialProjectionPropertyWidget_h

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

// SlicerQt includes
#include "qMRMLWidget.h"

#include "qSlicerTransformsModuleWidgetsExport.h"

class qMRMLTransformsFiducialProjectionPropertyWidgetPrivate;
class vtkMRMLTransformNode;

/// \ingroup Slicer_QtModules_Transforms
class Q_SLICER_MODULE_TRANSFORMS_WIDGETS_EXPORT
qMRMLTransformsFiducialProjectionPropertyWidget
  : public qMRMLWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qMRMLWidget Superclass;
  qMRMLTransformsFiducialProjectionPropertyWidget(QWidget *newParent = 0);
  virtual ~qMRMLTransformsFiducialProjectionPropertyWidget();

public slots:
  void setMRMLFiducialNode(vtkMRMLTransformNode* fiducialNode);
  void setProjectionVisibility(bool showProjection);
  void setProjectionColor(QColor newColor);
  void setUseFiducialColor(bool useFiducialColor);
  void setOutlinedBehindSlicePlane(bool outlinedBehind);
  void setProjectionOpacity(double opacity);

protected slots:
  void updateWidgetFromDisplayNode();

protected:
  QScopedPointer<qMRMLTransformsFiducialProjectionPropertyWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLTransformsFiducialProjectionPropertyWidget);
  Q_DISABLE_COPY(qMRMLTransformsFiducialProjectionPropertyWidget);

};

#endif
