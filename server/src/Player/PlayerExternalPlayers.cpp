#include "FileSystem.h"
#include "Player.h"
#include "Server.h"

std::vector<CString> TPlayer::getPMServerList()
{
	return PMServerList;
}

bool TPlayer::addPMServer(CString& option)
{
	auto& list = server->getServerList();

	bool PMSrvExist = false;
	for (auto& pmServer: PMServerList)
	{
		if (pmServer.text() == option)
		{
			PMSrvExist = true;
			break;
		}
	}

	if (!PMSrvExist)
	{
		PMServerList.push_back(option);
		list.sendPacket(CString() >> (char)SVO_REQUESTLIST >> (short)id << CString(CString() << "GraalEngine"
																							 << "\n"
																							 << "pmserverplayers"
																							 << "\n"
																							 << option << "\n")
																			   .gtokenizeI());
		return true;
	}
	else
		return false;
}

bool TPlayer::remPMServer(CString& option)
{
	if (PMServerList.empty())
		return true;

	if (!externalPlayers.empty())
	{
		// Check if a player has disconnected
		// By value to keep a hold of the shared_ptr until the next iteration.
		for (auto [externalId, externalPlayer]: externalPlayers)
		{
			if (option == externalPlayer->getServerName())
			{
				// Map iterators are valid after erase.
				externalPlayers.erase(externalId);

				if (isRC())
					sendPacket(CString() >> (char)PLO_DELPLAYER >> externalId);
				else
					sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> externalId >> (char)PLPROP_PCONNECTED);
			}
		}
	}

	// Find the player and remove him.
	for (auto ip = PMServerList.begin(); ip != PMServerList.end();)
	{
		//CString pl = i;
		if ((ip)->text() == option)
		{
			//delete (i);
			ip = PMServerList.erase(ip);
		}
		else
			++ip;
	}

	return true;
}

bool TPlayer::updatePMPlayers(CString& servername, CString& players)
{
	std::vector<CString> players2 = players.tokenize("\n");

	if (!externalPlayers.empty())
	{
		// Check if a player has disconnected
		// By value to keep a hold of the shared_ptr until the next iteration.
		for (auto [externalId, externalPlayer]: externalPlayers)
		{
			bool exist2 = false;
			for (auto& p2: players2)
			{
				CString tmpPlyr = p2.guntokenize();
				CString account = tmpPlyr.readString("\n");
				CString nick    = tmpPlyr.readString("\n");
				if (servername == externalPlayer->getServerName() && account == externalPlayer->getAccountName())
				{
					exist2 = true;
					externalPlayer->setNick(CString() << nick << " (on " << servername << ")");
				}
			}
			if (servername == externalPlayer->getServerName())
			{
				if (!exist2)
				{
					// Map iterators are valid after erase.
					externalPlayers.erase(externalId);

					if (isRC())
						sendPacket(CString() >> (char)PLO_DELPLAYER >> externalId);
					else
						sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> externalId >> (char)PLPROP_PCONNECTED);

					//server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)id >> (char)PLPROP_PCONNECTED, this);
					//server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_DELPLAYER >> (short)id, this);
				}
			}
		}
	}

	for (std::vector<CString>::const_iterator i = players2.begin(); i != players2.end(); ++i)
	{
		CString tmpPlyr = (i)->guntokenize();
		CString account = tmpPlyr.readString("\n");
		CString nick    = tmpPlyr.readString("\n");

		bool exist = false;
		if (!externalPlayers.empty())
		{
			for (auto& [externalId, externalPlayer]: externalPlayers)
			{
				if (servername == externalPlayer->getServerName() && account == externalPlayer->getAccountName())
				{
					externalPlayer->setNick(CString() << nick << " (on " << servername << ")");
					exist = true;
				}
			}
		}

		if (!exist)
		{
			// Get a free id to be assigned to the new player.
			unsigned int newId = nextExternalPlayerId;
			if (!freeExternalPlayerIds.empty())
			{
				newId = *(freeExternalPlayerIds.begin());
				freeExternalPlayerIds.erase(newId);
			}
			else
				++nextExternalPlayerId;

			auto tmpPlyr2          = std::make_shared<TPlayer>(server, nullptr, newId);
			externalPlayers[newId] = tmpPlyr2;
			tmpPlyr2->loadAccount(account);
			tmpPlyr2->setAccountName(account);
			tmpPlyr2->setServerName(servername);
			tmpPlyr2->setExternal(true);
			tmpPlyr2->setNick(CString() << nick << " (on " << servername << ")");
			tmpPlyr2->setId(newId);
		}
	}

	if (!externalPlayers.empty())
	{
		for (auto& [externalId, externalPlayer]: externalPlayers)
		{
			if (isRC())
			{
				sendPacket(CString() >> (char)PLO_ADDPLAYER >> (short)externalId << externalPlayer->getProp(PLPROP_ACCOUNTNAME) >> (char)PLPROP_NICKNAME << externalPlayer->getProp(PLPROP_NICKNAME) >> (char)PLPROP_UNKNOWN81 >> (char)1);
			}
			else
			{
				sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)externalId >> (char)PLPROP_ACCOUNTNAME << externalPlayer->getProp(PLPROP_ACCOUNTNAME) >> (char)PLPROP_NICKNAME << externalPlayer->getProp(PLPROP_NICKNAME) >> (char)PLPROP_UNKNOWN81 >> (char)(1));
			}
		}
	}

	return true;
}

bool TPlayer::pmExternalPlayer(CString servername, CString account, CString& pmMessage)
{
	auto& list = server->getServerList();
	list.sendPacket(CString() >> (char)SVO_PMPLAYER >> (short)id << CString(CString() << servername << "\n"
																					  << accountName << "\n"
																					  << nickName << "\n"
																					  << "GraalEngine"
																					  << "\n"
																					  << "pmplayer"
																					  << "\n"
																					  << account << "\n"
																					  << pmMessage)
																		.gtokenizeI());
	return true;
}

TPlayerPtr TPlayer::getExternalPlayer(const unsigned short id, bool includeRC) const
{
	auto iter = externalPlayers.find(id);
	if (iter == std::end(externalPlayers)) return nullptr;

	auto& externalPlayer = iter->second;
	if (!includeRC && externalPlayer->isControlClient()) return nullptr;
	return externalPlayer;
}

TPlayerPtr TPlayer::getExternalPlayer(const CString& account, bool includeRC) const
{
	for (auto& [externalId, externalPlayer]: externalPlayers)
	{
		if (externalPlayer == 0)
			continue;

		if (!includeRC && externalPlayer->isControlClient())
			continue;

		// Compare account names.
		if (externalPlayer->getAccountName().toLower() == account.toLower())
			return externalPlayer;
	}
	return nullptr;
}
