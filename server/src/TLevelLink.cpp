#include "IDebug.h"
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
	newLevel = pLink[0];
	if (pLink.size() > 7)
	{
		offset = pLink.size() - 7;
		for (unsigned int i = 0; i < offset; ++i)
			newLevel << " " << pLink[1 + i];
	}

	x = strtoint(pLink[1 + offset]);
	y = strtoint(pLink[2 + offset]);
	width = strtoint(pLink[3 + offset]);
	height = strtoint(pLink[4 + offset]);
	newX = pLink[5 + offset];
	newY = pLink[6 + offset];
}
