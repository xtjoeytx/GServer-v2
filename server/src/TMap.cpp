#include "IDebug.h"
#include <map>
#include <vector>

#include "CFileSystem.h"
#include "TMap.h"
#include "TServer.h"

TMap::TMap(int pType, bool pGroupMap)
: type(pType), modTime(0), width(0), height(0), groupMap(pGroupMap), loadFullMap(false)
{
}

TMap::TMap(int pType, const CString& pFileName, TServer* pServer, bool pGroupMap)
: type(pType), modTime(0), width(0), height(0), groupMap(pGroupMap), loadFullMap(false)
{
	load(pFileName, pServer);
}

bool TMap::load(const CString& pFileName, TServer* pServer)
{
	if (type == MAPTYPE_BIGMAP)
		return loadBigMap(pFileName, pServer);
	else if (type == MAPTYPE_GMAP)
		return loadGMap(pFileName, pServer);
	return true;
}

bool TMap::isLevelOnMap(const CString& level) const
{
	for (std::map<CString, SMapLevel>::const_iterator i = levels.begin(); i != levels.end(); ++i)
	{
		if (i->first == level)
			return true;
	}
	return false;
}

CString TMap::getLevelAt(int x, int y) const
{
	for (std::map<CString, SMapLevel>::const_iterator i = levels.begin(); i != levels.end(); ++i)
	{
		if (i->second.mapx == x && i->second.mapy == y)
			return i->first;
	}
	return CString();
}

int TMap::getLevelX(const CString& level) const
{
	if (levels.empty()) return 0;
	return levels.find(level)->second.mapx;
}

int TMap::getLevelY(const CString& level) const
{
	if (levels.empty()) return 0;
	return levels.find(level)->second.mapy;
}

bool TMap::loadBigMap(const CString& pFileName, TServer* pServer)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = pServer->getFileSystem();
	if ( !pServer->getSettings()->getBool("nofoldersconfig", false))
		fileSystem = pServer->getFileSystem(FS_FILE);

	CString fileName = fileSystem->find(pFileName);
	modTime = fileSystem->getModTime(pFileName);
	mapName = pFileName;

	// Make sure the file exists.
	if (fileName.length() == 0) return false;

	// Load the gmap.
	std::vector<CString> fileData = CString::loadToken(fileName);

	// Parse it.
	std::vector<CString>::iterator i = fileData.begin();
	levels.clear();

	int bmapx = 0;
	int bmapy = 0;
	while (i != fileData.end())
	{
		CString line = i->removeAll("\r").trim();
		if (line.length() == 0) { ++i; continue; }

		// Untokenize the level names and put them into a vector for easy loading.
		line.guntokenizeI();
		std::vector<CString> names = line.tokenize("\n");
		for (std::vector<CString>::iterator j = names.begin(); j != names.end(); ++j)
		{
			// Check for blank levels.
			if (*j == "\r")
			{
				++bmapx;
				continue;
			}

			// Save the level into the map.
			SMapLevel lvl(bmapx++, bmapy);
			levels[*j] = lvl;
		}

		if (bmapx > width) width = bmapx;
		bmapx = 0;
		++bmapy;
		++i;
	}
	height = bmapy;

	return true;
}

bool TMap::loadGMap(const CString& pFileName, TServer* pServer)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = pServer->getFileSystem();
	if ( !pServer->getSettings()->getBool("nofoldersconfig", false))
		fileSystem = pServer->getFileSystem(FS_LEVEL);

	CString fileName = fileSystem->find(pFileName);
	modTime = fileSystem->getModTime(pFileName);
	mapName = pFileName;

	// Make sure the file exists.
	if (fileName.length() == 0) return false;

	// Load the gmap.
	std::vector<CString> fileData = CString::loadToken(fileName);

	// Parse it.
	for (std::vector<CString>::iterator i = fileData.begin(); i != fileData.end(); ++i)
	{
		// Tokenize
		std::vector<CString> curLine = i->removeAll("\r").tokenize();
		if (curLine.size() < 1)
			continue;

		// Parse Each Type
		if (curLine[0] == "WIDTH")
		{
			if (curLine.size() != 2)
				continue;

			width = strtoint(curLine[1]);
		}
		else if (curLine[0] == "HEIGHT")
		{
			if (curLine.size() != 2)
				continue;

			height = strtoint(curLine[1]);
		}
		else if (curLine[0] == "GENERATED")
		{
			if (curLine.size() != 2)
				continue;

			// Not really needed.
		}
		else if (curLine[0] == "LEVELNAMES")
		{
			levels.clear();

			++i;
			int gmapx = 0;
			int gmapy = 0;
			while (i != fileData.end())
			{
				CString line = i->removeAll("\r").trim();
				if (line.length() == 0) { ++i; continue; }
				if (line == "LEVELNAMESEND") break;

				// Untokenize the level names and put them into a vector for easy loading.
				line.guntokenizeI();
				std::vector<CString> names = line.tokenize("\n");
				for (std::vector<CString>::iterator j = names.begin(); j != names.end(); ++j)
				{
					// Check for blank levels.
					if (*j == "\r")
					{
						++gmapx;
						continue;
					}

					// Save the level into the map.
					SMapLevel lvl(gmapx++, gmapy);
					levels[*j] = lvl;
				}

				gmapx = 0;
				++gmapy;
				++i;
			}
		}
		else if (curLine[0] == "MAPIMG")
		{
			if (curLine.size() != 2)
				continue;
			
			mapImage = curLine[1];
		}
		else if (curLine[0] == "MINIMAPIMG")
		{
			if (curLine.size() != 2)
				continue;

			miniMapImage = curLine[1];
		}
		else if (curLine[0] == "NOAUTOMAPPING")
		{
			// Clientside only.
		}
		else if (curLine[0] == "LOADFULLMAP")
		{
			loadFullMap = true;
		}
		else if (curLine[0] == "LOADATSTART")
		{
			loadFullMap = false;

			// TODO(joey):Untested, but should work
			++i;
			while (i != fileData.end())
			{
				CString line = i->removeAll("\r");
				if (line == "LOADATSTARTEND") break;

				line.guntokenizeI();
				std::vector<CString> names = line.tokenize("\n");
				for (auto& levelName : names) {
					preloadLevelList.push_back(levelName);
				}
			}
		}
		// TODO: 3D settings maybe?
	}

	return true;
}

CString TMap::getLevels() const
{
	CString retVal;
	
	for (std::map<CString, SMapLevel>::const_iterator i = levels.begin(); i != levels.end(); ++i)
	{
		retVal << i->first << "\n";
	}
	
	return retVal;
}

void TMap::loadMapLevels(TServer *server) const
{
	if (loadFullMap)
	{
		for (std::map<CString, SMapLevel>::const_iterator i = levels.begin(); i != levels.end(); ++i)
		{
			auto lvl = server->getLevel(i->first);
			assert(lvl);
		}
	}
	else if (!preloadLevelList.empty())
	{
		for (auto& level : preloadLevelList)
		{
			auto lvl = server->getLevel(level);
			assert(lvl);
		}
	}
}
