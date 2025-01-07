#include <vector>
#include <ranges>
#include <algorithm>
#include <format>

#include <IDebug.h>
#include <IEnums.h>
#include <IUtil.h>

#include "Server.h"
#include "object/Player.h"
#include "object/PlayerClient.h"
#include "level/Level.h"
#include "level/Map.h"

#define serverlog m_server->getServerLog()
#define rclog m_server->getRCLog()


uint8_t PropLimits::applyMaxHitpoints(uint8_t maxHitpoints)
{
	auto heartLimit = std::min(m_server->getSettings().getInt("heartlimit", 3), 20);
	return std::clamp(maxHitpoints, static_cast<uint8_t>(0), static_cast<uint8_t>(heartLimit));
}

int8_t PropLimits::applySwordPower(int8_t swordPower)
{
	auto& settings = m_server->getSettings();
	int8_t minimum = (settings.getBool("healswords", false) ? -(settings.getInt("swordlimit", 3)) : 0);
	int8_t maximum = settings.getInt("swordlimit", 3);
	return std::clamp(swordPower, minimum, maximum);
}

uint8_t PropLimits::applyShieldPower(uint8_t shieldPower)
{
	return std::clamp(shieldPower, static_cast<uint8_t>(0), static_cast<uint8_t>(m_server->getSettings().getInt("shieldlimit", 3)));
}

