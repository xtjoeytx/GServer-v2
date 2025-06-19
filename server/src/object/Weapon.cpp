#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <zlib.h>

#include <BabyDI.h>
#include <CString.h>
#include <IEnums.h>

#include <FileSystem.h>
#include <Server.h>
#include <level/LevelItem.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Weapon.h>
#include <scripting/Script.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

Weapon::Weapon(std::string_view name, std::string_view image, std::string_view script)
	: name(name), modTime(clock::now()), m_weaponDefault(LevelItemType::INVALID)
{
	updateWeapon(image, script);
}

//----------------------------

std::shared_ptr<Weapon> Weapon::loadWeapon(const CString& pWeapon)
{
	// File Path
	CString fileName = CString() << "weapons" << FileSystem::getPathSeparator() << pWeapon;

	// Load File
	CString fileData;
	if (!fileData.load(fileName))
		return nullptr;

	fileData.removeAllI("\r");

	// Grab some information.
	bool has_scriptend = fileData.find("SCRIPTEND") != -1;
	bool found_scriptend = false;

	// Parse header
	CString headerLine = fileData.readString("\n");
	if (headerLine != "GRAWP001")
		return nullptr;

	// Definitions
	std::string weaponName, weaponImage, weaponScript;
	//std::string byteCodeFile;
	//CString byteCodeData;

	// Parse File
	while (fileData.bytesLeft())
	{
		CString curLine = fileData.readString("\n");

		// Find Command
		CString curCommand = curLine.readString();

		// Parse Line
		if (curCommand == "REALNAME")
		{
			weaponName = curLine.readString("").toString();
		}
		else if (curCommand == "IMAGE")
			weaponImage = curLine.readString("").toString();
		else if (curCommand == "BYTECODE")
		{
			/*
			CString fileName = curLine.readString("");

			byteCodeData.load(CString() << "weapon_bytecode/" << fileName);
			if (!byteCodeData.isEmpty())
				byteCodeFile = fileName.toString();
			*/
		}
		else if (curCommand == "SCRIPT")
		{
			do {
				curLine = fileData.readString("\n");
				if (curLine == "SCRIPTEND")
				{
					found_scriptend = true;
					break;
				}

				weaponScript.append(curLine.text()).append("\n");
			}
			while (fileData.bytesLeft());
		}
	}

	// Valid Weapon Name?
	if (weaponName.empty())
		return nullptr;

	// Give a warning if our weapon was malformed.
	if (has_scriptend && !found_scriptend)
	{
		log::printLine(log::server, "WARNING: Weapon {} is malformed.", weaponName);
		log::printLine(log::server, "SCRIPTEND needs to be on its own line.");
	}

	// Create the weapon.
	auto weapon = std::make_shared<Weapon>(weaponName, weaponImage, weaponScript);

	// Set the mod time to the file mod time.
	weapon->modTime = clock::from_time_t(std::filesystem::last_write_time(fileName.toString()).time_since_epoch().count());

	// Give a warning if both a script and a bytecode was found.
	/*
	if (!weaponScript.empty() && !byteCodeData.isEmpty())
	{
		log::printLine(log::server, "WARNING: Weapon {} includes both script and bytecode.  Using bytecode.", weapon->name);
		weaponScript.clear();
	}
	*/

	/*
	* TODO(Nalin): Figure out how to reimplement this.
	if (!byteCodeData.isEmpty())
	{
		auto byteCodeDataPtr = reinterpret_cast<uint8_t*>(byteCodeData.text());
		std::vector<uint8_t> bytecode{ byteCodeDataPtr, byteCodeDataPtr + byteCodeData.length() };
		auto clientByteCode = std::make_shared<ScriptByteCode>(std::move(bytecode));
		weapon->m_source.setClientByteCode(clientByteCode);
		weapon->m_bytecodeFile = byteCodeFile;
	}
	*/

	return weapon;
}

//----------------------------

