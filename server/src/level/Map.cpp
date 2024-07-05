#include <map>
#include <vector>

#include <IDebug.h>

#include "FileSystem.h"
#include "Server.h"
#include "level/Map.h"

Map::Map(MapType pType, bool pGroupMap)
	: m_type(pType), m_modTime(0), m_width(0), m_height(0), m_groupMap(pGroupMap), m_loadFullMap(false)
{
}

//Map::Map(MapType pType, const CString& pFileName, Server* pServer, bool pGroupMap)
//: m_type(pType), m_modTime(0), m_width(0), m_height(0), m_groupMap(pGroupMap), m_loadFullMap(false)
//{
//	load(pFileName, pServer);
//}

bool Map::load(const CString& pFileName, Server* pServer)
{
	if (m_type == MapType::BIGMAP)
		return loadBigMap(pFileName, pServer);
	else if (m_type == MapType::GMAP)
		return loadGMap(pFileName, pServer);
	return true;
}

bool Map::isLevelOnMap(const std::string& level, int& mapx, int& mapy) const
{
	auto it = m_levels.find(level);
	if (it != m_levels.end())
	{
		mapx = it->second.mapx;
		mapy = it->second.mapy;
		return true;
	}

	return false;
}

const std::string& Map::getLevelAt(int mx, int my) const
{
	static const std::string emptyStr;

	if (mx < m_width && my < m_height)
		return m_levelList[mx + my * m_width];

	return emptyStr;
}

bool Map::loadBigMap(const CString& pFileName, Server* pServer)
{
	// Get the appropriate filesystem.
	FileSystem* fileSystem = pServer->getFileSystem();
	if (!pServer->getSettings().getBool("nofoldersconfig", false))
		fileSystem = pServer->getFileSystem(FS_FILE);

	CString fileName = fileSystem->find(pFileName);
	m_modTime = fileSystem->getModTime(pFileName);
	m_mapName = pFileName.text();

	// Make sure the file exists.
	if (fileName.length() == 0) return false;

	// Load the gmap.
	std::vector<CString> fileData = CString::loadToken(fileName);

	// Parse it.
	m_levels.clear();
	m_width = 0;
	m_height = 0;

	std::vector<std::vector<CString>> mapData;

	for (auto& line: fileData)
	{
		line = line.removeAll("\r").trim();
		if (line.isEmpty())
			continue;

		auto levelList = line.guntokenize().tokenize("\n", true);
		int empty = 0;
		for (const auto& lvl: levelList)
		{
			// dont calculate the width based on any extra padding
			empty = (lvl.isEmpty() ? ++empty : 0);
		}

		// calculate width/height
		auto currentWidth = levelList.size() - empty;
		m_height++;
		if (m_width < currentWidth)
			m_width = currentWidth;

		mapData.push_back(levelList);
	}

	{
		std::vector<std::string> levelMap(m_width * m_height);

		for (size_t my = 0; my < mapData.size(); my++)
		{
			for (size_t mx = 0; mx < mapData[my].size(); mx++)
			{
				if (mx < m_width)
				{
					std::string lcLevelName(mapData[my][mx].toLower().text());
					if (!lcLevelName.empty())
					{
						levelMap[mx + my * m_width] = lcLevelName;
						m_levels[lcLevelName] = MapLevel(static_cast<int>(mx), static_cast<int>(my));
					}
				}
			}
		}

		m_levelList = std::move(levelMap);
	}

	return true;
}

bool Map::loadGMap(const CString& pFileName, Server* pServer)
{
	// Get the appropriate filesystem.
	FileSystem* fileSystem = pServer->getFileSystem();
	if (!pServer->getSettings().getBool("nofoldersconfig", false))
		fileSystem = pServer->getFileSystem(FS_LEVEL);

	CString fileName = fileSystem->find(pFileName);
	m_modTime = fileSystem->getModTime(pFileName);
	m_mapName = pFileName.text();

	// Make sure the file exists.
	if (fileName.length() == 0) return false;

	m_levels.clear();
	m_width = 0;
	m_height = 0;

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

			m_width = strtoint(curLine[1]);
		}
		else if (curLine[0] == "HEIGHT")
		{
			if (curLine.size() != 2)
				continue;

			m_height = strtoint(curLine[1]);
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

			std::vector<std::string> levelMap(m_width * m_height);

			while (it != fileData.end())
			{
				CString line = it->removeAll("\r").trim();
				if (line.length() == 0)
				{
					++it;
					continue;
				}
				if (line == "LEVELNAMESEND") break;

				if (gmapy < m_height)
				{
					int gmapx = 0;

					// Untokenize the level names and put them into a vector for easy loading.
					line.guntokenizeI();
					std::vector<CString> names = line.tokenize("\n");
					for (auto& levelName: names)
					{
						if (gmapx < m_width)
						{
							// Check for blank levels.
							if (levelName != "\r")
							{
								std::string lcLevelName(levelName.toLower().text());
								levelMap[gmapx + gmapy * m_width] = lcLevelName;
								m_levels[lcLevelName] = MapLevel(gmapx, gmapy);
							}

							++gmapx;
						}
					}

					++gmapy;
				}

				++it;
			}

			m_levelList = std::move(levelMap);
		}
		else if (curLine[0] == "MAPIMG")
		{
			if (curLine.size() != 2)
				continue;

			m_mapImage = curLine[1].text();
		}
		else if (curLine[0] == "MINIMAPIMG")
		{
			if (curLine.size() != 2)
				continue;

			m_miniMapImage = curLine[1].text();
		}
		else if (curLine[0] == "NOAUTOMAPPING")
		{
			// Clientside only.
		}
		else if (curLine[0] == "LOADFULLMAP")
		{
			m_loadFullMap = true;
		}
		else if (curLine[0] == "LOADATSTART")
		{
			m_loadFullMap = false;

			++it;
			while (it != fileData.end())
			{
				CString line = it->removeAll("\r");
				if (line == "LOADATSTARTEND") break;

				line.guntokenizeI();
				std::vector<CString> names = line.tokenize("\n");
				for (auto& levelName: names)
				{
					m_preloadLevelList.push_back(levelName.toLower().text());
				}
			}
		}
		// TODO: 3D settings maybe?
	}

	return true;
}

void Map::loadMapLevels(Server* server) const
{
	if (m_loadFullMap)
	{
		for (const auto& levelName: m_levelList)
		{
			if (!levelName.empty())
			{
				auto lvl = server->getLevel(levelName);
				assert(lvl);
			}
		}
	}
	else if (!m_preloadLevelList.empty())
	{
		for (auto& level: m_preloadLevelList)
		{
			auto lvl = server->getLevel(level);
			assert(lvl);
		}
	}
}
