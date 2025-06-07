#define UPNPCOMMAND_CONFLICTING_MAPPING 718

#include <memory.h>

#include <common.h>
#include <Server.h>

#if defined(_WIN32) || defined(_WIN64)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

	#ifndef __GNUC__ // rain
		#pragma comment(lib, "ws2_32.lib")
	#endif

	#include <windows.h>
#endif

#include <misc/UPNP.h>

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

	memset(&m_urls, 0, sizeof(UPNPUrls));
	memset(&m_data, 0, sizeof(IGDdatas));

	std::vector<std::pair<uint8_t, std::string>> logbatch;
	logbatch.emplace_back(0_ui8, ":: Discovering UPNP devices:");

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

	int r = UPNP_AddPortMapping(m_urls.controlURL, m_data.first.servicetype, port.data(), port.data(), address.data(), "Graal GServer", "TCP", 0, 0);
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
		log::printLine(log::server, ":: [UPnP] Forwarded port {} to {}.", port, address);
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
	log::printLine(log::server, ":: [UPnP] Removing forward on port {}.", port);
	m_portsForwarded.erase(port);
#endif
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
