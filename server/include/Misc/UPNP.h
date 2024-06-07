#ifndef CUPNP_H
#define CUPNP_H

#ifdef UPNP

	#include "CString.h"
	#include "miniupnpc.h"
	#include "miniwget.h"
	#include "upnpcommands.h"
	#include <memory.h>
	#include <set>

class Server;

class UPNP
{
public:
	UPNP(Server* server)
	{
		m_server = server;
	}

	// Allows std::thread to work.
	void operator()()
	{
		discover();
		add_port_forward(m_localIp, port);
	}

	void initialize(const char* m_localIp, const char* port)
	{
		m_localIp = m_localIp;
		m_port = port;
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
		while (!m_portsForwarded.empty())
			remove_port_forward(*m_portsForwarded.rbegin());
		m_portsForwarded.clear();
	}

	// Returns true if the port was successfully forwarded.
	bool port_was_forwarded(const CString& port)
	{
		return m_portsForwarded.find(port) != m_portsForwarded.end();
	}

private:
	Server* m_server;
	std::set<CString> m_portsForwarded;
	CString m_localIp;
	CString m_port;
	struct UPNPUrls m_urls;
	struct IGDdatas m_data;
};

#endif
#endif
