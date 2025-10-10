#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <ranges>
#include <string_view>
#include <string>
#include <vector>

#include <BabyDI.h>
#include <CString.h>

#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <loader/flatfile/FlatFileNPCLoader.h>
#include <object/NPC.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>
#include <filesystem/FileSystemTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

static constexpr std::array<uint8_t, 30> attrPackets = { 36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 };

///////////////////////////////////////////////////////////////////////////////

NPCPtr FlatFileNPCLoader::loadNPC(std::string_view npcName) noexcept
{
	auto server = BabyDI::Get<Server>();
	auto fileInfo = server->getFileSystemServer().info(fs::FileCategory::NPC, std::format("npc{}.txt", npcName));
	if (fileInfo == nullptr)
		return nullptr;

	return loadNPC(fileInfo->file);
}

NPCPtr FlatFileNPCLoader::loadNPC(const std::filesystem::path& filePath) noexcept
{
	auto server = BabyDI::Get<Server>();

	// Load file
	auto file = server->getFileSystemServer().open(fs::FileCategory::NPC, filePath);
	if (file == nullptr)
		return nullptr;

	std::string header = string::trimMutate(file->readLine());
	if (header != "GRNPC001")
		return nullptr;

	auto npcNameFromFile = filePath.stem().string();
	auto name = file->readConfigLine("NAME", " "sv).value_or(npcNameFromFile.substr(3, npcNameFromFile.length() - 7));

	// Search for the ID of the NPC from the file data.
	NPCID id = 0;
	if (auto sectionId = file->readConfigLine("ID", " "sv); sectionId.has_value())
	{
		id = string::toNumber<NPCID>(sectionId.value());
		if (id < NPCID_GEN_DATABASE)
		{
			id = 0;
			log::printLine(log::server, "** NPC [{}] ID is less than {}, getting next available.", name, NPCID_GEN_DATABASE);
		}
		else if (server->m_npcIdGenerator.isIdUsed(id))
		{
			id = 0;
			log::printLine(log::server, "** NPC [{}] ID is already in use, getting next available.", name);
		}
		else server->m_npcIdGenerator.markAsUsed(id);
	}

	if (id == 0)
		id = server->m_npcIdGenerator.getAvailableId(NPCID_GEN_DATABASE);

	// Make the NPC.
	auto npc = std::make_shared<NPC>(id, NPCStorageType::DATABASE);
	npc->lastSaveTime = fs::getFileModTime(filePath);

	// Set the default warp type.
	if (server->hasNPCServer())
		npc->warpRestrictions = NPCWarpRestrictions::NOTALLOWED;

	// Set some default values.
	bool isMale = true;
	npc->visFlags = PROPID(NPCVisFlags::VISIBLE) | PROPID(NPCVisFlags::CREATED);

	const auto& updateTime = server->getServerStartTime();
	std::string script;
	std::vector<std::string> joinedClasses;

	// Parse File
	std::string line;
	std::string command;
	while (!file->finishedReading())
	{
		line = string::trimMutate(file->readLine());

		std::string_view lineView = line;
		command = string::extractLine(lineView, ' ');

		// Parse Line
		if (command == "NAME")
		{
			npc->name = lineView;
			npc->modTime[PROPID(NPCProp::NAME)] = updateTime;
		}
		else if (command == "ID")
			; // npc->id = string::toNumber<NPCID>(std::string{ lineView });
		else if (command == "TYPE")
			npc->scriptType = lineView;
		else if (command == "SCRIPTER")
		{
			npc->scripter = lineView;
			npc->modTime[PROPID(NPCProp::SCRIPTER)] = updateTime;
		}
		else if (command == "IMAGE")
		{
			npc->image = lineView;
			npc->modTime[PROPID(NPCProp::IMAGE)] = updateTime;
		}
		else if (command == "IMGPART")
		{
			auto parts = string::splitToVector(lineView, " "sv);
			if (parts.size() >= 4)
			{
				npc->imagePart.position = { string::toNumber<uint16_t>(parts[0]), string::toNumber<uint16_t>(parts[1]) };
				npc->imagePart.size = { string::toNumber<uint8_t>(parts[2]), string::toNumber<uint8_t>(parts[3]) };
				npc->modTime[PROPID(NPCProp::IMAGEPART)] = updateTime;
			}
		}
		else if (command == "STARTLEVEL")
			npc->m_initialLevel = lineView;
		else if (command == "STARTX")
			npc->m_initialCharacter.localPixelX = static_cast<int16_t>(string::toFloat(std::string{ lineView }) * 16);
		else if (command == "STARTY")
			npc->m_initialCharacter.localPixelY = static_cast<int16_t>(string::toFloat(std::string{ lineView }) * 16);
		else if (command == "STARTZ")
			npc->m_initialCharacter.localPixelZ = static_cast<int16_t>(string::toFloat(std::string{ lineView }) * 16);
		else if (command == "LEVEL")
			npc->level = lineView;
		else if (command == "X")
		{
			npc->character.localPixelX = static_cast<int16_t>(string::toFloat(std::string{ lineView }) * 16);
			npc->modTime[PROPID(NPCProp::X)] = updateTime;
			npc->modTime[PROPID(NPCProp::X2)] = updateTime;
		}
		else if (command == "Y")
		{
			npc->character.localPixelY = static_cast<int16_t>(string::toFloat(std::string{ lineView }) * 16);
			npc->modTime[PROPID(NPCProp::Y)] = updateTime;
			npc->modTime[PROPID(NPCProp::Y2)] = updateTime;
		}
		else if (command == "Z")
		{
			npc->character.localPixelZ = static_cast<int16_t>(string::toFloat(std::string{ lineView }) * 16);
			npc->modTime[PROPID(NPCProp::Z)] = updateTime;
			npc->modTime[PROPID(NPCProp::Z2)] = updateTime;
		}
		else if (command == "MAPX")
		{
			npc->character.mapX = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::GMAPLEVELX)] = updateTime;
		}
		else if (command == "MAPY")
		{
			npc->character.mapY = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::GMAPLEVELY)] = updateTime;
		}
		else if (command == "NICK")
		{
			npc->character.nickName = lineView;
			npc->modTime[PROPID(NPCProp::NICKNAME)] = updateTime;
		}
		else if (command == "ANI")
		{
			npc->character.gani = lineView;
			npc->modTime[PROPID(NPCProp::GANI)] = updateTime;
		}
		else if (command == "HP")
		{
			npc->character.hitpointsInHalves = static_cast<uint8_t>(2 * string::toFloat(std::string{ lineView }));
			npc->modTime[PROPID(NPCProp::POWER)] = updateTime;
		}
		else if (command == "GRALATS")
		{
			npc->character.gralats = string::toNumber<uint32_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::RUPEES)] = updateTime;
		}
		else if (command == "ARROWS")
		{
			npc->character.arrows = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::ARROWS)] = updateTime;
		}
		else if (command == "BOMBS")
		{
			npc->character.bombs = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::BOMBS)] = updateTime;
		}
		else if (command == "GLOVEP")
		{
			npc->character.glovePower = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::GLOVEPOWER)] = updateTime;
		}
		else if (command == "SWORDP")
		{
			npc->character.swordPower = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::SWORDIMAGE)] = updateTime;
		}
		else if (command == "SHIELDP")
		{
			npc->character.shieldPower = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::SHIELDIMAGE)] = updateTime;
		}
		else if (command == "BOWP")
		{
			npc->character.bowPower = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::GANI)] = updateTime;
		}
		else if (command == "BOW")
		{
			npc->character.bowImage = lineView;
			npc->modTime[PROPID(NPCProp::GANI)] = updateTime;
		}
		else if (command == "HEAD")
		{
			npc->character.headImage = lineView;
			npc->modTime[PROPID(NPCProp::HEADIMAGE)] = updateTime;
		}
		else if (command == "BODY")
		{
			npc->character.bodyImage = lineView;
			npc->modTime[PROPID(NPCProp::BODYIMAGE)] = updateTime;
		}
		else if (command == "SWORD")
		{
			npc->character.swordImage = lineView;
			npc->modTime[PROPID(NPCProp::SWORDIMAGE)] = updateTime;
		}
		else if (command == "SHIELD")
		{
			npc->character.shieldImage = lineView;
			npc->modTime[PROPID(NPCProp::SHIELDIMAGE)] = updateTime;
		}
		else if (command == "HORSE")
		{
			npc->character.horseImage = lineView;
			npc->modTime[PROPID(NPCProp::HORSEIMAGE)] = updateTime;
		}
		else if (command == "COLORS")
		{
			auto tokens = string::splitToVector(lineView, ","sv);
			for (size_t idx = 0; idx < std::min(tokens.size(), (size_t)5); idx++)
				npc->character.colors[idx] = string::toNumber<uint8_t>(tokens[idx]);
			npc->modTime[PROPID(NPCProp::COLORS)] = updateTime;
		}
		else if (command == "SPRITE")
		{
			auto sprite = string::toNumber<uint8_t>(std::string{ lineView });
			npc->character.sprite = sprite >> 2;
			npc->character.direction = sprite & 0b11;
			npc->modTime[PROPID(NPCProp::SPRITE)] = updateTime;
		}
		else if (command == "AP")
		{
			npc->character.ap = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::ALIGNMENT)] = updateTime;
		}
		else if (command == "TIMEOUT")
		{
			npc->timeout = std::chrono::milliseconds(string::toNumber<int>(std::string{ lineView }) * 20);
		}
		else if (command == "LAYER")
		{
			auto layer = string::toNumber<uint8_t>(std::string{ lineView });
			if (layer == 0)
				npc->visFlags |= PROPID(NPCVisFlags::DRAWUNDERPLAYER);
			if (layer == 2)
				npc->visFlags |= PROPID(NPCVisFlags::DRAWOVERPLAYER);
			npc->modTime[PROPID(NPCProp::VISFLAGS)] = updateTime;
		}
		else if (command == "SHAPETYPE")
		{
			// Only shape type 1 is supported, but we just look at the dimension of the shape data.
		}
		else if (command == "SHAPE")
		{
			std::get<0>(npc->shape.data) = string::toNumber<uint16_t>(string::extractLine(lineView, ' '));
			std::get<1>(npc->shape.data) = string::toNumber<uint16_t>(std::string{ string::trim(lineView) });
		}
		else if (command == "DONTBLOCK")
		{
			npc->blockFlags = string::toNumber<uint8_t>(std::string{ lineView });
			npc->modTime[PROPID(NPCProp::BLOCKFLAGS)] = updateTime;
		}
		else if (command == "NOPLAYERONWALL")
		{
			npc->noPlayerOnWall = string::toNumber<uint8_t>(std::string{ lineView }) != 0;
		}
		else if (command == "SAVEARR")
		{
			auto tokens = string::splitToVector(lineView, ","sv);
			for (size_t idx = 0; idx < std::min(tokens.size(), npc->saves.size()); idx++)
			{
				npc->saves[idx] = string::toNumber<uint8_t>(tokens[idx]);
				npc->modTime[PROPID(NPCProp::SAVE0) + idx] = updateTime;
			}
		}
		else if (command == "CANWARP")
		{
			npc->warpRestrictions = string::toNumber<uint8_t>(std::string{ lineView }) != 0 ? NPCWarpRestrictions::ALLOWED : npc->warpRestrictions;
		}
		else if (command == "CANWARP2")
		{
			npc->warpRestrictions = string::toNumber<uint8_t>(std::string{ lineView }) != 0 ? NPCWarpRestrictions::ONLYOVERWORLD : npc->warpRestrictions;
		}

		// Official variables for these are unknown.
		else if (command == "CANCARRY")
		{
			auto value = string::toNumber<uint8_t>(std::string{ lineView });
			if (value != 0)
				npc->blockFlags |= PROPID(NPCBlockFlags::CANBECARRIED);
		}
		else if (command == "CANPULL")
		{
			auto value = string::toNumber<uint8_t>(std::string{ lineView });
			if (value != 0)
				npc->blockFlags |= PROPID(NPCBlockFlags::CANBEPULLED);
		}
		else if (command == "CANPUSH")
		{
			auto value = string::toNumber<uint8_t>(std::string{ lineView });
			if (value != 0)
				npc->blockFlags |= PROPID(NPCBlockFlags::CANBEPUSHED);
		}
		else if (command == "VISIBLE")
		{
			auto value = string::toNumber<uint8_t>(std::string{ lineView });
			if (value == 0)
				npc->visFlags &= ~PROPID(NPCVisFlags::VISIBLE);
		}
		else if (command == "TIMERSHOW")
		{
			auto value = string::toNumber<uint8_t>(std::string{ lineView });
			if (value != 0)
				npc->visFlags |= PROPID(NPCVisFlags::TIMERSHOW);
		}
		else if (command == "MALE")
		{
			auto value = string::toNumber<uint8_t>(std::string{ lineView });
			if (value == 0)
				isMale = false;
		}
		//---

		else if (command == "FLAG")
		{
			std::string flagName = string::trimMutate(string::extractLine(lineView, '='));
			std::string flagValue = std::string{ string::trim(lineView) };
			npc->scripting.variables.add(GameValue::deserialize(flagName, flagValue));
		}
		else if (command.substr(0, 4) == "ATTR")
		{
			auto attrIdStr = command.substr(5);
			int attrId = string::toNumber<uint8_t>(attrIdStr);
			if (attrId > 0 && attrId < 30)
			{
				int idx = attrId - 1;
				npc->character.ganiAttributes[idx] = lineView;
				npc->modTime[attrPackets[idx]] = updateTime;
			}
		}
		else if (command == "JOINEDCLASSES")
		{
			joinedClasses = string::fromCSV(lineView);
		}
		else if (command == "NPCSCRIPT")
		{
			do {
				line = string::trimNewlines(file->readLine());
				if (string::trim(line) == "NPCSCRIPTEND")
					break;

				script.append(line).append(1, '\n');
			} while (!file->finishedReading());

			npc->modTime[PROPID(NPCProp::SCRIPT)] = updateTime;
		}
	}

	// If the NPC is a character, force the shape to be 48x48.
	// Also, set the gender.
	if (npc->isCharacter())
	{
		npc->shape = { 48, 48 };
		if (isMale)
			npc->visFlags |= PROPID(NPCVisFlags::MALE);
	}

	// Set the script.
	npc->setScript(script);

	// Join the classes.
	for (const auto& className : joinedClasses)
	{
		if (!className.empty())
			npc->joinClass(className);
	}

	// Check if the level is a gmap.
	// If it is, we need to determine the actual level.
	string::trimMutate(npc->level);
	if (npc->level.ends_with(".gmap"))
	{
		if (auto foundMap = server->findMap(npc->level); foundMap != nullptr)
		{
			if (auto mapLevel = foundMap->getLevelAt(npc->character.mapX, npc->character.mapY); mapLevel != nullptr)
				npc->level = mapLevel->levelName;
		}
	}

	// Add the NPC to the server.
	server->addNPC(npc, false);

	// Add it to the level.
	if (auto level = server->stubOrGetLevel(npc->level); level != nullptr)
	{
		level->addNPC(npc);
		npc->m_currentLevel = level;
	}

	return npc;
}