/*
	Player: Prop-Manipulation
*/
bool Player::getProp(CString& buffer, int pPropId) const
{
	switch (pPropId)
	{
		case PLPROP_NICKNAME:
			buffer >> (char)account.character.nickName.length() << account.character.nickName;
			return true;

		case PLPROP_MAXPOWER:
			buffer >> (char)account.maxHitpoints;
			return true;

		case PLPROP_CURPOWER:
			buffer >> (char)(account.character.hitpointsInHalves);
			return true;

		case PLPROP_RUPEESCOUNT:
			buffer >> (int)account.character.gralats;
			return true;

		case PLPROP_ARROWSCOUNT:
			buffer >> (char)account.character.arrows;
			return true;

		case PLPROP_BOMBSCOUNT:
			buffer >> (char)account.character.bombs;
			return true;

		case PLPROP_GLOVEPOWER:
			buffer >> (char)account.character.glovePower;
			return true;

		case PLPROP_BOMBPOWER:
			buffer >> (char)account.character.bombPower;
			return true;

		case PLPROP_SWORDPOWER:
			buffer >> (char)(account.character.swordPower + 30) >> (char)account.character.swordImage.length() << account.character.swordImage;
			return true;

		case PLPROP_SHIELDPOWER:
			buffer >> (char)(account.character.shieldPower + 10) >> (char)account.character.shieldImage.length() << account.character.shieldImage;
			return true;

		case PLPROP_GANI:
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

		case PLPROP_HEADGIF:
			buffer >> (char)(account.character.headImage.length() + 100) << account.character.headImage;
			return true;

		case PLPROP_CURCHAT:
			buffer >> (char)account.character.chatMessage.length() << account.character.chatMessage;
			return true;

		case PLPROP_COLORS:
			buffer >> (char)account.character.colors[0] >> (char)account.character.colors[1] >> (char)account.character.colors[2] >> (char)account.character.colors[3] >> (char)account.character.colors[4];
			return true;

		case PLPROP_ID:
			buffer >> (short)m_id;
			return true;

		case PLPROP_X:
			buffer >> (char)(account.character.pixelX / 8);
			return true;

		case PLPROP_Y:
			buffer >> (char)(account.character.pixelY / 8);
			return true;

		case PLPROP_Z:
			// range: -25 to 85
			buffer >> (char)(std::min(85 * 2, std::max(-25 * 2, (account.character.pixelZ / 8))) + 50);
			return true;

		case PLPROP_SPRITE:
			buffer >> (char)account.character.sprite;
			return true;

		case PLPROP_STATUS:
			buffer >> (char)account.status;
			return true;

		case PLPROP_CARRYSPRITE:
			buffer >> (char)-1;
			return true;

		case PLPROP_CURLEVEL:
			buffer >> (char)1 << " ";
			return true;

		case PLPROP_HORSEGIF:
			buffer >> (char)account.character.horseImage.length() << account.character.horseImage;
			return true;

		case PLPROP_HORSEBUSHES:
			buffer >> (char)0;
			return true;

		case PLPROP_EFFECTCOLORS:
			buffer >> (char)0;
			return true;

		case PLPROP_CARRYNPC:
			buffer >> (int)0;
			return true;

		case PLPROP_APCOUNTER:
			buffer >> (short)(account.apCounter + 1);
			return true;

		case PLPROP_MAGICPOINTS:
			buffer >> (char)account.character.mp;
			return true;

		case PLPROP_KILLSCOUNT:
			buffer >> (int)account.kills;
			return true;

		case PLPROP_DEATHSCOUNT:
			buffer >> (int)account.deaths;
			return true;

		case PLPROP_ONLINESECS:
			buffer >> (int)account.onlineSeconds;
			return true;

		case PLPROP_IPADDR:
			buffer.writeGInt5(m_accountIp);
			return true;

		case PLPROP_UDPPORT:
			buffer >> (int)0;
			return true;

		case PLPROP_ALIGNMENT:
			buffer >> (char)account.character.ap;
			return true;

		case PLPROP_ADDITFLAGS:
			buffer >> (char)0; // m_additionalFlags;
			return true;

		case PLPROP_ACCOUNTNAME:
			buffer >> (char)account.name.length() << account.name;
			return true;

		case PLPROP_BODYIMG:
			buffer >> (char)account.character.bodyImage.length() << account.character.bodyImage;
			return true;

		case PLPROP_RATING:
		{
			int temp = (((int)account.eloRating & 0xFFF) << 9) | ((int)account.eloDeviation & 0x1FF);
			buffer >> (int)temp;
			return true;
		}

		case PLPROP_ATTACHNPC:
			buffer >> (char)0 >> (int)0;
			return true;

		// Simplifies login.
		// Manually send prop if you are leaving the level.
		// 1 = join level, 0 = leave level.
		case PLPROP_JOINLEAVELVL:
			buffer >> (char)1;
			return true;

		case PLPROP_PCONNECTED:
			//return true CString();
			return true;

		case PLPROP_PLANGUAGE:
			buffer >> (char)account.language.length() << account.language;
			return true;

		case PLPROP_PSTATUSMSG:
		{
			//if (id == -1)
			//	break;

			if (m_statusMsg > m_server->getStatusList().size() - 1)
				buffer >> (char)0;
			else
				buffer >> (char)m_statusMsg;
			return true;
		}

		// OS type.
		// Windows: wind
		case PLPROP_OSTYPE:
			buffer >> (char)m_os.length() << m_os;
			return true;

			// Text codepage.
			// Example: 1252
		case PLPROP_TEXTCODEPAGE:
			buffer.writeGInt(m_envCodePage);
			return true;

		case PLPROP_ONLINESECS2:
			//buffer.writeGInt5(m_onlineTime);
			return true;

		case PLPROP_X2:
		{
			uint16_t val = (uint16_t)std::abs(account.character.pixelX) << 1;
			if (account.character.pixelX < 0)
				val |= 0x0001;
			buffer.writeGShort(val);
			return true;
		}

		case PLPROP_Y2:
		{
			uint16_t val = (uint16_t)std::abs(account.character.pixelY) << 1;
			if (account.character.pixelY < 0)
				val |= 0x0001;
			buffer.writeGShort(val);
			return true;
		}

		case PLPROP_Z2:
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
		case PLPROP_GMAPLEVELX:
			buffer >> (char)0;
			return true;

		case PLPROP_GMAPLEVELY:
			buffer >> (char)0;
			return true;

			// TODO(joey): figure this out. Something to do with guilds? irc-related
			//	(char)(some bitflag for something, uses the first 3 bits im not sure)
			//		okay i tested some flags, 1 removes the channel. 3 adds it. not sure what third bit does.
		case PLPROP_PLAYERLISTCATEGORY:
			//return CString();
			return true;

		case PLPROP_COMMUNITYNAME:
			buffer >> (char)account.communityName.length() << account.communityName;
			return true;

		default:
			break;
	}

	if (auto iter = std::ranges::find(GaniAttributePropList, pPropId); iter != GaniAttributePropList.end())
	{
		auto propIndex = std::distance(GaniAttributePropList.begin(), iter);
		buffer >> (char)account.character.ganiAttributes[propIndex].length() << account.character.ganiAttributes[propIndex];
		return true;
	}

	return false;
}

