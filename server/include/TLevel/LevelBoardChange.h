#ifndef TLEVELBOARDCHANGE_H
#define TLEVELBOARDCHANGE_H

#include <vector>
#include <time.h>
#include "CTimeout.h"
#include "CString.h"

class TLevelBoardChange
{
	public:
		// constructor - destructor
		TLevelBoardChange(const int pX, const int pY, const int pWidth, const int pHeight,
			const CString& pTiles, const CString& pOldTiles, const int respawn = 15)
			: x(pX), y(pY), width(pWidth), height(pHeight),
			tiles(pTiles), oldTiles(pOldTiles), modTime(time(0)) { timeout.setTimeout(respawn); }

		// functions
		CString getBoardStr() const;
		void swapTiles();

		// get private variables
		int getX() const				{ return x; }
		int getY() const				{ return y; }
		int getWidth() const			{ return width; }
		int getHeight() const			{ return height; }
		CString getTiles() const		{ return tiles; }
		time_t getModTime() const		{ return modTime; }

		// set private variables
		void setModTime(time_t ntime)	{ modTime = ntime; }

		CTimeout timeout;

	private:
		int x, y, width, height;
		CString tiles, oldTiles;
		time_t modTime;
};

#endif // TLEVELBOARDCHANGE_H
