#ifndef TLEVELHORSE_H
#define TLEVELHORSE_H

#include "CString.h"
#include "CTimeout.h"

class TServer;
class TLevelHorse
{
	public:
		// constructor - destructor
		TLevelHorse(TServer* server, const CString& pImage, float pX, float pY, char pDir = 0, char pBushes = 0);

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
