/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLColorLogic.cxx,v $
  Date:      $Date$
  Version:   $Revision$

=========================================================================auto=*/

// MRMLLogic includes
#include "vtkMRMLColorLogic.h"

// MRML includes
#include "vtkMRMLColorTableNode.h"
#include "vtkMRMLColorTableStorageNode.h"
#include "vtkMRMLFreeSurferProceduralColorNode.h"
#include "vtkMRMLdGEMRICProceduralColorNode.h"
#include "vtkMRMLPETProceduralColorNode.h"
#include "vtkMRMLProceduralColorStorageNode.h"
#include "vtkMRMLScene.h"

// VTK sys includes
#include <vtkLookupTable.h>
#include <vtksys/SystemTools.hxx>

// VTK includes
#include <vtkCollection.h>
#include <vtkColorTransferFunction.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <cassert>

std::string vtkMRMLColorLogic::TempColorNodeID;

vtkStandardNewMacro(vtkMRMLColorLogic);

//----------------------------------------------------------------------------
vtkMRMLColorLogic::vtkMRMLColorLogic()
{
  this->UserColorFilePaths = NULL;
  this->DefaultColorsScene = NULL;
}

//----------------------------------------------------------------------------
vtkMRMLColorLogic::~vtkMRMLColorLogic()
{
  // clear out the lists of files
  this->ColorFiles.clear();
  this->UserColorFiles.clear();

  if (this->UserColorFilePaths)
    {
    delete [] this->UserColorFilePaths;
    this->UserColorFilePaths = NULL;
    }

  if (this->DefaultColorsScene)
    {
    this->DefaultColorsScene->Delete();
    this->DefaultColorsScene = NULL;
    }
}

//---------------------------------------------------------------------------
vtkMRMLScene* vtkMRMLColorLogic::GetDefaultColorsScene()
{
  if (!this->DefaultColorsScene)
    {
    this->DefaultColorsScene = vtkMRMLScene::New();
    this->LoadDefaultColors();
    }
  return this->DefaultColorsScene;
}

//---------------------------------------------------------------------------
vtkMRMLColorNode* vtkMRMLColorLogic::GetDefaultColorByName(
    const char* colorName)
{
  vtkMRMLScene * defaultColorsScene = this->GetDefaultColorsScene();
  if (!defaultColorsScene)
    {
    vtkErrorMacro("vtkMRMLColorLogic::GetDefaultColorByName failed: invalid DefaultColorsScene");
    return NULL;
    }
  if (!colorName)
    {
    vtkErrorMacro("vtkMRMLColorLogic::GetDefaultColorByName failed: invalid color name");
    return 0;
    }
  vtkSmartPointer<vtkCollection> defaultColors =
    vtkSmartPointer<vtkCollection>::Take(defaultColorsScene->GetNodesByClassByName("vtkMRMLColorNode", colorName));
  if (defaultColors->GetNumberOfItems() == 0)
    {
    return 0;
    }
  return vtkMRMLColorNode::SafeDownCast(defaultColors->GetItemAsObject(0));
}

//----------------------------------------------------------------------------
void vtkMRMLColorLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);

  os << indent << "vtkMRMLColorLogic:             " << this->GetClassName() << "\n";

  os << indent << "UserColorFilePaths: " << this->GetUserColorFilePaths() << "\n";
  os << indent << "Color Files:\n";
  for (int i = 0; i << this->ColorFiles.size(); i++)
    {
    os << indent.GetNextIndent() << i << " " << this->ColorFiles[i].c_str() << "\n";
    }
  os << indent << "User Color Files:\n";
  for (int i = 0; i << this->UserColorFiles.size(); i++)
    {
    os << indent.GetNextIndent() << i << " " << this->UserColorFiles[i].c_str() << "\n";
    }
}

