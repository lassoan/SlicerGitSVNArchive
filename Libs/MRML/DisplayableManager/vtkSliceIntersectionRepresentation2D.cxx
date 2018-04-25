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

vtkCxxSetObjectMacro(vtkSliceIntersectionRepresentation2D,Property,vtkProperty2D);
vtkCxxSetObjectMacro(vtkSliceIntersectionRepresentation2D,SelectedProperty,vtkProperty2D);

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
    this->SetAndObserveSliceNode(NULL, NULL);
  }

  //----------------------------------------------------------------------
  void SetAndObserveSliceNode(vtkMRMLSliceNode* sliceNode, vtkCallbackCommand* callback)
  {
    if (sliceNode == this->SliceNode)
    {
      // no change
      return;
    }
    if (this->SliceNode && this->Callback)
    {
      this->SliceNode->RemoveObserver(this->Callback);
    }
    if (sliceNode)
    {
      sliceNode->AddObserver(vtkCommand::ModifiedEvent, callback);
    }
    this->SliceNode = sliceNode;
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
  vtkWeakPointer<vtkMRMLSliceNode> SliceNode;
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

  vtkNew<vtkLineSource> CenterOfRotationSource;
  vtkNew<vtkPolyDataMapper2D> CenterOfRotationMapper;
  vtkNew<vtkActor2D> CenterOfRotationActor;

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

  // Keep track of transformations
  //this->DisplayOrigin[0] = this->DisplayOrigin[1] = this->DisplayOrigin[2] = 0.0;
  //this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;

  this->Internal->CenterOfRotationActor->SetVisibility(false); // invisible until slice node is set
  this->Internal->CenterOfRotationMapper->SetInputConnection(this->Internal->CenterOfRotationSource->GetOutputPort());
  this->Internal->CenterOfRotationActor->SetMapper(this->Internal->CenterOfRotationMapper);
  //this->Actor->SetProperty(this->Property);


  // Create properties
  this->CreateDefaultProperties();
}

