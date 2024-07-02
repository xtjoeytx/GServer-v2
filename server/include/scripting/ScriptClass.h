#ifndef TSCRIPTCLASS_H
#define TSCRIPTCLASS_H

#include <string>

#include <CString.h>
#include "BabyDI.h"

#include "scripting/SourceCode.h"

class Server;
class ScriptClass
{
public:
	ScriptClass(const std::string& className, const std::string& classSource);
	~ScriptClass();

	// Functions -> Inline Get-Functions
	CString getClassPacket() const;

	const std::string& getName() const
	{
		return m_className;
	}

	const CString& getByteCode() const
	{
		return m_bytecode;
	}

	const SourceCode& getSource() const
	{
		return m_source;
	}

private:
	BabyDI_INJECT(Server, m_server);

	void parseScripts(const std::string& classSource);

	std::string m_className;
	SourceCode m_source;
	CString m_bytecode;
};

#endif
