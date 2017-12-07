/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsCurveStorageNode.h"
#include "vtkMRMLMarkupsNode.h"

#include "vtkMRMLScene.h"
#include "vtkSlicerVersionConfigure.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include <vtksys/SystemTools.hxx>

#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsCurveStorageNode);

//----------------------------------------------------------------------------
vtkMRMLMarkupsCurveStorageNode::vtkMRMLMarkupsCurveStorageNode()
{
  //this->DefaultWriteFileExtension = "ccsv";
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsCurveStorageNode::~vtkMRMLMarkupsCurveStorageNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveStorageNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveStorageNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveStorageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveStorageNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}

//----------------------------------------------------------------------------
bool vtkMRMLMarkupsCurveStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLMarkupsCurveNode");
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsCurveStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsCurveStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveStorageNode::InitializeSupportedReadFileTypes()
{
  //this->SupportedReadFileTypes->InsertNextValue("Markups Curve CSV (.ccsv)");
  //this->SupportedReadFileTypes->InsertNextValue("Annotation Fiducial CSV (.acsv)");
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveStorageNode::InitializeSupportedWriteFileTypes()
{
  //this->SupportedWriteFileTypes->InsertNextValue("Markups Curve CSV (.ccsv)");
}
