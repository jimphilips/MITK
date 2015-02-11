/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "mitkPluginActivator.h"

#include "mitkLog.h"

#include <QString>
#include <QFileInfo>

#include "internal/mitkDataStorageService.h"

#include <usModuleRegistry.h>
#include <usModule.h>
#include <usModuleContext.h>

#include <mitkVtkLoggingAdapter.h>
#include <mitkItkLoggingAdapter.h>


namespace mitk
{

class InterfaceMapToQObjectAdapter : public QObject
{

public:

  InterfaceMapToQObjectAdapter(const us::InterfaceMap& im)
    : interfaceMap(im)
  {}

  // This method is called by the Qt meta object system. It is usually
  // generated by the moc, but we create it manually to be able to return
  // a MITK micro service object (derived from itk::LightObject). It basically
  // works as if the micro service class had used the Q_INTERFACES macro in
  // its declaration. Now we can successfully do a
  // qobject_cast<mitk::SomeMicroServiceInterface>(lightObjectToQObjectAdapter)
  void* qt_metacast(const char *_clname)
  {
    if (!_clname) return 0;
    if (!strcmp(_clname, "InterfaceMapToQObjectAdapter"))
      return static_cast<void*>(const_cast<InterfaceMapToQObjectAdapter*>(this));

    us::InterfaceMap::const_iterator iter = interfaceMap.find(_clname);
    if (iter != interfaceMap.end())
      return iter->second;

    return QObject::qt_metacast(_clname);
  }

private:

