#include "ICommon.h"
#include "TLevelBoardChange.h"
#include "TPlayer.h"

CString TLevelBoardChange::getBoardStr(const CString ignore) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return CString() >> (char)x >> (char)y >> (char)width >> (char)height << tiles;
}

void TLevelBoardChange::swapTiles()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	CString temp = tiles;
	tiles = oldTiles;
	oldTiles = temp;
}
