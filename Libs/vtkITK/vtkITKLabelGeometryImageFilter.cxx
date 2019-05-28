/*=========================================================================

  Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   vtkITK
  Module:    $HeadURL$
  Date:      $Date$
  Version:   $Revision$

==========================================================================*/

// vtkITK includes
#include "vtkITKLabelGeometryImageFilter.h"

// VTK includes
#include <vtkDataArray.h>
#include "vtkImageData.h"
#include <vtkImageExport.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkITKUtility.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkVersion.h>

// VTKsys includes
//#include <vtksys/SystemTools.hxx>

// ITK includes
#include "itkLabelGeometryImageFilter.h"
#include "itkVTKImageImport.h"

vtkStandardNewMacro(vtkITKLabelGeometryImageFilter);

// Used instead of vtkTemplate2Macro to restrict first type to non-floating-point type
// and removed some rarely used types to reduce number of supported combinations
// (to reduce build time and code size).
#define vtkTemplate2MacroCP(call) \
  vtkTemplate2MacroCase1CP(VTK_SHORT, short, call);                             \
  vtkTemplate2MacroCase1CP(VTK_UNSIGNED_SHORT, unsigned short, call);           \
  vtkTemplate2MacroCase1CP(VTK_CHAR, char, call);                               \
  vtkTemplate2MacroCase1CP(VTK_SIGNED_CHAR, signed char, call);                 \
  vtkTemplate2MacroCase1CP(VTK_UNSIGNED_CHAR, unsigned char, call);             \
  vtkTemplate2MacroCase1CP(VTK_INT, int, call);                                 \
  vtkTemplate2MacroCase1CP(VTK_UNSIGNED_INT, unsigned int, call);               \
  vtkTemplate2MacroCase1CP(VTK_LONG, long, call);                               \
  vtkTemplate2MacroCase1CP(VTK_UNSIGNED_LONG, unsigned long, call)
#define vtkTemplate2MacroCase1CP(type1N, type1, call) \
  vtkTemplate2MacroCase2(type1N, type1, VTK_SHORT, short, call);                             \
  vtkTemplate2MacroCase2(type1N, type1, VTK_UNSIGNED_SHORT, unsigned short, call);           \
  vtkTemplate2MacroCase2(type1N, type1, VTK_CHAR, char, call);                               \
  vtkTemplate2MacroCase2(type1N, type1, VTK_SIGNED_CHAR, signed char, call);                 \
  vtkTemplate2MacroCase2(type1N, type1, VTK_UNSIGNED_CHAR, unsigned char, call);             \
  vtkTemplate2MacroCase2(type1N, type1, VTK_INT, int, call);                                 \
  vtkTemplate2MacroCase2(type1N, type1, VTK_UNSIGNED_INT, unsigned int, call);               \
  vtkTemplate2MacroCase2(type1N, type1, VTK_DOUBLE, double, call);                           \
  vtkTemplate2MacroCase2(type1N, type1, VTK_FLOAT, float, call);                             \
  vtkTemplate2MacroCase2(type1N, type1, VTK_LONG, long, call);                               \
  vtkTemplate2MacroCase2(type1N, type1, VTK_UNSIGNED_LONG, unsigned long, call)

