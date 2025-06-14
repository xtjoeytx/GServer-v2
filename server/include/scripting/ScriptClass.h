#ifndef SCRIPTCLASS_H
#define SCRIPTCLASS_H

#include <string>
#include <string_view>

#include <CString.h>

#include <scripting/Script.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class ScriptClass
{
public:
	ScriptClass(std::string_view className, std::string_view classSource);
	~ScriptClass() = default;

	CString getClassPacket() const;

	[[inline]] const Script& getSource() const;

public:
	const std::string name;

private:
	void parseScripts(std::string_view classSource);

	Script m_source;
};

//----------------------------

inline const Script& ScriptClass::getSource() const
{
	return m_source;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTCLASS_H
