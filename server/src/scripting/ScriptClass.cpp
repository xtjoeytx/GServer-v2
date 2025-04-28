#include "BabyDI.h"

#include <GS2Context.h>

#include "Server.h"
#include "npcserver/NPCServer.h"
#include "scripting/ScriptClass.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

ScriptClass::ScriptClass(std::string_view className, std::string_view classSource)
	: m_className(className)
{
	parseScripts(classSource);
}

ScriptClass::~ScriptClass()
{
}

void ScriptClass::parseScripts(std::string_view classSource)
{
	auto* server = BabyDI::Get<Server>();

	m_source = { classSource };

	// Compile GS2 code.
	auto npcServer = server->getNpcServer();
	if (auto clientResults = npcServer->scripting.getCompiledClientScript(ScriptType::CLASS, m_className, m_source.getClientSide()); clientResults != nullptr && clientResults->success)
	{
		m_source.setClientByteCode(clientResults->bytecode);
		m_source.addClientJoinedClasses(clientResults->joinedClasses);
	}
	if (auto serverResults = npcServer->scripting.getCompiledServerScript(ScriptType::CLASS, m_className, m_source.getServerSide()); serverResults != nullptr && serverResults->success)
	{
		m_source.setServerByteCode(serverResults->bytecode);
		m_source.addServerJoinedClasses(serverResults->joinedClasses);
	}
}

// -- Function: Get Player Packet -- //
CString ScriptClass::getClassPacket() const
{
	if (auto bytecode = m_source.getClientByteCode(); bytecode != nullptr && !bytecode->empty())
	{
		CString b;
		b.write(reinterpret_cast<const char*>(bytecode->data()), bytecode->size());

		CString header = b.readChars(b.readGUShort());

		// Get the mod time and send packet 197.
		CString smod = CString() >> (long long)time(0);
		smod.gtokenizeI();
		return CString() >> (char)PLO_UNKNOWN197 << header << "," << smod << "\n";
	}

	return {};
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