bool PlayerClient::getProp(CString& buffer, int pPropId) const
{
	auto level = m_currentLevel.lock();
	auto map = m_pmap.lock();

	switch (pPropId)
	{
		case PLPROP_CARRYSPRITE:
			buffer >> (char)m_carrySprite;
			return true;

		case PLPROP_CURLEVEL:
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

		case PLPROP_HORSEBUSHES:
			buffer >> (char)m_horseBombCount;
			return true;

		case PLPROP_CARRYNPC:
			buffer >> (int)m_carryNpcId;
			return true;

		case PLPROP_UDPPORT:
			buffer >> (int)m_udpport;
			return true;

		case PLPROP_ATTACHNPC:
			// Only attach type 0 (NPC) supported.
			buffer >> (char)0 >> (int)m_attachNPC;
			return true;

		case PLPROP_GMAPLEVELX:
			buffer >> (char)(level ? level->getGmapX() : 0);
			return true;

		case PLPROP_GMAPLEVELY:
			buffer >> (char)(level ? level->getGmapY() : 0);
			return true;

		default:
			return Player::getProp(buffer, pPropId);
	}

	return false;
}

void Player::setProps(CString& pPacket, uint8_t options, Player* rc)
{
	auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this());
	auto level = player ? player->getLevel() : nullptr;

	CString globalBuff, levelBuff, levelBuff2, selfBuff;
	bool doTouchTest = false;
	bool sentInvalid = false;
	int len = 0;

	while (pPacket.bytesLeft() > 0)
	{
		unsigned char propId = pPacket.readGUChar();

		switch (propId)
		{
		case PLPROP_NICKNAME:
		{
			CString nick = pPacket.readChars(pPacket.readGUChar());

			// Word filter.
			int filter = m_server->getWordFilter().apply(this, nick, FILTER_CHECK_NICK);
			if (filter & FILTER_ACTION_WARN)
			{
				if (account.character.nickName.empty())
					setNick("unknown");
			}
			else
				setNick(nick, !(options & PLSETPROPS_SETBYPLAYER));

			if (options & PLSETPROPS_FORWARD)
				globalBuff >> (char)propId << getProp(propId);

			// Send this if the player is located on another server
			// globalBuff >> (char)81;

			if (!(options & PLSETPROPS_FORWARDSELF))
				selfBuff >> (char)propId << getProp(propId);
		}
		break;

		case PLPROP_MAXPOWER:
		{
			uint8_t newMaxPower = pPacket.readGUChar();

#ifdef V8NPCSERVER
			if (!(options & PLSETPROPS_SETBYPLAYER))
			{
#endif
				account.maxHitpoints = PropLimits::applyMaxHitpoints(newMaxPower);
				account.character.hitpointsInHalves = newMaxPower * 2;

#ifdef V8NPCSERVER
				levelBuff >> (char)PLPROP_MAXPOWER << getProp(PLPROP_MAXPOWER);
				selfBuff >> (char)PLPROP_MAXPOWER << getProp(PLPROP_MAXPOWER);
#endif
				levelBuff >> (char)PLPROP_CURPOWER << getProp(PLPROP_CURPOWER);
				selfBuff >> (char)PLPROP_CURPOWER << getProp(PLPROP_CURPOWER);
#ifdef V8NPCSERVER
			}
#endif

			break;
		}

		case PLPROP_CURPOWER:
		{
			uint8_t power = pPacket.readGUChar();
			if (account.character.ap < 40 && power > account.character.hitpointsInHalves) break;
			account.character.hitpointsInHalves = PropLimits::apply(power, 0, account.maxHitpoints * 2);
			break;
		}

		case PLPROP_RUPEESCOUNT:
		{
			unsigned int newGralatCount = std::min(pPacket.readGUInt(), 9999999u);

#ifdef V8NPCSERVER
			if (!(options & PLSETPROPS_SETBYPLAYER))
			{
#endif
				if (rc != nullptr)
				{
					if (m_server->getSettings().getBool("normaladminscanchangegralats", true) || (rc->isStaff() && rc->account.hasRight(PLPERM_SETRIGHTS)))
						account.character.gralats = newGralatCount;
				}
				else
				{
					account.character.gralats = newGralatCount;
				}
#ifdef V8NPCSERVER
			}
#endif
			break;
		}

		case PLPROP_ARROWSCOUNT:
			account.character.arrows = PropLimits::apply(pPacket.readGUChar(), PropLimits::MaxArrows);
			break;

		case PLPROP_BOMBSCOUNT:
			account.character.bombs = PropLimits::apply(pPacket.readGUChar(), PropLimits::MaxBombs);
			break;

		case PLPROP_GLOVEPOWER:
		{
			uint8_t newGlovePower = pPacket.readGUChar();
#ifdef V8NPCSERVER
			if (!(options & PLSETPROPS_SETBYPLAYER))
			{
#endif
				account.character.glovePower = PropLimits::apply(newGlovePower, PropLimits::MaxGlovePower);
#ifdef V8NPCSERVER
			}
#endif
			break;
		}

		case PLPROP_BOMBPOWER:
			account.character.bombPower = PropLimits::apply(pPacket.readGUChar(), PropLimits::MaxBombPower);
			break;

		case PLPROP_SWORDPOWER:
		{
			int sp = pPacket.readGUChar();
			CString img;

			if (sp <= 4)
			{
				auto& settings = m_server->getSettings();
				sp = PropLimits::applySwordPower(sp);
				img = CString() << "sword" << CString(sp) << (m_versionId < CLVER_2_1 ? ".gif" : ".png");
			}
			else
			{
				sp = PropLimits::applySwordPower(sp - 30);
				len = pPacket.readGUChar();
				if (len > 0)
				{
					img = pPacket.readChars(len);
					if (!img.isEmpty() && m_versionId < CLVER_2_1 && getExtension(img).isEmpty())
						img << ".gif";
				}
				else
					img = "";
			}

#ifdef V8NPCSERVER
			if (!(options & PLSETPROPS_SETBYPLAYER))
			{
#endif
				account.character.swordPower = sp;
#ifdef V8NPCSERVER
			}
#endif
			account.character.swordImage = PropLimits::apply(img.toString(), PropLimits::SwordImageLength);
		}
		break;

		case PLPROP_SHIELDPOWER:
		{
			int sp = pPacket.readGUChar();
			CString img;

			if (sp <= 3)
			{
				auto& settings = m_server->getSettings();
				sp = PropLimits::applyShieldPower(sp);
				img = CString() << "shield" << CString(sp) << (m_versionId < CLVER_2_1 ? ".gif" : ".png");
			}
			else
			{
				// This fixes an odd bug with the 1.41 client.
				if (pPacket.bytesLeft() == 0) continue;

				sp = PropLimits::applyShieldPower(sp - 10);
				if (sp < 0) break;

				len = pPacket.readGUChar();
				if (len > 0)
				{
					img = pPacket.readChars(len);
					if (!img.isEmpty() && m_versionId < CLVER_2_1 && getExtension(img).isEmpty())
						img << ".gif";
				}
				else
					img = "";
			}

#ifdef V8NPCSERVER
			if (!(options & PLSETPROPS_SETBYPLAYER))
			{
#endif
				account.character.shieldPower = sp;
#ifdef V8NPCSERVER
			}
#endif
			account.character.shieldImage = PropLimits::apply(img.toString(), PropLimits::ShieldImageLength);
		}
		break;

		case PLPROP_GANI:
		{
			if (isClient() && m_versionId < CLVER_2_1)
			{
				int sp = pPacket.readGUChar();
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
					account.character.bowImage = pPacket.readChars(sp).toString();
					if (!account.character.bowImage.empty() && m_versionId < CLVER_2_1 && getExtension(account.character.bowImage).isEmpty())
						account.character.bowImage += ".gif";
				}
				break;
			}

			account.character.gani = PropLimits::apply(pPacket.readChars(pPacket.readGUChar()).toString(), PropLimits::GaniLength);
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
		}
		break;

		case PLPROP_HEADGIF:
		{
			len = pPacket.readGUChar();
			CString img;
			if (len < 100)
			{
				img = CString() << "head" << CString(len) << (m_versionId < CLVER_2_1 ? ".gif" : ".png");
			}
			else if (len > 100)
			{
				img = pPacket.readChars(len - 100);

				// TODO(joey): We need to check properties for newline, especially if they are sending to other clients
				//	as it causes havoc on the client...
				int check = img.find("\n", 0);
				if (check > 0)
					img = img.readChars(check);

				if (!img.isEmpty() && m_versionId < CLVER_2_1 && getExtension(img).isEmpty())
					img << ".gif";
			}

			if (len != 100)
			{
				account.character.headImage = PropLimits::apply(img.toString(), PropLimits::HeadImageLength);
				globalBuff >> (char)propId << getProp(propId);
			}

			break;
		}

		case PLPROP_CURCHAT:
		{
			len = PropLimits::apply(pPacket.readGUChar(), PropLimits::ChatMessageLength);
			account.character.chatMessage = pPacket.readChars(len).toString();

			if (player != nullptr)
			{
				player->setLastChatTime(time(0));

				// Try to process the chat.  If it wasn't processed, apply the word filter to it.
				if (!player->processChat(account.character.chatMessage))
				{
					CString chat = account.character.chatMessage;
					int found = m_server->getWordFilter().apply(this, chat, FILTER_CHECK_CHAT);
					account.character.chatMessage = chat.toString();

					if (!(options & PLSETPROPS_FORWARDSELF))
					{
						if ((found & FILTER_ACTION_REPLACE) || (found & FILTER_ACTION_WARN))
							selfBuff >> (char)propId << getProp(propId);
					}
				}

#ifdef V8NPCSERVER
				// Send chat to npcs if this wasn't changed by the npcserver
				if (!rc && !account.character.chatMessage.isEmpty())
				{
					if (level != nullptr)
						level->sendChatToLevel(this, account.character.chatMessage.text());
				}
#endif
			}
		}
		break;

		case PLPROP_COLORS:
			for (unsigned char& color : account.character.colors)
				color = pPacket.readGUChar();
			break;

		case PLPROP_ID:
			pPacket.readGUShort();
			break;

		case PLPROP_X:
			account.character.pixelX = (pPacket.readGUChar() * 8);
			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			doTouchTest = true;

			// Let 2.30+ clients see pre-2.30 movement.
			levelBuff2 >> (char)PLPROP_X2 << getProp(PLPROP_X2);
			break;

		case PLPROP_Y:
			account.character.pixelY = (pPacket.readGUChar() * 8);
			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			doTouchTest = true;

			// Let 2.30+ clients see pre-2.30 movement.
			levelBuff2 >> (char)PLPROP_Y2 << getProp(PLPROP_Y2);
			break;

		case PLPROP_Z:
			account.character.pixelZ = (pPacket.readGUChar() - 50) * 8;
			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			doTouchTest = true;

			// Let 2.30+ clients see pre-2.30 movement.
			levelBuff2 >> (char)PLPROP_Z2 << getProp(PLPROP_Z2);
			break;

		case PLPROP_SPRITE:
			account.character.sprite = pPacket.readGUChar();

#ifndef V8NPCSERVER
			// Do collision testing.
			doTouchTest = true;
#endif
			break;

		case PLPROP_STATUS:
		{
			int oldStatus = account.status;
			account.status = pPacket.readGUChar();
			//printf("%s: status: %d, oldStatus: %d\n", m_accountName.text(), status, oldStatus );

			if (m_id == -1) break;

			// When they come back to life, give them hearts.
			if ((oldStatus & PLSTATUS_DEAD) > 0 && (account.status & PLSTATUS_DEAD) == 0)
			{
				// Give them full hearts.  If they have less than 20 AP, give them 3 hearts.  If they have less than 40 AP, give them 5 hearts.
				auto newPower = PropLimits::applyMaxHitpoints(account.character.ap < 20 ? 3 : (account.character.ap < 40 ? 5 : account.maxHitpoints)) * 2;
				account.character.hitpointsInHalves = newPower;

				selfBuff >> (char)PLPROP_CURPOWER >> (char)(account.character.hitpointsInHalves);
				levelBuff >> (char)PLPROP_CURPOWER >> (char)(account.character.hitpointsInHalves);

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
		}
		break;

		case PLPROP_CARRYSPRITE:
		{
			uint8_t sprite = pPacket.readGUChar();
			if (player == nullptr) break;
			player->m_carrySprite = sprite;
			break;
		}

		case PLPROP_CURLEVEL:
			len = pPacket.readGUChar();
#ifdef V8NPCSERVER
			pPacket.readChars(len);
#else
			account.level = pPacket.readChars(len).toString();
#endif
			break;

		case PLPROP_HORSEGIF:
			len = PropLimits::apply(pPacket.readGUChar(), PropLimits::HorseImageLength);
			account.character.horseImage = pPacket.readChars(len).toString();
			if (!account.character.horseImage.empty() && m_versionId < CLVER_2_1 && getExtension(account.character.horseImage).isEmpty())
				account.character.horseImage += ".gif";
			break;

		case PLPROP_HORSEBUSHES:
		{
			uint8_t count = pPacket.readGUChar();
			if (player == nullptr) break;
			player->m_horseBombCount = count;
			break;
		}

		case PLPROP_EFFECTCOLORS:
			len = pPacket.readGUChar();
			if (len > 0)
				pPacket.readGInt4();
			break;

		case PLPROP_CARRYNPC:
		{
			uint32_t newNpcId = pPacket.readGUInt();

			if (player != nullptr)
			{
				// Thrown.
				if (player->getCarryNpcId() != 0 && newNpcId == 0)
				{
					// TODO: Thrown
				}
				else
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
								if (other->getProp(PLPROP_CARRYNPC).readGUInt() == newNpcId)
								{
									// Somebody else got this NPC first.  Force the player to throw his down
									// and tell the player to remove the NPC from memory.
									sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_CARRYNPC >> (int)0);
									sendPacket(CString() >> (char)PLO_NPCDEL2 >> (char)level->getLevelName().length() << level->getLevelName() >> (int)newNpcId);
									m_server->sendPacketToOneLevel(CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PLPROP_CARRYNPC >> (int)0, level, { m_id });
									isOwner = false;
									newNpcId = 0;
									break;
								}
							}
						}
						if (isOwner)
						{
							// We own this NPC now so remove it from the level and have everybody else delete it.
							auto npc = m_server->getNPC(newNpcId);
							level->removeNPC(npc);
							m_server->sendPacketToAll(CString() >> (char)PLO_NPCDEL2 >> (char)level->getLevelName().length() << level->getLevelName() >> (int)newNpcId, { m_id });
						}
					}
				}
				player->setCarryNpcId(newNpcId);
			}
		}
		break;

		case PLPROP_APCOUNTER:
			account.apCounter = pPacket.readGUShort();
			break;

		case PLPROP_MAGICPOINTS:
		{
			uint8_t newMP = pPacket.readGUChar();
#ifdef V8NPCSERVER
			if (!(options & PLSETPROPS_SETBYPLAYER))
			{
#endif
				account.character.mp = PropLimits::apply(newMP, PropLimits::MaxMP);
#ifdef V8NPCSERVER
			}
#endif
			break;
		}

		case PLPROP_KILLSCOUNT:
			pPacket.readGInt();
			break;

		case PLPROP_DEATHSCOUNT:
			pPacket.readGInt();
			break;

		case PLPROP_ONLINESECS:
			pPacket.readGInt();
			break;

		case PLPROP_IPADDR:
			pPacket.readGInt5();
			break;

		case PLPROP_UDPPORT:
		{
			uint16_t udpPort = static_cast<uint16_t>(pPacket.readGInt());
			if (player == nullptr) break;
			player->m_udpport = udpPort;
			if (m_id != -1 && m_loaded)
				m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PLPROP_UDPPORT >> (int)player->m_udpport, this);
			// TODO: udp support.
			break;
		}

		case PLPROP_ALIGNMENT:
		{
			uint8_t newAlignment = pPacket.readGUChar();
#ifdef V8NPCSERVER
			if (!(options & PLSETPROPS_SETBYPLAYER))
			{
#endif
				account.character.ap = std::min<uint8_t>(newAlignment, 100);
#ifdef V8NPCSERVER
			}
#endif
			break;
		}

		case PLPROP_ADDITFLAGS:
			pPacket.readGUChar();
			// m_additionalFlags = pPacket.readGUChar();
			break;

		case PLPROP_ACCOUNTNAME:
			len = pPacket.readGUChar();
			pPacket.readChars(len);
			break;

		case PLPROP_BODYIMG:
			len = pPacket.readGUChar();
			account.character.bodyImage = PropLimits::apply(pPacket.readChars(len).toString(), PropLimits::BodyImageLength);
			break;

		case PLPROP_RATING:
			len = pPacket.readGInt();
			//m_eloRating = (float)((len >> 9) & 0xFFF);
			break;

		case PLPROP_ATTACHNPC:
		{
			// Only supports object_type 0 (NPC).
			unsigned char object_type = pPacket.readGUChar();
			unsigned int npcID = pPacket.readGUInt();
			if (player == nullptr) break;
			player->m_attachNPC = npcID;
			levelBuff >> (char)PLPROP_ATTACHNPC << getProp(PLPROP_ATTACHNPC);
			break;
		}

		case PLPROP_GMAPLEVELX:
		{
			int mx = pPacket.readGUChar();

			if (auto cmap = level->getMap(); level && cmap && cmap->isGmap())
			{
				auto& newLevelName = cmap->getLevelAt(mx, level->getMapY());
				setLevel(newLevelName, -1);
			}
#ifdef DEBUG
			printf("gmap level x: %d\n", level->getMapX());
#endif
			break;
		}

		case PLPROP_GMAPLEVELY:
		{
			int my = pPacket.readGUChar();

			if (auto cmap = level->getMap(); level && cmap && cmap->isGmap())
			{
				auto& newLevelName = cmap->getLevelAt(level->getMapX(), my);
				setLevel(newLevelName, -1);
			}
#ifdef DEBUG
			printf("gmap level y: %d\n", level->getMapY());
#endif
			break;
		}

		/*
	case PLPROP_UNKNOWN50:
		break;
*/
		case PLPROP_PCONNECTED:
			break;

		case PLPROP_PLANGUAGE:
			len = pPacket.readGUChar();
			account.language = pPacket.readChars(len).toString();
			break;

		case PLPROP_PSTATUSMSG:
			m_statusMsg = pPacket.readGUChar();
			if (m_id == -1 || !m_loaded)
				break;

			m_server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PLPROP_PSTATUSMSG >> (char)m_statusMsg, { m_id });
			break;

		case PLPROP_GATTRIB1:
		case PLPROP_GATTRIB2:
		case PLPROP_GATTRIB3:
		case PLPROP_GATTRIB4:
		case PLPROP_GATTRIB5:
		case PLPROP_GATTRIB6:
		case PLPROP_GATTRIB7:
		case PLPROP_GATTRIB8:
		case PLPROP_GATTRIB9:
		case PLPROP_GATTRIB10:
		case PLPROP_GATTRIB11:
		case PLPROP_GATTRIB12:
		case PLPROP_GATTRIB13:
		case PLPROP_GATTRIB14:
		case PLPROP_GATTRIB15:
		case PLPROP_GATTRIB16:
		case PLPROP_GATTRIB17:
		case PLPROP_GATTRIB18:
		case PLPROP_GATTRIB19:
		case PLPROP_GATTRIB20:
		case PLPROP_GATTRIB21:
		case PLPROP_GATTRIB22:
		case PLPROP_GATTRIB23:
		case PLPROP_GATTRIB24:
		case PLPROP_GATTRIB25:
		case PLPROP_GATTRIB26:
		case PLPROP_GATTRIB27:
		case PLPROP_GATTRIB28:
		case PLPROP_GATTRIB29:
		case PLPROP_GATTRIB30:
		{
			int index = propId - PLPROP_GATTRIB1;
			account.character.ganiAttributes[index] = pPacket.readChars(pPacket.readGUChar()).toString();
			break;
		}

		// OS type.
		// Windows: wind
		case PLPROP_OSTYPE:
			m_os = pPacket.readChars(pPacket.readGUChar());
			break;

		// Text codepage.
		// Example: 1252
		case PLPROP_TEXTCODEPAGE:
			m_envCodePage = pPacket.readGInt();
			break;

		case PLPROP_ONLINESECS2:
			//m_onlineTime = pPacket.readGUInt5();
			break;

		// Location, in pixels, of the player on the level in 2.30+ clients.
		// Bit 0x0001 controls if it is negative or not.
		// Bits 0xFFFE are the actual value.
		case PLPROP_X2:
			len = pPacket.readGUShort();
			account.character.pixelX = (len >> 1);

			// If the first bit is 1, our position is negative.
			if ((uint16_t)len & 0x0001)
				account.character.pixelX = -account.character.pixelX;

			// Let pre-2.30+ clients see 2.30+ movement.
			levelBuff2 >> (char)PLPROP_X << getProp(PLPROP_X);

			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			doTouchTest = true;
			break;

		case PLPROP_Y2:
			len = pPacket.readGUShort();
			account.character.pixelY = (len >> 1);

			// If the first bit is 1, our position is negative.
			if ((uint16_t)len & 0x0001)
				account.character.pixelY = -account.character.pixelY;

			// Let pre-2.30+ clients see 2.30+ movement.
			levelBuff2 >> (char)PLPROP_Y << getProp(PLPROP_Y);

			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			doTouchTest = true;
			break;

		case PLPROP_Z2:
			len = pPacket.readGUShort();
			account.character.pixelZ = (len >> 1);

			// If the first bit is 1, our position is negative.
			if ((uint16_t)len & 0x0001)
				account.character.pixelZ = -account.character.pixelZ;

			// Let pre-2.30+ clients see 2.30+ movement.
			levelBuff2 >> (char)PLPROP_Z << getProp(PLPROP_Z);

			account.status &= (~PLSTATUS_PAUSED);

			if (player != nullptr)
				player->setLastMovementTime(time(0));

			// Do collision testing.
			doTouchTest = true;
			break;

		case PLPROP_PLAYERLISTCATEGORY:
			(void)pPacket.readGUChar();
			break;

		case PLPROP_COMMUNITYNAME:
			pPacket.readChars(pPacket.readGUChar());
			break;

		default:
		{
			printf("Unidentified PLPROP: %i, readPos: %d\n", propId, pPacket.readPos());
			for (int i = 0; i < pPacket.length(); ++i)
				printf("%02x ", (unsigned char)pPacket[i]);
			printf("\n");
			sentInvalid = true;
			return;
		}
		}

		if ((options & PLSETPROPS_FORWARD) && clientPropsSharedLocal[propId])
			levelBuff >> (char)propId << getProp(propId);

		if ((options & PLSETPROPS_FORWARDSELF))
			selfBuff >> (char)propId << getProp(propId);
	}

	// Send Buffers Out
	if (isLoggedIn() && isLoaded())
	{
		if (globalBuff.length() > 0)
			m_server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->m_id << globalBuff, { m_id });
		if (levelBuff.length() > 0)
		{
			// We need to arrange the props packet in a certain way depending
			// on if our client supports precise movement or not.  Versions 2.3+
			// support precise movement.
			bool MOVE_PRECISE = false;
			if (m_versionId >= CLVER_2_3) MOVE_PRECISE = true;

			if (auto client = std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); client)
				m_server->sendPacketToLevelArea(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->m_id << (!MOVE_PRECISE ? levelBuff : levelBuff2) << (!MOVE_PRECISE ? levelBuff2 : levelBuff), client, { m_id });
		}
		if (selfBuff.length() > 0)
			this->sendPacket(CString() >> (char)PLO_PLAYERPROPS << selfBuff);

