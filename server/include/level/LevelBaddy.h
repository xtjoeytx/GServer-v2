#ifndef LEVELBADDY_H
#define LEVELBADDY_H

#include <cstdint>
#include <memory>
#include <string_view>
#include <string>
#include <vector>

#include <CString.h>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>

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

class Level;

class LevelBaddy
{
public:
	LevelBaddy(const LocalPixelPosition& position, BaddyType type, std::weak_ptr<Level> level);

	void reset();
	void dropItem() const;
	bool canRespawn() const { return m_canRespawn; }
	bool canBeReplaced() const { return !m_canRespawn && mode == BaddyMode::DEAD; }

public:
	CString getProp(BaddyProp propId) const;
	CString getProps() const;
	void setPropsFromPacket(CString& pProps);

public:
	void setRespawn(const bool pRespawn) { m_canRespawn = pRespawn; }
	void setImage(std::string_view image);

public:
	float getTileX() const { return position.x() / 16.0f; }
	float getTileY() const { return position.y() / 16.0f; }

public:
	uint8_t id;
	BaddyType type;
	LocalPixelPosition position;
	BaddyMode mode;
	uint8_t power;
	uint8_t animation;
	uint8_t direction;
	uint8_t headDirection;
	std::string image;
	std::vector<std::string> verses;

	TimeoutGenerator timeout;

public:
	[[inline]] void constructScriptParameters();
	string_map<GameValue> scriptParameters;

private:
	std::weak_ptr<Level> m_level;
	LocalPixelPosition m_originalPosition;
	bool m_canRespawn = true;
	bool m_hasCustomImage = false;
};

//----------------------------

inline void LevelBaddy::constructScriptParameters()
{
	// TODO: headdir
	scriptParameters.try_emplace("x", set_temporary, "x", gameValueGetter([this]() { return position.x() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameValueGetter([this]() { return position.y() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("type", set_temporary, "type", gameValueGetter([this]() { return (double)type; }), GameValue::func_set{});
	scriptParameters.try_emplace("dir", set_temporary, "dir", gameValueGetter(direction), GameValue::func_set{});
	scriptParameters.try_emplace("headdir", set_temporary, "headdir", gameValueGetter(headDirection), GameValue::func_set{});
	scriptParameters.try_emplace("power", set_temporary, "power", gameValueGetter(power), GameValue::func_set{});
	scriptParameters.try_emplace("mode", set_temporary, "mode", gameValueGetter([this]() { return (double)mode; }), GameValue::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBADDY_H
