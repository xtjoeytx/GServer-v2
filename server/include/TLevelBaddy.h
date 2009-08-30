#ifndef TLEVELBADDY_H
#define TLEVELBADDY_H

#include <vector>
#include "ICommon.h"
#include "CString.h"
#include "CTimeout.h"

// Baddy props
enum {
	BDPROP_ID			= 0,
	BDPROP_X			= 1,
	BDPROP_Y			= 2,
	BDPROP_TYPE			= 3,
	BDPROP_POWERIMAGE	= 4,
	BDPROP_MODE			= 5,
	BDPROP_ANI			= 6,
	BDPROP_DIR			= 7,
	BDPROP_VERSESIGHT	= 8,
	BDPROP_VERSEHURT	= 9,
	BDPROP_VERSEATTACK	= 10,
};

// Baddy modes
enum {
	BDMODE_WALK			= 0,
	BDMODE_LOOK			= 1,
	BDMODE_HUNT			= 2,
	BDMODE_HURT			= 3,
	BDMODE_BUMPED		= 4,
	BDMODE_DIE			= 5,
	BDMODE_SWAMPSHOT	= 6,
	BDMODE_HAREJUMP		= 7,
	BDMODE_OCTOSHOT		= 8,
	BDMODE_DEAD			= 9,
};

class TServer;
class TLevel;
class TLevelBaddy
{
	public:
		TLevelBaddy(const float pX, const float pY, const unsigned char pType, TLevel* pLevel, TServer* pServer);

		void reset();
		void dropItem();

		// get functions
		char getId() const						{ return id; }
		float getX() const						{ return x; }
		float getY() const						{ return y; }
		CString getProp(const int propId, int clientVersion = CLVER_2_17) const;
		CString getProps(int clientVersion = CLVER_2_17) const;

		// set functions
		void setProps(CString& pProps);
		void setRespawn(const bool pRespawn)	{ respawn = pRespawn; }
		void setId(const char pId)				{ id = pId; }

		CTimeout timeout;

	private:
		TLevel* level;
		TServer* server;
		unsigned char type;
		char id, power, mode, ani, dir;
		float x, y, startX, startY;
		CString image;
		std::vector<CString> verses;
		bool respawn;
		bool setImage;
};

#endif // TLEVELBADDY_H