  us::InterfaceMap interfaceMap;
};

const std::string org_mitk_core_services_Activator::PLUGIN_ID = "org.mitk.core.services";

void org_mitk_core_services_Activator::start(ctkPluginContext* context)
{
  pluginContext = context;

  //initialize logging
  mitk::LoggingBackend::Register();
  QString logFilenamePrefix = "mitk";
  QFileInfo path = context->getDataFile(logFilenamePrefix);
  try
    {
    mitk::LoggingBackend::RotateLogFiles(path.absoluteFilePath().toStdString());
    }
  catch(mitk::Exception& e)
    {
      MITK_ERROR << "Problem during logfile initialization: " << e.GetDescription() << " Caution: Logging to harddisc might be disabled!";
    }
  mitk::VtkLoggingAdapter::Initialize();
  mitk::ItkLoggingAdapter::Initialize();

  //initialize data storage service
  DataStorageService* service = new DataStorageService();
  dataStorageService = IDataStorageService::Pointer(service);
  context->registerService<mitk::IDataStorageService>(service);

  // Get the MitkCore Module Context
  mitkContext = us::ModuleRegistry::GetModule(1)->GetModuleContext();

  // Process all already registered services
  std::vector<us::ServiceReferenceU> refs = mitkContext->GetServiceReferences("");
  for (std::vector<us::ServiceReferenceU>::const_iterator i = refs.begin();
       i != refs.end(); ++i)
  {
    this->AddMitkService(*i);
  }

  mitkContext->AddServiceListener(this, &org_mitk_core_services_Activator::MitkServiceChanged);
}

void org_mitk_core_services_Activator::stop(ctkPluginContext* /*context*/)
{
  mitkContext->RemoveServiceListener(this, &org_mitk_core_services_Activator::MitkServiceChanged);

  foreach(ctkServiceRegistration reg, mapMitkIdToRegistration.values())
  {
    reg.unregister();
  }
  mapMitkIdToRegistration.clear();

  qDeleteAll(mapMitkIdToAdapter);
  mapMitkIdToAdapter.clear();

  //clean up logging
  mitk::LoggingBackend::Unregister();

  dataStorageService = 0;
  mitkContext = 0;
  pluginContext = 0;
}

void org_mitk_core_services_Activator::MitkServiceChanged(const us::ServiceEvent event)
{
  switch (event.GetType())
  {
  case us::ServiceEvent::REGISTERED:
  {
    this->AddMitkService(event.GetServiceReference());
    break;
  }
  case us::ServiceEvent::UNREGISTERING:
  {
    long mitkServiceId = us::any_cast<long>(event.GetServiceReference().GetProperty(us::ServiceConstants::SERVICE_ID()));
    ctkServiceRegistration reg = mapMitkIdToRegistration.take(mitkServiceId);
    if (reg)
    {
      reg.unregister();
    }
    delete mapMitkIdToAdapter.take(mitkServiceId);
    break;
  }
  case us::ServiceEvent::MODIFIED:
  {
    long mitkServiceId = us::any_cast<long>(event.GetServiceReference().GetProperty(us::ServiceConstants::SERVICE_ID()));
    ctkDictionary newProps = CreateServiceProperties(event.GetServiceReference());
    mapMitkIdToRegistration[mitkServiceId].setProperties(newProps);
    break;
  }
  default:
    break; // do nothing
  }
}

void org_mitk_core_services_Activator::AddMitkService(const us::ServiceReferenceU& ref)
{
  // Get the MITK micro service object
  us::InterfaceMap mitkService = mitkContext->GetService(ref);
  if (mitkService.empty()) return;

  // Get the interface names against which the service was registered
  QStringList qclazzes;
  for(us::InterfaceMap::const_iterator clazz = mitkService.begin();
      clazz != mitkService.end(); ++clazz)
  {
    qclazzes << QString::fromStdString(clazz->first);
  }

  long mitkServiceId = us::any_cast<long>(ref.GetProperty(us::ServiceConstants::SERVICE_ID()));

  QObject* adapter = new InterfaceMapToQObjectAdapter(mitkService);
  mapMitkIdToAdapter[mitkServiceId] = adapter;

  ctkDictionary props = CreateServiceProperties(ref);
  mapMitkIdToRegistration[mitkServiceId] = pluginContext->registerService(qclazzes, adapter, props);
}

ctkDictionary org_mitk_core_services_Activator::CreateServiceProperties(const us::ServiceReferenceU& ref)
{
  ctkDictionary props;

  long mitkServiceId = us::any_cast<long>(ref.GetProperty(us::ServiceConstants::SERVICE_ID()));
  props.insert("mitk.serviceid", QVariant::fromValue(mitkServiceId));

  // Add all other properties from the MITK micro service
  std::vector<std::string> keys;
  ref.GetPropertyKeys(keys);
  for (std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); ++it)
  {
    QString key = QString::fromStdString(*it);
    us::Any value = ref.GetProperty(*it);
    // We cannot add any mitk::Any object, we need to query the type
    const std::type_info& objType = value.Type();
    if (objType == typeid(std::string))
    {
      props.insert(key, QString::fromStdString(us::ref_any_cast<std::string>(value)));
    }
    else if (objType == typeid(std::vector<std::string>))
    {
      const std::vector<std::string>& list = us::ref_any_cast<std::vector<std::string> >(value);
      QStringList qlist;
      for (std::vector<std::string>::const_iterator str = list.begin();
           str != list.end(); ++str)
      {
         qlist << QString::fromStdString(*str);
      }
      props.insert(key, qlist);
    }
    else if (objType == typeid(std::list<std::string>))
    {
      const std::list<std::string>& list = us::ref_any_cast<std::list<std::string> >(value);
      QStringList qlist;
      for (std::list<std::string>::const_iterator str = list.begin();
           str != list.end(); ++str)
      {
         qlist << QString::fromStdString(*str);
      }
      props.insert(key, qlist);
    }
    else if (objType == typeid(char))
    {
      props.insert(key, QChar(us::ref_any_cast<char>(value)));
    }
    else if (objType == typeid(unsigned char))
    {
      props.insert(key, QChar(us::ref_any_cast<unsigned char>(value)));
    }
    else if (objType == typeid(bool))
    {
      props.insert(key, us::any_cast<bool>(value));
    }
    else if (objType == typeid(short))
    {
      props.insert(key, us::any_cast<short>(value));
    }
    else if (objType == typeid(unsigned short))
    {
      props.insert(key, us::any_cast<unsigned short>(value));
    }
    else if (objType == typeid(int))
    {
      props.insert(key, us::any_cast<int>(value));
    }
    else if (objType == typeid(unsigned int))
    {
      props.insert(key, us::any_cast<unsigned int>(value));
    }
    else if (objType == typeid(float))
    {
      props.insert(key, us::any_cast<float>(value));
    }
    else if (objType == typeid(double))
    {
      props.insert(key, us::any_cast<double>(value));
    }
    else if (objType == typeid(long long int))
    {
      props.insert(key, us::any_cast<long long int>(value));
    }
    else if (objType == typeid(unsigned long long int))
    {
      props.insert(key, us::any_cast<unsigned long long int>(value));
    }
  }

  return props;
}

org_mitk_core_services_Activator::org_mitk_core_services_Activator()
  : mitkContext(0), pluginContext(0)
{
}

}
