#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <tomcrypt.h>
#include <zlib.h>

#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <BabyDI.h>
#include <Server.h>
#include <scripting/ScriptClass.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

ScriptClass::ScriptClass(const std::string_view className, const std::string_view classScript)
	: name(className), m_plainChecksum(0), m_encryptedChecksum(0)
{
	setScript(classScript);
	m_server = BabyDI::Get<Server>();
	assert(m_server);
}

ScriptClass& ScriptClass::setScript(std::string_view classScript)
{
	m_script = {name, ScriptType::CLASS, classScript};
	generateScriptHeader();

	// Set the modification time to now.
	modTime = std::chrono::system_clock::now();

	// Send out events.
	onScriptModified.post(this);

	return *this;
}

void ScriptClass::generateScriptHeader()
{
	if (const auto& bytecode = m_script.getClientByteCode(); bytecode.empty())
		return;

	std::string desKey;
	m_script.calculateClientByteCodeChecksums(desKey, m_plainChecksum, m_encryptedChecksum);

	// Create the header.
	// [GBYTE2 length_header_and_bytecode]
	// [STRING type,name,[0/1 save_to_disk],[GBYTE[10] desKey]]
	std::vector<std::string> headerParts =
	{
		"class",
		name,
		"1",
		desKey
	};
	m_header = string::toCSV(headerParts);

	// Header with CRC32.
	const CString crc32 = CString() >> (long long)m_encryptedChecksum;
	headerParts.push_back(crc32.toString());
	m_headerWithCRC = string::toCSV(headerParts);
}

void ScriptClass::registerWithPlayer(const std::shared_ptr<Player>& player) const
{
	if (m_server->Generation != ServerGeneration::MODERN)
		return;

	// Tell the client the load the script.
	if (!m_headerWithCRC.empty())
	{
		if (player->getVersion() >= CLVER_5_07)
			player->sendPacket(CString() >> (char)PLO_LOADSCRIPT << m_headerWithCRC);
		else sendByteCodeToPlayer(player);
	}
}

void ScriptClass::sendByteCodeToPlayer(const std::shared_ptr<Player>& player) const
{
	// Send the bytecode.
	if (const auto& bytecode = m_script.getClientByteCode(); !bytecode.empty())
	{
		const auto bytecodePtr = reinterpret_cast<const char*>(bytecode.data());
		const std::string_view bytecodeView(bytecodePtr, bytecode.size());

		if (player->getVersion() >= CLVER_5_07)
			player->sendRawPacket(CString() >> (char)PLO_HEADEREDBYTECODE >> (short)m_header.length() << m_header << bytecodeView);
		else player->sendRawPacket(CString() >> (char)PLO_CLASSBYTECODE >> (char)name.length() << name << bytecodeView);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
