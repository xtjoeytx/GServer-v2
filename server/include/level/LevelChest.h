#ifndef LEVELCHEST_H
#define LEVELCHEST_H

#include <cstdint>

#include <level/LevelItem.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct LevelChest
{
	float getTileX() const { return (float)position.x(); }
	float getTileY() const { return (float)position.y(); }

	LocalWholeTilePosition position;
	LevelItemType item;
	uint8_t sign;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELCHEST_H
