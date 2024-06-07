#ifndef TSCRIPTCLASS_H
#define TSCRIPTCLASS_H

#include "CString.h"
#include "SourceCode.h"
#include <string>

class TServer;
class TScriptClass
{
public:
	TScriptClass(TServer* server, const std::string& className, const std::string& classSource);
	~TScriptClass();

	// Functions -> Inline Get-Functions
	CString getClassPacket() const;

	const std::string& getName() const
	{
		return _className;
	}

	const CString& getByteCode() const
	{
		return _bytecode;
	}

	const SourceCode& getSource() const
	{
		return _source;
	}

private:
	void parseScripts(TServer* server, const std::string& classSource);

	std::string _className;
	SourceCode _source;
	CString _bytecode;
};

#endif
