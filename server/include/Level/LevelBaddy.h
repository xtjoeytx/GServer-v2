#ifndef TLEVELBADDY_H
#define TLEVELBADDY_H

#include "CString.h"
#include "CTimeout.h"
#include "IUtil.h"
#include <memory>
#include <vector>

// Baddy props
enum
{
	BDPROP_ID = 0,
	BDPROP_X = 1,
	BDPROP_Y = 2,
	BDPROP_TYPE = 3,
	BDPROP_POWERIMAGE = 4,
	BDPROP_MODE = 5,
	BDPROP_ANI = 6,
	BDPROP_DIR = 7,
	BDPROP_VERSESIGHT = 8,
	BDPROP_VERSEHURT = 9,
	BDPROP_VERSEATTACK = 10,
	BDPROP_COUNT
};

// Baddy modes
enum
{
	BDMODE_WALK = 0,
	BDMODE_LOOK = 1,
	BDMODE_HUNT = 2,
	BDMODE_HURT = 3,
	BDMODE_BUMPED = 4,
	BDMODE_DIE = 5,
	BDMODE_SWAMPSHOT = 6,
	BDMODE_HAREJUMP = 7,
	BDMODE_OCTOSHOT = 8,
	BDMODE_DEAD = 9,
	BDMODE_COUNT
};

class Server;

class Level;

class LevelBaddy
{
public:
	LevelBaddy(const float pX, const float pY, const unsigned char pType, std::weak_ptr<Level> pLevel, Server* pServer);

	void reset();

	void dropItem();

	// get functions
	unsigned char getType() const { return m_type; }

	char getId() const { return m_id; }

	char getPower() const { return m_power; }

	char getMode() const { return m_mode; }

	char getAni() const { return m_ani; }

	char getDir() const { return m_dir; }

	float getX() const { return m_x; }

	float getY() const { return m_y; }

	float getStartX() const { return m_startX; }

	float getStartY() const { return m_startY; }

	CString getProp(const int propId, int clientVersion = CLVER_2_17) const;

	CString getProps(int clientVersion = CLVER_2_17) const;

	std::vector<CString> getVerses() const { return m_verses; };

	// set functions
	void setProps(CString& pProps);

	void setRespawn(const bool pRespawn) { m_canRespawn = pRespawn; }

	void setId(const char pId) { m_id = pId; }

	CTimeout timeout;

private:
	std::weak_ptr<Level> m_level;
	Server* m_server;
	unsigned char m_type;
	char m_id, m_power, m_mode, m_ani, m_dir;
	float m_x, m_y, m_startX, m_startY;
	CString m_image;
	std::vector<CString> m_verses;
	bool m_canRespawn;
	bool m_hasCustomImage;
};

using LevelBaddyPtr = std::unique_ptr<LevelBaddy>;

#endif // TLEVELBADDY_H
