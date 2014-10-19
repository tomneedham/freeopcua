/******************************************************************************
 *   Copyright (C) 2013-2014 by Alexander Rykovanov                        *
 *   rykovanov.as@gmail.com                                                   *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation; version 3 of the License.     *
 *                                                                            *
 *   This library is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public License *
 *   along with this library; if not, write to the                            *
 *   Free Software Foundation, Inc.,                                          *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                *
 ******************************************************************************/

#include "common_addons.h"
#include "endpoints_parameters.h"
#include "server_object_addon.h"

#include <opc/common/addons_core/config_file.h>
#include <opc/ua/server/addons/asio_addon.h>
#include <opc/ua/server/addons/address_space.h>
#include <opc/ua/server/addons/endpoints_services.h>
#include <opc/ua/server/addons/opcua_protocol.h>
#include <opc/ua/server/addons/opc_tcp_async.h>
#include <opc/ua/server/addons/services_registry.h>
#include <opc/ua/server/addons/standard_namespace.h>
#include <opc/ua/server/addons/subscription_service.h>

#include <algorithm>

namespace
{

  using namespace OpcUa;

  void AddParameters(Common::AddonInformation& info, const Common::ParametersGroup& params)
  {
    info.Parameters.Groups = params.Groups;
    info.Parameters.Parameters = params.Parameters;
  }

  void CreateCommonAddonsConfiguration(const Common::AddonParameters& params, std::vector<Common::AddonInformation>& addons)
  {
    Common::AddonInformation endpointsRegistry = Server::CreateEndpointsRegistryAddon();
    Common::AddonInformation addressSpaceRegistry = Server::CreateAddressSpaceAddon();
    Common::AddonInformation asioAddon = Server::CreateAsioAddon();
    Common::AddonInformation subscriptionService = Server::CreateSubscriptionServiceAddon();
    Common::AddonInformation serverObject = Server::CreateServerObjectAddon();

    for (const Common::ParametersGroup& group : params.Groups)
    {
      if (group.Name == OpcUa::Server::EndpointsRegistryAddonID)
      {
        AddParameters(endpointsRegistry, group);
      }
      else if (group.Name == OpcUa::Server::OpcUaProtocolAddonID)
      {
        Common::AddonInformation binaryProtocol = Server::CreateBinaryServerAddon();
        AddParameters(binaryProtocol, group);
        addons.push_back(binaryProtocol);
      }
      else if (group.Name == OpcUa::Server::AddressSpaceRegistryAddonID)
      {
        AddParameters(addressSpaceRegistry, group);
      }
      else if (group.Name == OpcUa::Server::AsyncOpcTcpAddonID)
      {
        Common::AddonInformation opcTcpAsync = Server::CreateOpcTcpAsyncAddon();
        AddParameters(opcTcpAsync, group);
        addons.push_back(opcTcpAsync);
      }
      else if (group.Name == OpcUa::Server::AsioAddonID)
      {
        AddParameters(asioAddon, group);
      }
      else if (group.Name == OpcUa::Server::SubscriptionServiceAddonID)
      {
        AddParameters(subscriptionService, group);
      }
      else if (group.Name == OpcUa::Server::ServerObjectAddonID)
      {
        AddParameters(serverObject, group);
      }
    }

    addons.push_back(endpointsRegistry);
    addons.push_back(addressSpaceRegistry);
    addons.push_back(asioAddon);
    addons.push_back(subscriptionService);
    addons.push_back(Server::CreateServicesRegistryAddon());
    addons.push_back(Server::CreateStandardNamespaceAddon());
    addons.push_back(serverObject);
  }

  inline void RegisterAddons(std::vector<Common::AddonInformation> addons, Common::AddonsManager& manager)
  {
    std::for_each(std::begin(addons), std::end(addons), [&manager](const Common::AddonInformation& addonConfig){
      manager.Register(addonConfig);
    });
  }

  Common::AddonParameters CreateAddonsParameters(const OpcUa::Server::Parameters& serverParams)
  {
    Common::Parameter debugMode("debug", std::to_string(serverParams.Debug));

    Common::AddonParameters addons;

    Common::ParametersGroup async("async");
    async.Parameters.push_back(Common::Parameter("threads", std::to_string(serverParams.ThreadsCount)));
    async.Parameters.push_back(debugMode);
    addons.Groups.push_back(async);

    Common::ParametersGroup addressSpace(OpcUa::Server::AddressSpaceRegistryAddonID);
    addressSpace.Parameters.push_back(debugMode);
    addons.Groups.push_back(addressSpace);

    Common::ParametersGroup endpointServices(OpcUa::Server::EndpointsRegistryAddonID);
    endpointServices.Parameters.push_back(debugMode);
    addons.Groups.push_back(endpointServices);

    Common::ParametersGroup subscriptionServices(OpcUa::Server::SubscriptionServiceAddonID);
    subscriptionServices.Parameters.push_back(debugMode);
    addons.Groups.push_back(subscriptionServices);

    Common::ParametersGroup opc_tcp(OpcUa::Server::AsyncOpcTcpAddonID);
    opc_tcp.Parameters.push_back(debugMode);
    OpcUa::Server::ApplicationData applicationData;
    applicationData.Application = serverParams.Endpoint.ServerDescription;
    applicationData.Endpoints.push_back(serverParams.Endpoint);
    opc_tcp.Groups = OpcUa::CreateCommonParameters({applicationData}, serverParams.Debug);
    addons.Groups.push_back(opc_tcp);

    return addons;
  }

} // namespace

