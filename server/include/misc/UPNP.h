#ifndef CUPNP_H
#define CUPNP_H

#ifdef UPNP

	#include <memory.h>
	#include <set>

	#include <CString.h>

	#include <miniupnpc.h>
	#include <miniwget.h>
	#include <upnpcommands.h>

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
		addPortForward(m_localIp, port);
	}

	void initialize(const char* m_localIp, const char* port)
	{
		m_localIp = m_localIp;
		m_port = port;
	}

	// Finds a valid UPNP device.
	void discover();

	// Adds a port forward.
	void addPortForward(const CString& addr, const CString& port);

	// Removes a port forward.
	void removePortForward(const CString& port);

	// Removes all the port forwards created by the addPortForward command.
	void removeAllForwardedPorts()
	{
		while (!m_portsForwarded.empty())
			removePortForward(*m_portsForwarded.rbegin());
		m_portsForwarded.clear();
	}

	// Returns true if the port was successfully forwarded.
	bool wasPortForwarded(const CString& port)
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
