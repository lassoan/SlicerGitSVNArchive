/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliceIntersectionRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkSliceIntersectionRepresentation2D
 * @brief   represent intersections of other slice views in the current slice view
 *
 * @sa
 * vtkSliceIntersectionWidget vtkWidgetRepresentation vtkAbstractWidget
*/

#ifndef vtkSliceIntersectionRepresentation2D_h
#define vtkSliceIntersectionRepresentation2D_h

#include "vtkMRMLDisplayableManagerExport.h" // For export macro
#include "vtkWidgetRepresentation.h"

#include "vtkMRMLSliceNode.h"

class vtkMRMLApplicationLogic;
class vtkMRMLModelDisplayNode;
class vtkMRMLSliceLogic;

class vtkProperty2D;
class vtkActor2D;
class vtkPolyDataMapper2D;
class vtkPolyData;
class vtkPoints;
class vtkCellArray;
class vtkTextProperty;
class vtkLeaderActor2D;
class vtkTextMapper;
class vtkTransform;
class vtkActor2D;

class SliceIntersectionDisplayPipeline;


class VTK_MRML_DISPLAYABLEMANAGER_EXPORT vtkSliceIntersectionRepresentation2D : public vtkWidgetRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static vtkSliceIntersectionRepresentation2D *New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  vtkTypeMacro(vtkSliceIntersectionRepresentation2D, vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * The tolerance representing the distance to the widget (in pixels)
   * in which the cursor is considered near enough to the widget to
   * be active.
   */
  vtkSetClampMacro(Tolerance,int,1,100);
  vtkGetMacro(Tolerance,int);
  //@}

  // Enums define the state of the representation relative to the mouse pointer
  // position. Used by ComputeInteractionState() to communicate with the
  // widget.
  enum _InteractionState
  {
    StateOutside=0,
    StateJump,
    StateTranslate, // move origin (all slices are centered on mouse pointer)
    StateRotate     // all slices are rotated around origin
  };

  //@{
  /**
   * Specify the origin of the widget (in world coordinates). The origin
   * is the point where the widget places itself. Note that rotations
   * occurs around the origin.
   */
  void SetOrigin(const double o[3]) {this->SetOrigin(o[0],o[1],o[2]);}
  void SetOrigin(double ox, double oy, double oz);
  vtkGetVector3Macro(Origin,double);
  //@}

  /**
   * Retrieve a linear transform characterizing the affine transformation
   * generated by this widget. This method copies its internal transform into
   * the transform provided. Note that the PlaceWidget() method initializes
   * the internal matrix to identity. All subsequent widget operations (i.e.,
   * scale, translate, rotate, shear) are concatenated with the internal
   * transform.
   */
  void SetSliceNode(vtkMRMLSliceNode* sliceNode);
  vtkMRMLSliceNode* GetSliceNode();

  void AddIntersectingSliceLogic(vtkMRMLSliceLogic* sliceLogic);
  void RemoveIntersectingSliceNode(vtkMRMLSliceNode* sliceNode);
  void UpdateIntersectingSliceNodes();
  void RemoveAllIntersectingSliceNodes();

  //@{
  /**
   * Set/Get the properties when unselected and selected.
   */
  void SetProperty(vtkProperty2D*);
  void SetSelectedProperty(vtkProperty2D*);
  vtkGetObjectMacro(Property,vtkProperty2D);
  vtkGetObjectMacro(SelectedProperty,vtkProperty2D);
  //@}

  //@{
  /**
   * Subclasses of vtkSliceIntersectionRepresentation2D must implement these methods. These
   * are the methods that the widget and its representation use to
   * communicate with each other. Note: PlaceWidget() reinitializes the
   * transformation matrix (i.e., sets it to identity). It also sets the
   * origin for scaling and rotation.
   */
  void PlaceWidget(double bounds[6]) override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  void EndWidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modify=0) override;
  void BuildRepresentation() override;
  //@}

  //@{
  /**
   * Methods to make this class behave as a vtkProp.
   */
  void ShallowCopy(vtkProp *prop) override;
  void GetActors2D(vtkPropCollection *) override;
  void ReleaseGraphicsResources(vtkWindow *) override;
  int RenderOverlay(vtkViewport *viewport) override;
  //@}

  void SetMRMLApplicationLogic(vtkMRMLApplicationLogic*);
  vtkGetObjectMacro(MRMLApplicationLogic, vtkMRMLApplicationLogic);

protected:
  vtkSliceIntersectionRepresentation2D();
  ~vtkSliceIntersectionRepresentation2D() override;

  SliceIntersectionDisplayPipeline* GetDisplayPipelineFromSliceLogic(vtkMRMLSliceLogic* sliceLogic);

  static void SliceNodeModifiedCallback(vtkObject* caller, unsigned long eid, void* clientData, void* callData);
  void SliceNodeModified(vtkMRMLSliceNode* sliceNode);
  void SliceModelDisplayNodeModified(vtkMRMLModelDisplayNode* sliceNode);

  void UpdateSliceIntersectionDisplay(SliceIntersectionDisplayPipeline *pipeline);

  double* GetSliceIntersectionPoint();

  // Methods to manipulate the cursor
  void Jump(double eventPos[2]);
  void Translate(double eventPos[2]);
  void Rotate(double eventPos[2]);

  // The tolerance for selecting different parts of the widget.
  double Tolerance;

  // The internal transformation matrix
  vtkTransform *CurrentTransform;
  vtkTransform *TotalTransform;
  double Origin[4]; //the current origin in world coordinates
  double DisplayOrigin[3]; //the current origin in display coordinates
  double CurrentTranslation[3]; //translation this movement
  double StartWorldPosition[4]; //Start event position converted to world

  double PreviousRotationAngleRad;
  double PreviousEventPosition[2];
  double StartRotationCenter[2];
  double StartRotationCenter_RAS[4];

  double CurrentAngle;
  double CurrentScale[2];
  double CurrentShear[2];

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty2D   *Property;
  vtkProperty2D   *SelectedProperty;
  void             CreateDefaultProperties();
  double           Opacity;
  double           SelectedOpacity;

  // Support picking
  double LastEventPosition[2];

  // Slice intersection point in XY
  double SliceIntersectionPoint[4];

  vtkMRMLApplicationLogic* MRMLApplicationLogic;

  class vtkInternal;
  vtkInternal * Internal;

private:
  vtkSliceIntersectionRepresentation2D(const vtkSliceIntersectionRepresentation2D&) = delete;
  void operator=(const vtkSliceIntersectionRepresentation2D&) = delete;
};

#endif