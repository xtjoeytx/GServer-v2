#include "IDebug.h"
#include <vector>
#include <math.h>

#include "IConfig.h"
#include "TServer.h"
#include "TPlayer.h"
#include "IUtil.h"
#include "TLevel.h"
#include "TMap.h"
#include "TNPC.h"
#include "TWeapon.h"

#define serverlog	server->getServerLog()
#define rclog		server->getRCLog()
extern bool __sendLogin[propscount];
extern bool __getLogin[propscount];
extern bool __getLoginNC[propscount];
extern bool __getRCLogin[propscount];

CString _zlibFix(
"//#CLIENTSIDE\xa7"
"if(playerchats) {\xa7"
"  this.chr = {ascii(#e(0,1,#c)),0,0,0,0};\xa7"
"  for(this.c=0;this.c<strlen(#c)*(strlen(#c)>=11);this.c++) {\xa7"
"    this.chr[2] = ascii(#e(this.c,1,#c));\xa7"
"    this.chr[3] += 1*(this.chr[2]==this.chr[0]);\xa7"
"    if(!(this.chr[2] in {this.chr[0],this.chr[1]})) {\xa7"
"      if(this.chr[1]==0) {\xa7"
"        if(this.chr[2]!=this.chr[0]) this.chr[1]=this.chr[2];\xa7"
"      } else break; //[A][B][C]\xa7"
"    }\xa7"
"    this.chr[4] += 1*(this.chr[2]==this.chr[1]);\xa7"
"    if(this.chr[1]>0 && this.chr[3] in |2,10|) break; //[1<A<11][B]\xa7"
"    if(this.chr[3]>=11 && this.chr[4]>1) break; //[A>=11][B>1]\xa7"
"  }\xa7"
"  if(this.c>0 && this.c == strlen(#c)) setplayerprop #c,\xa0#c\xa0; //Pad\xa7"
"}\xa7"
);

