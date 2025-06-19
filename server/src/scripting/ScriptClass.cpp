#include <chrono>
#include <cstdint>
#include <string_view>
#include <string>
#include <vector>

#include <zlib.h>

#include <CString.h>
#include <IEnums.h>

#include <scripting/ScriptClass.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

ScriptClass::ScriptClass(std::string_view className, std::string_view classScript)
	: name(className)
{
	setScript(classScript);
}

ScriptClass& ScriptClass::setScript(std::string_view classScript)
{
	m_script = { classScript };

	// Set the cryptographic key to be the script's hash.
	string::string_hash hash{};
	uint64_t scriptHash = static_cast<uint64_t>(hash(classScript));

	// Package the key into two GYBTE5's.
	uint32_t* hashBytes = reinterpret_cast<uint32_t*>(&scriptHash);
	CString key = CString() >> (long long)(hashBytes[0]) >> (long long)(hashBytes[1]);
	m_desKey = key.toString();

	// CRC32 checksum.
	m_checksum = crc32(0L, Z_NULL, 0);
	m_checksum = crc32(m_checksum, (const uint8_t*)classScript.data(), classScript.length());

	// Create the header.
	// [GBYTE2 length_header_and_bytecode]
	// [STRING type,name,[0/1 save_to_disk],[GBYTE[10] checksum]]
	std::vector<std::string> headerParts =
	{
		"class",
		name,
		"1",
		m_desKey,
		CString(m_checksum).toString()
	};
	m_header = string::toCSV(headerParts);

	// Set the modification time to now.
	modTime = std::chrono::system_clock::now();
	return *this;
}

// -- Function: Get Player Packet -- //
CString ScriptClass::getClassPacket() const
{
	if (const auto& bytecode = m_script.getClientByteCode(); !bytecode.empty())
	{
		const char* bytecodePtr = reinterpret_cast<const char*>(bytecode.data());
		std::string_view bytecodeView(bytecodePtr, bytecode.size());

		return CString() >> (char)PLO_LOADSCRIPT >> (char)m_header.length() << m_header << bytecodeView;
	}

	return {};
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
