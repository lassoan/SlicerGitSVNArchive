/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __qMRMLTableViewControllerWidget_p_h
#define __qMRMLTableViewControllerWidget_p_h

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Slicer API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// qMRML includes
#include "qMRMLTableViewControllerWidget.h"
#include "qMRMLViewControllerBar_p.h"
#include "ui_qMRMLTableViewControllerWidget.h"

// VTK includes
#include <vtkWeakPointer.h>

class QAction;
class qMRMLSceneViewMenu;
class vtkMRMLTableViewNode;
class vtkMRMLTableNode;
class QString;

//-----------------------------------------------------------------------------
class qMRMLTableViewControllerWidgetPrivate
  : public qMRMLViewControllerBarPrivate
  , public Ui_qMRMLTableViewControllerWidget
{
  Q_OBJECT
  QVTK_OBJECT
  Q_DECLARE_PUBLIC(qMRMLTableViewControllerWidget);

public:
  typedef qMRMLViewControllerBarPrivate Superclass;
  qMRMLTableViewControllerWidgetPrivate(qMRMLTableViewControllerWidget& object);
  virtual ~qMRMLTableViewControllerWidgetPrivate();

  virtual void init();

  vtkWeakPointer<vtkMRMLTableNode> TableNode;
  vtkWeakPointer<vtkMRMLTableViewNode> TableViewNode;
  qMRMLTableView* TableView;

  QString TableViewLabel;

  QAction* CopyAction;
  QAction* PasteAction;

public slots:
  void onTableNodeSelected(vtkMRMLNode* node);
  void onLockTableButtonClicked();
  void insertColumn();
  void deleteColumn();
  void insertRow();
  void deleteRow();
  void setFirstRowLocked(bool locked);
  void setFirstColumnLocked(bool locked);
  void copySelection();
  void pasteSelection();

protected:
  virtual void setupPopupUi();

public:

};

#endif
