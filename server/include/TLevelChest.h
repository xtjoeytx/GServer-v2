#ifndef TLEVELCHEST_H
#define TLEVELCHEST_H

#include <vector>
#include "ICommon.h"
#include "CString.h"

class TLevelChest
{
	public:
		// constructor - destructor
		TLevelChest() { }
		TLevelChest(const std::vector<CString>& pChest);

		// functions
		CString getChestStr(const CString pLevelName) const;
		void parseChestStr(const std::vector<CString>& pChest);

		// get private variables
		inline int getItemIndex();
		inline int getSignIndex();
		inline int getX();
		inline int getY();

	private:
		int itemIndex, signIndex, x, y;
		mutable boost::recursive_mutex m_preventChange;
};

/*
	TLevelChest: Get Private Variables
*/
inline int TLevelChest::getItemIndex()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return itemIndex;
}

inline int TLevelChest::getSignIndex()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return signIndex;
}

inline int TLevelChest::getX()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return x;
}

inline int TLevelChest::getY()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return y;
}

#endif // TLEVELCHEST_H
