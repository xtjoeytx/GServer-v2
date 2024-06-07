#include "IDebug.h"
#include "LevelBoardChange.h"

CString TLevelBoardChange::getBoardStr() const
{
	return CString() >> (char)x >> (char)y >> (char)width >> (char)height << tiles;
}

void TLevelBoardChange::swapTiles()
{
	CString temp = tiles;
	tiles        = oldTiles;
	oldTiles     = temp;
}
