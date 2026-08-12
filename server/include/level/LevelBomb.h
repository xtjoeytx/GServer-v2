#ifndef LEVELBOMB_H
#define LEVELBOMB_H

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

struct LevelBomb
{
	[[a::inline]] float getTileX() const;
	[[a::inline]] float getTileY() const;

	PixelPosition position;
	uint8_t power;
	ScriptObject owner;
	TimeoutGenerator timeout;

	[[a::inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline float LevelBomb::getTileX() const
{
	return static_cast<float>(position.x()) / 16.0f;
}

inline float LevelBomb::getTileY() const
{
	return static_cast<float>(position.y()) / 16.0f;
}

inline void LevelBomb::constructScriptParameters()
{
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{.name = "x"sv, .modTime = std::nullopt, .value = std::ref(position.x()), .factor = 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{.name = "y"sv, .modTime = std::nullopt, .value = std::ref(position.y()), .factor = 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::TimeoutProperty{.name = "time"sv, .value = std::ref(timeout)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "power"sv, .modTime = std::nullopt, .value = std::ref(power)});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBOMB_H
