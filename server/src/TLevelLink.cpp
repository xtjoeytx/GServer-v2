#include "TLevelLink.h"

/*
	TLevelLink: Constructor - Deconstructor
*/
TLevelLink::TLevelLink(const std::vector<CString>& pLink)
{
	parseLinkStr(pLink);
}

/*
	TLevelLink: Functions
*/

CString TLevelLink::getLinkStr()
{
	static char retVal[500];
	sprintf(retVal, "%s %i %i %i %i %s %s", newLevel.text(), x, y, width, height, newX.text(), newY.text());
	return retVal;
}

void TLevelLink::parseLinkStr(const std::vector<CString>& pLink)
{
	newLevel = pLink[1];
	x = strtoint(pLink[2]);
	y = strtoint(pLink[3]);
	width = strtoint(pLink[4]);
	height = strtoint(pLink[5]);
	newX = pLink[6];
	newY = pLink[7];
}
