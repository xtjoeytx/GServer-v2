#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
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

ScriptClass::ScriptClass(const std::string_view className, const std::string_view classScript)
	: name(className), m_checksum(0)
{
	setScript(classScript);
}

ScriptClass& ScriptClass::setScript(std::string_view classScript)
{
	m_script = {util::constructScriptName(std::format("(Class)"), name), classScript};

	// Set the cryptographic key to be the script's hash.
	constexpr string::string_hash hash{};
	auto scriptHash = static_cast<uint64_t>(hash(classScript));

	// Package the key into two GYBTE5's.
	const auto hashBytes = reinterpret_cast<uint32_t*>(&scriptHash);
	const CString key = CString() >> (long long)(hashBytes[0]) >> (long long)(hashBytes[1]);
	m_desKey = key.toString();

	// CRC32 checksum.
	m_checksum = crc32(0L, Z_NULL, 0);
	m_checksum = crc32(m_checksum, reinterpret_cast<const uint8_t*>(classScript.data()), classScript.length());

	// Create the header.
	// [GBYTE2 length_header_and_bytecode]
	// [STRING type,name,[0/1 save_to_disk],[GBYTE[10] checksum]]
	std::vector<std::string> headerParts =
	{
		"class",
		name,
		"1",
		m_desKey,
		(CString() >> (long long)m_checksum).toString()
	};
	m_header = string::toCSV(headerParts);

	// Set the modification time to now.
	modTime = std::chrono::system_clock::now();

	// Send out events.
	onScriptModified.post(this);

	return *this;
}

// -- Function: Get Player Packet -- //
CString ScriptClass::getClassPacket() const
{
	if (const auto& bytecode = m_script.getClientByteCode(); !bytecode.empty())
	{
		const auto bytecodePtr = reinterpret_cast<const char*>(bytecode.data());
		const std::string_view bytecodeView(bytecodePtr, bytecode.size());

		return CString() >> (char)PLO_LOADSCRIPT >> (char)m_header.length() << m_header << bytecodeView;
	}

	return {};
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