#ifdef V8NPCSERVER
		// Movement check.
		//if (options & PLSETPROPS_SETBYPLAYER)
		if (!rc)
		{
			if (doTouchTest)
			{
				if (account.character.sprite % 4 == 0)
					testSign();
				testTouch();
			}
		}
#endif
	}

	if (sentInvalid)
	{
		// If we are getting a whole bunch of invalid packets, something went wrong.  Disconnect the player.
		InvalidPackets++;
		if (InvalidPackets > 5)
		{
			serverlog.out("Player %s is sending invalid packets.\n", account.character.nickName.c_str());
			sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Disconnected for sending invalid packets.");
			m_server->deletePlayer(shared_from_this());
		}
	}
}

void Player::sendProps(const PropList& props)
{
	// Definition
	CString propPacket;

	int propCount = (isClient() && m_versionId < CLVER_2_1 ? 37 : props.size());
	for (int i = 0; i < propCount; ++i)
	{
		if (props[i])
			propPacket >> (char)i << getProp(i);
	}

	// Send Packet
	sendPacket(CString() >> (char)PLO_PLAYERPROPS << propPacket);
}

CString Player::getProps(const PropList& props)
{
	CString propPacket;

	// Start the prop packet.
	propPacket >> (char)PLO_OTHERPLPROPS >> (short)m_id;

	// Check if PLPROP_JOINLEAVELVL is set.
	if (isClient() && props[PLPROP_JOINLEAVELVL])
		propPacket >> (char)PLPROP_JOINLEAVELVL >> (char)1;

	// Create Props
	int propCount = (isClient() && m_versionId < CLVER_2_1 ? 37 : props.size());
	for (int i = 0; i < propCount; ++i)
	{
		if (i == PLPROP_JOINLEAVELVL) continue;

		if (i == PLPROP_ATTACHNPC)
		{
			if (auto client = std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); client != nullptr && client->m_attachNPC != 0)
			{
				propPacket >> (char)i;
				getProp(propPacket, i);
			}
		}

		if (props[i])
		{
			propPacket >> (char)i;
			getProp(propPacket, i);
		}
	}

	if (m_isExternal)
		propPacket >> (char)PLPROP_PLAYERLISTCATEGORY >> (char)PlayerListCategory::SERVERS;

	return propPacket;
}

