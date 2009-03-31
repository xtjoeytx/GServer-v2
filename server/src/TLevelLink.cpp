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
	unsigned int offset = 0;

	// Find the whole level name.
	newLevel = pLink[1];
	if (pLink.size() > 8)
	{
		offset = pLink.size() - 8;
		for (unsigned int i = 0; i < offset; ++i)
			newLevel << " " << pLink[2 + i];
	}

	x = strtoint(pLink[2 + offset]);
	y = strtoint(pLink[3 + offset]);
	width = strtoint(pLink[4 + offset]);
	height = strtoint(pLink[5 + offset]);
	newX = pLink[6 + offset];
	newY = pLink[7 + offset];
}
