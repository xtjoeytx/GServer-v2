#ifndef TGMAP_H
#define TGMAP_H

#include "CString.h"
#include <ctime>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

enum class MapType
{
	BIGMAP = 0,
	GMAP   = 1,
};

struct MapLevel
{
	MapLevel() : mapx(-1), mapy(-1) {}
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

	int mapx;
	int mapy;
};

class Server;

class Map
{
public:
	Map(MapType pType, bool pGroupMap = false);

	bool load(const CString& filename, Server* pServer);
	void loadMapLevels(Server* server) const;

	bool isLevelOnMap(const std::string& level, int& mx, int& my) const;
	const std::string& getLevelAt(int mx, int my) const;
	//int getLevelX(const std::string& level) const;
	//int getLevelY(const std::string& level) const;

	const std::string& getMapName() const { return mapName; }
	MapType getType() const { return type; }
	size_t getWidth() const { return width; }
	size_t getHeight() const { return height; }
	bool isBigMap() const { return type == MapType::BIGMAP; }
	bool isGmap() const { return type == MapType::GMAP; }
	bool isGroupMap() const { return groupMap; }

private:
	bool loadBigMap(const CString& pFileName, Server* pServer);
	bool loadGMap(const CString& pFileName, Server* pServer);

	MapType type;
	time_t modTime;
	size_t width;
	size_t height;
	bool groupMap;
	bool loadFullMap;
	std::string mapName;
	std::string mapImage;
	std::string miniMapImage;

	std::unordered_map<std::string, MapLevel> levels;
	std::vector<std::string> _levelList;
	std::vector<std::string> preloadLevelList;
};

#endif
