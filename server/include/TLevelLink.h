#ifndef TLEVELLINK_H
#define TLEVELLINK_H

#include <vector>
#include "CString.h"

class TLevelLink
{
	public:
		// constructor - destructor
		TLevelLink() { }
		TLevelLink(const std::vector<CString>& pLink);

		// functions
		CString getLinkStr();
		void parseLinkStr(const std::vector<CString>& pLink);

		// get private variables
		inline CString getNewLevel();
		inline CString getNewX();
		inline CString getNewY();
		inline int getX();
		inline int getY();
		inline int getWidth();
		inline int getHeight();

	private:
		CString newLevel, newX, newY;
		int x, y, width, height;
};

/*
	TLevelLink: Get Private Variables
*/
inline CString TLevelLink::getNewLevel()
{
	return newLevel;
}

inline CString TLevelLink::getNewX()
{
	return newX;
}

inline CString TLevelLink::getNewY()
{
	return newY;
}

inline int TLevelLink::getX()
{
	return x;
}

inline int TLevelLink::getY()
{
	return y;
}

inline int TLevelLink::getWidth()
{
	return width;
}

inline int TLevelLink::getHeight()
{
	return height;
}

#endif // TLEVELLINK_H
