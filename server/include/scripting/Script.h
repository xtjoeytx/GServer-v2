#ifndef SCRIPT_H
#define SCRIPT_H

#include <cstdint>
#include <memory>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/std/generator.h>
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
	Script(const std::string_view who, const std::string& src) noexcept : Script(who, std::string{ src }) {}
	Script(const std::string_view who, const std::string_view src) noexcept : Script(who, std::string{ src }) {}
	Script(const Script& o) noexcept { *this = o; }
	Script(Script&& o) noexcept { *this = std::move(o); }

	Script(const std::string_view who, std::string&& src) noexcept
	{
		setOriginalSource(who, std::move(src));
	}

	[[a::inline]] Script& operator=(const Script& o) noexcept;
	[[a::inline]] Script& operator=(Script&& o) noexcept;

public:
	[[a::inline]] [[nodiscard]] size_t getHash() const noexcept;
	[[a::inline]] [[nodiscard]] const std::string& getOriginalSource() const noexcept;
	[[a::inline]] [[nodiscard]] const std::string& getModifiedSource() const noexcept;
	[[a::inline]] [[nodiscard]] std::string_view getClientSide() const noexcept;
	[[a::inline]] [[nodiscard]] std::string_view getServerSide() const noexcept;
	[[nodiscard]] const ScriptByteCode& getClientByteCode() const noexcept;

public:
	[[a::inline]] Script& setOriginalSource(std::string_view who, std::string&& source) noexcept;
	[[a::inline]] Script& setOriginalSource(std::string_view who, const std::string& source) noexcept;
	[[a::inline]] Script& setModifiedSource(const std::string& source) noexcept;
	[[a::inline]] Script& setClientCompiledScript(CompiledScriptResultPtr script) noexcept;
	[[a::inline]] Script& setServerCompiledScript(CompiledScriptResultPtr script) noexcept;

public:
	[[nodiscard]] std::generator<decltype(ScriptExecutionContext::joinedClasses)::const_reference> getServerJoinedClasses() const noexcept;

public:
	void executeEvents(ScriptContainer& container, const ScriptObject& source) const;
	void executeEvents(ScriptEventQueue& events, const ScriptObject& source) const;
	void executeEvents(clear_container_t, ScriptContainer& container, const ScriptObject& source) const;
	void executeEvents(clear_container_t, ScriptEventQueue& events, const ScriptObject& source) const;
	bool runUserDefinedFunction(std::string_view functionName, ScriptEvent& event, const ScriptObject& source) const;

public:
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
	void compileScript() noexcept;
};

//----------------------------

inline size_t Script::getHash() const noexcept
{
	return m_hash;
}

inline Script& Script::operator=(const Script& o) noexcept
{
	if (this == &o)
		return *this;

	m_who = o.m_who;
	m_original_source = o.m_original_source;
	m_modified_source = o.m_modified_source;
	split(m_modified_source);
	m_client_script = o.m_client_script;
	m_server_script = o.m_server_script;
	m_hash = o.m_hash;
	return *this;
}

inline Script& Script::operator=(Script&& o) noexcept
{
	if (this == &o)
		return *this;

	m_who = std::move(o.m_who);
	m_original_source = std::move(o.m_original_source);
	m_modified_source = std::move(o.m_modified_source);
	m_clientside = o.m_clientside;
	m_serverside = o.m_serverside;
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

inline Script& Script::setOriginalSource(const std::string_view who, std::string&& source) noexcept
{
	m_who = who;
	m_original_source = std::move(source);
	m_hash = string::string_hash{}(m_original_source);
	return setModifiedSource(m_original_source);
}

inline Script& Script::setOriginalSource(const std::string_view who, const std::string& source) noexcept
{
	m_who = who;
	m_original_source = source;
	m_hash = string::string_hash{}(m_original_source);
	return setModifiedSource(m_original_source);
}

inline Script& Script::setModifiedSource(const std::string& source) noexcept
{
	m_modified_source = minify(source);
	split(m_modified_source);
	compileScript();
	return *this;
}

inline Script& Script::setClientCompiledScript(CompiledScriptResultPtr script) noexcept
{
	m_client_script = std::move(script);
	return *this;
}

inline Script& Script::setServerCompiledScript(CompiledScriptResultPtr script) noexcept
{
	m_server_script = std::move(script);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPT_H
