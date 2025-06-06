#ifndef LEVELBADDY_H
#define LEVELBADDY_H

#include <memory>
#include <vector>

#include <CString.h>
#include <CTimeout.h>
#include <IUtil.h>
#include "BabyDI.h"

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/// @brief Baddy types
enum class BaddyType : uint8_t
{
	GRAYSOLDIER = 0,
	BLUESOLDIER = 1,
	REDSOLDIER = 2,
	SHOOTINGSOLDIER = 3,
	SWAMPSOLDIER = 4,
	FROG = 5,
	OCTOPUS = 6,
	GOLDENWARRIOR = 7,
	LIZARDON = 8,
	DRAGON = 9,
	COUNT
};
constexpr size_t BADDYTYPE_COUNT = static_cast<size_t>(BaddyType::COUNT);

//----------------------------

/// @brief Baddy props
enum class BaddyProp : uint8_t
{
	ID = 0,
	X = 1,
	Y = 2,
	TYPE = 3,
	POWERIMAGE = 4,
	MODE = 5,
	ANI = 6,
	DIR = 7,
	VERSESIGHT = 8,
	VERSEHURT = 9,
	VERSEATTACK = 10,
	COUNT
};
constexpr size_t BADDYPROP_COUNT = static_cast<size_t>(BaddyProp::COUNT);

//----------------------------

/// @brief Baddy modes
enum class BaddyMode : uint8_t
{
	WALK = 0,
	LOOK = 1,
	HUNT = 2,
	HURT = 3,
	BUMPED = 4,
	DIE = 5,
	SWAMPSHOT = 6,
	HAREJUMP = 7,
	OCTOSHOT = 8,
	DEAD = 9,
	COUNT
};
constexpr size_t BADDYMODE_COUNT = static_cast<size_t>(BaddyMode::COUNT);

//----------------------------

class Server;
class Level;
class LevelBaddy
{
public:
	LevelBaddy(float x, float y, BaddyType type, std::weak_ptr<Level> level);

	void reset();
	void dropItem();
	bool canRespawn() const { return m_canRespawn; }
	bool canBeReplaced() const { return !m_canRespawn && mode == BaddyMode::DEAD; }

public:
	CString getProp(BaddyProp propId, int clientVersion = CLVER_2_17) const;
	CString getProps(int clientVersion = CLVER_2_17) const;
	void setPropsFromPacket(CString& pProps);

public:
	void setRespawn(const bool pRespawn) { m_canRespawn = pRespawn; }
	void setImage(std::string_view image);

public:
	uint8_t id;
	BaddyType type;
	int8_t x;
	int8_t y;
	BaddyMode mode;
	uint8_t power;
	uint8_t animation;
	uint8_t direction;
	std::string image;
	std::vector<std::string> verses;

	CTimeout timeout;

private:
	BabyDI_INJECT(Server, m_server);

	std::weak_ptr<Level> m_level;
	int8_t m_originalX, m_originalY;
	bool m_canRespawn = true;
	bool m_hasCustomImage = false;
};

using LevelBaddyPtr = std::unique_ptr<LevelBaddy>;

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBADDY_H
