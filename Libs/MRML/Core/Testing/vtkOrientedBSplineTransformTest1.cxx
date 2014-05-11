/*=auto=========================================================================

  Portions (c) Copyright 2010 Brigham and Women's Hospital (BWH)
  All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer

=========================================================================auto=*/

// MRML includes
#include "vtkMRMLCoreTestingMacros.h"
#include "vtkOrientedBSplineTransform.h"

// ITK includes
#include <itkAffineTransform.h>
#include <itkBSplineDeformableTransform.h>

// VTK includes
#include "vtkImageData.h"
#include "vtkNew.h"

typedef itk::BSplineDeformableTransform<double,3,3> itkBSplineType;

//----------------------------------------------------------------------------
void CreateBSplineVtk(vtkOrientedBSplineTransform* bsplineTransform,
  double origin[3], double spacing[3], double dims[3],
  const double bulkMatrix[3][3], const double bulkOffset[3])
{
  vtkNew<vtkImageData> bsplineCoefficients;
  bsplineCoefficients->SetExtent(0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1);
  bsplineCoefficients->SetOrigin(origin);
  bsplineCoefficients->SetSpacing(spacing);
  bsplineCoefficients->SetScalarTypeToDouble();
  bsplineCoefficients->SetNumberOfScalarComponents(3);
  bsplineCoefficients->AllocateScalars();

  // Fill with 0
  for (int z = 0; z < dims[2]; z++)
  {
  for (int y = 0; y < dims[1]; y++)
    {
    for (int x = 0; x < dims[0]; x++)
      {
      bsplineCoefficients->SetScalarComponentFromDouble(x,y,z,0, 0.0);
      bsplineCoefficients->SetScalarComponentFromDouble(x,y,z,1, 0.0);
      bsplineCoefficients->SetScalarComponentFromDouble(x,y,z,2, 0.0);
      }
    }
  }

  vtkNew<vtkMatrix4x4> bulkTransform;
  for (int row=0; row<3; row++)
    {
    for (int col=0; col<3; col++)
      {
      bulkTransform->SetElement(row,col,bulkMatrix[row][col]);
      }
    bulkTransform->SetElement(row,3,bulkOffset[row]);
    }

  bsplineTransform->SetCoefficients(bsplineCoefficients.GetPointer());
  bsplineTransform->SetBulkTransformMatrix(bulkTransform.GetPointer());

  bsplineTransform->SetBorderModeToZero();
}

//----------------------------------------------------------------------------
void SetBSplineNodeVtk(vtkOrientedBSplineTransform* bsplineTransform,int nodeIndex[3], double nodeValue[3])
{
  vtkImageData* bsplineCoefficients=bsplineTransform->GetCoefficients();
  bsplineCoefficients->SetScalarComponentFromDouble(nodeIndex[0],nodeIndex[1],nodeIndex[2],0, nodeValue[0]);
  bsplineCoefficients->SetScalarComponentFromDouble(nodeIndex[0],nodeIndex[1],nodeIndex[2],1, nodeValue[1]);
  bsplineCoefficients->SetScalarComponentFromDouble(nodeIndex[0],nodeIndex[1],nodeIndex[2],2, nodeValue[2]);
}

//----------------------------------------------------------------------------
itkBSplineType::Pointer CreateBSplineItk(
  const double origin[3], const double spacing[3], const double dims[3],
  const double bulkMatrix[3][3], const double bulkOffset[3])
{
  itkBSplineType::RegionType region;
  itkBSplineType::OriginType originItk;
  itkBSplineType::SpacingType spacinItk;
  itkBSplineType::RegionType::SizeType sz;

  for (int i=0; i<3; i++)
    {
    originItk[i] = origin[i];
    spacinItk[i] = spacing[i];
    sz[i]=dims[i];
    }

  region.SetSize( sz );

  itkBSplineType::RegionType::IndexType idx;
  idx[0] = idx[1] = idx[2] = 0;
  region.SetIndex( idx );

  itkBSplineType::Pointer bspline = itkBSplineType::New();
  bspline->SetGridRegion( region );
  bspline->SetGridOrigin( originItk );
  bspline->SetGridSpacing( spacinItk );

  typedef itk::AffineTransform< double,3 > BulkTransformType;
  const BulkTransformType::Pointer bulkTransform = BulkTransformType::New();
  BulkTransformType::MatrixType m;
  for (int row=0; row<3; row++)
    {
    for (int col=0; col<3; col++)
      {
      m[row][col]=bulkMatrix[row][col];
      }
    }
  bulkTransform->SetMatrix(m);
  BulkTransformType::OffsetType v;
  v[0]=bulkOffset[0];
  v[1]=bulkOffset[1];
  v[2]=bulkOffset[2];
  bulkTransform->SetOffset(v);

  bspline->SetBulkTransform(bulkTransform);

  const unsigned int numberOfParameters = bspline->GetNumberOfParameters();
  typedef itkBSplineType::ParametersType ParametersType;
  ParametersType parameters( numberOfParameters );
  const unsigned int numberOfNodes = numberOfParameters / 3;
  assert( numberOfNodes == dims[0] * dims[1] * dims[2] );
  parameters.Fill( 0.0 );

  bspline->SetParameters( parameters );

  return bspline;
}

