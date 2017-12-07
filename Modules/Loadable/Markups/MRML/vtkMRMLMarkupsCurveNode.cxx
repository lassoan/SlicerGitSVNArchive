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

// MRML includes
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsCurveNode.h"
#include "vtkMRMLMarkupsCurveStorageNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsCurveNode);


//----------------------------------------------------------------------------
vtkMRMLMarkupsCurveNode::vtkMRMLMarkupsCurveNode()
{

}

//----------------------------------------------------------------------------
vtkMRMLMarkupsCurveNode::~vtkMRMLMarkupsCurveNode()
{

}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);

  //of << " locked=\"" << this->Locked << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  this->RemoveAllMarkups();

  Superclass::ReadXMLAttributes(atts);
  const char* attName;
  const char* attValue;

  int fidID = 0;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);
    // backward compatibility reading of annotation fiducials
    if (!strcmp(attName, "ctrlPtsCoord"))
      {
      std::string valStr(attValue);
      std::stringstream ss;
      double x, y, z;
      ss << valStr;
      ss >> x;
      ss >> y;
      ss >> z;
      fidID = this->AddFiducial(x,y,z);
      }
    else if (!strcmp(attName, "ctrlPtsSelected"))
      {
      std::stringstream ss;
      int selected;
      ss << attValue;
      ss >> selected;
      this->SetNthFiducialSelected(fidID, (selected == 1 ? true : false));
      }
    else if (!strcmp(attName, "ctrlPtsVisible"))
      {
      std::stringstream ss;
      int visible;
      ss << attValue;
      ss >> visible;
      this->SetNthFiducialVisibility(fidID, (visible == 1 ? true : false));
      }
    else if (!strcmp(attName, "ctrlPtsNumberingScheme"))
      {
      // ignore
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}


//-----------------------------------------------------------
void vtkMRMLMarkupsCurveNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event,
                                           void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLMarkupsCurveNode::CreateDefaultStorageNode()
{
  return vtkMRMLStorageNode::SafeDownCast(vtkMRMLMarkupsCurveStorageNode::New());
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::CreateDefaultDisplayNodes()
{
  if (this->GetDisplayNode() != NULL &&
      vtkMRMLMarkupsDisplayNode::SafeDownCast(this->GetDisplayNode()) != NULL)
    {
    // display node already exists
    return;
    }
  if (this->GetScene()==NULL)
    {
    vtkErrorMacro("vtkMRMLMarkupsCurveNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
    }
  vtkNew<vtkMRMLMarkupsDisplayNode> dispNode;
  this->GetScene()->AddNode(dispNode.GetPointer());
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//-------------------------------------------------------------------------
vtkMRMLMarkupsDisplayNode *vtkMRMLMarkupsCurveNode::GetMarkupsDisplayNode()
{
  vtkMRMLDisplayNode *displayNode = this->GetDisplayNode();
  if (displayNode &&
      displayNode->IsA("vtkMRMLMarkupsDisplayNode"))
    {
    return vtkMRMLMarkupsDisplayNode::SafeDownCast(displayNode);
    }
  return NULL;
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsCurveNode::AddFiducial(double x, double y, double z)
{
  return this->AddFiducial(x, y, z, std::string());
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsCurveNode::AddFiducial(double x, double y, double z,
                                            std::string label)
{
  vtkVector3d point;
  point.SetX(x);
  point.SetY(y);
  point.SetZ(z);
  return this->AddPointToNewMarkup(point, label);
}

//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
int vtkMRMLMarkupsCurveNode::AddFiducialFromArray(double pos[3], std::string label)
{
  return this->AddFiducial(pos[0], pos[1], pos[2], label);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::GetNthFiducialPosition(int n, double pos[3])
{
  vtkVector3d point= this->GetMarkupPointVector(n, 0);
  pos[0] = point.GetX();
  pos[1] = point.GetY();
  pos[2] = point.GetZ();
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthFiducialPositionFromArray(int n, double pos[3])
{
  this->SetMarkupPoint(n, 0, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthFiducialPosition(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, 0, x, y, z);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsCurveNode::GetNthFiducialSelected(int n)
{
  return this->GetNthMarkupSelected(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthFiducialSelected(int n, bool flag)
{
  this->SetNthMarkupSelected(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsCurveNode::GetNthFiducialVisibility(int n)
{
  return this->GetNthMarkupVisibility(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthFiducialVisibility(int n, bool flag)
{
  this->SetNthMarkupVisibility(n, flag);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsCurveNode::GetNthFiducialLabel(int n)
{
  return this->GetNthMarkupLabel(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthFiducialLabel(int n, std::string label)
{
  this->SetNthMarkupLabel(n, label);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsCurveNode::GetNthFiducialAssociatedNodeID(int n)
{
  return this->GetNthMarkupAssociatedNodeID(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthFiducialAssociatedNodeID(int n, const char* id)
{
  this->SetNthMarkupAssociatedNodeID(n, std::string(id));
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::SetNthFiducialWorldCoordinates(int n, double coords[4])
{
  this->SetMarkupPointWorld(n, 0, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::GetNthFiducialWorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, 0, coords);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::GetRASBounds(double bounds[6])
{
  vtkBoundingBox box;
  box.GetBounds(bounds);

  int numberOfMarkups = this->GetNumberOfMarkups();
  if (numberOfMarkups == 0)
    {
    return;
    }
  double markup_RAS[4] = { 0, 0, 0, 1 };

  for (int i = 0; i < numberOfMarkups; i++)
    {
    this->GetNthFiducialWorldCoordinates(i, markup_RAS);
    box.AddPoint(markup_RAS);
    }
  box.GetBounds(bounds);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsCurveNode::GetBounds(double bounds[6])
{
   vtkBoundingBox box;
  box.GetBounds(bounds);

  int numberOfMarkups = this->GetNumberOfMarkups();
  if (numberOfMarkups == 0)
    {
    return;
    }
  double markupPos[4] = { 0, 0, 0 };

  for (int i = 0; i < numberOfMarkups; i++)
    {
    this->GetNthFiducialPosition(i, markupPos);
    box.AddPoint(markupPos);
    }
  box.GetBounds(bounds);
}
