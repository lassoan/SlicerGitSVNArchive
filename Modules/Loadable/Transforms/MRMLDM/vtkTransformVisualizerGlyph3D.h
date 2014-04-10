/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Andras Lasso and Franklin King at
  PerkLab, Queen's University and was supported through the Applied Cancer
  Research Unit program of Cancer Care Ontario with funds provided by the
  Ontario Ministry of Health and Long-Term Care.

==============================================================================*/

#ifndef __vtkTransformVisualizerGlyph3D_h
#define __vtkTransformVisualizerGlyph3D_h

#include "vtkSlicerTransformsModuleMRMLDisplayableManagerExport.h"

#include "vtkGlyph3D.h"
#include <vtkSmartPointer.h>

//------------------------------------------------------------------------------
class VTK_SLICER_TRANSFORMS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkTransformVisualizerGlyph3D : public vtkGlyph3D
{
public:
  vtkTypeMacro(vtkTransformVisualizerGlyph3D,vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkTransformVisualizerGlyph3D *New();

  vtkSetMacro(ScaleDirectional,bool);
  vtkGetMacro(ScaleDirectional,bool);

  vtkSetMacro(MagnitudeThresholding,bool);
  vtkGetMacro(MagnitudeThresholding,bool);
  vtkSetMacro(MagnitudeThresholdLower,double);
  vtkGetMacro(MagnitudeThresholdLower,double);
  vtkSetMacro(MagnitudeThresholdUpper,double);
  vtkGetMacro(MagnitudeThresholdUpper,double);

protected:
  vtkTransformVisualizerGlyph3D();
  ~vtkTransformVisualizerGlyph3D() {};

  bool ScaleDirectional;

  bool MagnitudeThresholding; // if nonzero then points with magnitude outside the lower/upper range are ignored
  double MagnitudeThresholdLower;
  double MagnitudeThresholdUpper;

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkTransformVisualizerGlyph3D(const vtkTransformVisualizerGlyph3D&);  // Not implemented.
  void operator=(const vtkTransformVisualizerGlyph3D&);  // Not implemented.
};

#endif