//----------------------------------------------------------------------------
void SetBSplineNodeItk(itkBSplineType::Pointer bspline,int nodeIndex[3], double nodeValue[3])
{
  itkBSplineType::RegionType region = bspline->GetGridRegion();
  itkBSplineType::RegionType::SizeType dims = region.GetSize();
  itkBSplineType::ParametersType parameters = bspline->GetParameters();

  const unsigned int numberOfNodes = dims[0] * dims[1] * dims[2];
  parameters.SetElement( numberOfNodes*0 + nodeIndex[0] + nodeIndex[1]*dims[0] + nodeIndex[2]*dims[0]*dims[1], nodeValue[0] );
  parameters.SetElement( numberOfNodes*1 + nodeIndex[0] + nodeIndex[1]*dims[0] + nodeIndex[2]*dims[0]*dims[1], nodeValue[1] );
  parameters.SetElement( numberOfNodes*2 + nodeIndex[0] + nodeIndex[1]*dims[0] + nodeIndex[2]*dims[0]*dims[1], nodeValue[2] );

  bspline->SetParameters( parameters );
}

//----------------------------------------------------------------------------
double getTransformedPointDifferenceItkVtk(const double inputPoint[3], itkBSplineType::Pointer bsplineItk, vtkOrientedBSplineTransform* bsplineVtk, bool logDetails)
{
  // ITK
  itkBSplineType::InputPointType inputPointItk;
  inputPointItk[0] = inputPoint[0];
  inputPointItk[1] = inputPoint[1];
  inputPointItk[2] = inputPoint[2];
  itkBSplineType::OutputPointType outputPointItk;
  outputPointItk = bsplineItk->TransformPoint( inputPointItk );

  // VTK
  double outputPoint[3]={0};
  bsplineVtk->TransformPoint( inputPoint, outputPoint );

  itk::Point<double,3> vtkInputPoint( inputPoint );
  itk::Point<double,3> vtkOutputPoint( outputPoint );
  double difference = outputPointItk.EuclideanDistanceTo( vtkOutputPoint );

  if (logDetails)
    {
    std::cout << "Compare ITK and VTK transform results" << std::endl;
    std::cout << " Input point: " << inputPoint[0] << " " << inputPoint[1] << " " << inputPoint[2] << std::endl;
    std::cout << " Output point (transformed by ITK): " << outputPointItk << std::endl;
    std::cout << " Output point (transformed by VTK): " << outputPoint[0] << " " << outputPoint[1] << " " << outputPoint[2] << std::endl;
    std::cout << " Difference between ITK and VTK transform results: "<<difference << std::endl;
    }

  return difference;
}

//----------------------------------------------------------------------------
double getInverseDifferenceVtk(const double inputPoint[3], vtkOrientedBSplineTransform* bsplineVtk, bool logDetails)
{
  double outputPoint[3] = { -1, -1, -1 };
  bsplineVtk->TransformPoint( inputPoint, outputPoint);

  double inversePoint[3] = { -1, -1, -1 };
  bsplineVtk->Inverse();
  bsplineVtk->TransformPoint( outputPoint, inversePoint );
  bsplineVtk->Inverse();

  itk::Point<double,3> inputPointVtk( inputPoint );
  itk::Point<double,3> inversePointVtk( inversePoint );
  const double tolerance=0.01; // the tolerance value matching the inverse computation tolerance in vtkITKBsplineTransform
  double errorOfInverseComputation = inputPointVtk.EuclideanDistanceTo( inversePointVtk );

  if (logDetails)
    {
    std::cout << "Verify VTK transform inverse" << std::endl;
    std::cout << " Input point: " << inputPoint[0] << " " << inputPoint[1] << " " << inputPoint[2] << std::endl;
    std::cout << " Transformed point: " << outputPoint[0] << " " << outputPoint[1] << " " << outputPoint[2] << std::endl;
    std::cout << " Transformed point transformed by inverse: " << inversePoint[0] << " " << inversePoint[1] << " " << inversePoint[2] << std::endl;
    std::cout << " Difference between VTK transform inverse and the ground truth: " << errorOfInverseComputation << std::endl;
    }

  return errorOfInverseComputation;
}