//----------------------------------------------------------------------------
bool vtkMRMLColorLogic::LoadDefaultColors()
{
  if (this->DefaultColorsScene == NULL)
    {
    vtkWarningMacro("vtkMRMLColorLogic::LoadDefaultColors failed: invalid scene");
    return false;
    }

  this->DefaultColorsScene->StartState(vtkMRMLScene::BatchProcessState);

  // add the labels first
  this->AddLabelsNode();

  // add the rest of the default color table nodes
  this->AddDefaultTableNodes();

  // add default procedural nodes, including a random one
  this->AddDefaultProceduralNodes();

  // add freesurfer nodes
  this->AddFreeSurferNodes();

  // add the PET nodes
  this->AddPETNodes();

  // add the dGEMRIC nodes
  this->AddDGEMRICNodes();

  // file based labels
  // first check for any new ones

  // load the one from the default resources directory
  this->AddDefaultFileNodes();

  // now add ones in files that the user pointed to, these ones are not hidden
  // from the editors
  this->AddUserFileNodes();

  vtkDebugMacro("Done adding default color nodes");
  this->DefaultColorsScene->EndState(vtkMRMLScene::BatchProcessState);

  return true;
}

//----------------------------------------------------------------------------
const char *vtkMRMLColorLogic::GetColorTableNodeID(int type)
{
  vtkNew<vtkMRMLColorTableNode> basicNode;
  basicNode->SetType(type);
  return vtkMRMLColorLogic::GetColorNodeID(basicNode.GetPointer());
}

//----------------------------------------------------------------------------
const char * vtkMRMLColorLogic::GetFreeSurferColorNodeID(int type)
{
  vtkNew<vtkMRMLFreeSurferProceduralColorNode> basicNode;
  basicNode->SetType(type);
  return vtkMRMLColorLogic::GetColorNodeID(basicNode.GetPointer());
}

//----------------------------------------------------------------------------
const char * vtkMRMLColorLogic::GetPETColorNodeID (int type )
{
  vtkNew<vtkMRMLPETProceduralColorNode> basicNode;
  basicNode->SetType(type);
  return vtkMRMLColorLogic::GetColorNodeID(basicNode.GetPointer());
}

//----------------------------------------------------------------------------
const char * vtkMRMLColorLogic::GetdGEMRICColorNodeID(int type)
{
  vtkNew<vtkMRMLdGEMRICProceduralColorNode> basicNode;
  basicNode->SetType(type);
  return vtkMRMLColorLogic::GetColorNodeID(basicNode.GetPointer());
}

//----------------------------------------------------------------------------
const char *vtkMRMLColorLogic::GetColorNodeID(vtkMRMLColorNode* colorNode)
{
  assert(colorNode);
  std::string id = std::string(colorNode->GetClassName()) +
                   std::string(colorNode->GetTypeAsString());
  vtkMRMLColorLogic::TempColorNodeID = id;
  return vtkMRMLColorLogic::TempColorNodeID.c_str();
}

//----------------------------------------------------------------------------
const char * vtkMRMLColorLogic::GetProceduralColorNodeID(const char *name)
{
  std::string id = std::string("vtkMRMLProceduralColorNode") + std::string(name);
  vtkMRMLColorLogic::TempColorNodeID = id;
  return vtkMRMLColorLogic::TempColorNodeID.c_str();
}

//----------------------------------------------------------------------------
std::string vtkMRMLColorLogic::GetFileColorNodeSingletonTag(const char * fileName)
{
  std::string singleton = std::string("File") +
    vtksys::SystemTools::GetFilenameName(fileName);
  return singleton;
}

//----------------------------------------------------------------------------
const char *vtkMRMLColorLogic::GetFileColorNodeID(const char * fileName)
{
  std::string id = std::string("vtkMRMLColorTableNode") +
                   vtkMRMLColorLogic::GetFileColorNodeSingletonTag(fileName);
  vtkMRMLColorLogic::TempColorNodeID = id;
  return vtkMRMLColorLogic::TempColorNodeID.c_str();
}

//----------------------------------------------------------------------------
const char *vtkMRMLColorLogic::GetDefaultVolumeColorNodeID()
{
  return vtkMRMLColorLogic::GetColorTableNodeID(vtkMRMLColorTableNode::Grey);
}

//----------------------------------------------------------------------------
const char *vtkMRMLColorLogic::GetDefaultLabelMapColorNodeID()
{
  return vtkMRMLColorLogic::GetProceduralColorNodeID("RandomIntegers");
}

