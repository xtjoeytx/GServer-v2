#include "ICommon.h"
#include <boost/thread.hpp>
#include "CSocket.h"
#include "CSettings.h"
#include "CFileSystem.h"
#include "TServer.h"

TServer::TServer(CString pName)
: name(pName), lastTimer(time(0)), lastNWTimer(time(0))
{
	// Player ids 0 and 1 break things.  NPC id 0 breaks things.
	// Don't allow anything to have one of those ids.
	playerIds.resize(2);
	npcIds.resize(1);

	// This has the full path to the server directory.
	serverpath = CString() << getHomePath() << "servers/" << name << "/";

	// Set up the log files.
	serverlog.setFilename(CString() << serverpath << "logs/serverlog.txt");
	rclog.setFilename(CString() << serverpath << "logs/rclog.txt");

	serverlist.setServer(this);
	filesystem.setServer(this);
	filesystem_accounts.setServer(this);
}

TServer::~TServer()
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer*>::iterator i = playerList.begin(); i != playerList.end(); )
	{
		delete *i;
		i = playerList.erase(i);
	}

	boost::recursive_mutex::scoped_lock lock_npcList(m_npcList);
	for (std::vector<TNPC*>::iterator i = npcList.begin(); i != npcList.end(); )
	{
		delete *i;
		i = npcList.erase(i);
	}

	boost::recursive_mutex::scoped_lock lock_levelList(m_levelList);
	for (std::vector<TLevel*>::iterator i = levelList.begin(); i != levelList.end(); )
	{
		delete *i;
		i = levelList.erase(i);
	}

	boost::recursive_mutex::scoped_lock lock_mapList(m_mapList);
	for (std::vector<TMap*>::iterator i = mapList.begin(); i != mapList.end(); )
	{
		delete *i;
		i = mapList.erase(i);
	}

	boost::recursive_mutex::scoped_lock lock_weaponList(m_weaponList);
	for (std::vector<TWeapon*>::iterator i = weaponList.begin(); i != weaponList.end(); )
	{
		(*i)->saveWeapon(this);
		delete *i;
		i = weaponList.erase(i);
	}
}

int TServer::init()
{
	// Load Settings
	settings.setSeparator("=");
	settings.loadFile(CString() << serverpath << "config/serveroptions.txt");
	if (!settings.isOpened())
	{
		serverlog.out("** [Error] Could not open config/serveroptions.txt\n");
		return ERR_SETTINGS;
	}

	// Load file system.
	filesystem.addDir("world");
	if (settings.getStr("sharefolder").length() > 0)
	{
		std::vector<CString> folders = settings.getStr("sharefolder").tokenize(",");
		for (std::vector<CString>::iterator i = folders.begin(); i != folders.end(); ++i)
			filesystem.addDir(i->trim());
	}
	filesystem_accounts.addDir("accounts");

	// Load server message.
	servermessage.load(CString() << serverpath << "config/servermessage.html");
	servermessage.removeAllI("\r");
	servermessage.replaceAllI("\n", " ");

	// Load weapons.
	{
		CFileSystem weaponFS(this);
		weaponFS.addDir("weapons");
		std::map<CString, CString>* weaponFileList = weaponFS.getFileList();
		for (std::map<CString, CString>::iterator i = weaponFileList->begin(); i != weaponFileList->end(); ++i)
		{
			TWeapon* weapon = TWeapon::loadWeapon(i->first.removeAll(".txt"), this);
			if (weapon != 0)
				weaponList.push_back(weapon);
		}
	}

	// Load gmaps.
	std::vector<CString> gmaps = settings.getStr("gmaps").guntokenize().tokenize("\n");
	for (std::vector<CString>::iterator i = gmaps.begin(); i != gmaps.end(); ++i)
	{
		// Check for blank lines.
		if (*i == "\r") continue;

		// Load the gmap.
		TMap* gmap = new TMap(MAPTYPE_GMAP);
		if (gmap->load(CString() << *i << ".gmap", this) == false)
		{
			serverlog.out(CString() << "** [Error] Could not load " << *i << ".gmap" << "\n");
			delete gmap;
			continue;
		}

		mapList.push_back(gmap);
	}

	// Load bigmaps.
	std::vector<CString> bigmaps = settings.getStr("maps").guntokenize().tokenize("\n");
	for (std::vector<CString>::iterator i = bigmaps.begin(); i != bigmaps.end(); ++i)
	{
		// Check for blank lines.
		if (*i == "\r") continue;

		// Load the bigmap.
		TMap* bigmap = new TMap(MAPTYPE_BIGMAP);
		if (bigmap->load(*i, this) == false)
		{
			serverlog.out(CString() << "** [Error] Could not load " << *i << "\n");
			delete bigmap;
			continue;
		}

		mapList.push_back(bigmap);
	}

	// Initialize the player socket.
	playerSock.setType(SOCKET_TYPE_SERVER);
	playerSock.setProtocol(SOCKET_PROTOCOL_TCP);
	//playerSock.setOptions(SOCKET_OPTION_NONBLOCKING);
	playerSock.setDescription("playerSock");

	// Start listening on the player socket.
	if (playerSock.init("", settings.getStr("serverport")))
	{
		serverlog.out("** [Error] Could not initialize listening socket.\n");
		return ERR_LISTEN;
	}
	if (playerSock.connect())
	{
		serverlog.out("** [Error] Could not connect listening socket.\n");
		return ERR_LISTEN;
	}

	// Connect to the serverlist.
	if (!serverlist.init(settings.getStr("listip"), settings.getStr("listport")))
	{
		serverlog.out("** [Error] Cound not initialize serverlist socket.\n");
		return ERR_LISTEN;
	}
	serverlist.connectServer();

	return 0;
}

