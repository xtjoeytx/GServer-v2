#include "TPlayer.h"
#include "TServer.h"
#include "CFileSystem.h"

std::vector<CString> TPlayer::getPMServerList()
{
	return PMServerList;
}

bool TPlayer::addPMServer(CString& option)
{
	TServerList* list = server->getServerList();

	bool PMSrvExist = false;
	for (std::vector<CString>::const_iterator ij = PMServerList.begin(); ij != PMServerList.end(); ++ij)
	{
		if ((ij)->text() == option)
		{
			PMSrvExist = true;
		}
	}

	if (!PMSrvExist)
	{
		PMServerList.push_back(option);
		list->sendPacket(CString() >> (char)SVO_REQUESTLIST >> (short)id << CString(CString() << "GraalEngine" << "\n" << "pmserverplayers" << "\n" << option << "\n").gtokenizeI());
		return true;
	}
	else
		return false;
}

bool TPlayer::remPMServer(CString& option)
{
	if (PMServerList.empty())
		return true;

	if (!externalPlayerList.empty())
	{
		// Check if a player has disconnected
		for (auto ij = externalPlayerList.begin(); ij != externalPlayerList.end();)
		{
			TPlayer* p = *ij;

			if (option == p->getServerName())
			{
				short pid = p->getId();
				delete p;
				ij = externalPlayerList.erase(ij);
				sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> pid >> (char)PLPROP_PCONNECTED);
			}
			else
				++ij;
		}
	}

	// Find the player and remove him.
	for (auto ip = PMServerList.begin(); ip != PMServerList.end(); )
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
	int i22 = 0;

	if (!externalPlayerList.empty())
	{
		// Check if a player has disconnected
		for (std::vector<TPlayer*>::iterator ij = externalPlayerList.begin(); ij != externalPlayerList.end();)
		{
			TPlayer* p = *ij;
			bool exist2 = false;
			for (std::vector<CString>::const_iterator i = players2.begin(); i != players2.end(); ++i)
			{
				CString tmpPlyr = (i)->guntokenize();
				CString account = tmpPlyr.readString("\n");
				CString nick = tmpPlyr.readString("\n");
				if (servername == p->getServerName() && account == p->getAccountName())
				{
					exist2 = true;
					p->setNick(CString() << nick << " (on " << servername << ")");

				}

			}
			if (servername == p->getServerName())
			{
				if (!exist2)
				{
					short pid = p->getId();
					delete p;
					ij = externalPlayerList.erase(ij);
					sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> pid >> (char)PLPROP_PCONNECTED);
				}
				else
					++ij;
			}
			else
				++ij;
		}
	}

	for (std::vector<CString>::const_iterator i = players2.begin(); i != players2.end(); ++i)
	{
		CString tmpPlyr = (i)->guntokenize();
		CString account = tmpPlyr.readString("\n");
		CString nick = tmpPlyr.readString("\n");

		bool exist = false;
		if (!externalPlayerList.empty())
		{
			for (std::vector<TPlayer*>::iterator ij = externalPlayerList.begin(); ij != externalPlayerList.end();)
			{
				TPlayer* p = *ij;
				if (servername == p->getServerName() && account == p->getAccountName())
				{
					p->setNick(CString() << nick << " (on " << servername << ")");
					exist = true;
				}
				++ij;
			}
		}

		if (!exist)
		{
			i22 = externalPlayerList.size();
			// Get a free id to be assigned to the new player.
			unsigned int newId = 0;
			for (unsigned int i = 16000; i < externalPlayerIds.size(); ++i)
			{
				if (externalPlayerIds[i] == 0)
				{
					newId = i;
					i = externalPlayerIds.size();
				}
			}
			if (newId == 0)
			{
				newId = externalPlayerIds.size();
				externalPlayerIds.push_back(nullptr);
			}

			TPlayer* tmpPlyr2 = new TPlayer(server, 0, newId);
			externalPlayerIds[newId] = tmpPlyr2;
			tmpPlyr2->loadAccount(account);
			tmpPlyr2->setAccountName(account);
			tmpPlyr2->setServerName(servername);
			tmpPlyr2->setExternal(true);
			tmpPlyr2->setNick(CString() << nick << " (on " << servername << ")");
			tmpPlyr2->setId(newId);

			externalPlayerList.push_back(tmpPlyr2);

		}
	}

	if (!externalPlayerList.empty())
	{
		for (std::vector<TPlayer *>::iterator ij = externalPlayerList.begin(); ij != externalPlayerList.end();)
		{
			sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)(*ij)->getId() >> (char)PLPROP_ACCOUNTNAME << (*ij)->getProp(PLPROP_ACCOUNTNAME) >> (char)PLPROP_NICKNAME << (*ij)->getProp(PLPROP_NICKNAME) >> (char)81 >> (char)0);
			++ij;
		}
	}

	return true;
}

bool TPlayer::pmExternalPlayer(CString servername, CString account, CString& pmMessage)
{
	TServerList* list = server->getServerList();
	list->sendPacket(CString() >> (char)SVO_PMPLAYER >> (short)id << CString(CString() << servername << "\n" << accountName << "\n" << nickName << "\n" << "GraalEngine" << "\n" << "pmplayer" << "\n" << account << "\n" << pmMessage).gtokenizeI());
	return true;
}

TPlayer* TPlayer::getExternalPlayer(const unsigned short id, bool includeRC) const
{
	if (id >= (unsigned short)externalPlayerIds.size()) return 0;
	if (!includeRC && externalPlayerIds[id]->isControlClient()) return 0;
	return externalPlayerIds[id];
}

TPlayer* TPlayer::getExternalPlayer(const CString& account, bool includeRC) const
{
	for (std::vector<TPlayer *>::const_iterator i = externalPlayerList.begin(); i != externalPlayerList.end(); ++i)
	{
		TPlayer *player = (TPlayer*)*i;
		if (player == 0)
			continue;

		if (!includeRC && player->isControlClient())
			continue;

		// Compare account names.
		if (player->getAccountName().toLower() == account.toLower())
			return player;
	}
	return 0;
}
