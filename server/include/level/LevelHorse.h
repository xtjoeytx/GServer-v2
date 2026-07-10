#ifndef LEVELHORSE_H
#define LEVELHORSE_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <string>

#include <CString.h>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

constexpr uint8_t HORSETYPE_NORMAL = 0;
constexpr uint8_t HORSETYPE_BOAT = 1;

struct LevelHorse
{
	float getTileX() const { return position.x() / 16.0f; }
	float getTileY() const { return position.y() / 16.0f; }

	CString getPacket() const
	{
		auto localPosition = toLocalPixelPosition(position);
		char dir_bush = (bushes << 2) | (direction & 0x03);
		return CString() >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8) >> (char)dir_bush << image;
	}

	PixelPosition position;
	std::string image;
	uint8_t direction;
	uint8_t bushes;
	uint8_t bombs = 0;
	uint8_t bombpower = 0;
	uint8_t type;
	TimeoutGenerator timeout;

	[[inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline void LevelHorse::constructScriptParameters()
{
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"x"sv, std::nullopt, std::ref(position.x()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"y"sv, std::nullopt, std::ref(position.y()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"dir"sv, std::nullopt, std::ref(direction)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"bushes"sv, std::nullopt, std::ref(bushes)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"bombs"sv, std::nullopt, std::ref(bombs)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"bombpower"sv, std::nullopt, std::ref(bombpower)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"type"sv, std::nullopt, std::ref(type)});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELHORSE_H
