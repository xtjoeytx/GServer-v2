#ifndef LEVELBOARDCHANGE_H
#define LEVELBOARDCHANGE_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#include <CString.h>

#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Level;
class Server;

class LevelBoardChange
{
public:
	LevelBoardChange(const std::shared_ptr<Level>& level, const LocalWholeTileRectangleArea& area, CString tiles, CString oldTiles, std::chrono::seconds respawnTime = 15s);
	LevelBoardChange(const std::shared_ptr<Level>& level, const MapPosition& mapPosition, const LocalWholeTileRectangleArea& area, const CString& tiles, const CString& oldTiles, std::chrono::seconds respawnTime = 15s);

public:
	void update(const precise_clock::time_point& time);
	void sendToPlayersOnLevel() const;

public:
	[[nodiscard]] [[a::inline]] CString getTiles() const;
	[[nodiscard]] CString getPropsForSingleLevel() const;
	[[nodiscard]] CString getPropsForMapClassic() const;
	//[[nodiscard]] CString getPropsForMapNewMain() const;
	void swapTiles();
	[[nodiscard]] [[a::inline]] bool willRespawn() const;

public:
	LocalWholeTileRectangleArea area;
	uint8_t layer = 0;
	clock::time_point modTime;

private:
	Server* m_server;
	TimeoutGenerator m_timeout;
	std::weak_ptr<Level> m_level;
	std::optional<MapPosition> m_mapPosition;
	CString m_newTiles, m_oldTiles;
};

//----------------------------

inline CString LevelBoardChange::getTiles() const
{
	return m_newTiles;
}

inline bool LevelBoardChange::willRespawn() const
{
	return m_timeout.isRunning();
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELBOARDCHANGE_H
