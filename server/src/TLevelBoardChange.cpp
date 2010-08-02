#include "IDebug.h"
#include "TLevelBoardChange.h"

CString TLevelBoardChange::getBoardStr(const CString ignore) const
{
	return CString() >> (char)x >> (char)y >> (char)width >> (char)height << tiles;
}

void TLevelBoardChange::swapTiles()
{
	CString temp = tiles;
	tiles = oldTiles;
	oldTiles = temp;
}
