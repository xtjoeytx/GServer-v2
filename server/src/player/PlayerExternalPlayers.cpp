#include <iterator>
#include <memory>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Server.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerProps.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

std::vector<CString> Player::getPMServerList()
{
	return m_privateMessageServerList;
}

bool Player::addPMServer(const CString& option)
{
	auto& list = m_server->getServerList();

	bool PMSrvExist = false;
	for (auto& pmServer : m_privateMessageServerList)
	{
		if (pmServer == option)
		{
			PMSrvExist = true;
			break;
		}
	}

	if (!PMSrvExist)
	{
		m_privateMessageServerList.push_back(option);
		list.sendPacket(CString() >> (char)SVO_REQUESTLIST >> (short)m_id << string::toCSVFromPack("GraalEngine"sv, "pmserverplayers"sv, option.toStringView()));
		return true;
	}
	else
		return false;
}

bool Player::remPMServer(const CString& option)
{
	if (m_privateMessageServerList.empty())
		return true;

	if (!m_externalPlayers.empty())
	{
		// Check if a player has disconnected
		// By value to keep a hold of the shared_ptr until the next iteration.
		for (const auto& [externalId, externalPlayer] : m_externalPlayers)
		{
			if (option == externalPlayer->getServerName())
			{
				// Map iterators are valid after erase.
				m_externalPlayers.erase(externalId);

				if (isRC())
					sendPacket(CString() >> (char)PLO_DELPLAYER >> externalId);
				else
					sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> externalId >> (char)PlayerProp::DISCONNECT);
			}
		}
	}

	// Find the player and remove him.
	std::erase(m_privateMessageServerList, option);

	return true;
}

bool Player::updatePMPlayers(const CString& servername, const CString& players)
{
	const auto players2 = players.tokenize("\n");

	if (!m_externalPlayers.empty())
	{
		// Check if a player has disconnected
		// By value to keep a hold of the shared_ptr until the next iteration.
		for (const auto& [externalId, externalPlayer] : m_externalPlayers)
		{
			bool exist2 = false;
			for (auto& p2 : players2)
			{
				CString tmpPlyr = p2.guntokenize();
				CString accountName = tmpPlyr.readString("\n");
				CString nick = tmpPlyr.readString("\n");
				if (servername == externalPlayer->getServerName() && accountName == externalPlayer->account.name)
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
						sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> externalId >> (char)PlayerProp::DISCONNECT);

					//m_server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)id >> (char)PlayerProp::DISCONNECT, this);
					//m_server->sendPacketTo(PLTYPE_ANYRC, CString() >> (char)PLO_DELPLAYER >> (short)id, this);
				}
			}
		}
	}

	for (const auto& i : players2)
	{
		CString tmpPlyr = i.guntokenize();
		const CString accountName = tmpPlyr.readString("\n");
		const CString nick = tmpPlyr.readString("\n");

		bool exist = false;
		if (!m_externalPlayers.empty())
		{
			for (const auto& externalPlayer : m_externalPlayers | std::views::values)
			{
				if (servername == externalPlayer->getServerName() && accountName == externalPlayer->account.name)
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
			const auto tmpPlyr2 = std::make_shared<Player>(nullptr, newId);
			m_externalPlayers[newId] = tmpPlyr2;
			m_server->getAccountLoader().loadAccount(accountName.toString(), tmpPlyr2->account);
			tmpPlyr2->account.name = accountName.toString();
			tmpPlyr2->setServerName(servername);
			tmpPlyr2->setExternal(true);
			tmpPlyr2->setNick(CString() << nick << " (on " << servername << ")");
			tmpPlyr2->setId(newId);
		}
	}

	if (!m_externalPlayers.empty())
	{
		for (auto& [externalId, externalPlayer] : m_externalPlayers)
		{
			if (isRC())
			{
				sendPacket(CString() >> (char)PLO_ADDPLAYER >> (short)externalId << externalPlayer->getProp<PlayerProp::ACCOUNTNAME>().serialize()
					>> (char)PlayerProp::NICKNAME << externalPlayer->getProp<PlayerProp::NICKNAME>().serialize()
					>> (char)PlayerProp::PLAYERLISTCATEGORY >> (char)PlayerListCategory::EXTERNAL);
			}
			else
			{
				sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)externalId
					>> (char)PlayerProp::ACCOUNTNAME << externalPlayer->getProp<PlayerProp::ACCOUNTNAME>().serialize()
					>> (char)PlayerProp::NICKNAME << externalPlayer->getProp<PlayerProp::NICKNAME>().serialize()
					>> (char)PlayerProp::PLAYERLISTCATEGORY >> (char)PlayerListCategory::EXTERNAL);
			}
		}
	}

	return true;
}

bool Player::pmExternalPlayer(const CString& servername, const CString& externalAccount, const CString& pmMessage) const
{
	const auto output = string::toCSVFromPack(
		servername.toStringView(),
		std::string_view{account.name},
		std::string_view{account.character.nickName},
		"GraalEngine"sv,
		"pmplayer"sv,
		externalAccount.toStringView(),
		pmMessage.toStringView()
	);

	auto& list = m_server->getServerList();
	list.sendPacket(CString() >> (char)SVO_PMPLAYER >> (short)m_id << output);
	return true;
}

PlayerPtr Player::getExternalPlayer(const PlayerID id, const bool includeRC) const
{
	const auto iter = m_externalPlayers.find(id);
	if (iter == std::end(m_externalPlayers)) return nullptr;

	auto& externalPlayer = iter->second;
	if (!includeRC && externalPlayer->isControlClient()) return nullptr;
	return externalPlayer;
}

PlayerPtr Player::getExternalPlayer(const CString& externalAccountName, const bool includeRC) const
{
	for (const auto& externalPlayer : m_externalPlayers | std::views::values)
	{
		if (externalPlayer == nullptr)
			continue;
		if (!includeRC && externalPlayer->isControlClient())
			continue;

		// Compare account names.
		if (string::equalsi(externalPlayer->account.name, externalAccountName.toString()))
			return externalPlayer;
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