// Called when the TServer is put into its own thread.
void TServer::operator()()
{
	bool running = true;
	while (running)
	{
		// TODO: If something happens, attempt to restart the server.
		if (doMain() == false)
			break;

		try
		{
			boost::this_thread::interruption_point();
		}
		catch (boost::thread_interrupted e)
		{
			boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
			for (std::map<boost::thread::id, boost::thread*>::iterator i = playerThreads.begin(); i != playerThreads.end(); ++i)
			{
				i->second->interrupt();
				i->second->join();
			}
			running = false;
		}
	}
}

bool TServer::doMain()
{
	// If we aren't connected to the serverlist, reconnect.
	if (!serverlist.main())
		serverlist.connectServer();

	// Accept new player connections.
	// Will block for 1 second.
	acceptSock(playerSock);

	// Check for terminated threads.
	if (terminatedThreads.size() != 0)
	{
		for (std::vector<boost::thread::id>::iterator i = terminatedThreads.begin(); i != terminatedThreads.end(); )
		{
			std::map<boost::thread::id, boost::thread*>::iterator j = playerThreads.find(*i);
			if (j != playerThreads.end())
			{
				delete j->second;
				playerThreads.erase(j);
			}
			i = terminatedThreads.erase(i);
		}
	}

	// Every second, do some events.
	if (time(0) != lastTimer) doTimedEvents();

	return true;
}

bool TServer::doTimedEvents()
{
	lastTimer = time(0);

	// Do player events.
	{
		boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
		for (std::vector<TPlayer *>::iterator i = playerList.begin(); i != playerList.end(); ++i)
		{
			TPlayer *player = (TPlayer*)*i;
			if (player == 0)
				continue;

			lock_playerList.unlock();
			if (!player->doTimedEvents())
				player->disconnect();
			lock_playerList.lock();
		}
	}

	// Do level events.
	{
		boost::recursive_mutex::scoped_lock lock_levelList(m_levelList);
		for (std::vector<TLevel *>::iterator i = levelList.begin(); i != levelList.end(); ++i)
		{
			TLevel* level = *i;
			if (level == 0)
				continue;

			lock_levelList.unlock();
			level->doTimedEvents();
			lock_levelList.lock();
		}
	}

	// Send NW time.
	if ((int)difftime(lastTimer, lastNWTimer) >= 5)
	{
		lastNWTimer = lastTimer;
		sendPacketToAll(CString() >> (char)PLO_NEWWORLDTIME << CString().writeGInt4(getNWTime()));
	}

	return true;
}