//----------------------------------------------------------------------------
const char *vtkMRMLColorLogic::GetDefaultEditorColorNodeID()
{
  return vtkMRMLColorLogic::GetProceduralColorNodeID("RandomIntegers");
}

//----------------------------------------------------------------------------
const char *vtkMRMLColorLogic::GetDefaultModelColorNodeID()
{
  return vtkMRMLColorLogic::GetFreeSurferColorNodeID(vtkMRMLFreeSurferProceduralColorNode::Heat);
}

//----------------------------------------------------------------------------
const char *vtkMRMLColorLogic::GetDefaultChartColorNodeID()
{
  return vtkMRMLColorLogic::GetProceduralColorNodeID("RandomIntegers");
}

//----------------------------------------------------------------------------
const char * vtkMRMLColorLogic::GetDefaultFreeSurferLabelMapColorNodeID()
{
  return vtkMRMLColorLogic::GetFreeSurferColorNodeID(vtkMRMLFreeSurferProceduralColorNode::Labels);
}

//----------------------------------------------------------------------------
void vtkMRMLColorLogic::AddColorFile(const char *fileName, std::vector<std::string> *Files)
{
  if (fileName == NULL)
    {
    vtkErrorMacro("AddColorFile: can't add a null color file name");
    return;
    }
  if (Files == NULL)
    {
    vtkErrorMacro("AddColorFile: no array to which to add color file to!");
    return;
    }
  // check if it's in the vector already
  std::string fileNameStr = std::string(fileName);
  for (unsigned int i = 0; i <  Files->size(); i++)
    {
    std::string fileToCheck;
    try
      {
      fileToCheck = Files->at(i);
      }
    catch (...)
      {
      // an out_of_range exception can be thrown.
      }
    if (fileToCheck.compare(fileNameStr) == 0)
      {
      vtkDebugMacro("AddColorFile: already have this file at index " << i << ", not adding it again: " << fileNameStr.c_str());
      return;
      }
    }
  vtkDebugMacro("AddColorFile: adding file name to Files: " << fileNameStr.c_str());
  Files->push_back(fileNameStr);
}

