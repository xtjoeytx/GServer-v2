#include "IDebug.h"
#include <map>
#include <vector>

#include "CFileSystem.h"
#include "TMap.h"
#include "TServer.h"

TMap::TMap(MapType pType, bool pGroupMap)
: type(pType), modTime(0), width(0), height(0), groupMap(pGroupMap), loadFullMap(false)
{
}

//TMap::TMap(MapType pType, const CString& pFileName, TServer* pServer, bool pGroupMap)
//: type(pType), modTime(0), width(0), height(0), groupMap(pGroupMap), loadFullMap(false)
//{
//	load(pFileName, pServer);
//}

bool TMap::load(const CString& pFileName, IMain* pServer)
{
	if (type == MapType::BIGMAP)
		return loadBigMap(pFileName, pServer);
	else if (type == MapType::GMAP)
		return loadGMap(pFileName, pServer);
	return true;
}

bool TMap::isLevelOnMap(const std::string& level, int& mapx, int& mapy) const
{
	auto it = levels.find(level);
	if (it != levels.end())
	{
		mapx = it->second.mapx;
		mapy = it->second.mapy;
		return true;
	}

	return false;
}

const std::string& TMap::getLevelAt(int mx, int my) const
{
	static const std::string emptyStr;

	if (mx < width && my < height)
		return _levelList[mx + my * width];

	return emptyStr;
}

bool TMap::loadBigMap(const CString& pFileName, IMain* pServer)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = pServer->getFileSystem();
	if ( !pServer->getSettings().getBool("nofoldersconfig", false))
		fileSystem = pServer->getFileSystem(FS_FILE);

	CString fileName = fileSystem->find(pFileName);
	modTime = fileSystem->getModTime(pFileName);
	mapName = pFileName.text();

	// Make sure the file exists.
	if (fileName.length() == 0) return false;

	// Load the gmap.
	std::vector<CString> fileData = CString::loadToken(fileName);

	// Parse it.
	levels.clear();
	width = 0;
	height = 0;

	std::vector<std::vector<CString>> mapData;

	for (auto& line : fileData)
	{
	    line = line.removeAll("\r").trim();
	    if (line.isEmpty())
            continue;

	    auto levelList = line.guntokenize().tokenize("\n", true);
        int empty = 0;
	    for (const auto& lvl : levelList) {
	        // dont calculate the width based on any extra padding
	        empty = (lvl.isEmpty() ? ++empty : 0);
	    }

	    // calculate width/height
	    auto currentWidth = levelList.size() - empty;
        height++;
	    if (width < currentWidth)
	        width = currentWidth;

        mapData.push_back(levelList);
    }

    {
        std::vector<std::string> levelMap(width * height);

        for (size_t my = 0; my < mapData.size(); my++)
        {
            for (size_t mx = 0; mx < mapData[my].size(); mx++)
            {
				if (mx < width)
				{
					std::string lcLevelName(mapData[my][mx].toLower().text());
					if (!lcLevelName.empty())
					{
						levelMap[mx + my * width] = lcLevelName;
						levels[lcLevelName] = SMapLevel(mx, my);
					}
				}
            }
        }

        _levelList = std::move(levelMap);
    }

	return true;
}

bool TMap::loadGMap(const CString& pFileName, IMain* pServer)
{
	// Get the appropriate filesystem.
	CFileSystem* fileSystem = pServer->getFileSystem();
	if ( !pServer->getSettings().getBool("nofoldersconfig", false))
		fileSystem = pServer->getFileSystem(FS_LEVEL);

	CString fileName = fileSystem->find(pFileName);
	modTime = fileSystem->getModTime(pFileName);
	mapName = pFileName.text();

	// Make sure the file exists.
	if (fileName.length() == 0) return false;

	levels.clear();
	width = 0;
	height = 0;

	// Load the gmap.
	std::vector<CString> fileData = CString::loadToken(fileName);

	// Parse it.
	for (auto it = fileData.begin(); it != fileData.end(); ++it)
	{
		// Tokenize
		std::vector<CString> curLine = it->removeAll("\r").tokenize();
		if (curLine.empty())
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
			++it;
			int gmapy = 0;

            std::vector<std::string> levelMap(width * height);

            while (it != fileData.end())
			{
				CString line = it->removeAll("\r").trim();
				if (line.length() == 0) { ++it; continue; }
				if (line == "LEVELNAMESEND") break;

				if (gmapy < height)
				{
				    int gmapx = 0;

                    // Untokenize the level names and put them into a vector for easy loading.
                    line.guntokenizeI();
                    std::vector<CString> names = line.tokenize("\n");
                    for (auto &levelName : names)
                    {
                        if (gmapx < width)
                        {
                            // Check for blank levels.
							if (levelName != "\r")
							{
								std::string lcLevelName(levelName.toLower().text());
								levelMap[gmapx + gmapy * width] = lcLevelName;
								levels[lcLevelName] = SMapLevel(gmapx, gmapy);
							}

                            ++gmapx;
                        }
                    }

                    ++gmapy;
                }

				++it;
			}

            _levelList = std::move(levelMap);
		}
		else if (curLine[0] == "MAPIMG")
		{
			if (curLine.size() != 2)
				continue;

			mapImage = curLine[1].text();
		}
		else if (curLine[0] == "MINIMAPIMG")
		{
			if (curLine.size() != 2)
				continue;

			miniMapImage = curLine[1].text();
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

			++it;
			while (it != fileData.end())
			{
				CString line = it->removeAll("\r");
				if (line == "LOADATSTARTEND") break;

				line.guntokenizeI();
				std::vector<CString> names = line.tokenize("\n");
				for (auto& levelName : names) {
					preloadLevelList.push_back(levelName.toLower().text());
				}
			}
		}
		// TODO: 3D settings maybe?
	}

	return true;
}

void TMap::loadMapLevels(TServer *server) const
{
	if (loadFullMap)
	{
		for (const auto& levelName : _levelList)
		{
			if (!levelName.empty())
			{
				auto lvl = server->getLevel(levelName);
				assert(lvl);
			}
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
