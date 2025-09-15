#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <string_view>
#include <string>
#include <vector>

#include <BabyDI.h>
#include <CString.h>

#include <FileSystem.h>
#include <Server.h>
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

static constexpr std::array<uint8_t, 30> attrPackets = { 36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 };

///////////////////////////////////////////////////////////////////////////////

NPCPtr FlatFileNPCLoader::loadNPC(std::string_view npcName) noexcept
{
	// Get the NPCs.
	// TODO(Nalin): Add a proper NPC filesystem.
	FileSystem npcFS;
	npcFS.addDir("npcs", "npc*.txt");

	// Find the NPC to load.
	auto filePath = npcFS.findi(std::format("npc{}.txt", npcName));
	return loadNPC(std::filesystem::path{ filePath.toStringView()});
}

NPCPtr FlatFileNPCLoader::loadNPC(const std::filesystem::path& filePath) noexcept
{
	auto server = BabyDI::Get<Server>();

	// Load file
	CString fileData;
	if (!fileData.load(filePath.string()))
		return nullptr;

	fileData.removeAllI("\r");

	CString headerLine = fileData.readString("\n");
	if (headerLine != "GRNPC001")
		return nullptr;

	// Search for the ID of the NPC from the file data.
	NPCID id = 0;
	if (auto start = fileData.find("ID "); start != -1)
	{
		auto idStr = fileData.subString(start + 3, fileData.find("\n", start) - start - 3);
		idStr.trimI();
		id = std::strtol(idStr.text(), nullptr, 10);

		if (id < NPCID_GEN_DATABASE)
		{
			id = 0;
			log::printLine(log::server, "** NPC [{}] ID is less than {}, getting next available.", filePath.filename().string(), NPCID_GEN_DATABASE);
		}
		else if (server->m_npcIdGenerator.isIdUsed(id))
		{
			id = 0;
			log::printLine(log::server, "** NPC [{}] ID is already in use, getting next available.", filePath.filename().string());
		}
		else server->m_npcIdGenerator.markAsUsed(id);
	}

	if (id == 0)
		id = server->m_npcIdGenerator.getAvailableId(NPCID_GEN_DATABASE);

	// Make the NPC.
	auto npc = std::make_shared<NPC>(id, NPCStorageType::DATABASE);

	// Set the default warp type.
	if (server->hasNPCServer())
	{
		npc->warpRestrictions = NPCWarpRestrictions::NOTALLOWED;
	}

	// Set some default values.
	bool isMale = true;
	npc->visFlags = PROPID(NPCVisFlags::VISIBLE) | PROPID(NPCVisFlags::CREATED);

	const auto& updateTime = server->getServerStartTime();
	std::string script;
	std::vector<std::string> joinedClasses;

	// Parse File
	while (fileData.bytesLeft())
	{
		CString curLine = fileData.readString("\n");

		// Find Command
		CString curCommand = curLine.readString();

		// Parse Line
		if (curCommand == "NAME")
		{
			npc->name = curLine.readString("").text();
			npc->modTime[PROPID(NPCProp::NAME)] = updateTime;
		}
		else if (curCommand == "ID")
			; // npc->id = strtoint(curLine.readString(""));
		else if (curCommand == "TYPE")
			npc->scriptType = curLine.readString("");
		else if (curCommand == "SCRIPTER")
		{
			npc->scripter = curLine.readString("");
			npc->modTime[PROPID(NPCProp::SCRIPTER)] = updateTime;
		}
		else if (curCommand == "IMAGE")
		{
			npc->image = curLine.readString("").text();
			npc->modTime[PROPID(NPCProp::IMAGE)] = updateTime;
		}
		else if (curCommand == "IMGPART")
		{
			auto parts = curLine.tokenize();
			if (parts.size() >= 4)
			{
				npc->imagePart.position = { static_cast<uint16_t>(strtoint(parts[0])), static_cast<uint16_t>(strtoint(parts[1])) };
				npc->imagePart.size = { static_cast<uint8_t>(strtoint(parts[2])), static_cast<uint8_t>(strtoint(parts[3])) };
				npc->modTime[PROPID(NPCProp::IMAGEPART)] = updateTime;
			}
		}
		else if (curCommand == "STARTLEVEL")
			npc->m_initialLevel = curLine.readString("");
		else if (curCommand == "STARTX")
			npc->m_initialCharacter.localPixelX = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "STARTY")
			npc->m_initialCharacter.localPixelY = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "STARTZ")
			npc->m_initialCharacter.localPixelZ = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "LEVEL")
			npc->level = curLine.readString("");
		else if (curCommand == "X")
		{
			npc->character.localPixelX = int(strtofloat(curLine.readString("")) * 16);
			npc->modTime[PROPID(NPCProp::X)] = updateTime;
			npc->modTime[PROPID(NPCProp::X2)] = updateTime;
		}
		else if (curCommand == "Y")
		{
			npc->character.localPixelY = int(strtofloat(curLine.readString("")) * 16);
			npc->modTime[PROPID(NPCProp::Y)] = updateTime;
			npc->modTime[PROPID(NPCProp::Y2)] = updateTime;
		}
		else if (curCommand == "Z")
		{
			npc->character.localPixelZ = int(strtofloat(curLine.readString("")) * 16);
			npc->modTime[PROPID(NPCProp::Z)] = updateTime;
			npc->modTime[PROPID(NPCProp::Z2)] = updateTime;
		}
		else if (curCommand == "MAPX")
		{
			npc->character.mapX = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::GMAPLEVELX)] = updateTime;
		}
		else if (curCommand == "MAPY")
		{
			npc->character.mapY = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::GMAPLEVELY)] = updateTime;
		}
		else if (curCommand == "NICK")
		{
			npc->character.nickName = curLine.readString("").text();
			npc->modTime[PROPID(NPCProp::NICKNAME)] = updateTime;
		}
		else if (curCommand == "ANI")
		{
			npc->character.gani = curLine.readString("").text();
			npc->modTime[PROPID(NPCProp::GANI)] = updateTime;
		}
		else if (curCommand == "HP")
		{
			npc->character.hitpointsInHalves = 2 * (int)strtofloat(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::POWER)] = updateTime;
		}
		else if (curCommand == "GRALATS")
		{
			npc->character.gralats = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::RUPEES)] = updateTime;
		}
		else if (curCommand == "ARROWS")
		{
			npc->character.arrows = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::ARROWS)] = updateTime;
		}
		else if (curCommand == "BOMBS")
		{
			npc->character.bombs = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::BOMBS)] = updateTime;
		}
		else if (curCommand == "GLOVEP")
		{
			npc->character.glovePower = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::GLOVEPOWER)] = updateTime;
		}
		else if (curCommand == "SWORDP")
		{
			npc->character.swordPower = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::SWORDIMAGE)] = updateTime;
		}
		else if (curCommand == "SHIELDP")
		{
			npc->character.shieldPower = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::SHIELDIMAGE)] = updateTime;
		}
		else if (curCommand == "BOWP")
		{
			npc->character.bowPower = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::GANI)] = updateTime;
		}
		else if (curCommand == "BOW")
		{
			npc->character.bowImage = curLine.readString("").toString();
			npc->modTime[PROPID(NPCProp::GANI)] = updateTime;
		}
		else if (curCommand == "HEAD")
		{
			npc->character.headImage = curLine.readString("").toString();
			npc->modTime[PROPID(NPCProp::HEADIMAGE)] = updateTime;
		}
		else if (curCommand == "BODY")
		{
			npc->character.bodyImage = curLine.readString("").toString();
			npc->modTime[PROPID(NPCProp::BODYIMAGE)] = updateTime;
		}
		else if (curCommand == "SWORD")
		{
			npc->character.swordImage = curLine.readString("").toString();
			npc->modTime[PROPID(NPCProp::SWORDIMAGE)] = updateTime;
		}
		else if (curCommand == "SHIELD")
		{
			npc->character.shieldImage = curLine.readString("").toString();
			npc->modTime[PROPID(NPCProp::SHIELDIMAGE)] = updateTime;
		}
		else if (curCommand == "HORSE")
		{
			npc->character.horseImage = curLine.readString("").toString();
			npc->modTime[PROPID(NPCProp::HORSEIMAGE)] = updateTime;
		}
		else if (curCommand == "COLORS")
		{
			auto tokens = curLine.readString("").tokenize(",");
			for (size_t idx = 0; idx < std::min(tokens.size(), (size_t)5); idx++)
				npc->character.colors[idx] = strtoint(tokens[idx]);
			npc->modTime[PROPID(NPCProp::COLORS)] = updateTime;
		}
		else if (curCommand == "SPRITE")
		{
			auto sprite = strtoint(curLine.readString(""));
			npc->character.sprite = sprite >> 2;
			npc->character.direction = sprite & 0b11;
			npc->modTime[PROPID(NPCProp::SPRITE)] = updateTime;
		}
		else if (curCommand == "AP")
		{
			npc->character.ap = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::ALIGNMENT)] = updateTime;
		}
		else if (curCommand == "TIMEOUT")
		{
			npc->timeout = std::chrono::milliseconds(strtoint(curLine.readString("")) * 20);
		}
		else if (curCommand == "LAYER")
		{
			auto layer = strtoint(curLine.readString(""));
			if (layer == -1)
				npc->visFlags |= PROPID(NPCVisFlags::DRAWUNDERPLAYER);
			if (layer == 1)
				npc->visFlags |= PROPID(NPCVisFlags::DRAWOVERPLAYER);
			npc->modTime[PROPID(NPCProp::VISFLAGS)] = updateTime;
		}
		else if (curCommand == "SHAPETYPE")
		{
			// Only shape type 1 is supported, but we just look at the dimension of the shape data.
		}
		else if (curCommand == "SHAPE")
		{
			std::get<0>(npc->shape.data) = strtoint(curLine.readString(" "));
			std::get<1>(npc->shape.data) = strtoint(curLine.readString(" "));
		}
		else if (curCommand == "DONTBLOCK")
		{
			npc->blockFlags = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::BLOCKFLAGS)] = updateTime;
		}
		else if (curCommand == "NOPLAYERONWALL")
		{
			npc->noPlayerOnWall = strtoint(curLine.readString("")) != 0;
		}
		else if (curCommand == "SAVEARR")
		{
			auto tokens = curLine.readString("").tokenize(",");
			for (size_t idx = 0; idx < std::min(tokens.size(), npc->saves.size()); idx++)
			{
				npc->saves[idx] = (unsigned char)strtoint(tokens[idx]);
				npc->modTime[PROPID(NPCProp::SAVE0) + idx] = updateTime;
			}
		}
		else if (curCommand == "CANWARP")
		{
			npc->warpRestrictions = strtoint(curLine.readString("")) != 0 ? NPCWarpRestrictions::ALLOWED : npc->warpRestrictions;
		}
		else if (curCommand == "CANWARP2")
		{
			npc->warpRestrictions = strtoint(curLine.readString("")) != 0 ? NPCWarpRestrictions::ONLYOVERWORLD : npc->warpRestrictions;
		}

		// Official variables for these are unknown.
		else if (curCommand == "CANCARRY")
		{
			auto value = strtoint(curLine.readString(""));
			if (value != 0)
				npc->blockFlags |= PROPID(NPCBlockFlags::CANBECARRIED);
		}
		else if (curCommand == "CANPULL")
		{
			auto value = strtoint(curLine.readString(""));
			if (value != 0)
				npc->blockFlags |= PROPID(NPCBlockFlags::CANBEPULLED);
		}
		else if (curCommand == "CANPUSH")
		{
			auto value = strtoint(curLine.readString(""));
			if (value != 0)
				npc->blockFlags |= PROPID(NPCBlockFlags::CANBEPUSHED);
		}
		else if (curCommand == "VISIBLE")
		{
			auto value = strtoint(curLine.readString(""));
			if (value == 0)
				npc->visFlags &= ~PROPID(NPCVisFlags::VISIBLE);
		}
		else if (curCommand == "TIMERSHOW")
		{
			auto value = strtoint(curLine.readString(""));
			if (value != 0)
				npc->visFlags |= PROPID(NPCVisFlags::TIMERSHOW);
		}
		else if (curCommand == "MALE")
		{
			auto value = strtoint(curLine.readString(""));
			if (value == 0)
				isMale = false;
		}
		//---

		else if (curCommand == "FLAG")
		{
			std::string flagName = curLine.readString("=").toString();
			std::string flagValue = curLine.readString("").toString();
			npc->scripting.variables.add(GameValue::deserialize(flagName, flagValue));
		}
		else if (curCommand.subString(0, 4) == "ATTR")
		{
			CString attrIdStr = curCommand.subString(5);
			int attrId = strtoint(attrIdStr);
			if (attrId > 0 && attrId < 30)
			{
				int idx = attrId - 1;
				npc->character.ganiAttributes[idx] = curLine.readString("").toString();
				npc->modTime[attrPackets[idx]] = updateTime;
			}
		}
		else if (curCommand == "JOINEDCLASSES")
		{
			joinedClasses = string::fromCSV(curLine.readString("").toString());
		}
		else if (curCommand == "NPCSCRIPT")
		{
			do {
				curLine = fileData.readString("\n");
				if (curLine == "NPCSCRIPTEND")
					break;

				script.append(curLine.text(), curLine.length()).append(1, '\n');
			} while (fileData.bytesLeft());

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

	// TODO(joey): check if properties have been modified before deciding to save
	// enumerate scriptObject variables, to save into file and load later..?

	// Clean up old samples
	//m_scriptExecutionContext.getExecutionData();

	auto level = npc->getLevel();

	int layer = 0;
	if (npc->visFlags & PROPID(NPCVisFlags::DRAWUNDERPLAYER))
		layer = -1;
	else if (npc->visFlags & PROPID(NPCVisFlags::DRAWOVERPLAYER))
		layer = 1;

	static const char* NL = "\r\n";
	CString fileName = CString() << "npcs/npc" << npc->name << ".txt";
	CString fileData = CString("GRNPC001") << NL;
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
	fileData << "NICK " << npc->character.nickName << NL;
	fileData << "ANI " << npc->character.gani << NL;
	fileData << "HP " << CString(npc->character.hitpointsInHalves / 2.0f) << NL;
	fileData << "GRALATS " << CString(npc->character.gralats) << NL;
	fileData << "ARROWS " << CString(npc->character.arrows) << NL;
	fileData << "BOMBS " << CString(npc->character.bombs) << NL;
	fileData << "GLOVEP " << CString(npc->character.glovePower) << NL;
	fileData << "SWORDP " << CString(npc->character.swordPower) << NL;
	fileData << "SHIELDP " << CString(npc->character.shieldPower) << NL;
	fileData << "BOWP " << CString(npc->character.bowPower) << NL;
	fileData << "BOW " << npc->character.bowImage << NL;
	fileData << "HEAD " << npc->character.headImage << NL;
	fileData << "BODY " << npc->character.bodyImage << NL;
	fileData << "SWORD " << npc->character.swordImage << NL;
	fileData << "SHIELD " << npc->character.shieldImage << NL;
	fileData << "HORSE " << npc->character.horseImage << NL;
	fileData << "COLORS " << CString((int)npc->character.colors[0]) << "," << CString((int)npc->character.colors[1]) << "," << CString((int)npc->character.colors[2]) << "," << CString((int)npc->character.colors[3]) << "," << CString((int)npc->character.colors[4]) << NL;
	fileData << "SPRITE " << CString(npc->character.sprite << 2 | npc->character.direction) << NL;
	fileData << "AP " << CString(npc->character.ap) << NL;
	fileData << "TIMEOUT " << CString(static_cast<int>(npc->timeout.count() * 0.05)) << NL;
	fileData << "LAYER " << CString(layer) << NL;
	fileData << "SHAPETYPE " << (npc->shape.width() != 0 && npc->shape.height() != 0 ? "1" : "0") << NL;
	fileData << "SHAPE " << CString(npc->shape.width()) << " " << CString(npc->shape.height()) << NL;

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
	if ((npc->visFlags & PROPID(NPCVisFlags::MALE)) == 0)
		fileData << "MALE 0" << NL;
	// ---

	fileData << "SAVEARR " << CString((int)npc->saves[0]) << "," << CString((int)npc->saves[1]) << "," << CString((int)npc->saves[2]) << ","
		<< CString((int)npc->saves[3]) << "," << CString((int)npc->saves[4]) << "," << CString((int)npc->saves[5]) << ","
		<< CString((int)npc->saves[6]) << "," << CString((int)npc->saves[7]) << "," << CString((int)npc->saves[8]) << ","
		<< CString((int)npc->saves[9]) << NL;

	for (int i = 0; i < 30; i++)
	{
		if (!npc->character.ganiAttributes[i].empty())
			fileData << "ATTR" << std::to_string(i + 1) << " " << npc->character.ganiAttributes[i] << NL;
	}

	auto* server = BabyDI::Get<Server>();
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
		fileData << "JOINEDCLASSES " << npc->getJoinedClasses() << NL;
	}

	fileData << "NPCSCRIPT" << NL << CString(npc->getScript().getOriginalSource()).replaceAll("\n", NL);
	if (fileData[fileData.length() - 1] != '\n')
		fileData << NL;
	fileData << "NPCSCRIPTEND" << NL;
	fileData.save(fileName);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
