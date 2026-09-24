#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <BabyDI.h>

#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <loader/flatfile/FlatFileNPCLoader.h>
#include <object/NPC.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

static constexpr std::array<uint8_t, 30> attrPackets = {36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73};

///////////////////////////////////////////////////////////////////////////////

NPCPtr FlatFileNPCLoader::loadNPC(std::string_view npcName) noexcept
{
	const auto server = BabyDI::Get<Server>();
	const auto fileInfo = server->getFileSystemServer().info(fs::FileCategory::NPC, std::format("npc{}.txt", npcName));
	if (fileInfo == nullptr)
		return nullptr;

	return loadNPC(fileInfo->file);
}

NPCPtr FlatFileNPCLoader::loadNPC(const std::filesystem::path& filePath) noexcept
{
	const auto server = BabyDI::Get<Server>();

	// Load file
	const auto file = server->getFileSystemServer().open(fs::FileCategory::NPC, filePath);
	if (file == nullptr)
		return nullptr;

	if (const auto header = string::trimMutate(file->readLine()); header != "GRNPC001")
		return nullptr;

	const auto npcNameFromFile = filePath.stem().string();
	const auto name = file->readConfigLine("NAME", " "sv).value_or(npcNameFromFile.substr(3, npcNameFromFile.length() - 7));

	// Search for the ID of the NPC from the file data.
	NPCID id = 0;
	if (const auto sectionId = file->readConfigLine("ID", " "sv); sectionId.has_value())
	{
		id = string::toNumber<NPCID>(sectionId.value());

		if (id < NPCID_GEN_MANUAL)
		{
			id = 0;
			log::printLine(log::server, "** NPC [{}] ID is less than {}, getting next available.", name, NPCID_GEN_MANUAL);
		}
		else if (server->m_npcIdGenerator.isIdUsed(id))
		{
			id = 0;
			log::printLine(log::server, "** NPC [{}] ID is already in use, getting next available.", name);
		}
		else
		{
			if (id < NPCID_GEN_DELETEABLE)
				log::printLine(log::server, "!! [WARNING] NPC [{}] ID is in the client non-deletable range, consider using an ID over {}.", name, NPCID_GEN_DELETEABLE);

			server->m_npcIdGenerator.markAsUsed(id);
		}
	}

	if (id == 0)
		id = server->m_npcIdGenerator.getAvailableId(NPCID_GEN_DATABASE);

	// Make the NPC.
	auto npc = std::make_shared<NPC>(id, NPCStorageType::DATABASE);

	// Load the NPC.
	loadNPC(*file, npc, server->getServerStartTime());

	// Save the loaded warp restriction.
	// Adding the NPC to the server will change this.
	const auto warpRestriction = npc->warpRestrictions;

	// Add the NPC to the server.
	server->addNPC(npc, false);

	// Set the warp restriction (do this after adding to the server since that will overwrite the restriction).
	npc->warpRestrictions = warpRestriction;

	// Check if we need to rename the file.
	const auto expectedFileName = fs::getHTMLEscapedFileName(std::format("npc{}.txt", npc->name)).string();
	const auto currentFileName = fs::getANSIFileName(filePath);
	if (expectedFileName != currentFileName)
	{
		if (const auto fileData = server->getFileSystemServer().infoi(fs::FileCategory::NPC, currentFileName); fileData != nullptr)
		{
			auto indent = log::server.indent();
			if (server->getFileSystemServer().rename(*fileData, expectedFileName))
				log::printLine(log::server, "Renamed NPC file [{}] to [{}]", currentFileName, expectedFileName);
			else
				log::printLine(log::server, "** Failed to rename NPC file [{}] to [{}]", currentFileName, expectedFileName);
		}
	}

	return npc;
}

