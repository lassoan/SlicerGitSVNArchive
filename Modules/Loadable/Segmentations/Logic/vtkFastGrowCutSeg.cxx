#include "vtkFastGrowCutSeg.h"


#include <QProgressBar>
#include <QMainWindow>
#include <QStatusBar>
#include "qSlicerApplication.h"

const unsigned short SrcDimension = 3;
typedef float DistPixelType;  // float type pixel for cost function
typedef short SrcPixelType;
typedef unsigned char LabPixelType;

#include <iostream>
#include <vector>

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include "itkImage.h"
#include "itkTimeProbe.h"

#include "fibheap.h"

//vtkCxxRevisionMacro(vtkFastGrowCutSeg, "$Revision$"); //necessary?
vtkStandardNewMacro(vtkFastGrowCutSeg); //for the new() macro

//----------------------------------------------------------------------------

#include <math.h>
#include <queue>
#include <set>
#include <vector>
#include <stdlib.h>
#include <fstream>
#include <iterator>

#include "FibHeap.h"

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
  double GetKeyValue() { return N; }
  void SetKeyValue(double n) { N = n; }
  long int GetIndexValue() { return IndexV; }
  void SetIndexValue(long int v) { IndexV = v; }
};

void HeapNode::Print()
{
  FibHeapNode::Print();
  //    mexPrintf( "%f (%d)" , N , IndexV );
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
  if (FHN_Cmp(RHS)) return 0;
  return N == ((HeapNode&)RHS).N ? 1 : 0;
}

int  HeapNode::operator <(FibHeapNode& RHS)
{
  int X;
  if ((X = FHN_Cmp(RHS)) != 0)
    return X < 0 ? 1 : 0;
  return N < ((HeapNode&)RHS).N ? 1 : 0;
}


//----------------------------------------------------------------------------
template <typename T> void CalculateEffectiveExtent(vtkImageData* image, int effectiveExtent[6], T threshold = 0, int padding = 17)
{
  // Get increments to march through image
  int *wholeExt = image->GetExtent();

  effectiveExtent[0] = wholeExt[1] + 1;
  effectiveExtent[1] = wholeExt[0] - 1;
  effectiveExtent[2] = wholeExt[3] + 1;
  effectiveExtent[3] = wholeExt[2] - 1;
  effectiveExtent[4] = wholeExt[5] + 1;
  effectiveExtent[5] = wholeExt[4] - 1;

  if (image->GetScalarPointer() == NULL)
  {
    // no image data is allocated, return with empty extent
    return;
  }

  // Loop through output pixels
  for (int k = wholeExt[4]; k <= wholeExt[5]; k++)
  {
    for (int j = wholeExt[2]; j <= wholeExt[3]; j++)
    {
      bool currentLineInEffectiveExtent = (k >= effectiveExtent[4] && k <= effectiveExtent[5] && j >= effectiveExtent[2] && j <= effectiveExtent[3]);
      int i = wholeExt[0];
      T* imagePtr = static_cast<T*>(image->GetScalarPointer(i, j, k));
      int firstSegmentEnd = currentLineInEffectiveExtent ? effectiveExtent[0] : wholeExt[1];
      for (; i <= firstSegmentEnd; i++)
      {
        if (*(imagePtr++) > threshold)
        {
          if (i < effectiveExtent[0]) { effectiveExtent[0] = i; }
          if (i > effectiveExtent[1]) { effectiveExtent[1] = i; }
          if (j < effectiveExtent[2]) { effectiveExtent[2] = j; }
          if (j > effectiveExtent[3]) { effectiveExtent[3] = j; }
          if (k < effectiveExtent[4]) { effectiveExtent[4] = k; }
          if (k > effectiveExtent[5]) { effectiveExtent[5] = k; }
          currentLineInEffectiveExtent = true;
          break;
        }
      }
      if (!currentLineInEffectiveExtent)
      {
        // We haven't found any non-empty voxel in this line
        continue;
      }
      // Now we need to find the other end of the extent: the last non-empty voxel in the line.
      // The fastest way to find it is to start backward search from the end of the line.
      i = wholeExt[1];
      imagePtr = static_cast<T*>(image->GetScalarPointer(i, j, k));
      for (; i > effectiveExtent[1]; i--)
      {
        if (*(imagePtr--)>threshold)
        {
          if (i < effectiveExtent[0]) { effectiveExtent[0] = i; }
          if (i > effectiveExtent[1]) { effectiveExtent[1] = i; }
          if (j < effectiveExtent[2]) { effectiveExtent[2] = j; }
          if (j > effectiveExtent[3]) { effectiveExtent[3] = j; }
          if (k < effectiveExtent[4]) { effectiveExtent[4] = k; }
          if (k > effectiveExtent[5]) { effectiveExtent[5] = k; }
          break;
        }
      }
    }
  }

  for (int kk = 0; kk < 3; kk++)
  {
    effectiveExtent[kk * 2] -= padding;
    effectiveExtent[kk * 2 + 1] += padding;
    if (effectiveExtent[kk * 2] < wholeExt[kk * 2])
    {
      effectiveExtent[kk * 2] = wholeExt[kk * 2];
    }
    if (effectiveExtent[kk * 2 + 1] > wholeExt[kk * 2 + 1])
    {
      effectiveExtent[kk * 2 + 1] = wholeExt[kk * 2 + 1];
    }
  }
}

