#ifndef SCRIPT_H
#define SCRIPT_H

#include <cstdint>
#include <memory>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>

using namespace std::literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

using ScriptByteCode = std::vector<uint8_t>;
using ScriptByteCodePtr = std::shared_ptr<ScriptByteCode>;

class Script
{
public:
	Script() = default;
	Script(const Script& o) noexcept : Script(o.m_original_source) {}
	Script(Script&& o) noexcept : Script(std::move(o.m_original_source)) {}
	Script(const std::string& src) noexcept : Script(std::move(std::string{ src })) {}
	Script(std::string_view src) noexcept : Script(std::move(std::string{ src })) {}

	Script(std::string&& src) noexcept
		: m_original_source(std::move(src))
	{
		setModifiedSource(m_original_source);
	}

	[[inline]] Script& operator=(const Script& o) noexcept;
	[[inline]] Script& operator=(Script&& o) noexcept;

public:
	[[inline]] const std::string& getOriginalSource() const noexcept;
	[[inline]] const std::string& getModifiedSource() const noexcept;
	[[inline]] std::string_view getClientSide() const noexcept;
	[[inline]] std::string_view getServerSide() const noexcept;
	const ScriptByteCode& getClientByteCode() const noexcept;

public:
	[[inline]] void setOriginalSource(const std::string& source) noexcept;
	[[inline]] void setModifiedSource(const std::string& source) noexcept;
	[[inline]] void setClientCompiledScript(CompiledScriptResultPtr script) noexcept;
	[[inline]] void setServerCompiledScript(CompiledScriptResultPtr script) noexcept;

public:
	void executeEvents(ScriptContainer& container, ScriptObjectSource source) const;
	void executeEvents(ScriptEventQueue& events, ScriptObjectSource source) const;

private:
	static std::string minify(const std::string& src) noexcept;

private:
	std::string m_original_source;
	std::string m_modified_source;
	std::string_view m_clientside;
	std::string_view m_serverside;
	CompiledScriptResultPtr m_client_script;
	CompiledScriptResultPtr m_server_script;

	void split(std::string& source) noexcept;
};

inline Script& Script::operator=(const Script& o) noexcept
{
	m_original_source = o.m_original_source;
	setModifiedSource(m_original_source);
	m_client_script = o.m_client_script;
	m_server_script = o.m_server_script;
	return *this;
}

inline Script& Script::operator=(Script&& o) noexcept
{
	m_original_source = std::move(o.m_original_source);
	m_modified_source = std::move(o.m_modified_source);
	m_clientside = std::move(o.m_clientside);
	m_serverside = std::move(o.m_serverside);
	m_client_script = std::move(o.m_client_script);
	m_server_script = std::move(o.m_server_script);
	return *this;
}

inline const std::string& Script::getOriginalSource() const noexcept
{
	return m_original_source;
}

inline const std::string& Script::getModifiedSource() const noexcept
{
	return m_modified_source;
}

inline std::string_view Script::getClientSide() const noexcept
{
	return m_clientside;
}

inline std::string_view Script::getServerSide() const noexcept
{
	return m_serverside;
}

inline void Script::setOriginalSource(const std::string& source) noexcept
{
	m_original_source = source;
	setModifiedSource(m_original_source);
}

inline void Script::setModifiedSource(const std::string& source) noexcept
{
	m_modified_source = std::move(minify(source));
	split(m_modified_source);
}

inline void Script::setClientCompiledScript(CompiledScriptResultPtr script) noexcept
{
	m_client_script = script;
}

inline void Script::setServerCompiledScript(CompiledScriptResultPtr script) noexcept
{
	m_server_script = script;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPT_H
