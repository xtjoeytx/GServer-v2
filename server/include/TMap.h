#ifndef TGMAP_H
#define TGMAP_H

#include <map>
#include <time.h>
#include "CString.h"

enum
{
	MAPTYPE_BIGMAP	= 0,
	MAPTYPE_GMAP	= 1,
};

struct SMapLevel
{
	SMapLevel() : mapx(-1), mapy(-1) {}
	SMapLevel(int x, int y) : mapx(x), mapy(y) {}
	SMapLevel(const SMapLevel& level)
	{
		mapx = level.mapx;
		mapy = level.mapy;
	}

	SMapLevel& operator=(const SMapLevel& level)
	{
		mapx = level.mapx;
		mapy = level.mapy;
		return *this;
	}

	int mapx;
	int mapy;
};

class TServer;

class TMap
{
	public:
		TMap(int pType, bool pGroupMap = false);
		TMap(int pType, const CString& pFileName, TServer* pServer, bool pGroupMap = false);

		bool load(const CString& filename, TServer* pServer);

		bool isLevelOnMap(const CString& level) const;

		CString getLevelAt(int x, int y) const;
		int getLevelX(const CString& level) const;
		int getLevelY(const CString& level) const;
		CString getMapName() const			{ return mapName; }
		int getType() const					{ return type; }
		bool isGroupMap() const				{ return groupMap; }
		CString getLevels();

	private:
		bool loadBigMap(const CString& pFileName, TServer* pServer);
		bool loadGMap(const CString& pFileName, TServer* pServer);

		int type;
		CString mapName;
		time_t modTime;
		int width;
		int height;
		bool groupMap;
		CString mapImage;
		CString miniMapImage;
		//bool loadFullMap;
		std::map<CString, SMapLevel> levels;
};

#endif