template<typename PixelType, typename VectorPixelType>
void ExtractVTKImageROI(vtkImageData* image, const int extent[6], std::vector<VectorPixelType>& imROIVec)
{
  if (image->GetScalarPointer() == NULL)
  {
    return;
  }
  imROIVec.clear();
  int imSize[3] = { 0 };
  for (int i = 0; i < 3; i++)
  {
    imSize[i] = extent[i * 2 + 1] - extent[i * 2] + 1;
  }
  imROIVec.resize(imSize[0] * imSize[1] * imSize[2]);
  int index = 0;
  for (int k = extent[4]; k <= extent[5]; k++)
  {
    for (int j = extent[2]; j <= extent[3]; j++)
    {
      int i = extent[0];
      PixelType* imagePtr = static_cast<PixelType*>(image->GetScalarPointer(i, j, k));
      int segmentEnd = extent[1];
      for (; i <= segmentEnd; i++)
      {
        imROIVec[index++] = *(imagePtr++);
      }
    }
  }
}

template<typename PixelType, typename VectorPixelType>
void UpdateVTKImageROI(const std::vector<VectorPixelType>& imROIVec, const int extent[6], vtkImageData* image)
{
  // Set non-ROI as zeros
  memset((PixelType*)(image->GetScalarPointer()), 0, image->GetScalarSize()*image->GetNumberOfPoints());
  int index = 0;
  for (int k = extent[4]; k <= extent[5]; k++)
  {
    for (int j = extent[2]; j <= extent[3]; j++)
    {
      int i = extent[0];
      PixelType* imagePtr = static_cast<PixelType*>(image->GetScalarPointer(i, j, k));
      int segmentEnd = extent[1];
      for (; i <= segmentEnd; i++)
      {
        *(imagePtr++) = imROIVec[index++];
      }
    }
  }
  image->Modified();
}

template<typename SrcPixelType, typename LabPixelType>
class FastGrowCut
{
public:
  FastGrowCut()
  {
    m_heap = NULL;
    m_hpNodes = NULL;
    m_bSegInitialized = false;
  };
  ~FastGrowCut()
  {
    if (m_heap != NULL)
    {
      delete m_heap;
      m_heap = NULL;
    }
    if (m_hpNodes != NULL)
    {
      delete[]m_hpNodes;
      m_hpNodes = NULL;
    }
  };

  void SetSourceImage(vtkImageData* image, int extent[6])
  {
    switch (image->GetScalarType())
    {
      vtkTemplateMacro((ExtractVTKImageROI<VTK_TT, SrcPixelType>(image, extent, m_imSrc)));
      break;
    }
  }
  void SetSeedImage(vtkImageData* image, int extent[6])
  {
    switch (image->GetScalarType())
    {
      vtkTemplateMacro((ExtractVTKImageROI<VTK_TT, LabPixelType>(image, extent, m_imSeed)));
      break;
    }
  }
  void GetLabeImage(vtkImageData* image, int extent[6])
  {
    switch (image->GetScalarType())
    {
      vtkTemplateMacro((UpdateVTKImageROI<VTK_TT, LabPixelType>(m_imLab, extent, image)));
      break;
    }
  }

void SetWorkMode(bool bSegInitialized = false)
  {
    m_bSegInitialized = bSegInitialized;
  };

