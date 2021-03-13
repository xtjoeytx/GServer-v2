#include "TScriptClass.h"
#include "TServer.h"

TScriptClass::TScriptClass(TServer* server, const std::string& className, const std::string& classSource)
	: _server(server), _className(className), _classSource(classSource)
{
	parseScripts();
}

TScriptClass::~TScriptClass()
{
	
}

void TScriptClass::parseScripts()
{
	CString codeSrc(_classSource);
	_serverCode = codeSrc.readString("//#CLIENTSIDE").text();
	_clientCode = codeSrc.readString("").text();
}
