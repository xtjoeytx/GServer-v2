#include "IDebug.h"
#include "TLevelChest.h"

extern int findItemId(const CString& pItemName);

/*
	TLevelChest: Constructor - Deconstructor
*/
TLevelChest::TLevelChest(const std::vector<CString>& pChest)
{
	parseChestStr(pChest);
}

/*
	TLevelChest: Functions
*/

CString TLevelChest::getChestStr(const CString pLevelName)  const
{
	static char retVal[500];
	sprintf(retVal, "%i:%i:%s", x, y, pLevelName.text());
	return retVal;
}

void TLevelChest::parseChestStr(const std::vector<CString>& pChest)
{
	x = strtoint(pChest[1]);
	y = strtoint(pChest[2]);
	itemIndex = strtoint(pChest[3]);
	signIndex = strtoint(pChest[4]);
}
