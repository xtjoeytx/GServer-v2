#ifndef TLEVELCHEST_H
#define TLEVELCHEST_H

#include <vector>
#include "ICommon.h"
#include "CString.h"

class TLevelChest
{
	public:
		// constructor - destructor
		TLevelChest(char nx, char ny, char nitemIndex, char nsignIndex) : x(nx), y(ny), itemIndex(nitemIndex), signIndex(nsignIndex) {}
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
};

/*
	TLevelChest: Get Private Variables
*/
inline int TLevelChest::getItemIndex()
{
	return itemIndex;
}

inline int TLevelChest::getSignIndex()
{
	return signIndex;
}

inline int TLevelChest::getX()
{
	return x;
}

inline int TLevelChest::getY()
{
	return y;
}

#endif // TLEVELCHEST_H
