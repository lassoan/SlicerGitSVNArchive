#include "vtkFastGrowCutSeg.h"

const unsigned short SrcDimension = 3;
typedef float DistPixelType;  // float type pixel for cost function
const int VTKDistPixelType = VTK_FLOAT;

#include <iostream>
#include <vector>

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkLoggingMacros.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTimerLog.h>

#include "FibHeap.h"

vtkStandardNewMacro(vtkFastGrowCutSeg);

//----------------------------------------------------------------------------
const float  DIST_INF = std::numeric_limits<float>::max();
const float  DIST_EPSILON = 1e-3;
unsigned char NNGBH = 26; // 3x3x3 pixel neighborhood (center voxel is not included)
typedef float FPixelType;

//----------------------------------------------------------------------------

class HeapNode : public FibHeapNode
{
  float   N;
  long IndexV;

public:
  HeapNode() : FibHeapNode() { N = 0; }
  virtual void operator =(FibHeapNode& RHS);
  virtual int  operator ==(FibHeapNode& RHS);
  virtual int  operator <(FibHeapNode& RHS);
  virtual void operator =(double NewKeyVal);
  virtual void Print();
  float GetKeyValue() { return N; }
  void SetKeyValue(float n) { N = n; }
  long int GetIndexValue() { return IndexV; }
  void SetIndexValue(long int v) { IndexV = v; }
};

void HeapNode::Print()
{
  FibHeapNode::Print();
}

void HeapNode::operator =(double NewKeyVal)
{
  HeapNode Tmp;
  Tmp.N = N = NewKeyVal;
  FHN_Assign(Tmp);
}

void HeapNode::operator =(FibHeapNode& RHS)
{
  FHN_Assign(RHS);
  N = ((HeapNode&)RHS).N;
}

int  HeapNode::operator ==(FibHeapNode& RHS)
{
  if (FHN_Cmp(RHS))
  {
    return 0;
  }
  return N == ((HeapNode&)RHS).N ? 1 : 0;
}

int  HeapNode::operator <(FibHeapNode& RHS)
{
  int X;
  if ((X = FHN_Cmp(RHS)) != 0)
  {
    return X < 0 ? 1 : 0;
  }
  return N < ((HeapNode&)RHS).N ? 1 : 0;
}

//----------------------------------------------------------------------------
class vtkFastGrowCutSeg::vtkInternal
{
public:
  vtkInternal();
  virtual ~vtkInternal();

  void Reset();

  template<typename SrcPixelType, typename LabPixelType>
  bool InitializationAHP(vtkImageData *sourceVol, vtkImageData *seedVol);

  template<typename SrcPixelType, typename LabPixelType>
  void DijkstraBasedClassificationAHP(vtkImageData *sourceVol, vtkImageData *seedVol);

  template <class SourceVolType>
  bool ExecuteGrowCut(vtkImageData *sourceVol, vtkImageData *seedVol, vtkImageData *outputVol);

  template< class SourceVolType, class SeedVolType>
  bool ExecuteGrowCut2(vtkImageData *sourceVol, vtkImageData *seedVol);

  vtkSmartPointer<vtkImageData> m_imDist;
  vtkSmartPointer<vtkImageData> m_imDistPre;
  vtkSmartPointer<vtkImageData> m_imLab;
  vtkSmartPointer<vtkImageData> m_imLabPre;

  long m_DIMX;
  long m_DIMY;
  long m_DIMZ;
  long m_DIMXYZ;
  std::vector<long> m_indOff;
  std::vector<unsigned char>  m_NBSIZE;

  FibHeap *m_heap;
  //std::vector<HeapNode> m_hpNodes;
  HeapNode *m_hpNodes;
  bool m_bSegInitialized;
};

//-----------------------------------------------------------------------------
vtkFastGrowCutSeg::vtkInternal::vtkInternal()
{
  m_heap = NULL;
  m_hpNodes = NULL;
  m_bSegInitialized = false;
  m_imDist = vtkSmartPointer<vtkImageData>::New();
  m_imDistPre = vtkSmartPointer<vtkImageData>::New();
  m_imLab = vtkSmartPointer<vtkImageData>::New();
  m_imLabPre = vtkSmartPointer<vtkImageData>::New();
};

//-----------------------------------------------------------------------------
vtkFastGrowCutSeg::vtkInternal::~vtkInternal()
{
  this->Reset();
};

//-----------------------------------------------------------------------------
void vtkFastGrowCutSeg::vtkInternal::Reset()
{
  if (m_heap != NULL)
  {
    delete m_heap;
    m_heap = NULL;
  }
  //m_hpNodes.clear();
  if (m_hpNodes != NULL)
  {
    delete[]m_hpNodes;
    m_hpNodes = NULL;
  }
  m_bSegInitialized = false;
  m_imDist->Initialize();
  m_imDistPre->Initialize();
  m_imLab->Initialize();
  m_imLabPre->Initialize();
}

