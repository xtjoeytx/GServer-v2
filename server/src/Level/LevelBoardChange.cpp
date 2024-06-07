#include "IDebug.h"
#include "LevelBoardChange.h"

CString LevelBoardChange::getBoardStr() const
{
	return CString() >> (char)x >> (char)y >> (char)width >> (char)height << tiles;
}

void LevelBoardChange::swapTiles()
{
	CString temp = tiles;
	tiles        = oldTiles;
	oldTiles     = temp;
}
