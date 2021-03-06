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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes

// SlicerQT includes
#include "qSlicerCLILoadableModuleFactory.h"
#include "qSlicerCLIModule.h"
#include "qSlicerCLIModuleFactoryHelper.h"
#include "qSlicerUtils.h"

// SlicerExecutionModule
#include <ModuleLogo.h>

//-----------------------------------------------------------------------------
qSlicerCLILoadableModuleFactoryItem::qSlicerCLILoadableModuleFactoryItem(
  const QString& newTempDirectory) : TempDirectory(newTempDirectory)
{
}

//-----------------------------------------------------------------------------
qSlicerAbstractCoreModule* qSlicerCLILoadableModuleFactoryItem::instanciator()
{
  // Using a scoped pointer ensures the memory will be cleaned if instantiator
  // fails before returning the module. See QScopedPointer::take()
  QScopedPointer<qSlicerCLIModule> module(new qSlicerCLIModule());

  // Resolves symbol
  const char* xmlDescription = const_cast<const char *>(reinterpret_cast<char*>(
    this->symbolAddress("XMLModuleDescription")));

  // Retrieve
  //if (!xmlDescription) { xmlDescription = xmlFunction ? (*xmlFunction)() : 0; }

  if (!xmlDescription)
    {
    this->appendInstantiateErrorString(QString("CLI loadable: %1").arg(this->path()));
    this->appendInstantiateErrorString("Failed to retrieve Xml Description");
    return 0;
    }

  // Resolves symbol
  typedef int (*ModuleEntryPointType)(int argc, char* argv[]);
  ModuleEntryPointType moduleEntryPoint =
    reinterpret_cast<ModuleEntryPointType>(
      this->symbolAddress("ModuleEntryPoint"));

  if (!moduleEntryPoint)
    {
    this->appendInstantiateErrorString(QString("CLI loadable: %1").arg(this->path()));
    this->appendInstantiateErrorString("Failed to retrieve Module Entry Point");
    return 0;
    }

  char buffer[256];
  // The entry point address must be encoded the same way it is decoded. As it
  // is decoded using  sscanf, it must be encoded using sprintf
  sprintf(buffer, "slicer:%p", moduleEntryPoint);
  module->setEntryPoint(QString(buffer));
  module->setModuleType("SharedObjectModule");

  module->setXmlModuleDescription(QString(xmlDescription));
  module->setTempDirectory(this->TempDirectory);
  module->setPath(this->path());
  module->setInstalled(qSlicerCLIModuleFactoryHelper::isInstalled(this->path()));
  module->setBuiltIn(qSlicerCLIModuleFactoryHelper::isBuiltIn(this->path()));

  ModuleLogo logo;
  if (updateLogo(this, logo))
    {
    module->setLogo(logo);
    }

  return module.take();
}

