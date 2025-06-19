#ifndef SCRIPTCLASS_H
#define SCRIPTCLASS_H

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include <CString.h>

#include <scripting/Script.h>
#include <utilities/CommonTypes.h>

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

	[[inline]] const Script& getScript() const;
	ScriptClass& setScript(std::string_view classScript);

	[[inline]] uint32_t getCheckSum() const;

public:
	const std::string name;
	clock::time_point modTime;

private:
	Script m_script;
	uint32_t m_checksum;
	std::string m_desKey;
	std::string m_header;
};

//----------------------------

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
