#ifdef UPNP
	#define UPNPCOMMAND_CONFLICTING_MAPPING 718

	#if defined(_WIN32) || defined(_WIN64)
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN
		#endif

		#ifndef __GNUC__ // rain
			#pragma comment(lib, "ws2_32.lib")
		#endif

		#include <windows.h>
	#endif
	#include "UPNP.h"
	#include "Server.h"

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

		// m_server->getServerLog().out("[%s] :: [UPnP] Device desc: %s, st: %s\n", m_server->getName().text(), device->descURL, device->st);

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
		m_server->getServerLog().out("[%s] ** [UPnP] No devices found.\n", m_server->getName().text());
	}
}

void UPNP::addPortForward(const CString& addr, const CString& port)
{
	if (m_urls.controlURL == 0 || m_urls.controlURL[0] == '\0')
		return;

	CLog& serverlog = m_server->getServerLog();
	int r = UPNP_AddPortMapping(m_urls.controlURL, m_data.first.servicetype, port.text(), port.text(), addr.text(), "Graal GServer", "TCP", 0, 0);
	if (r != 0)
	{
		serverlog.out("[%s] ** [UPnP] Failed to forward port %s to %s: ", m_server->getName().text(), port.text(), addr.text());
		switch (r)
		{
			case UPNPCOMMAND_INVALID_ARGS:
				serverlog.out("Invalid arguments.\n");
				break;
			case UPNPCOMMAND_HTTP_ERROR:
				serverlog.out("HTTP error.\n");
				break;
			case UPNPCOMMAND_CONFLICTING_MAPPING:
				serverlog.out("Port mapping already exists.\n");
				break;
			default:
			case UPNPCOMMAND_UNKNOWN_ERROR:
				serverlog.out("Unknown error.\n");
				break;
		}
	}
	else
	{
		m_server->getServerLog().out("[%s] :: [UPnP] Forwarded port %s to %s.\n", m_server->getName().text(), port.text(), addr.text());
		m_portsForwarded.insert(port);
	}
}

void UPNP::removePortForward(const CString& port)
{
	if (m_urls.controlURL == 0 || m_urls.controlURL[0] == '\0')
		return;

	UPNP_DeletePortMapping(m_urls.controlURL, m_data.first.servicetype, port.text(), "TCP", 0);
	m_server->getServerLog().out("[%s] :: [UPnP] Removing forward on port %s.\n", m_server->getName().text(), port.text());
	m_portsForwarded.erase(port);
}
#endif
