#ifndef CUPNP_H
#define CUPNP_H

#include <memory.h>
#include "CString.h"
#include "miniupnpc/miniwget.h"
#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"

class TServer;

class CUPNP
{
	public:
		CUPNP(TServer* server)
		{
			this->server = server;
		}

		// Allows boost::thread to work.
		void operator()()
		{
			discover();
			add_port_forward(local_ip, port);
		}

		void initialize(const char* local_ip, const char* port)
		{
			this->local_ip << local_ip;
			this->port << port;
		}

		// Finds a valid UPNP device.
		void discover();

		// Adds a port forward.
		void add_port_forward(const CString& addr, const CString& port);

		// Removes a port forward.
		void remove_port_forward(const CString& port);

	private:
		TServer* server;
		CString local_ip;
		CString port;
		struct UPNPUrls urls;
		struct IGDdatas data;
};

#endif
