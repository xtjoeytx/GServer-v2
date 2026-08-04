#ifndef SCRIPTCLASS_H
#define SCRIPTCLASS_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <string>

#include <CString.h>

#include <scripting/Script.h>
#include <utilities/CommonTypes.h>
#include <utilities/Events.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class ScriptClass
{
public:
	ScriptClass(std::string_view className, std::string_view classScript);
	~ScriptClass() = default;

	CString getClassPacket() const;

	[[a::inline]] Script& getScript();
	[[a::inline]] const Script& getScript() const;
	ScriptClass& setScript(std::string_view classScript);

	[[a::inline]] uint32_t getCheckSum() const;

public:
	const std::string name;
	clock::time_point modTime;

public:
	EventDispatcher<ScriptClass*> onScriptModified;

private:
	Script m_script;
	uint32_t m_checksum;
	std::string m_desKey;
	std::string m_header;
};
using ScriptClassPtr = std::shared_ptr<ScriptClass>;

//----------------------------

inline Script& ScriptClass::getScript()
{
	return m_script;
}

inline const Script& ScriptClass::getScript() const
{
	return m_script;
}

inline uint32_t ScriptClass::getCheckSum() const
{
	return m_checksum;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTCLASS_H
