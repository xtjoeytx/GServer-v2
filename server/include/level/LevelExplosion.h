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
	float getTileX() const { return position.x() / 16.0f; }
	float getTileY() const { return position.y() / 16.0f; }

	PixelPosition position;
	uint8_t power;
	uint8_t direction;
	ScriptObject from;
	TimeoutGenerator timeout;

	[[inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline void LevelExplosion::constructScriptParameters()
{
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"x"sv, std::nullopt, std::ref(position.x()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"y"sv, std::nullopt, std::ref(position.y()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::TimeoutProperty{"time"sv, std::ref(timeout)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"dir"sv, std::nullopt, std::ref(direction)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"power"sv, std::nullopt, std::ref(power)});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELEXPLOSION_H