/*
	TPlayer: Manage Account
*/
bool TPlayer::sendLogin()
{
	// We don't need to check if this fails.. because the defaults have already been loaded :)
	loadAccount(accountName, (isRC() || isNC() ? true : false));

	// Check to see if the player is banned or not.
	if (isBanned && !hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You have been banned.  Reason: " << banReason.guntokenize().replaceAll("\n", "\r"));
		return false;
	}

	// If we are an RC, check to see if we can log in.
	if (isRC() || isNC())
	{
		// Check and see if we are allowed in.
		if (!isStaff() || !isAdminIp())
		{
			rclog.out("[%s] Attempted RC login by %s.\n", server->getName().text(), accountName.text());
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "You do not have RC rights.");
			return false;
		}
	}

	// NPC-Control login, then return.
	if (isNC())
		return sendLoginNC();

	// Check to see if we can log in if we are a client.
	if (isClient())
	{
		// Staff only.
		if (server->getSettings().getBool("onlystaff", false) && !isStaff())
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "This server is currently restricted to staff only.");
			return false;
		}

		// Check and see if we are allowed in.
		std::vector<CString> adminIps = adminIp.tokenize(",");
		if (!isAdminIp() && vecSearch<CString>(adminIps, "0.0.0.0") == -1)
		{
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your IP doesn't match one of the allowed IPs for this account.");
			return false;
		}
	}

	// Server Signature
	// 0x49 (73) is used to tell the client that more than eight
	// players will be playing.
	sendPacket(CString() >> (char)PLO_SIGNATURE >> (char)73);

	//if(loginserver) {
	//	sendPacket(CString() >> (char)PLO_FULLSTOP);
	//	sendPacket(CString() >> (char)PLO_GHOSTICON >> (char)1);
	//}

	if (isClient())
	{
#ifdef V8NPCSERVER
		// If we have an NPC Server, send this to prevent clients from sending
		// npc props it modifies.
		//
		// NOTE: This may have been deprecated after v5/v6, don't see it in iLogs
		sendPacket(CString() >> (char)PLO_HASNPCSERVER);
#endif

		sendPacket(CString() >> (char)PLO_UNKNOWN168);
	}

	// Check if the account is already in use.
	if (!getGuest())
	{
		auto& playerList = server->getPlayerList();
		for (auto& [id, player] : playerList)
		{
			CString oacc = player->getAccountName();
			unsigned short oid = (unsigned short)player->getId();
			int meClient = ((type & PLTYPE_ANYCLIENT) ? 0 : ((type & PLTYPE_ANYRC) ? 1 : 2));
			int themClient = ((player->getType() & PLTYPE_ANYCLIENT) ? 0 : ((player->getType() & PLTYPE_ANYRC) ? 1 : 2));
			if (oacc.comparei(accountName) && meClient == themClient && oid != id)
			{
				if ((int)difftime(time(0), player->getLastData()) > 30)
				{
					player->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Someone else has logged into your account.");
					player->disconnect();
				}
				else
				{
					sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Account is already in use.");
					return false;
				}
			}
		}
	}

	// TODO(joey): Placing this here so warp doesn't queue events for this player before
	//	the login is finished. The server should get first dibs on the player.
	server->playerLoggedIn(shared_from_this());

	// Player's load different than RCs.
	bool succeeded = false;
	if (isClient()) succeeded = sendLoginClient();
	else if (isRC()) succeeded = sendLoginRC();
	if (succeeded == false) return false;

	// Set loaded to true so our account is saved when we leave.
	// This also lets us send data.
	loaded = true;

	auto& settings = server->getSettings();

	// Send out what guilds should be placed in the Staff section of the playerlist.
	std::vector<CString> guilds = settings.getStr("staffguilds").tokenize(",");
	CString guildPacket = CString() >> (char)PLO_STAFFGUILDS;
	for (std::vector<CString>::iterator i = guilds.begin(); i != guilds.end(); ++i)
		guildPacket << "\"" << ((CString)(*i)).trim() << "\",";
	sendPacket(guildPacket.remove(guildPacket.length() - 1, 1));

	// Send out the server's available status list options.
	if ((isClient() && versionID >= CLVER_2_1) || isRC())
	{
		// graal doesn't quote these
		CString pliconPacket = CString() >> (char)PLO_STATUSLIST;
		for (const auto& status : server->getStatusList())
			pliconPacket << status.trim() << ",";

		sendPacket(pliconPacket.remove(pliconPacket.length() - 1, 1));
	}

	// This comes after status icons for RC
	if (isRC())
		sendPacket(CString() >> (char)PLO_RC_MAXUPLOADFILESIZE >> (long long)(1048576 * 20));

	// Then during iterating the playerlist to send players to the rc client, it sends addplayer followed by rc chat per person.

	// Exchange props with everybody on the server.
	{
		// RC props are sent in a "special" way.  As in retarded.
		CString myRCProps;
		myRCProps >> (char)PLO_ADDPLAYER >> (short)id
			>> (char)accountName.length() << accountName
			>> (char)PLPROP_CURLEVEL << getProp(PLPROP_CURLEVEL)
			>> (char)PLPROP_PSTATUSMSG << getProp(PLPROP_PSTATUSMSG)
			>> (char)PLPROP_NICKNAME << getProp(PLPROP_NICKNAME)
			>> (char)PLPROP_COMMUNITYNAME << getProp(PLPROP_COMMUNITYNAME);

		// Get our client props.
		CString myClientProps = (isClient() ? getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)) : getProps(__getRCLogin, sizeof(__getRCLogin)/sizeof(bool)));

		CString rcsOnline;
		auto& playerList = server->getPlayerList();
		for (auto& [id, player] : playerList)
		{
			if (player.get() == this) continue;

			// Don't send npc-control players to others
			if (player->isNC()) continue;

			// Send the other player my props.
			// Send my flags to the npcserver.
			player->sendPacket(player->isClient() ? myClientProps : myRCProps);

			// Add Player / RC.
			if (isClient())
				sendPacket(player->isClient() ? player->getProps(__getLogin, sizeof(__getLogin)/sizeof(bool)) : player->getProps(__getRCLogin, sizeof(__getRCLogin)/sizeof(bool)));
			else
			{
				// Level name.  If no level, send an empty space.
				CString levelName = (player->getLevel() ? player->getLevel()->getLevelName() : " ");

				// Get the other player's RC props.
				this->sendPacket(CString()
					>> (char)PLO_ADDPLAYER >> (short)player->getId()
					>> (char)player->getAccountName().length() << player->getAccountName()
					>> (char)PLPROP_CURLEVEL >> (char)levelName.length() << levelName
					>> (char)PLPROP_PSTATUSMSG << player->getProp(PLPROP_PSTATUSMSG)
					>> (char)PLPROP_NICKNAME << player->getProp(PLPROP_NICKNAME)
					>> (char)PLPROP_COMMUNITYNAME << player->getProp(PLPROP_COMMUNITYNAME));

				// If the other player is an RC, add them to the list of logged in RCs.
				if (player->isRC())
					rcsOnline << (rcsOnline.isEmpty() ? "" : ", ") << player->getAccountName();
			}
		}

		// If we are an RC, announce the list of currently logged in RCs.
		if (isRC() && !rcsOnline.isEmpty())
			sendPacket(CString() >> (char)PLO_RC_CHAT << "Currently online: " << rcsOnline);
	}

	// Ask for processes. This causes windows v6 clients to crash
	if (isClient() && versionID < CLVER_6_015)
		sendPacket(CString() >> (char)PLO_LISTPROCESSES);

	return true;
}

