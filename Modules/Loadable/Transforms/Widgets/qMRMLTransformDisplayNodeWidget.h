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

#ifndef __qMRMLTransformDisplayNodeWidget_h
#define __qMRMLTransformDisplayNodeWidget_h

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

// SlicerQt includes
#include "qMRMLWidget.h"

#include "qSlicerTransformsModuleWidgetsExport.h"

class qMRMLTransformDisplayNodeWidgetPrivate;
class vtkMRMLTransformNode;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_Transforms
class Q_SLICER_MODULE_TRANSFORMS_WIDGETS_EXPORT
qMRMLTransformDisplayNodeWidget
  : public qMRMLWidget
{
  Q_OBJECT
  QVTK_OBJECT

public:
  typedef qMRMLWidget Superclass;
  qMRMLTransformDisplayNodeWidget(QWidget *newParent = 0);
  virtual ~qMRMLTransformDisplayNodeWidget();

public slots:

  /// Set the MRML node of interest
  /// Note that setting transformNode to 0 will disable the widget
  void setMRMLTransformNode(vtkMRMLTransformNode* transformNode);

  /// Utility function that calls setMRMLTransformNode(vtkMRMLTransformNode* transformNode)
  /// It's useful to connect to vtkMRMLNode* signals
  void setMRMLTransformNode(vtkMRMLNode* node);

  void updateLabels();
  void updateGlyphSourceOptions(int sourceOption);
  void referenceVolumeChanged(vtkMRMLNode* node);
  void setGlyphPointMax(double pointMax);
  void setSeed();
  void setGlyphSeed(int seed);
  void setGlyphScale(double scale);
  void setGlyphThreshold(double min, double max);
  void setGlyphSourceOption(int option);
  void setGlyphArrowScaleDirectional(bool state);
  void setGlyphArrowScaleIsotropic(bool state);
  void setGlyphArrowTipLength(double length);
  void setGlyphArrowTipRadius(double radius);
  void setGlyphArrowShaftRadius(double radius);
  void setGlyphArrowResolution(double resolution);
  void setGlyphConeScaleDirectional(bool state);
  void setGlyphConeScaleIsotropic(bool state);
  void setGlyphConeHeight(double height);
  void setGlyphConeRadius(double radius);
  void setGlyphConeResolution(double resolution);
  void setGlyphSphereResolution(double resolution);
  void setGridScale(double scale);
  void setGridSpacingMM(double spacing);
  void setBlockScale(double scale);
  void setBlockDisplacementCheck(int state);
  void setContourValues(QString values_str);
  void setContourDecimation(double reduction);
  void setGlyphSlicePointMax(double pointMax);
  void setGlyphSliceThreshold(double min, double max);
  void setGlyphSliceScale(double scale);
  void setGlyphSliceSeed(int seed);
  void setSeed2();
  void setGridSliceScale(double scale);
  void setGridSliceSpacingMM(double spacing);

protected slots:
  void updateWidgetFromDisplayNode();

protected:
  QScopedPointer<qMRMLTransformDisplayNodeWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLTransformDisplayNodeWidget);
  Q_DISABLE_COPY(qMRMLTransformDisplayNodeWidget);

};

#endif
