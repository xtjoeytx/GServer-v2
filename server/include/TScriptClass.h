#ifndef TSCRIPTCLASS_H
#define TSCRIPTCLASS_H

#pragma once

#include <string>

class TServer;
class TScriptClass
{
public:
	TScriptClass(TServer* server, const std::string& className, const std::string& classSource);
	~TScriptClass();

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
	std::string _classSource;
	std::string _clientCode, _serverCode;
};

#endif