// helper function
template <typename TLabelPixelType, typename TIntensityPixelType>
void ITKLabelGeometryImageFilterFromVTKImage2(vtkITKLabelGeometryImageFilter *self, vtkImageData *inputLabelImage, vtkImageData* inputIntensityImage)
{
  typedef itk::Image<TLabelPixelType, 3> LabelImageType;
  typedef itk::Image<TIntensityPixelType, 3> IntensityImageType;
  typedef itk::LabelGeometryImageFilter<LabelImageType, IntensityImageType> LabelGeometryImageFilterType;
  typename LabelGeometryImageFilterType::Pointer labelGeometryFilter = LabelGeometryImageFilterType::New();

  // Set label input
  typedef typename itk::VTKImageImport<LabelImageType> LabelImageImportType;
  typename LabelImageImportType::Pointer itkLabelImporter = LabelImageImportType::New();
  vtkNew<vtkImageExport> vtkLabelExporter;
  vtkLabelExporter->SetInputData ( inputLabelImage );
  ConnectPipelines(vtkLabelExporter.GetPointer(), itkLabelImporter);
  labelGeometryFilter->SetInput(itkLabelImporter->GetOutput());

  // Set optional intensity input
  if (inputIntensityImage)
    {
    typedef typename itk::VTKImageImport<IntensityImageType> IntensityImageImportType;
    typename IntensityImageImportType::Pointer itkIntensityImporter = IntensityImageImportType::New();
    vtkNew<vtkImageExport> vtkIntensityExporter;
    vtkIntensityExporter->SetInputData(inputIntensityImage);
    ConnectPipelines(vtkIntensityExporter.GetPointer(), itkIntensityImporter);
    labelGeometryFilter->SetIntensityInput(itkIntensityImporter->GetOutput());
    }

  labelGeometryFilter->SetCalculateOrientedBoundingBox(true);

  try
    {
    labelGeometryFilter->Update();
    }
  catch (itk::ExceptionObject &err)
    {
    vtkErrorWithObjectMacro(self, "Failed to compute label geometry. Details: " << err);
    }

  typename LabelGeometryImageFilterType::MatrixType rotMatrix = labelGeometryFilter->GetRotationMatrix(self->GetLabelValue());

  typename LabelGeometryImageFilterType::LabelPointType boundingBoxSize = labelGeometryFilter->GetOrientedBoundingBoxSize(self->GetLabelValue());

  double spacing[3] = { 1.0 };
  inputLabelImage->GetSpacing(spacing);
  vnl_diag_matrix<double> spacingMatrix;
  spacingMatrix.set_size(3);
  spacingMatrix[0] = spacing[0];
  spacingMatrix[1] = spacing[1];
  spacingMatrix[2] = spacing[2];

  vnl_vector<double> boundingBoxSizeVector;
  boundingBoxSizeVector.set_size(3);
  boundingBoxSizeVector[0] = boundingBoxSize[0];
  boundingBoxSizeVector[1] = boundingBoxSize[1];
  boundingBoxSizeVector[2] = boundingBoxSize[2];
  vnl_vector<double> boundingBoxSizeScaled = spacingMatrix * rotMatrix.transpose() * boundingBoxSizeVector;

/*  double spacing[3] = { 1.0 };
  inputLabelImage->GetSpacing(spacing);
  boundingBoxSizeScaled[0] *= spacing[0];
  boundingBoxSizeScaled[1] *= spacing[1];
  boundingBoxSizeScaled[2] *= spacing[2];
  */
  /*
  double spacing[3] = { 1.0 };
  inputLabelImage->GetSpacing(spacing);
  typename LabelGeometryImageFilterType::AxesLengthType axesLengthPixels = labelGeometryFilter->GetAxesLength(self->GetLabelValue());
  double axesLengthSorted[3] =
    {
    axesLengthPixels[0] * spacing[0],
    axesLengthPixels[1] * spacing[1],
    axesLengthPixels[2] * spacing[2]
    };
  // sort in descending order (largest value comes first)
  if (axesLengthSorted[0] < axesLengthSorted[1])
    {
    std::swap(axesLengthSorted[0], axesLengthSorted[1]);
    }
  if (axesLengthSorted[1] < axesLengthSorted[2])
    {
    std::swap(axesLengthSorted[1], axesLengthSorted[2]);
    if (axesLengthSorted[0] < axesLengthSorted[1])
      {
      std::swap(axesLengthSorted[0], axesLengthSorted[1]);
      }
    }
  self->SetAxesLength(axesLengthSorted);
  */
  self->SetAxesLength(fabs(boundingBoxSizeScaled[0]), fabs(boundingBoxSizeScaled[1]), fabs(boundingBoxSizeScaled[2]));
}

