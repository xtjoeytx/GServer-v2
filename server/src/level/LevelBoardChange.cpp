#include <chrono>
#include <memory>

#include <CString.h>
#include <IEnums.h>

#include <BabyDI.h>
#include <Server.h>
#include <level/Level.h>
#include <level/LevelBoardChange.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

LevelBoardChange::LevelBoardChange(std::shared_ptr<Level> level, const LocalWholeTileRectangleArea& area, const CString& tiles, const CString& oldTiles, std::chrono::seconds respawnTime)
	: area(area), m_level(level), m_newTiles(tiles), m_oldTiles(oldTiles)
{
	auto server = BabyDI::Get<Server>();
	modTime = server->getFrameStartTime();

	if (respawnTime != 0s)
		m_timeout.runOnceFor(respawnTime);
}

LevelBoardChange::LevelBoardChange(std::shared_ptr<Level> level, const MapPosition& mapPosition, const LocalWholeTileRectangleArea& area, const CString& tiles, const CString& oldTiles, std::chrono::seconds respawnTime)
	: LevelBoardChange(level, area, tiles, oldTiles, respawnTime)
{
	m_mapPosition = mapPosition;
}

void LevelBoardChange::update(const precise_clock::time_point& time)
{
	if (m_timeout.isRunning())
	{
		m_timeout.update(time);
		if (!m_timeout.isRunning())
		{
			swapTiles();
			sendToPlayersOnLevel();
		}
	}
}

void LevelBoardChange::sendToPlayersOnLevel() const
{
	auto server = BabyDI::Get<Server>();
	if (auto level = m_level.lock(); level != nullptr && server != nullptr)
	{
		if (!level->isGmap())
			server->sendPacketToOneLevelPart(CString() >> (char)PLO_BOARDMODIFY << getPropsForSingleLevel(), { 0, 0 }, level);
		else
		{
			server->sendPacketToNearby(CString() >> (char)PLO_BOARDMODIFY2 << getPropsForMapClassic(), toPixelPosition(m_mapPosition.value(), area.position), level, {});

			/*
			// Classic mode clients don't support board updates in adjacent levels, but still need the map position.
			server->sendPacketToOneLevel(CString() >> (char)PLO_BOARDMODIFY2 << getPropsForMapClassic(), level, {}, [](const Player* player) { return player->getVersion() < CLVER_4_0211; });

			// Newmain and up can see nearby level board changes.
			server->sendPacketToNearby(CString() >> (char)PLO_BOARDMODIFY2 << getPropsForMapNewMain(), toPixelPosition(this->area.position), level, {}, [](const Player* player) { return player->getVersion() >= CLVER_4_0211; });
			*/
		}
	}
}

CString LevelBoardChange::getPropsForSingleLevel() const
{
	// {7}{CHAR tileX}{CHAR tileY}{CHAR width}{CHAR height}{tiles}
	// {7}{CHAR layer +64}{CHAR tileX}{CHAR tileY}{CHAR width}{CHAR height}{tiles}
	if (layer == 0) [[likely]]
		return CString() >> (char)area.position.x() >> (char)area.position.y() >> (char)area.size.width() >> (char)area.size.height() << m_newTiles;

	return CString() >> (char)(layer + 64) >> (char)area.position.x() >> (char)area.position.y() >> (char)area.size.width() >> (char)area.size.height() << m_newTiles;
}

CString LevelBoardChange::getPropsForMapClassic() const
{
	// {186}{CHAR mapX}{CHAR mapY}{CHAR tileX}{CHAR tileY}{CHAR width}{CHAR height}{tiles}
	// {186}{CHAR mapX}{CHAR mapY}{CHAR layer +64}{CHAR tileX}{CHAR tileY}{CHAR width}{CHAR height}{tiles}

	if (m_level.expired() || !m_mapPosition.has_value())
		return CString();

	const auto& [mapX, mapY, _] = m_mapPosition.value();
	if (layer == 0) [[likely]]
		return CString() >> (char)mapX >> (char)mapY >> (char)area.position.x() >> (char)area.position.y() >> (char)area.size.width() >> (char)area.size.height() << m_newTiles;

	return CString() >> (char)mapX >> (char)mapY >> (char)(layer + 64) >> (char)area.position.x() >> (char)area.position.y() >> (char)area.size.width() >> (char)area.size.height() << m_newTiles;
}

/*
CString LevelBoardChange::getPropsForMapNewMain() const
{
	props::PropertyPixelCoordinate positionX{ static_cast<int16_t>(area.position.x()) };
	props::PropertyPixelCoordinate positionY{ static_cast<int16_t>(area.position.y()) };
	return CString() << positionX.serialize() << positionY.serialize() >> (char)area.size.width() >> (char)area.size.height() << m_newTiles;
}
*/

void LevelBoardChange::swapTiles()
{
	CString temp = m_newTiles;
	m_newTiles = m_oldTiles;
	m_oldTiles = temp;

	auto server = BabyDI::Get<Server>();
	modTime = server->getFrameStartTime();
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
