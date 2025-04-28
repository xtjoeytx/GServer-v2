#ifndef SOURCECODE_H
#define SOURCECODE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <set>
#include <vector>
#include <utility>

#include "utilities/StringUtils.h"

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
	[[inline]] ScriptByteCodePtr getClientByteCode() const noexcept;
	[[inline]] ScriptByteCodePtr getServerByteCode() const noexcept;

public:
	[[inline]] void setModifiedSource(const std::string& source) noexcept;
	[[inline]] void setClientByteCode(ScriptByteCodePtr bytecode) noexcept;
	[[inline]] void setServerByteCode(ScriptByteCodePtr bytecode) noexcept;

public:
	[[inline]] void addClientJoinedClasses(const std::set<std::string>& classes) noexcept;
	[[inline]] void addServerJoinedClasses(const std::set<std::string>& classes) noexcept;
	[[inline]] const std::set<std::string>& getClientJoinedClasses() const noexcept;
	[[inline]] const std::set<std::string>& getServerJoinedClasses() const noexcept;

public:
	static std::string minify(const std::string& src) noexcept;

private:
	std::string m_original_source;
	std::string m_modified_source;
	std::string_view m_clientside;
	std::string_view m_serverside;
	ScriptByteCodePtr m_client_bytecode;
	ScriptByteCodePtr m_server_bytecode;
	std::set<std::string> m_client_joined_classes;
	std::set<std::string> m_server_joined_classes;

	void split(std::string& source) noexcept;
};

inline SourceCode& SourceCode::operator=(const SourceCode& o) noexcept
{
	m_original_source = o.m_original_source;
	setModifiedSource(m_original_source);
	m_client_bytecode = o.m_client_bytecode;
	m_server_bytecode = o.m_server_bytecode;
	m_client_joined_classes = o.m_client_joined_classes;
	m_server_joined_classes = o.m_server_joined_classes;
	return *this;
}

inline SourceCode& SourceCode::operator=(SourceCode&& o) noexcept
{
	m_original_source = std::move(o.m_original_source);
	m_modified_source = std::move(o.m_modified_source);
	m_clientside = std::move(o.m_clientside);
	m_serverside = std::move(o.m_serverside);
	m_client_bytecode = std::move(o.m_client_bytecode);
	m_server_bytecode = std::move(o.m_server_bytecode);
	m_client_joined_classes = std::move(o.m_client_joined_classes);
	m_server_joined_classes = std::move(o.m_server_joined_classes);
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

inline ScriptByteCodePtr SourceCode::getClientByteCode() const noexcept
{
	return m_client_bytecode;
}

inline ScriptByteCodePtr SourceCode::getServerByteCode() const noexcept
{
	return m_server_bytecode;
}

inline void SourceCode::setModifiedSource(const std::string& source) noexcept
{
	m_modified_source = std::move(minify(source));
	std::replace(m_modified_source.begin(), m_modified_source.end(), '\n', '\xa7');
	split(m_modified_source);
}

inline void SourceCode::setClientByteCode(ScriptByteCodePtr bytecode) noexcept
{
	m_client_bytecode = bytecode;
}

inline void SourceCode::setServerByteCode(ScriptByteCodePtr bytecode) noexcept
{
	m_server_bytecode = bytecode;
}

inline void SourceCode::addClientJoinedClasses(const std::set<std::string>& classes) noexcept
{
	m_client_joined_classes.insert(classes.begin(), classes.end());
}

inline void SourceCode::addServerJoinedClasses(const std::set<std::string>& classes) noexcept
{
	m_server_joined_classes.insert(classes.begin(), classes.end());
}

inline const std::set<std::string>& SourceCode::getClientJoinedClasses() const noexcept
{
	return m_client_joined_classes;
}

inline const std::set<std::string>& SourceCode::getServerJoinedClasses() const noexcept
{
	return m_server_joined_classes;
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SOURCECODE_H
