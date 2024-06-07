#ifndef TSCRIPTCLASS_H
#define TSCRIPTCLASS_H

#pragma once

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
	void parseScripts(TServer* server, const std::string& classSource);

	std::string m_className;
	SourceCode m_source;
	CString m_bytecode;
};

#endif
