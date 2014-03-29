#include "vtkTransformVisualizerGlyph3D.h"

// STD includes
#include <vector>

// VTK includes
#include <vtkGlyph3D.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkCellData.h>
#include <vtkCell.h>
#include <vtkDataSet.h>
#include <vtkFloatArray.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTransform.h>
#include <vtkUnsignedCharArray.h>

#include <vtkMinimalStandardRandomSequence.h>

vtkStandardNewMacro(vtkTransformVisualizerGlyph3D);

//------------------------------------------------------------------------------
vtkTransformVisualizerGlyph3D::vtkTransformVisualizerGlyph3D()
{
  this->Scaling = 1;
  this->ColorMode = VTK_COLOR_BY_SCALE;
  this->ScaleMode = VTK_SCALE_BY_SCALAR;
  this->ScaleFactor = 1.0;
  this->ScaleDirectional = true;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;
  this->Orient = 1;
  this->VectorMode = VTK_USE_VECTOR;
  this->Clamping = 0;
  this->IndexMode = VTK_INDEXING_OFF;
  this->GeneratePointIds = 0;
  this->PointIdsName = NULL;
  this->SetPointIdsName("InputPointIds");
  this->SetNumberOfInputPorts(2);
  this->FillCellData = 0;
  this->SourceTransform = 0;

  this->MagnitudeThresholding = 0;
  this->MagnitudeThresholdLower = 0.0;
  this->MagnitudeThresholdUpper = 100.0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  // by default process active point vectors
  this->SetInputArrayToProcess(1,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
  // by default process active point normals
  this->SetInputArrayToProcess(2,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::NORMALS);
  // by default process active point scalars
  this->SetInputArrayToProcess(3,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
int vtkTransformVisualizerGlyph3D::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd;
  vtkDataArray *inSScalars; // Scalars for Scaling
  vtkDataArray *inCScalars; // Scalars for Coloring
  vtkDataArray *inVectors;
  vtkIdType numPts, numSourcePts, numSourceCells, inPtId, i;
  int index;
  vtkPoints *sourcePts = NULL;
  vtkSmartPointer<vtkPoints> transformedSourcePts = vtkSmartPointer<vtkPoints>::New();
  vtkPoints *newPts;
  vtkDataArray *newScalars=NULL;
  vtkDataArray *newVectors=NULL;
  double x[3], v[3], vNew[3], s = 0.0, vMag = 0.0, value, tc[3];
  vtkTransform *trans = vtkTransform::New();
  vtkCell *cell;
  vtkIdList *cellPts;
  int npts;
  vtkIdList *pts;
  vtkIdType ptIncr, cellIncr, cellId;
  double scalex,scaley,scalez, den;
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();
  int numberOfSources = this->GetNumberOfInputConnections(1);
  vtkPolyData *defaultSource = NULL;
  vtkIdTypeArray *pointIds=0;
  vtkPolyData *source = 0;

  vtkDebugMacro(<<"Generating glyphs");

  pts = vtkIdList::New();
  pts->Allocate(VTK_CELL_SIZE);

  pd = input->GetPointData();
  inSScalars = this->GetInputArrayToProcess(0,inputVector);
  inVectors = this->GetInputArrayToProcess(1,inputVector);

  vtkDataArray* temp = 0;
  if (pd)
    {
    temp = pd->GetArray("vtkGhostLevels");
    //else return?
    }

  numPts = input->GetNumberOfPoints();
  if (numPts < 1)
    {
    vtkDebugMacro(<<"No points to glyph!");
    pts->Delete();
    trans->Delete();
    return 1;
    }

  // Check input for consistency
  if ( (den = this->Range[1] - this->Range[0]) == 0.0 )
    {
    den = 1.0;
    }

  // Allocate storage for output PolyData
  outputPD->CopyVectorsOff();
  outputPD->CopyNormalsOff();
  outputPD->CopyTCoordsOff();

  source = this->GetSource(0, inputVector[1]);
  sourcePts = source->GetPoints();
  numSourcePts = sourcePts->GetNumberOfPoints();
  numSourceCells = source->GetNumberOfCells();

  // Prepare to copy output.
  pd = input->GetPointData();
  outputPD->CopyAllocate(pd,numPts*numSourcePts);

  newPts = vtkPoints::New();
  newPts->Allocate(numPts*numSourcePts);

  newScalars = vtkFloatArray::New();
  newScalars->Allocate(numPts*numSourcePts);
  newScalars->SetName("DisplacementMagnitude");

  newVectors = vtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors->Allocate(3*numPts*numSourcePts);
  newVectors->SetName("DisplacementVector");

  // Setting up for calls to PolyData::InsertNextCell()
  output->Allocate(this->GetSource(0, inputVector[1]), 3*numPts*numSourceCells, numPts*numSourceCells);

  transformedSourcePts->SetDataTypeToDouble();
  transformedSourcePts->Allocate(numSourcePts);

  // Traverse all Input points, transforming Source points and copying
  // point attributes.
  ptIncr=0;
  cellIncr=0;
  for (inPtId=0; inPtId < numPts; inPtId++)
    {
    scalex = scaley = scalez = 1.0;
    if ( ! (inPtId % 10000) )
      {
      this->UpdateProgress(static_cast<double>(inPtId)/numPts);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    // Get the scalar and vector data
    if ( inSScalars )
      {
      s = inSScalars->GetComponent(inPtId, 0);
      }

    vtkDataArray *array3D = inVectors;

    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
    array3D->GetTuple(inPtId, v);
    vMag = vtkMath::Norm(v);

    if (this->MagnitudeThresholding && (vMag<this->MagnitudeThresholdLower || vMag>this->MagnitudeThresholdUpper))
    {
      continue;
    }

    scalex = scaley = scalez = vMag;

    // Compute index into table of glyphs (or not?!)
    index = 0;

    // Now begin copying/transforming glyph
    trans->Identity();

    // Copy all topology (transformation independent)
    for (cellId=0; cellId < numSourceCells; cellId++)
      {
      cell = this->GetSource(index, inputVector[1])->GetCell(cellId);
      cellPts = cell->GetPointIds();
      npts = cellPts->GetNumberOfIds();
      for (pts->Reset(), i=0; i < npts; i++)
        {
        pts->InsertId(i,cellPts->GetId(i) + ptIncr);
        }
      output->InsertNextCell(cell->GetCellType(),pts);
      }

    // translate Source to Input point
    input->GetPoint(inPtId, x);
    trans->Translate(x[0], x[1], x[2]);

    // Copy Input vector
    for (i=0; i < numSourcePts; i++)
      {
      newVectors->InsertTuple(i+ptIncr, v);
      }
    if (this->Orient && (vMag > 0.0))
      {
      // if there is no y or z component
      if ( v[1] == 0.0 && v[2] == 0.0 )
        {
        if (v[0] < 0) //just flip x if we need to
          {
          trans->RotateWXYZ(180.0,0,1,0);
          }
        }
      else
        {
        vNew[0] = (v[0]+vMag) / 2.0;
        vNew[1] = v[1] / 2.0;
        vNew[2] = v[2] / 2.0;
        trans->RotateWXYZ(180.0,vNew[0],vNew[1],vNew[2]);
        }
      }

    //Colour by vector
    for (i=0; i < numSourcePts; i++)
      {
      newScalars->InsertTuple(i+ptIncr, &vMag);
      }


    // Scale data if appropriate
    if ( this->Scaling )
      {
      scalex *= this->ScaleFactor;
      scaley *= this->ScaleFactor;
      scalez *= this->ScaleFactor;

      if ( scalex == 0.0 )
        {
        scalex = 1.0e-10;
        }
      if ( scaley == 0.0 )
        {
        scaley = 1.0e-10;
        }
      if ( scalez == 0.0 )
        {
        scalez = 1.0e-10;
        }

      if ( this->ScaleDirectional )
        {
        scaley = 1;
        scalez = 1;
        }

      trans->Scale(scalex,scaley,scalez);
      }

    // Multiply points and normals by resulting matrix
    if (this->SourceTransform)
      {
      transformedSourcePts->Reset();
      this->SourceTransform->TransformPoints(sourcePts, transformedSourcePts);
      trans->TransformPoints(transformedSourcePts, newPts);
      }
    else
      {
      trans->TransformPoints(sourcePts,newPts);
      }

    // Copy point data from source
    for (i=0; i < numSourcePts; i++)
      {
      outputPD->CopyData(pd,inPtId,ptIncr+i);
      }

    ptIncr += numSourcePts;
    cellIncr += numSourceCells;
    }

  // Update ourselves and release memory
  output->SetPoints(newPts);
  newPts->Delete();

  if (newScalars)
    {
    int idx = outputPD->AddArray(newScalars);
    outputPD->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
    }

  if (newVectors)
    {
    outputPD->SetVectors(newVectors);
    newVectors->Delete();
    }

  output->Squeeze();
  trans->Delete();
  pts->Delete();

  return 1;
}

//------------------------------------------------------------------------------
void vtkTransformVisualizerGlyph3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}