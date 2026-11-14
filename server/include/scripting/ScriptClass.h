#ifndef SCRIPTCLASS_H
#define SCRIPTCLASS_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <scripting/Script.h>
#include <utilities/CommonTypes.h>
#include <utilities/Events.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Server;
class Player;

class ScriptClass
{
public:
	ScriptClass(std::string_view className, std::string_view classScript);
	~ScriptClass() = default;

	void registerWithPlayer(const std::shared_ptr<Player>& player) const;
	void sendByteCodeToPlayer(const std::shared_ptr<Player>& player) const;

	[[a::inline]] bool hasChecksum(uint32_t checksum) const noexcept;

	[[a::inline]] Script& getScript();
	[[a::inline]] const Script& getScript() const;
	ScriptClass& setScript(std::string_view classScript);

public:
	const std::string name;
	clock::time_point modTime;

public:
	EventDispatcher<ScriptClass*> onScriptModified;

private:
	Server* m_server;
	Script m_script;
	uint32_t m_plainChecksum;
	uint32_t m_encryptedChecksum;
	std::string m_desKey;
	std::string m_header;
	std::string m_headerWithCRC;
};
using ScriptClassPtr = std::shared_ptr<ScriptClass>;

//----------------------------

inline bool ScriptClass::hasChecksum(const uint32_t checksum) const noexcept
{
	return m_plainChecksum == checksum || m_encryptedChecksum == checksum;
}

inline Script& ScriptClass::getScript()
{
	return m_script;
}

inline const Script& ScriptClass::getScript() const
{
	return m_script;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTCLASS_H
