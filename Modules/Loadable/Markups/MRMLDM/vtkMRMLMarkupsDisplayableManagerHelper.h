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

// STL includes
#include <set>

class vtkMRMLAbstractDisplayableManager;
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

  vtkGetObjectMacro(DisplayableManager, vtkMRMLAbstractDisplayableManager);
  void SetDisplayableManager(vtkMRMLAbstractDisplayableManager*);

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

  /// Get a vtkSlicerAbstractWidget* given a node
  vtkSlicerAbstractWidget * GetWidget(vtkMRMLMarkupsDisplayNode * markupsDisplayNode);
  /// Get first visible widget for this markup
  vtkSlicerAbstractWidget * GetWidget(vtkMRMLMarkupsNode * markupsNode);

  /// Remove all widgets, intersection widgets, nodes
  void RemoveAllWidgetsAndNodes();

  /// Map of vtkWidget indexed using associated node ID
  typedef std::map < vtkMRMLMarkupsDisplayNode*, vtkSlicerAbstractWidget* > DisplayNodeToWidgetType;
  typedef std::map < vtkMRMLMarkupsDisplayNode*, vtkSlicerAbstractWidget* >::iterator DisplayNodeToWidgetIt;
  DisplayNodeToWidgetType MarkupsDisplayNodesToWidgets;

  typedef std::map < vtkMRMLMarkupsNode*, std::set< vtkMRMLMarkupsDisplayNode* > > MarkupsToDisplayType;
  typedef std::map < vtkMRMLMarkupsNode*, std::set< vtkMRMLMarkupsDisplayNode* > >::iterator  MarkupsToDisplayIt;
  MarkupsToDisplayType MarkupsToDisplayNodes;


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

  void AddMarkupsNode(vtkMRMLMarkupsNode* node);
  void RemoveMarkupsNode(vtkMRMLMarkupsNode* node);
  void AddDisplayNode(vtkMRMLMarkupsDisplayNode* displayNode);
  void RemoveDisplayNode(vtkMRMLMarkupsDisplayNode* displayNode);

  void AddObservations(vtkMRMLMarkupsNode* node);
  void RemoveObservations(vtkMRMLMarkupsNode* node);

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

  bool AddingMarkupsNode;

  vtkMRMLAbstractDisplayableManager* DisplayableManager;
};

#endif /* VTKMRMLMARKUPSDISPLAYABLEMANAGERHELPER_H_ */
