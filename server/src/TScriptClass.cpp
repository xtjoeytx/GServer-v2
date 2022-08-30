#include "GS2Context.h"
#include "TScriptClass.h"
#include "TServer.h"

TScriptClass::TScriptClass(TServer *server, const std::string& className, const std::string& classSource)
	: _className(className)
{
	parseScripts(server, classSource);
}

TScriptClass::~TScriptClass()
{

}

void TScriptClass::parseScripts(TServer *server, const std::string& classSource)
{
	bool gs2default = server->getSettings()->getBool("gs2default", false);

	_source = { classSource, gs2default };

	// Compile GS2 code
	auto gs2Script = _source.getClientGS2();
	if (!gs2Script.empty())
	{
		server->compileGS2Script(this, [this](const CompilerResponse &response)
		{
			if (response.success)
			{
				auto bytecodeWithHeader = GS2Context::CreateHeader(response.bytecode, "class", _className, true);

				// these should be sent for compilation right after
				//_joinedClasses = { response.joinedClasses.begin(), response.joinedClasses.end() };

				_bytecode.clear(bytecodeWithHeader.length());
				_bytecode.write((const char*)bytecodeWithHeader.buffer(), bytecodeWithHeader.length());

				// temp: save bytecode to file
				//CString bytecodeFile;
				//bytecodeFile << _server->getServerPath() << "bytecode/classes/";
				//std::filesystem::create_directories(bytecodeFile.text());
				//bytecodeFile << "class_" << _className << ".gs2bc";

				//CString bytecodeDump;
				//bytecodeDump.writeInt(1);
				//bytecodeDump.write((const char*)bytecodeWithHeader.buffer(), bytecodeWithHeader.length());
				//bytecodeDump.save(bytecodeFile);
			}
		});
	}
}

// -- Function: Get Player Packet -- //
void TScriptClass::sendClassPacket(TPlayer *p) const
{

	if (!_bytecode.isEmpty())
	{
		CString b = _bytecode;

		CString header = b.readChars(b.readGUShort());

		// Get the mod time and send packet 197.
		CString smod = CString() >> (long long)time(0);
		smod.gtokenizeI();
		p->sendPacket(PLO_UNKNOWN197, CString() << header << "," << smod);
	}
}
