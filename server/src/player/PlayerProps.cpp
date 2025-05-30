#include <vector>
#include <ranges>
#include <algorithm>
#include <format>

#include <IEnums.h>
#include <IUtil.h>

#include "BabyDI.h"
#include "Server.h"
#include "object/NPC.h"
#include "object/Player.h"
#include "player/PlayerClient.h"
#include "level/Level.h"
#include "level/Map.h"
#include "utilities/Log.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

uint8_t PropLimits::applyMaxHitpoints(uint8_t maxHitpoints)
{
	auto server = BabyDI::Get<Server>();
	auto heartLimit = std::min(server->getSettings().getInt("heartlimit", 3), 20);
	return std::clamp(maxHitpoints, static_cast<uint8_t>(0), static_cast<uint8_t>(heartLimit));
}

int8_t PropLimits::applySwordPower(int8_t swordPower)
{
	auto server = BabyDI::Get<Server>();
	auto& settings = server->getSettings();
	int8_t minimum = (settings.getBool("healswords", false) ? -(settings.getInt("swordlimit", 3)) : 0);
	int8_t maximum = settings.getInt("swordlimit", 3);
	return std::clamp(swordPower, minimum, maximum);
}

uint8_t PropLimits::applyShieldPower(uint8_t shieldPower)
{
	auto server = BabyDI::Get<Server>();
	return std::clamp(shieldPower, static_cast<uint8_t>(0), static_cast<uint8_t>(server->getSettings().getInt("shieldlimit", 3)));
}

///////////////////////////////////////////////////////////////////////////////

prop_access Player::getPropAccess(PlayerProp prop)
{
	static uint32_t prevent_access_int = 0;
	static float prevent_access_float = 0.0f;
	static std::string prevent_access_string;

	switch (prop)
	{
		case PlayerProp::NICKNAME:
			return &account.nickname;
		case PlayerProp::MAXPOWER:
			return &account.maxHitpoints;
		case PlayerProp::CURPOWER:
			return &account.character.hitpointsInHalves;
		case PlayerProp::RUPEESCOUNT:
			return &account.character.gralats;
		case PlayerProp::ARROWSCOUNT:
			return &account.character.arrows;
		case PlayerProp::BOMBSCOUNT:
			return &account.character.bombs;
		case PlayerProp::GLOVEPOWER:
			return &account.character.glovePower;
		case PlayerProp::BOMBPOWER:
			return &account.character.bombPower;
		case PlayerProp::SWORDPOWER:
			return &account.character.swordPower;
		case PlayerProp::SHIELDPOWER:
			return &account.character.shieldPower;
		case PlayerProp::GANI:
		{
			/* TODO(Nalin): How should this hack work?
			if (isClient() && m_versionId < CLVER_2_1)
			{
				if (!account.character.bowImage.empty())
					return &account.character.bowImage;
				else
					return &account.character.bowPower;
			}
			*/
			return &account.character.gani;
		}
		case PlayerProp::HEADGIF:
			return &account.character.headImage;
		case PlayerProp::CURCHAT:
			return &account.character.chatMessage;
		case PlayerProp::COLORS:
			return &account.character.colors[0];
		case PlayerProp::ID:
			return &prevent_access_int;
		case PlayerProp::X:
			throw std::exception("NPC::getPropAccess: use X2 instead of X");
		case PlayerProp::Y:
			throw std::exception("NPC::getPropAccess: use Y2 instead of Y");
		case PlayerProp::SPRITE:
			return &account.character.sprite;
		case PlayerProp::STATUS:
			return &account.status;
		case PlayerProp::CARRYSPRITE:
			// TODO: Make this work.
			return &prevent_access_int;
		case PlayerProp::CURLEVEL:
			return &account.level;
		case PlayerProp::HORSEGIF:
			return &account.character.horseImage;
		case PlayerProp::HORSEBUSHES:
			// TODO: Make this work.
			return &prevent_access_int;
		case PlayerProp::EFFECTCOLORS:
			// TODO: Make this work.
			return &prevent_access_int;
		case PlayerProp::CARRYNPC:
			// TODO: Make this work.
			return &prevent_access_int;
		case PlayerProp::APCOUNTER:
			return &account.apCounter;
		case PlayerProp::MAGICPOINTS:
			return &account.character.mp;
		case PlayerProp::KILLSCOUNT:
			return &account.kills;
		case PlayerProp::DEATHSCOUNT:
			return &account.deaths;
		case PlayerProp::ONLINESECS:
			return &account.onlineSeconds;
		case PlayerProp::IPADDR:
			return &m_accountIp;
		case PlayerProp::UDPPORT:
			// TODO: Make this work.
			return &prevent_access_int;
		case PlayerProp::ALIGNMENT:
			return &account.character.ap;
		case PlayerProp::ADDITFLAGS:
			return &m_additionalFlags;
		case PlayerProp::ACCOUNTNAME:
			return &account.name;
		case PlayerProp::BODYIMG:
			return &account.character.bodyImage;
		case PlayerProp::RATING:
			return std::make_pair(&account.eloRating, &account.eloDeviation);
		case PlayerProp::ATTACHNPC:
			// TODO: Make this work.
			return &prevent_access_int;
		case PlayerProp::GMAPLEVELX:
			// TODO: Make this work.
			return &prevent_access_int;
		case PlayerProp::GMAPLEVELY:
			// TODO: Make this work.
			return &prevent_access_int;
		case PlayerProp::Z:
			throw std::exception("NPC::getPropAccess: use Z2 instead of Z");
		case PlayerProp::JOINLEAVELVL:
			return &prevent_access_int;
		case PlayerProp::PCONNECTED:
			return &prevent_access_int;
		case PlayerProp::PLANGUAGE:
			return &account.language;
		case PlayerProp::PSTATUSMSG:
			return &m_statusMsg;
		case PlayerProp::GATTRIB1:
		case PlayerProp::GATTRIB2:
		case PlayerProp::GATTRIB3:
		case PlayerProp::GATTRIB4:
		case PlayerProp::GATTRIB5:
		case PlayerProp::GATTRIB6:
		case PlayerProp::GATTRIB7:
		case PlayerProp::GATTRIB8:
		case PlayerProp::GATTRIB9:
		case PlayerProp::GATTRIB10:
		case PlayerProp::GATTRIB11:
		case PlayerProp::GATTRIB12:
		case PlayerProp::GATTRIB13:
		case PlayerProp::GATTRIB14:
		case PlayerProp::GATTRIB15:
		case PlayerProp::GATTRIB16:
		case PlayerProp::GATTRIB17:
		case PlayerProp::GATTRIB18:
		case PlayerProp::GATTRIB19:
		case PlayerProp::GATTRIB20:
		case PlayerProp::GATTRIB21:
		case PlayerProp::GATTRIB22:
		case PlayerProp::GATTRIB23:
		case PlayerProp::GATTRIB24:
		case PlayerProp::GATTRIB25:
		case PlayerProp::GATTRIB26:
		case PlayerProp::GATTRIB27:
		case PlayerProp::GATTRIB28:
		case PlayerProp::GATTRIB29:
		case PlayerProp::GATTRIB30:
			return &account.character.ganiAttributes[std::ranges::distance(GaniAttributePropList.begin(), std::ranges::find(GaniAttributePropList, PROPID(prop)))];
		case PlayerProp::OSTYPE:
			return &m_os;
		case PlayerProp::TEXTCODEPAGE:
			return &m_envCodePage;
		case PlayerProp::ONLINESECS2:
			return &prevent_access_int;
		case PlayerProp::X2:
			return &account.character.pixelX;
		case PlayerProp::Y2:
			return &account.character.pixelY;
		case PlayerProp::Z2:
			return &account.character.pixelZ;
		case PlayerProp::PLAYERLISTCATEGORY:
			return &prevent_access_int;
		case PlayerProp::COMMUNITYNAME:
			return &account.communityName;
	};

	return &prevent_access_int; // Should never be reached, but prevents compiler warnings.
}

