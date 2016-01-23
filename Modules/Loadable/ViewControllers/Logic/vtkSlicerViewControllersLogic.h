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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// .NAME vtkSlicerViewControllersLogic - slicer logic class for SliceViewControllers module

#ifndef __vtkSlicerViewControllersLogic_h
#define __vtkSlicerViewControllersLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// STD includes
#include <cstdlib>

#include "vtkSlicerViewControllersModuleLogicExport.h"

class vtkMRMLSliceNode;
class vtkMRMLViewNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_VIEWCONTROLLERS_MODULE_LOGIC_EXPORT vtkSlicerViewControllersLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerViewControllersLogic *New();
  vtkTypeMacro(vtkSlicerViewControllersLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  vtkMRMLSliceNode* GetDefaultSliceViewNode();
  vtkMRMLViewNode* GetDefaultThreeDViewNode();
  void ResetAllViewNodesToDefault();

  /*
  vtkGetMacro( DefaultSliceViewNode, vtkMRMLSliceNode* );
  vtkGetMacro( DefaultThreeDViewNode, vtkMRMLViewNode* );
  */

protected:
  vtkSlicerViewControllersLogic();
  virtual ~vtkSlicerViewControllersLogic();

  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();

  /*
  vtkMRMLSliceNode* DefaultSliceViewNode;
  vtkMRMLViewNode* DefaultThreeDViewNode;
  */

private:
  vtkSlicerViewControllersLogic(const vtkSlicerViewControllersLogic&); // Not implemented
  void operator=(const vtkSlicerViewControllersLogic&);               // Not implemented

};

#endif