bool FlatFileNPCLoader::saveNPC(NPCPtr npc) noexcept
{
	if (npc->storageType != NPCStorageType::DATABASE)
		return false;

	static const char* NL = "\r\n";
	CString folder{ "npcs" };
	CString fileName = CString() << "npc" << npc->name << ".txt";
	CString fileData = CString("GRNPC001") << NL;

	auto writeProp = [&](NPCProp prop, std::string_view key, std::string_view value)
	{
		if (npc->modTime[PROPID(prop)] != clock::time_point::min())
			fileData << key << " " << value << NL;
	};

	auto level = npc->getLevel();
	auto server = BabyDI::Get<Server>();

	int layer = 0;
	if (npc->visFlags & PROPID(NPCVisFlags::DRAWUNDERPLAYER))
		layer = -1;
	else if (npc->visFlags & PROPID(NPCVisFlags::DRAWOVERPLAYER))
		layer = 1;

	fileData << "NAME " << npc->name << NL;
	fileData << "ID " << CString(npc->id) << NL;
	fileData << "TYPE " << npc->scriptType << NL;
	fileData << "SCRIPTER " << npc->scripter << NL;
	fileData << "IMAGE " << npc->image << NL;
	if (npc->imagePart.size.width() > 0 && npc->imagePart.size.height() > 0)
	{
		fileData << "IMGPART "
			<< CString(npc->imagePart.position.x()) << " " << CString(npc->imagePart.position.y()) << " "
			<< CString(npc->imagePart.size.width()) << " " << CString(npc->imagePart.size.height()) << NL;
	}
	fileData << "STARTLEVEL " << npc->m_initialLevel << NL;
	fileData << "STARTX " << CString((float)npc->m_initialCharacter.localPixelX / 16.0f) << NL;
	fileData << "STARTY " << CString((float)npc->m_initialCharacter.localPixelY / 16.0f) << NL;
	fileData << "STARTZ " << CString((float)npc->m_initialCharacter.localPixelZ / 16.0f) << NL;
	if (level)
	{
		fileData << "LEVEL " << npc->getLevelName() << NL;
		fileData << "X " << CString((float)npc->character.localPixelX / 16.0f) << NL;
		fileData << "Y " << CString((float)npc->character.localPixelY / 16.0f) << NL;
		fileData << "Z " << CString((float)npc->character.localPixelZ / 16.0f) << NL;

		if (level->isOnGmap())
		{
			fileData << "MAPX " << CString(npc->character.mapX) << NL;
			fileData << "MAPY " << CString(npc->character.mapY) << NL;
		}
	}

	writeProp(NPCProp::NICKNAME, "NICK", npc->character.nickName);

	if (server->Generation != ServerGeneration::ORIGINAL)
		writeProp(NPCProp::GANI, "ANI", npc->character.gani);

	writeProp(NPCProp::POWER, "HP", std::format("{:2f}", npc->character.hitpointsInHalves / 2.0f));
	writeProp(NPCProp::RUPEES, "GRALATS", std::to_string(npc->character.gralats));
	writeProp(NPCProp::ARROWS, "ARROWS", std::to_string(npc->character.arrows));
	writeProp(NPCProp::BOMBS, "BOMBS", std::to_string(npc->character.bombs));
	writeProp(NPCProp::GLOVEPOWER, "GLOVEP", std::to_string(npc->character.glovePower));
	writeProp(NPCProp::SWORDIMAGE, "SWORDP", std::to_string(npc->character.swordPower));
	writeProp(NPCProp::SHIELDIMAGE, "SHIELDP", std::to_string(npc->character.shieldPower));

	if (server->Generation == ServerGeneration::ORIGINAL)
	{
		writeProp(NPCProp::GANI, "BOWP", std::to_string(npc->character.bowPower));
		writeProp(NPCProp::GANI, "BOW", npc->character.bowImage);
	}

	writeProp(NPCProp::HEADIMAGE, "HEAD", npc->character.headImage);
	writeProp(NPCProp::BODYIMAGE, "BODY", npc->character.bodyImage);
	writeProp(NPCProp::SWORDIMAGE, "SWORD", npc->character.swordImage);
	writeProp(NPCProp::SHIELDIMAGE, "SHIELD", npc->character.shieldImage);
	writeProp(NPCProp::HORSEIMAGE, "HORSE", npc->character.horseImage);
	writeProp(NPCProp::COLORS, "COLORS", std::format("{},{},{},{},{}", npc->character.colors[0], npc->character.colors[1], npc->character.colors[2], npc->character.colors[3], npc->character.colors[4]));
	writeProp(NPCProp::SPRITE, "SPRITE", std::to_string(npc->character.sprite << 2 | npc->character.direction));
	writeProp(NPCProp::ALIGNMENT, "AP", std::to_string(npc->character.ap));

	if (npc->timeout != 0ms)
		fileData << "TIMEOUT " << std::to_string(static_cast<int>(npc->timeout.count() * 0.05)) << NL;

	if (layer != 0)
		fileData << "LAYER " << std::to_string(layer + 1) << NL;

	if (npc->shape.width() != 0 || npc->shape.height() != 0)
	{
		fileData << "SHAPETYPE " << (npc->shape.width() != 0 && npc->shape.height() != 0 ? "1" : "0") << NL;
		fileData << "SHAPE " << std::format("{} {}", npc->shape.width(), npc->shape.height()) << NL;
	}

	if (npc->blockFlags & PROPID(NPCBlockFlags::NOBLOCK))
		fileData << "DONTBLOCK 1" << NL;
	if (npc->noPlayerOnWall)
		fileData << "NOPLAYERONWALL 1" << NL;
	if (npc->warpRestrictions == NPCWarpRestrictions::NOTALLOWED)
		fileData << "CANWARP" << NL;
	if (npc->warpRestrictions == NPCWarpRestrictions::ONLYOVERWORLD)
		fileData << "CANWARP2" << NL;

	// Official variables for these are unknown.
	if (npc->blockFlags & PROPID(NPCBlockFlags::CANBECARRIED))
		fileData << "CANCARRY 1" << NL;
	if (npc->blockFlags & PROPID(NPCBlockFlags::CANBEPULLED))
		fileData << "CANPULL 1" << NL;
	if (npc->blockFlags & PROPID(NPCBlockFlags::CANBEPUSHED))
		fileData << "CANPUSH 1" << NL;
	if ((npc->visFlags & PROPID(NPCVisFlags::VISIBLE)) == 0)
		fileData << "VISIBLE 0" << NL;
	if ((npc->visFlags & PROPID(NPCVisFlags::TIMERSHOW)) != 0)
		fileData << "TIMERSHOW 1" << NL;
	if (npc->isCharacter() && (npc->visFlags & PROPID(NPCVisFlags::MALE)) == 0)
		fileData << "MALE 0" << NL;
	// ---

	if (!std::ranges::empty(NPCSaveProps | std::views::filter([&npc](NPCProp prop) { return npc->modTime[PROPID(prop)] != clock::time_point::min(); })))
		fileData << "SAVEARR " << string::toCSV(npc->saves | std::views::transform([](uint8_t x) { return std::to_string(x); })) << NL;

	for (int i = 0; i < 30; i++)
	{
		NPCProp prop = static_cast<NPCProp>(NPCGaniAttrPackets[i]);
		if (!npc->character.ganiAttributes[i].empty())
			writeProp(prop, std::format("ATTR{}", i + 1), npc->character.ganiAttributes[i]);
	}

	for (auto& [flag, value] : npc->scripting.variables.store | variables::no_temporary)
	{
		// Ignore flags.
		if (value->has<bool>() && !value->has<std::string>()) continue;

		// Serialize the variable entirely.
		if (server->Generation == ServerGeneration::MODERN)
		{
			auto var = npc->scripting.variables.serializeModern(flag);
			if (var.has_value())
				fileData << "FLAG " << var.value() << NL;
		}
		else
		{
			for (const auto& serialized : npc->scripting.variables.serialize(flag))
				fileData << serialized << NL;
		}
	}

	if (!npc->m_joinedClasses.empty())
	{
		fileData << "JOINEDCLASSES " << npc->getJoinedClassesList() << NL;
	}

	fileData << "NPCSCRIPT" << NL << CString(npc->getScript().getOriginalSource()).replaceAll("\n", NL);
	if (fileData[fileData.length() - 1] != '\n')
		fileData << NL;
	fileData << "NPCSCRIPTEND" << NL;
	fileData.save(folder << "/" << fileName);

	// If the NPC exists on the filesystem, refresh its mod time to avoid any modification events.
	if (auto info = server->getFileSystemServer().info(fs::FileCategory::NPC, fileName.toStringView()); info != nullptr)
		info->refreshModTime();

	npc->lastSaveTime = fs::getFileModTime(folder.toString());

	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
