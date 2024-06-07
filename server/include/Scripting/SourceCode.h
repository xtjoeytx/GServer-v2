#ifndef SOURCECODE_H
#define SOURCECODE_H

#include <string>
#include <string_view>

class SourceCode
{
public:
	SourceCode() : m_gs2default(false) {}
	SourceCode(std::string src, bool gs2default = false) : m_src(std::move(src)), m_gs2default(gs2default) { init(); }
	SourceCode(SourceCode&& o) noexcept : m_src(std::move(o.m_src)), m_gs2default(o.m_gs2default) { init(); }

	SourceCode& operator=(SourceCode&& o) noexcept
	{
		m_gs2default = o.m_gs2default;
		m_src = std::move(o.m_src);
		init();
		return *this;
	}

	explicit operator bool() const
	{
		return !empty();
	}

	bool empty() const
	{
		return m_src.empty();
	}

	const std::string& getSource() const
	{
		return m_src;
	}

	std::string_view getServerSide() const
	{
		return m_serverside;
	}

	std::string_view getClientSide() const
	{
		return m_clientside;
	}

	std::string_view getClientGS1() const
	{
		return m_clientGS1;
	}

	std::string_view getClientGS2() const
	{
		return m_clientGS2;
	}

	void clearServerSide()
	{
		m_serverside = {};
	}

private:
	bool m_gs2default;
	std::string m_src;
	std::string_view m_clientside, m_serverside;
	std::string_view m_clientGS1, m_clientGS2;

	void init() noexcept
	{
		m_clientside = m_serverside = m_clientGS1 = m_clientGS2 = {};

#ifdef V8NPCSERVER
		auto clientSep = m_src.find("//#CLIENTSIDE");
		if (clientSep != std::string::npos)
		{
			// Separate clientside and serverside
			m_clientside = std::string_view{ m_src }.substr(clientSep);
			m_serverside = std::string_view{ m_src }.substr(0, clientSep);
		}
		else
			m_serverside = std::string_view{ m_src };
#else
		// For non-npcserver builds all code is considered clientside
		m_clientside = std::string_view{ m_src };
#endif

		if (!m_clientside.empty())
		{
			// Switch separator depending on if GS2 is set to default or not
			const char* gs2sep_char;
			if (m_gs2default)
				gs2sep_char = "//#GS1";
			else
				gs2sep_char = "//#GS2";

			// Determine if this code is GS1 or GS2
			size_t codeSeparatorLoc = m_clientside.find(gs2sep_char);
			if (codeSeparatorLoc != std::string::npos)
			{
				auto origCode = m_clientside.substr(0, codeSeparatorLoc);
				auto otherCode = m_clientside.substr(codeSeparatorLoc);
				m_clientGS2 = m_gs2default ? origCode : otherCode;
				m_clientGS1 = m_gs2default ? otherCode : origCode;
			}
			else
			{
				if (m_gs2default)
					m_clientGS2 = m_clientside;
				else
					m_clientGS1 = m_clientside;
			}
		}
	}
};

#endif
