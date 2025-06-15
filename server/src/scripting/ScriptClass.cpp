#include <ctime>
#include <string_view>

#include <CString.h>
#include <IEnums.h>

#include <BabyDI.h>
#include <Server.h>
#include <npcserver/NPCServer.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

ScriptClass::ScriptClass(std::string_view className, std::string_view classSource)
	: name(className)
{
	parseScripts(classSource);
}

void ScriptClass::parseScripts(std::string_view classSource)
{
	auto* server = BabyDI::Get<Server>();

	m_source = { classSource };

	// Compile the scripts.
	auto npcServer = server->getNPCServer();
	if (auto clientResults = npcServer->scripting.getCompiledClientScript(ScriptType::CLASS, m_className, m_source.getClientSide()); clientResults != nullptr)
	{
		m_source.setClientCompiledScript(clientResults);
	}
	if (auto serverResults = npcServer->scripting.getCompiledServerScript(ScriptType::CLASS, m_className, m_source.getServerSide()); serverResults != nullptr)
	{
		m_source.setServerCompiledScript(serverResults);
	}
}

// -- Function: Get Player Packet -- //
CString ScriptClass::getClassPacket() const
{
	if (const auto& bytecode = m_source.getClientByteCode(); !bytecode.empty())
	{
		CString b;
		b.write(reinterpret_cast<const char*>(bytecode.data()), bytecode.size());

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
