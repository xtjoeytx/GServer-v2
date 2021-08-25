#ifndef TSCRIPTCLASS_H
#define TSCRIPTCLASS_H

#pragma once

#include <string>
#include <SourceCode.h>
#include <CString.h>

class TServer;
class TScriptClass
{
public:
	TScriptClass(TServer* server, const std::string& className, const std::string& classSource);
	~TScriptClass();

	// Functions -> Inline Get-Functions
	CString getClassPacket() const;
	inline const CString& getByteCode() const		{ return _bytecode; }

	const std::string& source() const {
		return _classSource;
	}

	const std::string& serverCode() const {
		return _serverCode;
	}

	const std::string& clientCode() const {
		return _clientCode;
	}

private:
	void parseScripts();

	TServer* _server;
	std::string _className;
	CString _bytecode;
	SourceCode _source;
	std::string _classSource;
	std::string _clientCode, _serverCode;
};

#endif