namespace OpcUa
{
  Common::AddonInformation Server::CreateServicesRegistryAddon()
  {
    Common::AddonInformation services;
    services.Factory = std::make_shared<OpcUa::Server::ServicesRegistryFactory>();
    services.ID = OpcUa::Server::ServicesRegistryAddonID;
    return services;
  }

  Common::AddonInformation Server::CreateAddressSpaceAddon()
  {
    Common::AddonInformation config;
    config.Factory = std::make_shared<OpcUa::Server::AddressSpaceAddonFactory>();
    config.ID = OpcUa::Server::AddressSpaceRegistryAddonID;
    config.Dependencies.push_back(OpcUa::Server::ServicesRegistryAddonID);
    return config;
  }

  Common::AddonInformation Server::CreateStandardNamespaceAddon()
  {
    Common::AddonInformation config;
    config.Factory = std::make_shared<OpcUa::Server::StandardNamespaceAddonFactory>();
    config.ID = OpcUa::Server::StandardNamespaceAddonID;
    config.Dependencies.push_back(OpcUa::Server::AddressSpaceRegistryAddonID);
    return config;
  }

  Common::AddonInformation Server::CreateEndpointsRegistryAddon()
  {
    Common::AddonInformation endpoints;
    endpoints.Factory = std::make_shared<OpcUa::Server::EndpointsRegistryAddonFactory>();
    endpoints.ID = OpcUa::Server::EndpointsRegistryAddonID;
    endpoints.Dependencies.push_back(OpcUa::Server::ServicesRegistryAddonID);
    return endpoints;
  }

  Common::AddonInformation Server::CreateBinaryServerAddon()
  {
    Common::AddonInformation opcTcp;
    opcTcp.Factory = std::make_shared<OpcUa::Server::OpcUaProtocolAddonFactory>();
    opcTcp.ID = OpcUa::Server::OpcUaProtocolAddonID;
    opcTcp.Dependencies.push_back(OpcUa::Server::EndpointsRegistryAddonID);
    opcTcp.Dependencies.push_back(OpcUa::Server::SubscriptionServiceAddonID);
    return opcTcp;
  }

  Common::AddonInformation Server::CreateOpcTcpAsyncAddon()
  {
    Common::AddonInformation opcTcp;
    opcTcp.Factory = std::make_shared<OpcUa::Server::AsyncOpcTcpAddonFactory>();
    opcTcp.ID = OpcUa::Server::AsyncOpcTcpAddonID;
    opcTcp.Dependencies.push_back(OpcUa::Server::AsioAddonID);
    opcTcp.Dependencies.push_back(OpcUa::Server::EndpointsRegistryAddonID);
    opcTcp.Dependencies.push_back(OpcUa::Server::SubscriptionServiceAddonID);
    return opcTcp;
  }

  Common::AddonInformation Server::CreateServerObjectAddon()
  {
    Common::AddonInformation serverObjectAddon;
    serverObjectAddon.Factory = std::make_shared<OpcUa::Server::ServerObjectFactory>();
    serverObjectAddon.ID = OpcUa::Server::ServerObjectAddonID;
    serverObjectAddon.Dependencies.push_back(OpcUa::Server::StandardNamespaceAddonID);
    serverObjectAddon.Dependencies.push_back(OpcUa::Server::ServicesRegistryAddonID);
    serverObjectAddon.Dependencies.push_back(OpcUa::Server::AsioAddonID);
    return serverObjectAddon;
  }

  Common::AddonInformation Server::CreateAsioAddon()
  {
    Common::AddonInformation asioAddon;
    asioAddon.Factory = std::make_shared<OpcUa::Server::AsioAddonFactory>();
    asioAddon.ID = OpcUa::Server::AsioAddonID;
    return asioAddon;
  }

  Common::AddonInformation Server::CreateSubscriptionServiceAddon()
  {
    Common::AddonInformation subscriptionAddon;
    subscriptionAddon.Factory = std::make_shared<OpcUa::Server::SubscriptionServiceAddonFactory>();
    subscriptionAddon.ID = OpcUa::Server::SubscriptionServiceAddonID;
    subscriptionAddon.Dependencies.push_back(OpcUa::Server::AsioAddonID);
    subscriptionAddon.Dependencies.push_back(OpcUa::Server::AddressSpaceRegistryAddonID);
    subscriptionAddon.Dependencies.push_back(OpcUa::Server::ServicesRegistryAddonID);
    return subscriptionAddon;
  }

  void Server::RegisterCommonAddons(const Parameters& serverParams, Common::AddonsManager& manager)
  {
    std::vector<Common::AddonInformation> addons;
    Common::AddonParameters addonParameters = CreateAddonsParameters(serverParams);
    CreateCommonAddonsConfiguration(addonParameters, addons);
    RegisterAddons(addons, manager);
  }

  void Server::LoadConfiguration(const std::string& configDirectoryPath, Common::AddonsManager& manager)
  {
    const Common::Configuration& configuration = Common::ParseConfigurationFiles(configDirectoryPath);
    std::vector<Common::AddonInformation> addons(configuration.Modules.size());
    // modules are dynamic addons.
    std::transform(configuration.Modules.begin(), configuration.Modules.end(), addons.begin(), [] (const Common::ModuleConfiguration& module){
    	return Common::GetAddonInfomation(module);
    });
    CreateCommonAddonsConfiguration(configuration.Parameters, addons);
	RegisterAddons(addons, manager);
  }

}
