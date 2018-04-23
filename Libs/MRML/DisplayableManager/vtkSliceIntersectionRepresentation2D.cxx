/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceIntersectionRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSliceIntersectionRepresentation2D.h"


#include <deque>

#include "vtkMRMLApplicationLogic.h"
#include "vtkMRMLDisplayableNode.h"
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLSliceLogic.h"
#include "vtkMRMLSliceNode.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPlane.h"
#include "vtkSphereSource.h"
#include "vtkActor2D.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkTextProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkLineSource.h"
#include "vtkCoordinate.h"
#include "vtkGlyph2D.h"
#include "vtkCursor2D.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkLeaderActor2D.h"
#include "vtkTransform.h"
#include "vtkTextMapper.h"
#include "vtkActor2D.h"
#include "vtkWindow.h"


vtkStandardNewMacro(vtkSliceIntersectionRepresentation2D);

vtkCxxSetObjectMacro(vtkSliceIntersectionRepresentation2D, MRMLApplicationLogic, vtkMRMLApplicationLogic);
vtkCxxSetObjectMacro(vtkSliceIntersectionRepresentation2D, Property, vtkProperty2D);
vtkCxxSetObjectMacro(vtkSliceIntersectionRepresentation2D, SelectedProperty, vtkProperty2D);

class SliceIntersectionDisplayPipeline
{
public:

  //----------------------------------------------------------------------
  SliceIntersectionDisplayPipeline()
  {
    this->LineSource = vtkSmartPointer<vtkLineSource>::New();
    this->Mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
    this->Property = vtkSmartPointer<vtkProperty2D>::New();
    this->Actor = vtkSmartPointer<vtkActor2D>::New();
    this->Actor->SetVisibility(false); // invisible until slice node is set

    this->Mapper->SetInputConnection(this->LineSource->GetOutputPort());
    this->Actor->SetMapper(this->Mapper);
    this->Actor->SetProperty(this->Property);
  }

  //----------------------------------------------------------------------
  virtual ~SliceIntersectionDisplayPipeline()
  {
    this->SetAndObserveSliceLogic(NULL, NULL);
  }

  //----------------------------------------------------------------------
  void SetAndObserveSliceLogic(vtkMRMLSliceLogic* sliceLogic, vtkCallbackCommand* callback)
  {

    if (sliceLogic != this->SliceLogic || callback != this->Callback)
    {
      if (this->SliceLogic && this->Callback)
      {
        this->SliceLogic->RemoveObserver(this->Callback);
      }
      if (sliceLogic)
      {
        sliceLogic->AddObserver(vtkCommand::ModifiedEvent, callback);
        sliceLogic->AddObserver(vtkMRMLSliceLogic::CompositeModifiedEvent, callback);
      }
      this->SliceLogic = sliceLogic;
    }
    this->Callback = callback;
  }

  //----------------------------------------------------------------------
  void GetActors2D(vtkPropCollection *pc)
  {
    pc->AddItem(this->Actor);
  }

  //----------------------------------------------------------------------
  void AddActors(vtkRenderer* renderer)
  {
    if (!renderer)
    {
      return;
    }
    renderer->AddViewProp(this->Actor);
  }

  //----------------------------------------------------------------------
  void ReleaseGraphicsResources(vtkWindow *win)
  {
    this->Actor->ReleaseGraphicsResources(win);
  }

  //----------------------------------------------------------------------
  int RenderOverlay(vtkViewport *viewport)
  {
    int count = 0;
    if (this->Actor->GetVisibility())
    {
      count += this->Actor->RenderOverlay(viewport);
    }
    return count;
  }


  //----------------------------------------------------------------------
  void RemoveActors(vtkRenderer* renderer)
  {
    if (!renderer)
    {
      return;
    }
    renderer->RemoveViewProp(this->Actor);
  }

  //----------------------------------------------------------------------
  void SetVisibility(bool visibility)
  {
    this->Actor->SetVisibility(visibility);
  }

  //----------------------------------------------------------------------
  bool GetVisibility()
  {
    return this->Actor->GetVisibility();
  }

