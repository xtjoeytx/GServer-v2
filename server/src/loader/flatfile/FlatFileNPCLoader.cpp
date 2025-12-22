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
	auto warpRestriction = server->hasNPCServer() ? NPCWarpRestrictions::NOTALLOWED : NPCWarpRestrictions::ALLOWED;

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
			auto parts = string::splitToVectorView(lineView, " "sv);
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
			auto tokens = string::splitToVectorView(lineView, ","sv);
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
			auto tokens = string::splitToVectorView(lineView, ","sv);
			for (size_t idx = 0; idx < std::min(tokens.size(), npc->saves.size()); idx++)
			{
				npc->saves[idx] = string::toNumber<uint8_t>(tokens[idx]);
				npc->modTime[PROPID(NPCProp::SAVE0) + idx] = updateTime;
			}
		}
		else if (command == "CANWARP")
		{
			warpRestriction = string::toNumber<uint8_t>(std::string{ lineView }) != 0 ? NPCWarpRestrictions::ALLOWED : warpRestriction;
		}
		else if (command == "CANWARP2")
		{
			warpRestriction = string::toNumber<uint8_t>(std::string{ lineView }) != 0 ? NPCWarpRestrictions::ONLYOVERWORLD : warpRestriction;
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
	file->close();

	// If the NPC is a character, set the gender prop.
	// Also, set the gender.
	if (npc->isCharacter() && isMale)
	{
		npc->visFlags |= PROPID(NPCVisFlags::MALE);
	}

	// If the NPC has no image, make it invisible.
	if (!npc->hasImage() && !npc->hasShape())
	{
		npc->visFlags &= ~PROPID(NPCVisFlags::VISIBLE);
	}

	// Set the script.
	npc->setScript(script);

	// Join the classes.
	for (const auto& className : joinedClasses)
	{
		if (!className.empty())
			npc->joinClass(className);
	}

	// Add the NPC to the server.
	server->addNPC(npc, false);

	// Set the warp restriction (do this after adding to the server since that will overwrite the restriction).
	npc->warpRestrictions = warpRestriction;

	// Check if we need to rename the file.
	auto expectedFileName = fs::getHTMLEscapedFileName(std::format("npc{}.txt", npc->name)).string();
	auto currentFileName = fs::getANSIFileName(filePath);
	if (expectedFileName != currentFileName)
	{
		auto fileData = server->getFileSystemServer().infoi(fs::FileCategory::NPC, currentFileName);
		if (fileData != nullptr)
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

bool FlatFileNPCLoader::saveNPC(NPCPtr npc) noexcept
{
	if (npc->storageType != NPCStorageType::DATABASE)
		return false;

	// Open the file for writing.
	auto server = BabyDI::Get<Server>();
	auto fileName = fs::getHTMLEscapedFileName(std::format("npc{}.txt", npc->name));
	auto file = server->getFileSystemServer().openiForWriting(fs::FileCategory::NPC , fileName, true);
	if (!file)
		return false;

	// Function to check for prop modification before writing.
	auto writeProp = [&](NPCProp prop, std::string_view key, std::string_view value)
	{
		if (npc->modTime[PROPID(prop)] != clock::time_point::min())
			file->writeConfigLine(key, value);
	};

	auto level = npc->getLevel();

	// Get the draw layer number.
	int layer = 0;
	if (npc->visFlags & PROPID(NPCVisFlags::DRAWUNDERPLAYER))
		layer = -1;
	else if (npc->visFlags & PROPID(NPCVisFlags::DRAWOVERPLAYER))
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
	file->writeConfigLine("STARTZ", string::to_string(npc->m_initialCharacter.localPixelZ / 16.0, 2));

	if (level)
	{
		file->writeConfigLine("LEVEL", npc->getLevelName());
		file->writeConfigLine("X", string::to_string(npc->character.localPixelX / 16.0, 2));
		file->writeConfigLine("Y", string::to_string(npc->character.localPixelY / 16.0, 2));
		file->writeConfigLine("Z", string::to_string(npc->character.localPixelZ / 16.0, 2));
		if (level->isGmap())
		{
			file->writeConfigLine("MAPX", string::to_string(npc->character.mapX));
			file->writeConfigLine("MAPY", string::to_string(npc->character.mapY));
		}
	}

	writeProp(NPCProp::NICKNAME, "NICK", npc->character.nickName);

	if (server->Generation != ServerGeneration::ORIGINAL)
		writeProp(NPCProp::GANI, "ANI", npc->character.gani);

	writeProp(NPCProp::POWER, "HP", std::format("{:2f}", npc->character.hitpointsInHalves / 2.0f));
	writeProp(NPCProp::RUPEES, "GRALATS", string::to_string(npc->character.gralats));
	writeProp(NPCProp::ARROWS, "ARROWS", string::to_string(npc->character.arrows));
	writeProp(NPCProp::BOMBS, "BOMBS", string::to_string(npc->character.bombs));
	writeProp(NPCProp::GLOVEPOWER, "GLOVEP", string::to_string(npc->character.glovePower));
	writeProp(NPCProp::SWORDIMAGE, "SWORDP", string::to_string(npc->character.swordPower));
	writeProp(NPCProp::SHIELDIMAGE, "SHIELDP", string::to_string(npc->character.shieldPower));

	if (server->Generation == ServerGeneration::ORIGINAL)
	{
		writeProp(NPCProp::GANI, "BOWP", string::to_string(npc->character.bowPower));
		writeProp(NPCProp::GANI, "BOW", npc->character.bowImage);
	}

	writeProp(NPCProp::HEADIMAGE, "HEAD", npc->character.headImage);
	writeProp(NPCProp::BODYIMAGE, "BODY", npc->character.bodyImage);
	writeProp(NPCProp::SWORDIMAGE, "SWORD", npc->character.swordImage);
	writeProp(NPCProp::SHIELDIMAGE, "SHIELD", npc->character.shieldImage);
	writeProp(NPCProp::HORSEIMAGE, "HORSE", npc->character.horseImage);
	writeProp(NPCProp::COLORS, "COLORS", std::format("{},{},{},{},{}", npc->character.colors[0], npc->character.colors[1], npc->character.colors[2], npc->character.colors[3], npc->character.colors[4]));
	writeProp(NPCProp::SPRITE, "SPRITE", string::to_string(npc->character.sprite << 2 | npc->character.direction));
	writeProp(NPCProp::ALIGNMENT, "AP", string::to_string(npc->character.ap));

	if (npc->timeout != 0ms)
		file->writeConfigLine("TIMEOUT", string::to_string(static_cast<int>(npc->timeout.count() * 0.05)));

	if (layer != 0)
		file->writeConfigLine("LAYER", string::to_string(layer + 1));

	if (npc->shape.width() != 0 || npc->shape.height() != 0)
	{
		file->writeConfigLine("SHAPETYPE", npc->shape.width() != 0 && npc->shape.height() != 0 ? "1" : "0");
		file->writeConfigLine("SHAPE", std::format("{} {}", npc->shape.width(), npc->shape.height()));
	}

	if (npc->blockFlags & PROPID(NPCBlockFlags::NOBLOCK))
		file->writeLine("DONTBLOCK 1");
	if (npc->noPlayerOnWall)
		file->writeLine("NOPLAYERONWALL 1");
	if (npc->warpRestrictions == NPCWarpRestrictions::NOTALLOWED)
		file->writeLine("CANWARP");
	if (npc->warpRestrictions == NPCWarpRestrictions::ONLYOVERWORLD)
		file->writeLine("CANWARP2");

	// Official variables for these are unknown.
	if (npc->blockFlags & PROPID(NPCBlockFlags::CANBECARRIED))
		file->writeLine("CANCARRY 1");
	if (npc->blockFlags & PROPID(NPCBlockFlags::CANBEPULLED))
		file->writeLine("CANPULL");
	if (npc->blockFlags & PROPID(NPCBlockFlags::CANBEPUSHED))
		file->writeLine("CANPUSH");
	if ((npc->visFlags & PROPID(NPCVisFlags::VISIBLE)) == 0)
		file->writeLine("VISIBLE 0");
	if ((npc->visFlags & PROPID(NPCVisFlags::TIMERSHOW)) != 0)
		file->writeLine("TIMERSHOW 1");
	if (npc->isCharacter() && (npc->visFlags & PROPID(NPCVisFlags::MALE)) == 0)
		file->writeLine("MALE 0");
	// ---

	if (!std::ranges::empty(NPCSaveProps | std::views::filter([&npc](NPCProp prop) { return npc->modTime[PROPID(prop)] != clock::time_point::min(); })))
		file->writeConfigLine("SAVEARR", string::toCSV(npc->saves | std::views::transform([](uint8_t x) { return string::to_string(x); })));

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
	npc->lastSaveTime = fs::toModTime(file->modifiedTime());

	// If the NPC exists on the filesystem, refresh its mod time to avoid any modification events.
	if (auto info = server->getFileSystemServer().info(fs::FileCategory::NPC, file->filePath().filename()); info != nullptr)
		info->refreshModTime();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
