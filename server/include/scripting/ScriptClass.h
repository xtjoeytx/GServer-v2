#ifndef SCRIPTCLASS_H
#define SCRIPTCLASS_H

#include <string>
#include <string_view>

#include <CString.h>

#include "scripting/SourceCode.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class ScriptClass
{
public:
	ScriptClass(std::string_view className, std::string_view classSource);
	~ScriptClass();

	// Functions -> Inline Get-Functions
	CString getClassPacket() const;

	const std::string& getName() const
	{
		return m_className;
	}

	const SourceCode& getSource() const
	{
		return m_source;
	}

private:
	void parseScripts(std::string_view classSource);

	std::string m_className;
	SourceCode m_source;
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SCRIPTCLASS_H
