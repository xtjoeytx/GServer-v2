#include <IEnums.h>
#include <IUtil.h>

// GS2 Compiler includes
#include <GS2Context.h>

#include "Server.h"
#include "object/NPC.h"
#include "object/Weapon.h"
#include "level/LevelItem.h"
#include "npcserver/NPCServer.h"
#include "scripting/SourceCode.h"
#include "utilities/Log.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

// -- Constructor: Default Weapons -- //
Weapon::Weapon(LevelItemType pId)
	: m_modTime(0), m_weaponDefault(pId)
{
	m_weaponName = LevelItem::getItemName(m_weaponDefault);
}

// -- Constructor: Weapon Script -- //
Weapon::Weapon(std::string pName, std::string pImage, std::string pScript, const time_t pModTime, bool pSaveWeapon)
	: m_weaponName(std::move(pName)), m_modTime(pModTime), m_weaponDefault(LevelItemType::INVALID)
{
	// Update Weapon
	this->updateWeapon(std::move(pImage), std::move(pScript), pModTime, pSaveWeapon);
}

Weapon::~Weapon()
{
}

// -- Function: Load Weapon -- //
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
	CString byteCodeData;
	std::string byteCodeFile, weaponImage, weaponName, weaponScript;

	// Parse File
	while (fileData.bytesLeft())
	{
		CString curLine = fileData.readString("\n");

		// Find Command
		CString curCommand = curLine.readString();

		// Parse Line
		if (curCommand == "REALNAME")
			weaponName = curLine.readString("").toString();
		else if (curCommand == "IMAGE")
			weaponImage = curLine.readString("").toString();
		else if (curCommand == "BYTECODE")
		{
			CString fileName = curLine.readString("");

			byteCodeData.load(CString() << "weapon_bytecode/" << fileName);
			if (!byteCodeData.isEmpty())
				byteCodeFile = fileName.toString();
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

	// Give a warning if both a script and a bytecode was found.
	if (!weaponScript.empty() && !byteCodeData.isEmpty())
	{
		log::printLine(log::server, "WARNING: Weapon {} includes both script and bytecode.  Using bytecode.", weaponName);
		weaponScript.clear();
	}

	auto weapon = std::make_shared<Weapon>(weaponName, weaponImage, weaponScript, 0);
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

// -- Function: Save Weapon -- //
bool Weapon::saveWeapon()
{
	// Don't save default weapons / empty weapons
	if (this->isDefault() || m_weaponName.empty())
		return false;

	// If the bytecode filename is set, the weapon is treated as read-only so it can't be saved
	if (!m_bytecodeFile.empty())
		return false;

	// Prevent the loading/saving of filenames with illegal characters.
	CString name = m_weaponName;
	name.replaceAllI("\\", "_");
	name.replaceAllI("/", "_");
	name.replaceAllI("*", "@");
	name.replaceAllI(":", ";");
	name.replaceAllI("?", "!");
	CString filename = CString() << "weapons" << FileSystem::getPathSeparator() << "weapon" << name << ".txt";

	// Write the File.
	CString output = "GRAWP001\r\n";
	output << "REALNAME " << m_weaponName << "\r\n";
	output << "IMAGE " << m_weaponImage << "\r\n";

	const auto& originalSource = m_source.getOriginalSource();
	if (!originalSource.empty())
	{
		output << "SCRIPT\r\n";
		output << CString(originalSource).replaceAll("\n", "\r\n");

		// Append a new line to the end of the script if one doesn't exist.
		if (originalSource.back() != '\n')
			output << "\r\n";

		output << "SCRIPTEND\r\n";
	}

	// Save it.
	return output.save(filename);
}

// -- Function: Get Player Packet -- //
CString Weapon::getWeaponPacket(int clientVersion) const
{
	if (this->isDefault())
		return CString() >> (char)PLO_DEFAULTWEAPON >> (char)m_weaponDefault;

	CString weaponPacket;
	weaponPacket >> (char)PLO_NPCWEAPONADD >> (char)m_weaponName.length() << m_weaponName >> (char)NPCProp::IMAGE >> (char)m_weaponImage.length() << m_weaponImage;

	const auto& bytecode = m_source.getClientByteCode();

	// Classic weapons.
	if (m_source.getClientByteCode().empty())
	{
		weaponPacket >> (char)NPCProp::SCRIPT >> (short)m_source.getClientSide().length() << m_source.getClientSide();
	}
	// If we have bytecode, send the weapon headers.
	else
	{
		// Weapons don't have a class.
		// Maybe?  Confused about this.
		weaponPacket >> (char)NPCProp::CLASS >> (short)0 << "\n";

		// Extract the header and send it.
		CString header = std::string_view{ reinterpret_cast<const char*>(bytecode.data()), bytecode.size() };
		weaponPacket >> (char)PLO_UNKNOWN197 << header.readChars(header.readGUShort()) << "," >> (long long)time(0) << "\n";
	}

	return weaponPacket;
}

// -- Function: Update Weapon Image/Script -- //
void Weapon::updateWeapon(std::string pImage, std::string pCode, const time_t pModTime, bool pSaveWeapon)
{
	m_source = std::move(SourceCode{ std::move(pCode) });
	m_weaponImage = std::move(pImage);
	setModTime(pModTime == 0 ? time(0) : pModTime);

	if (m_server->isNpcServerEnabled())
	{
		// If we have an npc-server, compile the scripts.
		auto npcServer = m_server->getNpcServer();
		if (m_server->Generation == ServerGeneration::CLASSIC)
		{
			m_source.setServerCompiledScript(npcServer->scripting.getCompiledServerScript(ScriptType::WEAPON, m_weaponName, m_source.getServerSide()));
		}
		else if (m_server->Generation == ServerGeneration::NEWMAIN || m_server->Generation == ServerGeneration::MODERN)
		{
			m_source.setClientCompiledScript(npcServer->scripting.getCompiledClientScript(ScriptType::WEAPON, m_weaponName, m_source.getClientSide()));
			m_source.setServerCompiledScript(npcServer->scripting.getCompiledServerScript(ScriptType::WEAPON, m_weaponName, m_source.getServerSide()));
		}
	}

	// Save Weapon
	if (pSaveWeapon)
		saveWeapon();
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