//----------------------------------------------------------------------
vtkSliceIntersectionRepresentation2D::~vtkSliceIntersectionRepresentation2D()
{
  this->SetSliceNode(NULL);
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
    if (!(*sliceIntersectionIt)->SliceNode)
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

  /*
    vtkNew<vtkMatrix4x4> rasToXY;
  vtkMatrix4x4::Invert(this->Internal->SliceNode->GetXYToRAS(), rasToXY);
  double sliceIntersectionPoint[4] = { 0.0,0.0,0.0,1.0 };
  rasToXY->MultiplyPoint(sliceIntersectionPoint_RAS, sliceIntersectionPoint);
  */

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


  /*
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,
                                               startEventPos[0], startEventPos[1], 0.0,
                                               this->StartWorldPosition);

  this->StartAngle = VTK_FLOAT_MAX;
  */

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
  /*
  // Have to play games here because of the "pipelined" nature of the
  // transformations.
  this->GetTransform(this->TempTransform);
  this->TotalTransform->SetMatrix(this->TempTransform->GetMatrix());

  // Adjust the origin as necessary
  this->Origin[0] += this->CurrentTranslation[0];
  this->Origin[1] += this->CurrentTranslation[1];
  this->Origin[2] += this->CurrentTranslation[2];

  // Reset the current transformations
  this->CurrentTranslation[0] = 0.0;
  this->CurrentTranslation[1] = 0.0;
  this->CurrentTranslation[2] = 0.0;

  this->CurrentAngle = 0.0;

  this->CurrentScale[0] = 1.0;
  this->CurrentScale[1] = 1.0;

  this->CurrentShear[0] = 0.0;
  this->CurrentShear[1] = 0.0;
  */
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
    if (!(*sliceIntersectionIt)->GetVisibility())
    {
      continue;
    }
    wasModified.push_back((*sliceIntersectionIt)->SliceNode->StartModify());
    vtkMatrix4x4::Multiply4x4(rotatedSliceToSliceTransform->GetMatrix(), (*sliceIntersectionIt)->SliceNode->GetSliceToRAS(),
      (*sliceIntersectionIt)->SliceNode->GetSliceToRAS());
    (*sliceIntersectionIt)->SliceNode->UpdateMatrices();
  }
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    if (!(*sliceIntersectionIt)->GetVisibility())
    {
      continue;
    }
    (*sliceIntersectionIt)->SliceNode->EndModify(wasModified.front());
    wasModified.pop_front();
  }
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::CreateDefaultProperties()
{
/*
  this->Property = vtkProperty2D::New();
  this->Property->SetColor(0.0,1.0,0.0);
  this->Property->SetLineWidth(0.5);

  this->SelectedProperty = vtkProperty2D::New();
  this->SelectedProperty->SetColor(1.0,0.0,0.0);
  this->SelectedProperty->SetLineWidth(1.0);
  */
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::BuildRepresentation()
{
  /*
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
  {
    // Determine where the origin is on the display
    vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, this->Origin[0], this->Origin[1],
                                                 this->Origin[2], this->DisplayOrigin);

    // draw the box
    this->CurrentWidth = this->BoxWidth;
    this->CurrentWidth /= 2.0;
    double p1[3], p2[3],p3[3], p4[3];
    p1[0] = this->DisplayOrigin[0] - this->CurrentWidth;
    p1[1] = this->DisplayOrigin[1] - this->CurrentWidth;
    p1[2] = 0.0;
    p2[0] = this->DisplayOrigin[0] + this->CurrentWidth;
    p2[1] = this->DisplayOrigin[1] - this->CurrentWidth;
    p2[2] = 0.0;
    p3[0] = this->DisplayOrigin[0] + this->CurrentWidth;
    p3[1] = this->DisplayOrigin[1] + this->CurrentWidth;
    p3[2] = 0.0;
    p4[0] = this->DisplayOrigin[0] - this->CurrentWidth;
    p4[1] = this->DisplayOrigin[1] + this->CurrentWidth;
    p4[2] = 0.0;
    this->BoxPoints->SetPoint(0,p1);
    this->BoxPoints->SetPoint(1,p2);
    this->BoxPoints->SetPoint(2,p3);
    this->BoxPoints->SetPoint(3,p4);
    this->BoxPoints->Modified();

    // draw the circle
    int i;
    double theta, delTheta = 2.0 * vtkMath::Pi() / VTK_CIRCLE_RESOLUTION;
    this->CurrentRadius = this->CurrentWidth * 0.75;
    this->CircleCellArray->InsertNextCell(VTK_CIRCLE_RESOLUTION+1);
    for (i=0; i<VTK_CIRCLE_RESOLUTION; i++)
    {
      theta = i * delTheta;
      p1[0] = this->DisplayOrigin[0] + this->CurrentRadius * cos(theta);
      p1[1] = this->DisplayOrigin[1] + this->CurrentRadius * sin(theta);
      this->CirclePoints->SetPoint(i,p1);
      this->CircleCellArray->InsertCellPoint(i);
    }
    this->CircleCellArray->InsertCellPoint(0);

    // draw the translation axes
    this->CurrentAxesWidth = this->CurrentWidth * this->AxesWidth/this->BoxWidth;
    p1[0] = this->DisplayOrigin[0] - this->CurrentAxesWidth;
    p1[1] = this->DisplayOrigin[1];
    this->XAxis->GetPositionCoordinate()->SetValue(p1);
    p2[0] = this->DisplayOrigin[0] + this->CurrentAxesWidth;
    p2[1] = this->DisplayOrigin[1];
    this->XAxis->GetPosition2Coordinate()->SetValue(p2);

    p1[0] = this->DisplayOrigin[0];
    p1[1] = this->DisplayOrigin[1] - this->CurrentAxesWidth;
    this->YAxis->GetPositionCoordinate()->SetValue(p1);
    p2[0] = this->DisplayOrigin[0];
    p2[1] = this->DisplayOrigin[1] + this->CurrentAxesWidth;
    this->YAxis->GetPosition2Coordinate()->SetValue(p2);

    this->BuildTime.Modified();
  }
  */
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
  pc->AddItem(this->Internal->CenterOfRotationActor);
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::ReleaseGraphicsResources(vtkWindow *win)
{
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    (*sliceIntersectionIt)->ReleaseGraphicsResources(win);
  }

  this->Internal->CenterOfRotationActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkSliceIntersectionRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  //this->BuildRepresentation();

  int count = 0;

  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    count += (*sliceIntersectionIt)->RenderOverlay(viewport);
  }

  count += this->Internal->CenterOfRotationActor->RenderOverlay(viewport);

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
  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(caller);
  vtkSliceIntersectionRepresentation2D* self = vtkSliceIntersectionRepresentation2D::SafeDownCast((vtkObject*)clientData);
  self->SliceNodeModified(sliceNode);
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
  else
  {
    // update one slice intersection
    this->UpdateSliceIntersectionDisplay(this->GetDisplayPipelineFromSliceNode(sliceNode));
  }

  if (this->Internal->SliceNode)
  {
    // Update centerline position
    double* sliceIntersectionPoint = this->GetSliceIntersectionPoint();
    //this->Internal->CenterOfRotationSource->SetCenter(sliceIntersectionPoint[0], sliceIntersectionPoint[1], 0);
    this->Internal->CenterOfRotationSource->SetPoint1(sliceIntersectionPoint[0], sliceIntersectionPoint[1], 0);
    this->Internal->CenterOfRotationSource->SetPoint2(sliceIntersectionPoint[0] - 50, sliceIntersectionPoint[1] - 50, 0);
    this->Internal->CenterOfRotationActor->SetVisibility(true);
  }
  else
  {
    this->Internal->CenterOfRotationActor->SetVisibility(false);
  }
}