void TServer::acceptSock(CSocket& pSocket)
{
	// Create socket.
	CSocket *newSock = pSocket.accept(1, 0/*50000*/);	// 1 second
	if (newSock == 0)
		return;

	// Make the new socket blocking.
	newSock->setOptions(0);

	// Get a free id to be assigned to the new player.
	boost::recursive_mutex::scoped_lock lock_playerIds(m_playerIds);
	unsigned int newId = 0;
	for (unsigned int i = 2; i < playerIds.size(); ++i)
	{
		if (playerIds[i] == 0)
		{
			newId = i;
			i = playerIds.size();
		}
	}
	if (newId == 0)
	{
		newId = playerIds.size();
		playerIds.push_back(0);
	}

	// Create the new player.
	TPlayer* newPlayer = new TPlayer(this, newSock, newId);
	playerIds[newId] = newPlayer;

	// Add them to the player list.
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	playerList.push_back(newPlayer);

	// Keep a record of the player's thread.
	boost::thread* pthread = new boost::thread(boost::ref(*newPlayer));
	{
		boost::recursive_mutex::scoped_lock lock_playerThreads(m_playerThreads);
		playerThreads[pthread->get_id()] = pthread;
	}
}

/////////////////////////////////////////////////////

TPlayer* TServer::getPlayer(const unsigned short id) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	if (id >= (unsigned short)playerIds.size()) return 0;
	return playerIds[id];
}

TPlayer* TServer::getPlayer(const CString& account) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		TPlayer *player = (TPlayer*)*i;
		if (player == 0)
			continue;

		// Compare account names.
		if (player->getProp(PLPROP_ACCOUNTNAME).subString(1) == account)
			return player;
	}
	return 0;
}

TNPC* TServer::getNPC(const unsigned int id) const
{
	boost::recursive_mutex::scoped_lock lock_npcIds(m_npcIds);
	if (id >= npcIds.size()) return 0;
	return npcIds[id];
}

TLevel* TServer::getLevel(const CString& pLevel)
{
	return TLevel::findLevel(pLevel, this);
}

TMap* TServer::getMap(const CString& name) const
{
	boost::recursive_mutex::scoped_lock lock_mapList(m_mapList);
	for (std::vector<TMap*>::const_iterator i = mapList.begin(); i != mapList.end(); ++i)
	{
		TMap* map = *i;
		if (map->getMapName() == name)
			return map;
	}
	return 0;
}

TMap* TServer::getMap(const TLevel* pLevel) const
{
	if (pLevel == 0) return 0;

	boost::recursive_mutex::scoped_lock lock_mapList(m_mapList);
	for (std::vector<TMap*>::const_iterator i = mapList.begin(); i != mapList.end(); ++i)
	{
		TMap* pMap = *i;
		if (pMap->isLevelOnMap(pLevel->getLevelName()))
			return pMap;
	}
	return 0;
}

TWeapon* TServer::getWeapon(const CString& name) const
{
	boost::recursive_mutex::scoped_lock lock_weaponList(m_weaponList);
	for (std::vector<TWeapon*>::const_iterator i = weaponList.begin(); i != weaponList.end(); ++i)
	{
		TWeapon* weapon = *i;
		if (weapon->getName() == name)
			return weapon;
	}
	return 0;
}

CString TServer::getFlag(const CString& pName) const
{
	boost::recursive_mutex::scoped_lock lock_serverFlags(m_serverFlags);
	for (std::vector<CString>::const_iterator i = serverFlags.begin(); i != serverFlags.end(); ++i)
	{
		if (*i == pName)
			return *i;
	}
	return CString();
}

TNPC* TServer::addNPC(const CString& pImage, const CString& pScript, float pX, float pY, TLevel* pLevel, bool pLevelNPC, bool sendToPlayers)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// New Npc
	TNPC* newNPC = new TNPC(pImage, pScript, pX, pY, pLevel, pLevelNPC, settings.getBool("trimnpccode", false));
	{
		boost::recursive_mutex::scoped_lock lock_npcList(m_npcList);
		npcList.push_back(newNPC);
	}

	// Assign NPC Id
	bool assignedId = false;
	boost::recursive_mutex::scoped_lock lock_npcIds(m_npcIds);
	for (unsigned int i = 1; i < npcIds.size(); ++i)
	{
		if (npcIds[i] == 0)
		{
			npcIds[i] = newNPC;
			newNPC->setId(i);
			assignedId = true;
		}
	}

	// Assign NPC Id
	if (assignedId == false)
	{
		newNPC->setId(npcIds.size());
		npcIds.push_back(newNPC);
	}
	lock_npcIds.unlock();

	// Send the NPC's props to everybody in range.
	if (sendToPlayers)
	{
		TMap* map = getMap(pLevel);
		if (map && map->getType() == MAPTYPE_GMAP)
			sendPacketToLevel(CString() >> (char)PLO_NPCPROPS >> (int)newNPC->getId() << newNPC->getProps(0), map, pLevel);
		else sendPacketToLevel(CString() >> (char)PLO_NPCPROPS >> (int)newNPC->getId() << newNPC->getProps(0), pLevel);
	}

	return newNPC;
}

