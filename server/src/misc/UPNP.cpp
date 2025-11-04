#define UPNPCOMMAND_CONFLICTING_MAPPING 718

#include <cstdint>
#include <cstring>
#include <format>
#include <malloc.h>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <misc/UPNP.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>

#ifdef ENABLE_UPNP
#include <miniupnpc.h>
#include <miniwget.h>
#include <upnpcommands.h>
#if DEBUG
#include <portlistingparse.h>
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

void UPNP::discover()
{
#ifdef ENABLE_UPNP
	struct UPNPDev* device_list;
	struct UPNPDev* device;
	char* xmlDescription;
	int xmlDescriptionSize = 0, responseCode = 0;

	std::memset(&m_urls, 0, sizeof(UPNPUrls));
	std::memset(&m_data, 0, sizeof(IGDdatas));

	std::vector<std::pair<uint8_t, std::string>> logbatch;
	logbatch.emplace_back(0_ui8, "Discovering UPNP devices:");

	device_list = upnpDiscover(2000, 0, 0, UPNP_LOCAL_PORT_ANY, 0, 2, 0);
	if (device_list)
	{
		std::vector<UPNPDev*> gatewayDevices;

		device = device_list;
		while (device)
		{
			std::string_view device_view{ device->st };
			logbatch.emplace_back(1_ui8, std::format("{}{}", (device == device_list ? "* " : ""), device_view));

			// We are searching for our gateway device.  If we found it, break out.
			if (device_view.contains("InternetGatewayDevice"))
				gatewayDevices.push_back(device);

			device = device->pNext;
		}

		// Get the first device in the list.
		device = gatewayDevices.front();

		// Get the XML description of the UPNP device.
		xmlDescription = (char*)miniwget(device->descURL, &xmlDescriptionSize, 0, &responseCode);
		if (xmlDescription)
		{
			// Parse the XML description.
			parserootdesc(xmlDescription, xmlDescriptionSize, &m_data);
			free(xmlDescription);
			xmlDescription = 0;

			// Get the UPNP urls from the description.
			GetUPNPUrls(&m_urls, &m_data, device->descURL, 0);
		}
		freeUPNPDevlist(device_list);
	}
	else
	{
		logbatch.emplace_back(1_ui8, "** [UPnP] No devices found.");
	}

	log::batch(log::server, logbatch);
#endif
}

void UPNP::addPortForward(std::string_view address, std::string_view port)
{
#ifdef ENABLE_UPNP
	if (m_urls.controlURL == 0 || m_urls.controlURL[0] == '\0')
		return;

	int r = UPNP_AddPortMapping(m_urls.controlURL, m_data.first.servicetype, port.data(), port.data(), address.data(), "Graal GServer", "TCP", "", 0);
	if (r == UPNPCOMMAND_CONFLICTING_MAPPING)
	{
		// Check if this port map was likely created by us and was left behind.
		char intClient[16] = { 0 };
		char intPort[6] = { 0 };
		char desc[80] = { 0 };
		int r2 = UPNP_GetSpecificPortMappingEntry(m_urls.controlURL, m_data.first.servicetype, port.data(), "TCP", "", intClient, intPort, desc, 0, 0);
		if (r2 == 0 && std::string_view{ desc }.find("Graal GServer") != std::string_view::npos)
		{
			log::printLine(log::server, "[UPnP] Found existing port mapping on port {} likely created by us.", port);
			m_portsForwarded.emplace(port);
			return;
		}
#if DEBUG
		else
		{
			unsigned int entries;
			if (UPNP_GetPortMappingNumberOfEntries(m_urls.controlURL, m_data.first.servicetype, &entries) == UPNPCOMMAND_SUCCESS)
			{
				char index[6] = { 0 };
				for (auto i = 0; i < entries; ++i)
				{
					std::snprintf(index, 6, "%u", i);
					UPNP_GetGenericPortMappingEntry(m_urls.controlURL, m_data.first.servicetype, index, 0, intClient, intPort, 0, desc, 0, 0, 0);
					log::printLine(log::server, "[UPnP] Existing mapping #{}: {} -> {} ({})", i, intClient, intPort, desc);
				}
			}
			else
			{
				PortMappingParserData parserData;
				memset(&parserData, 0, sizeof(PortMappingParserData));
				if (UPNP_GetListOfPortMappings(m_urls.controlURL, m_data.first.servicetype, "1", "65535", "TCP", 0, &parserData) == UPNPCOMMAND_SUCCESS)
				{
					int i = 0;
					for (auto pm = parserData.l_head; pm != nullptr; pm = pm->l_next)
					{
						log::printLine(log::server, "[UPnP] {}: {} {}", i, pm->description, pm->externalPort);
						i++;
					}
					FreePortListing(&parserData);
				}
			}
		}
#endif
	}
	if (r != 0)
	{
		log::print(log::server, "** [UPnP] Failed to forward port {} to {}: ", port, address);
		switch (r)
		{
			case UPNPCOMMAND_INVALID_ARGS:
				log::printLine(log::server, "Invalid arguments.");
				break;
			case UPNPCOMMAND_HTTP_ERROR:
				log::printLine(log::server, "HTTP error.");
				break;
			case UPNPCOMMAND_CONFLICTING_MAPPING:
				log::printLine(log::server, "Port mapping already exists.");
				break;
			default:
			case UPNPCOMMAND_UNKNOWN_ERROR:
				log::printLine(log::server, "Unknown error.");
				break;
		}
	}
	else
	{
		log::printLine(log::server, "[UPnP] Forwarded port {} to {}.", port, address);
		m_portsForwarded.emplace(port);
	}
#endif
}

void UPNP::removePortForward(std::string_view port)
{
#ifdef ENABLE_UPNP
	if (m_urls.controlURL == 0 || m_urls.controlURL[0] == '\0')
		return;

	UPNP_DeletePortMapping(m_urls.controlURL, m_data.first.servicetype, port.data(), "TCP", 0);
	log::printLine(log::server, "[UPnP] Removing forward on port {}.", port);
	m_portsForwarded.erase(std::string{ port });
#endif
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
