#ifndef LEVELBOARDCHANGE_H
#define LEVELBOARDCHANGE_H

#include <chrono>
#include <memory>

#include <CString.h>

#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Level;
class LevelBoardChange
{
public:
	LevelBoardChange(std::shared_ptr<Level> level, const LocalWholeTileRectangleArea& area, const CString& pTiles, const CString& pOldTiles, std::chrono::seconds respawn = 15s);

public:
	void update(const precise_clock::time_point& time);

public:
	CString getTiles() const { return m_newTiles; }
	CString getPropsForSingleLevel() const;
	CString getPropsForMap() const;
	void swapTiles();

public:
	WholeTileRectangleArea area;
	clock::time_point modTime;

private:
	TimeoutGenerator m_timeout;
	std::weak_ptr<Level> m_level;
	CString m_newTiles, m_oldTiles;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBOARDCHANGE_H
