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
	float getTileX() const { return position.x() / 16.0f; }
	float getTileY() const { return position.y() / 16.0f; }

	PixelPosition position;
	uint8_t power;
	ScriptObject owner;
	TimeoutGenerator timeout;

	[[inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline void LevelBomb::constructScriptParameters()
{
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"x"sv, std::nullopt, std::ref(position.x()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"y"sv, std::nullopt, std::ref(position.y()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::TimeoutProperty{"time"sv, std::ref(timeout)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"power"sv, std::nullopt, std::ref(power)});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBOMB_H
