#ifndef SOURCECODE_H
#define SOURCECODE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <set>
#include <vector>
#include <utility>

#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std::literals;

namespace preagonal
{

using ScriptByteCode = std::vector<uint8_t>;
using ScriptByteCodePtr = std::shared_ptr<ScriptByteCode>;

///////////////////////////////////////////////////////////////////////////////

class SourceCode
{
public:
	SourceCode() = default;
	SourceCode(const SourceCode& o) noexcept : SourceCode(o.m_original_source) {}
	SourceCode(SourceCode&& o) noexcept : SourceCode(std::move(o.m_original_source)) {}
	SourceCode(const std::string& src) noexcept : SourceCode(std::move(std::string{ src })) {}
	SourceCode(std::string_view src) noexcept : SourceCode(std::move(std::string{ src })) {}

	SourceCode(std::string&& src) noexcept
		: m_original_source(std::move(src))
	{
		setModifiedSource(m_original_source);
	}

	[[inline]] SourceCode& operator=(const SourceCode& o) noexcept;
	[[inline]] SourceCode& operator=(SourceCode&& o) noexcept;

public:
	[[inline]] const std::string& getOriginalSource() const noexcept;
	[[inline]] const std::string& getModifiedSource() const noexcept;
	[[inline]] std::string_view getClientSide() const noexcept;
	[[inline]] std::string_view getServerSide() const noexcept;
	const ScriptByteCode& getClientByteCode() const noexcept;

public:
	[[inline]] void setModifiedSource(const std::string& source) noexcept;
	[[inline]] void setClientCompiledScript(CompiledScriptResultPtr script) noexcept;
	[[inline]] void setServerCompiledScript(CompiledScriptResultPtr script) noexcept;

public:
	void executeEvents(ScriptContainer& container, ScriptEventSource source) const;
	void executeEvents(ScriptEventQueue& events, ScriptEventSource source) const;

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

inline SourceCode& SourceCode::operator=(const SourceCode& o) noexcept
{
	m_original_source = o.m_original_source;
	setModifiedSource(m_original_source);
	m_client_script = o.m_client_script;
	m_server_script = o.m_server_script;
	return *this;
}

inline SourceCode& SourceCode::operator=(SourceCode&& o) noexcept
{
	m_original_source = std::move(o.m_original_source);
	m_modified_source = std::move(o.m_modified_source);
	m_clientside = std::move(o.m_clientside);
	m_serverside = std::move(o.m_serverside);
	m_client_script = std::move(o.m_client_script);
	m_server_script = std::move(o.m_server_script);
	return *this;
}

inline const std::string& SourceCode::getOriginalSource() const noexcept
{
	return m_original_source;
}

inline const std::string& SourceCode::getModifiedSource() const noexcept
{
	return m_modified_source;
}

inline std::string_view SourceCode::getClientSide() const noexcept
{
	return m_clientside;
}

inline std::string_view SourceCode::getServerSide() const noexcept
{
	return m_serverside;
}

inline void SourceCode::setModifiedSource(const std::string& source) noexcept
{
	m_modified_source = std::move(minify(source));
	split(m_modified_source);
}

inline void SourceCode::setClientCompiledScript(CompiledScriptResultPtr script) noexcept
{
	m_client_script = script;
}

inline void SourceCode::setServerCompiledScript(CompiledScriptResultPtr script) noexcept
{
	m_server_script = script;
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SOURCECODE_H
