#ifndef LEVELBOMB_H
#define LEVELBOMB_H

#include <chrono>
#include <cstdint>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/TimeoutGenerator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct LevelBomb
{
	float getTileX() const { return position.x() / 16.0f; }
	float getTileY() const { return position.y() / 16.0f; }

	PixelPosition position;
	uint8_t power;
	TimeoutGenerator timeout;

	[[inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline void LevelBomb::constructScriptParameters()
{
	scriptParameters.try_emplace("x", set_temporary, "x", gameVariableGetter([this]() { return position.x() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameVariableGetter([this]() { return position.y() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("power", set_temporary, "power", gameVariableGetter([this]() { return (double)power; }), GameVariable::func_set{});
	scriptParameters.try_emplace("time", set_temporary, "time",
		gameVariableGetter([this]() { return std::chrono::duration_cast<duration_seconds_double>(timeout.getRemainingTime()).count(); }),
		GameVariable::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBOMB_H