//----------------------------------------------------------------------------
vtkMRMLColorNode* vtkMRMLColorLogic::LoadColorFile(const char *fileName, const char *nodeName)
{
  // try loading it as a color table node first
  vtkMRMLColorTableNode* node = this->CreateFileNode(fileName);
  vtkMRMLColorNode * addedNode = NULL;

  if (node)
    {
    node->SetAttribute("Category", "File");
    node->SaveWithSceneOn();
    node->GetStorageNode()->SaveWithSceneOn();
    node->HideFromEditorsOff();
    node->SetSingletonTag(NULL);

    if (nodeName != NULL)
      {
      std::string uname( this->DefaultColorsScene->GetUniqueNameByString(nodeName));
      node->SetName(uname.c_str());
      }
    addedNode =
      vtkMRMLColorNode::SafeDownCast(this->DefaultColorsScene->AddNode(node));
    vtkDebugMacro("LoadColorFile: Done: Read and added file node: " <<  fileName);
    node->Delete();
    }
  else
    {
    // try loading it as a procedural node
    vtkWarningMacro("Trying to read color file as a procedural color node");
    vtkMRMLProceduralColorNode *procNode = this->CreateProceduralFileNode(fileName);
    if (procNode)
      {
      procNode->SetAttribute("Category", "File");
      procNode->SaveWithSceneOn();
      procNode->GetStorageNode()->SaveWithSceneOn();
      procNode->HideFromEditorsOff();
      procNode->SetSingletonTag(NULL);

      if (nodeName != NULL)
        {
        std::string uname( this->DefaultColorsScene->GetUniqueNameByString(nodeName));
        procNode->SetName(uname.c_str());
        }
      addedNode =
        vtkMRMLColorNode::SafeDownCast(this->DefaultColorsScene->AddNode(procNode));
      vtkDebugMacro("LoadColorFile: Done: Read and added file procNode: " <<  fileName);
      procNode->Delete();
      }
    }
  return addedNode;
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLColorLogic::CreateLabelsNode()
{
  vtkMRMLColorTableNode *labelsNode = vtkMRMLColorTableNode::New();
  labelsNode->SetTypeToLabels();
  labelsNode->SetAttribute("Category", "Discrete");
  labelsNode->SaveWithSceneOff();
  labelsNode->SetName(labelsNode->GetTypeAsString());
  labelsNode->SetSingletonTag(labelsNode->GetTypeAsString());
  return labelsNode;
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLColorLogic::CreateDefaultTableNode(int type)
{
  vtkMRMLColorTableNode *node = vtkMRMLColorTableNode::New();
  node->SetType(type);
  const char* typeName = node->GetTypeAsString();
  if (strstr(typeName, "Tint") != NULL)
    {
    node->SetAttribute("Category", "Tint");
    }
  else if (strstr(typeName, "Shade") != NULL)
    {
    node->SetAttribute("Category", "Shade");
    }
  else
    {
    node->SetAttribute("Category", "Discrete");
    }
  if (strcmp(typeName, "(unknown)") == 0)
    {
    return node;
    }
  node->SaveWithSceneOff();
  node->SetName(node->GetTypeAsString());
  node->SetSingletonTag(node->GetTypeAsString());
  return node;
}

//------------------------------------------------------------------------------
vtkMRMLProceduralColorNode* vtkMRMLColorLogic::CreateRandomNode()
{
  vtkDebugMacro("vtkMRMLColorLogic::CreateRandomNode: making a random  mrml proc color node");
  vtkMRMLProceduralColorNode *procNode = vtkMRMLProceduralColorNode::New();
  procNode->SetName("RandomIntegers");
  procNode->SetAttribute("Category", "Discrete");
  procNode->SaveWithSceneOff();
  procNode->SetSingletonTag(procNode->GetTypeAsString());

  vtkColorTransferFunction *func = procNode->GetColorTransferFunction();
  const int dimension = 1000;
  double table[3*dimension];
  double* tablePtr = table;
  for (int i = 0; i < dimension; ++i)
    {
    *tablePtr++ = static_cast<double>(rand())/RAND_MAX;
    *tablePtr++ = static_cast<double>(rand())/RAND_MAX;
    *tablePtr++ = static_cast<double>(rand())/RAND_MAX;
    }
  func->BuildFunctionFromTable(VTK_INT_MIN, VTK_INT_MAX, dimension, table);
  func->Build();
  procNode->SetNamesFromColors();

  return procNode;
}

//------------------------------------------------------------------------------
vtkMRMLProceduralColorNode* vtkMRMLColorLogic::CreateRedGreenBlueNode()
{
  vtkDebugMacro("vtkMRMLColorLogic::AddDefaultColorNodes: making a red - green - blue mrml proc color node");
  vtkMRMLProceduralColorNode *procNode = vtkMRMLProceduralColorNode::New();
  procNode->SetName("RedGreenBlue");
  procNode->SetAttribute("Category", "Continuous");
  procNode->SaveWithSceneOff();
  procNode->SetSingletonTag(procNode->GetTypeAsString());
  procNode->SetDescription("A color transfer function that maps from -6 to 6, red through green to blue");
  vtkColorTransferFunction *func = procNode->GetColorTransferFunction();
  func->SetColorSpaceToRGB();
  func->AddRGBPoint(-6.0, 1.0, 0.0, 0.0);
  func->AddRGBPoint(0.0, 0.0, 1.0, 0.0);
  func->AddRGBPoint(6.0, 0.0, 0.0, 1.0);

  procNode->SetNamesFromColors();

  return procNode;
}

//------------------------------------------------------------------------------
vtkMRMLFreeSurferProceduralColorNode* vtkMRMLColorLogic::CreateFreeSurferNode(int type)
{
  vtkMRMLFreeSurferProceduralColorNode *node = vtkMRMLFreeSurferProceduralColorNode::New();
  vtkDebugMacro("vtkMRMLColorLogic::AddDefaultColorNodes: setting free surfer proc color node type to " << type);
  node->SetType(type);
  node->SetAttribute("Category", "FreeSurfer");
  node->SaveWithSceneOff();
  if (node->GetTypeAsString() == NULL)
    {
    vtkWarningMacro("Node type as string is null");
    node->SetName("NoName");
    }
  else
    {
    node->SetName(node->GetTypeAsString());
    }
  vtkDebugMacro("vtkMRMLColorLogic::AddDefaultColorNodes: set proc node name to " << node->GetName());
  /*
  if (this->GetFreeSurferColorNodeID(i) == NULL)
    {
    vtkDebugMacro("Error getting default node id for freesurfer node " << i);
    }
  */
  vtkDebugMacro("vtkMRMLColorLogic::AddDefaultColorNodes: Getting default fs color node id for type " << type);
  node->SetSingletonTag(node->GetTypeAsString());
  return node;
}

//------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLColorLogic::CreateFreeSurferFileNode(const char* fileName)
{
  if (fileName == NULL)
    {
    vtkErrorMacro("Unable to get the labels file name, not adding");
    return 0;
    }

  vtkMRMLColorTableNode* node = this->CreateFileNode(fileName);

  if (!node)
    {
    return 0;
    }
  node->SetAttribute("Category", "FreeSurfer");
  node->SetName("FreeSurferLabels");
  node->SetSingletonTag(node->GetTypeAsString());

  return node;
}

//--------------------------------------------------------------------------------
vtkMRMLPETProceduralColorNode* vtkMRMLColorLogic::CreatePETColorNode(int type)
{
  vtkMRMLPETProceduralColorNode *nodepcn = vtkMRMLPETProceduralColorNode::New();
  nodepcn->SetType(type);
  nodepcn->SetAttribute("Category", "PET");
  nodepcn->SaveWithSceneOff();

  if (nodepcn->GetTypeAsString() == NULL)
    {
    vtkWarningMacro("Node type as string is null");
    nodepcn->SetName("NoName");
    }
  else
    {
    vtkDebugMacro("Got node type as string " << nodepcn->GetTypeAsString());
    nodepcn->SetName(nodepcn->GetTypeAsString());
    }

  nodepcn->SetSingletonTag(nodepcn->GetTypeAsString());

  return nodepcn;
}

//---------------------------------------------------------------------------------
vtkMRMLdGEMRICProceduralColorNode* vtkMRMLColorLogic::CreatedGEMRICColorNode(int type)
{
  vtkMRMLdGEMRICProceduralColorNode *pcnode = vtkMRMLdGEMRICProceduralColorNode::New();
  pcnode->SetType(type);
  pcnode->SetAttribute("Category", "Cartilage MRI");
  pcnode->SaveWithSceneOff();
  if (pcnode->GetTypeAsString() == NULL)
    {
    vtkWarningMacro("Node type as string is null");
    pcnode->SetName("NoName");
    }
  else
    {
    vtkDebugMacro("Got node type as string " << pcnode->GetTypeAsString());
    pcnode->SetName(pcnode->GetTypeAsString());
    }

  pcnode->SetSingletonTag(pcnode->GetTypeAsString());

  return pcnode;
}

//---------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLColorLogic::CreateDefaultFileNode(const std::string& colorFileName)
{
  vtkMRMLColorTableNode* ctnode = this->CreateFileNode(colorFileName.c_str());

  if (!ctnode)
    {
    return 0;
    }

  if (strcmp(ctnode->GetName(),"GenericColors") == 0 ||
      strcmp(ctnode->GetName(),"GenericAnatomyColors") == 0)
    {
    vtkDebugMacro("Found default lut node");
    // No category to float to the top of the node
    // can't unset an attribute, so just don't set it at all
    }
  else
    {
    ctnode->SetAttribute("Category", "Default Labels from File");
    }

  return ctnode;
}

//---------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLColorLogic::CreateUserFileNode(const std::string& colorFileName)
{
  vtkMRMLColorTableNode * ctnode = this->CreateFileNode(colorFileName.c_str());
  if (ctnode == 0)
    {
    return 0;
    }
  ctnode->SetAttribute("Category", "Auto Loaded User Color Files");
  ctnode->SaveWithSceneOn();
  ctnode->HideFromEditorsOff();

  return ctnode;
}

//--------------------------------------------------------------------------------
std::vector<std::string> vtkMRMLColorLogic::FindDefaultColorFiles()
{
  return std::vector<std::string>();
}

//--------------------------------------------------------------------------------
std::vector<std::string> vtkMRMLColorLogic::FindUserColorFiles()
{
  return std::vector<std::string>();
}

//--------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLColorLogic::CreateFileNode(const char* fileName)
{
  vtkMRMLColorTableNode * ctnode =  vtkMRMLColorTableNode::New();
  ctnode->SetTypeToFile();
  ctnode->SaveWithSceneOff();
  ctnode->HideFromEditorsOn();
  ctnode->SetScene(this->DefaultColorsScene);

  // make a storage node
  vtkNew<vtkMRMLColorTableStorageNode> colorStorageNode;
  colorStorageNode->SaveWithSceneOff();
  if (this->DefaultColorsScene)
    {
    this->DefaultColorsScene->AddNode(colorStorageNode.GetPointer());
    ctnode->SetAndObserveStorageNodeID(colorStorageNode->GetID());
    }

  ctnode->GetStorageNode()->SetFileName(fileName);
  std::string basename = vtksys::SystemTools::GetFilenameWithoutExtension(fileName);
  std::string uname( this->DefaultColorsScene->GetUniqueNameByString(basename.c_str()));
  ctnode->SetName(uname.c_str());

  vtkDebugMacro("CreateFileNode: About to read user file " << fileName);

  if (ctnode->GetStorageNode()->ReadData(ctnode) == 0)
    {
    vtkErrorMacro("Unable to read file as color table " << (ctnode->GetFileName() ? ctnode->GetFileName() : ""));

    if (this->DefaultColorsScene)
      {
      ctnode->SetAndObserveStorageNodeID(NULL);
      ctnode->SetScene(NULL);
      this->DefaultColorsScene->RemoveNode(colorStorageNode.GetPointer());
      }

      ctnode->Delete();
      return 0;
    }
  vtkDebugMacro("CreateFileNode: finished reading user file " << fileName);
  ctnode->SetSingletonTag(
    this->GetFileColorNodeSingletonTag(fileName).c_str());

  return ctnode;
}

//--------------------------------------------------------------------------------
vtkMRMLProceduralColorNode* vtkMRMLColorLogic::CreateProceduralFileNode(const char* fileName)
{
  vtkMRMLProceduralColorNode * cpnode =  vtkMRMLProceduralColorNode::New();
  cpnode->SetTypeToFile();
  cpnode->SaveWithSceneOff();
  cpnode->HideFromEditorsOn();
  cpnode->SetScene(this->DefaultColorsScene);

  // make a storage node
  vtkMRMLProceduralColorStorageNode *colorStorageNode = vtkMRMLProceduralColorStorageNode::New();
  colorStorageNode->SaveWithSceneOff();
  if (this->DefaultColorsScene)
    {
    this->DefaultColorsScene->AddNode(colorStorageNode);
    cpnode->SetAndObserveStorageNodeID(colorStorageNode->GetID());
    }
  colorStorageNode->Delete();

  cpnode->GetStorageNode()->SetFileName(fileName);
  std::string basename = vtksys::SystemTools::GetFilenameWithoutExtension(fileName);
  std::string uname( this->DefaultColorsScene->GetUniqueNameByString(basename.c_str()));
  cpnode->SetName(uname.c_str());

  vtkDebugMacro("CreateProceduralFileNode: About to read user file " << fileName);

  if (cpnode->GetStorageNode()->ReadData(cpnode) == 0)
    {
    vtkErrorMacro("Unable to read procedural colour file " << (cpnode->GetFileName() ? cpnode->GetFileName() : ""));
    if (this->DefaultColorsScene)
      {
      cpnode->SetAndObserveStorageNodeID(NULL);
      cpnode->SetScene(NULL);
      this->DefaultColorsScene->RemoveNode(colorStorageNode);
      }
      cpnode->Delete();
      return 0;
    }
  vtkDebugMacro("CreateProceduralFileNode: finished reading user procedural color file " << fileName);
  cpnode->SetSingletonTag(
    this->GetFileColorNodeSingletonTag(fileName).c_str());

  return cpnode;
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddLabelsNode()
{
  vtkSmartPointer<vtkMRMLColorNode> labelsNode = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreateLabelsNode());
  this->DefaultColorsScene->AddNode(labelsNode);
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddDefaultTableNode(int i)
{
  vtkSmartPointer<vtkMRMLColorNode> node = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreateDefaultTableNode(i));
  vtkDebugMacro("vtkMRMLColorLogic::AddDefaultTableNode: requesting id " << node->GetID() << endl);
  this->DefaultColorsScene->AddNode(node);
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddDefaultProceduralNodes()
{
  // random one
  vtkSmartPointer<vtkMRMLColorNode> randomNode = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreateRandomNode());
  this->DefaultColorsScene->AddNode(randomNode);

  // red green blue one
  vtkSmartPointer<vtkMRMLColorNode> rgbNode = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreateRedGreenBlueNode());
  this->DefaultColorsScene->AddNode(rgbNode);
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddFreeSurferNode(int type)
{
  vtkSmartPointer<vtkMRMLColorNode> node = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreateFreeSurferNode(type));
  vtkDebugMacro("vtkMRMLColorLogic::AddFreeSurferNode: requesting id " << node->GetID() << endl);
  this->DefaultColorsScene->AddNode(node);
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddFreeSurferFileNode(vtkMRMLFreeSurferProceduralColorNode* basicFSNode)
{
  vtkSmartPointer<vtkMRMLColorNode> node = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreateFreeSurferFileNode(basicFSNode->GetLabelsFileName()));
  vtkDebugMacro("vtkMRMLColorLogic::AddFreeSurferFileNode: requesting id " << node->GetID() << endl);
  this->DefaultColorsScene->AddNode(node);
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddPETNode(int type)
{
  vtkDebugMacro("AddPETNode: adding PET nodes");
  vtkSmartPointer<vtkMRMLColorNode> node = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreatePETColorNode(type));
  this->DefaultColorsScene->AddNode(node);
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddDGEMRICNode(int type)
{
  vtkDebugMacro("AddDGEMRICNode: adding dGEMRIC nodes");
  vtkSmartPointer<vtkMRMLColorNode> node = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreatedGEMRICColorNode(type));
  this->DefaultColorsScene->AddNode(node);
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddDefaultFileNode(int i)
{
  vtkSmartPointer<vtkMRMLColorNode> node = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreateDefaultFileNode(this->ColorFiles[i]));
  if (node.GetPointer()==NULL)
    {
    vtkErrorMacro("Unable to read color file " << this->ColorFiles[i].c_str());
    return;
    }
  this->DefaultColorsScene->AddNode(node);
  vtkDebugMacro("AddDefaultFileNode: Read and added file node: " <<  this->ColorFiles[i].c_str());
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddUserFileNode(int i)
{
  vtkSmartPointer<vtkMRMLColorNode> node = vtkSmartPointer<vtkMRMLColorNode>::Take(this->CreateUserFileNode(this->UserColorFiles[i]));
  if (node.GetPointer()==NULL)
    {
    vtkErrorMacro("Unable to read color file " << this->UserColorFiles[i].c_str());
    return;
    }
  this->DefaultColorsScene->AddNode(node);
  vtkDebugMacro("AddDefaultColorFiles: Read and added file node: " <<  this->ColorFiles[i].c_str());
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddDefaultTableNodes()
{
  vtkNew<vtkMRMLColorTableNode> basicNode;
  for (int i = basicNode->GetFirstType(); i <= basicNode->GetLastType(); i++)
    {
    // don't add a second Lables node, File node or the old atlas node
    if (i != vtkMRMLColorTableNode::Labels &&
        i != vtkMRMLColorTableNode::File &&
        i != vtkMRMLColorTableNode::Obsolete &&
        i != vtkMRMLColorTableNode::User)
      {
      this->AddDefaultTableNode(i);
      }
    }
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddFreeSurferNodes()
{
  vtkDebugMacro("AddDefaultColorNodes: Adding Freesurfer nodes");
  vtkNew<vtkMRMLFreeSurferProceduralColorNode> basicFSNode;
  vtkDebugMacro("AddDefaultColorNodes: first type = " <<  basicFSNode->GetFirstType() << ", last type = " <<  basicFSNode->GetLastType());
  for (int type = basicFSNode->GetFirstType(); type <= basicFSNode->GetLastType(); ++type)
    {
    this->AddFreeSurferNode(type);
    }

  // add a regular colour tables holding the freesurfer volume file colours and
  // surface colours
  this->AddFreeSurferFileNode(basicFSNode.GetPointer());
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddPETNodes()
{
  vtkNew<vtkMRMLPETProceduralColorNode> basicPETNode;
  for (int type = basicPETNode->GetFirstType(); type <= basicPETNode->GetLastType(); ++type)
    {
    this->AddPETNode(type);
    }
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddDGEMRICNodes()
{
  vtkNew<vtkMRMLdGEMRICProceduralColorNode> basicdGEMRICNode;
  for (int type = basicdGEMRICNode->GetFirstType(); type <= basicdGEMRICNode->GetLastType(); ++type)
    {
    this->AddDGEMRICNode(type);
    }
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddDefaultFileNodes()
{
  this->ColorFiles = this->FindDefaultColorFiles();
  vtkDebugMacro("AddDefaultColorNodes: found " <<  this->ColorFiles.size() << " default color files");
  for (unsigned int i = 0; i < this->ColorFiles.size(); i++)
    {
    this->AddDefaultFileNode(i);
    }
}

//----------------------------------------------------------------------------------------
void vtkMRMLColorLogic::AddUserFileNodes()
{
  this->UserColorFiles = this->FindUserColorFiles();
  vtkDebugMacro("AddDefaultColorNodes: found " <<  this->UserColorFiles.size() << " user color files");
  for (unsigned int i = 0; i < this->UserColorFiles.size(); i++)
    {
    this->AddUserFileNode(i);
    }

}

//----------------------------------------------------------------------------------------
vtkMRMLColorTableNode* vtkMRMLColorLogic::CopyNode(vtkMRMLColorNode* nodeToCopy, const char* copyName)
{
  vtkMRMLColorTableNode *colorNode = vtkMRMLColorTableNode::New();
  colorNode->SetName(copyName);
  colorNode->SetTypeToUser();
  colorNode->SetAttribute("Category", "User Generated");
  colorNode->SetHideFromEditors(false);
  if (nodeToCopy->GetLookupTable())
    {
    double* range = nodeToCopy->GetLookupTable()->GetRange();
    colorNode->GetLookupTable()->SetRange(range[0], range[1]);
    }
  colorNode->SetNumberOfColors(nodeToCopy->GetNumberOfColors());
  for (int i = 0; i < nodeToCopy->GetNumberOfColors(); ++i)
    {
    double color[4];
    nodeToCopy->GetColor(i, color);
    colorNode->SetColor(i, nodeToCopy->GetColorName(i), color[0], color[1], color[2], color[3]);
    }
  return colorNode;
}

//----------------------------------------------------------------------------------------
vtkMRMLProceduralColorNode* vtkMRMLColorLogic::CopyProceduralNode(vtkMRMLColorNode* nodeToCopy, const char* copyName)
{
  vtkMRMLProceduralColorNode *colorNode = vtkMRMLProceduralColorNode::New();
  if (nodeToCopy->IsA("vtkMRMLProceduralColorNode"))
    {
    colorNode->Copy(nodeToCopy);
    // the copy will copy any singleton tag, make sure it's unset
    colorNode->SetSingletonTag(NULL);
    }

  colorNode->SetName(copyName);
  colorNode->SetTypeToUser();
  colorNode->SetAttribute("Category", "User Generated");
  colorNode->SetHideFromEditors(false);

  return colorNode;
}

