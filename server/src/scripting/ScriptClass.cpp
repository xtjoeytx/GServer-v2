#include <GS2Context.h>

#include "Server.h"
#include "scripting/ScriptClass.h"

ScriptClass::ScriptClass(const std::string& className, const std::string& classSource)
	: m_className(className)
{
	parseScripts(classSource);
}

ScriptClass::~ScriptClass()
{
}

void ScriptClass::parseScripts(const std::string& classSource)
{
	bool gs2default = m_server->getSettings().getBool("gs2default", false);

	m_source = { classSource, gs2default };

	// Compile GS2 code
	auto gs2Script = m_source.getClientGS2();
	if (!gs2Script.empty())
	{
		m_server->compileGS2Script(this, [this](const CompilerResponse& response)
								   {
									   if (response.success)
									   {
										   auto bytecodeWithHeader = GS2Context::CreateHeader(response.bytecode, "class", m_className, true);

										   // these should be sent for compilation right after
										   //m_joinedClasses = { response.joinedClasses.begin(), response.joinedClasses.end() };

										   m_bytecode.clear(bytecodeWithHeader.length());
										   m_bytecode.write((const char*)bytecodeWithHeader.buffer(), static_cast<int>(bytecodeWithHeader.length()));

										   // temp: save bytecode to file
										   //CString bytecodeFile;
										   //bytecodeFile << m_server->getServerPath() << "bytecode/classes/";
										   //std::filesystem::create_directories(bytecodeFile.text());
										   //bytecodeFile << "class_" << m_className << ".gs2bc";

										   //CString bytecodeDump;
										   //bytecodeDump.writeInt(1);
										   //bytecodeDump.write((const char*)bytecodeWithHeader.buffer(), bytecodeWithHeader.length());
										   //bytecodeDump.save(bytecodeFile);
									   }
								   });
	}
}

// -- Function: Get Player Packet -- //
CString ScriptClass::getClassPacket() const
{
	CString out;

	if (!m_bytecode.isEmpty())
	{
		CString b = m_bytecode;

		CString header = b.readChars(b.readGUShort());

		// Get the mod time and send packet 197.
		CString smod = CString() >> (long long)time(0);
		smod.gtokenizeI();
		out >> (char)PLO_UNKNOWN197 << header << "," << smod << "\n";
	}

	return out;
}
