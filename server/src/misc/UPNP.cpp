#ifdef UPNP

#define UPNPCOMMAND_CONFLICTING_MAPPING 718

#include "Server.h"

#if defined(_WIN32) || defined(_WIN64)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

	#ifndef __GNUC__ // rain
		#pragma comment(lib, "ws2_32.lib")
	#endif

	#include <windows.h>
#endif
#include "misc/UPNP.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

void UPNP::discover()
{
	struct UPNPDev* device_list;
	struct UPNPDev* device;
	char* xmlDescription;
	int xmlDescriptionSize = 0, responseCode = 0;

	memset(&m_urls, 0, sizeof(UPNPUrls));
	memset(&m_data, 0, sizeof(IGDdatas));

	device_list = upnpDiscover(2000, 0, 0, 0, 0, 0, 0);
	if (device_list)
	{
		device = device_list;
		while (device)
		{
			// We are searching for our gateway device.  If we found it, break out.
			if (strstr(device->st, "InternetGatewayDevice"))
				break;
			device = device->pNext;
		}

		// If no valid device was found, default to the first device.
		if (!device)
			device = device_list;

		// log::printLine(log::server, ":: [UPnP] Device desc: {}, st: {}", device->descURL, device->st);

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
		log::printLine(log::server, "** [UPnP] No devices found.");
	}
}

void UPNP::addPortForward(const CString& addr, const CString& port)
{
	if (m_urls.controlURL == 0 || m_urls.controlURL[0] == '\0')
		return;

	int r = UPNP_AddPortMapping(m_urls.controlURL, m_data.first.servicetype, port.text(), port.text(), addr.text(), "Graal GServer", "TCP", 0, 0);
	if (r != 0)
	{
		log::print(log::server, "** [UPnP] Failed to forward port {} to {}: ", port, addr);
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
		log::printLine(log::server, ":: [UPnP] Forwarded port {} to {}.", port, addr);
		m_portsForwarded.insert(port);
	}
}

void UPNP::removePortForward(const CString& port)
{
	if (m_urls.controlURL == 0 || m_urls.controlURL[0] == '\0')
		return;

	UPNP_DeletePortMapping(m_urls.controlURL, m_data.first.servicetype, port.text(), "TCP", 0);
	log::printLine(log::server, ":: [UPnP] Removing forward on port {}.", port);
	m_portsForwarded.erase(port);
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // UPNP
