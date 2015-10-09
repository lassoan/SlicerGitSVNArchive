/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, PMH.
  and was partially funded by OCAIRO and Sparkit.

==============================================================================*/

#ifndef __vtkMRMLTableNode_h
#define __vtkMRMLTableNode_h

#include <string>
#include <vector>

#include "vtkMRMLStorableNode.h"

// MRML Includes
class vtkMRMLStorageNode;

// VTK Includes
class vtkTable;

/// \brief MRML node to represent a table object
///
/// It is meant to be a replacement to the vtkMRMLDoubleArrayNode
/// to store anything table like.
class VTK_MRML_EXPORT vtkMRMLTableNode : public vtkMRMLStorableNode
{
public:
  static vtkMRMLTableNode *New();
  vtkTypeMacro(vtkMRMLTableNode,vtkMRMLStorableNode);

  void PrintSelf(ostream& os, vtkIndent indent);

  //----------------------------------------------------------------
  /// Standard methods for MRML nodes
  //----------------------------------------------------------------

  virtual vtkMRMLNode* CreateNodeInstance();

  ///
  /// Set node attributes
  virtual void ReadXMLAttributes( const char** atts);

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName()
    {return "Table";};

  ///
  /// Method to propagate events generated in mrml
  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData );

  //----------------------------------------------------------------
  /// Get and Set Macros
  //----------------------------------------------------------------
  virtual void SetAndObserveTable(vtkTable* table);
  vtkGetObjectMacro ( Table, vtkTable );

  ///
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode();

  //----------------------------------------------------------------
  /// Constructor and destroctor
  //----------------------------------------------------------------
 protected:
  vtkMRMLTableNode();
  ~vtkMRMLTableNode();
  vtkMRMLTableNode(const vtkMRMLTableNode&);
  void operator=(const vtkMRMLTableNode&);


 protected:
  //----------------------------------------------------------------
  /// Data
  //----------------------------------------------------------------

  vtkTable* Table;
};

#endif

