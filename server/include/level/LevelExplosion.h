#ifndef LEVELEXPLOSION_H
#define LEVELEXPLOSION_H

#include <chrono>
#include <cstdint>

#include <scripting/ScriptContainers.h>
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
	string_map<GameValue> scriptParameters;
};

//----------------------------

inline void LevelExplosion::constructScriptParameters()
{
	scriptParameters.try_emplace("x", set_temporary, "x", gameValueGetter([this]() { return position.x() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameValueGetter([this]() { return position.y() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("power", set_temporary, "power", gameValueGetter(power), GameValue::func_set{});
	scriptParameters.try_emplace("time", set_temporary, "time",
		gameValueGetter([this]() { return std::chrono::duration_cast<duration_seconds_double>(timeout.getRemainingTime()).count(); }),
		GameValue::func_set{});
	scriptParameters.try_emplace("dir", set_temporary, "dir", gameValueGetter(direction), GameValue::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELEXPLOSION_H