//-----------------------------------------------------------------------------
template<typename SrcPixelType, typename LabPixelType>
bool vtkFastGrowCutSeg::vtkInternal::InitializationAHP(vtkImageData *sourceVol, vtkImageData *seedVol)
{
  m_heap = new FibHeap;
  //m_hpNodes.resize(m_DIMXYZ + 1);
  if ((m_hpNodes = new HeapNode[m_DIMXYZ + 1]) == NULL)
  {
    vtkGenericWarningMacro("Memory allocation failed. Dimensions: " << m_DIMX << "x" << m_DIMY << "x" << m_DIMZ);
    return false;
  }
  SrcPixelType* imSrc = static_cast<SrcPixelType*>(sourceVol->GetScalarPointer());
  LabPixelType* imSeed = static_cast<LabPixelType*>(seedVol->GetScalarPointer());

  if (!m_bSegInitialized)
  {
    m_imLab->SetOrigin(seedVol->GetOrigin());
    m_imLab->SetSpacing(seedVol->GetSpacing());
    m_imLab->SetExtent(seedVol->GetExtent());
    m_imLab->AllocateScalars(seedVol->GetScalarType(), 1);
    m_imDist->SetOrigin(seedVol->GetOrigin());
    m_imDist->SetSpacing(seedVol->GetSpacing());
    m_imDist->SetExtent(seedVol->GetExtent());
    m_imDist->AllocateScalars(VTKDistPixelType, 1);
    m_imLabPre->SetExtent(0, -1, 0, -1, 0, -1);
    m_imLabPre->AllocateScalars(seedVol->GetScalarType(), 1);
    m_imDistPre->SetExtent(0, -1, 0, -1, 0, -1);
    m_imDistPre->AllocateScalars(VTKDistPixelType, 1);
    LabPixelType* imLab = static_cast<LabPixelType*>(m_imLab->GetScalarPointer());
    DistPixelType* imDist = static_cast<DistPixelType*>(m_imDist->GetScalarPointer());

    // Compute index offset
    m_indOff.clear();
    for (int iz = -1; iz <= 1; iz++)
    {
      for (int iy = -1; iy <= 1; iy++)
      {
        for (int ix = -1; ix <= 1; ix++)
        {
          if (ix == 0 && iy == 0 && iz == 0)
          {
            continue;
          }
          m_indOff.push_back(long(ix) + m_DIMX*(long(iy) + m_DIMY*long(iz)));
        }
      }
    }

    // Determine neighborhood size at each vertex
    // (everywhere NNGBH except at the edges of the cube)
    m_NBSIZE.resize(m_DIMXYZ);
    unsigned char* nbSizePtr = &(m_NBSIZE[0]);
    for (int z = 0; z < m_DIMZ; z++)
    {
      bool zEdge = (z == 0 || z == m_DIMZ - 1);
      for (int y = 0; y < m_DIMY; y++)
      {
        bool yEdge = (y == 0 || y == m_DIMY - 1);
        *(nbSizePtr++) = 0; // x == 0 (there is always padding, so we don't need to check if m_DIMX>0)
        unsigned char nbSize = (zEdge || yEdge) ? 0 : NNGBH;
        for (int x = m_DIMX-2; x > 0; x--)
        {
          *(nbSizePtr++) = nbSize;
        }
        *(nbSizePtr++) = 0; // x == m_DIMX-1 (there is always padding, so we don't need to check if m_DIMX>1)
      }
    }

    for (long index = 0; index < m_DIMXYZ; index++)
    {
      LabPixelType seedValue = imSeed[index];
      imLab[index] = seedValue;
      if (seedValue == 0)
      {
        m_hpNodes[index] = DIST_INF;
        imDist[index] = DIST_INF;
      }
      else
      {
        m_hpNodes[index] = DIST_EPSILON;
        imDist[index] = DIST_EPSILON;
      }
      m_heap->Insert(&m_hpNodes[index]);
      m_hpNodes[index].SetIndexValue(index);
    }
  }
  else
  {
    // Already initialized
    LabPixelType* imLab = static_cast<LabPixelType*>(m_imLab->GetScalarPointer());
    LabPixelType* imLabPre = static_cast<LabPixelType*>(m_imLabPre->GetScalarPointer());
    DistPixelType* imDist = static_cast<DistPixelType*>(m_imDist->GetScalarPointer());

    for (long index = 0; index < m_DIMXYZ; index++)
    {
      if (imSeed[index] != 0 && imSeed[index] != imLabPre[index])
      {
        // if(imSeed[index] != 0 && (imDistPre[index] != 0 ||  (imDistPre[index] == 0 && imSeed[index] != imLabPre[index]))) {
        m_hpNodes[index] = (float)DIST_EPSILON;
        imDist[index] = DIST_EPSILON;
        imLab[index] = imSeed[index];
      }
      else
      {
        m_hpNodes[index] = (float)DIST_INF;
        imDist[index] = DIST_INF;
        imLab[index] = 0;
      }
      m_heap->Insert(&m_hpNodes[index]);
      m_hpNodes[index].SetIndexValue(index);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
template<typename SrcPixelType, typename LabPixelType>
void vtkFastGrowCutSeg::vtkInternal::DijkstraBasedClassificationAHP(vtkImageData *sourceVol, vtkImageData *seedVol)
{
  LabPixelType* imLab = static_cast<LabPixelType*>(m_imLab->GetScalarPointer());
  SrcPixelType* imSrc = static_cast<SrcPixelType*>(sourceVol->GetScalarPointer());

  // Insert 0 then extract it, which will balance heap
  {
    HeapNode hnTmp;
    m_heap->Insert(&hnTmp);
    m_heap->ExtractMin();
  }

  long k = 0; // it could be used for early termination of segmentation

  if (m_bSegInitialized)
  {
    LabPixelType* imLabPre = static_cast<LabPixelType*>(m_imLabPre->GetScalarPointer());

    // Adaptive Dijkstra
    HeapNode hnTmp;
    while (!m_heap->IsEmpty())
    {
      DistPixelType* imDistPre = static_cast<DistPixelType*>(m_imDistPre->GetScalarPointer());
      DistPixelType* imDist = static_cast<DistPixelType*>(m_imDist->GetScalarPointer());

      HeapNode* hnMin = (HeapNode *)m_heap->ExtractMin();
      float tSrc = hnMin->GetKeyValue();

      // Stop of minimum value is infinite
      if (tSrc == DIST_INF)
      {
        for (long index = 0; index < m_DIMXYZ; index++)
        {
          if (imLab[index] == 0)
          {
            imLab[index] = imLabPre[index];
            imDist[index] = imDistPre[index];
          }
        }
        break;
      }

      // Stop propagation when the new distance is larger than the previous one
      long index = hnMin->GetIndexValue();
      if (tSrc > imDistPre[index])
      {
        imDist[index] = imDistPre[index];
        imLab[index] = imLabPre[index];
        continue;
      }

      LabPixelType labSrc = imLab[index];
      imDist[index] = tSrc;

      // Update neighbors
      DistPixelType pixCenter = imSrc[index];
      unsigned char nbSize = m_NBSIZE[index];
      for (unsigned char i = 0; i < nbSize; i++)
      {
        long indexNgbh = index + m_indOff[i];
        float tOri = imDist[indexNgbh];
        float t = fabs(pixCenter - imSrc[indexNgbh]) + tSrc;
        if (tOri > t)
        {
          imDist[indexNgbh] = t;
          imLab[indexNgbh] = labSrc;

          hnTmp = m_hpNodes[indexNgbh];
          hnTmp.SetKeyValue(t);
          m_heap->DecreaseKey(&m_hpNodes[indexNgbh], hnTmp);
        }
      }

      k++;
    }
  }
  else
  {
    DistPixelType* imDist = static_cast<DistPixelType*>(m_imDist->GetScalarPointer());
    LabPixelType* imLab = static_cast<LabPixelType*>(m_imLab->GetScalarPointer());

    // Normal Dijkstra (to be used in initializing the segmenter for the current image)
    HeapNode hnTmp;
    while (!m_heap->IsEmpty())
    {
      HeapNode* hnMin = (HeapNode *)m_heap->ExtractMin();
      long index = hnMin->GetIndexValue();
      float tSrc = hnMin->GetKeyValue();
      LabPixelType labSrc = imLab[index];
      imDist[index] = tSrc;

      // Update neighbors
      DistPixelType pixCenter = imSrc[index];
      unsigned char nbSize = m_NBSIZE[index];
      for (unsigned char i = 0; i < nbSize; i++)
      {
        long indexNgbh = index + m_indOff[i];
        float tOri = imDist[indexNgbh];
        DistPixelType t = fabs(pixCenter - imSrc[indexNgbh]) + tSrc;
        if (tOri > t)
        {
          imDist[indexNgbh] = t;
          imLab[indexNgbh] = labSrc;

          hnTmp = m_hpNodes[indexNgbh];
          hnTmp.SetKeyValue(t);
          m_heap->DecreaseKey(&m_hpNodes[indexNgbh], hnTmp);
        }
      }
      k++;
    }
  }

  // Update previous labels and distance information
  m_imLabPre->DeepCopy(m_imLab);
  m_imDistPre->DeepCopy(m_imDist);
  m_bSegInitialized = true;

  // Release memory
  if (m_heap != NULL)
  {
    delete m_heap;
    m_heap = NULL;
  }
  //m_hpNodes.clear();
  if (m_hpNodes != NULL)
  {
    delete[]m_hpNodes;
    m_hpNodes = NULL;
  }
}

//-----------------------------------------------------------------------------
template< class SrcPixelType, class LabPixelType>
bool vtkFastGrowCutSeg::vtkInternal::ExecuteGrowCut2(vtkImageData *sourceVol, vtkImageData *seedVol)
{
  int* imSize = sourceVol->GetDimensions();
  m_DIMX = imSize[0];
  m_DIMY = imSize[1];
  m_DIMZ = imSize[2];
  m_DIMXYZ = m_DIMX*m_DIMY*m_DIMZ;

  if (m_DIMX <= 2 || m_DIMY <= 2 || m_DIMZ <= 2)
  {
    // image is too small (there should be space for at least one voxel padding around the image)
    return false;
  }

  if (!InitializationAHP<SrcPixelType, LabPixelType>(sourceVol, seedVol))
  {
    return false;
  }
  DijkstraBasedClassificationAHP<SrcPixelType, LabPixelType>(sourceVol, seedVol);
  return true;
}

//----------------------------------------------------------------------------
template <class SourceVolType>
bool vtkFastGrowCutSeg::vtkInternal::ExecuteGrowCut(vtkImageData *sourceVol, vtkImageData *seedVol, vtkImageData *outData)
{
  // Restart growcut from scratch if image size is changed (then cached buffers cannot be reused)
  int* extent = sourceVol->GetExtent();
  double* spacing = sourceVol->GetSpacing();
  double* origin = sourceVol->GetOrigin();
  int* outExtent = m_imLab->GetExtent();
  double* outSpacing = m_imLab->GetSpacing();
  double* outOrigin = m_imLab->GetOrigin();
  const double compareTolerance = 1e-6;
  if (outExtent[0] != extent[0] || outExtent[1] != extent[1]
    || outExtent[2] != extent[2] || outExtent[3] != extent[3]
    || outExtent[4] != extent[4] || outExtent[5] != extent[5]
    || fabs(outOrigin[0] - origin[0]) > compareTolerance
    || fabs(outOrigin[1] - origin[1]) > compareTolerance
    || fabs(outOrigin[2] - origin[2]) > compareTolerance
    || fabs(outSpacing[0] - spacing[0]) > compareTolerance
    || fabs(outSpacing[1] - spacing[1]) > compareTolerance
    || fabs(outSpacing[2] - spacing[2]) > compareTolerance)
  {
    this->Reset();
  }

  bool success = false;
  switch (seedVol->GetScalarType())
  {
    vtkTemplateMacro((success = ExecuteGrowCut2<SourceVolType, VTK_TT>(sourceVol, seedVol)));
  default:
    vtkGenericWarningMacro("vtkOrientedImageDataResample::MergeImage: Unknown ScalarType");
  }

  if (success)
  {
    outData->ShallowCopy(this->m_imLab);
  }
  else
  {
    outData->Initialize();
  }
  return success;
}

//-----------------------------------------------------------------------------
vtkFastGrowCutSeg::vtkFastGrowCutSeg()
{
  this->Internal = new vtkInternal();
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkFastGrowCutSeg::~vtkFastGrowCutSeg()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkFastGrowCutSeg::ExecuteDataWithInformation(
  vtkDataObject *outData, vtkInformation* outInfo)
{
  vtkImageData *sourceVol = vtkImageData::SafeDownCast(GetInput(0));
  vtkImageData *seedVol = vtkImageData::SafeDownCast(GetInput(1));
  vtkImageData *outVol = vtkImageData::SafeDownCast(outData);

  //TODO: check if source and seed volumes have the same size, origin, spacing

  vtkNew<vtkTimerLog> logger;
  logger->StartTimer();

  switch (sourceVol->GetScalarType())
  {
    vtkTemplateMacro(this->Internal->ExecuteGrowCut<VTK_TT>(sourceVol, seedVol, outVol));
    break;
  }
  logger->StopTimer();
  vtkInfoMacro(<< "vtkFastGrowCutSeg execution time: " << logger->GetElapsedTime());
}

//-----------------------------------------------------------------------------
int vtkFastGrowCutSeg::RequestInformation(
  vtkInformation * request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(1);

  if (inInfo != NULL)
  {
    this->Superclass::RequestInformation(request, inputVector, outputVector);
  }
  return 1;
}

void vtkFastGrowCutSeg::Reset()
{
  this->Internal->Reset();
}

void vtkFastGrowCutSeg::PrintSelf(ostream &os, vtkIndent indent)
{
  std::cout << "This function has been found" << std::endl;
}
