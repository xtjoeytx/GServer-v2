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
	[[a::inline]] float getTileX() const;
	[[a::inline]] float getTileY() const;

	CString getPacket() const
	{
		auto localPosition = toLocalPixelPosition(position);
		const char dir_bush = static_cast<char>((bushes << 2) | (direction & 0x03));
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

	[[a::inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline float LevelHorse::getTileX() const
{
	return static_cast<float>(position.x()) / 16.0f;
}

inline float LevelHorse::getTileY() const
{
	return static_cast<float>(position.y()) / 16.0f;
}

inline void LevelHorse::constructScriptParameters()
{
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{.name = "x"sv, .modTime = std::nullopt, .value = std::ref(position.x()), .factor = 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{.name = "y"sv, .modTime = std::nullopt, .value = std::ref(position.y()), .factor = 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "dir"sv, .modTime = std::nullopt, .value = std::ref(direction)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "bushes"sv, .modTime = std::nullopt, .value = std::ref(bushes)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "bombs"sv, .modTime = std::nullopt, .value = std::ref(bombs)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "bombpower"sv, .modTime = std::nullopt, .value = std::ref(bombpower)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "type"sv, .modTime = std::nullopt, .value = std::ref(type)});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELHORSE_H
