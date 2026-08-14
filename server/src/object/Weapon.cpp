#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <zlib.h>

#include <BabyDI.h>
#include <CString.h>
#include <IEnums.h>

#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystemTypes.h>
#include <level/LevelItem.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <scripting/Script.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

Weapon::Weapon(const LevelItemType itemType)
	: name(LevelItem::getItemName(itemType)), modTime(clock::now()), m_weaponDefault(itemType)
{
	m_server = BabyDI::Get<Server>();
	assert(m_server != nullptr);
}

Weapon::Weapon(const std::string_view name, const std::string_view image, const std::string_view script)
	: name(name), modTime(clock::now()), m_weaponDefault(LevelItemType::INVALID)
{
	m_server = BabyDI::Get<Server>();
	assert(m_server != nullptr);
	updateWeapon(image, script);
}

//----------------------------

std::shared_ptr<Weapon> Weapon::loadWeapon(const std::filesystem::path& fileName)
{
	// Calculated file name.
	// Non-alphanumeric characters are encoded as %000.

	const auto server = BabyDI::Get<Server>();
	const auto file = server->getFileSystemServer().openi(fs::FileCategory::WEAPON, fileName.string());
	if (file == nullptr || !file->opened())
		return nullptr;

	if (const auto header = file->readLine(); header != "GRAWP001")
		return nullptr;

	const auto weaponName = file->readConfigLine("REALNAME", " "sv);
	const auto weaponImage = file->readConfigLine("IMAGE", " "sv);
	const auto weaponScript = file->readConfigSection("SCRIPT", "SCRIPTEND");

	// Valid Weapon Name?
	if (!weaponName.has_value() || weaponName->empty())
		return nullptr;

	// Create the weapon.
	auto weapon = std::make_shared<Weapon>(weaponName.value(), weaponImage.value_or(""s), weaponScript.value_or(""s));

	// Check for script limits.
	if (const auto& script = weapon->getScript(); script.getClientByteCode().empty() && script.getClientSide().length() > 0x705F)
		log::printLine(log::server, "WARNING: Clientside script of weapon ({}) exceeds the limit of 28767 bytes.", *weaponName);

	// Set the mod time to the file mod time.
	weapon->modTime = fs::getFileModTime(file->filePath());

	// Check if we need to rename the file.
	const auto expectedFileName = fs::getHTMLEscapedFileName(std::format("weapon{}.txt", weapon->name)).string();
	const auto currentFileName = fs::getANSIFileName(fileName);
	if (expectedFileName != currentFileName)
	{
		if (const auto fileData = server->getFileSystemServer().infoi(fs::FileCategory::WEAPON, currentFileName); fileData != nullptr)
		{
			auto indent = log::server.indent();
			if (server->getFileSystemServer().rename(*fileData, expectedFileName))
				log::printLine(log::server, "Renamed weapon file [{}] to [{}]", currentFileName, expectedFileName);
			else
				log::printLine(log::server, "** Failed to rename weapon file [{}] to [{}]", currentFileName, expectedFileName);
		}
	}

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

	const auto fileName = fs::getHTMLEscapedFileName(std::format("weapon{}.txt", name));
	const auto file = m_server->getFileSystemServer().openiForWriting(fs::FileCategory::WEAPON, fileName, true);
	if (!file)
		return false;

	// Write the file.
	file->clear();
	file->writeLine("GRAWP001");
	file->writeConfigLine("REALNAME"sv, name);
	file->writeConfigLine("IMAGE"sv, image);

	// Write the script.
	const auto& originalSource = m_script.getOriginalSource();
	if (!originalSource.empty())
		file->writeConfigSection("SCRIPT", originalSource, "SCRIPTEND");

	file->close();

	calculateHeaderChecksum();

	// Queue the created event.
	scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());

	return true;
}

Weapon& Weapon::updateWeapon(const std::string_view newImage, const std::string_view newScript)
{
	setJoinedClasses("");
	m_script = Script{name, newScript};
	image = newImage;
	modTime = clock::now();

	calculateHeaderChecksum();

	// Queue the created event.
	scripting.events.addEvent(ScriptEventType::CREATED, source::FromServer());

	return *this;
}

//----------------------------

void Weapon::registerWeaponWithPlayer(const std::shared_ptr<Player>& player) const
{
	if (isDefault())
	{
		player->sendPacket(CString() >> (char)PLO_DEFAULTWEAPON >> (char)m_weaponDefault);
		return;
	}

	CString weaponPacket;
	weaponPacket >> (char)PLO_NPCWEAPONADD >> (char)name.length() << name >> (char)NPCProp::IMAGE >> (char)image.length() << image;

	// Classic weapons.
	if (m_script.getClientByteCode().empty() && player->getVersion() <= CLVER_5_07)
	{
		const std::string script = getClientSideScript();
		weaponPacket >> (char)NPCProp::SCRIPT >> (short)script.length() << script;
		player->sendPacket(weaponPacket);
	}
	// If we have bytecode, send the weapon headers.
	else if (m_server->Generation == ServerGeneration::MODERN)
	{
		const auto classes = getJoinedClassesList();
		if (!classes.empty())
		{
			weaponPacket >> (char)NPCProp::CLASS >> (short)classes.length() << classes;
			player->sendPacket(weaponPacket);
		}

		// Tell the client the load the script.
		if (!m_headerWithCRC.empty())
			player->sendPacket(CString() >> (char)PLO_LOADSCRIPT << m_headerWithCRC);
	}
}