///////////////////////////////////////////////////////////////////////////////

void Player::setPropsRC(CString& pPacket, Player* rc)
{
	bool hadBomb = false, hadBow = false;
	CString outPacket;

	// Skip playerworld
	pPacket.readChars(pPacket.readGUChar());

	// Read props from the packet.
	CString props = pPacket.readChars(pPacket.readGUChar());

	// Send props out.
	setProps(props, (m_id != -1 ? PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF : 0), rc);

	// Clear flags
	for (const auto& [flag, value] : account.flags)
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
	if (m_id != -1)
		sendPacket(outPacket);

	// Re-populate the flag list.
	auto flagCount = pPacket.readGUShort();
	while (flagCount > 0)
	{
		CString flag = pPacket.readChars(pPacket.readGUChar());
		std::string name = flag.readString("=").text();
		CString val = flag.readString("");

		setFlag(name, val, (m_id != -1));
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
	if (m_id != -1)
	{
		if (!hadBomb)
			sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << "Bomb");
	}

	// Warp the player to his new location now.
	if (m_id != -1 && isClient())
	{
		if (auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); player != nullptr)
			player->warp(account.level, getX(), getY(), 0);
	}
}

CString Player::getPropsRC()
{
	CString ret, props;
	ret >> (char)account.name.length() << account.name;
	ret >> (char)4 << "main"; // worldName

	// Add the props.
	for (int i = 0; i < PROPSCOUNT; ++i)
	{
		if (clientPropsForRCView[i])
			props >> (char)i << getProp(i);
	}
	ret >> (char)props.length() << props;

	// Add the player's flags.
	ret >> (short)account.flags.size();
	for (const auto& [flag, value] : account.flags)
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
