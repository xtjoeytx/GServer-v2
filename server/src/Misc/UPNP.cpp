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

void CUPNP::discover()
{
	struct UPNPDev* device_list;
	struct UPNPDev* device;
	char* xmlDescription;
	int xmlDescriptionSize = 0, responseCode = 0;

	memset(&urls, 0, sizeof(UPNPUrls));
	memset(&data, 0, sizeof(IGDdatas));

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
		
		//server->getServerLog().out("[%s] :: [UPnP] Device desc: %s, st: %s\n", server->getName().text(), device->descURL, device->st);

		// Get the XML description of the UPNP device.
		xmlDescription = (char*)miniwget(device->descURL, &xmlDescriptionSize, 0, &responseCode);
		if (xmlDescription)
		{
			// Parse the XML description.
			parserootdesc(xmlDescription, xmlDescriptionSize, &data);
			free(xmlDescription);
			xmlDescription = 0;

			// Get the UPNP urls from the description.
			GetUPNPUrls(&urls, &data, device->descURL, 0);
		}
		freeUPNPDevlist(device_list);
	}
	else
	{
		server->getServerLog().out("[%s] ** [UPnP] No devices found.\n", server->getName().text());
	}
}

void CUPNP::add_port_forward(const CString& addr, const CString& port)
{
	if (urls.controlURL == 0 || urls.controlURL[0] == '\0')
		return;

	CLog& serverlog = server->getServerLog();
	int r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, port.text(), port.text(), addr.text(), "Graal GServer", "TCP", 0, 0);
	if (r != 0)
	{
		serverlog.out("[%s] ** [UPnP] Failed to forward port %s to %s: ", server->getName().text(), port.text(), addr.text());
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
		server->getServerLog().out("[%s] :: [UPnP] Forwarded port %s to %s.\n", server->getName().text(), port.text(), addr.text());
		ports_forwarded.insert(port);
	}
}

void CUPNP::remove_port_forward(const CString& port)
{
	if (urls.controlURL == 0 || urls.controlURL[0] == '\0')
		return;

	UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port.text(), "TCP", 0);
	server->getServerLog().out("[%s] :: [UPnP] Removing forward on port %s.\n", server->getName().text(), port.text());
	ports_forwarded.erase(port);
}
#endif
