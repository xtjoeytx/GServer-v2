#ifndef LEVELSHOOT_H
#define LEVELSHOOT_H

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <scripting/ScriptContainers.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/*
shoot x,y,z,angle,zangle,power,gani,ganiparams;

angle: east = 0, north = pi/2, west = pi, south = 3*pi/2
zangle: horizontal = 0, up = pi/2
power: 0 = old projectile (no gravity, 20 tiles per second), otherwise

z <= 3:
	Collides with walls and NPCs

When landing/hitting:
	spawned from client: actionprojectile:  #p(0) = x, #p(1) = y, #p(3+) = shootparams
	spawned from server: actionsprojectile: #p(0) = x, #p(1) = y, #p(3+) = shootparams
	triggers on players/npcs at the coordinate, and on the control-npc
	clientside weapons: actionprojectile2
	serverside weapons: ???

On creation:
	horzspeed = cos(zangle) * (power * 44)
	vertspeed = sin(zangle) * (power * 44)

Every second (but done spread out every 0.05ms):
	vertspeed = vertspeed - gravity
	newx = x + cos(angle) * horzspeed
	newy = y - sin(angle) * horzspeed
	newz = z + vertspeed
*/

struct LevelShoot
{
	TilePosition position;
	float angle = 0.0f;
	float zangle = 0.0f;
	uint8_t powerIn44Pixels = 0;
	std::string gani;

	float gravity = 2.0;
	TilePosition movementPerFrame;
	std::vector<std::string> shootParams;
	ScriptObject from;

	[[inline]] void calculateSpeeds();
	[[inline]] void move();
};

//----------------------------

inline void LevelShoot::calculateSpeeds()
{
	float horizSpeed = std::cos(zangle) * (powerIn44Pixels / 44.0f);
	float vertSpeed = std::sin(zangle) * (powerIn44Pixels / 44.0f);
	movementPerFrame.x() = std::cos(angle) * horizSpeed;
	movementPerFrame.y() = std::sin(angle) * horizSpeed;
	movementPerFrame.z() = vertSpeed;
}

inline void LevelShoot::move()
{
	movementPerFrame.z() -= gravity * 0.05f;
	position.x() += movementPerFrame.x();
	position.y() -= movementPerFrame.y();
	position.z() += movementPerFrame.z();
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELSHOOT_H