void FlatFileNPCLoader::loadNPC(fs::File& file, NPCPtr& npc, const clock::time_point& updateTime) noexcept
{
	file.setStreamPosition(0);
	if (const auto header = string::trimMutate(file.readLine()); header != "GRNPC001")
		return;

	// Set some default values.
	bool isMale = true;
	npc->visFlags = PROPID(NPCVisFlags::VISIBLE) | PROPID(NPCVisFlags::CREATED);

	// Record if we saw a map X/Y entry.
	// We need to properly handle situations where the map position is removed.
	bool hasMapXPosition = false;
	bool hasMapYPosition = false;

	std::string script;
	std::vector<std::string> joinedClasses;

	// Parse File
	bool scriptUpdated = false;
	std::string line;
	std::string command;
	std::string strval;
	while (!file.finishedReading())
	{
		line = string::trimMutate(file.readLine());

		std::string_view lineView = line;
		command = string::extractLine(lineView, ' ');

		// Parse Line
		if (command == "NAME")
		{
			if (npc->name != lineView)
			{
				npc->name = lineView;
				npc->modTime[PROPID(NPCProp::NAME)] = updateTime;
			}
		}
		else if (command == "ID")
			; // npc->id = string::toNumber<NPCID>(std::string{ lineView });
		else if (command == "TYPE")
			npc->scriptType = lineView;
		else if (command == "SCRIPTER")
		{
			if (npc->scripter != lineView)
			{
				npc->scripter = lineView;
				npc->modTime[PROPID(NPCProp::SCRIPTER)] = updateTime;
			}
		}
		else if (command == "IMAGE")
		{
			if (npc->image != lineView)
			{
				npc->image = lineView;
				npc->modTime[PROPID(NPCProp::IMAGE)] = updateTime;
			}
		}
		else if (command == "IMGPART")
		{
			const auto parts = string::splitToVectorView(lineView, " "sv);
			if (parts.size() >= 4)
			{
				Rectangle<uint16_t, uint8_t> imagePart;
				imagePart.position = {string::toNumber<uint16_t>(parts[0]), string::toNumber<uint16_t>(parts[1])};
				imagePart.size = {string::toNumber<uint8_t>(parts[2]), string::toNumber<uint8_t>(parts[3])};
				if (npc->imagePart.position != imagePart.position || npc->imagePart.size != imagePart.size)
				{
					npc->imagePart = imagePart;
					npc->modTime[PROPID(NPCProp::IMAGEPART)] = updateTime;
				}
			}
		}
		else if (command == "STARTLEVEL")
			npc->m_initialLevel = lineView;
		else if (command == "STARTX")
			npc->m_initialCharacter.localPixelX = static_cast<int16_t>(string::toFloat(lineView) * 16);
		else if (command == "STARTY")
			npc->m_initialCharacter.localPixelY = static_cast<int16_t>(string::toFloat(lineView) * 16);
		else if (command == "STARTZ")
		{
			npc->m_initialCharacter.localPixelZ = static_cast<int16_t>(string::toFloat(lineView) * 16);
			if (npc->m_initialCharacter.localPixelZ.has_value() && (npc->m_initialCharacter.localPixelZ.value() < Character::ValidZRangePixels[0] || npc->m_initialCharacter.localPixelZ.value() > Character::ValidZRangePixels[1]))
				npc->m_initialCharacter.localPixelZ.reset();
		}
		else if (command == "STARTMAPX")
			npc->m_initialCharacter.mapX = string::toNumber<uint8_t>(lineView);
		else if (command == "STARTMAPY")
			npc->m_initialCharacter.mapY = string::toNumber<uint8_t>(lineView);
		else if (command == "LEVEL")
			npc->level = lineView;
		else if (command == "GROUPNAME")
			npc->groupName = lineView;
		else if (command == "X")
		{
			const auto val = static_cast<int16_t>(string::toFloat(lineView) * 16);
			if (npc->character.localPixelX != val)
			{
				npc->character.localPixelX = val;
				npc->modTime[PROPID(NPCProp::X)] = updateTime;
				npc->modTime[PROPID(NPCProp::X2)] = updateTime;
			}
		}
		else if (command == "Y")
		{
			const auto val = static_cast<int16_t>(string::toFloat(lineView) * 16);
			if (npc->character.localPixelY != val)
			{
				npc->character.localPixelY = val;
				npc->modTime[PROPID(NPCProp::Y)] = updateTime;
				npc->modTime[PROPID(NPCProp::Y2)] = updateTime;
			}
		}
		else if (command == "Z")
		{
			const auto val = static_cast<int16_t>(string::toFloat(lineView) * 16);
			if (npc->character.localPixelZ != val)
			{
				npc->character.localPixelZ = val;
				if (npc->character.localPixelZ.has_value() && (npc->character.localPixelZ.value() < Character::ValidZRangePixels[0] || npc->character.localPixelZ.value() > Character::ValidZRangePixels[1]))
					npc->character.localPixelZ.reset();

				npc->modTime[PROPID(NPCProp::Z)] = updateTime;
				npc->modTime[PROPID(NPCProp::Z2)] = updateTime;
			}
		}
		else if (command == "MAPX")
		{
			hasMapXPosition = true;
			const auto val = string::toNumber<uint8_t>(lineView);
			if (npc->character.mapX != val)
			{
				npc->character.mapX = val;
				npc->modTime[PROPID(NPCProp::GMAPLEVELX)] = updateTime;
			}
		}
		else if (command == "MAPY")
		{
			hasMapYPosition = true;
			const auto val = string::toNumber<uint8_t>(lineView);
			if (npc->character.mapY != val)
			{
				npc->character.mapY = val;
				npc->modTime[PROPID(NPCProp::GMAPLEVELY)] = updateTime;
			}
		}
		else if (command == "NICK")
		{
			if (npc->character.nickName != lineView)
			{
				npc->character.nickName = lineView;
				npc->modTime[PROPID(NPCProp::NICKNAME)] = updateTime;
			}
		}
		else if (command == "ANI")
		{
			if (npc->character.gani != lineView)
			{
				npc->character.gani = lineView;
				npc->modTime[PROPID(NPCProp::GANI)] = updateTime;
			}
		}
		else if (command == "HP")
		{
			const auto val = static_cast<uint8_t>(2 * string::toFloat(lineView));
			if (npc->character.hitpointsInHalves != val)
			{
				npc->character.hitpointsInHalves = val;
				npc->modTime[PROPID(NPCProp::HALFHEARTS)] = updateTime;
			}
		}
		else if (command == "GRALATS")
		{
			const auto val = string::toNumber<uint32_t>(lineView);
			if (npc->character.gralats != val)
			{
				npc->character.gralats = val;
				npc->modTime[PROPID(NPCProp::GRALATS)] = updateTime;
			}
		}
		else if (command == "ARROWS")
		{
			const auto val = string::toNumber<uint8_t>(lineView);
			if (npc->character.arrows != val)
			{
				npc->character.arrows = val;
				npc->modTime[PROPID(NPCProp::ARROWS)] = updateTime;
			}
		}
		else if (command == "BOMBS")
		{
			const auto val = string::toNumber<uint8_t>(lineView);
			if (npc->character.bombs != val)
			{
				npc->character.bombs = val;
				npc->modTime[PROPID(NPCProp::BOMBS)] = updateTime;
			}
		}
		else if (command == "GLOVEP")
		{
			const auto val = string::toNumber<uint8_t>(lineView);
			if (npc->character.glovePower != val)
			{
				npc->character.glovePower = val;
				npc->modTime[PROPID(NPCProp::GLOVEPOWER)] = updateTime;
			}
		}
		else if (command == "SWORDP")
		{
			const auto val = string::toNumber<int8_t>(lineView);
			if (npc->character.swordPower != val)
			{
				npc->character.swordPower = val;
				npc->modTime[PROPID(NPCProp::SWORDIMAGE)] = updateTime;
			}
		}
		else if (command == "SHIELDP")
		{
			const auto val = string::toNumber<uint8_t>(lineView);
			if (npc->character.shieldPower != val)
			{
				npc->character.shieldPower = val;
				npc->modTime[PROPID(NPCProp::SHIELDIMAGE)] = updateTime;
			}
		}
		else if (command == "BOWP")
		{
			const auto val = string::toNumber<uint8_t>(lineView);
			if (npc->character.bowPower != val)
			{
				npc->character.bowPower = val;
				npc->modTime[PROPID(NPCProp::GANI)] = updateTime;
			}
		}
		else if (command == "BOW")
		{
			if (npc->character.bowImage != lineView)
			{
				npc->character.bowImage = lineView;
				npc->modTime[PROPID(NPCProp::GANI)] = updateTime;
			}
		}
		else if (command == "HEAD")
		{
			if (npc->character.headImage != lineView)
			{
				npc->character.headImage = lineView;
				npc->modTime[PROPID(NPCProp::HEADIMAGE)] = updateTime;
			}
		}
		else if (command == "BODY")
		{
			if (npc->character.bodyImage != lineView)
			{
				npc->character.bodyImage = lineView;
				npc->modTime[PROPID(NPCProp::BODYIMAGE)] = updateTime;
			}
		}
		else if (command == "SWORD")
		{
			if (npc->character.swordImage != lineView)
			{
				npc->character.swordImage = lineView;
				npc->modTime[PROPID(NPCProp::SWORDIMAGE)] = updateTime;
			}
		}
		else if (command == "SHIELD")
		{
			if (npc->character.shieldImage != lineView)
			{
				npc->character.shieldImage = lineView;
				npc->modTime[PROPID(NPCProp::SHIELDIMAGE)] = updateTime;
			}
		}
		else if (command == "HORSE")
		{
			if (npc->character.horseImage != lineView)
			{
				npc->character.horseImage = lineView;
				npc->modTime[PROPID(NPCProp::HORSEIMAGE)] = updateTime;
			}
		}
		else if (command == "COLORS")
		{
			bool updated = false;
			auto tokens = string::splitToVectorView(lineView, ","sv);
			for (size_t idx = 0; idx < std::min(tokens.size(), 8ZU); idx++)
			{
				const auto val = string::toNumber<uint8_t>(tokens[idx]);
				if (npc->character.colors[idx] != val)
				{
					npc->character.colors[idx] = val;
					updated = true;
				}
			}
			if (updated)
				npc->modTime[PROPID(NPCProp::COLORS)] = updateTime;
		}
		else if (command == "SPRITE")
		{
			const auto spritedir = string::toNumber<uint8_t>(lineView);
			const uint8_t sprite = spritedir >> 2;
			const uint8_t dir = sprite & 0b11;
			if (npc->character.sprite != sprite || npc->character.direction != dir)
			{
				npc->character.sprite = sprite;
				npc->character.direction = dir;
				npc->modTime[PROPID(NPCProp::SPRITE)] = updateTime;
			}
		}
		else if (command == "AP")
		{
			const auto val = string::toNumber<uint8_t>(lineView);
			if (npc->character.ap != val)
			{
				npc->character.ap = val;
				npc->modTime[PROPID(NPCProp::ALIGNMENT)] = updateTime;
			}
		}
		else if (command == "TIMEOUT")
		{
			npc->timeout = std::chrono::duration_cast<std::chrono::milliseconds>(duration_seconds_double(string::toDouble(lineView)));
		}
		else if (command == "LAYER")
		{
			auto visFlags = npc->visFlags & ~(PROPID(NPCVisFlags::DRAWUNDERPLAYER) | PROPID(NPCVisFlags::DRAWOVERPLAYER));
			switch (string::toNumber<int8_t>(lineView))
			{
				case -1:
					visFlags |= PROPID(NPCVisFlags::DRAWUNDERPLAYER);
					break;
				case 1:
					visFlags |= PROPID(NPCVisFlags::DRAWOVERPLAYER);
					break;
				default:;
			}

			if (visFlags != npc->visFlags)
			{
				npc->visFlags = visFlags;
				npc->modTime[PROPID(NPCProp::VISFLAGS)] = updateTime;
			}
		}
		else if (command == "SHAPETYPE")
		{
			// Only shape type 1 is supported, but we just look at the dimension of the shape data.
		}
		else if (command == "SHAPE")
		{
			std::get<0>(npc->shape.data) = string::toNumber<uint16_t>(string::extractLine(lineView, ' '));
			std::get<1>(npc->shape.data) = string::toNumber<uint16_t>(std::string{string::trim(lineView)});
		}
		else if (command == "DONTBLOCK")
		{
			const auto val = string::toNumber<uint8_t>(lineView);
			if (npc->blockFlags != val)
			{
				npc->blockFlags = val;
				npc->modTime[PROPID(NPCProp::BLOCKFLAGS)] = updateTime;
			}
		}
		else if (command == "NOPLAYERONWALL")
		{
			npc->noPlayerOnWall = string::toNumber<uint8_t>(lineView) != 0;
		}
		else if (command == "SAVEARR")
		{
			auto tokens = string::splitToVectorView(lineView, ","sv);
			for (size_t idx = 0; idx < std::min(tokens.size(), npc->saves.size()); idx++)
			{
				const auto val = string::toNumber<uint8_t>(tokens[idx]);
				if (npc->saves[idx] != val)
				{
					npc->saves[idx] = val;
					npc->modTime[PROPID(NPCProp::SAVE0) + idx] = updateTime;
				}
			}
		}
		else if (command == "CANWARP")
		{
			npc->warpRestrictions = NPCWarpRestrictions::ALLOWED;
		}
		else if (command == "CANWARP2")
		{
			npc->warpRestrictions = NPCWarpRestrictions::ONLYOVERWORLD;
		}

		// Official variables for these are unknown.
		else if (command == "CANCARRY")
		{
			const auto block = npc->blockFlags | PROPID(NPCBlockFlags::CANBECARRIED);
			if (npc->blockFlags != block)
			{
				npc->blockFlags = block;
				npc->modTime[PROPID(NPCProp::BLOCKFLAGS)] = updateTime;
			}
		}
		else if (command == "CANPULL")
		{
			const auto block = npc->blockFlags | PROPID(NPCBlockFlags::CANBEPULLED);
			if (npc->blockFlags != block)
			{
				npc->blockFlags = block;
				npc->modTime[PROPID(NPCProp::BLOCKFLAGS)] = updateTime;
			}
		}
		else if (command == "CANPUSH")
		{
			const auto block = npc->blockFlags | PROPID(NPCBlockFlags::CANBEPUSHED);
			if (npc->blockFlags != block)
			{
				npc->blockFlags = block;
				npc->modTime[PROPID(NPCProp::BLOCKFLAGS)] = updateTime;
			}
		}
		else if (command == "VISIBLE")
		{
			if (const auto value = string::toNumber<uint8_t>(lineView); value == 0)
			{
				const auto visFlags = npc->visFlags & ~PROPID(NPCVisFlags::VISIBLE);
				if (npc->visFlags != visFlags)
				{
					npc->visFlags = visFlags;
					npc->modTime[PROPID(NPCProp::VISFLAGS)] = updateTime;
				}
			}
		}
		else if (command == "TIMERSHOW")
		{
			const auto visFlags = npc->visFlags | PROPID(NPCVisFlags::TIMERSHOW);
			if (npc->visFlags != visFlags)
			{
				npc->visFlags = visFlags;
				npc->modTime[PROPID(NPCProp::VISFLAGS)] = updateTime;
			}
		}
		else if (command == "MALE")
		{
			if (const auto value = string::toNumber<uint8_t>(lineView); value == 0)
				isMale = false;
		}
		//---

		else if (command == "FLAG")
		{
			std::string flagName = string::trimMutate(string::extractLine(lineView, '='));
			std::string flagValue{string::trim(lineView)};
			npc->scripting.variables.add(GameVariable::deserialize(flagName, flagValue));
		}
		else if (command.substr(0, 4) == "ATTR")
		{
			auto attrIdStr = command.substr(5);
			int attrId = string::toNumber<uint8_t>(attrIdStr);
			if (attrId > 0 && attrId < 30)
			{
				const int idx = attrId - 1;
				if (npc->character.ganiAttributes[idx] != lineView)
				{
					npc->character.ganiAttributes[idx] = lineView;
					npc->modTime[attrPackets[idx]] = updateTime;
				}
			}
		}
		else if (command == "JOINEDCLASSES")
		{
			joinedClasses = string::fromCSV(lineView);
		}
		else if (command == "NPCSCRIPT")
		{
			do
			{
				line = string::trimNewlines(file.readLine());
				if (string::trim(line) == "NPCSCRIPTEND")
					break;

				script.append(line).append(1, '\n');
			}
			while (!file.finishedReading());

			scriptUpdated = (script != npc->getScript().getOriginalSource());
		}
	}
	file.close();

	// If the NPC is a character, always send the colors.
	if (npc->isCharacter())
	{
		npc->modTime[PROPID(NPCProp::COLORS)] = updateTime;
	}

	// If the NPC is a character, set the gender prop.
	// Also, set the gender.
	if (npc->isCharacter() && isMale)
	{
		const auto visFlags = npc->visFlags | PROPID(NPCVisFlags::MALE);
		if (npc->visFlags != visFlags)
		{
			npc->visFlags = visFlags;
			npc->modTime[PROPID(NPCProp::VISFLAGS)] = updateTime;
		}
	}

	// If the NPC has no image, make it invisible.
	if (!npc->hasImage() && !npc->hasShape())
	{
		const auto visFlags = npc->visFlags & ~PROPID(NPCVisFlags::VISIBLE);
		if (npc->visFlags != visFlags)
		{
			npc->visFlags = visFlags;
			npc->modTime[PROPID(NPCProp::VISFLAGS)] = updateTime;
		}
	}

	// If the NPC did not set a map position, and our mapX/Y property is set, reset it back to 0.
	if (!hasMapXPosition && npc->character.mapX != 0)
	{
		npc->character.mapX = 0;
		npc->modTime[PROPID(NPCProp::GMAPLEVELX)] = updateTime;
	}
	if (!hasMapYPosition && npc->character.mapY != 0)
	{
		npc->character.mapY = 0;
		npc->modTime[PROPID(NPCProp::GMAPLEVELY)] = updateTime;
	}

	// Set the script.
	if (scriptUpdated)
	{
		npc->setScript(script);
		npc->modTime[PROPID(NPCProp::SCRIPT)] = updateTime;
	}

	// Join the classes.
	for (const auto& className : joinedClasses)
	{
		if (!className.empty())
			npc->joinClass(className);
	}

	// Set our last update / save times.
	npc->lastUpdateTime = npc->lastSaveTime = toSystemClock(file.modifiedTime());
}

