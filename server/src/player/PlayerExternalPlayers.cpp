#include "FileSystem.h"
#include "Server.h"
#include "object/Player.h"
#include "utilities/StringUtils.h"

std::vector<CString> Player::getPMServerList()
{
	return m_privateMessageServerList;
}

bool Player::addPMServer(CString& option)
{
	auto& list = m_server->getServerList();

	bool PMSrvExist = false;
	for (auto& pmServer: m_privateMessageServerList)
	{
		if (pmServer.text() == option)
		{
			PMSrvExist = true;
			break;
		}
	}

	if (!PMSrvExist)
	{
		m_privateMessageServerList.push_back(option);
		list.sendPacket(CString() >> (char)SVO_REQUESTLIST >> (short)m_id << CString(CString() << "GraalEngine"
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

bool Player::remPMServer(CString& option)
{
	if (m_privateMessageServerList.empty())
		return true;

	if (!m_externalPlayers.empty())
	{
		// Check if a player has disconnected
		// By value to keep a hold of the shared_ptr until the next iteration.
		for (const auto& [externalId, externalPlayer]: m_externalPlayers)
		{
			if (option == externalPlayer->getServerName())
			{
				// Map iterators are valid after erase.
				m_externalPlayers.erase(externalId);

				if (isRC())
					sendPacket(CString() >> (char)PLO_DELPLAYER >> externalId);
				else
					sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> externalId >> (char)PLPROP_PCONNECTED);
			}
		}
	}

	// Find the player and remove him.
	for (auto ip = m_privateMessageServerList.begin(); ip != m_privateMessageServerList.end();)
	{
		//CString pl = i;
		if ((ip)->text() == option)
		{
			//delete (i);
			ip = m_privateMessageServerList.erase(ip);
		}
		else
			++ip;
	}

	return true;
}

bool Player::updatePMPlayers(CString& servername, CString& players)
{
	std::vector<CString> players2 = players.tokenize("\n");

	if (!m_externalPlayers.empty())
	{
		// Check if a player has disconnected
		// By value to keep a hold of the shared_ptr until the next iteration.
		for (const auto& [externalId, externalPlayer]: m_externalPlayers)
		{
			bool exist2 = false;
			for (auto& p2: players2)
			{
				CString tmpPlyr = p2.guntokenize();
				CString account = tmpPlyr.readString("\n");
				CString nick = tmpPlyr.readString("\n");
				if (servername == externalPlayer->getServerName() && account == externalPlayer->account.name)
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
					m_externalPlayers.erase(externalId);

					if (isRC())
						sendPacket(CString() >> (char)PLO_DELPLAYER >> externalId);
					else
						sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> externalId >> (char)PLPROP_PCONNECTED);

					//m_server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)id >> (char)PLPROP_PCONNECTED, this);
					//m_server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_DELPLAYER >> (short)id, this);
				}
			}
		}
	}

	for (std::vector<CString>::const_iterator i = players2.begin(); i != players2.end(); ++i)
	{
		CString tmpPlyr = (i)->guntokenize();
		CString account = tmpPlyr.readString("\n");
		CString nick = tmpPlyr.readString("\n");

		bool exist = false;
		if (!m_externalPlayers.empty())
		{
			for (auto& [externalId, externalPlayer]: m_externalPlayers)
			{
				if (servername == externalPlayer->getServerName() && account == externalPlayer->account.name)
				{
					externalPlayer->setNick(CString() << nick << " (on " << servername << ")");
					exist = true;
				}
			}
		}

		if (!exist)
		{
			// Get a free id to be assigned to the new player.
			auto newId = m_externalPlayerIdGenerator.getAvailableId();
			auto tmpPlyr2 = std::make_shared<Player>(nullptr, newId);
			m_externalPlayers[newId] = tmpPlyr2;
			m_server->getAccountLoader().loadAccount(account.toString(), tmpPlyr2->account);
			tmpPlyr2->account.name = account.toString();
			tmpPlyr2->setServerName(servername);
			tmpPlyr2->setExternal(true);
			tmpPlyr2->setNick(CString() << nick << " (on " << servername << ")");
			tmpPlyr2->setId(newId);
		}
	}

	if (!m_externalPlayers.empty())
	{
		for (auto& [externalId, externalPlayer]: m_externalPlayers)
		{
			if (isRC())
			{
				sendPacket(CString() >> (char)PLO_ADDPLAYER >> (short)externalId << externalPlayer->getProp(PLPROP_ACCOUNTNAME) >> (char)PLPROP_NICKNAME << externalPlayer->getProp(PLPROP_NICKNAME) >> (char)PLPROP_PLAYERLISTCATEGORY >> (char)PlayerListCategory::SERVERS);
			}
			else
			{
				sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)externalId >> (char)PLPROP_ACCOUNTNAME << externalPlayer->getProp(PLPROP_ACCOUNTNAME) >> (char)PLPROP_NICKNAME << externalPlayer->getProp(PLPROP_NICKNAME) >> (char)PLPROP_PLAYERLISTCATEGORY >> (char)PlayerListCategory::SERVERS);
			}
		}
	}

	return true;
}

bool Player::pmExternalPlayer(CString servername, CString account, CString& pmMessage)
{
	auto& list = m_server->getServerList();
	list.sendPacket(CString() >> (char)SVO_PMPLAYER >> (short)m_id << CString(CString() << servername << "\n"
																						<< this->account.name << "\n"
																						<< this->account.character.nickName << "\n"
																						<< "GraalEngine"
																						<< "\n"
																						<< "pmplayer"
																						<< "\n"
																						<< account << "\n"
																						<< pmMessage)
																		  .gtokenizeI());
	return true;
}

PlayerPtr Player::getExternalPlayer(const uint16_t id, bool includeRC) const
{
	auto iter = m_externalPlayers.find(id);
	if (iter == std::end(m_externalPlayers)) return nullptr;

	auto& externalPlayer = iter->second;
	if (!includeRC && externalPlayer->isControlClient()) return nullptr;
	return externalPlayer;
}

PlayerPtr Player::getExternalPlayer(const CString& account, bool includeRC) const
{
	for (auto& [externalId, externalPlayer]: m_externalPlayers)
	{
		if (externalPlayer == 0)
			continue;

		if (!includeRC && externalPlayer->isControlClient())
			continue;

		// Compare account names.
		if (graal::utilities::string::comparei(externalPlayer->account.name, account.toString()) == 0)
			return externalPlayer;
	}
	return nullptr;
}