bool TPlayer::sendLoginClient()
{
	auto& settings = server->getSettings();

	// Recalculate player spar deviation.
	{
		// c = sqrt( (350*350 - 50*50) / t )
		// where t is the number of rating periods for deviation to go from 50 to 350.
		// t = 60 days for us.
		const float c = 44.721f;
		time_t current_time = time(0);
		time_t periods = (current_time - lastSparTime)/60/60/24;
		if (periods != 0)
		{
			// Find the new deviation.
			float deviate = MIN( sqrt((deviation*deviation) + (c*c) * periods), 350.0f );

			// Set the new rating.
			deviation = deviate;
			lastSparTime = current_time;
		}
	}

	// Send the player his login props.
	sendProps(__sendLogin, sizeof(__sendLogin) / sizeof(bool));

	// Workaround for the 2.31 client.  It doesn't request the map file when used with setmap.
	// So, just send them all the maps loaded into the server.
	if (versionID == CLVER_2_31 || versionID == CLVER_1_411)
	{
		for (const auto & map : server->getMapList())
		{
			if (map->getType() == MapType::BIGMAP)
				msgPLI_WANTFILE(CString() << map->getMapName());
		}
	}

	// Sent to rc and client, but rc ignores it so...
    sendPacket(CString() >> (char)PLO_CLEARWEAPONS);

	// If the gr.ip hack is enabled, add it to the player's flag list.
	if (settings.getBool("flaghack_ip", false) == true)
		this->setFlag("gr.ip", this->accountIpStr, true);

	// Send the player's flags.
	for (auto i = flagList.begin(); i != flagList.end(); ++i)
	{
		if (i->second.isEmpty()) sendPacket(CString() >> (char)PLO_FLAGSET << i->first);
		else sendPacket(CString() >> (char)PLO_FLAGSET << i->first << "=" << i->second);
	}

	// Send the server's flags to the player.
	auto& serverFlags = server->getServerFlags();
	for (auto& [flag, value] : serverFlags)
		sendPacket(CString() >> (char)PLO_FLAGSET << flag << "=" << value);

	// Delete the bomb and bow.  They get automagically added by the client for
	// God knows which reason.  Bomb and Bow must be capitalized.
	sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << "Bomb");
	sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << "Bow");

	// Send the player's weapons.
	for (auto& weaponName : weaponList)
	{
		auto weapon = server->getWeapon(weaponName.toString());
		if (weapon == 0)
		{
			// Let's check to see if it is a default weapon.  If so, we can add it to the server now.
			LevelItemType itemType = TLevelItem::getItemId(weaponName.toString());
			if (itemType != LevelItemType::INVALID)
			{
				CString defWeapPacket = CString() >> (char)PLI_WEAPONADD >> (char)0 >> (char)TLevelItem::getItemTypeId(itemType);
				defWeapPacket.readGChar();
				msgPLI_WEAPONADD(defWeapPacket);
				continue;
			}
			continue;
		}
		sendPacket(weapon->getWeaponPacket(versionID));
	}

	if (versionID >= CLVER_4_0211)
	{
		// Send the player's weapons.
		for (auto & i : server->getClassList())
		{
			if (i.second != nullptr)
				sendPacket(i.second->getClassPacket());
		}
	}

	// Send the zlib fixing NPC to client versions 2.21 - 2.31.
	if (versionID >= CLVER_2_21 && versionID <= CLVER_2_31)
	{
		sendPacket(CString() >> (char)PLO_NPCWEAPONADD
			>> (char)12 << "-gr_zlib_fix"
			>> (char)0 >> (char)1 << "-"
			>> (char)1 >> (short)_zlibFix.length() << _zlibFix);
	}

	// Was blank.  Sent before weapon list.
	sendPacket(CString() >> (char)PLO_UNKNOWN190);

	// Send the level to the player.
	// warp will call sendCompress() for us.
	bool warpSuccess = warp(levelName, x, y);
	if (!warpSuccess && curlevel.expired())
	{
		sendPacket(CString() >> (char)PLO_DISCMESSAGE << "No level available.");
		serverlog.out(CString() << "[" << server->getName() << "] " << "Cannot find level for " << accountName << "\n");
		return false;
	}

	// Send the bigmap if it was set.
	if (isClient() && versionID >= CLVER_2_1)
	{
		CString bigmap = settings.getStr("bigmap");
		if (!bigmap.isEmpty())
		{
			std::vector<CString> vbigmap = bigmap.tokenize(",");
			if (vbigmap.size() == 4)
				sendPacket(CString() >> (char)PLO_BIGMAP << vbigmap[0].trim() << "," << vbigmap[1].trim() << "," << vbigmap[2].trim() << "," << vbigmap[3].trim());
		}
	}

	// Send the minimap if it was set.
	if (isClient() && versionID >= CLVER_2_1)
	{
		CString minimap = settings.getStr("minimap");
		if (!minimap.isEmpty())
		{
			std::vector<CString> vminimap = minimap.tokenize(",");
			if (vminimap.size() == 4)
				sendPacket(CString() >> (char)PLO_MINIMAP << vminimap[0].trim() << "," << vminimap[1].trim() << "," << vminimap[2].trim() << "," << vminimap[3].trim());
		}
	}

	// Send out RPG Window greeting.
	if (isClient() && versionID >= CLVER_2_1)
		sendPacket(CString() >> (char)PLO_RPGWINDOW << "\"Welcome to " << settings.getStr("name") << ".\",\"" << CString(APP_VENDOR) << " " << CString(APP_NAME) << " programmed by " << CString(APP_CREDITS) << ".\"" );

	// Send the start message to the player.
	sendPacket(CString() >> (char)PLO_STARTMESSAGE << server->getServerMessage());

	// This will allow serverwarp and some other things, for some reason.
	sendPacket(CString() >> (char)PLO_SERVERTEXT);

	return true;
}