//----------------------------------------------------------------------
SliceIntersectionDisplayPipeline* vtkSliceIntersectionRepresentation2D::GetDisplayPipelineFromSliceNode(vtkMRMLSliceNode* sliceNode)
{
  for (std::deque<SliceIntersectionDisplayPipeline*>::iterator sliceIntersectionIt = this->Internal->SliceIntersectionDisplayPipelines.begin();
    sliceIntersectionIt != this->Internal->SliceIntersectionDisplayPipelines.end(); ++sliceIntersectionIt)
  {
    if (sliceNode == (*sliceIntersectionIt)->SliceNode)
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
  if (!pipeline || !this->Internal->SliceNode)
    {
    return;
    }
  if (!pipeline->SliceNode || !this->GetVisibility()
    || this->Internal->SliceNode->GetViewGroup() != pipeline->SliceNode->GetViewGroup())
  {
    pipeline->SetVisibility(false);
    return;
  }



  pipeline->Property->SetColor(pipeline->SliceNode->GetLayoutColor());

  vtkMatrix4x4* intersectingXYToRAS = pipeline->SliceNode->GetXYToRAS();
  vtkMatrix4x4* xyToRAS = this->Internal->SliceNode->GetXYToRAS();

  //double slicePlaneAngleDifference = vtkMath::AngleBetweenVectors()

  vtkNew<vtkMatrix4x4> rasToXY;
  vtkMatrix4x4::Invert(xyToRAS, rasToXY);
  vtkNew<vtkMatrix4x4> intersectingXYToXY;
  vtkMatrix4x4::Multiply4x4(rasToXY, intersectingXYToRAS, intersectingXYToXY);

  double slicePlaneNormal[3] = { 0.,0.,1. };
  double slicePlaneOrigin[3] = { 0., 0., 0. };

  int* intersectingSliceSizeDimensions = pipeline->SliceNode->GetDimensions();
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
}

//----------------------------------------------------------------------
vtkMRMLSliceNode* vtkSliceIntersectionRepresentation2D::GetSliceNode()
{
  return this->Internal->SliceNode;
}

//----------------------------------------------------------------------
void vtkSliceIntersectionRepresentation2D::AddIntersectingSliceNode(vtkMRMLSliceNode* sliceNode)
{
  if (!sliceNode)
  {
    return;
  }
  if (sliceNode == this->Internal->SliceNode)
  {
    return;
  }
  if (this->GetDisplayPipelineFromSliceNode(sliceNode))
  {
    // slice node already added
    return;
  }
  SliceIntersectionDisplayPipeline* pipeline = new SliceIntersectionDisplayPipeline;
  pipeline->SetAndObserveSliceNode(sliceNode, this->Internal->SliceNodeModifiedCommand);
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
    if (sliceNode == (*sliceIntersectionIt)->SliceNode)
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
