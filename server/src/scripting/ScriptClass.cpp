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

	if (const auto& bytecode = m_script.getClientByteCode(); !bytecode.empty())
	{
		// Set the cryptographic key to be the script's hash.
		constexpr string::string_hash hash{};
		auto scriptHash = static_cast<uint64_t>(hash(classScript));

		// Package the key into two GYBTE5's.
		const auto hashBytes = reinterpret_cast<uint32_t*>(&scriptHash);
		const CString key = CString() >> (long long)(hashBytes[0]) >> (long long)(hashBytes[1]);
		m_desKey = key.toString();

		// CRC32 checksum.
		m_plainChecksum = crc32(0L, Z_NULL, 0);
		m_plainChecksum = crc32(m_plainChecksum, bytecode.data(), static_cast<uint32_t>(bytecode.size()));

		ScriptByteCode encrypt{bytecode};
		{
			// Convert the hash into individual bytes.
			std::array<uint8_t, 8> keyBytes{};
			for (size_t i = 0; i < 4; ++i)
			{
				keyBytes[i] = static_cast<uint8_t>(hashBytes[0] >> (8 * i));
				keyBytes[i + 4] = static_cast<uint8_t>(hashBytes[1] >> (8 * i));
			}

			// Reverse the bits.
			// The client has a non-standard DES method that reads every byte of the key in a bit-reversed way.
			for (size_t i = 0; i < 8; ++i)
			{
				uint8_t& b = keyBytes[i];
				b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
				b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
				b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
			}

			// Encrypt the bytecode.
			symmetric_key skey;
			if (des_setup(keyBytes.data(), static_cast<int>(keyBytes.size()), 0, &skey) == CRYPT_OK)
			{
				for (size_t i = 0; i + 8 <= encrypt.size(); i += 8)
					des_ecb_encrypt(&encrypt[i], &encrypt[i], &skey);
				des_done(&skey);
			}
		}

 		m_encryptedChecksum = crc32(0L, Z_NULL, 0);
		m_encryptedChecksum = crc32(m_encryptedChecksum, encrypt.data(), static_cast<uint32_t>(encrypt.size()));

		// Create the header.
		// [GBYTE2 length_header_and_bytecode]
		// [STRING type,name,[0/1 save_to_disk],[GBYTE[10] desKey]]
		std::vector<std::string> headerParts =
		{
			"class",
			name,
			"1",
			m_desKey
		};
		m_header = string::toCSV(headerParts);

		// Header with CRC32.
		const CString crc32 = CString() >> (long long)m_encryptedChecksum;
		headerParts.push_back(crc32.toString());
		m_headerWithCRC = string::toCSV(headerParts);
	}

	// Set the modification time to now.
	modTime = std::chrono::system_clock::now();

	// Send out events.
	onScriptModified.post(this);

	return *this;
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
		else if (player->getVersion() >= CLVER_4_042)
			player->sendPacket(CString() >> (char)PLO_WEAPONHASH >> (long long)m_plainChecksum << name);
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
			player->sendRawPacket(CString() >> (char)PLO_WEAPONBYTECODE2 >> (short)m_header.length() << m_header << bytecodeView);
		else player->sendRawPacket(CString() >> (char)PLO_CLASSBYTECODE >> (char)name.length() << name << bytecodeView);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
