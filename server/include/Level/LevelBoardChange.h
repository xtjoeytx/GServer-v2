#ifndef TLEVELBOARDCHANGE_H
#define TLEVELBOARDCHANGE_H

#include "CString.h"
#include "CTimeout.h"
#include <time.h>
#include <vector>

class LevelBoardChange
{
public:
	// constructor - destructor
	LevelBoardChange(const int pX, const int pY, const int pWidth, const int pHeight,
					 const CString& pTiles, const CString& pOldTiles, const int respawn = 15)
		: m_x(pX), m_y(pY), m_width(pWidth), m_height(pHeight),
		  m_newTiles(pTiles), m_oldTiles(pOldTiles), m_modTime(time(0)) { timeout.setTimeout(respawn); }

	// functions
	CString getBoardStr() const;

	void swapTiles();

	// get private variables
	int getX() const { return m_x; }

	int getY() const { return m_y; }

	int getWidth() const { return m_width; }

	int getHeight() const { return m_height; }

	CString getTiles() const { return m_newTiles; }

	time_t getModTime() const { return m_modTime; }

	// set private variables
	void setModTime(time_t ntime) { m_modTime = ntime; }

	CTimeout timeout;

private:
	int m_x, m_y, m_width, m_height;
	CString m_newTiles, m_oldTiles;
	time_t m_modTime;
};

#endif // TLEVELBOARDCHANGE_H
