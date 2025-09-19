#include <chrono>
#include <cstdint>
#include <memory>

#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <BabyDI.h>
#include <Server.h>
#include <level/Level.h>
#include <level/LevelBoardChange.h>
#include <object/Player.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/PropertySerializers.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

LevelBoardChange::LevelBoardChange(std::shared_ptr<Level> level, const LocalWholeTileRectangleArea& area, const CString& pTiles, const CString& pOldTiles, std::chrono::seconds respawnTime)
	: m_level(level), m_newTiles(pTiles), m_oldTiles(pOldTiles)
{
	this->area.size.data = area.size.data;
	this->area.position = toTilePosition(level->convertToMapPosition(area.position));

	if (respawnTime != 0s)
		m_timeout.runOnceFor(respawnTime);
}

void LevelBoardChange::update(const precise_clock::time_point& time)
{
	if (m_timeout.isRunning())
	{
		m_timeout.update(time);
		if (!m_timeout.isRunning())
		{
			auto server = BabyDI::Get<Server>();
			swapTiles();
			modTime = server->getFrameStartTime();
			if (auto level = m_level.lock(); level != nullptr)
			{
				server->sendPacketToNearby(CString() >> (char)PLO_BOARDMODIFY2 << getPropsForMap(), toPixelPosition(this->area.position), level, {}, [](const Player* player) { return player->getVersion() >= CLVER_4_0211; });
				server->sendPacketToOneLevel(CString() >> (char)PLO_BOARDMODIFY << getPropsForSingleLevel(), level, {}, [](const Player* player) { return player->getVersion() < CLVER_4_0211; });
			}
		}
	}
}

CString LevelBoardChange::getPropsForSingleLevel() const
{
	return CString() >> (char)area.position.x() >> (char)area.position.y() >> (char)area.size.width() >> (char)area.size.height() << m_newTiles;
}

CString LevelBoardChange::getPropsForMap() const
{
	props::PropertyPixelCoordinate positionX{ static_cast<int16_t>(area.position.x()) };
	props::PropertyPixelCoordinate positionY{ static_cast<int16_t>(area.position.y()) };
	return CString() << positionX.serialize() << positionY.serialize() >> (char)area.size.width() >> (char)area.size.height() << m_newTiles;
}

void LevelBoardChange::swapTiles()
{
	CString temp = m_newTiles;
	m_newTiles = m_oldTiles;
	m_oldTiles = temp;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
