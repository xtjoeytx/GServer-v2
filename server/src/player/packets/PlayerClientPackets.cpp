#include <chrono>
#include <ranges>

#include <CString.h>

#include "FileSystem.h"
#include "Server.h"
#include "UpdatePackage.h"

#include "level/Level.h"
#include "level/LevelBaddy.h"
#include "level/LevelBoardChange.h"
#include "level/LevelChest.h"
#include "level/LevelHorse.h"
#include "level/Map.h"
#include "object/NPC.h"
#include "object/PlayerClient.h"
#include "object/Weapon.h"
#include "player/PlayerProps.h"
#include "utilities/StringUtils.h"

///////////////////////////////////////////////////////////////////////////////

#define serverlog m_server->getServerLog()
#define rclog m_server->getRCLog()

///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerClient::msgPLI_LEVELWARP(CString& pPacket)
{
	time_t modTime = 0;

	if (pPacket[0] - 32 == PLI_LEVELWARPMOD)
		modTime = (time_t)pPacket.readGUInt5();

	float loc[2] = { (float)(pPacket.readGChar() / 2.0f), (float)(pPacket.readGChar() / 2.0f) };
	CString newLevel = pPacket.readString("");
	warp(newLevel, loc[0], loc[1], modTime);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BOARDMODIFY(CString& pPacket)
{
	CSettings& settings = m_server->getSettings();
	signed char loc[2] = { pPacket.readGChar(), pPacket.readGChar() };
	signed char dim[2] = { pPacket.readGChar(), pPacket.readGChar() };
	CString tiles = pPacket.readString("");

	// Alter level data.
	auto level = getLevel();
	if (level->alterBoard(tiles, loc[0], loc[1], dim[0], dim[1], this))
		m_server->sendPacketToOneLevel(CString() >> (char)PLO_BOARDMODIFY << (pPacket.text() + 1), level);

	if (loc[0] < 0 || loc[0] > 63 || loc[1] < 0 || loc[1] > 63)
		return HandlePacketResult::Handled;

	// Older clients drop items clientside.
	if (m_versionId < CLVER_2_1)
		return HandlePacketResult::Handled;

	// Lay items when you destroy objects.
	short oldTile = (getLevel()->getTiles())[loc[0] + (loc[1] * 64)];
	bool bushitems = settings.getBool("bushitems", true);
	bool vasesdrop = settings.getBool("vasesdrop", true);
	int tiledroprate = settings.getInt("tiledroprate", 50);
	LevelItemType dropItem = LevelItemType::INVALID;

	// Bushes, grass, swamp.
	if ((oldTile == 2 || oldTile == 0x1a4 || oldTile == 0x1ff ||
		oldTile == 0x3ff) &&
		bushitems)
	{
		if (tiledroprate > 0)
		{
			if ((rand() % 100) < tiledroprate)
			{
				dropItem = LevelItem::getItemId(rand() % 6);
			}
		}
	}
	// Vase.
	else if (oldTile == 0x2ac && vasesdrop)
		dropItem = LevelItemType::HEART;

	// Send the item now.
	// TODO: Make this a more generic function.
	if (dropItem != LevelItemType::INVALID)
	{
		// TODO: GS2 replacement of item drops. How does it work?
		CString packet = CString() >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)LevelItem::getItemTypeId(dropItem);
		CString packet2 = CString() >> (char)PLI_ITEMADD << packet;
		packet2.readGChar(); // So msgPLI_ITEMADD works.

		spawnLevelItem(packet2, false);

		if (getVersion() <= CLVER_5_12)
			sendPacket(CString() >> (char)PLO_ITEMADD << packet);
	}

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
	serverlog.out(":: Received PLI_REQUESTUPDATEBOARD - level: %s - x: %d - y: %d - w: %d - h: %d - modtime: %d\n", level.text(), x, y, w, h, modTime);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_NPCPROPS(CString& pPacket)
{
	// Dont accept npc-properties from clients when an npc-server is present
#ifdef V8NPCSERVER
	return HandlePacketResult::Handled;
#endif

	unsigned int npcId = pPacket.readGUInt();
	CString npcProps = pPacket.readString("");

	//printf( "npcId: %d\n", npcId );
	//printf( "pPacket: %s\n", npcProps.text());
	//for (int i = 0; i < pPacket.length(); ++i) printf( "%02x ", (unsigned char)pPacket[i] );
	//printf( "\n" );

	auto level = getLevel();
	auto npc = m_server->getNPC(npcId);
	if (!npc)
		return HandlePacketResult::Handled;

	if (npc->getLevel() != level)
		return HandlePacketResult::Handled;

	CString packet = CString() >> (char)PLO_NPCPROPS >> (int)npcId;
	packet << npc->setProps(npcProps, m_versionId);
	m_server->sendPacketToLevelOnlyGmapArea(packet, std::dynamic_pointer_cast<PlayerClient>(shared_from_this()), { m_id });

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BOMBADD(CString& pPacket)
{
	// TODO(joey): gmap support
	unsigned char loc[2] = { pPacket.readGUChar(), pPacket.readGUChar() };
	//float loc[2] = {(float)pPacket.readGChar() / 2.0f, (float)pPacket.readGChar() / 2.0f};
	unsigned char player_power = pPacket.readGUChar();
	unsigned char player = player_power >> 2;
	unsigned char power = player_power & 0x03;
	unsigned char timeToExplode = pPacket.readGUChar(); // How many 0.05 sec increments until it explodes.  Defaults to 55 (2.75 seconds.)

	/*
	printf("Place bomb\n");
	printf("Position: (%d, %d)\n", loc[0], loc[1]);
	//printf("Position: (%0.2f, %0.2f)\n", loc[0], loc[1]);
	printf("Player (?): %d\n", player);
	printf("Bomb Power: %d\n", power);
	printf("Bomb Explode Timer: %d\n", timeToExplode);
	//for (int i = 0; i < pPacket.length(); ++i) printf( "%02x ", (unsigned char)pPacket[i] ); printf( "\n" );
	*/

	m_server->sendPacketToOneLevel(CString() >> (char)PLO_BOMBADD >> (short)m_id << (pPacket.text() + 1), m_currentLevel, { m_id });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BOMBDEL(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_BOMBDEL << (pPacket.text() + 1), m_currentLevel, { m_id });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_HORSEADD(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_HORSEADD << (pPacket.text() + 1), m_currentLevel, { m_id });

	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	unsigned char dir_bush = pPacket.readGUChar();
	char hdir = dir_bush & 0x03;
	char hbushes = dir_bush >> 2;
	CString image = pPacket.readString("");

	auto level = getLevel();
	level->addHorse(image, loc[0], loc[1], hdir, hbushes);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_HORSEDEL(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_HORSEDEL << (pPacket.text() + 1), m_currentLevel, { m_id });

	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };

	auto level = getLevel();
	level->removeHorse(loc[0], loc[1]);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_ARROWADD(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_ARROWADD >> (short)m_id << (pPacket.text() + 1), m_currentLevel, { m_id });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_FIRESPY(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_FIRESPY >> (short)m_id << (pPacket.text() + 1), m_currentLevel, { m_id });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_THROWCARRIED(CString& pPacket)
{
	// TODO: Remove when an npcserver is created.
	if (!m_server->getSettings().getBool("duplicatecanbecarried", false) && m_carryNpcId != 0)
	{
		auto npc = m_server->getNPC(m_carryNpcId);
		if (npc)
		{
			m_carryNpcThrown = true;

			// Add the NPC back to the level if it never left.
			auto level = getLevel();
			if (npc->getLevel() == level)
				level->addNPC(npc);
		}
	}
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_THROWCARRIED >> (short)m_id << (pPacket.text() + 1), m_currentLevel, { m_id });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_ITEMADD(CString& pPacket)
{
	spawnLevelItem(pPacket, true);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_ITEMDEL(CString& pPacket)
{
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_ITEMDEL << (pPacket.text() + 1), m_currentLevel, { m_id });

	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };

	// Remove the item from the level, getting the type of the item in the process.
	auto level = getLevel();
	LevelItemType item = level->removeItem(loc[0], loc[1]);
	if (item == LevelItemType::INVALID) return HandlePacketResult::Handled;

	// If this is a PLI_ITEMTAKE packet, give the item to the player.
	if (pPacket[0] - 32 == PLI_ITEMTAKE)
		this->setProps(CString() << LevelItem::getItemPlayerProp(item, this), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);

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
	if (level->isSparringZone())
	{
		// Get some stats we are going to use.
		// Need to parse the other player's PLPROP_RATING.
		unsigned int otherRating = killer->getProp(PLPROP_RATING).readGUInt();
		float oldStats[4] = { account.eloRating, account.eloDeviation, (float)((otherRating >> 9) & 0xFFF), (float)(otherRating & 0x1FF) };

		// If the IPs are the same, don't update the rating to prevent cheating.
		if (CString(m_playerSock->getRemoteIp()) == CString(killer->getSocket()->getRemoteIp()))
			return HandlePacketResult::Handled;

		float gSpar[2] = { static_cast<float>(1.0f / pow((1.0f + 3.0f * pow(0.0057565f, 2) * (pow(oldStats[3], 2)) / pow(3.14159265f, 2)), 0.5f)),   //Winner
						   static_cast<float>(1.0f / pow((1.0f + 3.0f * pow(0.0057565f, 2) * (pow(oldStats[1], 2)) / pow(3.14159265f, 2)), 0.5f)) }; //Loser
		float ESpar[2] = { 1.0f / (1.0f + pow(10.0f, (-gSpar[1] * (oldStats[2] - oldStats[0]) / 400.0f))),                                           //Winner
						   1.0f / (1.0f + pow(10.0f, (-gSpar[0] * (oldStats[0] - oldStats[2]) / 400.0f))) };                                         //Loser
		float dSpar[2] = { static_cast<float>(1.0f / (pow(0.0057565f, 2) * pow(gSpar[0], 2) * ESpar[0] * (1.0f - ESpar[0]))),                        //Winner
						   static_cast<float>(1.0f / (pow(0.0057565f, 2) * pow(gSpar[1], 2) * ESpar[1] * (1.0f - ESpar[1]))) };                      //Loser

		float tWinRating = oldStats[2] + (0.0057565f / (1.0f / powf(oldStats[3], 2) + 1.0f / dSpar[0])) * (gSpar[0] * (1.0f - ESpar[0]));
		float tLoseRating = oldStats[0] + (0.0057565f / (1.0f / powf(oldStats[1], 2) + 1.0f / dSpar[1])) * (gSpar[1] * (0.0f - ESpar[1]));
		float tWinDeviation = powf((1.0f / (1.0f / powf(oldStats[3], 2) + 1 / dSpar[0])), 0.5f);
		float tLoseDeviation = powf((1.0f / (1.0f / powf(oldStats[1], 2) + 1 / dSpar[1])), 0.5f);

		// Cap the rating.
		tWinRating = clip(tWinRating, 0.0f, 4000.0f);
		tLoseRating = clip(tLoseRating, 0.0f, 4000.0f);
		tWinDeviation = clip(tWinDeviation, 50.0f, 350.0f);
		tLoseDeviation = clip(tLoseDeviation, 50.0f, 350.0f);

		// Update the Ratings.
		// setProps will cause it to grab the new rating and send it to everybody in the level.
		// Therefore, just pass a dummy value.  setProps doesn't alter your rating for packet hacking reasons.
		if (oldStats[0] != tLoseRating || oldStats[1] != tLoseDeviation)
		{
			account.eloRating = tLoseRating;
			account.eloDeviation = tLoseDeviation;
			this->setProps(CString() >> (char)PLPROP_RATING >> (int)0, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
		if (oldStats[2] != tWinRating || oldStats[3] != tWinDeviation)
		{
			killer->account.eloRating = tWinRating;
			killer->account.eloRating = tWinDeviation;
			killer->setProps(CString() >> (char)PLPROP_RATING >> (int)0, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		}
		this->account.lastSparTime = std::chrono::system_clock::now();
		killer->account.lastSparTime = std::chrono::system_clock::now();
	}
	else
	{
		CSettings& settings = m_server->getSettings();

		// Give a kill to the player who killed me.
		if (!settings.getBool("dontchangekills", false))
			++killer->account.kills;

		// Now, adjust their AP if allowed.
		if (settings.getBool("apsystem", true))
		{
			signed char oAp = killer->getProp(PLPROP_ALIGNMENT).readGChar();

			// If I have 20 or more AP, they lose AP.
			if (oAp > 0 && account.character.ap > 19)
			{
				int aptime[] = { settings.getInt("aptime0", 30), settings.getInt("aptime1", 90),
								 settings.getInt("aptime2", 300), settings.getInt("aptime3", 600),
								 settings.getInt("aptime4", 1200) };
				oAp -= (((oAp / 20) + 1) * (account.character.ap / 20));
				if (oAp < 0) oAp = 0;
				killer->account.apCounter = (oAp < 20 ? aptime[0] : (oAp < 40 ? aptime[1] : (oAp < 60 ? aptime[2] : (oAp < 80 ? aptime[3] : aptime[4]))));
				killer->setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)oAp, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
			}
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BADDYPROPS(CString& pPacket)
{
	auto level = getLevel();
	if (level == nullptr) return HandlePacketResult::Handled;

	unsigned char id = pPacket.readGUChar();
	CString props = pPacket.readString("");

	// Get the baddy.
	LevelBaddy* baddy = level->getBaddy(id);
	if (baddy == 0) return HandlePacketResult::Handled;

	// Get the leader.
	auto leaderId = level->getPlayers().front();
	auto leader = m_server->getPlayer(leaderId);

	// Set the props and send to everybody in the level, except the leader.
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_BADDYPROPS >> (char)id << props, level, { leaderId });
	baddy->setProps(props);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BADDYHURT(CString& pPacket)
{
	auto level = getLevel();
	auto leaderId = level->getPlayers().front();
	auto leader = m_server->getPlayer(leaderId);
	if (leader == nullptr) return HandlePacketResult::Handled;
	leader->sendPacket(CString() >> (char)PLO_BADDYHURT << (pPacket.text() + 1));
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_BADDYADD(CString& pPacket)
{
	// Don't add a baddy if we aren't in a level!
	if (m_currentLevel.expired())
		return HandlePacketResult::Handled;

	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };
	unsigned char bType = pPacket.readGUChar();
	unsigned char bPower = pPacket.readGUChar();
	CString bImage = pPacket.readString("");
	bPower = MIN(bPower, 12); // Hard-limit to 6 hearts.

	// Fix the image for 1.41 clients.
	if (!bImage.isEmpty() && getExtension(bImage).isEmpty())
		bImage << ".gif";

	// Add the baddy.
	auto level = getLevel();
	LevelBaddy* baddy = level->addBaddy(loc[0], loc[1], bType);
	if (baddy == 0) return HandlePacketResult::Handled;

	// Set the baddy props.
	baddy->setRespawn(false);
	baddy->setProps(CString() >> (char)BDPROP_POWERIMAGE >> (char)bPower >> (char)bImage.length() << bImage);

	// Send the props to everybody in the level.
	m_server->sendPacketToOneLevel(CString() >> (char)PLO_BADDYPROPS >> (char)baddy->getId() << baddy->getProps(), level);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_FLAGSET(CString& pPacket)
{
	CSettings& settings = m_server->getSettings();
	CString flagPacket = pPacket.readString("");
	CString flagName, flagValue;
	if (flagPacket.find("=") != -1)
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
	else
		flagName = flagPacket;

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
				float pos = (float)atof(flagValue.text());
				if (pos != getX())
					m_grMovementPackets >> (char)PLPROP_X >> (char)(pos * 2.0f) << "\n";
				return HandlePacketResult::Handled;
			}
			else if (flagName == "gr.y")
			{
				if (m_versionId >= CLVER_2_3) return HandlePacketResult::Handled;
				float pos = (float)atof(flagValue.text());
				if (pos != getY())
					m_grMovementPackets >> (char)PLPROP_Y >> (char)(pos * 2.0f) << "\n";
				return HandlePacketResult::Handled;
			}
			else if (flagName == "gr.z")
			{
				if (m_versionId >= CLVER_2_3) return HandlePacketResult::Handled;
				float pos = (float)atof(flagValue.text());
				if (pos != getZ())
					m_grMovementPackets >> (char)PLPROP_Z >> (char)((pos + 0.5f) + 50.0f) << "\n";
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
	if (flagName.find("server.") != -1)
	{
		m_server->setFlag(flagName.text(), flagValue);
		return HandlePacketResult::Handled;
	}

	// Set Flag
	this->setFlag(flagName.text(), flagValue, (m_versionId > CLVER_2_31));
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_FLAGDEL(CString& pPacket)
{
	CString flagPacket = pPacket.readString("");
	std::string flagName;
	if (flagPacket.find("=") != -1)
		flagName = flagPacket.readString("=").trim().text();
	else
		flagName = flagPacket.text();

	// this.flags should never be in any server flag list, so just exit.
	if (flagName.find("this.") != std::string::npos) return HandlePacketResult::Handled;

	// Don't allow anybody to alter read-only strings.
	if (flagName.find("clientr.") != std::string::npos) return HandlePacketResult::Handled;
	if (flagName.find("serverr.") != std::string::npos) return HandlePacketResult::Handled;

	// Server flags are handled differently than client flags.
	// TODO: check serveroptions
	if (flagName.find("server.") != std::string::npos)
	{
		m_server->deleteFlag(flagName);
		return HandlePacketResult::Handled;
	}

	// Remove Flag
	this->deleteFlag(flagName);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_OPENCHEST(CString& pPacket)
{
	unsigned char cX = pPacket.readGUChar();
	unsigned char cY = pPacket.readGUChar();

	if (auto level = getLevel(); level)
	{
		if (auto chest = level->getChest(cX, cY); chest.has_value())
		{
			std::string_view levelName = level->getLevelName().toStringView();
			if (!account.hasChest(levelName, cX, cY))
			{
				LevelItemType chestItem = chest.value()->getItemIndex();
				setProps(CString() << LevelItem::getItemPlayerProp(chestItem, this), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
				sendPacket(CString() >> (char)PLO_LEVELCHEST >> (char)1 >> (char)cX >> (char)cY);
				account.savedChests.insert(std::make_pair(levelName, std::make_pair(cX, cY)));
			}
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_PUTNPC(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Disable if we have an NPC-Server.
	return HandlePacketResult::Handled;
#endif

	CSettings& settings = m_server->getSettings();

	CString nimage = pPacket.readChars(pPacket.readGUChar());
	CString ncode = pPacket.readChars(pPacket.readGUChar());
	float loc[2] = { (float)pPacket.readGUChar() / 2.0f, (float)pPacket.readGUChar() / 2.0f };

	// See if putnpc is allowed.
	if (!settings.getBool("putnpcenabled"))
		return HandlePacketResult::Handled;

	// Load the code.
	CString code = m_server->getFileSystem(0)->load(ncode);
	code.removeAllI("\r");

	// Add NPC to level
	m_server->addNPC(nimage, code, loc[0], loc[1], m_currentLevel, false, true);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_NPCDEL(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Disable if we have an NPC-Server.
	return HandlePacketResult::Handled;
#endif

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
	this->sendFile(file);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_SHOWIMG(CString& pPacket)
{
	m_server->sendPacketToLevelArea(CString() >> (char)PLO_SHOWIMG >> (short)m_id << (pPacket.text() + 1), std::dynamic_pointer_cast<PlayerClient>(shared_from_this()), { m_id });
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
	if (victim->getProp(PLPROP_STATUS).readGChar() & PLSTATUS_PAUSED) return HandlePacketResult::Handled;

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

	// Send the packet out.
	CString packet = CString() >> (char)PLO_EXPLOSION >> (short)m_id >> (char)eradius >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)epower;
	m_server->sendPacketToOneLevel(packet, m_currentLevel, { m_id });

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_PRIVATEMESSAGE(CString& pPacket)
{
	// TODO(joey): Is this needed?
	const int sendLimit = 4;
	if (isClient() && (int)difftime(time(0), m_lastMessage) <= 4)
	{
		sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Server message:\xa7You can only send messages once every " << CString((int)sendLimit) << " seconds.");
		return HandlePacketResult::Handled;
	}
	m_lastMessage = time(0);

	// Check if the player is in a jailed level.
	std::vector<CString> jailList = m_server->getSettings().getStr("jaillevels").tokenize(",");
	bool jailed = false;
	for (std::vector<CString>::iterator i = jailList.begin(); i != jailList.end(); ++i)
	{
		if (i->trim() == account.level)
		{
			jailed = true;
			break;
		}
	}

	// Get the players this message was addressed to.
	std::vector<uint16_t> pmPlayers;
	auto pmPlayerCount = pPacket.readGUShort();
	for (auto i = 0; i < pmPlayerCount; ++i)
		pmPlayers.push_back(static_cast<uint16_t>(pPacket.readGUShort()));

	// Start constructing the message based on if it is a mass message or a private message.
	CString pmMessageType("\"\",");
	if (pmPlayerCount > 1) pmMessageType << "\"Mass message:\",";
	else
		pmMessageType << "\"Private message:\",";

	// Grab the message.
	CString pmMessage = pPacket.readString("");
	int messageLimit = 1024;
	if (pmMessage.length() > messageLimit)
	{
		sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Server message:\xa7There is a message limit of " << CString((int)messageLimit) << " characters.");
		return HandlePacketResult::Handled;
	}

	// Word filter.
	pmMessage.guntokenizeI();
	if (isClient())
	{
		int filter = m_server->getWordFilter().apply(this, pmMessage, FILTER_CHECK_PM);
		if (filter & FILTER_ACTION_WARN)
		{
			sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Word Filter:\xa7Your PM could not be sent because it was caught by the word filter.");
			return HandlePacketResult::Handled;
		}
	}

	// Always retokenize string, I don't believe our behavior is inline with official. It was escaping "\", so this unescapes that.
	pmMessage.gtokenizeI();

	// Send the message out.
	for (auto pmPlayerId : pmPlayers)
	{
		if (pmPlayerId >= 16000)
		{
			auto pmPlayer = getExternalPlayer(pmPlayerId);
			if (pmPlayer != nullptr)
			{
				serverlog.out("Sending PM to global player: %s.\n", pmPlayer->account.character.nickName.c_str());
				pmMessage.guntokenizeI();
				pmExternalPlayer(pmPlayer->getServerName(), pmPlayer->account.name, pmMessage);
				pmMessage.gtokenizeI();
			}
		}
		else
		{
			auto pmPlayer = m_server->getPlayer(pmPlayerId, PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
			if (pmPlayer == nullptr || pmPlayer.get() == this) continue;

#ifdef V8NPCSERVER
			if (pmPlayer->isNPCServer())
			{
				m_server->handlePM(this, pmMessage.guntokenize());
				continue;
			}
#endif

			// Don't send to people who don't want mass messages.
			if (pmPlayerCount != 1 && (pmPlayer->getProp(PLPROP_ADDITFLAGS).readGUChar() & PLFLAG_NOMASSMESSAGE))
				continue;

			// Jailed people cannot send PMs to normal players.
			if (jailed && !isStaff() && !pmPlayer->isStaff())
			{
				sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)pmPlayer->getId() << "\"Server Message:\","
																							 << "\"From jail you can only send PMs to admins (RCs).\"");
				continue;
			}

			// Send the message.
			pmPlayer->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)m_id << pmMessageType << pmMessage);
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_NPCWEAPONDEL(CString& pPacket)
{
	std::string weapon = pPacket.readString("").toString();
	std::erase(account.weapons, weapon);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_WEAPONADD(CString& pPacket)
{
#ifdef V8NPCSERVER
	// Disable if we have an NPC-Server.
	return HandlePacketResult::Handled;
#endif

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
		if (npc == nullptr || npc->getLevel() == nullptr)
			return HandlePacketResult::Handled;

		// Get the name of the weapon.
		CString name = npc->getWeaponName();
		if (name.length() == 0)
			return HandlePacketResult::Handled;

		// See if we can find the weapon in the server weapon list.
		auto weapon = m_server->getWeapon(name.toString());

		// If weapon is nullptr, that means the weapon was not found.  Add the weapon to the list.
		if (weapon == nullptr)
		{
			weapon = std::make_shared<Weapon>(name.toString(), npc->getImage(), std::string{ npc->getSource().getClientGS1() }, npc->getLevel()->getModTime(), true);
			m_server->NC_AddWeapon(weapon);
		}

		// Check and see if the weapon has changed recently.  If it has, we should
		// send the new NPC to everybody on the server.  After updating the script, of course.
		if (weapon->getModTime() < npc->getLevel()->getModTime())
		{
			// Update Weapon
			weapon->updateWeapon(npc->getImage(), std::string{ npc->getSource().getClientGS1() }, npc->getLevel()->getModTime());

			// Send to Players
			m_server->updateWeaponForPlayers(weapon);
		}

		// Send the weapon to the player now.
		if (!std::ranges::contains(account.weapons, weapon->getName()))
			this->addWeapon(weapon);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_UPDATEFILE(CString& pPacket)
{
	FileSystem* fileSystem = m_server->getFileSystem();

	// Get the packet data and file mod time.
	time_t modTime = pPacket.readGUInt5();
	CString file = pPacket.readString("");
	time_t fModTime = fileSystem->getModTime(file);

	// If we are the 1.41 client, make sure a file extension was sent.
	if (m_versionId < CLVER_2_1 && getExtension(file).isEmpty())
		file << ".gif";

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
	time_t modTime = pPacket.readGUInt5();
	CString levelName = pPacket.readString("");
	CString packet;
	auto adjacentLevel = Level::findLevel(levelName, m_server);

	if (!adjacentLevel)
		return HandlePacketResult::Handled;

	if (m_currentLevel.expired())
		return HandlePacketResult::Handled;

	bool alreadyVisited = false;
	for (const auto& cl : m_cachedLevels)
	{
		if (auto clevel = cl->level.lock(); clevel == adjacentLevel)
		{
			alreadyVisited = true;
			break;
		}
	}

	// Send the level.
	if (m_versionId >= CLVER_2_1)
		sendLevel(adjacentLevel, modTime, true);
	else
		sendLevel141(adjacentLevel, modTime, true);

	// Set our old level back to normal.
	//sendPacket(CString() >> (char)PLO_LEVELNAME << level->getLevelName());
	auto map = m_pmap.lock();
	if (map && map->getType() == MapType::GMAP)
		sendPacket(CString() >> (char)PLO_LEVELNAME << map->getMapName());
	else
		sendPacket(CString() >> (char)PLO_LEVELNAME << getLevel()->getLevelName());

	if (getLevel()->isPlayerLeader(m_id))
		sendPacket(CString() >> (char)PLO_ISLEADER);

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

	m_server->sendPacketToLevelOnlyGmapArea(nPacket, std::dynamic_pointer_cast<PlayerClient>(shared_from_this()), { m_id });
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_TRIGGERACTION(CString& pPacket)
{
	// Read packet data
	unsigned int npcId = pPacket.readGUInt();
	float loc[2] = {
		(float)pPacket.readGUChar() / 2.0f,
		(float)pPacket.readGUChar() / 2.0f
	};
	CString action = pPacket.readString("").trim();

	// Split action data into tokens
	std::vector<CString> triggerActionData = action.gCommaStrTokens();
	if (triggerActionData.empty())
	{
		return HandlePacketResult::Handled;
	}

	// Grab action name
	std::string actualActionName = triggerActionData[0].toLower().toString();

	// (int)(loc[0]) % 64 == 0.0f, for gmap?
	// TODO(joey): move into trigger command dispatcher, some use private player vars.
	if (loc[0] == 0.0f && loc[1] == 0.0f)
	{
		CSettings& settings = m_server->getSettings();

		if (settings.getBool("triggerhack_execscript", false))
		{
			if (action.find("gr.es_clear") == 0)
			{
				// Clear the parameters.
				m_grExecParameterList.clear();
				return HandlePacketResult::Handled;
			}
			else if (action.find("gr.es_set") == 0)
			{
				// Add the parameter to our saved parameter list.
				CString parameters = action.subString(9);
				if (m_grExecParameterList.isEmpty())
					m_grExecParameterList = parameters;
				else
					m_grExecParameterList << "," << parameters;
				return HandlePacketResult::Handled;
			}
			else if (action.find("gr.es_append") == 0)
			{
				// Append doesn't add the beginning comma.
				CString parameters = action.subString(9);
				if (m_grExecParameterList.isEmpty())
					m_grExecParameterList = parameters;
				else
					m_grExecParameterList << parameters;
				return HandlePacketResult::Handled;
			}
			else if (action.find("gr.es") == 0)
			{
				std::vector<CString> actionParts = action.tokenize(",");
				if (actionParts.size() != 1)
				{
					CString account = actionParts[1];
					CString wepname = CString() << "-gr_exec_" << removeExtension(actionParts[2]);
					CString wepimage = "wbomb1.png";

					// Load in all the execscripts.
					FileSystem execscripts;
					execscripts.addDir("execscripts");
					CString wepscript = execscripts.load(actionParts[2]);

					// Check to see if we were able to load the weapon.
					if (wepscript.isEmpty())
					{
						serverlog.out("Error: Player %s tried to load execscript %s, but the script was not found.\n", this->account.name.c_str(), actionParts[2].text());
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
					if (actionParts[1] == "ALLPLAYERS")
						m_server->sendPacketToType(PLTYPE_ANYCLIENT, weapon_packet);
					else
					{
						auto p = m_server->getPlayer(actionParts[1], PLTYPE_ANYCLIENT);
						if (p) p->sendPacket(weapon_packet);
					}
					m_grExecParameterList.clear();
				}
				return HandlePacketResult::Handled;
			}
		}

		if (settings.getBool("triggerhack_files", false))
		{
			if (action.find("gr.appendfile") == 0)
			{
				int start = action.find(",") + 1;
				if (start == 0) return HandlePacketResult::Handled;
				int finish = action.find(",", start) + 1;
				if (finish == 0) return HandlePacketResult::Handled;

				// Assemble the file name.
				CString filename = action.subString(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Load the file.
				CString file;
				file.load(m_server->getServerPath() << "logs/" << filename);

				// Save the file.
				file << action.subString(finish) << "\r\n";
				file.save(m_server->getServerPath() << "logs/" << filename);
				return HandlePacketResult::Handled;
			}
			else if (action.find("gr.writefile") == 0)
			{
				int start = action.find(",") + 1;
				if (start == 0) return HandlePacketResult::Handled;
				int finish = action.find(",", start) + 1;
				if (finish == 0) return HandlePacketResult::Handled;

				// Grab the filename.
				CString filename = action.subString(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Save the file.
				CString file = action.subString(finish) << "\r\n";
				file.save(m_server->getServerPath() << "logs/" << filename);
				return HandlePacketResult::Handled;
			}
			else if (action.find("gr.readfile") == 0)
			{
				int start = action.find(",") + 1;
				if (start == 0) return HandlePacketResult::Handled;
				int finish = action.find(",", start) + 1;
				if (finish == 0) return HandlePacketResult::Handled;

				// Grab the filename.
				CString filename = action.subString(start, finish - start - 1);
				filename.removeAllI("../");
				filename.removeAllI("..\\");

				// Load the file.
				CString filedata;
				filedata.load(m_server->getServerPath() << "logs/" << filename);
				filedata.removeAllI("\r");

				// Tokenize it.
				std::vector<CString> tokens = filedata.tokenize("\n");

				// Find the line.
				int id = rand() % 0xFFFF;
				CString error;
				size_t line = strtoint(action.subString(finish));
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
			if (action.find("gr.attr") == 0)
			{
				int start = action.find(",");
				if (start != -1)
				{
					int attrNum = strtoint(action.subString(7, start - 7));
					if (attrNum > 0 && attrNum <= 30)
					{
						++start;
						CString val = action.subString(start);
						setProps(CString() >> (char)(GaniAttributePropList[attrNum - 1]) >> (char)val.length() << val, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
					}
				}
			}
			if (action.find("gr.fullhearts") == 0)
			{
				int start = action.find(",");
				if (start != -1)
				{
					++start;
					int hearts = strtoint(action.subString(start).trim());
					setProps(CString() >> (char)PLPROP_MAXPOWER >> (char)hearts, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
				}
			}
		}

		if (settings.getBool("triggerhack_levels", false))
		{
			if (action.find("gr.updatelevel") == 0)
			{
				auto level = getLevel();
				int start = action.find(",");
				if (start != -1)
				{
					++start;
					CString levelName = action.subString(start).trim();
					if (levelName.isEmpty())
						level->reload();
					else
					{
						LevelPtr targetLevel;
						if (getExtension(levelName) == ".singleplayer")
							targetLevel = m_singleplayerLevels[removeExtension(levelName)];
						else
							targetLevel = m_server->getLevel(levelName.toString());
						if (targetLevel != nullptr)
							targetLevel->reload();
					}
				}
				else
					level->reload();
			}
		}
	}

	bool handled = m_server->getTriggerDispatcher().execute(actualActionName, this, triggerActionData);

	if (!handled)
	{
		if (auto level = getLevel(); level)
		{
#ifdef V8NPCSERVER
			// Send to server scripts
			auto npcList = level->findAreaNpcs(int(loc[0] * 16.0), int(loc[1] * 16.0), 8, 8);
			for (auto npcTouched : npcList)
				npcTouched->queueNpcTrigger(actualActionName, this, utilities::retokenizeArray(triggerActionData, 1));
#endif

			// Send to the level.
			m_server->sendPacketToOneLevel(CString() >> (char)PLO_TRIGGERACTION >> (short)m_id << (pPacket.text() + 1), level, { m_id });
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_MAPINFO(CString& pPacket)
{
	// Don't know what this does exactly.  Might be gmap related.
	pPacket.readString("");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_SHOOT(CString& pPacket)
{
	ShootPacketNew newPacket{};
	int unknown = pPacket.readGInt(); // May be a shoot id for the npc-server. (5/25d/19) joey: all my tests just give 0, my guess would be different types of projectiles but it never came to fruition

	newPacket.pixelx = 16 * pPacket.readGChar();        // 16 * ((float)pPacket.readGUChar() / 2.0f);
	newPacket.pixely = 16 * pPacket.readGChar();        // 16 * ((float)pPacket.readGUChar() / 2.0f);
	newPacket.pixelz = 16 * (pPacket.readGChar() - 50); // 16 * ((float)pPacket.readGUChar() / 2.0f);
	// TODO: calculate offsetx from pixelx/pixely/ - level offset
	newPacket.offsetx = 0;
	newPacket.offsety = 0;
	//if (newPacket.pixelx < 0) {
	//	newPacket.offsetx = -1;
	//}
	//if (newPacket.pixely < 0) {
	//	newPacket.offsety = -1;
	//}
	newPacket.sangle = pPacket.readGUChar();  // 0-pi = 0-220
	newPacket.sanglez = pPacket.readGUChar(); // 0-pi = 0-220
	newPacket.speed = pPacket.readGUChar();   // speed = pixels per 0.05 seconds.  In gscript, each value of 1 translates to 44 pixels.
	newPacket.gravity = 8;
	newPacket.gani = pPacket.readChars(pPacket.readGUChar());
	unsigned char someParam = pPacket.readGUChar(); // This seems to be the length of shootparams, but the client doesn't limit itself and sends the overflow anyway
	newPacket.shootParams = pPacket.readString("");

	CString oldPacketBuf = CString() >> (char)PLO_SHOOT >> (short)m_id << newPacket.constructShootV1();
	CString newPacketBuf = CString() >> (char)PLO_SHOOT2 >> (short)m_id << newPacket.constructShootV2();

	m_server->sendPacketToLevelArea(oldPacketBuf, std::dynamic_pointer_cast<PlayerClient>(shared_from_this()), { m_id }, [](const auto pl)
									{
										return pl->getVersion() < CLVER_5_07;
									});
	m_server->sendPacketToLevelArea(newPacketBuf, std::dynamic_pointer_cast<PlayerClient>(shared_from_this()), { m_id }, [](const auto pl)
									{
										return pl->getVersion() >= CLVER_5_07;
									});

	// ActionProjectile on server.
	// TODO(joey): This is accurate, but have not figured out power/zangle stuff yet.

	//this.speed = (this.power > 0 ? 0 : 20 * 0.05);
	//this.horzspeed = cos(this.zangle) * this.speed;
	//this.vertspeed = sin(this.zangle) * this.speed;
	//this.newx = playerx + 1.5; // offset
	//this.newy = playery + 2; // offset
	//function CalcPos() {
	//	this.newx = this.newx + (cos(this.angle) * this.horzspeed);
	//	this.newy = this.newy - (sin(this.angle) * this.horzspeed);
	//	setplayerprop #c, Positions #v(this.newx), #v(this.newy);
	//	if (onwall(this.newx, this.newy)) {
	//		this.calcpos = 0;
	//		this.hittime = timevar2;
	//	}
	//}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_SHOOT2(CString& pPacket)
{
	ShootPacketNew newPacket{};
	newPacket.pixelx = pPacket.readGUShort();
	newPacket.pixely = pPacket.readGUShort();
	newPacket.pixelz = pPacket.readGUShort();
	newPacket.offsetx = pPacket.readGChar();  // level offset x
	newPacket.offsety = pPacket.readGChar();  // level offset y
	newPacket.sangle = pPacket.readGUChar();  // 0-pi = 0-220
	newPacket.sanglez = pPacket.readGUChar(); // 0-pi = 0-220
	newPacket.speed = pPacket.readGUChar();   // speed = pixels per 0.05 seconds.  In gscript, each value of 1 translates to 44 pixels.
	newPacket.gravity = pPacket.readGUChar();
	newPacket.gani = pPacket.readChars(pPacket.readGUShort());
	unsigned char someParam = pPacket.readGUChar(); // This seems to be the length of shootparams, but the client doesn't limit itself and sends the overflow anyway
	newPacket.shootParams = pPacket.readString("");

	CString oldPacketBuf = CString() >> (char)PLO_SHOOT >> (short)m_id << newPacket.constructShootV1();
	CString newPacketBuf = CString() >> (char)PLO_SHOOT2 >> (short)m_id << newPacket.constructShootV2();

	m_server->sendPacketToLevelArea(oldPacketBuf, std::dynamic_pointer_cast<PlayerClient>(shared_from_this()), { m_id }, [](const auto pl)
									{
										return pl->getVersion() < CLVER_5_07;
									});
	m_server->sendPacketToLevelArea(newPacketBuf, std::dynamic_pointer_cast<PlayerClient>(shared_from_this()), { m_id }, [](const auto pl)
									{
										return pl->getVersion() >= CLVER_5_07;
									});

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_SERVERWARP(CString& pPacket)
{
	CString servername = pPacket.readString("");
	m_server->getServerLog().out("%s is requesting serverwarp to %s", account.name.c_str(), servername.text());
	m_server->getServerList().sendPacket(CString() >> (char)SVO_SERVERINFO >> (short)m_id << servername);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_PROCESSLIST(CString& pPacket)
{
	std::vector<CString> processes = pPacket.readString("").guntokenize().tokenize("\n");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_UNKNOWN46(CString& pPacket)
{
#ifdef DEBUG
	printf("TODO: PlayerClient::msgPLI_UNKNOWN46: ");
	CString packet = pPacket.readString("");
	for (int i = 0; i < packet.length(); ++i) printf("%02x ", (unsigned char)packet[i]);
	printf("\n");
#endif
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
		CString fileData = m_server->getFileSystem()->load(fileName);
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
	this->sendFile(fileName);
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

	// v4 and up needs this for some reason.
	sendPacket(CString() >> (char)PLO_UNKNOWN195 >> (char)gani.length() << gani << "\"SETBACKTO " << findAni->getSetBackTo() << "\"");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_UPDATESCRIPT(CString& pPacket)
{
	CString weaponName = pPacket.readString("");

	m_server->getServerLog().out("PLI_UPDATESCRIPT: \"%s\"\n", weaponName.text());

	CString out;

	auto weaponObj = m_server->getWeapon(weaponName.toString());
	if (weaponObj != nullptr)
	{
		CString b = weaponObj->getByteCode();
		out >> (char)PLO_RAWDATA >> (int)b.length() << "\n";
		out >> (char)PLO_NPCWEAPONSCRIPT << b;

		sendPacket(out);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerClient::msgPLI_UPDATECLASS(CString& pPacket)
{
	// Get the packet data and file mod time.
	time_t modTime = pPacket.readGInt5();
	std::string className = pPacket.readString("").toString();

	m_server->getServerLog().out("PLI_UPDATECLASS: \"%s\"\n", className.c_str());

	ScriptClass* classObj = m_server->getClass(className);

	if (classObj != nullptr)
	{
		CString out;
		out >> (char)PLO_RAWDATA >> (int)classObj->getByteCode().length() << "\n";
		out >> (char)PLO_NPCWEAPONSCRIPT << classObj->getByteCode();
		sendPacket(out);
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

		// Should technically be PLO_UNKNOWN197 but for some reason the client breaks player.join() scripts
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
