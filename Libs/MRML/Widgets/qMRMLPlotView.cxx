/*==============================================================================

  Copyright (c) Kapteyn Astronomical Institute
  University of Groningen, Groningen, Netherlands. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Davide Punzo, Kapteyn Astronomical Institute,
  and was supported through the European Research Council grant nr. 291531.

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QToolButton>

// STD includes
#include <algorithm>
#include <sstream>
#include <vector>

// CTK includes
#include <ctkAxesWidget.h>
#include <ctkLogger.h>
#include <ctkPopupWidget.h>

// qMRML includes
#include "qMRMLColors.h"
#include "qMRMLPlotView_p.h"

// MRML includes
#include <vtkMRMLPlotDataNode.h>
#include <vtkMRMLPlotChartNode.h>
#include <vtkMRMLPlotViewNode.h>
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTableNode.h>

// VTK includes
#include <vtkAxis.h>
#include <vtkBrush.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkCollection.h>
#include <vtkColorSeries.h>
#include <vtkContextMouseEvent.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkNew.h>
#include <vtkPen.h>
#include <vtkPlot.h>
#include <vtkPlotLine.h>
#include <vtkPlotBar.h>
#include <vtkPlotPie.h>
#include <vtkPlotBox.h>
#include <vtkRenderer.h>
#include <vtkSelection.h>
#include <vtkStringArray.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>

static const char PLOT_NODE_ID_PROPERTY_NAME[] = "MRMLPlotNodeID";

//--------------------------------------------------------------------------
// qMRMLPlotViewPrivate methods

//---------------------------------------------------------------------------
qMRMLPlotViewPrivate::qMRMLPlotViewPrivate(qMRMLPlotView& object)
  : q_ptr(&object)
{
  this->MRMLScene = 0;
  this->MRMLPlotViewNode = 0;
  this->MRMLPlotChartNode = 0;
  this->ColorLogic = 0;
  this->PinButton = 0;
  this->PopupWidget = 0;
  this->UpdatingWidgetFromMRML = false;
}

//---------------------------------------------------------------------------
qMRMLPlotViewPrivate::~qMRMLPlotViewPrivate()
{
}

namespace
{
//----------------------------------------------------------------------------
template <typename T> T StringToNumber(const char* num)
{
  std::stringstream ss;
  ss << num;
  T result;
  return ss >> result ? result : 0;
}

//----------------------------------------------------------------------------
int StringToInt(const char* str)
{
  return StringToNumber<int>(str);
}

}// end namespace

//---------------------------------------------------------------------------
void qMRMLPlotViewPrivate::init()
{
  Q_Q(qMRMLPlotView);

  // Let the QWebView expand in both directions
  q->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  this->PopupWidget = new ctkPopupWidget;
  QHBoxLayout* popupLayout = new QHBoxLayout;
  popupLayout->addWidget(new QToolButton);
  this->PopupWidget->setLayout(popupLayout);

  if (!q->chart())
    {
    return;
    }

  q->chart()->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::LEFT_BUTTON);
  q->chart()->SetActionToButton(vtkChart::PAN, vtkContextMouseEvent::MIDDLE_BUTTON);
  q->chart()->SetActionToButton(vtkChart::ZOOM, vtkContextMouseEvent::RIGHT_BUTTON);

  qvtkConnect(q->chart(), vtkCommand::SelectionChangedEvent, this, SLOT(emitSelection()));

  if (!q->chart()->GetBackgroundBrush() ||
      !q->chart()->GetTitleProperties() ||
      !q->chart()->GetLegend()          ||
      !q->scene())
    {
    return;
    }

  if (!q->chart()->GetLegend()->GetLabelProperties() ||
      !q->scene()->GetRenderer())
    {
    return;
    }

  vtkColor4ub color;
  color.Set(255., 253., 246., 255.);
  q->chart()->GetBackgroundBrush()->SetColor(color);
  q->chart()->GetTitleProperties()->SetFontFamilyToArial();
  q->chart()->GetTitleProperties()->SetFontSize(20);
  q->chart()->GetLegend()->GetLabelProperties()->SetFontFamilyToArial();
  q->scene()->GetRenderer()->SetUseDepthPeeling(true);
  q->scene()->GetRenderer()->SetUseFXAA(true);

  vtkAxis* axis = q->chart()->GetAxis(vtkAxis::LEFT);
  if (axis)
    {
    axis->GetTitleProperties()->SetFontFamilyToArial();
    axis->GetTitleProperties()->SetFontSize(16);
    axis->GetTitleProperties()->SetBold(false);
    axis->GetLabelProperties()->SetFontFamilyToArial();
    axis->GetLabelProperties()->SetFontSize(12);
    }
  axis = q->chart()->GetAxis(vtkAxis::BOTTOM);
  if (axis)
    {
    axis->GetTitleProperties()->SetFontFamilyToArial();
    axis->GetTitleProperties()->SetFontSize(16);
    axis->GetTitleProperties()->SetBold(false);
    axis->GetLabelProperties()->SetFontFamilyToArial();
    axis->GetLabelProperties()->SetFontSize(12);
    }
  axis = q->chart()->GetAxis(vtkAxis::RIGHT);
  if (axis)
    {
    axis->GetTitleProperties()->SetFontFamilyToArial();
    axis->GetTitleProperties()->SetFontSize(16);
    axis->GetTitleProperties()->SetBold(false);
    axis->GetLabelProperties()->SetFontFamilyToArial();
    axis->GetLabelProperties()->SetFontSize(12);
    }
  axis = q->chart()->GetAxis(vtkAxis::TOP);
  if (axis)
    {
    axis->GetTitleProperties()->SetFontFamilyToArial();
    axis->GetTitleProperties()->SetFontSize(16);
    axis->GetTitleProperties()->SetBold(false);
    axis->GetLabelProperties()->SetFontFamilyToArial();
    axis->GetLabelProperties()->SetFontSize(12);
    }

}

//---------------------------------------------------------------------------
void qMRMLPlotViewPrivate::setMRMLScene(vtkMRMLScene* newScene)
{
  if (newScene == this->MRMLScene)
    {
    return;
    }

  this->qvtkReconnect(
    this->mrmlScene(), newScene,
    vtkMRMLScene::StartBatchProcessEvent, this, SLOT(startProcessing()));

  this->qvtkReconnect(
    this->mrmlScene(), newScene,
    vtkMRMLScene::EndBatchProcessEvent, this, SLOT(endProcessing()));

  this->MRMLScene = newScene;

  // Update chart (just in case plot view node was set before the scene)
  this->onPlotChartNodeChanged();
}

// --------------------------------------------------------------------------
vtkMRMLPlotDataNode* qMRMLPlotViewPrivate::plotDataNodeFromPlot(vtkPlot* plot)
{
  std::string nodeID = plot->GetProperty(PLOT_NODE_ID_PROPERTY_NAME).ToString();
  vtkMRMLPlotDataNode* plotDataNode = vtkMRMLPlotDataNode::SafeDownCast(this->mrmlScene()->GetNodeByID(nodeID));
  return plotDataNode;
}

// --------------------------------------------------------------------------
vtkPlot* qMRMLPlotViewPrivate::plotFromPlotDataNode(vtkMRMLPlotDataNode* plotDataNode)
{
  Q_Q(qMRMLPlotView);
  if (!plotDataNode)
    {
    return NULL;
    }
  for (int chartPlotDataNodesIndex = q->chart()->GetNumberOfPlots() - 1; chartPlotDataNodesIndex >= 0; chartPlotDataNodesIndex--)
    {
    vtkPlot *plot = q->chart()->GetPlot(chartPlotDataNodesIndex);
    if (!plot)
      {
      continue;
      }
    if (this->plotDataNodeFromPlot(plot) == plotDataNode)
      {
      return plot;
      }
    }
  return NULL;
}

// --------------------------------------------------------------------------
vtkSmartPointer<vtkPlot> qMRMLPlotViewPrivate::updatePlotFromPlotDataNode(vtkMRMLPlotDataNode* plotDataNode, vtkPlot* existingPlot)
{
  if (plotDataNode == NULL)
    {
    return NULL;
    }
  vtkMRMLTableNode* tableNode = plotDataNode->GetTableNode();
  if (tableNode == NULL || tableNode->GetTable() == NULL)
    {
    return NULL;
    }
  vtkTable *table = tableNode->GetTable();
  std::string yColumnName = plotDataNode->GetYColumnName();
  if (yColumnName.empty())
    {
    return NULL;
    }
  vtkAbstractArray* yColumn = table->GetColumnByName(yColumnName.c_str());
  if (!yColumn)
    {
    return NULL;
    }
  int yColumnType = yColumn->GetDataType();
  if (yColumnType == VTK_STRING || yColumnType == VTK_BIT)
    {
    qWarning() << Q_FUNC_INFO << ": Y column has unsupported data type: 'string' or 'bit'";
    return NULL;
    }
  std::string xColumnName = plotDataNode->GetXColumnName();
  vtkAbstractArray* xColumn = NULL;
  if (!xColumnName.empty())
    {
    xColumn = table->GetColumnByName(xColumnName.c_str());
    int xColumnType = xColumn->GetDataType();
    if (xColumnType == VTK_STRING || xColumnType == VTK_BIT)
      {
      qWarning() << Q_FUNC_INFO << ": X column has unsupported data type: 'string' or 'bit'";
      return NULL;
      }
    }

  int plotType = plotDataNode->GetPlotType();
  vtkSmartPointer<vtkPlot> newPlot = existingPlot;
  switch (plotType)
    {
    case vtkMRMLPlotDataNode::SCATTER:
      if (!existingPlot || !existingPlot->IsA("vtkPlotLine"))
        {
        newPlot = vtkSmartPointer<vtkPlotLine>::New();
        }
      break;
    case vtkMRMLPlotDataNode::BAR:
      if (!existingPlot || !existingPlot->IsA("vtkPlotBar"))
        {
        newPlot = vtkSmartPointer<vtkPlotBar>::New();
        }
      break;
    case vtkMRMLPlotDataNode::PIE:
      if (!existingPlot || !existingPlot->IsA("vtkPlotPie"))
      {
        newPlot = vtkSmartPointer<vtkPlotPie>::New();
      }
      break;
    case vtkMRMLPlotDataNode::BOX:
      if (!existingPlot || !existingPlot->IsA("vtkPlotBox"))
      {
        newPlot = vtkSmartPointer<vtkPlotBox>::New();
      }
      break;
    default:
      return NULL;
    }

  // Common properties
  newPlot->SetWidth(plotDataNode->GetLineWidth());
  double* color = plotDataNode->GetColor();
  newPlot->SetColor(color[0], color[1], color[2], 1.0);
  if (newPlot->GetPen())
    {
    newPlot->GetPen()->SetOpacityF(plotDataNode->GetOpacity());
    }

  // Type-specific properties
  vtkPlotLine* plotLine = vtkPlotLine::SafeDownCast(newPlot);
  vtkPlotBar* plotBar = vtkPlotBar::SafeDownCast(newPlot);
  vtkPlotPie* plotPie = vtkPlotPie::SafeDownCast(newPlot);
  vtkPlotBox* plotBox = vtkPlotBox::SafeDownCast(newPlot);
  if (plotLine)
    {
    plotLine->SetMarkerSize(plotDataNode->GetMarkerSize());
    plotLine->SetMarkerStyle(plotDataNode->GetMarkerStyle());
    }

  if (plotDataNode->IsXColumnIndex())
    {
    // In the case of Indexes, SetInputData still needs a proper Column.
    xColumnName = yColumnName;
    newPlot->SetUseIndexForXSeries(true);
    }
  else
    {
    newPlot->SetUseIndexForXSeries(false);
    }

  newPlot->SetInputData(table, xColumnName, yColumnName);

  return newPlot;
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::startProcessing()
{
}

//
// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::endProcessing()
{
  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
vtkMRMLScene* qMRMLPlotViewPrivate::mrmlScene()
{
  return this->MRMLScene;
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::onPlotChartNodeChanged()
{
  vtkMRMLPlotChartNode *newPlotChartNode = NULL;

  if (this->MRMLScene && this->MRMLPlotViewNode && this->MRMLPlotViewNode->GetPlotChartNodeID())
    {
    newPlotChartNode = vtkMRMLPlotChartNode::SafeDownCast
      (this->MRMLScene->GetNodeByID(this->MRMLPlotViewNode->GetPlotChartNodeID()));
    }

  this->qvtkReconnect(this->MRMLPlotChartNode, newPlotChartNode,
    vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  this->MRMLPlotChartNode = newPlotChartNode;
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::RecalculateBounds()
{
  Q_Q(qMRMLPlotView);

  if (!q->chart())
    {
    return;
    }

  q->chart()->RecalculateBounds();
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::switchSelectionMode()
{
  Q_Q(qMRMLPlotView);

  if (!q->chart())
    {
    return;
    }

  int buttonSelect, buttonSelectPoly, buttonSelectClickAndDrag;
  buttonSelect = q->chart()->GetActionToButton(vtkChart::SELECT);
  buttonSelectPoly = q->chart()->GetActionToButton(vtkChart::SELECT_POLYGON);
  buttonSelectClickAndDrag = q->chart()->GetActionToButton(vtkChart::CLICK_AND_DRAG);
  if (buttonSelect > 0 && buttonSelectPoly < 0 && buttonSelectClickAndDrag < 0)
    {
    q->chart()->SetActionToButton(vtkChart::SELECT_POLYGON, vtkContextMouseEvent::LEFT_BUTTON);
    }
  else if (buttonSelect < 0 && buttonSelectPoly > 0 && buttonSelectClickAndDrag < 0)
    {
    q->chart()->SetActionToButton(vtkChart::CLICK_AND_DRAG, vtkContextMouseEvent::LEFT_BUTTON);
    }
  else if (buttonSelect < 0 && buttonSelectPoly < 0 && buttonSelectClickAndDrag > 0)
    {
    q->chart()->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::LEFT_BUTTON);
    }
  else if (buttonSelectClickAndDrag < 0 && buttonSelectPoly < 0 && buttonSelect < 0)
    {
    q->chart()->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::LEFT_BUTTON);
  }
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::switchLeftAndMiddleClick()
{
  Q_Q(qMRMLPlotView);

  if (!q->chart())
    {
    return;
    }

  int buttonPan, buttonSelect, buttonSelectPoly, buttonSelectClickAndDrag;
  buttonPan = q->chart()->GetActionToButton(vtkChart::PAN);
  buttonSelect = q->chart()->GetActionToButton(vtkChart::SELECT);
  buttonSelectPoly = q->chart()->GetActionToButton(vtkChart::SELECT_POLYGON);
  buttonSelectClickAndDrag = q->chart()->GetActionToButton(vtkChart::CLICK_AND_DRAG);

  if (buttonPan == 2)
    {
    q->chart()->SetActionToButton(vtkChart::PAN, vtkContextMouseEvent::LEFT_BUTTON);
    if (buttonSelect == 1)
      {
      q->chart()->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::MIDDLE_BUTTON);
      }
    else if (buttonSelectPoly == 1)
      {
      q->chart()->SetActionToButton(vtkChart::SELECT_POLYGON, vtkContextMouseEvent::MIDDLE_BUTTON);
      }
    else if (buttonSelectClickAndDrag == 1)
      {
      q->chart()->SetActionToButton(vtkChart::CLICK_AND_DRAG, vtkContextMouseEvent::MIDDLE_BUTTON);
      }
    }
  else if (buttonPan == 1)
    {
    q->chart()->SetActionToButton(vtkChart::PAN, vtkContextMouseEvent::MIDDLE_BUTTON);
    if (buttonSelect == 2)
      {
      q->chart()->SetActionToButton(vtkChart::SELECT, vtkContextMouseEvent::LEFT_BUTTON);
      }
    else if (buttonSelectPoly == 2)
      {
      q->chart()->SetActionToButton(vtkChart::SELECT_POLYGON, vtkContextMouseEvent::LEFT_BUTTON);
      }
    else if (buttonSelectClickAndDrag == 2)
      {
      q->chart()->SetActionToButton(vtkChart::CLICK_AND_DRAG, vtkContextMouseEvent::LEFT_BUTTON);
      }
    }
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::emitSelection()
{
  Q_Q(qMRMLPlotView);

  if (!q->chart())
    {
    return;
    }

  const char *PlotChartNodeID = this->MRMLPlotViewNode->GetPlotChartNodeID();

  vtkMRMLPlotChartNode* plotChartNode = vtkMRMLPlotChartNode::SafeDownCast
    (this->MRMLScene->GetNodeByID(PlotChartNodeID));
  if (!plotChartNode)
    {
    return;
    }

  vtkNew<vtkStringArray> mrmlPlotDataIDs;
  vtkNew<vtkCollection> selectionCol;

  for (int plotIndex = 0; plotIndex < q->chart()->GetNumberOfPlots(); plotIndex++)
    {
    vtkPlot *plot = q->chart()->GetPlot(plotIndex);
    if (!plot)
      {
      continue;
      }
    vtkIdTypeArray *selection = plot->GetSelection();
    if (!selection)
      {
      continue;
      }

    if (selection->GetNumberOfValues() > 0)
      {
      selectionCol->AddItem(selection);
      vtkMRMLPlotDataNode* plotDataNode = this->plotDataNodeFromPlot(plot);
      if (plotDataNode)
        {
        // valid plot data node found
        mrmlPlotDataIDs->InsertNextValue(plotDataNode->GetID());
        }
      }
    }
  // emit the signal
  emit q->dataSelected(mrmlPlotDataIDs.GetPointer(), selectionCol.GetPointer());
}

// --------------------------------------------------------------------------
void qMRMLPlotViewPrivate::updateWidgetFromMRML()
{
  Q_Q(qMRMLPlotView);

  if (this->UpdatingWidgetFromMRML)
    {
    return;
    }
  this->UpdatingWidgetFromMRML = true;

  if (!this->MRMLScene || !this->MRMLPlotViewNode
      || !q->isEnabled() || !q->chart() || !q->chart()->GetLegend())
    {
    this->UpdatingWidgetFromMRML = false;
    return;
    }

  // Get the PlotChartNode
  const char *plotChartNodeID = this->MRMLPlotViewNode->GetPlotChartNodeID();
  vtkMRMLPlotChartNode* plotChartNode = vtkMRMLPlotChartNode::SafeDownCast(this->MRMLScene->GetNodeByID(plotChartNodeID));
  if (!plotChartNode)
    {
    // Clean all the plots in vtkChartXY
    while(q->chart()->GetNumberOfPlots() > 0)
      {
      // This if is necessary for a BUG at VTK level:
      // in the case of a plot removed with corner ID 0,
      // when successively the addPlot method is called
      // (to add the same plot instance to vtkChartXY) it will
      // fail to setup the graph in the vtkChartXY render.
      if (q->chart()->GetPlotCorner(q->chart()->GetPlot(0)) == 0)
        {
        q->chart()->SetPlotCorner(q->chart()->GetPlot(0), 1);
        }
      q->removePlot(q->chart()->GetPlot(0));
      }
    this->UpdatingWidgetFromMRML = false;
    return;
    }

  std::string defaultPlotColorNodeID = "vtkMRMLProceduralColorNodeRandomIntegers";
  if (this->ColorLogic)
    {
    defaultPlotColorNodeID = this->ColorLogic->GetDefaultPlotColorNodeID();
    }
  else
    {
    qWarning() << Q_FUNC_INFO << ": colorLogic is not defined for PlotView, using default color node "
      << defaultPlotColorNodeID.c_str();
    }

  vtkMRMLColorNode *defaultColorNode = vtkMRMLColorNode::SafeDownCast
    (this->MRMLScene->GetNodeByID(defaultPlotColorNodeID));
  vtkMRMLColorNode *colorNode = vtkMRMLColorNode::SafeDownCast
    (this->MRMLScene->GetNodeByID(plotChartNode->GetAttribute("LookupTable")));

  if (!colorNode)
    {
    colorNode = defaultColorNode;
    }

  if (!colorNode)
    {
    this->UpdatingWidgetFromMRML = false;
    return;
    }

  vtkSmartPointer<vtkCollection> allPlotDataNodesInScene = vtkSmartPointer<vtkCollection>::Take
    (this->mrmlScene()->GetNodesByClass("vtkMRMLPlotDataNode"));

  std::vector<std::string> plotDataNodesIDs;
  plotChartNode->GetPlotDataNodeIDs(plotDataNodesIDs);

  // Plot data nodes that should not be added to the chart
  // because they are already added or because they should not be added
  // (as not all necessary table data are available).
  std::set< vtkMRMLPlotDataNode* > plotDataNodesNotToAdd;

  // Remove plots from chart that are no longer needed or available
  for (int chartPlotDataNodesIndex = q->chart()->GetNumberOfPlots()-1; chartPlotDataNodesIndex >= 0; chartPlotDataNodesIndex--)
    {
    vtkPlot *plot = q->chart()->GetPlot(chartPlotDataNodesIndex);
    if (!plot)
      {
      continue;
      }
    // If it is NULL then it means that there is no usable associated plot data node
    // and so the plot should be removed.
    vtkMRMLPlotDataNode* plotDataNode = this->plotDataNodeFromPlot(plot);
    if (plotDataNode != NULL)
      {
      plotDataNodesNotToAdd.insert(plotDataNode);
      vtkMRMLTableNode *mrmlTableNode = plotDataNode->GetTableNode();
      if (mrmlTableNode != NULL)
        {
        if (!plotDataNode->IsXColumnIndex()
          && mrmlTableNode->GetColumnIndex(plotDataNode->GetXColumnName()) < 0)
          {
          // x column is not available in the source table anymore
          plotDataNode = NULL;
          }
        else if (mrmlTableNode->GetColumnIndex(plotDataNode->GetYColumnName()) < 0)
          {
          // y column is not available in the source table anymore
          plotDataNode = NULL;
          }
        }
      else
        {
        // Data table is not defined
        plotDataNode = NULL;
        }
      }

    bool deletePlot = true;
    if (plotDataNode)
      {
      vtkSmartPointer<vtkPlot> newPlot = this->updatePlotFromPlotDataNode(plotDataNode, plot);
      if (newPlot == plot)
        {
        // keep current plot
        deletePlot = false;
        }
      else
        {
        newPlot->SetProperty(PLOT_NODE_ID_PROPERTY_NAME, plotDataNode->GetID());
        q->addPlot(newPlot);
        }
      }

    if (deletePlot)
      {
      // This if is necessary for a BUG at VTK level:
      // in the case of a plot removed with corner ID 0,
      // when successively the addPlot method is called
      // (to add the same plot instance to vtkChartXY) it will
      // fail to setup the graph in the vtkChartXY render.
      if (q->chart()->GetPlotCorner(plot) == 0)
        {
        q->chart()->SetPlotCorner(plot, 1);
        }

      q->removePlot(plot);
      }
    }

  // Add missing plots to the chart
  for (std::vector<std::string>::iterator it = plotDataNodesIDs.begin(); it != plotDataNodesIDs.end(); ++it)
    {
    vtkMRMLPlotDataNode* plotDataNode = vtkMRMLPlotDataNode::SafeDownCast(this->mrmlScene()->GetNodeByID(it->c_str()));
    if (!plotDataNode || plotDataNodesNotToAdd.find(plotDataNode) != plotDataNodesNotToAdd.end())
      {
      // node is invalid or need not to be added
      continue;
      }
    vtkSmartPointer<vtkPlot> newPlot = this->updatePlotFromPlotDataNode(plotDataNode, NULL);
    if (!newPlot)
      {
      continue;
      }
    newPlot->SetProperty(PLOT_NODE_ID_PROPERTY_NAME, plotDataNode->GetID());
    q->addPlot(newPlot);
    }

  int fontTypeIndex = q->chart()->GetTitleProperties()->GetFontFamilyFromString(plotChartNode->GetFontType() ? plotChartNode->GetFontType() : "Arial");

  // Setting Title
  if (plotChartNode->GetTitleVisibility())
    {
    q->chart()->SetTitle(plotChartNode->GetTitle() ? plotChartNode->GetTitle() : "");
    }
  else
    {
    q->chart()->SetTitle("");
    }
  q->chart()->GetTitleProperties()->SetFontFamily(fontTypeIndex);
  q->chart()->GetTitleProperties()->SetFontSize(plotChartNode->GetTitleFontSize());

  // Setting Legend
  q->chart()->SetShowLegend(plotChartNode->GetLegendVisibility());
  q->chart()->GetLegend()->GetLabelProperties()->SetFontFamily(fontTypeIndex);

  // Setting ClickAndDrag action draggable along X and Y axes
  q->chart()->SetDragPointAlongX(plotChartNode->GetClickAndDragAlongX());
  q->chart()->SetDragPointAlongY(plotChartNode->GetClickAndDragAlongY());

  // Setting Axes
  const unsigned int numberOfAxisIDs = 4;
  int axisIDs[numberOfAxisIDs] = { vtkAxis::BOTTOM, vtkAxis::TOP, vtkAxis::LEFT, vtkAxis::RIGHT };
  for (int axisID = 0; axisID < numberOfAxisIDs; ++axisID)
    {
    vtkAxis *axis = q->chart()->GetAxis(axisID);
    if (!axis)
      {
      continue;
      }
    // Assuming the the Top and Bottom axes are the "X" axis
    if (axisID == vtkAxis::BOTTOM || axisID == vtkAxis::TOP)
      {
      if (plotChartNode->GetXAxisTitleVisibility())
        {
        axis->SetTitle(plotChartNode->GetXAxisTitle() ? plotChartNode->GetXAxisTitle() : "");
        }
      else
        {
        axis->SetTitle("");
        }
      }
    else if (axisID == vtkAxis::LEFT || axisID == vtkAxis::RIGHT)
      {
      if (plotChartNode->GetYAxisTitleVisibility())
        {
        axis->SetTitle(plotChartNode->GetYAxisTitle() ? plotChartNode->GetYAxisTitle() : "");
        }
      else
        {
        axis->SetTitle("");
        }
      }
    axis->SetGridVisible(plotChartNode->GetGridVisibility());
    axis->GetTitleProperties()->SetFontFamily(fontTypeIndex);
    axis->GetTitleProperties()->SetFontSize(plotChartNode->GetAxisTitleFontSize());
    axis->GetLabelProperties()->SetFontFamily(fontTypeIndex);
    axis->GetLabelProperties()->SetFontSize(plotChartNode->GetAxisLabelFontSize());
    }

  q->scene()->SetDirty(true);
  this->UpdatingWidgetFromMRML = false;
}

// --------------------------------------------------------------------------
// qMRMLPlotView methods

// --------------------------------------------------------------------------
qMRMLPlotView::qMRMLPlotView(QWidget* _parent) : Superclass(_parent)
  , d_ptr(new qMRMLPlotViewPrivate(*this))
{
  Q_D(qMRMLPlotView);
  d->init();
}

// --------------------------------------------------------------------------
qMRMLPlotView::~qMRMLPlotView()
{
  this->setMRMLScene(0);
}


//------------------------------------------------------------------------------
void qMRMLPlotView::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLPlotView);
  if (newScene == d->MRMLScene)
    {
    return;
    }

  d->setMRMLScene(newScene);

  if (d->MRMLPlotViewNode && newScene != d->MRMLPlotViewNode->GetScene())
    {
    this->setMRMLPlotViewNode(0);
    }

  emit mrmlSceneChanged(newScene);
}

//---------------------------------------------------------------------------
void qMRMLPlotView::setMRMLPlotViewNode(vtkMRMLPlotViewNode* newPlotViewNode)
{
  Q_D(qMRMLPlotView);
  if (d->MRMLPlotViewNode == newPlotViewNode)
    {
    return;
    }

  // connect modified event on PlotViewNode to updating the widget
  d->qvtkReconnect(d->MRMLPlotViewNode, newPlotViewNode,
    vtkMRMLPlotViewNode::PlotChartNodeChangedEvent, d, SLOT(updateWidgetFromMRML()));

  // connect on PlotDataNodeChangedEvent (e.g. PlotView is looking at a
  // different PlotDataNode
  d->qvtkReconnect(d->MRMLPlotViewNode, newPlotViewNode,
    vtkMRMLPlotViewNode::PlotChartNodeChangedEvent, d, SLOT(onPlotChartNodeChanged()));

  // cache the PlotViewNode
  d->MRMLPlotViewNode = newPlotViewNode;

  // ... and connect modified event on the PlotViewNode's PlotChartNode
  // to update the widget
  d->onPlotChartNodeChanged();

  // make sure the gui is up to date
  d->updateWidgetFromMRML();
}

//---------------------------------------------------------------------------
vtkMRMLPlotViewNode* qMRMLPlotView::mrmlPlotViewNode()const
{
  Q_D(const qMRMLPlotView);
  return d->MRMLPlotViewNode;
}

//---------------------------------------------------------------------------
void qMRMLPlotView::setColorLogic(vtkMRMLColorLogic *colorLogic)
{
  Q_D(qMRMLPlotView);
  d->ColorLogic = colorLogic;
}

//---------------------------------------------------------------------------
vtkMRMLColorLogic *qMRMLPlotView::colorLogic() const
{
  Q_D(const qMRMLPlotView);
  return d->ColorLogic;
}

//---------------------------------------------------------------------------
vtkMRMLScene* qMRMLPlotView::mrmlScene()const
{
  Q_D(const qMRMLPlotView);
  return d->MRMLScene;
}

//---------------------------------------------------------------------------
QSize qMRMLPlotView::sizeHint()const
{
  // return a default size hint (invalid size)
  return QSize();
}

// --------------------------------------------------------------------------
void qMRMLPlotView::keyPressEvent(QKeyEvent *event)
{
  Q_D(qMRMLPlotView);
  this->Superclass::keyPressEvent(event);

  if (event->key() == Qt::Key_S)
    {
    d->switchSelectionMode();
    }
  if (event->key() == Qt::Key_R)
    {
    d->RecalculateBounds();
    }
  if (event->key() == Qt::Key_Shift)
    {
    d->switchLeftAndMiddleClick();
    }
}

// --------------------------------------------------------------------------
void qMRMLPlotView::keyReleaseEvent(QKeyEvent *event)
{
  Q_D(qMRMLPlotView);
  this->Superclass::keyPressEvent(event);

  if (event->key() == Qt::Key_Shift)
    {
    d->switchLeftAndMiddleClick();
    }
}

// --------------------------------------------------------------------------
void qMRMLPlotView::fitToContent()
{
  Q_D(qMRMLPlotView);
  d->RecalculateBounds();
  // Repaint the chart scene
  this->scene()->SetDirty(true);
  if (this->scene()->GetRenderer())
    {
    this->scene()->GetRenderer()->Render();
    }
}