//----------------------------------------------------------------------------
int vtkOrientedBSplineTransformTest1(int , char * [] )
{
  // we want to transform a 300x400x300 image, with grid points 100
  // pixels apart. So, we need 4x5x4 "interior" grid points. Since the
  // spline order is 3, we need 1 exterior node on the "left" and 2
  // exterior nodes on the "right".  So, we need a spline grid of
  // 7x8x7. We want to transform the whole image, so the origin should
  // be set to spline grid location (0,0).


  //  B--o--o--o--o--o--o   A = image(0,0)       A = spline_grid(1,1)
  //  |  |  |  |  |  |  |   B = image(-100,-100) B = spline_grid(0,0)
  //  o--A--+--+--+--o--o
  //  |  |  |  |  |  |  |
  //  o--+--+--+--+--o--o   origin should be set to (-100,-100)
  //  |  |  |  |  |  |  |
  //  o--+--+--+--+--o--o
  //  |  |  |  |  |  |  |
  //  o--o--o--o--o--o--o
  //  |  |  |  |  |  |  |
  //  o--o--o--o--o--o--o

  double origin[3] = {-100, -100, -100};
  double spacing[3] = {100, 100, 100};
  double dims[3] = {7,8,7};

  const double bulkMatrix[3][3]={ { 0.7, 0.2, 0.1 }, { 0.1, 0.8, 0.1 }, { 0.05, 0.2, 0.9 }};
  const double bulkOffset[3]={-5, 3, 6};

  // set the parameter at image(100,200,0)=>node(2,3,1)
  // the displacement of x at (100,200,0)
  int modifiedBSplineNodeIndex[3]={2,3,1};
  double modifiedBSplineNodeValue[3]={2.0,3.0,6.0};

  // Create an ITK BSpline transform. It'll serve as the reference.
  itkBSplineType::Pointer bsplineItk=CreateBSplineItk(origin, spacing, dims, bulkMatrix, bulkOffset);
  // Modify a BSpline node
  SetBSplineNodeItk(bsplineItk, modifiedBSplineNodeIndex, modifiedBSplineNodeValue);

  // Create a VTK BSpline transform with the same parameters.
  vtkNew<vtkOrientedBSplineTransform> bsplineVtk;
  CreateBSplineVtk(bsplineVtk.GetPointer(), origin, spacing, dims, bulkMatrix, bulkOffset);
  // Modify a BSpline node
  SetBSplineNodeVtk(bsplineVtk.GetPointer(), modifiedBSplineNodeIndex, modifiedBSplineNodeValue);

  bool testPassed=true;
  int numberOfPointsTested=0;

  // We take samples in the bspline region between the firs+1 and last-1 node
  // because the boundaries are handled differently in ITK and VTK (in ITK there is an
  // abrupt change to 0, while in VTK it is smoothly converges to zero)
  const int numberOfSamplesPerAxis=10;
  double startX = origin[0]+1*spacing[0];
  double endX = origin[0]+(dims[0]-2)*spacing[0];
  double startY = origin[1]+1*spacing[1];
  double endY = origin[1]+(dims[1]-2)*spacing[1];
  double startZ = origin[2]+1*spacing[2];
  double endZ = origin[2]+(dims[2]-2)*spacing[2];
  int incX=static_cast<int>((endX-startX)/numberOfSamplesPerAxis);
  int incY=static_cast<int>((endY-startY)/numberOfSamplesPerAxis);
  int incZ=static_cast<int>((endZ-startZ)/numberOfSamplesPerAxis);
  for (int z=startZ; z<=endZ; z+=incZ)
    {
    for (int y=startY; y<=endY; y+=incY)
      {
      for (int x=startX; x<=endX; x+=incX)
        {
        numberOfPointsTested++;
        double inputPoint[3] = {x,y,z};
        // Compare transformation results computed by ITK and VTK.
        double differenceItkVtk = getTransformedPointDifferenceItkVtk(inputPoint, bsplineItk, bsplineVtk.GetPointer(), false);
        if ( differenceItkVtk > 1e-6 )
          {
          getTransformedPointDifferenceItkVtk(inputPoint, bsplineItk, bsplineVtk.GetPointer(), true);
          std::cout << "ERROR: Point transfom result mismatch between ITK and VTK" << std::endl;
          testPassed=false;
          }
        // Verify VTK inverse transform
        double inverseDifference=getInverseDifferenceVtk(inputPoint, bsplineVtk.GetPointer(), false);
        if ( inverseDifference > 1e-3 )
          {
          getInverseDifferenceVtk(inputPoint, bsplineVtk.GetPointer(), true);
          std::cout << "ERROR: Point transformed by forward and inverse transform does not match the original point" << std::endl;
          testPassed=false;
          }
        }
      }
    }

  std::cout << "Number of points tested: " << numberOfPointsTested << std::endl;
  if (testPassed)
    {
    std::cout << "Test result: PASSED" << std::endl;
    return EXIT_SUCCESS;
    }
  else
    {
    std::cout << "Test result: FAILED" << std::endl;
    return EXIT_FAILURE;
    }
}
