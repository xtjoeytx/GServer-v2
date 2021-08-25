#include <GS2Context.h>
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
	bool gs2default = _server->getSettings()->getBool("gs2default", false);

	_source = {_classSource, gs2default };
	_serverCode = _source.getServerSide();
	_clientCode = _source.getClientGS1();

	// Compile GS2 code
	auto gs2Script = _source.getClientGS2();
	if (!gs2Script.empty())
	{
		GS2Context context;
		auto byteCode = context.compile(std::string{ gs2Script }, "class", _className, true);

		if (!context.hasErrors())
		{
			_bytecode.clear();
			_bytecode.write((const char*)byteCode.buffer(), byteCode.length());
		}
		else
		{
			printf("Compilation Error: %s\n", context.getErrors()[0].msg().c_str());
		}
	}
}

// -- Function: Get Player Packet -- //
CString TScriptClass::getClassPacket() const
{
	CString out;

	if (!_bytecode.isEmpty())
	{
		CString b = _bytecode;

		CString header = b.readChars(b.readGUShort());

		// Get the mod time and send packet 197.
		CString smod = CString() >> (long long)time(0);
		smod.gtokenizeI();
		out >> (char)PLO_UNKNOWN197 << header << "," << smod << "\n";
	}

	return out;
}

