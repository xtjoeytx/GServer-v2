#ifndef TLEVELLINK_H
#define TLEVELLINK_H

#include <vector>
#include "CString.h"

class TLevelLink
{
	public:
		// constructor - destructor
		TLevelLink() : x(0), y(0), width(0), height(0) { }
		TLevelLink(const std::vector<CString>& pLink);

		// functions
		CString getLinkStr() const;
		void parseLinkStr(const std::vector<CString>& pLink);

		// get private variables
		inline CString getNewLevel() const;
		inline CString getNewX() const;
		inline CString getNewY() const;
		inline int getX() const;
		inline int getY() const;
		inline int getWidth() const;
		inline int getHeight() const;

	private:
		CString newLevel, newX, newY;
		int x, y, width, height;
};

/*
	TLevelLink: Get Private Variables
*/
inline CString TLevelLink::getNewLevel() const
{
	return newLevel;
}

inline CString TLevelLink::getNewX() const
{
	return newX;
}

inline CString TLevelLink::getNewY() const
{
	return newY;
}

inline int TLevelLink::getX() const
{
	return x;
}

inline int TLevelLink::getY() const
{
	return y;
}

inline int TLevelLink::getWidth() const
{
	return width;
}

inline int TLevelLink::getHeight() const
{
	return height;
}

#endif // TLEVELLINK_H
