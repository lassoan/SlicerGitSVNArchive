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
  /// Table contents cannot be edited through the user interface
  vtkGetMacro(Locked, bool);
  vtkSetMacro(Locked, bool);

  ///
  /// Rows are allowed be sorted based on values in a selected column
  vtkGetMacro(Sortable, bool);
  vtkSetMacro(Sortable, bool);

  /// First column should be treated as row label
  vtkGetMacro(LabelInFirstColumn, bool);
  vtkSetMacro(LabelInFirstColumn, bool);

  ///
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode();

  /// Add an array to the table as a new column.
  /// If no column is provided then an empty column is added.
  /// If a column is provided then the number of rows in the table and tuples in
  /// the provided array will be matched by either adding empty rows to the table
  /// or empty elements to the array.
  /// If a column is provided that does not have a name then a name will be generated
  /// automatically that is unique among all table column names.
  vtkAbstractArray* AddColumn(vtkAbstractArray* column = 0);

  /// Remove array from the table.
  /// Returns with true on success.
  bool RemoveColumn(int columnIndex);

  /// Add an empty row at the end of the table
  /// Returns the index of the inserted row or -1 on failure.
  int AddEmptyRow();

  /// Remove row from the table
  /// Returns with true on success.
  bool RemoveRow(int rowIndex);

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
  bool Locked;
  bool Sortable;
  bool LabelInFirstColumn;
};

#endif