void Weapon::sendByteCodeToPlayer(const std::shared_ptr<Player>& player) const
{
	// Send the bytecode.
	if (const auto& bytecode = m_script.getClientByteCode(); !bytecode.empty())
	{
		const auto bytecodePtr = reinterpret_cast<const char*>(bytecode.data());
		const std::string_view bytecodeView(bytecodePtr, bytecode.size());
		player->sendPacket(CString() >> (char)PLO_NPCWEAPONSCRIPT >> (short)m_header.length() << m_header << bytecodeView);
	}
}

//----------------------------

std::string Weapon::getJoinedClassesList() const
{
	bool hasExpired = false;
	std::string result;
	for (const auto& classPtr : m_joinedClasses | std::views::values)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
		{
			result += scriptClass->name;
			result += ',';
		}
		else hasExpired = true;
	}
	result.pop_back();

	// If we have expired, clear them out.
	if (hasExpired)
	{
		std::erase_if(m_joinedClasses, [](const decltype(m_joinedClasses)::value_type& pair) { return pair.second.expired(); });
	}

	return result;
}

void Weapon::setJoinedClasses(std::string_view classes)
{
	if (!m_server->hasNPCServer()) return;

	for (const auto& [handle, classPtr] : m_joinedClasses)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
			scriptClass->onScriptModified.unsubscribe(handle);
	}

	m_joinedClasses.clear();

	while (!classes.empty())
	{
		auto className = string::extractLine(classes, ',');
		if (className.empty())
			continue;

		className = string::trim(className);
		if (const auto scriptClass = m_server->getNPCServer()->getClass(className); scriptClass != nullptr)
		{
			auto handle = scriptClass->onScriptModified.subscribe(std::bind(&Weapon::updateScriptClass, this, std::placeholders::_1));
			m_joinedClasses.emplace_back(handle, scriptClass);
			m_server->updateWeaponForPlayers(this);
		}
	}
}

void Weapon::joinClass(const std::string_view className)
{
	auto it = std::ranges::find_if(m_joinedClasses, [&className](const decltype(m_joinedClasses)::value_type& kvp) { return kvp.second.lock()->name == className; });
	if (it != m_joinedClasses.end())
		return;

	if (!m_server->hasNPCServer())
		return;

	if (const auto scriptClass = m_server->getNPCServer()->getClass(className); scriptClass != nullptr)
	{
		auto handle = scriptClass->onScriptModified.subscribe(std::bind(&Weapon::updateScriptClass, this, std::placeholders::_1));
		m_joinedClasses.emplace_back(handle, scriptClass);
		m_server->updateWeaponForPlayers(this);
	}
	else
	{
		log::printLine(log::npc, "Error: Weapon '{}' tried to join class '{}', but it does not exist.", name, className);
	}
}

void Weapon::leaveClass(const std::string_view className)
{
	auto it = std::ranges::find_if(m_joinedClasses, [&className](const decltype(m_joinedClasses)::value_type& kvp) { return kvp.second.lock()->name == className; });
	if (it == m_joinedClasses.end())
		return;

	if (!m_server->hasNPCServer())
		return;

	if (const auto scriptClass = it->second.lock(); scriptClass != nullptr)
		scriptClass->onScriptModified.unsubscribe(it->first);

	m_joinedClasses.erase(it);
	m_server->updateWeaponForPlayers(this);
}

std::string Weapon::getClientSideScript() const
{
	std::string result{ m_script.getClientSide() };
	for (const auto& classPtr : m_joinedClasses | std::views::values)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
		{
			if (const auto& clientSide = scriptClass->getScript().getClientSide(); !clientSide.empty())
			{
				result += '\xa7';
				result += clientSide;
			}
		}
	}
	return result;
}

void Weapon::calculateHeaderChecksum()
{
	const auto& script = m_script.getOriginalSource();

	// Set the cryptographic key to be the script's hash.
	// This will become the DES encryption key the client will use the encrypt the bytecode.
	constexpr string::string_hash hash{};
	auto scriptHash = static_cast<uint64_t>(hash(script));

	// Package the key into two GYBTE5's.
	const auto hashBytes = reinterpret_cast<uint32_t*>(&scriptHash);
	const CString key = CString() >> (long long)(hashBytes[0]) >> (long long)(hashBytes[1]);
	m_desKey = key.toString();

	// CRC32 checksum.
	m_checksum = crc32(0L, Z_NULL, 0);
	m_checksum = crc32(m_checksum, reinterpret_cast<const uint8_t*>(script.data()), script.length());

	// Create the header.
	// [GBYTE2 length_header_and_bytecode]
	// [STRING type,name,[0/1 save_to_disk],[GBYTE[10] desKey]]
	std::vector<std::string> headerParts =
	{
		"weapon",
		name,
		"1",
		m_desKey
	};
	m_header = string::toCSV(headerParts);

	// Header with CRC32.
	const CString crc32{static_cast<long long>(m_checksum)};
	headerParts.push_back(crc32.toString());
	m_headerWithCRC = string::toCSV(headerParts);
}

void Weapon::updateScriptClass(ScriptClass* scriptClass) const
{
	m_server->updateWeaponForPlayers(this);
}

//----------------------------

void Weapon::executeEvents(ScriptEventQueue& events, const ScriptObject& source) const
{
	if (events.queue().empty())
		return;

	m_script.executeEvents(events, source);

	for (auto& scriptClassPtr : m_joinedClasses | std::views::values)
	{
		if (auto scriptClass = scriptClassPtr.lock(); scriptClass != nullptr)
			scriptClass->getScript().executeEvents(events, source);
	}

	events.queue().clear();
}

//----------------------------

ScriptObject source::FromWeapon(const WeaponPtr& weapon)
{
	size_t hash = string::string_hash{}(weapon->name);
	return std::make_pair(hash, ScriptObjectType::WEAPON);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
