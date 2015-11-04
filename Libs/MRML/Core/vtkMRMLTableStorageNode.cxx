/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kevin Wang, PMH.
  and was partially funded by OCAIRO and Sparkit.

==============================================================================*/

// MRML includes
#include "vtkMRMLTableStorageNode.h"
#include "vtkMRMLTableNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkDelimitedTextReader.h>
#include <vtkDelimitedTextWriter.h>
#include <vtkTable.h>
#include <vtkStringArray.h>
#include <vtkNew.h>
#include <vtksys/SystemTools.hxx>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTableStorageNode);

//----------------------------------------------------------------------------
vtkMRMLTableStorageNode::vtkMRMLTableStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLTableStorageNode::~vtkMRMLTableStorageNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLTableStorageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLStorageNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkMRMLTableStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLTableNode");
}

//----------------------------------------------------------------------------
int vtkMRMLTableStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  std::string fullName = this->GetFullNameFromFileName();

  if (fullName == std::string(""))
    {
    vtkErrorMacro("vtkMRMLTableStorageNode: File name not specified");
    return 0;
    }

  // cast the input node
  vtkMRMLTableNode *tableNode =
    vtkMRMLTableNode::SafeDownCast(refNode);

  if (tableNode == NULL)
    {
    vtkErrorMacro("ReadData: unable to cast input node " << refNode->GetID() << " to a table node");
    return 0;
    }

  // check that the file exists
  if (vtksys::SystemTools::FileExists(fullName.c_str()) == false)
    {
    vtkErrorMacro("ReadDataInternal: model file '" << fullName.c_str() << "' not found.");
    return 0;
    }

  // compute file prefix
  std::string extension = vtkMRMLStorageNode::GetLowercaseExtensionFromFileName(fullName);
  if( extension.empty() )
    {
    vtkErrorMacro("ReadData: no file extension specified: " << fullName.c_str());
    return 0;
    }

  vtkDebugMacro("ReadDataInternal: extension = " << extension.c_str());

  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(fullName.c_str());
  reader->SetHaveHeaders(true);
  reader->DetectNumericColumnsOff(); // we preserve all data better if we don't try to detect numeric columns

  int result = 1;
  try
    {
    if ( extension == std::string(".tsv") || extension == std::string(".txt"))
      {
      reader->SetFieldDelimiterCharacters("\t");
      reader->Update();
      tableNode->SetAndObserveTable(reader->GetOutput());
      }
    else if ( extension == std::string(".csv") )
      {
      reader->SetFieldDelimiterCharacters(",");
      reader->Update();
      tableNode->SetAndObserveTable(reader->GetOutput());
      }
    else
      {
      vtkDebugMacro("Cannot read table file '" << fullName.c_str() << "' (extension = " << extension.c_str() << ")");
      return 0;
      }
    }
  catch (...)
    {
    result = 0;
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkMRMLTableStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  if (this->GetFileName() == NULL)
    {
    vtkErrorMacro("WriteData: file name is not set");
    return 0;
    }

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName.empty())
    {
    vtkErrorMacro("vtkMRMLTableStorageNode: File name not specified");
    return 0;
    }

  // cast the input node
  vtkMRMLTableNode *tableNode =
    vtkMRMLTableNode::SafeDownCast(refNode);

  if (tableNode == NULL)
    {
    vtkErrorMacro("WriteData: unable to cast input node " << refNode->GetID() << " to a known table node");
    return 0;
    }

  std::string extension = vtkMRMLStorageNode::GetLowercaseExtensionFromFileName(fullName);

  int result = 1;
  if (extension == ".tsv" || extension == ".txt" || extension == ".csv")
    {
    vtkNew<vtkDelimitedTextWriter> writer;
    writer->SetFileName(fullName.c_str());
    writer->SetInputData( tableNode->GetTable() );
    writer->SetFieldDelimiter("\t"); // use tab as delimiter for tsv and txt
    if (extension == ".csv")
      {
      writer->SetFieldDelimiter(",");
      }
    try
      {
      writer->Write();
      }
    catch (...)
      {
      result = 0;
      vtkErrorMacro( << "Failed to write file: " << fullName.c_str() );
      }
    }
  else
    {
    result = 0;
    vtkErrorMacro( << "No file extension recognized: " << fullName.c_str() );
    }

  return result;
}

//----------------------------------------------------------------------------
void vtkMRMLTableStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Tab-separated values (.tsv)");
  this->SupportedReadFileTypes->InsertNextValue("Comma-separated values (.csv)");
  this->SupportedReadFileTypes->InsertNextValue("Text (.txt)");
}

//----------------------------------------------------------------------------
void vtkMRMLTableStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Tab-separated values (.tsv)");
  this->SupportedWriteFileTypes->InsertNextValue("Comma-separated values (.csv)");
  this->SupportedWriteFileTypes->InsertNextValue("Text (.txt)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLTableStorageNode::GetDefaultWriteFileExtension()
{
  return "tsv";
}