  void SetImageSize(const int imSize[3])
  {
    m_imSize[0] = imSize[0];
    m_imSize[1] = imSize[1];
    m_imSize[2] = imSize[2];
  };
  bool DoSegmentation()
  {
    if (!InitializationAHP())
    {
      return false;
    }
    DijkstraBasedClassificationAHP();
    return true;
  };

private:
  bool InitializationAHP();
  void DijkstraBasedClassificationAHP();

  std::vector<SrcPixelType> m_imSrc;
  std::vector<LabPixelType> m_imSeed;
  std::vector<LabPixelType> m_imLabPre;
  std::vector<FPixelType> m_imDistPre;
  std::vector<LabPixelType> m_imLab;
  std::vector<FPixelType> m_imDist;

  int m_imSize[3];
  long m_DIMX, m_DIMY, m_DIMZ, m_DIMXY, m_DIMXYZ;
  std::vector<int> m_indOff;
  std::vector<unsigned char>  m_NBSIZE;

  FibHeap *m_heap;
  HeapNode *m_hpNodes;
  bool m_bSegInitialized;
};

template<typename SrcPixelType, typename LabPixelType>
bool FastGrowCut<SrcPixelType, LabPixelType>
::InitializationAHP()
{
  m_DIMX = m_imSize[0];
  m_DIMY = m_imSize[1];
  m_DIMZ = m_imSize[2];
  m_DIMXY = m_DIMX*m_DIMY;
  m_DIMXYZ = m_DIMXY*m_DIMZ;

  std::cout << "DIM = [" << m_DIMX << ", " << m_DIMY << ", " << m_DIMZ << "]" << std::endl;


  if ((m_heap = new FibHeap) == NULL || (m_hpNodes = new HeapNode[m_DIMXYZ + 1]) == NULL)
  {
    std::cerr << "Memory allocation failed-- ABORTING.\n";
    return false;
  }
  m_heap->ClearHeapOwnership();

  long  i, j, k, index;
  if (!m_bSegInitialized)
  {
    m_imLab.resize(m_DIMXYZ);
    m_imDist.resize(m_DIMXYZ);
    m_imLabPre.resize(m_DIMXYZ);
    m_imDistPre.resize(m_DIMXYZ);
    // Compute index offset
    m_indOff.clear();
    long ix, iy, iz;
    for (ix = -1; ix <= 1; ix++)
      for (iy = -1; iy <= 1; iy++)
        for (iz = -1; iz <= 1; iz++)
        {
          if (!(ix == 0 && iy == 0 && iz == 0))
          {
            m_indOff.push_back(ix + iy*m_DIMX + iz*m_DIMXY);
          }
        }

    // Determine neighborhood size at each vertex
    m_NBSIZE = std::vector<unsigned char>(m_DIMXYZ, 0);
    for (i = 1; i < m_DIMX - 1; i++)
      for (j = 1; j < m_DIMY - 1; j++)
        for (k = 1; k < m_DIMZ - 1; k++)
        {
          index = i + j*m_DIMX + k*m_DIMXY;
          m_NBSIZE[index] = NNGBH;
        }

    for (index = 0; index < m_DIMXYZ; index++)
    {
      m_imLab[index] = m_imSeed[index];
      if (m_imLab[index] == 0)
      {
        m_hpNodes[index] = (float)DIST_INF;
        m_imDist[index] = DIST_INF;
      }
      else
      {
        m_hpNodes[index] = (float)DIST_EPSILON;
        m_imDist[index] = DIST_EPSILON;
      }

      m_heap->Insert(&m_hpNodes[index]);
      m_hpNodes[index].SetIndexValue(index);
    }
  }
  else
  {
    for (index = 0; index < m_DIMXYZ; index++)
    {
      if (m_imSeed[index] != 0 && m_imSeed[index] != m_imLabPre[index])
      {
        //             if(m_imSeed[index] != 0 && (m_imDistPre[index] != 0 ||  (m_imDistPre[index] == 0 && m_imSeed[index] != m_imLabPre[index]))) {
        m_hpNodes[index] = (float)DIST_EPSILON;
        m_imDist[index] = DIST_EPSILON;
        m_imLab[index] = m_imSeed[index];
      }
      else
      {
        m_hpNodes[index] = (float)DIST_INF;
        m_imDist[index] = DIST_INF;
        m_imLab[index] = 0;
      }
      m_heap->Insert(&m_hpNodes[index]);
      m_hpNodes[index].SetIndexValue(index);
    }
  }
  return true;
}

