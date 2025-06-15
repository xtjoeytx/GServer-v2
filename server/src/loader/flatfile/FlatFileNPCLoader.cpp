#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <memory>
#include <string_view>
#include <string>
#include <tuple>

#include <BabyDI.h>
#include <CString.h>
#include <FileSystem.h>
#include <Server.h>
#include <loader/flatfile/FlatFileNPCLoader.h>
#include <object/NPC.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>

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

		if (id < NPCID_INIT)
		{
			id = 0;
			log::printLine(log::server, "** NPC [{}] ID is less than {}, getting next available.", filePath.filename().string(), NPCID_INIT);
		}
		else if (server->m_npcIdGenerator.isIdUsed(id))
		{
			id = 0;
			log::printLine(log::server, "** NPC [{}] ID is already in use, getting next available.", filePath.filename().string());
		}
		else server->m_npcIdGenerator.markAsUsed(id);
	}

	if (id == 0)
		id = server->m_npcIdGenerator.getAvailableId();

	// Make the NPC.
	auto npc = std::make_shared<NPC>(id, NPCType::DBNPC);

	// Set the default warp type.
	if (server->hasNPCServer())
	{
		npc->warpRestrictions = NPCWarpRestrictions::NOTALLOWED;
	}

	auto updateTime = currentTime();
	CString npcLevel;
	std::string script;

	CString propPacket;

	std::string npcInitialLevel;

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
			; // npc->m_id = strtoint(curLine.readString(""));
		else if (curCommand == "TYPE")
			npc->m_npcScriptType = curLine.readString("");
		else if (curCommand == "SCRIPTER")
		{
			npc->m_npcScripter = curLine.readString("");
			npc->modTime[PROPID(NPCProp::SCRIPTER)] = updateTime;
		}
		else if (curCommand == "IMAGE")
		{
			npc->image = curLine.readString("").text();
			npc->modTime[PROPID(NPCProp::IMAGE)] = updateTime;
		}
		else if (curCommand == "STARTLEVEL")
			npcInitialLevel = curLine.readString("");
		else if (curCommand == "STARTX")
			npc->m_initialCharacter.pixelX = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "STARTY")
			npc->m_initialCharacter.pixelY = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "STARTZ")
			npc->m_initialCharacter.pixelZ = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "LEVEL")
			npcLevel = curLine.readString("");
		else if (curCommand == "X")
			npc->character.pixelX = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "Y")
			npc->character.pixelY = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "Z")
			npc->character.pixelZ = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "MAPX")
		{
			//gmaplevelx = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::GMAPLEVELX)] = updateTime;
		}
		else if (curCommand == "MAPY")
		{
			//gmaplevely = strtoint(curLine.readString(""));
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
		else if (curCommand == "SPRITE")
		{
			npc->character.sprite = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::SPRITE)] = updateTime;
		}
		else if (curCommand == "AP")
		{
			npc->character.ap = strtoint(curLine.readString(""));
			npc->modTime[PROPID(NPCProp::ALIGNMENT)] = updateTime;
		}
		else if (curCommand == "COLORS")
		{
			auto tokens = curLine.readString("").tokenize(",");
			for (size_t idx = 0; idx < std::min((int)tokens.size(), 5); idx++)
				npc->character.colors[idx] = strtoint(tokens[idx]);
			npc->modTime[PROPID(NPCProp::COLORS)] = updateTime;
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
		else if (curCommand == "SHAPE")
		{
			std::get<0>(npc->imageSize.data) = strtoint(curLine.readString(" "));
			std::get<1>(npc->imageSize.data) = strtoint(curLine.readString(" "));
		}
		else if (curCommand == "CANWARP")
		{
			npc->warpRestrictions = strtoint(curLine.readString("")) != 0 ? NPCWarpRestrictions::ALLOWED : npc->warpRestrictions;
		}
		else if (curCommand == "CANWARP2")
		{
			npc->warpRestrictions = strtoint(curLine.readString("")) != 0 ? NPCWarpRestrictions::ONLYOVERWORLD : npc->warpRestrictions;
		}
		// TODO(Nalin): This should be split up in stuff like DONTBLOCK 1, but I don't know all the fields.
		else if (curCommand == "BLOCKFLAGS")
		{
			npc->blockFlags = strtoint(curLine.readString(""));
		}
		else if (curCommand == "VISFLAGS")
		{
			npc->visFlags = strtoint(curLine.readString(""));
		}
		else if (curCommand == "TIMEOUT")
		{
			npc->timeout = std::chrono::milliseconds(strtoint(curLine.readString("")) * 20);
		}
		else if (curCommand == "FLAG")
		{
			std::string flagName = curLine.readString("=").toString();
			std::string flagValue = curLine.readString("").toString();
			npc->scripting.variables.add(GameVariable::deserialize(flagName, flagValue));
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
			;
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

	// Add the NPC to the server.
	server->addNPC(npc, false);

	// Set the script.
	npc->setScript(script);

	// Add it to the level, if needed.
	auto level = server->getLevel(npcLevel.toString());
	auto initialLevel = server->getLevel(npcInitialLevel);
	npc->level = level ? level : initialLevel;
	npc->m_initialLevel = initialLevel;

	if (level)
		level->addNPC(id);

	return npc;
}

bool FlatFileNPCLoader::saveNPC(NPCPtr npc) noexcept
{
	if (npc->type != NPCType::DBNPC)
		return false;

	// TODO(joey): check if properties have been modified before deciding to save
	// enumerate scriptObject variables, to save into file and load later..?

	// Clean up old samples
	//m_scriptExecutionContext.getExecutionData();

	auto level = npc->level.lock();
	auto initialLevel = npc->m_initialLevel.lock();

	CString levelName = level ? level->getLevelName() : "";
	CString initialLevelName = initialLevel ? initialLevel->getLevelName() : "";

	static const char* NL = "\r\n";
	CString fileName = CString() << "npcs/npc" << npc->name << ".txt";
	CString fileData = CString("GRNPC001") << NL;
	fileData << "NAME " << npc->name << NL;
	fileData << "ID " << CString(npc->id) << NL;
	fileData << "TYPE " << npc->m_npcScriptType << NL;
	fileData << "SCRIPTER " << npc->m_npcScripter << NL;
	fileData << "IMAGE " << npc->image << NL;
	fileData << "STARTLEVEL " << initialLevelName << NL;
	fileData << "STARTX " << CString((float)npc->m_initialCharacter.pixelX / 16.0f) << NL;
	fileData << "STARTY " << CString((float)npc->m_initialCharacter.pixelY / 16.0f) << NL;
	fileData << "STARTZ " << CString((float)npc->m_initialCharacter.pixelZ / 16.0f) << NL;
	if (level)
	{
		fileData << "LEVEL " << level->getLevelName() << NL;
		fileData << "X " << CString((float)npc->character.pixelX / 16.0f) << NL;
		fileData << "Y " << CString((float)npc->character.pixelY / 16.0f) << NL;
		fileData << "Z " << CString((float)npc->character.pixelZ / 16.0f) << NL;
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
	fileData << "BOWP" << CString(npc->character.bowPower) << NL;
	fileData << "BOW" << npc->character.bowImage << NL;
	fileData << "HEAD " << npc->character.headImage << NL;
	fileData << "BODY " << npc->character.bodyImage << NL;
	fileData << "SWORD " << npc->character.swordImage << NL;
	fileData << "SHIELD " << npc->character.shieldImage << NL;
	fileData << "HORSE " << npc->character.horseImage << NL;
	fileData << "COLORS " << CString((int)npc->character.colors[0]) << "," << CString((int)npc->character.colors[1]) << "," << CString((int)npc->character.colors[2]) << "," << CString((int)npc->character.colors[3]) << "," << CString((int)npc->character.colors[4]) << NL;
	fileData << "SPRITE " << CString(npc->character.sprite) << NL;
	fileData << "AP " << CString(npc->character.ap) << NL;
	fileData << "TIMEOUT " << CString(static_cast<int>(npc->timeout.count() * 0.05)) << NL;
	fileData << "LAYER 0" << NL;
	fileData << "SHAPETYPE 0" << NL;
	fileData << "SHAPE " << CString(npc->imageSize.width()) << " " << CString(npc->imageSize.height()) << NL;
	fileData << "BLOCKFLAGS " << CString(npc->blockFlags) << NL;
	fileData << "VISFLAGS " << CString(npc->visFlags) << NL;

	if (npc->warpRestrictions == NPCWarpRestrictions::NOTALLOWED)
		fileData << "CANWARP" << NL;
	if (npc->warpRestrictions == NPCWarpRestrictions::ONLYOVERWORLD)
		fileData << "CANWARP2" << NL;

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
	for (auto& [flag, value] : npc->scripting.variables.store)
	{
		// Ignore flags.
		if (value->has<bool>() && !value->has<std::string>()) continue;

		// Ignore temporary variables.
		if (value->temporary) continue;

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

	fileData << "NPCSCRIPT" << NL << CString(npc->getScript().getOriginalSource()).replaceAll("\n", NL);
	if (fileData[fileData.length() - 1] != '\n')
		fileData << NL;
	fileData << "NPCSCRIPTEND" << NL;
	fileData.save(fileName);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
