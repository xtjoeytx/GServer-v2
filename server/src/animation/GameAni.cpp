#include <any>
#include <cstdint>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Server.h>
#include <animation/GameAni.h>
#include <filesystem/FileSystemTypes.h>
#include <npcserver/NPCServer.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

std::optional<GameAni> GameAni::load(Server* const server, const std::string& name)
{
	auto& fileSystem = server->getFileSystem();

	// Search for the file in the filesystem
	auto filePath = fileSystem.find(fs::FileCategory::FILE, name);
	if (filePath.empty())
		return std::nullopt;

	// Load the animation file for parsing
	std::vector<CString> fileData = CString::loadToken(filePath.generic_string(), "\n", true);
	if (fileData.empty())
		return std::nullopt;

	GameAni gameAni(name);

	// Parse the animation
	for (auto i = fileData.begin(); i != fileData.end(); ++i)
	{
		// Tokenize
		std::vector<CString> curLine = i->tokenize();
		if (curLine.empty())
			continue;

		if (curLine[0] == "CONTINUOUS")
		{
			if (curLine.size() == 1 || strtoint(curLine[1]) != 0)
				gameAni.m_aniFlags |= AniFlags::Continous;
			else
				gameAni.m_aniFlags &= ~(AniFlags::Continous);
		}
		else if (curLine[0] == "LOOP")
		{
			if (curLine.size() == 1 || strtoint(curLine[1]) != 0)
				gameAni.m_aniFlags |= AniFlags::LoopAnimation;
			else
				gameAni.m_aniFlags &= ~(AniFlags::LoopAnimation);
		}
		else if (curLine[0] == "SINGLEDIRECTION")
		{
			if (curLine.size() == 1 || strtoint(curLine[1]) != 0)
				gameAni.m_aniFlags |= AniFlags::SingleDirOnly;
			else
				gameAni.m_aniFlags &= ~(AniFlags::SingleDirOnly);
		}
		else if (curLine[0] == "SETBACKTO")
		{
			if (curLine.size() >= 2)
				gameAni.m_setBackTo = curLine[1].toString();
		}
		else if (curLine[0] == "SCRIPT")
		{
			CString code;
			++i;
			while (i != fileData.end())
			{
				if ((*i).find("SCRIPTEND") == 0) break;
				code << *i << "\n";
				++i;
			}
			gameAni.m_script = code.toString();
		}
		// SPRITE
		// DEFAULTPARAMxx
		// ANI / ANIEND
		// WAIT
		// PLAYSOUND
		// REDBODY		- newworld, in lava
		// BLUEBODY		- newworld, in water

		if (i == fileData.end())
			break;
	}

	// Attempt to compile the script in GS2
	gameAni.m_bytecode.clear();
	if (!gameAni.m_script.empty() && server->hasNPCServer())
	{
		// Synchronous callback
		if (auto result = server->getNPCServer()->scripting.getCompiledClientScript(name, gameAni.m_script); result != nullptr)
		{
			auto bytecode = std::any_cast<std::vector<uint8_t>>(result->script.get());
			if (bytecode != nullptr)
			{
				gameAni.m_bytecode.clear(bytecode->size());
				gameAni.m_bytecode.write((const char*)bytecode->data(), static_cast<int>(bytecode->size()));
			}
		}
	}

	return gameAni;
}

CString GameAni::getBytecodePacket() const
{
	std::string_view gani = m_aniName;
	if (gani.ends_with(".gani"))
		gani = gani.substr(0, gani.length() - 5);

	CString out;
	// filename ".gani" protection
	if (!gani.empty() && !m_bytecode.isEmpty())
	{
		out >> (char)PLO_RAWDATA >> (int)(m_bytecode.length() + gani.length() + 1) << "\n";
		out >> (char)PLO_GANISCRIPT >> (char)gani.length() << std::string(gani) << m_bytecode;
	}

	return out;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
