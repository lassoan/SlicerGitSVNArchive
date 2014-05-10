/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

#include "vtkOrientedGridTransform.h"

#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkOrientedGridTransform);

vtkCxxSetObjectMacro(vtkOrientedGridTransform,GridDirectionMatrix,vtkMatrix4x4);

//----------------------------------------------------------------------------
vtkOrientedGridTransform::vtkOrientedGridTransform()
{
  this->GridDirectionMatrix = NULL;
  this->RasToRotatedRasMatrixCached = vtkMatrix4x4::New();
  this->RotatedRasToRasMatrixCached = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------------
vtkOrientedGridTransform::~vtkOrientedGridTransform()
{
  this->SetGridDirectionMatrix(NULL);
  if (this->RasToRotatedRasMatrixCached)
    {
    this->RasToRotatedRasMatrixCached->Delete();
    this->RasToRotatedRasMatrixCached = NULL;
    }
  if (this->RotatedRasToRasMatrixCached)
    {
    this->RotatedRasToRasMatrixCached->Delete();
    this->RotatedRasToRasMatrixCached = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkOrientedGridTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "GridDirectionMatrix: " << this->GridDirectionMatrix << "\n";
  if (this->GridDirectionMatrix)
    {
    this->GridDirectionMatrix->PrintSelf(os,indent.GetNextIndent());
    }
}

//------------------------------------------------------------------------
inline void vtkLinearTransformPoint(const double matrix[4][4],
                                    const double in[3], double out[3])
{
  double x =
    matrix[0][0]*in[0]+matrix[0][1]*in[1]+matrix[0][2]*in[2]+matrix[0][3];
  double y =
    matrix[1][0]*in[0]+matrix[1][1]*in[1]+matrix[1][2]*in[2]+matrix[1][3];
  double z =
    matrix[2][0]*in[0]+matrix[2][1]*in[1]+matrix[2][2]*in[2]+matrix[2][3];

  out[0] = x;
  out[1] = y;
  out[2] = z;
}

//------------------------------------------------------------------------
inline void vtkLinearTransformDerivative(const double matrix[4][4],
                                         const double derivativeIn[3][3],
                                         double derivativeOut[3][3])
{
  int i, k;
  double Accum[3][3];

  for (i = 0; i < 3; ++i)
    {
    for (k = 0; k < 3; ++k)
      {
      Accum[i][k] = matrix[i][0] * derivativeIn[k][0] +
                    matrix[i][1] * derivativeIn[k][1] +
                    matrix[i][2] * derivativeIn[k][2];
      }
    }

  // Copy to final dest
  for (i = 0; i < 3; ++i)
    {
    derivativeOut[i][0] = Accum[i][0];
    derivativeOut[i][1] = Accum[i][1];
    derivativeOut[i][2] = Accum[i][2];
    }
}

//----------------------------------------------------------------------------
void vtkOrientedGridTransform::ForwardTransformPoint(const double inPoint[3],
                                             double outPoint[3])
{
  if (this->GridDirectionMatrix==NULL)
    {
    this->Superclass::ForwardTransformPoint(inPoint,outPoint);
    return;
    }

  vtkLinearTransformPoint(this->RasToRotatedRasMatrixCached->Element, inPoint, outPoint);
  this->Superclass::ForwardTransformPoint(outPoint,outPoint);
  vtkLinearTransformPoint(this->RotatedRasToRasMatrixCached->Element, outPoint, outPoint);
}

//----------------------------------------------------------------------------
void vtkOrientedGridTransform::ForwardTransformDerivative(const double inPoint[3],
                                                  double outPoint[3],
                                                  double derivative[3][3])
{
  if (this->GridDirectionMatrix==NULL)
    {
    this->Superclass::ForwardTransformDerivative(inPoint,outPoint,derivative);
    return;
    }
  vtkLinearTransformPoint(this->RasToRotatedRasMatrixCached->Element, inPoint, outPoint);
  this->Superclass::ForwardTransformDerivative(outPoint,outPoint,derivative);
  vtkLinearTransformPoint(this->RotatedRasToRasMatrixCached->Element, outPoint, outPoint);
  vtkLinearTransformDerivative(this->RotatedRasToRasMatrixCached->Element, derivative, derivative);
}

//----------------------------------------------------------------------------
void vtkOrientedGridTransform::InverseTransformDerivative(const double inPoint[3],
                                                  double outPoint[3],
                                                  double derivative[3][3])
{
  if (this->GridDirectionMatrix==NULL)
    {
    this->Superclass::InverseTransformDerivative(inPoint,outPoint,derivative);
    return;
    }

  vtkLinearTransformPoint(this->RasToRotatedRasMatrixCached->Element, inPoint, outPoint);
  this->Superclass::InverseTransformDerivative(outPoint,outPoint,derivative);
  vtkLinearTransformPoint(this->RotatedRasToRasMatrixCached->Element, outPoint, outPoint);
  vtkLinearTransformDerivative(this->RotatedRasToRasMatrixCached->Element, derivative, derivative);
}

//----------------------------------------------------------------------------
void vtkOrientedGridTransform::InternalDeepCopy(vtkAbstractTransform *transform)
{
  vtkOrientedGridTransform *gridTransform = (vtkOrientedGridTransform *)transform;

  this->SetGridDirectionMatrix(gridTransform->GetGridDirectionMatrix());

  // Cached matrices will be recomputed automatically in InternalUpdate()
  // therefore we do not need to copy them.

  this->Superclass::InternalDeepCopy(transform);
}

//----------------------------------------------------------------------------
void vtkOrientedGridTransform::InternalUpdate()
{
  this->Superclass::InternalUpdate();

  // Pre-compute RasToRotatedRasMatrixCached transform and store it in a member variable
  // to avoid recomputing it each time a point is transformed.

  vtkNew<vtkMatrix4x4> ijkToRas;
  ijkToRas->SetElement(0,0,this->GridSpacing[0]);
  ijkToRas->SetElement(1,1,this->GridSpacing[1]);
  ijkToRas->SetElement(2,2,this->GridSpacing[2]);
  vtkMatrix4x4::Multiply4x4(ijkToRas.GetPointer(),this->GridDirectionMatrix,ijkToRas.GetPointer());
  ijkToRas->SetElement(0,3,this->GridOrigin[0]);
  ijkToRas->SetElement(1,3,this->GridOrigin[1]);
  ijkToRas->SetElement(2,3,this->GridOrigin[2]);
  vtkNew<vtkMatrix4x4> rasToIjk;
  vtkMatrix4x4::Invert(ijkToRas.GetPointer(),rasToIjk.GetPointer());

  vtkNew<vtkMatrix4x4> ijkToRotatedRas;
  ijkToRotatedRas->SetElement(0,0,this->GridSpacing[0]);
  ijkToRotatedRas->SetElement(1,1,this->GridSpacing[1]);
  ijkToRotatedRas->SetElement(2,2,this->GridSpacing[2]);
  ijkToRotatedRas->SetElement(0,3,this->GridOrigin[0]);
  ijkToRotatedRas->SetElement(1,3,this->GridOrigin[1]);
  ijkToRotatedRas->SetElement(2,3,this->GridOrigin[2]);

  vtkNew<vtkMatrix4x4> rasToRotatedRas;
  vtkMatrix4x4::Multiply4x4(ijkToRotatedRas.GetPointer(),rasToIjk.GetPointer(),this->RasToRotatedRasMatrixCached);

  vtkMatrix4x4::Invert(this->RasToRotatedRasMatrixCached,this->RotatedRasToRasMatrixCached);
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkOrientedGridTransform::MakeTransform()
{
  return vtkOrientedGridTransform::New();
}
