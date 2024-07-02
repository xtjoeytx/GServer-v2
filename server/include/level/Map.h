#ifndef TGMAP_H
#define TGMAP_H

#include <ctime>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <CString.h>
#include "BabyDI.h"

enum class MapType
{
	BIGMAP = 0,
	GMAP = 1,
};

struct MapLevel
{
	MapLevel() = default;
	MapLevel(int x, int y) : mapx(x), mapy(y) {}
	MapLevel(const MapLevel& level)
	{
		mapx = level.mapx;
		mapy = level.mapy;
	}

	MapLevel& operator=(const MapLevel& level)
	{
		mapx = level.mapx;
		mapy = level.mapy;
		return *this;
	}

	int mapx = -1;
	int mapy = -1;
};

class Server;

class Map
{
public:
	Map(MapType pType, bool pGroupMap = false);

	bool load(const CString& filename);
	void loadMapLevels() const;

	bool isLevelOnMap(const std::string& level, int& mx, int& my) const;
	const std::string& getLevelAt(int mx, int my) const;
	//int getLevelX(const std::string& level) const;
	//int getLevelY(const std::string& level) const;

	const std::string& getMapName() const { return m_mapName; }
	MapType getType() const { return m_type; }
	size_t getWidth() const { return m_width; }
	size_t getHeight() const { return m_height; }
	bool isBigMap() const { return m_type == MapType::BIGMAP; }
	bool isGmap() const { return m_type == MapType::GMAP; }
	bool isGroupMap() const { return m_groupMap; }

private:
	bool loadBigMap(const CString& pFileName);
	bool loadGMap(const CString& pFileName);

	BabyDI_INJECT(Server, m_server);

	MapType m_type;
	time_t m_modTime = 0;
	size_t m_width = 0;
	size_t m_height = 0;
	bool m_groupMap;
	bool m_loadFullMap = false;
	std::string m_mapName;
	std::string m_mapImage;
	std::string m_miniMapImage;

	std::unordered_map<std::string, MapLevel> m_levels;
	std::vector<std::string> m_levelList;
	std::vector<std::string> m_preloadLevelList;
};

#endif
