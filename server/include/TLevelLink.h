#ifndef TLEVELLINK_H
#define TLEVELLINK_H

#include <vector>
#include "ICommon.h"
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
		mutable boost::recursive_mutex m_preventChange;
};

/*
	TLevelLink: Get Private Variables
*/
inline CString TLevelLink::getNewLevel()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return newLevel;
}

inline CString TLevelLink::getNewX()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return newX;
}

inline CString TLevelLink::getNewY()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return newY;
}

inline int TLevelLink::getX()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return x;
}

inline int TLevelLink::getY()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return y;
}

inline int TLevelLink::getWidth()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return width;
}

inline int TLevelLink::getHeight()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return height;
}

#endif // TLEVELLINK_H
