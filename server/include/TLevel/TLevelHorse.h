#ifndef TLEVELHORSE_H
#define TLEVELHORSE_H

#include "CString.h"
#include "CTimeout.h"

class TServer;
class TLevelHorse
{
	public:
		TLevelHorse(int horselife, const CString& pImage, float pX, float pY, char pDir = 0, char pBushes = 0)
			: horselifetime(horselife), image(pImage), x(pX), y(pY), dir(pDir), bushes(pBushes)
		{
			timeout.setTimeout(horselifetime);
		}

		CString getHorseStr();

		// get private variables
		CString getImage() const	{ return image; }
		float getX() const			{ return x; }
		float getY() const			{ return y; }
		char getDir() const			{ return dir; }
		char getBushes() const		{ return bushes; }

		CTimeout timeout;

	private:
		CString image;
		CString horsePacket;
		float x, y;
		char dir, bushes;
		int horselifetime;
};

inline CString TLevelHorse::getHorseStr()
{
	if (horsePacket.isEmpty()) {
		char dir_bush = (bushes << 2) | (dir & 0x03);
		horsePacket = CString() << (char)(x * 2) >> (char)(y * 2) >> (char)dir_bush << image;
	}

	return horsePacket;
}

#endif // TLEVELHORSE_H
