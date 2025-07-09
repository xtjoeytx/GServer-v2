#ifndef LEVELCHEST_H
#define LEVELCHEST_H

#include <cstdint>

#include <level/LevelItem.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct LevelChest
{
	float getTileX() const { return (float)position.x(); }
	float getTileY() const { return (float)position.y(); }

	WholeTilePosition position;
	LevelItemType item;
	uint8_t sign;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELCHEST_H
