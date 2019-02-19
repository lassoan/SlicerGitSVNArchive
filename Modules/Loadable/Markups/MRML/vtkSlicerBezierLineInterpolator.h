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
 * @class   vtkSlicerBezierLineInterpolator
 * @brief   Interpolates supplied nodes with bezier line segments
 *
 * The line interpolator interpolates supplied nodes (see InterpolateLine)
 * with Bezier line segments. The fitness of the curve may be controlled using
 * SetMaximumCurveError and SetMaximumNumberOfLineSegments.
 *
 * @sa
 * vtkSlicerLineInterpolator
*/

#ifndef vtkSlicerBezierLineInterpolator_h
#define vtkSlicerBezierLineInterpolator_h

#include "vtkSlicerMarkupsModuleMRMLExport.h"
#include "vtkSlicerLineInterpolator.h"

class VTK_SLICER_MARKUPS_MODULE_MRML_EXPORT vtkSlicerBezierLineInterpolator
                          : public vtkSlicerLineInterpolator
{
public:
  /// Instantiate this class.
  static vtkSlicerBezierLineInterpolator *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkSlicerBezierLineInterpolator, vtkSlicerLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Interpolate the line between two control points.
  int InterpolateLine(vtkMRMLMarkupsNode::ControlPointsListType& controlPoints,
    bool closedLoop, int idx1, int idx2) VTK_OVERRIDE;

  void GetSpan(int nodeIndex, vtkIntArray *nodeIndices,
    vtkMRMLMarkupsNode::ControlPointsListType& controlPoints, bool closedLoop) VTK_OVERRIDE;

  /// The difference between a line segment connecting two points and the curve
  /// connecting the same points. In the limit of the length of the curve
  /// dx -> 0, the two values will be the same. The smaller this number, the
  /// finer the bezier curve will be interpolated. Default is 0.005
  vtkSetClampMacro(MaximumCurveError, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MaximumCurveError, double);

  /// Maximum number of bezier line segments between two nodes. Larger values
  /// create a finer interpolation. Default is 100.
  vtkSetClampMacro(MaximumCurveLineSegments, int, 1, 1000);
  vtkGetMacro(MaximumCurveLineSegments, int);

protected:
  vtkSlicerBezierLineInterpolator();
  ~vtkSlicerBezierLineInterpolator() VTK_OVERRIDE;

  /// Get the nth node's slope. Will return
  /// 1 on success, or 0 if there are not at least
  /// (n+1) nodes (0 based counting).
  int GetNthNodeSlope(vtkMRMLMarkupsNode::ControlPointsListType& controlPoints,
    bool closedLoop, int n, double slope[3]);

  void ComputeMidpoint(double p1[3], double p2[3], double mid[3])
  {
      mid[0] = (p1[0] + p2[0])/2;
      mid[1] = (p1[1] + p2[1])/2;
      mid[2] = (p1[2] + p2[2])/2;
  }

  double MaximumCurveError;
  int    MaximumCurveLineSegments;

private:
  vtkSlicerBezierLineInterpolator(const vtkSlicerBezierLineInterpolator&) = delete;
  void operator=(const vtkSlicerBezierLineInterpolator&) = delete;
};

#endif