bool Weapon::saveWeapon()
{
	// Don't save default weapons / empty weapons
	if (this->isDefault() || name.empty())
		return false;

	// If the bytecode filename is set, the weapon is treated as read-only so it can't be saved
	//if (!m_bytecodeFile.empty())
	//	return false;

	// Prevent the loading/saving of filenames with illegal characters.
	CString escapedName = name;
	escapedName.replaceAllI("\\", "_");
	escapedName.replaceAllI("/", "_");
	escapedName.replaceAllI("*", "@");
	escapedName.replaceAllI(":", ";");
	escapedName.replaceAllI("?", "!");
	CString filename = CString() << "weapons" << FileSystem::getPathSeparator() << "weapon" << escapedName << ".txt";

	// Write the File.
	CString output = "GRAWP001\r\n";
	output << "REALNAME " << name << "\r\n";
	output << "IMAGE " << image << "\r\n";

	const auto& originalSource = m_script.getOriginalSource();
	if (!originalSource.empty())
	{
		output << "SCRIPT\r\n";
		output << CString(originalSource).removeAllI("\r").replaceAllI("\n", "\r\n");

		// Append a new line to the end of the script if one doesn't exist.
		if (originalSource.back() != '\n')
			output << "\r\n";

		output << "SCRIPTEND\r\n";
	}

	// Save it.
	return output.save(filename);
}

Weapon& Weapon::updateWeapon(std::string_view image, std::string_view script)
{
	m_script = std::move(Script{ script });
	this->image = image;
	modTime = clock::now();

	// Set the cryptographic key to be the script's hash.
	string::string_hash hash{};
	uint64_t scriptHash = static_cast<uint64_t>(hash(script));

	// Package the key into two GYBTE5's.
	uint32_t* hashBytes = reinterpret_cast<uint32_t*>(&scriptHash);
	CString key = CString() >> (long long)(hashBytes[0]) >> (long long)(hashBytes[1]);
	m_desKey = key.toString();

	// CRC32 checksum.
	m_checksum = crc32(0L, Z_NULL, 0);
	m_checksum = crc32(m_checksum, (const uint8_t*)script.data(), script.length());

	// Create the header.
	// [GBYTE2 length_header_and_bytecode]
	// [STRING type,name,[0/1 save_to_disk],[GBYTE[10] checksum]]
	std::vector<std::string> headerParts =
	{
		"weapon",
		name,
		"1",
		m_desKey,
		CString(m_checksum).toString()
	};
	m_header = string::toCSV(headerParts);

	return *this;
}

//----------------------------

CString Weapon::getAddWeaponPacket() const
{
	if (this->isDefault())
		return CString() >> (char)PLO_DEFAULTWEAPON >> (char)m_weaponDefault;

	CString weaponPacket;
	weaponPacket >> (char)PLO_NPCWEAPONADD >> (char)name.length() << name >> (char)NPCProp::IMAGE >> (char)image.length() << image;

	// Classic weapons.
	if (m_script.getClientByteCode().empty())
	{
		weaponPacket >> (char)NPCProp::SCRIPT >> (short)m_script.getClientSide().length() << m_script.getClientSide();
	}
	// If we have bytecode, send the weapon headers.
	else
	{
		auto classes = getJoinedClasses();
		weaponPacket >> (char)NPCProp::CLASS >> (char)classes.length() << classes << "\n";

		// Send the bytecode.
		weaponPacket << getWeaponByteCodePacket();
	}

	return weaponPacket;
}

CString Weapon::getWeaponByteCodePacket() const
{
	// Send the bytecode.
	if (const auto& bytecode = m_script.getClientByteCode(); !bytecode.empty())
	{
		const char* bytecodePtr = reinterpret_cast<const char*>(bytecode.data());
		std::string_view bytecodeView(bytecodePtr, bytecode.size());

		return CString() >> (char)PLO_LOADSCRIPT >> (char)m_header.length() << m_header << bytecodeView;
	}
	return CString();
}

std::string Weapon::getJoinedClasses() const
{
	std::string result;
	for (const auto& classPtr : m_joinedClasses)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
		{
			result += scriptClass->name;
			result += ",";
		}
	}
	result.pop_back();
	return result;
}

void Weapon::setJoinedClasses(std::string_view classes)
{
	auto server = BabyDI::Get<Server>();
	if (server == nullptr || !server->hasNPCServer()) return;

	m_joinedClasses.clear();
	while (!classes.empty())
	{
		auto className = string::extractLine(classes, ',');
		if (className.empty())
			continue;

		className = string::trim(className);
		auto scriptClass = server->getNPCServer()->getClass(className);
		if (!scriptClass.expired())
			m_joinedClasses.push_back(scriptClass);

		// TODO(Nalin): Need a way to handle this on weapons.
		// modTime[PROPID(NPCProp::CLASS)] = currentTime();
	}
}

void Weapon::executeEvents(ScriptEventQueue& events, ScriptObjectSource source) const
{
	m_script.executeEvents(events, source);
	for (auto& scriptClassPtr : m_joinedClasses)
	{
		if (auto scriptClass = scriptClassPtr.lock(); scriptClass != nullptr)
			scriptClass->getScript().executeEvents(events, source);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