  vtkSmartPointer<vtkLineSource> LineSource;
  vtkSmartPointer<vtkPolyDataMapper2D> Mapper;
  vtkSmartPointer<vtkProperty2D> Property;
  vtkSmartPointer<vtkActor2D> Actor;
  vtkWeakPointer<vtkMRMLSliceLogic> SliceLogic;
  vtkWeakPointer<vtkCallbackCommand> Callback;
};

class vtkSliceIntersectionRepresentation2D::vtkInternal
{
public:
  vtkInternal(vtkSliceIntersectionRepresentation2D * external);
  ~vtkInternal();

  static int IntersectWithFinitePlane(double n[3], double o[3], double pOrigin[3], double px[3], double py[3], double x0[3], double x1[3]);

  vtkSliceIntersectionRepresentation2D* External;

  vtkSmartPointer<vtkMRMLSliceNode> SliceNode;

  std::deque<SliceIntersectionDisplayPipeline*> SliceIntersectionDisplayPipelines;
  vtkNew<vtkCallbackCommand> SliceNodeModifiedCommand;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkSliceIntersectionRepresentation2D::vtkInternal
::vtkInternal(vtkSliceIntersectionRepresentation2D * external)
{
  this->External = external;
}

//---------------------------------------------------------------------------
vtkSliceIntersectionRepresentation2D::vtkInternal::~vtkInternal()
{
}

//---------------------------------------------------------------------------
int vtkSliceIntersectionRepresentation2D::vtkInternal::IntersectWithFinitePlane(double n[3], double o[3],
  double pOrigin[3], double px[3], double py[3],
  double x0[3], double x1[3])
{
  // Since we are dealing with convex shapes, if there is an intersection a
  // single line is produced as output. So all this is necessary is to
  // intersect the four bounding lines of the finite line and find the two
  // intersection points.
  int numInts = 0;
  double t, *x = x0;
  double xr0[3], xr1[3];

  // First line
  xr0[0] = pOrigin[0];
  xr0[1] = pOrigin[1];
  xr0[2] = pOrigin[2];
  xr1[0] = px[0];
  xr1[1] = px[1];
  xr1[2] = px[2];
  if (vtkPlane::IntersectWithLine(xr0, xr1, n, o, t, x))
  {
    numInts++;
    x = x1;
  }

  // Second line
  xr1[0] = py[0];
  xr1[1] = py[1];
  xr1[2] = py[2];
  if (vtkPlane::IntersectWithLine(xr0, xr1, n, o, t, x))
  {
    numInts++;
    x = x1;
  }
  if (numInts == 2)
  {
    return 1;
  }

  // Third line
  xr0[0] = -pOrigin[0] + px[0] + py[0];
  xr0[1] = -pOrigin[1] + px[1] + py[1];
  xr0[2] = -pOrigin[2] + px[2] + py[2];
  if (vtkPlane::IntersectWithLine(xr0, xr1, n, o, t, x))
  {
    numInts++;
    x = x1;
  }
  if (numInts == 2)
  {
    return 1;
  }

  // Fourth and last line
  xr1[0] = px[0];
  xr1[1] = px[1];
  xr1[2] = px[2];
  if (vtkPlane::IntersectWithLine(xr0, xr1, n, o, t, x))
  {
    numInts++;
  }
  if (numInts == 2)
  {
    return 1;
  }

  // No intersection has occurred, or a single degenerate point
  return 0;
}


//----------------------------------------------------------------------
vtkSliceIntersectionRepresentation2D::vtkSliceIntersectionRepresentation2D()
{
  this->MRMLApplicationLogic = NULL;

  this->Internal = new vtkInternal(this);
  this->Internal->SliceNodeModifiedCommand->SetClientData(this);
  this->Internal->SliceNodeModifiedCommand->SetCallback(vtkSliceIntersectionRepresentation2D::SliceNodeModifiedCallback);

  this->SliceIntersectionPoint[0] = 0.0;
  this->SliceIntersectionPoint[1] = 0.0;
  this->SliceIntersectionPoint[2] = 0.0;
  this->SliceIntersectionPoint[3] = 1.0; // to allow easy homogeneous tranformations

  this->StartRotationCenter_RAS[0] = 0.0;
  this->StartRotationCenter_RAS[1] = 0.0;
  this->StartRotationCenter_RAS[2] = 0.0;
  this->StartRotationCenter_RAS[3] = 1.0; // to allow easy homogeneous tranformations

  this->Tolerance = 5.0;

  // Initialize state
  this->InteractionState = vtkSliceIntersectionRepresentation2D::StateOutside;
}

//----------------------------------------------------------------------
vtkSliceIntersectionRepresentation2D::~vtkSliceIntersectionRepresentation2D()
{
  this->SetSliceNode(NULL);
  this->SetMRMLApplicationLogic(NULL);
  delete this->Internal;
}

//-------------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::PlaceWidget(double bounds[6])
{
  // no action is needed, widget position is determined from slice positions
}

//-------------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::SetOrigin(double ox, double oy, double oz)
{
  this->Internal->SliceNode->JumpAllSlices(ox, oy, oz);
}

//-------------------------------------------------------------------------
int vtkSliceIntersectionRepresentation2D::ComputeInteractionState(int X, int Y, int modify)
{
  this->InteractionState = vtkSliceIntersectionRepresentation2D::StateOutside;
  if (!this->Internal->SliceNode || !modify)
  {
    return this->InteractionState;
  }
  double interactionPosition[4] = { static_cast<double>(X), static_cast<double>(Y), 0.0, 1.0 };

  double pixelSize = this->Internal->SliceNode->GetXYToSlice()->GetElement(0, 0);


  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    if (!(*sliceIntersectionIt)->SliceLogic)
    {
      continue;
    }
    if (!(*sliceIntersectionIt)->SliceLogic->GetSliceNode())
    {
      continue;
    }
    double parametricCoordinate = 0;
    double distanceSquared = vtkLine::DistanceToLine(interactionPosition,
      (*sliceIntersectionIt)->LineSource->GetPoint1(), (*sliceIntersectionIt)->LineSource->GetPoint2(), parametricCoordinate);

    if (distanceSquared <= this->Tolerance*this->Tolerance)
    {
      this->InteractionState = vtkSliceIntersectionRepresentation2D::StateRotate;
      break;
    }
  }
  if (this->InteractionState == vtkSliceIntersectionRepresentation2D::StateOutside)
  {
    // not near any slice intersections

    // TODO:
    this->InteractionState = vtkSliceIntersectionRepresentation2D::StateJump;

    return this->InteractionState;
  }

