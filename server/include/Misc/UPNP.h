#ifndef CUPNP_H
#define CUPNP_H

#ifdef UPNP

	#include <set>
	#include "CString.h"
	#include "miniupnpc.h"
	#include "miniwget.h"
	#include "upnpcommands.h"
	#include <memory.h>

class TServer;

class CUPNP
{
public:
	CUPNP(TServer* server)
	{
		this->server = server;
	}

	// Allows std::thread to work.
	void operator()()
	{
		discover();
		add_port_forward(local_ip, port);
	}

	void initialize(const char* local_ip, const char* port)
	{
		this->local_ip = local_ip;
		this->port = port;
	}

	// Finds a valid UPNP device.
	void discover();

	// Adds a port forward.
	void add_port_forward(const CString& addr, const CString& port);

	// Removes a port forward.
	void remove_port_forward(const CString& port);

	// Removes all the port forwards created by the add_port_forward command.
	void remove_all_forwarded_ports()
	{
		while (!ports_forwarded.empty())
			remove_port_forward(*ports_forwarded.rbegin());
		ports_forwarded.clear();
	}

	// Returns true if the port was successfully forwarded.
	bool port_was_forwarded(const CString& port)
	{
		return ports_forwarded.find(port) != ports_forwarded.end();
	}

private:
	TServer* server;
	std::set<CString> ports_forwarded;
	CString local_ip;
	CString port;
	struct UPNPUrls urls;
	struct IGDdatas data;
};

#endif
#endif
