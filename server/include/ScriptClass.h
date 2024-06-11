#ifndef TSCRIPTCLASS_H
#define TSCRIPTCLASS_H

#include "CString.h"
#include "Player.h"
#include "SourceCode.h"
#include <string>

class Server;
class ScriptClass
{
public:
	ScriptClass(Server* server, const std::string& className, const std::string& classSource);
	~ScriptClass();

	// Functions -> Inline Get-Functions
	PlayerOutPacket getClassPacket() const;

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
	void parseScripts(Server* server, const std::string& classSource);

	std::string m_className;
	SourceCode m_source;
	CString m_bytecode;
};

#endif
