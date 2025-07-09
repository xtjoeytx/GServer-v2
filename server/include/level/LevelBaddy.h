#ifndef LEVELBADDY_H
#define LEVELBADDY_H

#include <cstdint>
#include <memory>
#include <string_view>
#include <string>
#include <vector>

#include <CString.h>
#include <IUtil.h>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/TimeoutGenerator.h>

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
	LevelBaddy(const PixelPosition& position, BaddyType type, std::weak_ptr<Level> level);

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
	PixelPosition position;
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
	string_map<GameVariable> scriptParameters;

private:
	std::weak_ptr<Level> m_level;
	Position<int16_t> m_originalPosition;
	bool m_canRespawn = true;
	bool m_hasCustomImage = false;
};

//----------------------------

inline void LevelBaddy::constructScriptParameters()
{
	// TODO: headdir
	scriptParameters.try_emplace("x", set_temporary, "x", gameVariableGetter([this]() { return position.x() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameVariableGetter([this]() { return position.y() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("type", set_temporary, "type", gameVariableGetter([this]() { return (double)type; }), GameVariable::func_set{});
	scriptParameters.try_emplace("dir", set_temporary, "dir", gameVariableGetter([this]() { return (double)direction; }), GameVariable::func_set{});
	scriptParameters.try_emplace("headdir", set_temporary, "headdir", gameVariableGetter([this]() { return (double)headDirection; }), GameVariable::func_set{});
	scriptParameters.try_emplace("power", set_temporary, "power", gameVariableGetter([this]() { return (double)power; }), GameVariable::func_set{});
	scriptParameters.try_emplace("mode", set_temporary, "mode", gameVariableGetter([this]() { return (double)mode; }), GameVariable::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBADDY_H
