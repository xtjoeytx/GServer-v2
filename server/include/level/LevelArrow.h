#ifndef LEVELARROW_H
#define LEVELARROW_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/*
(20, 30), shooting default arrow up:
	dir=0 dx=0 dy=-1 dz=0 from=1 type=0 x=21.1875 y=29.0625 z=0  (+19 pixels, -16 pixels)

(20, 30), shooting default arrow down:
	dir=2 dx=0 dy=1 dz=0 from=1 type=0 x=21.1875 y=33.0625 z=0 (+19 pixels, +48 pixels)

(20, 30), shooting default arrow left:
	dir=1 dx=-1 dy=0 dz=0 from=1 type=0 x=18.5625 y=31.6875 z=0 (-24 pixels, +27 pixels)

(20, 30), shooting default arrow right:
	dir=3 dx=1 dy=0 dz=0 from=1 type=0 x=22.5625 y=31.6875 z=0 (+40 pixels, +27 pixels)

type 0 = arrow     arrow.x=21.5625
type 1 = fireball  arrow.x=21.5625
type 2 = fireblast arrow.x=21.5625
type 3 = nukeshot  arrow.x=21.5625
type -1 = ball (shot from center of the npc calculating dx/dy to travel towards the player)
*/

inline constexpr uint8_t arrowSpriteIndex = 107;
inline constexpr uint8_t ballSpriteIndex = 131;
inline constexpr uint8_t arrowTypeBall = 0;
inline constexpr uint8_t arrowTypeNormal = 1;
inline constexpr uint8_t arrowTypeFireball = 2;
inline constexpr uint8_t arrowTypeFireblast = 3;
inline constexpr uint8_t arrowTypeNukeshot = 4;
inline constexpr float arrowSpeedInTilesPerSecond = 20.0f;
inline constexpr float arrowSpeedInTilesPer50ms = 1.0f;
inline constexpr int16_t arrowSpeedInPixelsPer50ms = 16;

struct LevelArrow
{
	float getTileX() const { return position.x() / 16.0f; }
	float getTileY() const { return position.y() / 16.0f; }

	PixelPosition startPosition;
	PixelPosition position;
	PixelPosition speed;
	uint8_t direction;
	int8_t type;
	ScriptObject from;

	[[a::inline]] uint8_t getPacketFrom() const;

	[[a::inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline uint8_t LevelArrow::getPacketFrom() const
{
	if (from.second == ScriptObjectType::PLAYER)
		return (uint8_t)1;
	return (uint8_t)0;
}

inline void LevelArrow::constructScriptParameters()
{
	// clang-format off
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"x"sv, std::nullopt, std::ref(position.x()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"y"sv, std::nullopt, std::ref(position.y()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"dx"sv, std::nullopt, std::ref(speed.x()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"dy"sv, std::nullopt, std::ref(speed.y()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"dir"sv, std::nullopt, std::ref(direction)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"type"sv, std::nullopt, std::ref(type)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"type"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return from.second == ScriptObjectType::PLAYER ? 1.0 : 0.0; }
	});
	// clang-format on
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELARROW_H
