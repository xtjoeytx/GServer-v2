#include <filesystem>
#include "GS2Context.h"
#include "Animation/GameAni.h"
#include "Server.h"

std::optional<TGameAni> TGameAni::load(TServer* const server, const std::string& name)
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
	
	TGameAni gameAni(name);
	
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
				gameAni._aniFlags |= AniFlags::Continous;
			else
				gameAni._aniFlags &= ~(AniFlags::Continous);
		}
		else if (curLine[0] == "LOOP")
		{
			if (curLine.size() == 1 || strtoint(curLine[1]) != 0)
				gameAni._aniFlags |= AniFlags::LoopAnimation;
			else
				gameAni._aniFlags &= ~(AniFlags::LoopAnimation);
		}
		else if (curLine[0] == "SINGLEDIRECTION")
		{
			if (curLine.size() == 1 || strtoint(curLine[1]) != 0)
				gameAni._aniFlags |= AniFlags::SingleDirOnly;
			else
				gameAni._aniFlags &= ~(AniFlags::SingleDirOnly);
		}
		else if (curLine[0] == "SETBACKTO")
		{
			if (curLine.size() >= 2)
				gameAni._setBackTo = curLine[1].toString();
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
			gameAni._script = code.toString();
		}

		if (i == fileData.end())
			break;
	}
	
	// Attempt to compile the script in GS2
	if (!gameAni._script.empty())
	{
		// Synchronous callback
		server->compileGS2Script(gameAni._script, [&gameAni](const CompilerResponse &response)
		{
			if (response.success)
			{
				gameAni._bytecode.clear(response.bytecode.length());
				gameAni._bytecode.write((const char *)response.bytecode.buffer(), response.bytecode.length());
			}
			else gameAni._bytecode.clear();
		});
	}
	
	return gameAni;
}

CString TGameAni::getBytecodePacket() const
{
	std::string_view gani = _aniName;
	if (gani.ends_with(".gani"))
		gani = gani.substr(0, gani.length() - 5);

	CString out;
	// filename ".gani" protection
	if (!gani.empty() && !_bytecode.isEmpty())
	{
		out >> (char)PLO_RAWDATA >> (int)(_bytecode.length() + gani.length() + 1) << "\n";
		out >> (char)PLO_GANISCRIPT >> (char)gani.length() << std::string(gani) << _bytecode;
	}

	return out;
}