CString Player::getModifiedPropsPacket() const
{
	CString result;
	for (auto i = 0; i < PLAYERPROP_COUNT; ++i)
	{
		if (m_modTime[i] != m_savedModTime[i])
			result >> (char)i << getPropPacket((PlayerProp)i);
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////

/*
	Player: Prop-Manipulation
*/
bool Player::getPropPacket(CString& buffer, PlayerProp pPropId) const
{
	switch (pPropId)
	{
		case PlayerProp::NICKNAME:
			buffer >> (char)account.character.nickName.length() << account.character.nickName;
			return true;

		case PlayerProp::MAXPOWER:
			buffer >> (char)account.maxHitpoints;
			return true;

		case PlayerProp::CURPOWER:
			buffer >> (char)(account.character.hitpointsInHalves);
			return true;

		case PlayerProp::RUPEESCOUNT:
			buffer >> (int)account.character.gralats;
			return true;

		case PlayerProp::ARROWSCOUNT:
			buffer >> (char)account.character.arrows;
			return true;

		case PlayerProp::BOMBSCOUNT:
			buffer >> (char)account.character.bombs;
			return true;

		case PlayerProp::GLOVEPOWER:
			buffer >> (char)account.character.glovePower;
			return true;

		case PlayerProp::BOMBPOWER:
			buffer >> (char)account.character.bombPower;
			return true;

		case PlayerProp::SWORDPOWER:
			buffer >> (char)(account.character.swordPower + 30) >> (char)account.character.swordImage.length() << account.character.swordImage;
			return true;

		case PlayerProp::SHIELDPOWER:
			buffer >> (char)(account.character.shieldPower + 10) >> (char)account.character.shieldImage.length() << account.character.shieldImage;
			return true;

		case PlayerProp::GANI:
		{
			if (isClient() && m_versionId < CLVER_2_1)
			{
				if (!account.character.bowImage.empty())
					buffer >> (char)(10 + account.character.bowImage.length()) << account.character.bowImage;
				else
					buffer >> (char)account.character.bowPower;
				return true;
			}

			buffer >> (char)account.character.gani.length() << account.character.gani;
			return true;
		}

		case PlayerProp::HEADGIF:
			buffer >> (char)(account.character.headImage.length() + 100) << account.character.headImage;
			return true;

		case PlayerProp::CURCHAT:
			buffer >> (char)account.character.chatMessage.length() << account.character.chatMessage;
			return true;

		case PlayerProp::COLORS:
			buffer >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4];
			return true;

		case PlayerProp::ID:
			buffer >> (short)m_id;
			return true;

		case PlayerProp::X:
		{
			auto val = static_cast<uint8_t>(account.character.pixelX / 8);
			if (val == 233) val = 232;
			buffer.writeGCharUnsafe(val);
			return true;
		}

		case PlayerProp::Y:
		{
			auto val = static_cast<uint8_t>(account.character.pixelY / 8);
			if (val == 233) val = 232;
			buffer.writeGCharUnsafe(val);
			return true;
		}

		case PlayerProp::Z:
			// range: -25 to 85
			buffer >> (char)(std::min(85 * 2, std::max(-25 * 2, (account.character.pixelZ / 8))) + 50);
			return true;

		case PlayerProp::SPRITE:
			buffer >> (char)account.character.sprite;
			return true;

		case PlayerProp::STATUS:
			buffer >> (char)account.status;
			return true;

		case PlayerProp::CARRYSPRITE:
			buffer >> (char)-1;
			return true;

		case PlayerProp::CURLEVEL:
			buffer >> (char)1 << " ";
			return true;

		case PlayerProp::HORSEGIF:
			buffer >> (char)account.character.horseImage.length() << account.character.horseImage;
			return true;

		case PlayerProp::HORSEBUSHES:
			buffer >> (char)0;
			return true;

		case PlayerProp::EFFECTCOLORS:
			buffer >> (char)0;
			return true;

		case PlayerProp::CARRYNPC:
			buffer >> (int)0;
			return true;

		case PlayerProp::APCOUNTER:
			buffer >> (short)(account.apCounter + 1);
			return true;

		case PlayerProp::MAGICPOINTS:
			buffer >> (char)account.character.mp;
			return true;

		case PlayerProp::KILLSCOUNT:
			buffer >> (int)account.kills;
			return true;

		case PlayerProp::DEATHSCOUNT:
			buffer >> (int)account.deaths;
			return true;

		case PlayerProp::ONLINESECS:
			buffer >> (int)account.onlineSeconds;
			return true;

		case PlayerProp::IPADDR:
			buffer.writeGInt5(m_accountIp);
			return true;

		case PlayerProp::UDPPORT:
			buffer >> (int)0;
			return true;

		case PlayerProp::ALIGNMENT:
			buffer >> (char)account.character.ap;
			return true;

		case PlayerProp::ADDITFLAGS:
			buffer >> (char)m_additionalFlags;
			return true;

		case PlayerProp::ACCOUNTNAME:
			buffer >> (char)account.name.length() << account.name;
			return true;

		case PlayerProp::BODYIMG:
			buffer >> (char)account.character.bodyImage.length() << account.character.bodyImage;
			return true;

		case PlayerProp::RATING:
		{
			int temp = (((int)account.eloRating & 0xFFF) << 9) | ((int)account.eloDeviation & 0x1FF);
			buffer >> (int)temp;
			return true;
		}

		case PlayerProp::ATTACHNPC:
			buffer >> (char)0 >> (int)0;
			return true;

		// Simplifies login.
		// Manually send prop if you are leaving the level.
		// 1 = join level, 0 = leave level.
		case PlayerProp::JOINLEAVELVL:
			buffer >> (char)1;
			return true;

		case PlayerProp::PCONNECTED:
			return true;

		case PlayerProp::PLANGUAGE:
			buffer >> (char)account.language.length() << account.language;
			return true;

		case PlayerProp::PSTATUSMSG:
		{
			if (m_statusMsg > m_server->getStatusList().size() - 1)
				buffer >> (char)0;
			else
				buffer >> (char)m_statusMsg;
			return true;
		}

		// OS type.
		// Windows: wind
		case PlayerProp::OSTYPE:
			buffer >> (char)m_os.length() << m_os;
			return true;

		// Text codepage.
		// Example: 1252
		case PlayerProp::TEXTCODEPAGE:
			buffer.writeGInt(m_envCodePage);
			return true;

		case PlayerProp::ONLINESECS2:
			//buffer.writeGInt5(m_onlineTime);
			return true;

		case PlayerProp::X2:
		{
			uint16_t val = (uint16_t)std::abs(account.character.pixelX) << 1;
			if (account.character.pixelX < 0)
				val |= 0x0001;
			buffer.writeGShort(val);
			return true;
		}

		case PlayerProp::Y2:
		{
			uint16_t val = (uint16_t)std::abs(account.character.pixelY) << 1;
			if (account.character.pixelY < 0)
				val |= 0x0001;
			buffer.writeGShort(val);
			return true;
		}

		case PlayerProp::Z2:
		{
			// range: -25 to 85
			uint16_t val = std::min<int16_t>(85 * 16, std::max<int16_t>(-25 * 16, account.character.pixelZ));
			val = std::abs(val) << 1;
			if (account.character.pixelZ < 0)
				val |= 0x0001;
			buffer.writeGShort(val);
			return true;
		}

		// TODO: Better level handling.  We should be able to find this from the level name.
		case PlayerProp::GMAPLEVELX:
			buffer >> (char)0;
			return true;

		case PlayerProp::GMAPLEVELY:
			buffer >> (char)0;
			return true;

		// TODO(joey): figure this out. Something to do with guilds? irc-related
		//	(char)(some bitflag for something, uses the first 3 bits im not sure)
		//		okay i tested some flags, 1 removes the channel. 3 adds it. not sure what third bit does.
		case PlayerProp::PLAYERLISTCATEGORY:
			return true;

		case PlayerProp::COMMUNITYNAME:
			buffer >> (char)account.communityName.length() << account.communityName;
			return true;

		default:
			break;
	}

	if (auto iter = std::ranges::find(GaniAttributePropList, PROPID(pPropId)); iter != GaniAttributePropList.end())
	{
		auto propIndex = std::distance(GaniAttributePropList.begin(), iter);
		buffer >> (char)account.character.ganiAttributes[propIndex].length() << account.character.ganiAttributes[propIndex];
		return true;
	}

	return false;
}

bool PlayerClient::getPropPacket(CString& buffer, PlayerProp pPropId) const
{
	auto level = m_currentLevel.lock();
	auto map = m_pmap.lock();

	switch (pPropId)
	{
		case PlayerProp::CARRYSPRITE:
			buffer >> (char)m_carrySprite;
			return true;

		case PlayerProp::CURLEVEL:
		{
			if (map && map->getType() == MapType::GMAP)
				buffer >> (char)map->getMapName().length() << map->getMapName();
			else
			{
				if (level != nullptr && level->isSingleplayer())
					buffer >> (char)(account.level.length() + 13) << account.level << ".singleplayer";
				else
					buffer >> (char)account.level.length() << account.level;
			}
			return true;
		}

		case PlayerProp::HORSEBUSHES:
			buffer >> (char)m_horseBombCount;
			return true;

		case PlayerProp::CARRYNPC:
			buffer >> (int)m_carryNpcId;
			return true;

		case PlayerProp::UDPPORT:
			buffer >> (int)m_udpport;
			return true;

		case PlayerProp::ATTACHNPC:
			// Only attach type 0 (NPC) supported.
			buffer >> (char)0 >> (int)m_attachNPC;
			return true;

		case PlayerProp::GMAPLEVELX:
			buffer >> (char)(level ? level->getGmapX() : 0);
			return true;

		case PlayerProp::GMAPLEVELY:
			buffer >> (char)(level ? level->getGmapY() : 0);
			return true;

		default:
			return Player::getPropPacket(buffer, pPropId);
	}

	return false;
}

PropSetResults Player::setPropFromPacket(PlayerProp prop, CString& packet, PropSetBy setBy)
{
	auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this());
	auto level = player ? player->getLevel() : nullptr;
	bool restrictedPropAllowed = !m_server->isNpcServerEnabled() || setBy == PropSetBy::SERVER;

	PropSetResults result{ .resultPropIds = { PROPID(prop) } };
	result.resultFlags.set(PropSetResults::sendToLevel, clientPropsSharedLocal[PROPID(prop)]);
	result.resultFlags.set(PropSetResults::sendToSelf, setBy == PropSetBy::SERVER);

	m_modTime[PROPID(prop)] = currentTimeInSeconds();

	switch (prop)
	{
		case PlayerProp::NICKNAME:
		{
			CString nick = packet.readChars(packet.readGUChar());

			// Word filter.
			int filter = m_server->getWordFilter().apply(this, nick, FILTER_CHECK_NICK);
			if (filter & FILTER_ACTION_WARN)
			{
				if (account.character.nickName.empty())
					setNick("unknown");
			}
			else
			{
				setNick(nick, setBy == PropSetBy::SERVER);
			}

			result.resultFlags.set(PropSetResults::sendToAll);
			break;
		}

		case PlayerProp::MAXPOWER:
		{
			uint8_t newMaxPower = packet.readGUChar();

			if (restrictedPropAllowed)
			{
				account.maxHitpoints = PropLimits::applyMaxHitpoints(newMaxPower);
				account.character.hitpointsInHalves = newMaxPower * 2;

				result.resultPropIds.push_back(PROPID(PlayerProp::CURPOWER));
				result.resultFlags.set(PropSetResults::sendToSelf);
			}
			break;
		}

		case PlayerProp::CURPOWER:
		{
			uint8_t power = packet.readGUChar();
			if (account.character.ap < 40 && power > account.character.hitpointsInHalves) break;
			account.character.hitpointsInHalves = PropLimits::apply(power, 0, account.maxHitpoints * 2);
			break;
		}

		case PlayerProp::RUPEESCOUNT:
		{
			unsigned int newGralatCount = std::min(packet.readGUInt(), 9999999u);
			if (restrictedPropAllowed)
				account.character.gralats = newGralatCount;
			break;
		}

		case PlayerProp::ARROWSCOUNT:
			account.character.arrows = PropLimits::apply(packet.readGUChar(), PropLimits::MaxArrows);
			break;

		case PlayerProp::BOMBSCOUNT:
			account.character.bombs = PropLimits::apply(packet.readGUChar(), PropLimits::MaxBombs);
			break;

		case PlayerProp::GLOVEPOWER:
		{
			uint8_t newGlovePower = packet.readGUChar();
			if (restrictedPropAllowed)
				account.character.glovePower = PropLimits::apply(newGlovePower, PropLimits::MaxGlovePower);
			break;
		}

		case PlayerProp::BOMBPOWER:
			account.character.bombPower = PropLimits::apply(packet.readGUChar(), PropLimits::MaxBombPower);
			break;

		case PlayerProp::SWORDPOWER:
		{
			int sp = packet.readGUChar();
			std::string img;

			if (sp <= 4)
			{
				auto& settings = m_server->getSettings();
				sp = PropLimits::applySwordPower(sp);
				img = std::format("sword{}{}", sp, (m_versionId < CLVER_2_1 ? ".gif" : ".png"));
			}
			else
			{
				sp = PropLimits::applySwordPower(sp - 30);
				int len = packet.readGUChar();
				if (len > 0)
				{
					img = packet.readChars(len).toString();
					if (!img.empty() && m_versionId < CLVER_2_1 && getExtension(img).isEmpty())
						img += ".gif";
				}
			}

			if (restrictedPropAllowed)
				account.character.swordPower = sp;

			account.character.swordImage = PropLimits::apply(img, PropLimits::SwordImageLength);
			break;
		}

		case PlayerProp::SHIELDPOWER:
		{
			int sp = packet.readGUChar();
			std::string img;

			if (sp <= 3)
			{
				auto& settings = m_server->getSettings();
				sp = PropLimits::applyShieldPower(sp);
				img = std::format("shield{}{}", sp, (m_versionId < CLVER_2_1 ? ".gif" : ".png"));
			}
			else
			{
				// This fixes an odd bug with the 1.41 client.
				if (packet.bytesLeft() == 0) break;

				sp = PropLimits::applyShieldPower(sp - 10);
				if (sp < 0) break;

				int len = packet.readGUChar();
				if (len > 0)
				{
					img = packet.readChars(len);
					if (!img.empty() && m_versionId < CLVER_2_1 && getExtension(img).isEmpty())
						img += ".gif";
				}
			}

			if (restrictedPropAllowed)
				account.character.shieldPower = sp;

			account.character.shieldImage = PropLimits::apply(img, PropLimits::ShieldImageLength);
			break;
		}

		case PlayerProp::GANI:
		{
			if (isClient() && m_versionId < CLVER_2_1)
			{
				int sp = packet.readGUChar();
				if (sp < 10)
				{
					account.character.bowPower = PropLimits::apply(sp, PropLimits::MaxBowPower);
					account.character.bowImage.clear();
				}
				else
				{
					account.character.bowPower = 10;
					sp -= 10;
					if (sp < 0) break;
					account.character.bowImage = packet.readChars(sp).toString();
					if (!account.character.bowImage.empty() && m_versionId < CLVER_2_1 && getExtension(account.character.bowImage).isEmpty())
						account.character.bowImage += ".gif";
				}
				break;
			}

			account.character.gani = PropLimits::apply(packet.readChars(packet.readGUChar()).toString(), PropLimits::GaniLength);
			if (account.character.gani == "spin" && player != nullptr)
			{
				auto curlevel = player->getLevel();
				CString nPacket;
				nPacket >> (char)PLO_HITOBJECTS >> (short)m_id >> (char)account.character.swordPower;
				char hx = (char)((getX() + 1.5f) * 2);
				char hy = (char)((getY() + 2.0f) * 2);
				m_server->sendPacketToOneLevel(CString() << nPacket >> (char)(hx) >> (char)(hy - 4), curlevel, { m_id });
				m_server->sendPacketToOneLevel(CString() << nPacket >> (char)(hx) >> (char)(hy + 4), curlevel, { m_id });
				m_server->sendPacketToOneLevel(CString() << nPacket >> (char)(hx - 4) >> (char)(hy), curlevel, { m_id });
				m_server->sendPacketToOneLevel(CString() << nPacket >> (char)(hx + 4) >> (char)(hy), curlevel, { m_id });
			}
			break;
		}

		case PlayerProp::HEADGIF:
		{
			int len = packet.readGUChar();
			std::string img;
			if (len < 100)
			{
				img = std::format("head{}{}", len, (m_versionId < CLVER_2_1 ? ".gif" : ".png"));
			}
			else if (len > 100)
			{
				img = packet.readChars(len - 100);

				// TODO(joey): We need to check properties for newline, especially if they are sending to other clients
				//	as it causes havoc on the client...
				if (auto check = img.find("\n", 0); check != std::string::npos)
					img = img.substr(0, check);

				if (!img.empty() && m_versionId < CLVER_2_1 && getExtension(img).isEmpty())
					img += ".gif";
			}

			if (len != 100)
			{
				account.character.headImage = PropLimits::apply(img, PropLimits::HeadImageLength);
				result.resultFlags.set(PropSetResults::sendToAll);
			}
			break;
		}

		case PlayerProp::CURCHAT:
		{
			int len = PropLimits::apply(packet.readGUChar(), PropLimits::ChatMessageLength);
			account.character.chatMessage = packet.readChars(len).toString();

			if (player != nullptr)
			{
				player->setLastChatTime(time(0));

				// Try to process the chat.  If it wasn't processed, apply the word filter to it.
				if (!player->processChat(account.character.chatMessage))
				{
					CString chat = account.character.chatMessage;
					int found = m_server->getWordFilter().apply(this, chat, FILTER_CHECK_CHAT);
					account.character.chatMessage = chat.toString();

					if ((found & FILTER_ACTION_REPLACE) || (found & FILTER_ACTION_WARN))
						result.resultFlags.set(PropSetResults::sendToSelf);
				}
			}
			break;
		}

		case PlayerProp::COLORS:
			for (unsigned char& color : account.character.colors)
				color = packet.readGUChar();
			break;

		case PlayerProp::ID:
			packet.readGUShort();
			break;

		case PlayerProp::X:
			account.character.pixelX = (packet.readGChar() * 8);
			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			//doTouchTest = true;

			// Let 2.30+ clients see pre-2.30 movement.
			result.resultPropIds.push_back(PROPID(PlayerProp::X2));
			break;

		case PlayerProp::Y:
			account.character.pixelY = (packet.readGChar() * 8);
			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			//doTouchTest = true;

			// Let 2.30+ clients see pre-2.30 movement.
			result.resultPropIds.push_back(PROPID(PlayerProp::Y2));
			break;

		case PlayerProp::Z:
			account.character.pixelZ = (packet.readGUChar() - 50) * 8;
			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			//doTouchTest = true;

			// Let 2.30+ clients see pre-2.30 movement.
			result.resultPropIds.push_back(PROPID(PlayerProp::Z2));
			break;

		case PlayerProp::SPRITE:
			account.character.sprite = packet.readGUChar();

			// Do collision testing.
			//doTouchTest = true;
			break;

		case PlayerProp::STATUS:
		{
			int oldStatus = account.status;
			account.status = packet.readGUChar();
			//printf("%s: status: %d, oldStatus: %d\n", m_accountName.text(), status, oldStatus );

			if (m_id == 0) break;

			// When they come back to life, give them hearts.
			if ((oldStatus & PLSTATUS_DEAD) > 0 && (account.status & PLSTATUS_DEAD) == 0)
			{
				// Give them full hearts.  If they have less than 20 AP, give them 3 hearts.  If they have less than 40 AP, give them 5 hearts.
				auto newPower = PropLimits::applyMaxHitpoints(account.character.ap < 20 ? 3 : (account.character.ap < 40 ? 5 : account.maxHitpoints)) * 2;
				account.character.hitpointsInHalves = newPower;

				result.resultPropIds.push_back(PROPID(PlayerProp::CURPOWER));
				result.resultFlags.set(PropSetResults::sendToSelf);

				// TODO(Nalin): There could be a race condition on when this packet is sent.  Do we delay until after props are sent to the client?
				if (level != nullptr && level->isPlayerLeader(m_id))
					sendPacket(CString() >> (char)PLO_ISLEADER);

				/*
				// If we are the leader of the level, call warp().  This will fix NPCs not
				// working again after we respawn.
				if (level != 0 && level->getPlayer(0) == this)
					warp(m_levelName, x, y, time(0));
				*/
			}

			// When they die, increase deaths and make somebody else level leader.
			if ((oldStatus & PLSTATUS_DEAD) == 0 && (account.status & PLSTATUS_DEAD) > 0 && level != nullptr)
			{
				if (level->isSparringZone() == false)
				{
					++account.deaths;
					player->dropItemsOnDeath();
				}

				// If we are the leader and there are more players on the level, we want to remove
				// ourself from the leader position and tell the new leader that they are the leader.
				if (level->isPlayerLeader(m_id) && level->getPlayers().size() > 1)
				{
					level->removePlayer(m_id);
					level->addPlayer(m_id);

					auto leader = m_server->getPlayer(level->getPlayers().front());
					if (leader) leader->sendPacket(CString() >> (char)PLO_ISLEADER);
				}
			}
			break;
		}

		case PlayerProp::CARRYSPRITE:
		{
			uint8_t sprite = packet.readGUChar();
			if (player == nullptr) break;
			player->m_carrySprite = sprite;
			break;
		}

		case PlayerProp::CURLEVEL:
		{
			std::string level = packet.readChars(packet.readGUChar()).toString();
			if (restrictedPropAllowed)
				account.level = level;
			break;
		}

		case PlayerProp::HORSEGIF:
		{
			int len = PropLimits::apply(packet.readGUChar(), PropLimits::HorseImageLength);
			account.character.horseImage = packet.readChars(len).toString();
			if (!account.character.horseImage.empty() && m_versionId < CLVER_2_1 && getExtension(account.character.horseImage).isEmpty())
				account.character.horseImage += ".gif";
			break;
		}

		case PlayerProp::HORSEBUSHES:
		{
			uint8_t count = packet.readGUChar();
			if (player == nullptr) break;
			player->m_horseBombCount = count;
			break;
		}

		case PlayerProp::EFFECTCOLORS:
		{
			auto len = packet.readGUChar();
			if (len > 0)
				packet.readGInt4();
			break;
		}

		case PlayerProp::CARRYNPC:
		{
			NPCID newNpcId = packet.readGUInt();

			if (player == nullptr)
				break;

			// Not supported on gmaps.
			// If we want carry npcs to work with database npcs, a lot of work would be required.
			// The throw range is 9 tiles.  We could probably send a move2 command on a throw.
			if (auto map = player->getMap().lock(); map && map->getType() == MapType::GMAP)
			{
				// I tried to throw the NPC and make it visible again, but there seems to be a race condition with the client.
				// The client wasn't throwing the NPC automatically when setting the PlayerProp::CARRYNPC to 0.
				// It would also permanently hide the NPC when thrown and I couldn't bring it back.
				// There are probably race conditions with how long it takes for the pick up and throw animations to fully play out.
				break;
			}

			// Picked up.
			if (player->getCarryNpcId() == 0 && newNpcId != 0)
			{
				// TODO: Remove when an npcserver is created.
				if (m_server->getSettings().getBool("duplicatecanbecarried", false) == false)
				{
					bool isOwner = true;
					{
						auto& playerList = m_server->getPlayerList();
						for (auto& [otherId, other] : playerList)
						{
							if (other.get() == this) continue;
							if (other->getPropPacket(PlayerProp::CARRYNPC).readGUInt() == newNpcId)
							{
								// Somebody else got this NPC first.  Force the player to throw his down
								// and tell the player to remove the NPC from memory.
								sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::CARRYNPC >> (int)0);
								sendPacket(CString() >> (char)PLO_NPCDEL2 >> (char)level->getLevelName().length() << level->getLevelName() >> (int)newNpcId);
								m_server->sendPacketToLevelArea(CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PlayerProp::CARRYNPC >> (int)0, player, { m_id });
								isOwner = false;
								newNpcId = 0;
								break;
							}
						}
					}
				}
			}
			player->setCarryNpcId(newNpcId);
			break;
		}

		case PlayerProp::APCOUNTER:
			account.apCounter = packet.readGUShort();
			break;

		case PlayerProp::MAGICPOINTS:
		{
			uint8_t newMP = packet.readGUChar();
			if (restrictedPropAllowed)
				account.character.mp = PropLimits::apply(newMP, PropLimits::MaxMP);
			break;
		}

		case PlayerProp::KILLSCOUNT:
			packet.readGInt();
			break;

		case PlayerProp::DEATHSCOUNT:
			packet.readGInt();
			break;

		case PlayerProp::ONLINESECS:
			packet.readGInt();
			break;

		case PlayerProp::IPADDR:
			packet.readGInt5();
			break;

		case PlayerProp::UDPPORT:
		{
			uint16_t udpPort = static_cast<uint16_t>(packet.readGInt());
			if (player == nullptr) break;
			player->m_udpport = udpPort;
			break;
		}

		case PlayerProp::ALIGNMENT:
		{
			uint8_t newAlignment = packet.readGUChar();
			if (restrictedPropAllowed)
				account.character.ap = std::min<uint8_t>(newAlignment, 100);
			break;
		}

		case PlayerProp::ADDITFLAGS:
			m_additionalFlags = packet.readGUChar();
			break;

		case PlayerProp::ACCOUNTNAME:
			packet.readChars(packet.readGUChar());
			break;

		case PlayerProp::BODYIMG:
			account.character.bodyImage = PropLimits::apply(packet.readChars(packet.readGUChar()).toString(), PropLimits::BodyImageLength);
			break;

		case PlayerProp::RATING:
			packet.readGInt();
			//m_eloRating = (float)((len >> 9) & 0xFFF);
			break;

		case PlayerProp::ATTACHNPC:
		{
			// Only supports object_type 0 (NPC).
			unsigned char object_type = packet.readGUChar();
			unsigned int npcID = packet.readGUInt();
			if (player == nullptr) break;
			player->m_attachNPC = npcID;
			break;
		}

		case PlayerProp::GMAPLEVELX:
		{
			int mx = packet.readGUChar();
			if (auto cmap = level->getMap(); level && cmap && cmap->isGmap())
			{
				auto& newLevelName = cmap->getLevelAt(mx, level->getMapY());
				setLevel(newLevelName, -1);
				level = player->getLevel();
			}
			break;
		}

		case PlayerProp::GMAPLEVELY:
		{
			int my = packet.readGUChar();
			if (auto cmap = level->getMap(); level && cmap && cmap->isGmap())
			{
				auto& newLevelName = cmap->getLevelAt(level->getMapX(), my);
				setLevel(newLevelName, -1);
				level = player->getLevel();
			}
			break;
		}

		case PlayerProp::JOINLEAVELVL:
			break;

		case PlayerProp::PCONNECTED:
			break;

		case PlayerProp::PLANGUAGE:
			account.language = packet.readChars(packet.readGUChar()).toString();
			break;

		case PlayerProp::PSTATUSMSG:
			m_statusMsg = packet.readGUChar();
			if (m_id == 0 || !m_loaded)
				break;

			result.resultFlags.set(PropSetResults::sendToAll);
			break;

		case PlayerProp::GATTRIB1:
		case PlayerProp::GATTRIB2:
		case PlayerProp::GATTRIB3:
		case PlayerProp::GATTRIB4:
		case PlayerProp::GATTRIB5:
		case PlayerProp::GATTRIB6:
		case PlayerProp::GATTRIB7:
		case PlayerProp::GATTRIB8:
		case PlayerProp::GATTRIB9:
		case PlayerProp::GATTRIB10:
		case PlayerProp::GATTRIB11:
		case PlayerProp::GATTRIB12:
		case PlayerProp::GATTRIB13:
		case PlayerProp::GATTRIB14:
		case PlayerProp::GATTRIB15:
		case PlayerProp::GATTRIB16:
		case PlayerProp::GATTRIB17:
		case PlayerProp::GATTRIB18:
		case PlayerProp::GATTRIB19:
		case PlayerProp::GATTRIB20:
		case PlayerProp::GATTRIB21:
		case PlayerProp::GATTRIB22:
		case PlayerProp::GATTRIB23:
		case PlayerProp::GATTRIB24:
		case PlayerProp::GATTRIB25:
		case PlayerProp::GATTRIB26:
		case PlayerProp::GATTRIB27:
		case PlayerProp::GATTRIB28:
		case PlayerProp::GATTRIB29:
		case PlayerProp::GATTRIB30:
		{
			if (auto iter = std::ranges::find(GaniAttributePropList, PROPID(prop)); iter != GaniAttributePropList.end())
			{
				auto index = std::distance(GaniAttributePropList.begin(), iter);
				account.character.ganiAttributes[index] = packet.readChars(packet.readGUChar()).toString();
			}
			break;
		}

		// OS type.
		// Windows: wind
		case PlayerProp::OSTYPE:
			m_os = packet.readChars(packet.readGUChar());
			break;

		// Text codepage.
		// Example: 1252
		case PlayerProp::TEXTCODEPAGE:
			m_envCodePage = packet.readGInt();
			break;

		// TODO(Nalin): Does this need to be read?
		case PlayerProp::ONLINESECS2:
			//m_onlineTime = pPacket.readGUInt5();
			break;

		// Location, in pixels, of the player on the level in 2.30+ clients.
		// Bit 0x0001 controls if it is negative or not.
		// Bits 0xFFFE are the actual value.
		case PlayerProp::X2:
		{
			auto len = packet.readGUShort();
			account.character.pixelX = (len >> 1);

			// If the first bit is 1, our position is negative.
			if ((uint16_t)len & 0x0001)
				account.character.pixelX = -account.character.pixelX;

			// Let pre-2.30+ clients see 2.30+ movement.
			result.resultPropIds.push_back(PROPID(PlayerProp::X));

			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			//doTouchTest = true;
			break;
		}

		case PlayerProp::Y2:
		{
			auto len = packet.readGUShort();
			account.character.pixelY = (len >> 1);

			// If the first bit is 1, our position is negative.
			if ((uint16_t)len & 0x0001)
				account.character.pixelY = -account.character.pixelY;

			// Let pre-2.30+ clients see 2.30+ movement.
			result.resultPropIds.push_back(PROPID(PlayerProp::Y));

			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			//doTouchTest = true;
			break;
		}

		case PlayerProp::Z2:
		{
			auto len = packet.readGUShort();
			account.character.pixelZ = (len >> 1);

			// If the first bit is 1, our position is negative.
			if ((uint16_t)len & 0x0001)
				account.character.pixelZ = -account.character.pixelZ;

			// Let pre-2.30+ clients see 2.30+ movement.
			result.resultPropIds.push_back(PROPID(PlayerProp::Z));

			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			//doTouchTest = true;
			break;
		}

		case PlayerProp::PLAYERLISTCATEGORY:
			(void)packet.readGUChar();
			break;

		case PlayerProp::COMMUNITYNAME:
			packet.readChars(packet.readGUChar());
			break;

		default:
		{
			log::printLine(log::server, "Player {} sent an unidentified prop: {}.", account.name, PROPID(prop));
			for (int i = 0; i < packet.length(); ++i)
				log::print(log::server, "{:02x} ", (unsigned char)packet[i]);
			log::print(log::server, "\n");

			result.resultFlags.set(PropSetResults::wasInvalid);
			break;
		}
	}

	return result;
}