bool TServer::deleteNPC(const unsigned int pId, TLevel* pLevel)
{
	// Grab the NPC.
	TNPC* npc = getNPC(pId);
	if (npc == 0) return false;

	return deleteNPC(npc, pLevel);
}

bool TServer::deleteNPC(TNPC* npc, TLevel* pLevel)
{
	if (npc == 0) return false;
	if (npc->getId() >= npcIds.size()) return false;

	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// If pLevel == 0, then it is an npc-server NPC.
	// Not currently supported so just exit.
	if (pLevel == 0) return false;

	// Remove the NPC from all the lists.
	pLevel->removeNPC(npc);
	{
		boost::recursive_mutex::scoped_lock lock_npcIds(m_npcIds);
		npcIds[npc->getId()] = 0;
	}
	{
		boost::recursive_mutex::scoped_lock lock_npcList(m_npcList);
		for (std::vector<TNPC*>::iterator i = npcList.begin(); i != npcList.end(); )
		{
			if ((*i) == npc)
				i = npcList.erase(i);
			else ++i;
		}
	}

	// Tell the client to delete the NPC.
	sendPacketTo(CLIENTTYPE_CLIENT, CString() >> (char)PLO_NPCDEL2 >> (char)npc->getLevel()->getLevelName().length() << npc->getLevel()->getLevelName() >> (int)npc->getId());

	// Delete the NPC from memory.
	delete npc;

	return true;
}

bool TServer::addFlag(const CString& pFlag)
{
	if (settings.getBool("dontaddserverflags", false) == true)
		return false;

	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	CString flag(pFlag);
	CString flagName = flag.readString("=").trim();
	CString flagValue = flag.readString("").trim();
	CString flagNew = CString() << flagName << "=" << flagValue;

	boost::recursive_mutex::scoped_lock lock_serverFlags(m_serverFlags);
	for (std::vector<CString>::iterator i = serverFlags.begin(); i != serverFlags.end(); ++i)
	{
		CString tflagName = i->readString("=").trim();
		if (tflagName == flagName)
		{
			// A flag with a value of 0 means we should unset it.
			if (flagValue.length() == 0)
			{
				sendPacketToAll(CString() >> (char)PLO_FLAGDEL << flagName);
				serverFlags.erase(i);
				return true;
			}

			// If we didn't unset it, alter the existing flag.
			sendPacketToAll(CString() >> (char)PLO_FLAGSET << flagNew);
			*i = flagNew;
			return true;
		}
	}

	// We didn't find a pre-existing flag so let's create a new one.
	sendPacketToAll(CString() >> (char)PLO_FLAGSET << flagNew);
	serverFlags.push_back(flagNew);

	return true;
}

bool TServer::deleteFlag(const CString& pFlag)
{
	if (settings.getBool("dontaddserverflags", false) == true)
		return false;

	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	CString flag(pFlag);
	CString flagName = flag.readString("=").trim();

	// Loop for flags now.
	boost::recursive_mutex::scoped_lock lock_serverFlags(m_serverFlags);
	for (std::vector<CString>::iterator i = serverFlags.begin(); i != serverFlags.end(); )
	{
		CString tflagName = i->readString("=").trim();
		if (tflagName == flagName)
		{
			sendPacketToAll(CString() >> (char)PLO_FLAGDEL << flagName);
			serverFlags.erase(i);
			return true;
		}
	}
	return false;
}

bool TServer::deletePlayer(TPlayer* player)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Add thread to the list of threads to terminate.
	terminatedThreads.push_back(boost::this_thread::get_id());

	// Remove the player from the serverlist.
	serverlist.remPlayer(player->getProp(PLPROP_ACCOUNTNAME).removeI(0,1), player->getType());

	// Get rid of the player now.
	{
		boost::recursive_mutex::scoped_lock lock_playerIds(m_playerIds);
		playerIds[player->getId()] = 0;
	}
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer*>::iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if (*i == player)
		{
			playerList.erase(i);
			break;
		}
	}
	lock_playerList.unlock();

	delete player;

	return true;
}

