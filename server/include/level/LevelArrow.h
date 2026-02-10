#ifndef LEVELARROW_H
#define LEVELARROW_H

#include <cstdint>

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
inline constexpr float arrowSpeedInTilesPer50ms = 2.0f;
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

	[[inline]] uint8_t getPacketFrom() const;

	[[inline]] void constructScriptParameters();
	string_map<GameValue> scriptParameters;
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
	scriptParameters.try_emplace("x", set_temporary, "x", gameValueGetter([this]() { return position.x() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameValueGetter([this]() { return position.y() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("dx", set_temporary, "dx", gameValueGetter([this]() { return speed.x() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("dy", set_temporary, "dy", gameValueGetter([this]() { return speed.y() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("dir", set_temporary, "dir", gameValueGetter(direction), GameValue::func_set{});
	scriptParameters.try_emplace("type", set_temporary, "type", gameValueGetter(type), GameValue::func_set{});
	scriptParameters.try_emplace("from", set_temporary, "from",
		gameValueGetter([this]()
		{
			if (from.second == ScriptObjectType::PLAYER)
				return 1.0;
			return 0.0;
		}), GameValue::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELARROW_H