void Player::setPropsFromPacket(CString& packet, PropSetBy setBy, Player* originator)
{
	CString globalBuff, levelBuff, levelBuffEnd, selfBuff, selfBuffEnd;

	while (packet.bytesLeft() > 0)
	{
		PlayerProp propId = (PlayerProp)packet.readGUChar();

		// Admin check on changing gralats.
		if (propId == PlayerProp::RUPEESCOUNT && originator != nullptr)
		{
			bool canSet = m_server->getSettings().getBool("normaladminscanchangegralats", true);
			canSet = canSet || (originator->isStaff() && originator->account.hasRight(PLPERM_SETRIGHTS));
			if (!canSet)
			{
				packet.readGUInt();
				continue;
			}
		}

		auto result = setPropFromPacket(propId, packet, setBy);

		// TODO(Nalin): Might need to add the 5 bad prop grace back in.
		if (result.resultFlags.test(PropSetResults::wasInvalid))
		{
			log::printLine(log::server, "Player {} is sending invalid packets.", account.character.nickName);
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Disconnected for sending invalid packets.");
			m_server->deletePlayer(shared_from_this());
			return;
		}

		// Add to buffers.
		bool sendToAll = result.resultFlags.test(PropSetResults::sendToAll);
		bool sendToLevel = result.resultFlags.test(PropSetResults::sendToLevel);
		bool sendToSelf = result.resultFlags.test(PropSetResults::sendToSelf);
		if (isLoggedIn() && isLoaded() && (sendToAll || sendToLevel || sendToSelf))
		{
			CString prop;
			for (auto& resultPropId: result.resultPropIds)
			{
				PlayerProp resultProp = (PlayerProp)resultPropId;
				prop = getPropPacket(resultProp);
				if (sendToAll)
					globalBuff >> (char)resultProp << prop;
				if (sendToLevel)
				{
					if (resultProp >= PlayerProp::X2)
						levelBuffEnd >> (char)resultProp << prop;
					else levelBuff >> (char)resultProp << prop;
				}
				if (sendToSelf)
				{
					if (resultProp >= PlayerProp::X2)
						selfBuffEnd >> (char)resultProp << prop;
					else selfBuff >> (char)resultProp << prop;
				}
			}
		}
	}

	// Send Buffers Out
	if (isLoggedIn() && isLoaded())
	{
		if (globalBuff.length() > 0)
			m_server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->m_id << globalBuff, { m_id });

		auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this());
		if (player != nullptr && (levelBuff.length() > 0 || levelBuffEnd.length() > 0))
		{
			// TODO(Nalin): This needs to be fixed.  We need to add a way to extract the movement packets and send the appropriate ones to the appropriate client version.
			// Currently, we just send the 2.3+ props at the end.  This is so the 2.2 and earlier clients don't stop processing props once it hits a prop it doesn't know about.
			m_server->sendPacketToLevelArea(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->m_id << levelBuff << levelBuffEnd, player, { m_id });
		}

		if (selfBuff.length() > 0 || selfBuffEnd.length() > 0)
			this->sendPacket(CString() >> (char)PLO_PLAYERPROPS << selfBuff << selfBuffEnd);
	}
}

