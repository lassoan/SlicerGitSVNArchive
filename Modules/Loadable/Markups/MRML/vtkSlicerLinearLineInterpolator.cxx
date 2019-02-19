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

#include "vtkSlicerLinearLineInterpolator.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSlicerLinearLineInterpolator);

//----------------------------------------------------------------------
vtkSlicerLinearLineInterpolator::vtkSlicerLinearLineInterpolator() = default;

//----------------------------------------------------------------------
vtkSlicerLinearLineInterpolator::~vtkSlicerLinearLineInterpolator() = default;

//----------------------------------------------------------------------
int vtkSlicerLinearLineInterpolator::InterpolateLine(
  vtkMRMLMarkupsNode::ControlPointsListType& controlPoints, bool closedLoop,
  int idx1, int idx2)
{
  controlPoints[idx1]->IntermediatePositions.push_back(controlPoints[idx1]->Position);
  controlPoints[idx2]->IntermediatePositions.push_back(controlPoints[idx2]->Position);
  return 1;
}

//----------------------------------------------------------------------
void vtkSlicerLinearLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