template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::DijkstraBasedClassificationAHP()
{
  HeapNode *hnMin, hnTmp;
  float t, tOri, tSrc;
  long i, index, indexNgbh;
  LabPixelType labSrc;
  SrcPixelType pixCenter;

  // Insert 0 then extract it, which will balance heap
  m_heap->Insert(&hnTmp); m_heap->ExtractMin();

  long k = 0; // it could be used for early termination of segmentation

  if (m_bSegInitialized)
  {
    // Adaptive Dijkstra
    while (!m_heap->IsEmpty())
    {
      hnMin = (HeapNode *)m_heap->ExtractMin();
      index = hnMin->GetIndexValue();
      tSrc = hnMin->GetKeyValue();

      // stop propagation when the new distance is larger than the previous one
      if (tSrc == DIST_INF) break;
      if (tSrc > m_imDistPre[index])
      {
        m_imDist[index] = m_imDistPre[index];
        m_imLab[index] = m_imLabPre[index];
        continue;
      }

      labSrc = m_imLab[index];
      m_imDist[index] = tSrc;

      // Update neighbors
      pixCenter = m_imSrc[index];
      for (i = 0; i < m_NBSIZE[index]; i++)
      {
        indexNgbh = index + m_indOff[i];
        tOri = m_imDist[indexNgbh];
        t = std::abs(pixCenter - m_imSrc[indexNgbh]) + tSrc;
        if (tOri > t)
        {
          m_imDist[indexNgbh] = t;
          m_imLab[indexNgbh] = labSrc;

          hnTmp = m_hpNodes[indexNgbh];
          hnTmp.SetKeyValue(t);
          m_heap->DecreaseKey(&m_hpNodes[indexNgbh], hnTmp);
        }

      }

      k++;
    }

    // Update previous labels and distance information
    for (long i = 0; i < m_DIMXYZ; i++)
    {
      if (m_imDist[i] < DIST_INF)
      {
        m_imLabPre[i] = m_imLab[i];
        m_imDistPre[i] = m_imDist[i];
      }
    }
    std::copy(m_imLabPre.begin(), m_imLabPre.end(), m_imLab.begin());
    std::copy(m_imDistPre.begin(), m_imDistPre.end(), m_imDist.begin());
  }
  else
  {
    // Normal Dijkstra (to be used in initializing the segmenter for the current image)
    while (!m_heap->IsEmpty())
    {
      hnMin = (HeapNode *)m_heap->ExtractMin();
      index = hnMin->GetIndexValue();
      tSrc = hnMin->GetKeyValue();
      labSrc = m_imLab[index];
      m_imDist[index] = tSrc;

      // Update neighbors
      pixCenter = m_imSrc[index];
      for (i = 0; i < m_NBSIZE[index]; i++)
      {
        indexNgbh = index + m_indOff[i];
        tOri = m_imDist[indexNgbh];
        t = std::abs(pixCenter - m_imSrc[indexNgbh]) + tSrc;
        if (tOri > t)
        {
          m_imDist[indexNgbh] = t;
          m_imLab[indexNgbh] = labSrc;

          hnTmp = m_hpNodes[indexNgbh];
          hnTmp.SetKeyValue(t);
          m_heap->DecreaseKey(&m_hpNodes[indexNgbh], hnTmp);
        }
      }
      k++;
    }
  }

  std::copy(m_imLab.begin(), m_imLab.end(), m_imLabPre.begin());
  std::copy(m_imDist.begin(), m_imDist.end(), m_imDistPre.begin());

  // Release memory
  if (m_heap != NULL)
  {
    delete m_heap;
    m_heap = NULL;
  }
  if (m_hpNodes != NULL)
  {
    delete[]m_hpNodes;
    m_hpNodes = NULL;
  }
}

