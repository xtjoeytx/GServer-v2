#ifndef LEVELEXPLOSION_H
#define LEVELEXPLOSION_H

#include <chrono>
#include <cstdint>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/TimeoutGenerator.h>

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
	TimeoutGenerator timeout;

	[[inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline void LevelExplosion::constructScriptParameters()
{
	scriptParameters.try_emplace("x", set_temporary, "x", gameVariableGetter([this]() { return position.x() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameVariableGetter([this]() { return position.y() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("power", set_temporary, "power", gameVariableGetter([this]() { return (double)power; }), GameVariable::func_set{});
	scriptParameters.try_emplace("time", set_temporary, "time",
		gameVariableGetter([this]() { return std::chrono::duration_cast<duration_seconds_double>(timeout.getRemainingTime()).count(); }),
		GameVariable::func_set{});
	scriptParameters.try_emplace("dir", set_temporary, "dir", gameVariableGetter([this]() { return (double)direction; }), GameVariable::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELEXPLOSION_H
