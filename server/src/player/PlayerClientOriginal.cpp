#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include <CSocket.h>

#include <CString.h>
#include <IEnums.h>

#include <Account.h>
#include <Server.h>
#include <level/Level.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <player/PlayerClientOriginal.h>
#include <player/PlayerProps.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/PropertySerializers.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

PlayerClientOriginal::PlayerClientOriginal(CSocket* pSocket, PlayerID pId)
	: PlayerClient(pSocket, pId)
{
}

PlayerClientOriginal::~PlayerClientOriginal()
{
}

///////////////////////////////////////////////////////////////////////////////

bool PlayerClientOriginal::warp(std::shared_ptr<Level> level, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime)
{
	// If we are warping to the same level, just update the player's location.
	auto localPosition = toLocalPixelPosition(position);
	if (!m_currentLevel.expired() && account.level == level->levelName)
	{
		sendPropsFromResults(
			setPropWith<PlayerProp::X2>(props::SetBy::SERVER, localPosition.x()),
			setPropWith<PlayerProp::Y2>(props::SetBy::SERVER, localPosition.y())
		);
		return true;
	}

	// Set the player's position.
	account.character.localPixelX = localPosition.x();
	account.character.localPixelY = localPosition.y();

	// Tell the client their new level.
	sendPacket(CString() >> (char)PLO_PLAYERWARP << getProp<PlayerProp::X>().serialize() << getProp<PlayerProp::Y>().serialize() << level->levelName);

	// Enter the level.
	return enterLevel(level, clientCachedTime);
}

bool PlayerClientOriginal::enterLevel(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime)
{
	auto currentLevel = getLevel();
	bool sameLevel = currentLevel == level;

	// Leave the current level if we are changing levels.
	if (!sameLevel)
	{
		leaveLevel();
		m_currentLevel = level;
	}

	// Add myself to the level playerlist.
	if (!sameLevel)
	{
		level->addPlayer(m_id);
		account.level = level->levelName;
	}

	// Send the level now.
	auto subLevel = level->getSubLevelAtPosition(getMapPosition());
	bool succeed = sendStaticLevelData(subLevel->staticData.lock(), subLevel, clientCachedTime);
	succeed = succeed && sendDynamicLevelData(level, clientCachedTime);

	// If we failed, leave the level and inform the client.
	if (!succeed)
	{
		leaveLevel();
		sendPacket(CString() >> (char)PLO_WARPFAILED << level->levelName);
		return false;
	}

	// If the level is a sparring zone and you have 100 AP, change AP to 99 and
	// the apcounter to 1.
	if (level->isSparringZone(getMapPosition()) && account.character.ap == 100)
	{
		account.apCounter = 1;
		sendPropsFromResults(setPropWith<PlayerProp::ALIGNMENT>(props::SetBy::SERVER, 99_ui8));
	}

	// Inform everybody as to the client's new location.  This will update the minimap.
	CString minimap = CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id
		>> (char)PlayerProp::LEVEL << getProp<PlayerProp::LEVEL>().serialize()
		>> (char)PlayerProp::X << getProp<PlayerProp::X>().serialize()
		>> (char)PlayerProp::Y << getProp<PlayerProp::Y>().serialize();

	for (const auto& [pid, player] : players_of_type<PlayerClient>(m_server->getPlayerList()))
	{
		if (pid == this->getId()) continue;
		player->sendPacket(minimap);
	}

	// Update RCs.
	CString myRCProps = CString() >> (char)PLO_ADDPLAYER >> (short)getId() >> (char)account.name.length() << account.name
		>> (char)PlayerProp::LEVEL << getProp<PlayerProp::LEVEL>().serialize()
		>> (char)PlayerProp::PLAYERLISTSTATUS << getProp<PlayerProp::PLAYERLISTSTATUS>().serialize()
		>> (char)PlayerProp::NICKNAME << getProp<PlayerProp::NICKNAME>().serialize()
		>> (char)PlayerProp::COMMUNITYNAME << getProp<PlayerProp::COMMUNITYNAME>().serialize();
	m_server->sendPacketToType(PLTYPE_ANYCONTROL, myRCProps, this);

	return true;
}