//----------------------------------------------------------------------------
class vtkFastGrowCutSeg::vtkInternal
{
public:
  vtkInternal()
  {
    this->FastGC = NULL;
  };
  virtual ~vtkInternal()
  {
    if (this->FastGC != NULL)
    {
      delete this->FastGC;
      this->FastGC = NULL;
    }
  };

  FastGrowCut<SrcPixelType, LabPixelType> *FastGC;

  int SegmentationRoiExtent[6];
};


vtkFastGrowCutSeg::vtkFastGrowCutSeg()
{
  this->Internal = new vtkInternal();
  SourceVol = NULL;
  SeedVol = NULL;
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}


vtkFastGrowCutSeg::~vtkFastGrowCutSeg()
{
  //these functions decrement reference count on the vtkImageData's (incremented by the SetMacros)
  if (this->SourceVol)
  {
    this->SetSourceVol(NULL);
  }

  if (this->SeedVol)
  {
    this->SetSeedVol(NULL);
  }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
template< class SourceVolType, class SeedVolType>
void ExecuteGrowCut2(vtkFastGrowCutSeg::vtkInternal *self,
  vtkImageData *sourceVol,
  vtkImageData *seedVol,
  vtkImageData *outData,
  vtkInformation* outInfo)
{
  bool InitializationFlag = false;
  if (self->FastGC == NULL)
  {
    self->FastGC = new FastGrowCut<SrcPixelType, LabPixelType>();
  }

  // Find ROI
  if (!InitializationFlag)
  {
    CalculateEffectiveExtent<SeedVolType>(seedVol, self->SegmentationRoiExtent);
  }

  // Initialize FastGrowCut
  int imSize[3] = { 0 };
  for (int i = 0; i < 3; i++)
  {
    imSize[i] = self->SegmentationRoiExtent[i * 2 + 1] - self->SegmentationRoiExtent[i * 2] + 1;
  }

  self->FastGC->SetSourceImage(sourceVol, self->SegmentationRoiExtent);
  self->FastGC->SetSeedImage(seedVol, self->SegmentationRoiExtent);
  self->FastGC->SetImageSize(imSize);
  self->FastGC->SetWorkMode(InitializationFlag);

  // Do Segmentation
  self->FastGC->DoSegmentation();
  self->FastGC->GetLabeImage(outData, self->SegmentationRoiExtent);

}

//----------------------------------------------------------------------------
template <class SourceVolType>
void ExecuteGrowCut(
  vtkFastGrowCutSeg::vtkInternal *self,
  vtkImageData *sourceVol,
  vtkImageData *seedVol,
  vtkImageData *outData,
  vtkInformation* outInfo)
{
  switch (seedVol->GetScalarType())
  {
    vtkTemplateMacro((ExecuteGrowCut2<SourceVolType, VTK_TT>(
      self,
      sourceVol,
      seedVol,
      outData,
      outInfo)));
  default:
    vtkGenericWarningMacro("vtkOrientedImageDataResample::MergeImage: Unknown ScalarType");
  }
}

//-----------------------------------------------------------------------------
void vtkFastGrowCutSeg::ExecuteDataWithInformation(
  vtkDataObject *outData, vtkInformation* outInfo)
{
  vtkImageData *sourceVol = vtkImageData::SafeDownCast(GetInput(0));
  vtkImageData *seedVol = vtkImageData::SafeDownCast(GetInput(1));
  vtkImageData *outVol = vtkImageData::SafeDownCast(outData);

  int outExt[6];
  double spacing[3], origin[3];

  sourceVol->GetOrigin(origin);
  sourceVol->GetSpacing(spacing);
  sourceVol->GetExtent(outExt);

  outVol->SetOrigin(origin);
  outVol->SetSpacing(spacing);
  outVol->SetExtent(outExt);
  outVol->AllocateScalars(outInfo);

  vtkFastGrowCutSeg::vtkInternal *self = this->Internal;
  switch (sourceVol->GetScalarType())
  {
    vtkTemplateMacro(ExecuteGrowCut<VTK_TT>(self, sourceVol, seedVol,
      outVol, outInfo));
    break;
  }
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

void vtkFastGrowCutSeg::Initialization()
{
}


void vtkFastGrowCutSeg::RunFGC()
{

}

void vtkFastGrowCutSeg::PrintSelf(ostream &os, vtkIndent indent)
{
  std::cout << "This function has been found" << std::endl;
}
