#include "Animation/GameAni.h"
#include "GS2Context.h"
#include "Server.h"
#include <filesystem>

std::optional<GameAni> GameAni::load(Server* const server, const std::string& name)
{
	auto fileSystem = server->getFileSystem(FS_FILE);

	// Search for the file in the filesystem
	auto filePath = fileSystem->find(name);
	if (filePath.isEmpty())
		return std::nullopt;

	// Load the animation file for parsing
	std::vector<CString> fileData = CString::loadToken(filePath, "\n", true);
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

		if (i == fileData.end())
			break;
	}

	// Attempt to compile the script in GS2
	if (!gameAni.m_script.empty())
	{
		// Synchronous callback
		server->compileGS2Script(gameAni.m_script, [&gameAni](const CompilerResponse& response)
								   {
									   if (response.success)
									   {
										   gameAni.m_bytecode.clear(response.bytecode.length());
										   gameAni.m_bytecode.write((const char*)response.bytecode.buffer(), response.bytecode.length());
									   }
									   else
										   gameAni.m_bytecode.clear();
								   });
	}

	return gameAni;
}

PlayerOutPackets GameAni::getBytecodePackets(bool newProtocol) const
{
	PlayerOutPackets packets;
	std::string_view gani = m_aniName;
	if (gani.ends_with(".gani"))
		gani = gani.substr(0, gani.length() - 5);

	// filename ".gani" protection
	if (!gani.empty() && !m_bytecode.isEmpty())
	{
		if (!newProtocol) {
			packets.push_back({PLO_RAWDATA, CString() >> (int)(m_bytecode.length() + gani.length() + 1) << "\n"});
			packets.push_back({PLO_GANISCRIPT, CString() >> (char)gani.length() << std::string(gani) << m_bytecode});
		}
		else
			packets.push_back({PLO_GANISCRIPT, CString() >> (char)gani.length() << std::string(gani) << m_bytecode});
	}

	return packets;
}
