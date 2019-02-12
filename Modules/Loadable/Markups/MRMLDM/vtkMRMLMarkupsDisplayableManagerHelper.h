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
/// .NAME vtkMRMLMarkupsDisplayableManagerHelper - utility class to manage widgets
/// .SECTION Description
/// This class defines lists that are used to manage the widgets associated with markups.
/// A markup which is managed by a displayableManager consists of
///   a) the Markups MRML Node (MarkupsNodeList)
///   b) the vtkWidget to show this markup (Widgets)
///   c) a vtkWidget to represent sliceIntersections in the slice viewers (WidgetIntersections)
///


#ifndef VTKMRMLMARKUPSDISPLAYABLEMANAGERHELPER_H_
#define VTKMRMLMARKUPSDISPLAYABLEMANAGERHELPER_H_

// MarkupsModule includes
#include "vtkSlicerMarkupsModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRML includes
#include <vtkMRMLMarkupsNode.h>

// VTK includes
#include <vtkSlicerAbstractWidget.h>
#include <vtkSmartPointer.h>

// MRML includes
#include <vtkMRMLSliceNode.h>

class vtkMRMLMarkupsDisplayNode;
class vtkMRMLInteractionNode;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsDisplayableManagerHelper :
    public vtkObject
{
public:

  static vtkMRMLMarkupsDisplayableManagerHelper *New();
  vtkTypeMacro(vtkMRMLMarkupsDisplayableManagerHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Lock/Unlock all widgets based on the state of the nodes
  void UpdateLockedAllWidgetsFromNodes();
  /// Lock/Unlock all widgets
  void UpdateLockedAllWidgets(bool locked);
  /// Lock/Unlock a widget. If no interaction node is passed in, don't take the
  /// mouse mode into account, if it is passed in, widgets get locked while in
  /// Place mode
  void UpdateLocked(vtkMRMLMarkupsDisplayNode* node);

  /// Update all widget status
  void UpdateAllWidgetsFromInteractionNode(vtkMRMLInteractionNode* interactionNode);

  /// Set all widget status to manipulate
  void SetAllWidgetsToManipulate();

  /// Keep track of the mapping between widgets and nodes
  void RecordWidgetForNode(vtkSlicerAbstractWidget* widget, vtkMRMLMarkupsDisplayNode *node);

  /// Get a vtkSlicerAbstractWidget* given a node
  vtkSlicerAbstractWidget * GetWidget(vtkMRMLMarkupsDisplayNode * node);

  /// Remove all widgets, intersection widgets, nodes
  void RemoveAllWidgetsAndNodes();
  /// Remove a node, its widget and its intersection widget
  void RemoveWidgetAndNode(vtkMRMLMarkupsDisplayNode *node);

  /// List of nodes managed by the DisplayableManager
  std::vector<vtkMRMLMarkupsDisplayNode*> MarkupsDisplayNodeList;

  /// Typedef for iterator over the list of nodes managed by the DisplayableManager
  typedef std::vector<vtkMRMLMarkupsDisplayNode*>::iterator MarkupsNodeListIt;

  /// Map of vtkWidget indexed using associated node ID
  std::map<vtkMRMLMarkupsDisplayNode*, vtkSlicerAbstractWidget*> Widgets;

  /// .. and its associated convenient typedef
  typedef std::map<vtkMRMLMarkupsDisplayNode*, vtkSlicerAbstractWidget*>::iterator WidgetsIt;

  //----------------------------------------------------------------------------------

  /*
  /// Get the seed glyph type for the given display node.
  /// Returns -1 if not found
  int GetNodeGlyphType(vtkMRMLNode *displayNode, int index);
  /// Set the glyph type for the given display node, making a new entry if the
  /// display node or the index are out of bounds
  void SetNodeGlyphType(vtkMRMLNode *displayNode, int glyphType, int index);
  /// Remove the entry for this display node
  void RemoveNodeGlyphType(vtkMRMLNode *displayNode);
  /// Clear out the saved list of glyph types, called on scene close or node removed
  void ClearNodeGlyphTypes();
  */

protected:

  vtkMRMLMarkupsDisplayableManagerHelper();
  virtual ~vtkMRMLMarkupsDisplayableManagerHelper();

  /// utility method to print out the current glyph types
  void PrintNodeGlyphTypes();

private:

  vtkMRMLMarkupsDisplayableManagerHelper(const vtkMRMLMarkupsDisplayableManagerHelper&); /// Not implemented
  void operator=(const vtkMRMLMarkupsDisplayableManagerHelper&); /// Not Implemented

  /// Keep a record of the current glyph type for the handles in the widget
  /// associated with this node, prevents changing them unnecessarily
  std::map<vtkMRMLNode*, std::vector<int> > NodeGlyphTypes;
};

#endif /* VTKMRMLMARKUPSDISPLAYABLEMANAGERHELPER_H_ */
