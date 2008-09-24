#ifndef TLEVELHORSE_H
#define TLEVELHORSE_H

#include "ICommon.h"
#include "CString.h"
#include "CTimeout.h"

class TLevelHorse
{
	public:
		// constructor - destructor
		TLevelHorse(const CString& pImage, float pX, float pY, char pDir = 0, char pBushes = 0) : image(pImage), x(pX), y(pY), dir(pDir), bushes(pBushes) { timeout.setTimeout(180); }

		CString getHorseStr() const;

		// get private variables
		CString getImage() const	{ return image; }
		float getX() const			{ return x; }
		float getY() const			{ return y; }
		char getDir() const			{ return dir; }
		char getBushes() const		{ return bushes; }

		CTimeout timeout;

	private:
		CString image;
		float x, y;
		char dir, bushes;
};


#endif // TLEVELHORSE_H