unsigned int TServer::getNWTime() const
{
	return ((unsigned int)time(0) - 11078 * 24 * 60 * 60) * 2 / 10;
}

/*
	Packet-Sending Functions
*/
void TServer::sendPacketToAll(CString pPacket) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
		(*i)->sendPacket(pPacket);
}

void TServer::sendPacketToAll(CString pPacket, TPlayer *pPlayer) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i) == pPlayer) continue;
		(*i)->sendPacket(pPacket);
	}
}

void TServer::sendPacketToLevel(CString pPacket, TLevel *pLevel) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i)->getType() != CLIENTTYPE_CLIENT) continue;
		if ((*i)->getLevel() == pLevel)
			(*i)->sendPacket(pPacket);
	}
}

void TServer::sendPacketToLevel(CString pPacket, TLevel *pLevel, TPlayer *pPlayer) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i) == pPlayer || (*i)->getType() != CLIENTTYPE_CLIENT) continue;
		if ((*i)->getLevel() == pLevel)
			(*i)->sendPacket(pPacket);
	}
}

void TServer::sendPacketToLevel(CString pPacket, TMap* pMap, TLevel* pLevel) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i)->getType() != CLIENTTYPE_CLIENT) continue;
		if ((*i)->getMap() == pMap)
		{
			int sgmap[2] = {pMap->getLevelX(pLevel->getLevelName()), pMap->getLevelY(pLevel->getLevelName())};
			int ogmap[2];
			switch (pMap->getType())
			{
				case MAPTYPE_GMAP:
					ogmap[0] = (*i)->getProp(PLPROP_GMAPLEVELX).readGUChar();
					ogmap[1] = (*i)->getProp(PLPROP_GMAPLEVELY).readGUChar();
					break;

				default:
				case MAPTYPE_BIGMAP:
					ogmap[0] = pMap->getLevelX((*i)->getLevel()->getLevelName());
					ogmap[1] = pMap->getLevelY((*i)->getLevel()->getLevelName());
					break;
			}

			if (abs(ogmap[0] - sgmap[0]) < 2 && abs(ogmap[1] - sgmap[1]) < 2)
				(*i)->sendPacket(pPacket);
		}
	}
}

void TServer::sendPacketToLevel(CString pPacket, TMap* pMap, TPlayer* pPlayer, bool sendToSelf) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i)->getType() != CLIENTTYPE_CLIENT) continue;
		if ((*i) == pPlayer)
		{
			if (sendToSelf) pPlayer->sendPacket(pPacket);
			continue;
		}
		if ((*i)->getMap() == pMap)
		{
			int ogmap[2], sgmap[2];
			switch (pMap->getType())
			{
				case MAPTYPE_GMAP:
					ogmap[0] = (*i)->getProp(PLPROP_GMAPLEVELX).readGUChar();
					ogmap[1] = (*i)->getProp(PLPROP_GMAPLEVELY).readGUChar();
					sgmap[0] = pPlayer->getProp(PLPROP_GMAPLEVELX).readGUChar();
					sgmap[1] = pPlayer->getProp(PLPROP_GMAPLEVELY).readGUChar();
					break;
				
				default:
				case MAPTYPE_BIGMAP:
					ogmap[0] = pMap->getLevelX((*i)->getLevel()->getLevelName());
					ogmap[1] = pMap->getLevelY((*i)->getLevel()->getLevelName());
					sgmap[0] = pMap->getLevelX(pPlayer->getLevel()->getLevelName());
					sgmap[1] = pMap->getLevelY(pPlayer->getLevel()->getLevelName());
					break;
			}

			if (abs(ogmap[0] - sgmap[0]) < 2 && abs(ogmap[1] - sgmap[1]) < 2)
				(*i)->sendPacket(pPacket);
		}
	}
}

void TServer::sendPacketTo(int who, CString pPacket) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i)->getType() == who)
			(*i)->sendPacket(pPacket);
	}
}

void TServer::sendPacketTo(int who, CString pPacket, TPlayer* pPlayer) const
{
	boost::recursive_mutex::scoped_lock lock_playerList(m_playerList);
	for (std::vector<TPlayer *>::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
	{
		if ((*i) == pPlayer) continue;
		if ((*i)->getType() == who)
			(*i)->sendPacket(pPacket);
	}
}
