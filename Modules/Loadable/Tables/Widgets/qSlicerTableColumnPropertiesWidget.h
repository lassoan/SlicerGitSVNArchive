/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Matthew Holden, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerTableColumnPropertiesWidget_h
#define __qSlicerTableColumnPropertiesWidget_h

// Qt includes
#include "qSlicerWidget.h"

#include "qMRMLUtils.h"

// Tables Widgets includes
#include "qSlicerTablesModuleWidgetsExport.h"
#include "ui_qSlicerTableColumnPropertiesWidget.h"


class qSlicerTableColumnPropertiesWidgetPrivate;

/// \ingroup Slicer_QtModules_CreateModels
class Q_SLICER_MODULE_TABLES_WIDGETS_EXPORT
qSlicerTableColumnPropertiesWidget : public qSlicerWidget
{
  Q_OBJECT
  /*
  Q_PROPERTY(bool enterPlaceModeOnNodeChange READ enterPlaceModeOnNodeChange WRITE setEnterPlaceModeOnNodeChange)
  Q_PROPERTY(bool jumpToSliceEnabled READ jumpToSliceEnabled WRITE setJumpToSliceEnabled)
  Q_PROPERTY(bool nodeSelectorVisible READ nodeSelectorVisible WRITE setNodeSelectorVisible)
  Q_PROPERTY(bool optionsVisible READ optionsVisible WRITE setOptionsVisible)
  Q_PROPERTY(QColor nodeColor READ nodeColor WRITE setNodeColor)
  Q_PROPERTY(QColor defaultNodeColor READ defaultNodeColor WRITE setDefaultNodeColor)
  Q_PROPERTY(int viewGroup READ viewGroup WRITE setViewGroup)
  */

public:
  typedef qSlicerWidget Superclass;
  qSlicerTableColumnPropertiesWidget(QWidget *parent=0);
  virtual ~qSlicerTableColumnPropertiesWidget();

  /// Get the currently selected table node.
  Q_INVOKABLE vtkMRMLNode* currentNode() const;

  Q_INVOKABLE void setColumnProperty(QString propertyName, QString propertyValue);

  Q_INVOKABLE QString columnProperty(QString propertyName) const;
  /*
  /// Show/hide the table node selector widget.
  bool nodeSelectorVisible() const;

  /// Show/hide options (place, activate, color, etc buttons).
  bool optionsVisible() const;

  /// Get the selected color of the currently selected table node.
  QColor nodeColor() const;

  /// Get the default node color that is applied to newly created nodes.
  QColor defaultNodeColor() const;

  /// Set views where slice positions will be updated on jump to slice.
  /// If it is set to -1 (by default it is) then all slices will be jumped.
  void setViewGroup(int newViewGroup);

  /// Get view group where slice positions will be updated.
  int viewGroup()const;
  */
public slots:

  void setMRMLScene(vtkMRMLScene* scene);

  /// Set the currently selected table node.
  void setCurrentNode(vtkMRMLNode* currentNode);

  /*
  /// Accessors to control place mode behavior
  void setEnterPlaceModeOnNodeChange(bool);

  /// If enabled then the fiducial will be shown in all slice views when a fiducial is selected
  void setJumpToSliceEnabled(bool);

  /// Show/hide the table node selector widget.
  void setNodeSelectorVisible(bool);

  /// Show/hide options (place, activate, color, etc buttons).
  void setOptionsVisible(bool);

  /// Set the selected color of the currently selected table node.
  void setNodeColor(QColor color);

  /// Set the default color that is assigned to newly created table nodes in the combo box.
  void setDefaultNodeColor(QColor color);

  /// Scrolls to and selects the Nth fiducial in the table of fiducials.
  void highlightNthFiducial(int n);

  /// Set the currently selected table node to be the active table node in the Slicer scene.
  void activate();

  /// Set the currently selected table node to be the active table node in the Slicer scene.
  /// The argument \a place is true then also interaction mode is set to place mode.
  void placeActive(bool place);
  */
protected slots:

/*
  /// Update the widget when a different table node is selected by the combo box.
  void onTableFiducialNodeChanged();
  /// Setup a newly created markups node - add display node, set color.
  void onTableFiducialNodeAdded(vtkMRMLNode*);
  /// Create context menu for the table displaying the currently selected markups node.
  void onTableFiducialTableContextMenu(const QPoint& position);

  /// Edit the name or position of the currently selected markups node.
  void onTableFiducialEdited(int row, int column);

  /// Clicked on a fiducial or used keyboard to move between fiducials in the table.
  void onTableFiducialSelected(int row, int column);
  */

  /// Update the GUI to reflect the currently selected table node.
  void updateWidget();

signals:

  /// The signal is emitted when a different markup node is selected.
  //void tableNodeChanged();

  /// This signal is emitted if updates to the widget have finished.
  /// It is called after fiducials are changed (added, position modified, etc).
  void updateFinished();

protected:
  QScopedPointer<qSlicerTableColumnPropertiesWidgetPrivate> d_ptr;

  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerTableColumnPropertiesWidget);
  Q_DISABLE_COPY(qSlicerTableColumnPropertiesWidget);

};

#endif
