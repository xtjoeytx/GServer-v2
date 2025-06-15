#ifndef UPNP_H
#define UPNP_H

#include <functional>
#include <set>
#include <string_view>
#include <string>

#ifdef ENABLE_UPNP
#include <miniupnpc.h>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class UPNP
{
public:
	// Allows std::thread to work.
	void operator()()
	{
		discover();
		addPortForward(m_localIp, m_port);
	}

	void initialize(std::string_view localIp, std::string_view port)
	{
		m_localIp = localIp;
		m_port = port;
	}

	// Finds a valid UPNP device.
	void discover();

	// Adds a port forward.
	void addPortForward(std::string_view address, std::string_view port);

	// Removes a port forward.
	void removePortForward(std::string_view port);

	// Removes all the port forwards created by the addPortForward command.
	void removeAllForwardedPorts()
	{
		while (!m_portsForwarded.empty())
			removePortForward(*m_portsForwarded.rbegin());
		m_portsForwarded.clear();
	}

	// Returns true if the port was successfully forwarded.
	bool wasPortForwarded(std::string_view port)
	{
		return m_portsForwarded.find(port) != m_portsForwarded.end();
	}

private:
	std::set<std::string, std::less<>> m_portsForwarded;
	std::string m_localIp;
	std::string m_port;

#ifdef ENABLE_UPNP
	struct UPNPUrls m_urls;
	struct IGDdatas m_data;
#endif
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // UPNP_H
