#include <CString.h>
#include <level/LevelBoardChange.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

CString LevelBoardChange::getBoardStr() const
{
	return CString() >> (char)m_x >> (char)m_y >> (char)m_width >> (char)m_height << m_newTiles;
}

void LevelBoardChange::swapTiles()
{
	CString temp = m_newTiles;
	m_newTiles = m_oldTiles;
	m_oldTiles = temp;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