bool PlayerClientOriginal::sendStaticLevelData(std::shared_ptr<StaticLevelData> staticLevelData, std::shared_ptr<SubLevel> subLevel, std::optional<clock::time_point> clientCachedTime)
{
	if (staticLevelData == nullptr)
		return false;

	PlayerPtr self = shared_from_this();
	auto levelModTime = staticLevelData->modTime;
	auto cachedModTime = getLevelLastEnteredTime(staticLevelData.get());

	// If the player has seen this level before, don't sending anything.
	if (cachedModTime.has_value())
	{
		// Tell the client that there is no board data to load.
		// YOU GOTTA SEND THIS NO MATTER WHAT!  If you don't, weird things happen on the client.
		sendPacket(CString() >> (char)PLO_LEVELBOARD);
	}
	// Otherwise, send the level board data.
	else
	{
		if (!clientCachedTime.has_value() || clientCachedTime.value() != levelModTime)
		{
			// Send board tiles.
			if (subLevel != nullptr)
				subLevel->sendBoardToPlayer(self);
			else
				staticLevelData->sendBoardToPlayer(self);

			// Tell the client the current level name.
			// Only send it the very first time for original clients.
			if (!m_firstLevel)
				sendPacket(CString() >> (char)PLO_LEVELNAME << staticLevelData->levelName);
			m_firstLevel = true;

			// Send links.
			staticLevelData->sendLinksToPlayer(self, false);

			// Send signs.
			staticLevelData->sendSignsToPlayer(self);

			// Send the level mod time so the client can cache it.
			sendPacket(CString() >> (char)PLO_LEVELMODTIME >> (long long)clock::to_time_t(levelModTime));
		}
		else
		{
			// Tell the client that there is no board data to load.
			sendPacket(CString() >> (char)PLO_LEVELBOARD);
		}

		// Send chests.
		staticLevelData->sendChestsToPlayer(self);
	}

	return true;
}

bool PlayerClientOriginal::sendDynamicLevelData(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime)
{
	if (level == nullptr) return false;

	// Get the sub-level and static data we are on.
	auto [subLevel, staticLevelData] = level->getSubLevelAndStaticDataAtPosition(getMapPosition());
	if (subLevel == nullptr || staticLevelData == nullptr)
		return false;

	PlayerPtr self = shared_from_this();
	auto cachedModTime = getLevelLastEnteredTime(staticLevelData.get());

	// Send board changes, horses, and baddies.
	subLevel->sendBoardChangesToPlayer(self, cachedModTime);

	// TODO: Maybe bind horses to sub-level and send in sendStaticLevelData.
	level->sendHorsesToPlayer(self);
	level->sendBaddiesToPlayer(self);

	// If we are the leader, send it now.
	if (level->isSinglePlayer || level->isPlayerLeader(getId()))
		sendPacket(CString() >> (char)PLO_ISLEADER);

	// Send NPCs.
	level->sendNPCsToPlayer(self, cachedModTime);

	// Move the carry NPC to the new level.
	if (m_carryNPC != 0)
	{
		level->addNPC(m_carryNPC);
		if (auto npc = m_server->getNPC(m_carryNPC); npc)
		{
			npc->setLevel(level);
			npc->sendPropsFromResults(
				npc->setPropWith<NPCProp::GMAPLEVELX>(props::SetBy::SERVER, account.character.mapX),
				npc->setPropWith<NPCProp::GMAPLEVELY>(props::SetBy::SERVER, account.character.mapY)
			);

			// Send the carry NPC props to other players.
			// if (!level->isSingleplayer)
			{
				CString carryNPCProps = CString() >> (char)PLO_NPCPROPS >> (int)m_carryNPC << npc->getAllPropsPacket();
				m_server->sendPacketToNearby(carryNPCProps, getGlobalPosition(), level, { m_id });
			}
		}
	}

	// Send connecting player props to players in nearby levels.
	// if (!level->isSingleplayer)
	{
		CString myProps = CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PlayerProp::JOINLEAVELVL >> (char)1 << getPropsPacketFromList(loginPropsClientOthers);
		for (const auto& playerId : level->findInRangePlayersForCommunication(getGlobalPosition()))
		{
			if (playerId == m_id) continue;
			if (auto other = m_server->getPlayer(playerId); other != nullptr)
			{
				if (!other->isClient()) continue;

				// Exchange props.
				other->sendPacket(myProps);
				this->sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)other->getId() >> (char)PlayerProp::JOINLEAVELVL >> (char)1 << other->getPropsPacketFromList(loginPropsClientOthers));
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
