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

		// set private variables
		inline void setNewLevel(const CString& _newLevel) {
			newLevel = _newLevel;
		}

		inline void setNewX(const CString& _newX) {
			newX = _newX;
		}

		inline void setNewY(const CString& _newY) {
			newY = _newY;
		}

		inline void setX(int posX = 0) {
			x = posX;
		}

		inline void setY(int posY = 0) {
			y = posY;
		}
		inline void setWidth(int _width = 0) {
			width = _width;
		}
		inline void setHeight(int _height = 0) {
			height = _height;
		}
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