//----------------------------------------------------------------------------
vtkITKLabelGeometryImageFilter::vtkITKLabelGeometryImageFilter()
{
  this->LabelValue = 1;
  this->AxesLength[0] = 0.0;
  this->AxesLength[1] = 0.0;
  this->AxesLength[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkITKLabelGeometryImageFilter::~vtkITKLabelGeometryImageFilter()
= default;

//----------------------------------------------------------------------------
void vtkITKLabelGeometryImageFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AxesLength: "
    << this->AxesLength[0] << ", "
    << this->AxesLength[1] << ", "
    << this->AxesLength[2] << "\n";
}

//----------------------------------------------------------------------------
// Writes all the data from the input.
void vtkITKLabelGeometryImageFilter::Update()
{
  vtkImageData *inputLabelImage = this->GetImageDataInput(0);
  if (!inputLabelImage)
    {
    vtkErrorMacro("vtkITKLabelGeometryImageFilter: No input label image");
    this->SetAxesLength(0.0, 0.0, 0.0);
    return;
    }
  vtkPointData* inputLabelImagePointData = inputLabelImage->GetPointData();
  if (!inputLabelImagePointData)
    {
    // empty input label image
    this->SetAxesLength(0.0, 0.0, 0.0);
    return;
    }

  vtkImageData* inputIntensityImage = nullptr;
  if (this->GetNumberOfInputPorts() > 1)
    {
    inputIntensityImage = this->GetImageDataInput(1);
    }

  this->UpdateInformation();
  if (this->GetOutputInformation(0))
    {
    this->GetOutputInformation(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      this->GetOutputInformation(0)->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
    }

  if ( inputLabelImagePointData->GetScalars() == nullptr)
    {
    vtkErrorMacro(<<"vtkITKLabelGeometryImageFilter: Scalar input image is required");
    this->SetAxesLength(0.0, 0.0, 0.0);
    return;
    }
  int inputNumberOfScalarComponents = inputLabelImagePointData->GetScalars()->GetNumberOfComponents();
  if (inputNumberOfScalarComponents != 1)
    {
    vtkErrorMacro(<<"vtkITKLabelGeometryImageFilter: Scalar input image with a single component is required");
    this->SetAxesLength(0.0, 0.0, 0.0);
    return;
    }
  int inputLabelDataType = inputLabelImagePointData->GetScalars()->GetDataType();

  vtkPointData* inputIntensityImagePointData = nullptr;
  if (inputIntensityImage)
    {
    inputIntensityImagePointData = inputIntensityImage->GetPointData();
    }
  if (inputIntensityImagePointData != nullptr && inputIntensityImagePointData->GetScalars() == nullptr)
    {
    int inputNumberOfScalarComponents = inputIntensityImagePointData->GetScalars()->GetNumberOfComponents();
    if (inputNumberOfScalarComponents != 1)
      {
      vtkErrorMacro(<< "vtkITKLabelGeometryImageFilter: Scalar input intensity image with a single component is required");
      this->SetAxesLength(0.0, 0.0, 0.0);
      return;
      }
    int inputIntensityDataType = inputIntensityImagePointData->GetScalars()->GetDataType();
    switch (vtkTemplate2PackMacro(inputLabelDataType, inputIntensityDataType))
      {
      vtkTemplate2MacroCP((ITKLabelGeometryImageFilterFromVTKImage2<VTK_T1, VTK_T2>(this, inputLabelImage, inputIntensityImage)));
      default:
        vtkErrorMacro("Execute: Unknown scalar types: " << inputLabelDataType << " , " << inputIntensityDataType);
        this->SetAxesLength(0.0, 0.0, 0.0);
        return;
      }
    }
  else
    {
    switch (vtkTemplate2PackMacro(inputLabelDataType, VTK_UNSIGNED_CHAR))
      {
      vtkTemplate2MacroCP((ITKLabelGeometryImageFilterFromVTKImage2<VTK_T1, VTK_T2>(this, inputLabelImage, inputIntensityImage)));
      default:
        vtkErrorMacro("Execute: Unknown scalar type: " << inputLabelDataType);
        this->SetAxesLength(0.0, 0.0, 0.0);
        return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkITKLabelGeometryImageFilter::SetLabelImageConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(0, algOutput);
}

//----------------------------------------------------------------------------
void vtkITKLabelGeometryImageFilter::SetLabelImageData(vtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void vtkITKLabelGeometryImageFilter::SetIntensityImageConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkITKLabelGeometryImageFilter::SetIntensityImageData(vtkDataObject* input)
{
  this->SetInputData(1, input);
}
