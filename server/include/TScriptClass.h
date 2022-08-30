#ifndef TSCRIPTCLASS_H
#define TSCRIPTCLASS_H

#pragma once

#include <string>
#include "CString.h"
#include "SourceCode.h"
#include "TPlayer.h"

class TServer;
class TScriptClass
{
public:
	TScriptClass(TServer* server, const std::string& className, const std::string& classSource);
	~TScriptClass();

	// Functions -> Inline Get-Functions
	void sendClassPacket(TPlayer *p) const;

	const std::string& getName() const {
		return _className;
	}

	const CString& getByteCode() const {
		return _bytecode;
	}

	const SourceCode& getSource() const {
		return _source;
	}

private:
	void parseScripts(TServer *server, const std::string& classSource);

	std::string _className;
	SourceCode _source;
	CString _bytecode;
};

#endif