void Player::sendPropsToClient(const PropList& props)
{
	// Definition
	CString propPacket;

	int propCount = (isClient() && m_versionId < CLVER_2_1 ? 37 : props.size());
	for (int i = 0; i < propCount; ++i)
	{
		if (props[i])
			propPacket >> (char)i << getPropPacket((PlayerProp)i);
	}

	// Send Packet
	sendPacket(CString() >> (char)PLO_PLAYERPROPS << propPacket);
}

CString Player::getPropsPacketFromList(const PropList& props) const
{
	CString propPacket;

	// Check if PlayerProp::JOINLEAVELVL is set.
	// When the player leaves the level we explicitly send a 0.
	// This prop is only ever in the prop list when we are exchanging props after entering a level, so always just send 1.
	if (isClient() && props[PROPID(PlayerProp::JOINLEAVELVL)])
		propPacket >> (char)PlayerProp::JOINLEAVELVL >> (char)1;

	// Create Props
	int propCount = (isClient() && m_versionId < CLVER_2_1 ? 37 : props.size());
	for (int i = 0; i < propCount; ++i)
	{
		if (i == PROPID(PlayerProp::JOINLEAVELVL)) continue;

		/*
		* TODO(Nalin): Check if this is needed.
		if (i == PlayerProp::ATTACHNPC)
		{
			if (auto client = std::dynamic_pointer_cast<const PlayerClient>(shared_from_this()); client != nullptr && client->m_attachNPC != 0)
			{
				propPacket >> (char)i;
				getProp(propPacket, i);
			}
		}
		*/

		if (props[i])
		{
			propPacket >> (char)i;
			getPropPacket(propPacket, (PlayerProp)i);
		}
	}

	if (m_isExternal)
		propPacket >> (char)PlayerProp::PLAYERLISTCATEGORY >> (char)PlayerListCategory::SERVERS;

	return propPacket;
}

