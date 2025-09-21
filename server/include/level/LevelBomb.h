#ifndef LEVELBOMB_H
#define LEVELBOMB_H

#include <chrono>
#include <cstdint>

#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>

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
	ScriptObject owner;
	TimeoutGenerator timeout;

	[[inline]] void constructScriptParameters();
	string_map<GameValue> scriptParameters;
};

//----------------------------

inline void LevelBomb::constructScriptParameters()
{
	scriptParameters.try_emplace("x", set_temporary, "x", gameValueGetter([this]() { return position.x() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameValueGetter([this]() { return position.y() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("power", set_temporary, "power", gameValueGetter(power), GameValue::func_set{});
	scriptParameters.try_emplace("time", set_temporary, "time",
		gameValueGetter([this]() { return std::chrono::duration_cast<duration_seconds_double>(timeout.getRemainingTime()).count(); }),
		GameValue::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBOMB_H
