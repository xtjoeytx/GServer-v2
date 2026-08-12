#ifndef LEVELEXPLOSION_H
#define LEVELEXPLOSION_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/* Data:

Indices of a power 2 explosion:

    2
    1
4 3 0 7 8
    5
    6

Each explosion is 2x2 tiles in size and are tiled without overlap.

*/

constexpr std::chrono::milliseconds ExplosionDuration = 300ms;

struct LevelExplosion
{
	[[a::inline]] float getTileX() const;
	[[a::inline]] float getTileY() const;

	PixelPosition position;
	uint8_t power;
	uint8_t direction;
	ScriptObject from;
	TimeoutGenerator timeout;

	[[a::inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline float LevelExplosion::getTileX() const
{
	return static_cast<float>(position.x()) / 16.0f;
}

inline float LevelExplosion::getTileY() const
{
	return static_cast<float>(position.y()) / 16.0f;
}

inline void LevelExplosion::constructScriptParameters()
{
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{.name = "x"sv, .modTime = std::nullopt, .value = std::ref(position.x()), .factor = 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{.name = "y"sv, .modTime = std::nullopt, .value = std::ref(position.y()), .factor = 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::TimeoutProperty{.name = "time"sv, .value = std::ref(timeout)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "dir"sv, .modTime = std::nullopt, .value = std::ref(direction)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "power"sv, .modTime = std::nullopt, .value = std::ref(power)});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELEXPLOSION_H
