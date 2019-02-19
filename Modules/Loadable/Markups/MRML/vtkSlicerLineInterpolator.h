/*=========================================================================

 Copyright (c) ProxSim ltd., Kwun Tong, Hong Kong. All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 This file was originally developed by Davide Punzo, punzodavide@hotmail.it,
 and development was supported by ProxSim ltd.

=========================================================================*/

/**
 * @class   vtkSlicerLineInterpolator
 * @brief   Defines API for interpolating/modifying control points
 *
 * vtkSlicerLineInterpolator is an abstract base class for interpolators
 * that are used by the vtkSlicerRepresentation class to interpolate
 * and/or modify control points in a Slicer. Subclasses must override the virtual
 * method \c InterpolateLine. This is used by the markups curve nodes
 * to give the interpolator a chance to define an interpolation scheme
 * between control points. See vtkSlicerBezierLineInterpolator for a concrete
 * implementation. Subclasses may also override \c UpdateControlPoint. This provides
 * a way for the representation to give the interpolator a chance to modify
 * the control points, as the user constructs curves. For instance, a sticky
 * curve widget may be implemented that moves nodes to nearby regions of
 * high gradient, to be used in image-guided segmentation.
*/

#ifndef vtkSlicerLineInterpolator_h
#define vtkSlicerLineInterpolator_h

#include "vtkSlicerMarkupsModuleMRMLExport.h"
#include "vtkObject.h"

#include "vtkMRMLMarkupsNode.h"

class vtkRenderer;
class vtkSlicerAbstractWidgetRepresentation;
class vtkIntArray;

class VTK_SLICER_MARKUPS_MODULE_MRML_EXPORT vtkSlicerLineInterpolator : public vtkObject
{
public:
  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerLineInterpolator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Subclasses that wish to interpolate a line segment must implement this.
  /// For instance vtkSlicerBezierLineInterpolator adds control points between idx1
  /// and idx2, that allow the Slicer to adhere to a bezier curve.
  virtual int InterpolateLine(vtkMRMLMarkupsNode::ControlPointsListType& controlPoints,
    bool closedLoop, int idx1, int idx2) = 0;

  /// Span of the interpolator. ie. the number of control points its supposed
  /// to interpolate given a node.
  ///
  /// The first argument is the current nodeIndex.
  /// ie, you'd be trying to interpolate between nodes "nodeIndex" and
  /// "nodeIndex-1", unless you're closing the Slicer in which case, you're
  /// trying to interpolate "nodeIndex" and "Node=0".
  ///
  /// The node span is returned in a vtkIntArray. The default node span is 1
  /// (ie. nodeIndices is a 2 tuple (nodeIndex, nodeIndex-1)). However, it
  /// need not always be 1. For instance, cubic spline interpolators, which
  /// have a span of 3 control points, it can be larger. See
  /// vtkSlicerBezierLineInterpolator for instance.
  virtual void GetSpan(int nodeIndex, vtkIntArray *nodeIndices,
    vtkMRMLMarkupsNode::ControlPointsListType& controlPoints, bool closedLoop);

 protected:
  vtkSlicerLineInterpolator();
  ~vtkSlicerLineInterpolator() override;

private:
  vtkSlicerLineInterpolator(const vtkSlicerLineInterpolator&) = delete;
  void operator=(const vtkSlicerLineInterpolator&) = delete;
};

#endif
