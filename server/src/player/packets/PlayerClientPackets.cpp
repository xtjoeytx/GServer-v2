#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <CSettings.h>
#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <Server.h>
#include <UpdatePackage.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
#include <level/LevelBaddy.h>
#include <level/LevelChest.h>
#include <level/LevelItem.h>
#include <network/IPacketHandler.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerClient::msgPLI_LEVELWARP(CString& pPacket)
{
	std::optional<clock::time_point> modTime;

	if (pPacket[0] - 32 == PLI_LEVELWARPMOD)
		modTime = clock::from_time_t((time_t)pPacket.readGUInt5());

	LocalPixelPosition pos = { static_cast<int16_t>(pPacket.readGChar() * 8), static_cast<int16_t>(pPacket.readGChar() * 8) };
	CString newLevelC = pPacket.readString("");
	std::string_view newLevel = newLevelC.toStringView();

	bool success = false;

	// If this is a gmap, in order to prevent the client from glitching out, forcefully warp them to the gmap.
	if (newLevel.ends_with(".gmap"))
	{
		success = warp(newLevel, toPixelPosition({ 0, 0 }, pos), modTime);
	}
	else
	{
		if (auto level = m_server->getLoadedLevel(newLevel, shared_from_this()); level != nullptr)
		{
			// If this level is part of a gmap, send the static data first, then warp second (so the appear on the correct level).
			if (level->isGmap() && level->levelName != newLevel)
			{
				// Send the static data first.
				auto subLevel = level->getSubLevelByName(newLevel);
				success = sendStaticLevelData(subLevel->staticData.lock(), subLevel, modTime);

				// Now warp.
				success = success && warp(level, toPixelPosition(level->getSubLevelOrigin(subLevel).value_or(PixelPosition{}), pos), modTime);
			}
			// Otherwise, just enter the level.
			else
			{
				success = enterLevel(level, toPixelPosition({ 0, 0 }, pos), modTime);
			}
		}
	}

	// If we failed, try to resolve this.
	if (!success)
	{
		if (auto level = getLevel(); level != nullptr)
			success = warp(level->levelName, account.character.getLocalPosition());
	}
	if (!success)
	{
		CString unstickLevel = m_server->getSettings().getStr("unstickmelevel", "onlinestartlocal.nw");
		float unstickX = m_server->getSettings().getFloat("unstickmex", 30.0f);
		float unstickY = m_server->getSettings().getFloat("unstickmey", 30.5f);
		warp(unstickLevel, { static_cast<int16_t>(unstickX * 16.0f), static_cast<int16_t>(unstickY * 16.0f) });
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BOARDMODIFY(CString& pPacket)
{
	// Bushes, grasses, swamp, snow grass, desert grass.
	constexpr std::array<uint16_t, 7> dropTiles = { 0x002, 0x1a4, 0x1ff, 0x7ff, 0x3ff, 0x5d9, 0x34f };

	CSettings& settings = m_server->getSettings();
	uint8_t loc[2] = { pPacket.readGUChar(), pPacket.readGUChar() };
	uint8_t dim[2] = { pPacket.readGUChar(), pPacket.readGUChar() };
	CString tiles = pPacket.readString("");

	auto level = getLevel();
	if (level == nullptr)
		return HandlePacketResult::Handled;

	auto globalPosition = toPixelPosition(getSubLevelOrigin(), LocalWholeTilePosition{ loc[0], loc[1] });

	// Alter level data.
	if (level->alterBoard(tiles, { toWholeTilePosition(globalPosition), { dim[0], dim[1] } }, this))
	{
		if (!level->isGmap())
			m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_BOARDMODIFY << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });
		else
		{
			auto mapPosition = getMapPosition();
			m_server->sendPacketToNearby(CString() >> (char)PLO_BOARDMODIFY2 >> (char)mapPosition.x() >> (char)mapPosition.y() << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });
		}
	}

	if (loc[0] < 0 || loc[0] > 63 || loc[1] < 0 || loc[1] > 63)
		return HandlePacketResult::Handled;

	// Older clients drop items clientside.
	if (m_versionId < CLVER_2_1)
		return HandlePacketResult::Handled;

	// Lay items when you destroy objects.
	auto levelTiles = level->getTiles(getMapPosition());
	if (!levelTiles.has_value())
		return HandlePacketResult::Handled;

	auto oldTile = levelTiles.value()->at(loc[0] + static_cast<size_t>(loc[1] * 64));
	bool bushitems = settings.getBool("bushitems", true);
	bool vasesdrop = settings.getBool("vasesdrop", true);
	LevelItemType dropItem = LevelItemType::INVALID;

	// If we support item drops and the tile is in the allowed list, drop the item.
	if (std::ranges::contains(dropTiles, oldTile) && bushitems)
	{
		dropItem = m_server->rollBushItemDrop();
	}
	// Vases drop hearts.
	else if (oldTile == 0x2ac && vasesdrop)
	{
		dropItem = LevelItemType::HEART;
	}

	// Send the item now.
	if (dropItem != LevelItemType::INVALID)
		level->addItem(inform_client, toPixelPosition(getSubLevelOrigin(), LocalWholeTilePosition{ loc[0], loc[1] }), dropItem);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_REQUESTUPDATEBOARD(CString& pPacket)
{
	// {130}{CHAR level length}{level}{INT5 modtime}{SHORT x}{SHORT y}{SHORT width}{SHORT height}
	CString level = pPacket.readChars(pPacket.readGUChar());

	time_t modTime = (time_t)pPacket.readGUInt5();

	short x = pPacket.readGShort();
	short y = pPacket.readGShort();
	short w = pPacket.readGShort();
	short h = pPacket.readGShort();

	// TODO: What to return?
	log::printLine(log::server, "Received PLI_REQUESTUPDATEBOARD - level: {} - x: {} - y: {} - w: {} - h: {} - modtime: {}", level, x, y, w, h, modTime);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_NPCPROPS(CString& pPacket)
{
	// Don't accept if we have an npc-server.
	if (m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	unsigned int npcId = pPacket.readGUInt();
	CString npcProps = pPacket.readString("");

	//printf( "npcId: %d\n", npcId );
	//printf( "pPacket: %s\n", npcProps.text());
	//for (int i = 0; i < pPacket.length(); ++i) printf( "%02x ", (unsigned char)pPacket[i] );
	//printf( "\n" );

	auto npc = m_server->getNPC(npcId);
	if (!npc)
		return HandlePacketResult::Handled;

	if (auto level = getLevel(); npc->getLevel() != level)
		return HandlePacketResult::Handled;

	npc->setPropsFromPacket(npcProps, shared_from_this());

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BOMBADD(CString& pPacket)
{
	float loc[2] = { (pPacket.readGUChar() % 128) / 2.0f, (pPacket.readGUChar() % 128) / 2.0f };
	[[maybe_unused]] unsigned char player_power = pPacket.readGUChar();
	[[maybe_unused]] unsigned char player = player_power >> 2;
	[[maybe_unused]] unsigned char power = player_power & 0x03;

	// How many 0.05 sec increments until it explodes.
	// It takes 3 seconds for a bomb to explode, but by the time the client sends the packet, it has already counted down to 2.75 seconds.
	// The 0 is counted as a 0.05 second increment, so we add 50ms to the total.
	[[maybe_unused]] std::chrono::milliseconds timeToExplode = (pPacket.readGUChar() * 50ms) + 50ms;

	if (auto level = getLevel(); level != nullptr)
	{
		if (m_server->hasNPCServer())
		{
			auto position = toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]);
			if (level->addBombFromClient(position, power, m_id, timeToExplode) == nullptr)
				sendPacket(CString() >> (char)PLO_BOMBDEL >> (char)(loc[0] * 2) >> (char)(loc[1] * 2));
		}
		else
		{
			m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_BOMBADD >> (short)m_id << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BOMBDEL(CString& pPacket)
{
	if (auto level = getLevel(); level != nullptr)
	{
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_BOMBDEL << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });

		float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
		level->removeBomb(toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]));
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_HORSEADD(CString& pPacket)
{
	if (auto level = getLevel(); level != nullptr)
	{
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_HORSEADD << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });

		float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
		uint8_t dir_bush = pPacket.readGUChar();
		uint8_t hdir = dir_bush & 0x03;
		uint8_t hbushes = dir_bush >> 2;
		CString image = pPacket.readString("");

		level->addHorse(image, toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]), hdir, hbushes);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_HORSEDEL(CString& pPacket)
{
	if (auto level = getLevel(); level != nullptr)
	{
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_HORSEDEL << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });

		float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
		level->removeHorse(toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]));
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_ARROWADD(CString& pPacket)
{
	[[maybe_unused]] float loc[] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	[[maybe_unused]] uint8_t flags = pPacket.readGUChar();
	[[maybe_unused]] uint8_t sprite = pPacket.readGUChar();
	[[maybe_unused]] uint8_t power = pPacket.readGUChar();

	[[maybe_unused]] uint8_t dir = flags & 0b11;
	[[maybe_unused]] bool reflect = (flags & 0b100) != 0;
	[[maybe_unused]] bool fromPlayer = (flags & 0b1000) != 0;

	if (auto level = getLevel(); level != nullptr)
	{
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_ARROWADD >> (short)m_id << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });

		// Add it to the level.
		if (m_server->hasNPCServer())
		{
			PixelPosition speed = { (dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16) };
			level->addArrow(toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]), speed, dir, power, fromPlayer ? source::FromPlayer(m_id) : source::FromServer());
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_FIRESPY(CString& pPacket)
{
	uint8_t length_power = pPacket.readGUChar();
	uint8_t power = length_power & 0b111; // Power is the last three bits.
	uint8_t length = length_power >> 3;   // Length is the first five bits.

	if (auto level = getLevel(); level != nullptr)
	{
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_FIRESPY >> (short)m_id << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });

		// Add it to the level.
		if (m_server->hasNPCServer())
			level->addSpyFire(getGlobalPosition(), source::FromPlayer(m_id), account.character.direction, length, power);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_THROWCARRIED(CString& pPacket)
{
	m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_THROWCARRIED >> (short)m_id, getGlobalPosition(), getLevel(), { m_id });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_ITEMADD(CString& pPacket)
{
	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	uint8_t item = pPacket.readGUChar();
	LevelItemType itemType = LevelItem::getItemId(item);

	// If item drops are disabled, tell the client to delete the item and roll back the changes.
	if (m_server->getSettings().getBool("disableitemdropping", false))
	{
		sendPacket(CString() >> (char)PLO_ITEMDEL >> (char)(loc[0] * 2) >> (char)(loc[1] * 2));
		if (m_server->hasNPCServer())
			addItem(inform_client, itemType);
		return HandlePacketResult::Handled;
	}

	m_server->queueNPCEvent(m_currentLevel.lock(), getGlobalPosition(), ScriptEventType::PLAYERLAYSITEM, source::FromPlayer(m_id));

	// Check if we should send item drop events to the Control-NPC.
	bool itemDropEvents = m_server->getSettings().getBool("itemdropevents", false);
	if (itemDropEvents && m_server->getSettings().getBool("itemdropeventsonlyforgralats", false) && !LevelItem::isRupeeType(itemType))
		itemDropEvents = false;

	// If item drop events are enabled, send the item drop event to the Control-NPC.
	// This will prevent all client item drops, so beware.
	if (itemDropEvents)
	{
		m_server->getNPCServer()->addEventToControlNPC(ScriptEventType::CUSTOM, source::FromPlayer(m_id), "itemdrop", getLevelName(), std::format("{}", loc[0]), std::format("{}", loc[1]), LevelItem::getItemName(itemType));
		sendPacket(CString() >> (char)PLO_ITEMDEL >> (char)(loc[0] * 2) >> (char)(loc[1] * 2));
		return HandlePacketResult::Handled;
	}

	if (auto level = getLevel(); level != nullptr)
	{
		if (m_server->hasNPCServer())
		{
			// Try to drop the item on the level.
			// If the item was ultimately not dropped on the level (e.g., a gralats NPC was created), tell the client to delete it.
			if (!dropItem(toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]), itemType))
				sendPacket(CString() >> (char)PLO_ITEMDEL >> (char)(loc[0] * 2) >> (char)(loc[1] * 2));
		}
		else
		{
			level->addItem(toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]), itemType);
			m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_ITEMADD >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)item, getGlobalPosition(), level, { m_id });
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_ITEMDEL(CString& pPacket)
{
	if (auto level = getLevel(); level != nullptr)
	{
		m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_ITEMDEL << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });

		float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };

		// Remove the item from the level, getting the type of the item in the process.
		LevelItemType item = level->removeItem(toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]));
		if (item == LevelItemType::INVALID) return HandlePacketResult::Handled;

		// If this is a PLI_ITEMTAKE packet, give the item to the player.
		if (pPacket[0] - 32 == PLI_ITEMTAKE)
			this->setPropsFromPacket(CString() << LevelItem::getItemPlayerProp(item, this), props::SetBy::SERVER);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_CLAIMPKER(CString& pPacket)
{
	// Get the player who killed us.
	unsigned int pId = pPacket.readGUShort();
	auto killer = m_server->getPlayer(pId, PLTYPE_ANYCLIENT);
	if (killer == nullptr || killer.get() == this)
		return HandlePacketResult::Handled;

	// Sparring zone rating code.
	// Uses the glicko rating system.
	auto level = getLevel();
	if (level == nullptr) return HandlePacketResult::Handled;
	if (level->isSparringZone(getMapPosition()))
	{
		if (m_server->getSettings().getBool("dontupdateratingd", false) == false)
		{
			// Get some stats we are going to use.
			// Need to parse the other player's PlayerProp::RATING.
			auto otherRating = killer->getProp<PlayerProp::RATING>();
			float oldStats[4] = { account.eloRating, account.eloDeviation, (float)otherRating.rating, (float)otherRating.deviation };

			// If the IPs are the same, don't update the rating to prevent cheating.
			if (CString(m_playerSock->getRemoteIp()) == CString(killer->getSocket()->getRemoteIp()))
				return HandlePacketResult::Handled;

			float gSpar[2] = { static_cast<float>(1.0f / pow((1.0f + 3.0f * pow(0.0057565f, 2) * (pow(oldStats[3], 2)) / pow(3.14159265f, 2)), 0.5f)),   //Winner
							   static_cast<float>(1.0f / pow((1.0f + 3.0f * pow(0.0057565f, 2) * (pow(oldStats[1], 2)) / pow(3.14159265f, 2)), 0.5f)) }; //Loser
			float ESpar[2] = { static_cast<float>(1.0f / (1.0f + pow(10.0f, (-gSpar[1] * (oldStats[2] - oldStats[0]) / 400.0f)))),                       //Winner
							   static_cast<float>(1.0f / (1.0f + pow(10.0f, (-gSpar[0] * (oldStats[0] - oldStats[2]) / 400.0f)))) };                     //Loser
			float dSpar[2] = { static_cast<float>(1.0f / (pow(0.0057565f, 2) * pow(gSpar[0], 2) * ESpar[0] * (1.0f - ESpar[0]))),                        //Winner
							   static_cast<float>(1.0f / (pow(0.0057565f, 2) * pow(gSpar[1], 2) * ESpar[1] * (1.0f - ESpar[1]))) };                      //Loser

			float tWinRating = oldStats[2] + (0.0057565f / (1.0f / powf(oldStats[3], 2) + 1.0f / dSpar[0])) * (gSpar[0] * (1.0f - ESpar[0]));
			float tLoseRating = oldStats[0] + (0.0057565f / (1.0f / powf(oldStats[1], 2) + 1.0f / dSpar[1])) * (gSpar[1] * (0.0f - ESpar[1]));
			float tWinDeviation = powf((1.0f / (1.0f / powf(oldStats[3], 2) + 1 / dSpar[0])), 0.5f);
			float tLoseDeviation = powf((1.0f / (1.0f / powf(oldStats[1], 2) + 1 / dSpar[1])), 0.5f);

			// Cap the rating.
			tWinRating = std::clamp(tWinRating, 0.0f, 4000.0f);
			tLoseRating = std::clamp(tLoseRating, 0.0f, 4000.0f);
			tWinDeviation = std::clamp(tWinDeviation, 50.0f, 350.0f);
			tLoseDeviation = std::clamp(tLoseDeviation, 50.0f, 350.0f);

			// Update the Ratings.
			if (oldStats[0] != tLoseRating || oldStats[1] != tLoseDeviation)
			{
				sendPropsFromResults(setProp<PlayerProp::RATING>(props::SetBy::SERVER, PropertyEloRating{ tLoseRating, tLoseDeviation }));
			}
			if (oldStats[2] != tWinRating || oldStats[3] != tWinDeviation)
			{
				killer->sendPropsFromResults(killer->setProp<PlayerProp::RATING>(props::SetBy::SERVER, PropertyEloRating{ tWinRating, tWinDeviation }));
			}
			this->account.lastSparTime = std::chrono::system_clock::now();
			killer->account.lastSparTime = std::chrono::system_clock::now();
		}
	}
	else
	{
		CSettings& settings = m_server->getSettings();

		// Give a kill to the player who killed me.
		++killer->account.kills;

		// Now, adjust their AP if allowed.
		if (settings.getBool("apsystem", true))
		{
			auto oAp = killer->getProp<PlayerProp::ALIGNMENT>().value;

			// If I have 20 or more AP, they lose AP.
			if (oAp > 0 && account.character.ap > 19)
			{
				int aptime[] = { settings.getInt("aptime0", 30), settings.getInt("aptime1", 90),
								 settings.getInt("aptime2", 300), settings.getInt("aptime3", 600),
								 settings.getInt("aptime4", 1200) };
				oAp -= (((oAp / 20) + 1) * (account.character.ap / 20));
				if (oAp < 0) oAp = 0;
				killer->account.apCounter = (oAp < 20 ? aptime[0] : (oAp < 40 ? aptime[1] : (oAp < 60 ? aptime[2] : (oAp < 80 ? aptime[3] : aptime[4]))));
				killer->setPropsFromPacket(CString() >> (char)PlayerProp::ALIGNMENT >> (char)oAp, props::SetBy::SERVER);
			}
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BADDYPROPS(CString& pPacket)
{
	auto level = getLevel();
	if (level == nullptr || !level->hasPlayers())
		return HandlePacketResult::Handled;

	bool livingBaddies = level->hasLivingBaddies();

	unsigned char id = pPacket.readGUChar();
	CString props = pPacket.readString("");

	// Get the baddy.
	auto baddy = level->getBaddyById(id);
	if (!baddy.has_value() || baddy.value() == nullptr)
		return HandlePacketResult::Handled;

	// Get the leader.
	auto leaderId = level->getPlayers().front();

	// Set the props and send to everybody in the level, except the leader.
	m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_BADDYPROPS >> (char)id << props, getGlobalPosition(), level, { leaderId });
	baddy.value()->setPropsFromPacket(props);

	if (livingBaddies && !level->hasLivingBaddies())
		m_server->queueNPCEventLocal(m_currentLevel.lock(), ScriptEventType::COMPUSDIED, source::FromPlayer(m_id));

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BADDYHURT(CString& pPacket)
{
	auto level = getLevel();
	if (level == nullptr || !level->hasPlayers())
		return HandlePacketResult::Handled;

	auto leaderId = level->getPlayers().front();
	auto leader = m_server->getPlayer(leaderId);
	if (leader == nullptr)
		return HandlePacketResult::Handled;

	leader->sendPacket(CString() >> (char)PLO_BADDYHURT << (pPacket.text() + 1));
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BADDYADD(CString& pPacket)
{
	// Don't add a baddy if we aren't in a level!
	if (m_currentLevel.expired())
		return HandlePacketResult::Handled;

	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	uint8_t bType = pPacket.readGUChar();
	uint8_t bPower = pPacket.readGUChar();
	CString bImage = pPacket.readString("");
	bPower = std::min(bPower, 12_ui8); // Hard-limit to 6 hearts.

	auto level = getLevel();
	if (level == nullptr)
		return HandlePacketResult::Handled;

	// Fix the image for 1.41 clients.
	if (!bImage.isEmpty() && getExtension(bImage).isEmpty())
		bImage << ".gif";

	// Add the baddy.
	LevelBaddy* baddy = level->addBaddy(toLocalPixelPosition(loc[0], loc[1]), static_cast<BaddyType>(bType));
	if (baddy == nullptr) return HandlePacketResult::Handled;

	// Set the baddy props.
	baddy->setRespawn(false);
	baddy->setPropsFromPacket(CString() >> (char)BaddyProp::POWERIMAGE >> (char)bPower >> (char)bImage.length() << bImage);

	// Send the props to everybody in the level.
	m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->id << baddy->getProps(), getGlobalPosition(), level);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_FLAGSET(CString& pPacket)
{
	CSettings& settings = m_server->getSettings();
	CString flagPacket = pPacket.readString("");
	CString flagName, flagValue;

	if (flagPacket.find("=") == -1)
		flagName = flagPacket;
	else
	{
		flagName = flagPacket.readString("=");
		flagValue = flagPacket.readString("");

		// If the value is empty, delete the flag instead.
		if (flagValue.isEmpty())
		{
			pPacket.setRead(1); // Don't let us read the packet ID.
			return msgPLI_FLAGDEL(pPacket);
		}
	}

	// Add a little hack for our special gr.strings.
	if (flagName.find("gr.") != -1)
	{
		if (flagName == "gr.fileerror" || flagName == "gr.filedata")
			return HandlePacketResult::Handled;

		if (settings.getBool("flaghack_movement", true))
		{
			// gr.x and gr.y are used by the -gr_movement NPC to help facilitate smoother
			// movement amongst pre-2.3 clients.
			if (flagName == "gr.x")
			{
				if (m_versionId >= CLVER_2_3) return HandlePacketResult::Handled;
				auto globalPos = getGlobalPosition();
				globalPos.x() = static_cast<int32_t>(atof(flagValue.text()) * 16.0);
				if (auto localPos = toLocalPixelPosition(globalPos); localPos.x() != account.character.localPixelX)
				{
					auto xprop = getProp<PlayerProp::X>();
					xprop.pixelCoordinate = localPos.x();
					m_grMovementPackets >> (char)PlayerProp::X;
					m_grMovementPackets << xprop.serialize();
					m_grMovementPackets << "\n";
				}
				return HandlePacketResult::Handled;
			}
			else if (flagName == "gr.y")
			{
				if (m_versionId >= CLVER_2_3) return HandlePacketResult::Handled;
				auto globalPos = getGlobalPosition();
				globalPos.y() = static_cast<int32_t>(atof(flagValue.text()) * 16.0);
				if (auto localPos = toLocalPixelPosition(globalPos); localPos.y() != account.character.localPixelY)
				{
					auto yprop = getProp<PlayerProp::Y>();
					yprop.pixelCoordinate = localPos.y();
					m_grMovementPackets >> (char)PlayerProp::Y;
					m_grMovementPackets << yprop.serialize();
					m_grMovementPackets << "\n";
				}
				return HandlePacketResult::Handled;
			}
			else if (flagName == "gr.z")
			{
				if (m_versionId >= CLVER_2_3) return HandlePacketResult::Handled;
				float pos = (float)atof(flagValue.text());
				if (pos != account.character.localPixelZ / 16.0f)
					m_grMovementPackets >> (char)PlayerProp::Z >> (char)((pos + 0.5f) + 50.0f) << "\n";
				return HandlePacketResult::Handled;
			}
		}
	}

	// 2.171 clients didn't support this.strings and tried to set them as a
	// normal flag.  Don't allow that.
	if (flagName.find("this.") != -1) return HandlePacketResult::Handled;

	// Don't allow anybody to set read-only strings.
	if (flagName.find("clientr.") != -1) return HandlePacketResult::Handled;
	if (flagName.find("serverr.") != -1) return HandlePacketResult::Handled;

	// Server flags are handled differently than client flags.
	// If we have an npc-server, clients can't set server flags.
	if (!m_server->hasNPCServer())
	{
		if (flagName.find("server.") != -1)
		{
			m_server->setFlag(flagName.toStringView(), flagValue.toString());
			return HandlePacketResult::Handled;
		}
	}

	// Set Flag
	if (flagValue.isEmpty())
		setFlag(flagName.toStringView(), std::nullopt, (m_versionId > CLVER_2_31));
	else setFlag(flagName.toStringView(), flagValue.toString(), (m_versionId > CLVER_2_31));

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_FLAGDEL(CString& pPacket)
{
	CString flagPacket = pPacket.readString("");

	std::string_view flagName;
	bool hasValue = false;

	if (flagPacket.find('=') == -1)
		flagName = flagPacket.toStringView();
	else
	{
		flagName = flagPacket.toStringView();
		flagName = flagName.substr(0, flagName.find('='));
		hasValue = true;
	}

	// this.flags should never be in any server flag list, so just exit.
	if (flagName.find("this.") != std::string_view::npos) return HandlePacketResult::Handled;

	// Don't allow anybody to alter read-only strings.
	if (flagName.find("clientr.") != std::string_view::npos) return HandlePacketResult::Handled;
	if (flagName.find("serverr.") != std::string_view::npos) return HandlePacketResult::Handled;

	// Server flags are handled differently than client flags.
	// TODO: check serveroptions
	if (!m_server->hasNPCServer())
	{
		if (flagName.find("server.") != std::string_view::npos)
		{
			m_server->deleteFlag(std::string{ flagName });
			return HandlePacketResult::Handled;
		}
	}

	// Try to remove the flag.
	if (auto flag = account.variables.get(flagName).lock(); flag != nullptr)
	{
		if (flag->has<std::string>())
		{
			if (hasValue)
				account.variables.remove(flagName);
		}
		else if (flag->has<bool>())
		{
			if (!hasValue)
				account.variables.remove(flagName);
		}
	}
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_OPENCHEST(CString& pPacket)
{
	uint8_t cX = pPacket.readGChar();
	uint8_t cY = pPacket.readGChar();

	if (auto level = getLevel(); level)
	{
		LocalWholeTilePosition chestPos{ cX, cY };
		if (auto chest = level->getChest(getMapPosition(), chestPos); chest.has_value())
		{
			auto levelName = level->getLevelNameAtPosition(getGlobalPosition());
			if (!account.hasChest(levelName, chestPos))
			{
				LevelItemType chestItem = chest.value()->item;
				setPropsFromPacket(CString() << LevelItem::getItemPlayerProp(chestItem, this), props::SetBy::SERVER);
				sendPacket(CString() >> (char)PLO_LEVELCHEST >> (char)1 >> (char)cX >> (char)cY);
				account.savedChests.insert(std::make_pair(level->levelName, chestPos));
			}
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_PUTNPC(CString& pPacket)
{
	// Don't accept if we have an npc-server.
	if (m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	CSettings& settings = m_server->getSettings();

	CString nimage = pPacket.readChars(pPacket.readGUChar());
	CString ncode = pPacket.readChars(pPacket.readGUChar());
	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };

	// See if putnpc is allowed.
	if (!settings.getBool("putnpcenabled"))
		return HandlePacketResult::Handled;

	// Get the file.
	auto file = m_server->getFileSystem().open(fs::FileCategory::FILE, ncode.toStringView());
	if (!file)
		return HandlePacketResult::Handled;

	// Load the code.
	auto code = file->readAsString();
	string::eraseCharsMutate(code, "\r"sv);

	// Add NPC to level
	m_server->addNPC(nimage, code, loc[0], loc[1], m_currentLevel, NPCStorageType::LEVEL, true);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_NPCDEL(CString& pPacket)
{
	// Don't accept if we have an npc-server.
	if (m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	unsigned int nid = pPacket.readGUInt();

	// Remove the NPC.
	if (auto npc = m_server->getNPC(nid); npc)
		m_server->deleteNPC(npc, !m_currentLevel.expired());

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_WANTFILE(CString& pPacket)
{
	// Get file.
	CString file = pPacket.readString("");

	// If we are the 1.41 client, make sure a file extension was sent.
	if (m_versionId < CLVER_2_1 && getExtension(file).isEmpty())
		file << ".gif";

	//printf("WANTFILE: %s\n", file.text());

	// Send file.
	this->sendFile(std::filesystem::path{ file.toString() });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_SHOWIMGPLAYER(CString& pPacket)
{
	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGPLAYER >> (short)m_id << (pPacket.text() + 1), getGlobalPosition(), getLevel(), { m_id });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_HURTPLAYER(CString& pPacket)
{
	unsigned short pId = pPacket.readGUShort();
	char hurtdx = pPacket.readGChar();
	char hurtdy = pPacket.readGChar();
	unsigned char power = pPacket.readGUChar();
	unsigned int npc = pPacket.readGUInt();

	// Get the victim.
	auto victim = m_server->getPlayer(pId, PLTYPE_ANYCLIENT);
	if (victim == 0) return HandlePacketResult::Handled;

	// If they are paused, they don't get hurt.
	if (victim->getProp<PlayerProp::STATUS>().value & PLSTATUS_PAUSED) return HandlePacketResult::Handled;

	// Send the packet.
	victim->sendPacket(CString() >> (char)PLO_HURTPLAYER >> (short)m_id >> (char)hurtdx >> (char)hurtdy >> (char)power >> (int)npc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_EXPLOSION(CString& pPacket)
{
	CSettings& settings = m_server->getSettings();
	if (settings.getBool("noexplosions", false)) return HandlePacketResult::Handled;

	unsigned char eradius = pPacket.readGUChar();
	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	unsigned char epower = pPacket.readGUChar();

	if (auto level = getLevel(); level != nullptr)
	{
		// Send the packet out.
		CString packet = CString() >> (char)PLO_EXPLOSION >> (short)m_id >> (char)eradius >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)epower;
		m_server->sendPacketToOneLevelPart(packet, getGlobalPosition(), level, { m_id });

		// Add it to the level.
		if (m_server->hasNPCServer())
			level->addExplosion(toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]), source::FromPlayer(m_id), eradius, epower);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_PRIVATEMESSAGE(CString& pPacket)
{
	const int sendLimit = 4;
	if (isClient() && timeDifference(m_server->getFrameStartTime(), m_lastMessage) < 4s)
	{
		sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Server message:\xa7You can only send messages once every " << CString((int)sendLimit) << " seconds.");
		return HandlePacketResult::Handled;
	}
	m_lastMessage = m_server->getFrameStartTime();
	return HandlePacketResult::Bubble;
}

HandlePacketResult PlayerClient::msgPLI_NPCWEAPONDEL(CString& pPacket)
{
	std::string weapon = pPacket.readString("").toString();

	// If it is a protected weapon, don't delete it.
	auto protectedWeapons = m_server->getSettings().getStr("protectedweapons").gCommaStrTokens();
	if (std::find(protectedWeapons.begin(), protectedWeapons.end(), weapon) != protectedWeapons.end())
		return HandlePacketResult::Handled;

	std::erase(account.weapons, weapon);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_WEAPONADD(CString& pPacket)
{
	// Don't accept if we have an npc-server.
	if (m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	unsigned char type = pPacket.readGUChar();

	// Type 0 means it is a default weapon.
	if (type == 0)
	{
		this->addWeapon(LevelItem::getItemId(pPacket.readGChar()));
	}
	// NPC weapons.
	else
	{
		// Get the NPC id.
		unsigned int npcId = pPacket.readGUInt();
		auto npc = m_server->getNPC(npcId);
		if (npc == nullptr)
			return HandlePacketResult::Handled;

		// Get the level.
		auto level = npc->getLevel();
		if (level == nullptr)
			return HandlePacketResult::Handled;

		// Get the name of the weapon.
		const auto& name = npc->getWeaponName();
		if (name.length() == 0)
			return HandlePacketResult::Handled;

		// See if we can find the weapon in the server weapon list.
		auto weapon = m_server->getWeapon(name);

		// If weapon is nullptr, that means the weapon was not found.  Add the weapon to the list.
		if (weapon == nullptr)
		{
			weapon = std::make_shared<Weapon>(name, npc->image, std::string{ npc->getScript().getOriginalSource() });
			weapon->saveWeapon();
			m_server->NC_AddWeapon(weapon);
		}

		// Check and see if the weapon has changed recently.  If it has, we should
		// send the new NPC to everybody on the server.  After updating the script, of course.
		if (weapon->modTime < level->modTime)
		{
			// Update Weapon
			weapon->updateWeapon(npc->image, std::string{ npc->getScript().getOriginalSource() }).saveWeapon();

			// Send to Players
			m_server->updateWeaponForPlayers(weapon);
		}

		// Send the weapon to the player now.
		addWeapon(weapon);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_UPDATEFILE(CString& pPacket)
{
	// Get the packet data and file mod time.
	time_t modTime = pPacket.readGUInt5();
	CString file = pPacket.readString("");

	// If we are the 1.41 client, make sure a file extension was sent.
	if (m_versionId < CLVER_2_1 && getExtension(file).isEmpty())
		file << ".gif";

	auto& fileSystem = m_server->getFileSystem();
	time_t fModTime = 0;

	if (auto info = fileSystem.infoi(fs::FileCategory::ALL, file.toStringView()); info != nullptr)
		fModTime = clock::to_time_t(toSystemClock(info->modifiedTime));

	//printf("UPDATEFILE: %s\n", file.text());

	// Make sure it isn't one of the default files.
	bool isDefault = false;
	for (const auto& defaultFile : DefaultFiles)
	{
		if (file.match(CString(defaultFile.data())))
		{
			isDefault = true;
			break;
		}
	}

	// If the file on disk is different, send it to the player.
	file.setRead(0);
	if (!isDefault)
	{
		if (std::difftime(modTime, fModTime) != 0)
			return msgPLI_WANTFILE(file);
	}

	if (m_versionId < CLVER_2_1)
		sendPacket(CString() >> (char)PLO_FILESENDFAILED << file);
	else
		sendPacket(CString() >> (char)PLO_FILEUPTODATE << file);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_ADJACENTLEVEL(CString& pPacket)
{
	std::optional<clock::time_point> modTime = clock::from_time_t((time_t)pPacket.readGUInt5());

	CString levelNameC = pPacket.readString("");
	std::string_view levelName = levelNameC.toStringView();

	// Check if the adjacent level is on the player's current gmap.
	// The gmap might have customized data.
	if (auto currentLevel = getLevel(); currentLevel != nullptr && currentLevel->isGmap())
	{
		if (auto subLevel = currentLevel->getSubLevelByName(levelName); subLevel != nullptr)
		{
			sendStaticLevelData(subLevel->staticData.lock(), subLevel, modTime);
			return HandlePacketResult::Handled;
		}
	}

	// Otherwise, send the normal static data.
	if (auto cachedData = m_server->getCachedLevelData(levelName); cachedData != nullptr)
		sendStaticLevelData(cachedData, nullptr, modTime);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_HITOBJECTS(CString& pPacket)
{
	float power = (float)pPacket.readGChar() / 2.0f;
	float loc[2] = { (float)pPacket.readGChar() / 2.0f, (float)pPacket.readGChar() / 2.0f };
	int nid = (pPacket.bytesLeft() != 0) ? pPacket.readGUInt() : -1;

	// Construct the packet.
	// {46}{SHORT player_id / 0 for NPC}{CHAR power}{CHAR x}{CHAR y}[{INT npc_id}]
	CString nPacket;
	nPacket >> (char)PLO_HITOBJECTS;
	nPacket >> (short)((nid == -1) ? m_id : 0); // If it came from an NPC, send 0 for the id.
	nPacket >> (char)(power * 2) >> (char)(loc[0] * 2) >> (char)(loc[1] * 2);
	if (nid != -1) nPacket >> (int)nid;

	if (m_server->hasNPCServer())
	{
		if (auto level = getLevel(); level != nullptr)
		{
			auto hitNPCs = level->findIntersectingNPCsForCollision({ static_cast<int16_t>(loc[0] * 16), static_cast<int16_t>(loc[1] * 16) });
			for (const auto& npcId : hitNPCs)
			{
				if (auto npc = m_server->getNPC(npcId); npc != nullptr && npc->isCharacter() && npc->visFlags != PROPID(NPCVisFlags::HIDDEN))
				{
					npc->setPropWith<NPCProp::POWER>(SetBy::SERVER, static_cast<GBYTE1>(std::max(0, (int)npc->getProp<NPCProp::POWER>().value - int(power * 2))));
					npc->hurtAndPush(power, translatePosition(getGlobalPosition(), 24_i32, 32_i32), ScriptEventType::WASHIT, source::FromPlayer(m_id));
				}
			}
		}
	}

	m_server->sendPacketToNearby(nPacket, getGlobalPosition(), getLevel(), {m_id});
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_TRIGGERACTION(CString& pPacket)
{
	// Read packet data
	[[maybe_unused]] unsigned int npcId = pPacket.readGUInt();
	float loc[2] = {
		(float)pPacket.readGUChar() / 2.0f,
		(float)pPacket.readGUChar() / 2.0f
	};

	PixelPosition pixelLoc{ toPixelPosition(getSubLevelOrigin(), loc[0], loc[1]) };

	// Tokenize the actions.
	auto actionData = pPacket.readString("").toString();
	auto actions = string::fromCSV(actionData);
	if (actions.empty())
		return HandlePacketResult::Handled;

	// Grab action name.
	auto actualActionName{ string::trimMutate(string::toLower(actions[0])) };

	// TODO(joey): move into trigger command dispatcher, some use private player vars.
	{
		CSettings& settings = m_server->getSettings();
		if (settings.getBool("triggerhack_execscript", false))
		{
			if (actualActionName == "gr.es_clear")
			{
				// Clear the parameters.
				m_grExecParameterList.clear();
				return HandlePacketResult::Handled;
			}
			else if (actualActionName == "gr.es_set")
			{
				// Add the parameter to our saved parameter list.
				CString parameters = string::join(actions | std::views::drop(1));
				if (m_grExecParameterList.isEmpty())
					m_grExecParameterList = parameters;
				else
					m_grExecParameterList << "," << parameters;
				return HandlePacketResult::Handled;
			}
			else if (actualActionName == "gr.es_append")
			{
				// Append doesn't add the beginning comma.
				CString parameters = string::join(actions | std::views::drop(1));
				if (m_grExecParameterList.isEmpty())
					m_grExecParameterList = parameters;
				else
					m_grExecParameterList << parameters;
				return HandlePacketResult::Handled;
			}
			else if (actualActionName == "gr.es")
			{
				if (actions.size() > 2)
				{
					CString account = actions[1];
					CString wepname = CString() << "-gr_exec_" << removeExtension(actions[2]);
					CString wepimage = "wbomb1.png";

					auto filePath = std::filesystem::path{ "execscripts" } / actions[2];

					// Load in all the execscripts.
					CString wepscript;
					wepscript.load(filePath.string());

					// Check to see if we were able to load the weapon.
					if (wepscript.isEmpty())
					{
						log::printLine(log::server, "Error: Player {} tried to load execscript {}, but the script was not found.", this->account.name, actions[2]);
						return HandlePacketResult::Handled;
					}

					// Format the weapon script properly.
					wepscript.removeAllI("\r");
					wepscript.replaceAllI("\n", "\xa7");

					// Replace parameters.
					std::vector<CString> parameters = m_grExecParameterList.tokenize(",");
					for (int i = 0; i < (int)parameters.size(); i++)
					{
						CString parmName = "*PARM" + CString(i);
						wepscript.replaceAllI(parmName, parameters[i]);
					}

					// Set all unreplaced parameters to 0.
					for (int i = 0; i < 128; i++)
					{
						CString parmName = "*PARM" + CString(i);
						wepscript.replaceAllI(parmName, "0");
					}

					// Create the weapon packet.
					CString weapon_packet = CString() >> (char)PLO_NPCWEAPONADD >> (char)wepname.length() << wepname >> (char)0 >> (char)wepimage.length() << wepimage >> (char)1 >> (short)wepscript.length() << wepscript;

					// Send it to the players now.
					if (account == "ALLPLAYERS")
						m_server->sendPacketToType(PLTYPE_ANYCLIENT, weapon_packet);
					else
					{
						auto p = m_server->getPlayer(account, PLTYPE_ANYCLIENT);
						if (p) p->sendPacket(weapon_packet);
					}
					m_grExecParameterList.clear();
				}
				return HandlePacketResult::Handled;
			}
		}

		if (settings.getBool("triggerhack_files", false))
		{
			if (actualActionName == "gr.appendfile")
			{
				int start = actionData.find(",") + 1;
				if (start == 0) return HandlePacketResult::Handled;
				int finish = actionData.find(",", start) + 1;
				if (finish == 0) return HandlePacketResult::Handled;

				// Assemble the file name.
				CString filename = actionData.substr(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Load the file.
				CString file;
				file.load(CString() << "logs/" << filename);

				// Save the file.
				file << actionData.substr(finish) << "\r\n";
				file.save(CString() << "logs/" << filename);
				return HandlePacketResult::Handled;
			}
			else if (actualActionName == "gr.writefile")
			{
				int start = actionData.find(",") + 1;
				if (start == 0) return HandlePacketResult::Handled;
				int finish = actionData.find(",", start) + 1;
				if (finish == 0) return HandlePacketResult::Handled;

				// Grab the filename.
				CString filename = actionData.substr(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Save the file.
				CString file = CString(actionData.substr(finish)) << "\r\n";
				file.save(CString() << "logs/" << filename);
				return HandlePacketResult::Handled;
			}
			else if (actualActionName == "gr.readfile")
			{
				int start = actionData.find(",") + 1;
				if (start == 0) return HandlePacketResult::Handled;
				int finish = actionData.find(",", start) + 1;
				if (finish == 0) return HandlePacketResult::Handled;

				// Grab the filename.
				CString filename = actionData.substr(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Load the file.
				CString filedata;
				filedata.load(CString() << "logs/" << filename);
				filedata.removeAllI("\r");

				// Tokenize it.
				std::vector<CString> tokens = filedata.tokenize("\n");

				// Find the line.
				int id = rand() % 0xFFFF;
				CString error;
				size_t line = string::toNumber(actionData.substr(finish));
				if (line >= tokens.size())
				{
					// We asked for a line that doesn't exist.  Mark it as an error!
					line = tokens.size() - 1;
					error << CString("1,") + line;
				}

				// Check if an error was set.
				if (error.isEmpty())
					error = "0";

				// Apply the ID.
				error = CString(id) << "," << error;

				// Send it back to the player.
				sendPacket(CString() >> (char)PLO_FLAGSET << "gr.fileerror=" << error);
				sendPacket(CString() >> (char)PLO_FLAGSET << "gr.filedata=" << tokens[line]);
			}
		}

		if (settings.getBool("triggerhack_props", false))
		{
			if (actualActionName == "gr.attr")
			{
				int start = actionData.find(",");
				if (start != -1)
				{
					int attrNum = string::toNumber(actionData.substr(7, std::max(0, start - 7)));
					if (attrNum > 0 && attrNum <= 30)
					{
						++start;
						CString val = actionData.substr(start);
						setPropsFromPacket(CString() >> (char)(GaniAttributePropList[static_cast<size_t>(attrNum) - 1]) >> (char)val.length() << val, props::SetBy::SERVER);
					}
				}
			}
			if (actualActionName == "gr.fullhearts")
			{
				int start = actionData.find(",");
				if (start != -1)
				{
					++start;
					int hearts = string::toNumber(string::trimMutate(actionData.substr(start)));
					sendPropsFromResults(setPropWith<PlayerProp::MAXPOWER>(props::SetBy::SERVER, static_cast<uint8_t>(hearts)));
				}
			}
		}

		if (settings.getBool("triggerhack_levels", false))
		{
			if (actualActionName == "gr.updatelevel")
			{
				auto level = getLevel();
				int start = actionData.find(",");
				if (start == -1)
					level->reload(getMapPosition());
				else
				{
					++start;
					std::string levelName = string::trimMutate(actionData.substr(start));
					if (levelName.empty())
						level->reload(getMapPosition());
					else
					{
						LevelPtr targetLevel;
						if (levelName.ends_with(".singleplayer"))
							targetLevel = m_singleplayerLevels[removeExtension(levelName)];
						else
							targetLevel = m_server->getLoadedLevel(levelName, shared_from_this());
						if (targetLevel != nullptr)
							targetLevel->reload(levelName);
					}
				}
			}
		}
	}

	bool handled = m_server->getTriggerDispatcher().execute(actualActionName, this, actions);
	if (!handled)
	{
		if (actualActionName.starts_with("server") && m_server->hasNPCServer())
		{
			// TODO(Nalin): We really should be sending this to the NPC-Server player, not directly calling the NPC-Server.
			m_server->getNPCServer()->addEventToControlNPC(ScriptEventType::TRIGGERACTION, source::FromPlayer(m_id), actions);
			return HandlePacketResult::Handled;
		}

		if (auto level = getLevel(); level)
		{
			// Send to the level.
			if (m_server->getSettings().getBool("sendplayertriggers", true))
				m_server->sendPacketToOneLevelPart(CString() >> (char)PLO_TRIGGERACTION >> (short)m_id << (pPacket.text() + 1), getGlobalPosition(), level, { m_id });

			// Trigger on level NPCs.
			if (m_server->hasNPCServer())
				m_server->getNPCServer()->addEventToLevelNPCsAtPosition(ScriptEventType::TRIGGERACTION, source::FromPlayer(m_id), level, pixelLoc, actions);
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_TAMPERCHECK(CString& pPacket)
{
	pPacket.readString("");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_SHOOT(CString& pPacket)
{
	if (auto level = getLevel(); level != nullptr)
	{
		ShootPacketWrapper newPacket{};
		[[maybe_unused]] int unknown = pPacket.readGInt(); // May be a shoot id for the npc-server. (5/25d/19) joey: all my tests just give 0, my guess would be different types of projectiles but it never came to fruition

		newPacket.position.x() = 8 * pPacket.readGChar();
		newPacket.position.y() = 8 * pPacket.readGChar();
		newPacket.position.z() = 16 * (pPacket.readGChar() - 50);

		// If the player is on a gmap, we need to convert the local pixel position to a map position.
		if (level->isGmap())
			newPacket.position.translate(getSubLevelOrigin());

		// TODO: calculate offsetx from pixelx/pixely/ - level offset
		newPacket.offsetx = 0;
		newPacket.offsety = 0;
		//if (newPacket.pixelx < 0) {
		//	newPacket.offsetx = -1;
		//}
		//if (newPacket.pixely < 0) {
		//	newPacket.offsety = -1;
		//}

		newPacket.sangle = pPacket.readGUChar();  // 0 to 2*pi  = 0-220
		newPacket.sanglez = pPacket.readGUChar(); // -pi to pi  = 0-220
		newPacket.power = pPacket.readGUChar();   // power = 44 pixel increments
		newPacket.gravity = static_cast<uint8_t>(m_server->Scripting.variables.getValue<double>("gravity").value_or(2.0));
		newPacket.gani = pPacket.readChars(pPacket.readGUChar());

		// This seems to be the length of shootparams, but the client doesn't limit itself and sends the overflow anyway
		[[maybe_unused]] unsigned char someParam = pPacket.readGUChar();
		newPacket.shootParams = pPacket.readString("");

		CString oldPacketBuf = CString() >> (char)PLO_SHOOT >> (short)m_id << newPacket.constructShootV1();
		CString newPacketBuf = CString() >> (char)PLO_SHOOT2 >> (short)m_id << newPacket.constructShootV2();

		m_server->sendPacketToNearby(oldPacketBuf, getGlobalPosition(), level, { m_id }, [](const auto pl) { return pl->getVersion() < CLVER_5_07; });
		m_server->sendPacketToNearby(newPacketBuf, getGlobalPosition(), level, { m_id }, [](const auto pl) { return pl->getVersion() >= CLVER_5_07; });

		if (m_server->hasNPCServer())
			level->addShoot(newPacket.position, newPacket.sangle, newPacket.sanglez, newPacket.power, newPacket.gravity, newPacket.gani, source::FromPlayer(m_id));
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_SHOOT2(CString& pPacket)
{
	if (auto level = getLevel(); level != nullptr)
	{
		ShootPacketWrapper newPacket{};
		newPacket.position.x() = static_cast<int16_t>(pPacket.readGUShort());
		newPacket.position.y() = static_cast<int16_t>(pPacket.readGUShort());
		newPacket.position.z() = static_cast<int16_t>(pPacket.readGUShort());
		newPacket.offsetx = pPacket.readGChar();  // level offset x
		newPacket.offsety = pPacket.readGChar();  // level offset y
		newPacket.sangle = pPacket.readGUChar();  // 0 to 2*pi  = 0-220
		newPacket.sanglez = pPacket.readGUChar(); // -pi to pi  = 0-220
		newPacket.power = pPacket.readGUChar();   // power = 44 pixel increments
		newPacket.gravity = pPacket.readGUChar();
		newPacket.gani = pPacket.readChars(pPacket.readGUShort());
		[[maybe_unused]] unsigned char someParam = pPacket.readGUChar(); // This seems to be the length of shootparams, but the client doesn't limit itself and sends the overflow anyway
		newPacket.shootParams = pPacket.readString("");

		CString oldPacketBuf = CString() >> (char)PLO_SHOOT >> (short)m_id << newPacket.constructShootV1();
		CString newPacketBuf = CString() >> (char)PLO_SHOOT2 >> (short)m_id << newPacket.constructShootV2();

		m_server->sendPacketToNearby(oldPacketBuf, getGlobalPosition(), level, { m_id }, [](const auto pl) { return pl->getVersion() < CLVER_5_07; });
		m_server->sendPacketToNearby(newPacketBuf, getGlobalPosition(), level, { m_id }, [](const auto pl) { return pl->getVersion() >= CLVER_5_07; });

		if (m_server->hasNPCServer())
			level->addShoot(newPacket.position, newPacket.sangle, newPacket.sanglez, newPacket.power, newPacket.gravity / 16.0f, newPacket.gani, source::FromPlayer(m_id));
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_SERVERWARP(CString& pPacket)
{
	CString servername = pPacket.readString("");
	log::printLine(log::server, "{} is requesting serverwarp to {}", account.name, servername);
	m_server->getServerList().sendPacket(CString() >> (char)SVO_SERVERINFO >> (short)m_id << servername);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_PROCESSLIST(CString& pPacket)
{
	std::vector<CString> processes = pPacket.readString("").guntokenize().tokenize("\n");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_ENTERLEVEL(CString& pPacket)
{
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_VERIFYWANTSEND(CString& pPacket)
{
	unsigned long fileChecksum = pPacket.readGUInt5();
	CString fileName = pPacket.readString("");

	// There is a USECHECKSUM flag in the config, and im pretty
	// certain it works similar to this: By always sending the
	// update package the client will respond with another request
	// including the crc32 hashes of all the files in the package
	bool ignoreChecksum = false;
	if (getExtension(fileName) == ".gupd")
		ignoreChecksum = true;

	if (!ignoreChecksum)
	{
		auto info = m_server->getFileSystem().infoi(fs::FileCategory::ALL, fileName.toStringView());
		if (info == nullptr)
			return HandlePacketResult::Handled;

		CString fileData;
		fileData.load(info->file.string());

		if (!fileData.isEmpty())
		{
			if (calculateCrc32Checksum(fileData) == fileChecksum)
			{
				sendPacket(CString() >> (char)PLO_FILEUPTODATE << fileName);
				return HandlePacketResult::Handled;
			}
		}
	}

	// Send the file to the client
	this->sendFile(std::filesystem::path{ fileName.toString() });
	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerClient::msgPLI_UPDATEGANI(CString& pPacket)
{
	// Read packet data
	uint32_t checksum = pPacket.readGUInt5();
	std::string gani = pPacket.readString("").toString();
	const std::string ganiFile = gani + ".gani";

	// Try to find the animation in memory or on disk
	auto findAni = m_server->getAnimationManager().findOrAddResource(ganiFile);
	if (!findAni)
	{
		//printf("Client requested gani %s, but was not found\n", ganiFile.c_str());
		return HandlePacketResult::Handled;
	}

	// Compare the bytecode checksum from the client with the one for the
	// current script, if it doesn't match send the updated bytecode
	if (calculateCrc32Checksum(findAni->getByteCode()) != checksum)
		sendPacket(findAni->getBytecodePacket());

	// Tell the client to load the gani.
	sendPacket(CString() >> (char)PLO_LOADGANI >> (char)gani.length() << gani << "\"SETBACKTO " << findAni->getSetBackTo() << "\"");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_UPDATESCRIPT(CString& pPacket)
{
	CString weaponName = pPacket.readString("");

	if (auto weaponObj = m_server->getWeapon(weaponName.toString()); weaponObj != nullptr)
		weaponObj->sendByteCodeToPlayer(shared_from_this());

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_UPDATECLASS(CString& pPacket)
{
	// Get the checksum and class name.
	uint32_t checkSum = pPacket.readGInt5();
	std::string className = pPacket.readString("").toString();

	if (!m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	auto npcServer = m_server->getNPCServer();
	if (auto classObj = npcServer->getClass(className).lock(); classObj != nullptr)
	{
		if (classObj->getCheckSum() == checkSum)
			return HandlePacketResult::Handled;

		CString classPacket = classObj->getClassPacket();
		sendPacket(CString() >> (char)PLO_RAWDATA >> (int)classPacket.length());
		sendPacket(classPacket);
	}
	else
	{
		std::vector<CString> headerData;
		headerData.push_back("class");
		headerData.push_back(className);
		headerData.push_back('1');
		headerData.push_back(CString() >> (long long)0 >> (long long)0);
		headerData.push_back(CString() >> (long long)0);
		CString gstr = utilities::retokenizeCStringArray(headerData);

		// Should technically be PLO_LOADSCRIPT but for some reason the client breaks player.join() scripts
		// if a weapon decides to request an class that doesnt exist on the server. This seems to fix it by
		// sending an empty bytecode
		sendPacket(CString() >> (char)PLO_NPCWEAPONSCRIPT >> (short)gstr.length() << gstr);
	}

	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerClient::msgPLI_UPDATEPACKAGEREQUESTFILE(CString& pPacket)
{
	CString packageName = pPacket.readChars(pPacket.readGUChar());

	// 1 -> Install, 2 -> Reinstall
	unsigned char installType = pPacket.readGUChar();
	CString fileChecksums = pPacket.readString("");

	// If this is a reinstall, we need to download everything so clear the checksum data
	if (installType == 2)
		fileChecksums.clear();

	auto totalDownloadSize = 0;
	std::vector<std::string> missingFiles;

	{
		auto updatePackage = m_server->getPackageManager().findOrAddResource(packageName.toString());
		if (updatePackage)
		{
			for (const auto& [fileName, entry] : updatePackage->getFileList())
			{
				// Compare the checksum for each file entry if the checksum is provided
				bool needsFile = true;
				if (fileChecksums.bytesLeft() >= 5)
				{
					uint32_t userFileChecksum = fileChecksums.readGUInt5();
					if (entry.checksum == userFileChecksum)
						needsFile = false;
				}

				if (needsFile)
				{
					totalDownloadSize += entry.size;
					missingFiles.push_back(fileName);
				}
			}
		}
	}

	sendPacket(CString() >> (char)PLO_UPDATEPACKAGESIZE >> (char)packageName.length() << packageName >> (long long)totalDownloadSize);

	for (const auto& wantFile : missingFiles)
		this->sendFile(wantFile);

	sendPacket(CString() >> (char)PLO_UPDATEPACKAGEDONE << packageName);
	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