///////////////////////////////////////////////////////////////////////////////

void Player::setPropsFromRCPacket(CString& pPacket, Player* rc)
{
	bool hadBomb = false, hadBow = false;
	CString outPacket;

	// Skip playerworld
	pPacket.readChars(pPacket.readGUChar());

	// Read props from the packet.
	CString props = pPacket.readChars(pPacket.readGUChar());

	// Send props out.
	setPropsFromPacket(props, PropSetBy::SERVER, rc);

	// Clear flags
	for (const auto& [flag, value] : account.flags.container)
	{
		outPacket >> (char)PLO_FLAGDEL << flag;
		if (!value.empty()) outPacket << "=" << value;
		outPacket << "\n";
	}
	account.flags.clear();

	// Clear Weapons
	for (const auto& weapon : account.weapons)
	{
		outPacket >> (char)PLO_NPCWEAPONDEL << weapon << "\n";

		// Attempt to fix the funky client bomb capitalization issue.
		// Also fix the bomb coming back when you set the player props through RC.
		if (weapon == "bomb")
		{
			outPacket >> (char)PLO_NPCWEAPONDEL << "Bomb\n";
			hadBomb = true;
		}
		if (weapon == "Bomb")
			hadBomb = true;

		// Do the same thing with the bow.
		if (weapon == "bow")
		{
			outPacket >> (char)PLO_NPCWEAPONDEL << "Bow\n";
			hadBow = true;
		}
		if (weapon == "Bow")
			hadBow = true;
	}
	account.weapons.clear();

	// Send the packet to clear the flags and weapons from the client.
	if (isLoaded())
		sendPacket(outPacket);

	// Re-populate the flag list.
	auto flagCount = pPacket.readGUShort();
	while (flagCount > 0)
	{
		CString flag = pPacket.readChars(pPacket.readGUChar());
		std::string name = flag.readString("=").text();
		CString val = flag.readString("");

		setFlag(name, val, isLoaded());
		--flagCount;
	}

	// Clear the chests and re-populate the chest list.
	account.savedChests.clear();
	auto chestCount = pPacket.readGUShort();
	while (chestCount > 0)
	{
		unsigned char len = pPacket.readGUChar();
		char loc[2] = { pPacket.readGChar(), pPacket.readGChar() };
		std::string level = pPacket.readChars(len - 2).toString();

		account.savedChests.insert(std::make_pair(level, std::make_pair(loc[0], loc[1])));
		--chestCount;
	}

	// Re-populate the weapons list.
	auto weaponCount = pPacket.readGUChar();
	while (weaponCount > 0)
	{
		unsigned char len = pPacket.readGUChar();
		if (len == 0) continue;
		CString wpn = pPacket.readChars(len);

		// Allow the bomb through if we are actually adding it.
		if (wpn == "bomb" || wpn == "Bomb")
			hadBomb = true;

		// Allow the bow through if we are actually adding it.
		if (wpn == "bow" || wpn == "Bow")
			hadBow = true;

		// Send the weapon to the player.
		this->addWeapon(wpn.toString());
		--weaponCount;
	}

	// KILL THE BOMB DEAD
	if (isLoaded())
	{
		if (!hadBomb)
			sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << "Bomb");
	}

	// Warp the player to his new location now.
	if (isLoaded() && isClient())
	{
		if (auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); player != nullptr)
			player->warp(account.level, getX(), getY(), 0);
	}
}

CString Player::getPropsForRCPacket()
{
	CString ret, props;
	ret >> (char)account.name.length() << account.name;
	ret >> (char)4 << "main"; // worldName

	// Add the props.
	for (int i = 0; i < PLAYERPROP_COUNT; ++i)
	{
		if (clientPropsForRCView[i])
			props >> (char)i << getPropPacket((PlayerProp)i);
	}
	ret >> (char)props.length() << props;

	// Add the player's flags.
	ret >> (short)account.flags.container.size();
	for (const auto& [flag, value] : account.flags.container)
	{
		std::string computedFlag{ flag };
		if (!value.empty())
			computedFlag += std::format("={}", value);

		// Truncate the flag if it is too long.
		if (computedFlag.length() > 223)
			computedFlag.erase(223);

		ret >> (char)computedFlag.length() << computedFlag;
	}

	// Add the player's chests.
	ret >> (short)account.savedChests.size();
	for (const auto& [level, loc] : account.savedChests)
	{
		ret >> (char)(level.length() + 2) >> (char)loc.first >> (char)loc.second << level;
	}

	// Add the player's weapons.
	ret >> (char)account.weapons.size();
	for (const auto& weapon : account.weapons)
		ret >> (char)weapon.length() << weapon;

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