//-----------------------------------------------------------------------------
bool qSlicerCLILoadableModuleFactoryItem::updateLogo(qSlicerCLILoadableModuleFactoryItem* item,
                                                     ModuleLogo& logo)
{
  if (!item)
    {
    return false;
    }

  const char* logoImage = 0;
  int width = 0;
  int height = 0;
  int pixelSize = 0;
  unsigned long bufferLength = 0;

  void * resolvedGetModuleLogoSymbol = item->symbolAddress("GetModuleLogo");
  void * resolvedModuleLogoImageSymbol = item->symbolAddress("ModuleLogoImage");

  if(resolvedGetModuleLogoSymbol)
    {
    typedef const char * (*ModuleLogoFunction)(
          int* /*width*/, int* /*height*/, int* /*pixel_size*/, unsigned long * /*bufferLength*/);
    ModuleLogoFunction logoFunction =
        reinterpret_cast<ModuleLogoFunction>(resolvedGetModuleLogoSymbol);
    logoImage = (*logoFunction)(&width, &height, &pixelSize, &bufferLength);
    }
  else if(resolvedModuleLogoImageSymbol)
    {
    logoImage = reinterpret_cast<const char *>(resolvedModuleLogoImageSymbol);
    QStringList expectedSymbols;
    expectedSymbols << "ModuleLogoWidth" << "ModuleLogoHeight"
                    << "ModuleLogoPixelSize" << "ModuleLogoLength";
    QList<void*> resolvedSymbols;
    foreach(const QString& symbol, expectedSymbols)
      {
      void * resolvedSymbol = item->symbolAddress(symbol);
      if (resolvedSymbol)
        {
        resolvedSymbols << resolvedSymbol;
        }
      else
        {
        item->appendLoadErrorString(QString("Failed to resolve expected symbol '%1'").arg(symbol));
        }
      }
    if (resolvedSymbols.count() == 4)
      {
      width = *reinterpret_cast<int *>(resolvedSymbols.at(0));
      height = *reinterpret_cast<int *>(resolvedSymbols.at(1));
      pixelSize = *reinterpret_cast<int *>(resolvedSymbols.at(2));
      bufferLength = *reinterpret_cast<unsigned long *>(resolvedSymbols.at(3));
      }
    }

  if(resolvedGetModuleLogoSymbol || resolvedModuleLogoImageSymbol)
    {
    logo.SetLogo(logoImage, width, height, pixelSize, bufferLength, 0);
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
// qSlicerCLILoadableModuleFactoryPrivate

//-----------------------------------------------------------------------------
class qSlicerCLILoadableModuleFactoryPrivate
{
  Q_DECLARE_PUBLIC(qSlicerCLILoadableModuleFactory);
protected:
  qSlicerCLILoadableModuleFactory* const q_ptr;
public:
  typedef qSlicerCLILoadableModuleFactoryPrivate Self;
  qSlicerCLILoadableModuleFactoryPrivate(qSlicerCLILoadableModuleFactory& object);

  void init();

private:
  QString TempDirectory;
};

//-----------------------------------------------------------------------------
qSlicerCLILoadableModuleFactoryPrivate::qSlicerCLILoadableModuleFactoryPrivate(qSlicerCLILoadableModuleFactory& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
void qSlicerCLILoadableModuleFactoryPrivate::init()
{
  Q_Q(qSlicerCLILoadableModuleFactory);
  // Set the list of required symbols for CmdLineLoadableModule,
  // if one of these symbols can't be resolved, the library won't be registered.
  q->setSymbols(QStringList() << "XMLModuleDescription" << "ModuleEntryPoint");
  this->TempDirectory = QDir::tempPath();
}

//-----------------------------------------------------------------------------
// qSlicerCLILoadableModuleFactory

//-----------------------------------------------------------------------------
qSlicerCLILoadableModuleFactory::qSlicerCLILoadableModuleFactory()
  : d_ptr(new qSlicerCLILoadableModuleFactoryPrivate(*this))
{
  Q_D(qSlicerCLILoadableModuleFactory);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerCLILoadableModuleFactory::~qSlicerCLILoadableModuleFactory()
{
}

//-----------------------------------------------------------------------------
void qSlicerCLILoadableModuleFactory::registerItems()
{
  QStringList modulePaths = qSlicerCLIModuleFactoryHelper::modulePaths();
  this->registerAllFileItems(modulePaths);
}

//-----------------------------------------------------------------------------
ctkAbstractFactoryItem<qSlicerAbstractCoreModule>* qSlicerCLILoadableModuleFactory::
createFactoryFileBasedItem()
{
  Q_D(qSlicerCLILoadableModuleFactory);
  return new qSlicerCLILoadableModuleFactoryItem(d->TempDirectory);
}

//-----------------------------------------------------------------------------
QString qSlicerCLILoadableModuleFactory::fileNameToKey(const QString& fileName)const
{
  return qSlicerUtils::extractModuleNameFromLibraryName(fileName);
}

//-----------------------------------------------------------------------------
void qSlicerCLILoadableModuleFactory::setTempDirectory(const QString& newTempDirectory)
{
  Q_D(qSlicerCLILoadableModuleFactory);
  d->TempDirectory = newTempDirectory;
}

//-----------------------------------------------------------------------------
bool qSlicerCLILoadableModuleFactory::isValidFile(const QFileInfo& file)const
{
  if (!Superclass::isValidFile(file))
    {
    return false;
    }
  return qSlicerUtils::isCLILoadableModule(file.absoluteFilePath());
}
