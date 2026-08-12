#ifndef UPNP_H
#define UPNP_H

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
	~UPNP();

	// Allows std::thread to work.
	void operator()()
	{
		if (m_localIp.empty())
			return;

		discover();
		addPortForward(m_localIp, m_port);
	}

	// Initializes everything.
	bool initialize(std::string_view localIp, std::string_view port);

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
	[[nodiscard]] bool wasPortForwarded(const std::string_view port) const
	{
		return m_portsForwarded.contains(port);
	}

private:
	std::set<std::string, std::less<>> m_portsForwarded;
	std::string m_localIp;
	std::string m_port;

#ifdef ENABLE_UPNP
	struct UPNPUrls m_urls{};
	struct IGDdatas m_data{};
#endif
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // UPNP_H
