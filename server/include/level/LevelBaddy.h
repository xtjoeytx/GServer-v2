#ifndef LEVELBADDY_H
#define LEVELBADDY_H

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
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

/// @brief The names of the baddies. If using, spider needs to be manually mapped to octopus.
inline constexpr std::array<std::string_view, 11> BaddyNames =
{
	"graysoldier"sv, "bluesoldier"sv, "redsoldier"sv, "shootingsoldier"sv, "swampsoldier"sv,
	"frog"sv, "octopus"sv, "goldenwarrior"sv, "lizardon"sv, "dragon"sv, "spider"sv
};

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
class Server;

class LevelBaddy
{
public:
	static BaddyType getBaddyTypeFromString(const std::string& type);

public:
	LevelBaddy(const LocalPixelPosition& position, BaddyType type, std::weak_ptr<Level> level);

	void reset();
	void dropItem() const;
	bool isAlive() const { return mode != BaddyMode::DEAD; }
	bool canRespawn() const { return m_canRespawn; }
	bool canBeReplaced() const { return !m_canRespawn && mode == BaddyMode::DEAD; }

public:
	CString getProp(BaddyProp propId) const;
	CString getProps() const;
	void setPropsFromPacket(CString& pProps);

public:
	void setRespawn(const bool pRespawn) { m_canRespawn = pRespawn; }
	void setImage(std::string_view image);
	[[a::inline]] void setLevel(LevelPtr level);

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
	[[a::inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;

private:
	Server* m_server;
	std::weak_ptr<Level> m_level;
	LocalPixelPosition m_originalPosition;
	bool m_canRespawn = true;
	bool m_hasCustomImage = false;
};

//----------------------------

inline void LevelBaddy::setLevel(LevelPtr level)
{
	m_level = level;
}

inline void LevelBaddy::constructScriptParameters()
{
	// TODO: headdir
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"x"sv, std::nullopt, std::ref(position.x()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"y"sv, std::nullopt, std::ref(position.y()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"dir"sv, std::nullopt, std::ref(direction)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"type"sv, std::nullopt, std::ref(type)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"headdir"sv, std::nullopt, std::ref(headDirection)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"power"sv, std::nullopt, std::ref(power)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"mode"sv, std::nullopt, std::ref(mode)});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBADDY_H
