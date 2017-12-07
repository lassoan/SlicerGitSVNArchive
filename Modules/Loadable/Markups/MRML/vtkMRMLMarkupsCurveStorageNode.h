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

/// Markups Module MRML storage nodes
///
/// vtkMRMLMarkupsCurveStorageNode - MRML node for markups curve storage
///
/// vtkMRMLMarkupsCurveStorageNode nodes describe the markups storage
/// node that allows to read/write curve data from/to file.

#ifndef __vtkMRMLMarkupsCurveStorageNode_h
#define __vtkMRMLMarkupsCurveStorageNode_h

// Markups includes
#include "vtkSlicerMarkupsModuleMRMLExport.h"
#include "vtkMRMLMarkupsStorageNode.h"

class vtkMRMLMarkupsNode;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRML_EXPORT vtkMRMLMarkupsCurveStorageNode : public vtkMRMLMarkupsStorageNode
{
public:
  static vtkMRMLMarkupsCurveStorageNode *New();
  vtkTypeMacro(vtkMRMLMarkupsCurveStorageNode,vtkMRMLMarkupsStorageNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "MarkupsCurveStorage";};

  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  virtual bool CanReadInReferenceNode(vtkMRMLNode *refNode) VTK_OVERRIDE;

protected:
  vtkMRMLMarkupsCurveStorageNode();
  ~vtkMRMLMarkupsCurveStorageNode();
  vtkMRMLMarkupsCurveStorageNode(const vtkMRMLMarkupsCurveStorageNode&);
  void operator=(const vtkMRMLMarkupsCurveStorageNode&);

  virtual void InitializeSupportedReadFileTypes() VTK_OVERRIDE;

  virtual void InitializeSupportedWriteFileTypes() VTK_OVERRIDE;

  virtual int ReadDataInternal(vtkMRMLNode *refNode) VTK_OVERRIDE;

  virtual int WriteDataInternal(vtkMRMLNode *refNode) VTK_OVERRIDE;

};

#endif