  double* sliceIntersectionPoint = this->GetSliceIntersectionPoint();

  double centerPickTolerance = this->Tolerance * 3.0;
  if (vtkMath::Distance2BetweenPoints(interactionPosition, sliceIntersectionPoint) <= centerPickTolerance * centerPickTolerance)
  {
    this->InteractionState = vtkSliceIntersectionRepresentation2D::StateTranslate;
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkSliceIntersectionRepresentation2D::StartWidgetInteraction(double startEventPos[2])
{
  // Initialize bookeeping variables
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  double* sliceIntersectionPoint = this->GetSliceIntersectionPoint();
  this->PreviousRotationAngleRad = atan2(startEventPos[1] - sliceIntersectionPoint[1],
    startEventPos[0] - sliceIntersectionPoint[0]);
  this->PreviousEventPosition[0] = startEventPos[0];
  this->PreviousEventPosition[1] = startEventPos[1];
  this->StartRotationCenter[0] = sliceIntersectionPoint[0];
  this->StartRotationCenter[1] = sliceIntersectionPoint[1];

  double startRotationCenterXY[4] = { sliceIntersectionPoint[0] , sliceIntersectionPoint[1], 0.0, 1.0 };
  this->Internal->SliceNode->GetXYToRAS()->MultiplyPoint(startRotationCenterXY, this->StartRotationCenter_RAS);

  this->WidgetInteraction(startEventPos);
}


//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkSliceIntersectionRepresentation2D::WidgetInteraction(double eventPos[2])
{
  // Dispatch to the correct method
  switch (this->InteractionState)
  {
    case vtkSliceIntersectionRepresentation2D::StateRotate:
      this->Rotate(eventPos);
      break;
    case vtkSliceIntersectionRepresentation2D::StateTranslate:
      this->Translate(eventPos);
      break;
    case vtkSliceIntersectionRepresentation2D::StateJump:
      this->Jump(eventPos);
      break;
  }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];

  this->Modified();
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::EndWidgetInteraction(double vtkNotUsed(eventPos) [2])
{
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::Jump(double eventPos[2])
{
  double eventPos_RAS[4] = { eventPos[0], eventPos[1], 0, 1 };
  vtkMatrix4x4* xyToRas = this->Internal->SliceNode->GetXYToRAS();
  xyToRas->MultiplyPoint(eventPos_RAS, eventPos_RAS);
  this->Internal->SliceNode->JumpAllSlices(eventPos_RAS[0], eventPos_RAS[1], eventPos_RAS[2]);
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::Translate(double eventPos[2])
{
  double eventPos_RAS[4] = { eventPos[0], eventPos[1], 0, 1 };
  vtkMatrix4x4* xyToRas = this->Internal->SliceNode->GetXYToRAS();
  xyToRas->MultiplyPoint(eventPos_RAS, eventPos_RAS);
  this->Internal->SliceNode->JumpAllSlices(eventPos_RAS[0], eventPos_RAS[1], eventPos_RAS[2]);
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::Rotate(double eventPos[2])
{
  double sliceRotationAngleRad = atan2(eventPos[1] - this->StartRotationCenter[1],
    eventPos[0] - this->StartRotationCenter[0]);

  vtkMatrix4x4* sliceToRAS = this->Internal->SliceNode->GetSliceToRAS();
  vtkNew<vtkTransform> rotatedSliceToSliceTransform;

  rotatedSliceToSliceTransform->Translate(this->StartRotationCenter_RAS[0], this->StartRotationCenter_RAS[1], this->StartRotationCenter_RAS[2]);
  double rotationDirection = vtkMath::Determinant3x3(sliceToRAS->Element[0], sliceToRAS->Element[1], sliceToRAS->Element[2]) >= 0 ? 1.0 : -1.0;
  rotatedSliceToSliceTransform->RotateWXYZ(rotationDirection*vtkMath::DegreesFromRadians(sliceRotationAngleRad- this->PreviousRotationAngleRad),
    sliceToRAS->GetElement(0, 2), sliceToRAS->GetElement(1, 2), sliceToRAS->GetElement(2, 2));
  rotatedSliceToSliceTransform->Translate(-this->StartRotationCenter_RAS[0], -this->StartRotationCenter_RAS[1], -this->StartRotationCenter_RAS[2]);

  this->PreviousRotationAngleRad = sliceRotationAngleRad;
  this->PreviousEventPosition[0] = eventPos[0];
  this->PreviousEventPosition[1] = eventPos[1];

  std::deque<int> wasModified;
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    if (!(*sliceIntersectionIt)
      || !(*sliceIntersectionIt)->GetVisibility())
    {
      continue;
    }
    vtkMRMLSliceNode* sliceNode = (*sliceIntersectionIt)->SliceLogic->GetSliceNode();
    wasModified.push_back(sliceNode->StartModify());
    vtkMatrix4x4::Multiply4x4(rotatedSliceToSliceTransform->GetMatrix(), sliceNode->GetSliceToRAS(),
      sliceNode->GetSliceToRAS());
    sliceNode->UpdateMatrices();
  }
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    if (!(*sliceIntersectionIt)
      || !(*sliceIntersectionIt)->GetVisibility())
    {
      continue;
    }
    vtkMRMLSliceNode* sliceNode = (*sliceIntersectionIt)->SliceLogic->GetSliceNode();
    sliceNode->EndModify(wasModified.front());
    wasModified.pop_front();
  }
}


//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::BuildRepresentation()
{
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::ShallowCopy(vtkProp *prop)
{
  vtkSliceIntersectionRepresentation2D *rep =
    vtkSliceIntersectionRepresentation2D::SafeDownCast(prop);
  if (rep)
  {
    this->SetTolerance(rep->GetTolerance());
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->Internal->SliceIntersectionDisplayPipelines = rep->Internal->SliceIntersectionDisplayPipelines;
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::GetActors2D(vtkPropCollection *pc)
{
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    (*sliceIntersectionIt)->GetActors2D(pc);
  }
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::ReleaseGraphicsResources(vtkWindow *win)
{
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    (*sliceIntersectionIt)->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------
int vtkSliceIntersectionRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  int count = 0;

  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    count += (*sliceIntersectionIt)->RenderOverlay(viewport);
  }
  return count;
}


//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";

  if ( this->Property )
  {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  if ( this->SelectedProperty )
  {
    os << indent << "Selected Property:\n";
    this->SelectedProperty->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Selected Property: (none)\n";
  }

}


//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::SliceNodeModifiedCallback(
  vtkObject* caller, unsigned long eid, void* clientData, void* callData)
{
  vtkSliceIntersectionRepresentation2D* self = vtkSliceIntersectionRepresentation2D::SafeDownCast((vtkObject*)clientData);

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(caller);
  if (sliceNode)
  {
    self->SliceNodeModified(sliceNode);
    return;
  }

  vtkMRMLSliceLogic* sliceLogic = vtkMRMLSliceLogic::SafeDownCast(caller);
  if (sliceLogic)
  {
    self->UpdateSliceIntersectionDisplay(self->GetDisplayPipelineFromSliceLogic(sliceLogic));
    return;
  }
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::SliceNodeModified(vtkMRMLSliceNode* sliceNode)
{
  if (!sliceNode)
  {
    return;
  }
  if (sliceNode == this->Internal->SliceNode)
  {
    // update all slice intersection
    for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
      sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
    {
      this->UpdateSliceIntersectionDisplay(*sliceIntersectionIt);
    }
  }
}

//----------------------------------------------------------------------
SliceIntersectionDisplayPipeline* vtkSliceIntersectionRepresentation2D::GetDisplayPipelineFromSliceLogic(vtkMRMLSliceLogic* sliceLogic)
{
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    if (!(*sliceIntersectionIt)->SliceLogic)
    {
      continue;
    }
    if (sliceLogic == (*sliceIntersectionIt)->SliceLogic)
    {
      // found it
      return *sliceIntersectionIt;
    }
  }
  return NULL;
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::UpdateSliceIntersectionDisplay(SliceIntersectionDisplayPipeline *pipeline)
{
  if (!pipeline || !this->Internal->SliceNode || pipeline->SliceLogic == NULL)
    {
    return;
    }
  vtkMRMLSliceNode* intersectingSliceNode = pipeline->SliceLogic->GetSliceNode();
  if (!pipeline->SliceLogic || !this->GetVisibility()
    || this->Internal->SliceNode->GetViewGroup() != intersectingSliceNode->GetViewGroup())
  {
    pipeline->SetVisibility(false);
    return;
  }

  vtkMRMLModelDisplayNode* displayNode = NULL;
  vtkMRMLSliceLogic *sliceLogic = NULL;
  vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
  if (mrmlAppLogic)
  {
    sliceLogic = mrmlAppLogic->GetSliceLogic(intersectingSliceNode);
  }
  if (sliceLogic)
  {
    displayNode = sliceLogic->GetSliceModelDisplayNode();
  }
  if (displayNode)
  {
    if (!displayNode->GetSliceIntersectionVisibility())
    {
      pipeline->SetVisibility(false);
      return;
    }
    pipeline->Property->SetLineWidth(displayNode->GetLineWidth());
  }

  pipeline->Property->SetColor(intersectingSliceNode->GetLayoutColor());

  vtkMatrix4x4* intersectingXYToRAS = intersectingSliceNode->GetXYToRAS();
  vtkMatrix4x4* xyToRAS = this->Internal->SliceNode->GetXYToRAS();

  //double slicePlaneAngleDifference = vtkMath::AngleBetweenVectors()

  vtkNew<vtkMatrix4x4> rasToXY;
  vtkMatrix4x4::Invert(xyToRAS, rasToXY);
  vtkNew<vtkMatrix4x4> intersectingXYToXY;
  vtkMatrix4x4::Multiply4x4(rasToXY, intersectingXYToRAS, intersectingXYToXY);

  double slicePlaneNormal[3] = { 0.,0.,1. };
  double slicePlaneOrigin[3] = { 0., 0., 0. };

  int* intersectingSliceSizeDimensions = intersectingSliceNode->GetDimensions();
  double intersectingPlaneOrigin[4] = { 0, 0, 0, 1 };
  double intersectingPlaneX[4] = { double(intersectingSliceSizeDimensions[0]), 0., 0., 1. };
  double intersectingPlaneY[4] = { 0., double(intersectingSliceSizeDimensions[1]), 0., 1. };
  intersectingXYToXY->MultiplyPoint(intersectingPlaneOrigin, intersectingPlaneOrigin);
  intersectingXYToXY->MultiplyPoint(intersectingPlaneX, intersectingPlaneX);
  intersectingXYToXY->MultiplyPoint(intersectingPlaneY, intersectingPlaneY);

  double intersectionPoint1[4] = { 0.0, 0.0, 0.0, 1.0 };
  double intersectionPoint2[4] = { 0.0, 0.0, 0.0, 1.0 };

  int intersectionFound = vtkSliceIntersectionRepresentation2D::vtkInternal::IntersectWithFinitePlane(slicePlaneNormal, slicePlaneOrigin,
    intersectingPlaneOrigin, intersectingPlaneX, intersectingPlaneY,
    intersectionPoint1, intersectionPoint2);
  if (!intersectionFound)
  {
    pipeline->SetVisibility(false);
    return;
  }

  pipeline->LineSource->SetPoint1(intersectionPoint1);
  pipeline->LineSource->SetPoint2(intersectionPoint2);

  pipeline->SetVisibility(true);
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::SetSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if (sliceNode == this->Internal->SliceNode)
  {
    // no change
    return;
  }
  if (this->Internal->SliceNode)
  {
    this->Internal->SliceNode->RemoveObserver(this->Internal->SliceNodeModifiedCommand);
  }
  if (sliceNode)
  {
    sliceNode->AddObserver(vtkCommand::ModifiedEvent, this->Internal->SliceNodeModifiedCommand.GetPointer());
  }
  this->Internal->SliceNode = sliceNode;
  this->UpdateIntersectingSliceNodes();
}

//----------------------------------------------------------------------
vtkMRMLSliceNode* vtkSliceIntersectionRepresentation2D::GetSliceNode()
{
  return this->Internal->SliceNode;
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::AddIntersectingSliceLogic(vtkMRMLSliceLogic* sliceLogic)
{
  if (!sliceLogic)
  {
    return;
  }
  if (sliceLogic->GetSliceNode() == this->Internal->SliceNode)
  {
    // it is the slice itself, not an intersecting slice
    return;
  }
  if (this->GetDisplayPipelineFromSliceLogic(sliceLogic))
  {
    // slice node already added
    return;
  }

  SliceIntersectionDisplayPipeline* pipeline = new SliceIntersectionDisplayPipeline;
  pipeline->SetAndObserveSliceLogic(sliceLogic, this->Internal->SliceNodeModifiedCommand);
  pipeline->AddActors(this->Renderer);
  this->Internal->SliceIntersectionDisplayPipelines.push_back(pipeline);
  this->UpdateSliceIntersectionDisplay(pipeline);
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::RemoveIntersectingSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if (!sliceNode)
  {
    return;
  }
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    if (!(*sliceIntersectionIt)->SliceLogic)
    {
      continue;
    }
    if (sliceNode == (*sliceIntersectionIt)->SliceLogic->GetSliceNode())
    {
      // found it
      (*sliceIntersectionIt)->RemoveActors(this->Renderer);
      delete (*sliceIntersectionIt);
      this->Internal->SliceIntersectionDisplayPipelines.erase(sliceIntersectionIt);
      break;
    }
  }
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::UpdateIntersectingSliceNodes()
{
  this->RemoveAllIntersectingSliceNodes();
  if (this->GetSliceNode() && this->MRMLApplicationLogic)
  {
    vtkMRMLSliceLogic* sliceLogic;
    vtkCollectionSimpleIterator it;
    vtkCollection* sliceLogics = this->MRMLApplicationLogic->GetSliceLogics();
    for (sliceLogics->InitTraversal(it);
      (sliceLogic = vtkMRMLSliceLogic::SafeDownCast(sliceLogics->GetNextItemAsObject(it)));)
    {
      this->AddIntersectingSliceLogic(sliceLogic);
    }
  }
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::RemoveAllIntersectingSliceNodes()
{
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    (*sliceIntersectionIt)->RemoveActors(this->Renderer);
    delete (*sliceIntersectionIt);
  }
  this->Internal->SliceIntersectionDisplayPipelines.clear();
}

//----------------------------------------------------------------------
double* vtkSliceIntersectionRepresentation2D::GetSliceIntersectionPoint()
{
  size_t numberOfIntersections = this->Internal->SliceIntersectionDisplayPipelines.size();
  int numberOfFoundIntersectionPoints = 0;
  this->SliceIntersectionPoint[0] = 0.0;
  this->SliceIntersectionPoint[1] = 0.0;
  this->SliceIntersectionPoint[2] = 0.0;
  if (!this->Internal->SliceNode)
  {
    return this->SliceIntersectionPoint;
  }
  for (int slice1Index = 0; slice1Index < numberOfIntersections-1; slice1Index++)
  {
    if (!this->Internal->SliceIntersectionDisplayPipelines[slice1Index]->GetVisibility())
    {
      continue;
    }
    vtkLineSource* line1 = this->Internal->SliceIntersectionDisplayPipelines[slice1Index]->LineSource;
    double* line1Point1 = line1->GetPoint1();
    double* line1Point2 = line1->GetPoint2();
    for (int slice2Index = slice1Index + 1; slice2Index < numberOfIntersections; slice2Index++)
    {
      if (!this->Internal->SliceIntersectionDisplayPipelines[slice2Index]->GetVisibility())
      {
        continue;
      }
      vtkLineSource* line2 = this->Internal->SliceIntersectionDisplayPipelines[slice2Index]->LineSource;
      double line1ParametricPosition = 0;
      double line2ParametricPosition = 0;
      if (vtkLine::Intersection(line1Point1, line1Point2,
        line2->GetPoint1(), line2->GetPoint2(),
        line1ParametricPosition, line2ParametricPosition))
      {
        this->SliceIntersectionPoint[0] += line1Point1[0] + line1ParametricPosition * (line1Point2[0] - line1Point1[0]);
        this->SliceIntersectionPoint[1] += line1Point1[1] + line1ParametricPosition * (line1Point2[1] - line1Point1[1]);
        this->SliceIntersectionPoint[2] += line1Point1[2] + line1ParametricPosition * (line1Point2[2] - line1Point1[2]);
        numberOfFoundIntersectionPoints++;
      }
    }
  }
  if (numberOfFoundIntersectionPoints > 0)
  {
    this->SliceIntersectionPoint[0] /= numberOfFoundIntersectionPoints;
    this->SliceIntersectionPoint[1] /= numberOfFoundIntersectionPoints;
    this->SliceIntersectionPoint[2] /= numberOfFoundIntersectionPoints;
  }
  else
  {
    // No slice intersections, use slice centerpoint
    int* sliceDimension = this->Internal->SliceNode->GetDimensions();
    this->SliceIntersectionPoint[0] = sliceDimension[0] / 2.0;
    this->SliceIntersectionPoint[0] = sliceDimension[1] / 2.0;
    this->SliceIntersectionPoint[0] = 0.0;
  }
  return this->SliceIntersectionPoint;
}
