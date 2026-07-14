#ifndef LEVELTHROWNITEM_H
#define LEVELTHROWNITEM_H

#include <cstdint>

#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct LevelThrownItem
{
	TilePosition position{};
	TilePosition velocity{};
	CarryObjectSprite item = CarryObjectSprite::BOMB;
	ScriptObject source{};
	uint8_t flyDuration = 0;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELTHROWNITEM_H