bool TPlayer::sendLoginNC()
{
	// Send database npcs
	auto& npcList = server->getNPCNameList();
	for (auto& [npcName, npcPtr] : npcList)
	{
		auto npc = npcPtr.lock();
		if (npc == nullptr) continue;

		CString npcPacket = CString() >> (char)PLO_NC_NPCADD >> (int)npc->getId()
			>> (char)NPCPROP_NAME << npc->getProp(NPCPROP_NAME)
			>> (char)NPCPROP_TYPE << npc->getProp(NPCPROP_TYPE)
			>> (char)NPCPROP_CURLEVEL << npc->getProp(NPCPROP_CURLEVEL);
		sendPacket(npcPacket);
	}

	// Send classes
	CString classPacket;
	auto& classList = server->getClassList();
	for (auto it = classList.begin(); it != classList.end(); ++it)
		classPacket >> (char)PLO_NC_CLASSADD << it->first << "\n";
	sendPacket(classPacket);

	// Send list of currently connected NC's
	auto& playerList = server->getPlayerList();
	for (auto& [playerId, player] : playerList)
	{
		if (player.get() != this && player->isNC())
			sendPacket(CString() >> (char)PLO_RC_CHAT << "New NC: " << player->getAccountName());
	}

	// Announce to other nc's that we logged in
	server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_RC_CHAT << "New NC: " << accountName, this);

	loaded = true;
	return true;
}

bool TPlayer::sendLoginRC()
{
	// This packet clears the players weapons on the client, but official
	// also sends it to the RC's so we are maintaining the same behavior
    sendPacket(CString() >> (char)PLO_CLEARWEAPONS);

    // If no nickname was specified, set the nickname to the account name.
	if (nickName.length() == 0)
		nickName = CString("*") << accountName;
	levelName = " ";

	// Set the head to the server's set staff head.
	setHeadImage(server->getSettings().getStr("staffhead", "head25.png"));

	// Send the RC join message to the RC.
	std::vector<CString> rcmessage = CString::loadToken(server->getServerPath() << "config/rcmessage.txt", "\n", true);
	for (const auto & i : rcmessage)
		sendPacket(CString() >> (char)PLO_RC_CHAT << i);

    sendPacket(CString() >> (char)PLO_UNKNOWN190);

    server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << "New RC: " << accountName);
	return true;
}
