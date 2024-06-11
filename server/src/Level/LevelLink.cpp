#include "LevelLink.h"
#include "IDebug.h"

/*
	LevelLink: Constructor - Deconstructor
*/
LevelLink::LevelLink(const std::vector<CString>& pLink)
{
	parseLinkStr(pLink);
}

/*
	LevelLink: Functions
*/

CString LevelLink::getLinkStr() const
{
	static char retVal[500];
	sprintf(retVal, "%s %i %i %i %i %s %s", m_newLevel.text(), m_x, m_y, m_width, m_height, m_newX.text(),
			m_newY.text());
	return retVal;
}

void LevelLink::parseLinkStr(const std::vector<CString>& pLink)
{
	size_t offset = 0;

	// Find the whole level name.
	m_newLevel = pLink[0];
	if (pLink.size() > 7)
	{
		offset = pLink.size() - 7;
		for (size_t i = 0; i < offset; ++i)
			m_newLevel << " " << pLink[1 + i];
	}

	m_x = strtoint(pLink[1 + offset]);
	m_y = strtoint(pLink[2 + offset]);
	m_width = strtoint(pLink[3 + offset]);
	m_height = strtoint(pLink[4 + offset]);
	m_newX = pLink[5 + offset];
	m_newY = pLink[6 + offset];
}
