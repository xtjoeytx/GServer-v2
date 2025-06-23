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
#include <utilities/StringUtils.h>

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
	Script(std::string_view who, const std::string& src) noexcept : Script(who, std::move(std::string{ src })) {}
	Script(std::string_view who, std::string_view src) noexcept : Script(who, std::move(std::string{ src })) {}
	Script(const Script& o) noexcept { *this = o; }
	Script(Script&& o) noexcept { *this = std::move(o); }

	Script(std::string_view who, std::string&& src) noexcept
	{
		setOriginalSource(who, std::move(src));
	}

	[[inline]] Script& operator=(const Script& o) noexcept;
	[[inline]] Script& operator=(Script&& o) noexcept;

public:
	[[inline]] const size_t getHash() const noexcept;
	[[inline]] const std::string& getOriginalSource() const noexcept;
	[[inline]] const std::string& getModifiedSource() const noexcept;
	[[inline]] std::string_view getClientSide() const noexcept;
	[[inline]] std::string_view getServerSide() const noexcept;
	const ScriptByteCode& getClientByteCode() const noexcept;

public:
	[[inline]] Script& setOriginalSource(std::string_view who, std::string&& source) noexcept;
	[[inline]] Script& setOriginalSource(std::string_view who, const std::string& source) noexcept;
	[[inline]] Script& setModifiedSource(const std::string& source) noexcept;
	[[inline]] Script& setClientCompiledScript(CompiledScriptResultPtr script) noexcept;
	[[inline]] Script& setServerCompiledScript(CompiledScriptResultPtr script) noexcept;

public:
	void executeEvents(ScriptContainer& container, ScriptObjectSource source) const;
	void executeEvents(ScriptEventQueue& events, ScriptObjectSource source) const;

private:
	static std::string minify(const std::string& src) noexcept;

private:
	std::string m_who;
	std::string m_original_source;
	std::string m_modified_source;
	std::string_view m_clientside;
	std::string_view m_serverside;
	CompiledScriptResultPtr m_client_script;
	CompiledScriptResultPtr m_server_script;
	size_t m_hash = 0;

	void split(std::string& source) noexcept;
};

//----------------------------

inline const size_t Script::getHash() const noexcept
{
	return m_hash;
}

inline Script& Script::operator=(const Script& o) noexcept
{
	m_who = o.m_who;
	m_original_source = o.m_original_source;
	setModifiedSource(m_original_source);
	m_client_script = o.m_client_script;
	m_server_script = o.m_server_script;
	m_hash = o.m_hash;
	return *this;
}

inline Script& Script::operator=(Script&& o) noexcept
{
	m_who = std::move(o.m_who);
	m_original_source = std::move(o.m_original_source);
	m_modified_source = std::move(o.m_modified_source);
	m_clientside = std::move(o.m_clientside);
	m_serverside = std::move(o.m_serverside);
	m_client_script = std::move(o.m_client_script);
	m_server_script = std::move(o.m_server_script);
	m_hash = o.m_hash;
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

//----------------------------

inline Script& Script::setOriginalSource(std::string_view who, std::string&& source) noexcept
{
	m_who = who;
	m_original_source = std::move(source);
	m_hash = string::string_hash{}(m_original_source);
	return setModifiedSource(m_original_source);
}

inline Script& Script::setOriginalSource(std::string_view who, const std::string& source) noexcept
{
	m_who = who;
	m_original_source = source;
	m_hash = string::string_hash{}(m_original_source);
	return setModifiedSource(m_original_source);
}

inline Script& Script::setModifiedSource(const std::string& source) noexcept
{
	m_modified_source = std::move(minify(source));
	split(m_modified_source);
	return *this;
}

inline Script& Script::setClientCompiledScript(CompiledScriptResultPtr script) noexcept
{
	m_client_script = script;
	return *this;
}

inline Script& Script::setServerCompiledScript(CompiledScriptResultPtr script) noexcept
{
	m_server_script = script;
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPT_H