bool FlatFileNPCLoader::saveNPC(NPCPtr npc) noexcept
{
	if (npc->storageType != NPCStorageType::DATABASE)
		return false;

	// Open the file for writing.
	const auto server = BabyDI::Get<Server>();
	const auto fileName = fs::getHTMLEscapedFileName(std::format("npc{}.txt", npc->name));
	const auto file = server->getFileSystemServer().openiForWriting(fs::FileCategory::NPC, fileName, true);
	if (!file)
		return false;

	// Function to check for prop modification before writing.
	auto writeProp = [&](const NPCProp prop, std::string_view key, std::string_view value)
	{
		if (npc->modTime[PROPID(prop)].has_value())
			file->writeConfigLine(key, value);
	};

	const auto level = npc->getLevel();

	// Get the draw layer number.
	int layer = 0;
	if (npc->visFlags & ENUM(NPCVisFlags::DRAWUNDERPLAYER))
		layer = -1;
	else if (npc->visFlags & ENUM(NPCVisFlags::DRAWOVERPLAYER))
		layer = 1;

	// Start the file.
	file->clear();
	file->writeLine("GRNPC001");

	// Write our data.
	file->writeConfigLine("NAME", npc->name);
	file->writeConfigLine("ID", string::to_string(npc->id));
	file->writeConfigLine("TYPE", npc->scriptType);
	file->writeConfigLine("SCRIPTER", npc->scripter);

	file->writeConfigLine("IMAGE", npc->image);
	if (npc->imagePart.size.width() > 0 && npc->imagePart.size.height() > 0)
	{
		file->writeConfigLine("IMGPART", std::format("{} {} {} {}", npc->imagePart.position.x(), npc->imagePart.position.y(), npc->imagePart.size.width(), npc->imagePart.size.height()));
	}

	file->writeConfigLine("STARTLEVEL", npc->m_initialLevel);
	file->writeConfigLine("STARTX", string::to_string(npc->m_initialCharacter.localPixelX / 16.0, 2));
	file->writeConfigLine("STARTY", string::to_string(npc->m_initialCharacter.localPixelY / 16.0, 2));
	if (npc->m_initialCharacter.localPixelZ.has_value())
		file->writeConfigLine("STARTZ", string::to_string(npc->m_initialCharacter.localPixelZ.value() / 16.0, 2));
	if (npc->m_initialCharacter.mapX != 0 || npc->m_initialCharacter.mapY != 0)
	{
		file->writeConfigLine("STARTMAPX", string::to_string(npc->m_initialCharacter.mapX));
		file->writeConfigLine("STARTMAPY", string::to_string(npc->m_initialCharacter.mapY));
	}

	if (!npc->level.empty())
	{
		file->writeConfigLine("LEVEL", npc->getLevelName());
		if (!npc->groupName.empty())
			file->writeConfigLine("GROUPNAME", npc->groupName);

		file->writeConfigLine("X", string::to_string(npc->character.localPixelX / 16.0, 2));
		file->writeConfigLine("Y", string::to_string(npc->character.localPixelY / 16.0, 2));
		if (npc->character.localPixelZ.has_value())
			file->writeConfigLine("Z", string::to_string(npc->character.localPixelZ.value() / 16.0, 2));
		if (npc->character.mapX != 0 || npc->character.mapY != 0 || (level != nullptr && level->isGmap()))
		{
			file->writeConfigLine("MAPX", string::to_string(npc->character.mapX));
			file->writeConfigLine("MAPY", string::to_string(npc->character.mapY));
		}
	}

	writeProp(NPCProp::NICKNAME, "NICK", npc->character.nickName);

	if (server->Generation != ServerGeneration::CLASSIC)
		writeProp(NPCProp::GANI, "ANI", npc->character.gani);

	writeProp(NPCProp::HALFHEARTS, "HP", std::format("{:.2f}", static_cast<float>(npc->character.hitpointsInHalves) / 2.0f));
	writeProp(NPCProp::GRALATS, "GRALATS", string::to_string(npc->character.gralats));
	writeProp(NPCProp::ARROWS, "ARROWS", string::to_string(npc->character.arrows));
	writeProp(NPCProp::BOMBS, "BOMBS", string::to_string(npc->character.bombs));
	writeProp(NPCProp::GLOVEPOWER, "GLOVEP", string::to_string(npc->character.glovePower));
	writeProp(NPCProp::SWORDIMAGE, "SWORDP", string::to_string(npc->character.swordPower));
	writeProp(NPCProp::SHIELDIMAGE, "SHIELDP", string::to_string(npc->character.shieldPower));

	if (server->Generation == ServerGeneration::CLASSIC)
	{
		writeProp(NPCProp::GANI, "BOWP", string::to_string(npc->character.bowPower));
		writeProp(NPCProp::GANI, "BOW", npc->character.bowImage);
	}

	writeProp(NPCProp::HEADIMAGE, "HEAD", npc->character.headImage);
	writeProp(NPCProp::BODYIMAGE, "BODY", npc->character.bodyImage);
	writeProp(NPCProp::SWORDIMAGE, "SWORD", npc->character.swordImage);
	writeProp(NPCProp::SHIELDIMAGE, "SHIELD", npc->character.shieldImage);
	writeProp(NPCProp::HORSEIMAGE, "HORSE", npc->character.horseImage);

	if (server->isNewWorldMode())
		writeProp(NPCProp::COLORS, "COLORS", std::format("{},{},{},{},{},{},{},{}", npc->character.colors[0], npc->character.colors[1], npc->character.colors[2], npc->character.colors[3], npc->character.colors[4], npc->character.colors[5], npc->character.colors[6], npc->character.colors[7]));
	else writeProp(NPCProp::COLORS, "COLORS", std::format("{},{},{},{},{}", npc->character.colors[0], npc->character.colors[1], npc->character.colors[2], npc->character.colors[3], npc->character.colors[4]));

	writeProp(NPCProp::SPRITE, "SPRITE", string::to_string(npc->character.sprite << 2 | npc->character.direction));
	writeProp(NPCProp::ALIGNMENT, "AP", string::to_string(npc->character.ap));

	if (npc->timeout != 0ms)
		file->writeConfigLine("TIMEOUT", string::to_string(std::chrono::duration_cast<duration_seconds_double>(npc->timeout).count()));

	if (layer != 0)
		file->writeConfigLine("LAYER", string::to_string(layer));

	if (npc->shape.width() != 0 || npc->shape.height() != 0)
	{
		file->writeConfigLine("SHAPETYPE", npc->shape.width() != 0 && npc->shape.height() != 0 ? "1" : "0");
		file->writeConfigLine("SHAPE", std::format("{} {}", npc->shape.width(), npc->shape.height()));
	}

	if (npc->blockFlags & PROPID(NPCBlockFlags::NOBLOCK))
		file->writeLine("DONTBLOCK 1");
	if (npc->noPlayerOnWall)
		file->writeLine("NOPLAYERONWALL 1");
	if (npc->warpRestrictions == NPCWarpRestrictions::ALLOWED)
		file->writeLine("CANWARP 1");
	if (npc->warpRestrictions == NPCWarpRestrictions::ONLYOVERWORLD)
		file->writeLine("CANWARP2 1");

	// Official variables for these are unknown.
	if (npc->blockFlags & PROPID(NPCBlockFlags::CANBECARRIED))
		file->writeLine("CANCARRY");
	if (npc->blockFlags & PROPID(NPCBlockFlags::CANBEPULLED))
		file->writeLine("CANPULL");
	if (npc->blockFlags & PROPID(NPCBlockFlags::CANBEPUSHED))
		file->writeLine("CANPUSH");
	if ((npc->visFlags & PROPID(NPCVisFlags::VISIBLE)) == 0)
		file->writeLine("VISIBLE 0");
	if ((npc->visFlags & PROPID(NPCVisFlags::TIMERSHOW)) != 0)
		file->writeLine("TIMERSHOW");
	if (npc->isCharacter() && (npc->visFlags & PROPID(NPCVisFlags::MALE)) == 0)
		file->writeLine("MALE 0");
	// ---

	if (!std::ranges::empty(NPCSaveProps | std::views::filter([&npc](const NPCProp prop) { return npc->modTime[PROPID(prop)].has_value(); })))
		file->writeConfigLine("SAVEARR", string::toCSV(npc->saves | std::views::transform([](const uint8_t x) { return string::to_string(x); })));

	for (int i = 0; i < 30; i++)
	{
		const auto prop = static_cast<NPCProp>(NPCGaniAttrPackets[i]);
		if (!npc->character.ganiAttributes[i].empty())
			writeProp(prop, std::format("ATTR{}", i + 1), npc->character.ganiAttributes[i]);
	}

	for (auto& [flag, value] : npc->scripting.variables.store | variables::serializable)
	{
		// Serialize the variable entirely.
		if (server->Generation == ServerGeneration::MODERN)
		{
			if (auto var = npc->scripting.variables.serializeModern(flag); var.has_value())
				file->writeConfigLine("FLAG", var.value());
		}
		else
		{
			for (const auto& serialized : npc->scripting.variables.serialize(flag))
				file->writeLine(serialized);
		}
	}

	if (!npc->m_joinedClasses.empty())
	{
		file->writeConfigLine("JOINEDCLASSES", npc->getJoinedClassesList());
	}

	file->writeConfigSection("NPCSCRIPT", npc->getScript().getOriginalSource(), "NPCSCRIPTEND");

	// Finish up.
	file->close();

	// Update the NPC's last save time.
	npc->lastUpdateTime = npc->lastSaveTime = toSystemClock(file->modifiedTime());

	// If the NPC exists on the filesystem, refresh its mod time to avoid any modification events.
	auto& fs = server->getFileSystemServer();
	if (const auto info = fs.info(fs::FileCategory::NPC, file->filePath().filename()); info != nullptr)
		info->refreshModTime();
	// Else if the NPC doesn't exist, we want to add it to the file system so the file watcher doesn't cause a reload.
	else
	{
		fs.addExisting(fs::FileCategory::NPC, file->filePath());
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
