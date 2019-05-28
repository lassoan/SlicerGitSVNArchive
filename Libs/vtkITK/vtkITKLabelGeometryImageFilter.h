/*=========================================================================

  Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   vtkITK
  Module:    $HeadURL$
  Date:      $Date$
  Version:   $Revision$

==========================================================================*/

#ifndef __vtkITKLabelGeometryImageFilter_h
#define __vtkITKLabelGeometryImageFilter_h

#include "vtkImageAlgorithm.h"

//#include "vtkObjectFactory.h"
//#include "vtkMatrix4x4.h"

#include "vtkITK.h"
#include "itkImageIOBase.h"

//class vtkStringArray;

class VTK_ITK_EXPORT vtkITKLabelGeometryImageFilter : public vtkImageAlgorithm
{
public:
  static vtkITKLabelGeometryImageFilter *New();
  vtkTypeMacro(vtkITKLabelGeometryImageFilter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the label value that geomtry will be computed of.
   */
  vtkSetMacro(LabelValue, int);
  vtkGetMacro(LabelValue, int);
  //@}

  //@{
  /**
   * Stores the computed axes lengths, sorted as
   * major, minor, and orthogonal.
   */
  vtkSetVector3Macro(AxesLength, double);
  vtkGetVector3Macro(AxesLength, double);
  //@}

  /// Bring vtkAlgorithm::Update methods here
  /// to avoid hiding Update override.
  using vtkAlgorithm::Update;
  /// The main interface which triggers the writer to start.
  void Update() override;

  void SetLabelImageConnection(vtkAlgorithmOutput* algOutput);
  void SetLabelImageData(vtkDataObject* input);

  void SetIntensityImageConnection(vtkAlgorithmOutput* algOutput);
  void SetIntensityImageData(vtkDataObject* input);

protected:
  vtkITKLabelGeometryImageFilter();
  ~vtkITKLabelGeometryImageFilter() override;

  double AxesLength[3];
  int LabelValue;

private:
  vtkITKLabelGeometryImageFilter(const vtkITKLabelGeometryImageFilter&) = delete;
  void operator=(const vtkITKLabelGeometryImageFilter&) = delete;
};

//vtkStandardNewMacro(vtkITKLabelGeometryImageFilter)

#endif
