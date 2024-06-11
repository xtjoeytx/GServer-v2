#include "IDebug.h"
#include <vector>

#include "IEnums.h"
#include "IUtil.h"
#include "Level.h"
#include "Map.h"
#include "Player.h"
#include "Server.h"

#define serverlog m_server->getServerLog()
#define rclog m_server->getRCLog()
extern bool __sendLocal[propscount];
extern int __attrPackets[30];

/*
	Player: Prop-Manipulation
*/
void Player::getProp(CString& buffer, int pPropId) const
{
	auto level = m_currentLevel.lock();
	auto map = m_pmap.lock();

	switch (pPropId)
	{
		case PLPROP_NICKNAME:
			buffer >> (char)m_nickName.length() << m_nickName;
			return;

		case PLPROP_MAXPOWER:
			buffer >> (char)m_maxHitpoints;
			return;

		case PLPROP_CURPOWER:
			buffer >> (char)(m_hitpoints * 2);
			return;

		case PLPROP_RUPEESCOUNT:
			buffer >> (int)m_gralatCount;
			return;

		case PLPROP_ARROWSCOUNT:
			buffer >> (char)m_arrowCount;
			return;

		case PLPROP_BOMBSCOUNT:
			buffer >> (char)m_bombCount;
			return;

		case PLPROP_GLOVEPOWER:
			buffer >> (char)m_glovePower;
			return;

		case PLPROP_BOMBPOWER:
			buffer >> (char)m_bombPower;
			return;

		case PLPROP_SWORDPOWER:
			buffer >> (char)(m_swordPower + 30) >> (char)m_swordImage.length() << m_swordImage;
			return;

		case PLPROP_SHIELDPOWER:
			buffer >> (char)(m_shieldPower + 10) >> (char)m_shieldImage.length() << m_shieldImage;
			return;

		case PLPROP_GANI:
		{
			if (isClient() && m_versionId < CLVER_2_1)
			{
				if (!m_bowImage.isEmpty())
					buffer >> (char)(10 + m_bowImage.length()) << m_bowImage;
				else
					buffer >> (char)m_bowPower;
				return;
			}

			buffer >> (char)m_gani.length() << m_gani;
			return;
		}

		case PLPROP_HEADGIF:
			buffer >> (char)(m_headImage.length() + 100) << m_headImage;
			return;

		case PLPROP_CURCHAT:
			buffer >> (char)m_chatMessage.length() << m_chatMessage;
			return;

		case PLPROP_COLORS:
			buffer >> (char)m_colors[0] >> (char)m_colors[1] >> (char)m_colors[2] >> (char)m_colors[3] >> (char)m_colors[4];
			return;

		case PLPROP_ID:
			buffer >> (short)m_id;
			return;

		case PLPROP_X:
			buffer >> (char)(m_x * 2);
			return;

		case PLPROP_Y:
			buffer >> (char)(m_y * 2);
			return;

		case PLPROP_Z:
			buffer >> (char)((m_z + 0.5f) + 50);
			return;

		case PLPROP_SPRITE:
			buffer >> (char)m_sprite;
			return;

		case PLPROP_STATUS:
			buffer >> (char)m_status;
			return;

		case PLPROP_CARRYSPRITE:
			buffer >> (char)m_carrySprite;
			return;

		case PLPROP_CURLEVEL:
		{
			if (isClient()) // || type == PLTYPE_AWAIT)
			{
				if (map && map->getType() == MapType::GMAP)
					buffer >> (char)map->getMapName().length() << map->getMapName();
				else
				{
					if (level != nullptr && level->isSingleplayer())
						buffer >> (char)(m_levelName.length() + 13) << m_levelName << ".singleplayer";
					else
						buffer >> (char)m_levelName.length() << m_levelName;
				}
				return;
			}
			else
				buffer >> (char)1 << " ";
			return;
		}

		case PLPROP_HORSEGIF:
			buffer >> (char)m_horseImage.length() << m_horseImage;
			return;

		case PLPROP_HORSEBUSHES:
			buffer >> (char)m_horseBombCount;
			return;

		case PLPROP_EFFECTCOLORS:
			buffer >> (char)0;
			return;

		case PLPROP_CARRYNPC:
			buffer >> (int)m_carryNpcId;
			return;

		case PLPROP_APCOUNTER:
			buffer >> (short)(m_apCounter + 1);
			return;

		case PLPROP_MAGICPOINTS:
			buffer >> (char)m_mp;
			return;

		case PLPROP_KILLSCOUNT:
			buffer >> (int)m_kills;
			return;

		case PLPROP_DEATHSCOUNT:
			buffer >> (int)m_deaths;
			return;

		case PLPROP_ONLINESECS:
			buffer >> (int)m_onlineTime;
			return;

		case PLPROP_IPADDR:
			buffer.writeGInt5(m_accountIp);
			return;

		case PLPROP_UDPPORT:
			buffer >> (int)m_udpport;
			return;

		case PLPROP_ALIGNMENT:
			buffer >> (char)m_ap;
			return;

		case PLPROP_ADDITFLAGS:
			buffer >> (char)m_additionalFlags;
			return;

		case PLPROP_ACCOUNTNAME:
			buffer >> (char)m_accountName.length() << m_accountName;
			return;

		case PLPROP_BODYIMG:
			buffer >> (char)m_bodyImage.length() << m_bodyImage;
			return;

		case PLPROP_RATING:
		{
			int temp = (((int)m_eloRating & 0xFFF) << 9) | ((int)m_eloDeviation & 0x1FF);
			buffer >> (int)temp;
			return;
		}

		case PLPROP_ATTACHNPC:
		{
			// Only attach type 0 (NPC) supported.
			buffer >> (char)0 >> (int)m_attachNPC;
			return;
		}

			// Simplifies login.
			// Manually send prop if you are leaving the level.
			// 1 = join level, 0 = leave level.
		case PLPROP_JOINLEAVELVL:
			buffer >> (char)1;
			return;

		case PLPROP_PCONNECTED:
			//return CString();
			return;

		case PLPROP_PLANGUAGE:
			buffer >> (char)m_language.length() << m_language;
			return;

		case PLPROP_PSTATUSMSG:
		{
			//if (id == -1)
			//	break;

			if (m_statusMsg > m_server->getStatusList().size() - 1)
				buffer >> (char)0;
			else
				buffer >> (char)m_statusMsg;
			return;
		}

			// OS type.
			// Windows: wind
		case PLPROP_OSTYPE:
			buffer >> (char)m_os.length() << m_os;
			return;

			// Text codepage.
			// Example: 1252
		case PLPROP_TEXTCODEPAGE:
			buffer.writeGInt(m_envCodePage);
			return;

		case PLPROP_X2:
		{
			uint16_t val = ((uint16_t)std::abs(m_x * 16.0f)) << 1;
			if (m_x < 0)
				val |= 0x0001;
			buffer.writeGShort(val);
			return;
		}

		case PLPROP_Y2:
		{
			uint16_t val = ((uint16_t)std::abs(m_y * 16.0f)) << 1;
			if (m_y < 0)
				val |= 0x0001;
			buffer.writeGShort(val);
			return;
		}

		case PLPROP_Z2:
		{
			uint16_t val = ((uint16_t)std::abs(m_z * 16.0f)) << 1;
			if (m_z < 0)
				val |= 0x0001;
			buffer.writeGShort(val);
			return;
		}

		case PLPROP_GMAPLEVELX:
			buffer >> (char)(level ? level->getMapX() : 0);
			return;

		case PLPROP_GMAPLEVELY:
			buffer >> (char)(level ? level->getMapY() : 0);
			return;

			// TODO(joey): figure this out. Something to do with guilds? irc-related
			//	(char)(some bitflag for something, uses the first 3 bits im not sure)
			//		okay i tested some flags, 1 removes the channel. 3 adds it. not sure what third bit does.
		case PLPROP_UNKNOWN81:
			//return CString();
			return;

		case PLPROP_COMMUNITYNAME:
			buffer >> (char)m_communityName.length() << m_communityName;
			return;

		default:
			break;
	}

	if (inrange(pPropId, 37, 41) || inrange(pPropId, 46, 49) || inrange(pPropId, 54, 74))
	{
		for (auto i = 0; i < sizeof(__attrPackets) / sizeof(int); i++)
		{
			if (__attrPackets[i] == pPropId)
			{
				char len = std::min(m_attrList[i].length(), 223);
				buffer >> (char)len << m_attrList[i].subString(0, len);
				return;
			}
		}
	}
}

void Player::setProps(CString& pPacket, uint8_t options, Player* rc)
{
	auto level = getLevel();

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
					if (m_nickName.isEmpty())
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
					setMaxPower(newMaxPower);
					setPower((float)m_maxHitpoints);

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
				float p = (float)pPacket.readGUChar() / 2.0f;
				if (m_ap < 40 && p > m_hitpoints) break;
				//if ((status & PLSTATUS_HIDESWORD) != 0)
				//	break;
				setPower(p);
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
						if (m_server->getSettings().getBool("normaladminscanchangegralats", true) ||
							(rc->isStaff() && rc->hasRight(PLPERM_SETRIGHTS)))
							m_gralatCount = newGralatCount;
					}
					else
					{
						m_gralatCount = newGralatCount;
					}
#ifdef V8NPCSERVER
				}
#endif
				break;
			}

			case PLPROP_ARROWSCOUNT:
				m_arrowCount = pPacket.readGUChar();
				m_arrowCount = clip(m_arrowCount, 0, 99);
				break;

			case PLPROP_BOMBSCOUNT:
				m_bombCount = pPacket.readGUChar();
				m_bombCount = clip(m_bombCount, 0, 99);
				break;

			case PLPROP_GLOVEPOWER:
			{
				uint8_t newGlovePower = pPacket.readGUChar();
#ifdef V8NPCSERVER
				if (!(options & PLSETPROPS_SETBYPLAYER))
				{
#endif
					m_glovePower = std::min<uint8_t>(newGlovePower, 3);
#ifdef V8NPCSERVER
				}
#endif
				break;
			}

			case PLPROP_BOMBPOWER:
				m_bombPower = pPacket.readGUChar();
				m_bombPower = clip(m_bombPower, 0, 3);
				break;

			case PLPROP_SWORDPOWER:
			{
				int sp = pPacket.readGUChar();
				CString img;

				if (sp <= 4)
				{
					auto& settings = m_server->getSettings();
					sp = clip(sp, 0, settings.getInt("swordlimit", 3));
					img = CString() << "sword" << CString(sp) << (m_versionId < CLVER_2_1 ? ".gif" : ".png");
				}
				else
				{
					sp -= 30;
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
					setSwordPower(sp);
#ifdef V8NPCSERVER
				}
#endif
				setSwordImage(img);
			}
			break;

			case PLPROP_SHIELDPOWER:
			{
				int sp = pPacket.readGUChar();
				CString img;

				if (sp <= 3)
				{
					auto& settings = m_server->getSettings();
					sp = clip(sp, 0, settings.getInt("shieldlimit", 3));
					img = CString() << "shield" << CString(sp) << (m_versionId < CLVER_2_1 ? ".gif" : ".png");
				}
				else
				{
					// This fixes an odd bug with the 1.41 client.
					if (pPacket.bytesLeft() == 0) continue;

					sp -= 10;
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
					setShieldPower(sp);
#ifdef V8NPCSERVER
				}
#endif
				setShieldImage(img);
			}
			break;

			case PLPROP_GANI:
			{
				if (isClient() && m_versionId < CLVER_2_1)
				{
					int sp = pPacket.readGUChar();
					if (sp < 10)
					{
						m_bowPower = sp;
						m_bowImage.clear();
					}
					else
					{
						m_bowPower = 10;
						sp -= 10;
						if (sp < 0) break;
						m_bowImage = CString() << pPacket.readChars(sp);
						if (!m_bowImage.isEmpty() && m_versionId < CLVER_2_1 && getExtension(m_bowImage).isEmpty())
							m_bowImage << ".gif";
					}
					break;
				}

				setGani(pPacket.readChars(pPacket.readGUChar()));
				if (m_gani == "spin")
				{
					CString nPacket;
					nPacket >> (short)m_id >> (char)m_swordPower;
					char hx = (char)((m_x + 1.5f) * 2);
					char hy = (char)((m_y + 2.0f) * 2);
					m_server->sendPacketToOneLevel(
						{ PLO_HITOBJECTS, CString() << nPacket >> (char)(hx) >> (char)(hy - 4) }, m_currentLevel,
						{ m_id });
					m_server->sendPacketToOneLevel(
						{ PLO_HITOBJECTS, CString() << nPacket >> (char)(hx) >> (char)(hy + 4) }, m_currentLevel,
						{ m_id });
					m_server->sendPacketToOneLevel(
						{ PLO_HITOBJECTS, CString() << nPacket >> (char)(hx - 4) >> (char)(hy) }, m_currentLevel,
						{ m_id });
					m_server->sendPacketToOneLevel(
						{ PLO_HITOBJECTS, CString() << nPacket >> (char)(hx + 4) >> (char)(hy) }, m_currentLevel,
						{ m_id });
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
					setHeadImage(img);
					globalBuff >> (char)propId << getProp(propId);
				}

				break;
			}

			case PLPROP_CURCHAT:
			{
				len = pPacket.readGUChar();
				m_chatMessage = pPacket.readChars(std::min(len, 223));
				m_lastChat = time(0);

				// Try to process the chat.  If it wasn't processed, apply the word filter to it.
				if (!processChat(m_chatMessage))
				{
					int found = m_server->getWordFilter().apply(this, m_chatMessage, FILTER_CHECK_CHAT);
					if (!(options & PLSETPROPS_FORWARDSELF))
					{
						if ((found & FILTER_ACTION_REPLACE) || (found & FILTER_ACTION_WARN))
							selfBuff >> (char)propId << getProp(propId);
					}
				}

#ifdef V8NPCSERVER
				// Send chat to npcs if this wasn't changed by the npcserver
				if (!rc && !m_chatMessage.isEmpty())
				{
					if (level != nullptr)
						level->sendChatToLevel(this, m_chatMessage.text());
				}
#endif
			}
			break;

			case PLPROP_COLORS:
				for (unsigned char& color: m_colors)
					color = pPacket.readGUChar();
				break;

			case PLPROP_ID:
				pPacket.readGUShort();
				break;

			case PLPROP_X:
				m_x = (float)(pPacket.readGUChar() / 2.0f);
				m_status &= (~PLSTATUS_PAUSED);
				m_lastMovement = time(0);
				m_grMovementUpdated = true;

				// Do collision testing.
				doTouchTest = true;

				// Let 2.30+ clients see pre-2.30 movement.
				levelBuff2 >> (char)PLPROP_X2 << getProp(PLPROP_X2);
				break;

			case PLPROP_Y:
				m_y = (float)(pPacket.readGUChar() / 2.0f);
				m_status &= (~PLSTATUS_PAUSED);
				m_lastMovement = time(0);
				m_grMovementUpdated = true;

				// Do collision testing.
				doTouchTest = true;

				// Let 2.30+ clients see pre-2.30 movement.
				levelBuff2 >> (char)PLPROP_Y2 << getProp(PLPROP_Y2);
				break;

			case PLPROP_Z:
				m_z = (float)pPacket.readGUChar() - 50.0f;
				m_status &= (~PLSTATUS_PAUSED);
				m_lastMovement = time(0);
				m_grMovementUpdated = true;
				doTouchTest = true;

				// Let 2.30+ clients see pre-2.30 movement.
				levelBuff2 >> (char)PLPROP_Z2 << getProp(PLPROP_Z2);
				break;

			case PLPROP_SPRITE:
				m_sprite = pPacket.readGUChar();

#ifndef V8NPCSERVER
				// Do collision testing.
				doTouchTest = true;
#endif
				break;

			case PLPROP_STATUS:
			{
				int oldStatus = m_status;
				m_status = pPacket.readGUChar();
				//printf("%s: status: %d, oldStatus: %d\n", m_accountName.text(), status, oldStatus );

				if (m_id == -1) break;

				// When they come back to life, give them hearts.
				if ((oldStatus & PLSTATUS_DEAD) > 0 && (m_status & PLSTATUS_DEAD) == 0)
				{
					auto newPower = clip((m_ap < 20 ? 3 : (m_ap < 40 ? 5 : m_maxHitpoints)), 0.5f, m_maxHitpoints);
					setPower(newPower);

					selfBuff >> (char)PLPROP_CURPOWER >> (char)(m_hitpoints * 2.0f);
					levelBuff >> (char)PLPROP_CURPOWER >> (char)(m_hitpoints * 2.0f);

					if (level != nullptr && level->isPlayerLeader(m_id))
						sendPacket({ PLO_ISLEADER, CString() });

					/*
					// If we are the leader of the level, call warp().  This will fix NPCs not
					// working again after we respawn.
					if (level != 0 && level->getPlayer(0) == this)
						warp(m_levelName, x, y, time(0));
					*/
				}

				// When they die, increase deaths and make somebody else level leader.
				if ((oldStatus & PLSTATUS_DEAD) == 0 && (m_status & PLSTATUS_DEAD) > 0)
				{
					if (!level->isSparringZone())
					{
						m_deaths++;
						dropItemsOnDeath();
					}

					// If we are the leader and there are more players on the level, we want to remove
					// ourself from the leader position and tell the new leader that they are the leader.
					if (level->isPlayerLeader(m_id) && level->getPlayers().size() > 1)
					{
						level->removePlayer(m_id);
						level->addPlayer(m_id);

						auto leader = m_server->getPlayer(level->getPlayers().front());
						if (leader) leader->sendPacket({ PLO_ISLEADER, CString() });
					}
				}
			}
			break;

			case PLPROP_CARRYSPRITE:
				m_carrySprite = pPacket.readGUChar();
				break;

			case PLPROP_CURLEVEL:
				len = pPacket.readGUChar();
#ifdef V8NPCSERVER
				pPacket.readChars(len);
#else
				m_levelName = pPacket.readChars(len);
#endif
				break;

			case PLPROP_HORSEGIF:
				len = pPacket.readGUChar();
				m_horseImage = pPacket.readChars(std::min(len, 219)); // limit is 219 in case it appends .gif
				if (!m_horseImage.isEmpty() && m_versionId < CLVER_2_1 && getExtension(m_horseImage).isEmpty())
					m_horseImage << ".gif";
				break;

			case PLPROP_HORSEBUSHES:
				m_horseBombCount = pPacket.readGUChar();
				break;

			case PLPROP_EFFECTCOLORS:
				len = pPacket.readGUChar();
				if (len > 0)
					pPacket.readGInt4();
				break;

			case PLPROP_CARRYNPC:
			{
				m_carryNpcId = pPacket.readGUInt();

				// TODO: Remove when an npcserver is created.
				if (m_server->getSettings().getBool("duplicatecanbecarried", false) == false)
				{
					bool isOwner = true;
					{
						auto& playerList = m_server->getPlayerList();
						for (auto& [otherId, other]: playerList)
						{
							if (other.get() == this) continue;
							if (other->getProp(PLPROP_CARRYNPC).readGUInt() == m_carryNpcId)
							{
								// Somebody else got this NPC first.  Force the player to throw his down
								// and tell the player to remove the NPC from memory.
								m_carryNpcId = 0;
								isOwner = false;
								sendPacket({ PLO_PLAYERPROPS, CString() >> (char)PLPROP_CARRYNPC >> (int)0 });
								sendPacket({ PLO_NPCDEL2,
											 CString() >> (char)level->getLevelName().length() << level->getLevelName() >> (int)m_carryNpcId });
								m_server->sendPacketToOneLevel({ PLO_OTHERPLPROPS,
																 CString() >> (short)m_id >> (char)PLPROP_CARRYNPC >> (int)0 },
															   level, { m_id });
								break;
							}
						}
					}
					if (isOwner)
					{
						// We own this NPC now so remove it from the level and have everybody else delete it.
						auto npc = m_server->getNPC(m_carryNpcId);
						level->removeNPC(npc);
						m_server->sendPacketToAll({ PLO_NPCDEL2, CString() >> (char)level->getLevelName().length()
																				  << level->getLevelName() >>
																	 (int)m_carryNpcId },
												  { m_id });
					}
				}
			}
			break;

			case PLPROP_APCOUNTER:
				m_apCounter = pPacket.readGUShort();
				break;

			case PLPROP_MAGICPOINTS:
			{
				uint8_t newMP = pPacket.readGUChar();
#ifdef V8NPCSERVER
				if (!(options & PLSETPROPS_SETBYPLAYER))
				{
#endif
					m_mp = std::min<uint8_t>(newMP, 100);
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
				m_udpport = pPacket.readGInt();
				if (m_id != -1 && m_loaded)
					m_server->sendPacketToType(PLTYPE_ANYCLIENT, { PLO_OTHERPLPROPS, CString() >> (short)m_id >> (char)PLPROP_UDPPORT >> (int)m_udpport }, this);
				// TODO: udp support.
				break;

			case PLPROP_ALIGNMENT:
			{
				uint8_t newAlignment = pPacket.readGUChar();
#ifdef V8NPCSERVER
				if (!(options & PLSETPROPS_SETBYPLAYER))
				{
#endif
					m_ap = std::min<uint8_t>(newAlignment, 100);
#ifdef V8NPCSERVER
				}
#endif
				break;
			}

			case PLPROP_ADDITFLAGS:
				m_additionalFlags = pPacket.readGUChar();
				break;

			case PLPROP_ACCOUNTNAME:
				len = pPacket.readGUChar();
				pPacket.readChars(len);
				break;

			case PLPROP_BODYIMG:
				len = pPacket.readGUChar();
				setBodyImage(pPacket.readChars(len));
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
				m_attachNPC = npcID;
				levelBuff >> (char)PLPROP_ATTACHNPC << getProp(PLPROP_ATTACHNPC);
				break;
			}

			case PLPROP_GMAPLEVELX:
			{
				int mx = pPacket.readGUChar();

				if (auto cmap = level->getMap(); level && cmap)
				{
					auto& newLevelName = cmap->getLevelAt(mx, level->getMapY());
					leaveLevel();
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

				if (auto cmap = level->getMap(); level && cmap)
				{
					auto& newLevelName = cmap->getLevelAt(level->getMapX(), my);
					leaveLevel();
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
				m_language = pPacket.readChars(len);
				break;

			case PLPROP_PSTATUSMSG:
				m_statusMsg = pPacket.readGUChar();
				if (m_id == -1 || !m_loaded)
					break;

				m_server->sendPacketToAll(
					{ PLO_OTHERPLPROPS, CString() >> (short)m_id >> (char)PLPROP_PSTATUSMSG >> (char)m_statusMsg },
					{ m_id });
				break;

			case PLPROP_GATTRIB1:
				m_attrList[0] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB2:
				m_attrList[1] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB3:
				m_attrList[2] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB4:
				m_attrList[3] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB5:
				m_attrList[4] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB6:
				m_attrList[5] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB7:
				m_attrList[6] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB8:
				m_attrList[7] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB9:
				m_attrList[8] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB10:
				m_attrList[9] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB11:
				m_attrList[10] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB12:
				m_attrList[11] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB13:
				m_attrList[12] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB14:
				m_attrList[13] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB15:
				m_attrList[14] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB16:
				m_attrList[15] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB17:
				m_attrList[16] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB18:
				m_attrList[17] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB19:
				m_attrList[18] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB20:
				m_attrList[19] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB21:
				m_attrList[20] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB22:
				m_attrList[21] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB23:
				m_attrList[22] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB24:
				m_attrList[23] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB25:
				m_attrList[24] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB26:
				m_attrList[25] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB27:
				m_attrList[26] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB28:
				m_attrList[27] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB29:
				m_attrList[28] = pPacket.readChars(pPacket.readGUChar());
				break;
			case PLPROP_GATTRIB30:
				m_attrList[29] = pPacket.readChars(pPacket.readGUChar());
				break;

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

				// Location, in pixels, of the player on the level in 2.30+ clients.
				// Bit 0x0001 controls if it is negative or not.
				// Bits 0xFFFE are the actual value.
			case PLPROP_X2:
				len = pPacket.readGUShort();
				m_x = (len >> 1) / 16.0f;

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					m_x = -m_x;

				// Let pre-2.30+ clients see 2.30+ movement.
				levelBuff2 >> (char)PLPROP_X << getProp(PLPROP_X);

				m_status &= (~PLSTATUS_PAUSED);
				m_lastMovement = time(0);
				m_grMovementUpdated = true;
				doTouchTest = true;
				break;

			case PLPROP_Y2:
				len = pPacket.readGUShort();
				m_y = (len >> 1) / 16.0f;

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					m_y = -m_y;

				// Let pre-2.30+ clients see 2.30+ movement.
				levelBuff2 >> (char)PLPROP_Y << getProp(PLPROP_Y);

				m_status &= (~PLSTATUS_PAUSED);
				m_lastMovement = time(0);
				m_grMovementUpdated = true;

				// Do collision testing.
				doTouchTest = true;
				break;

			case PLPROP_Z2:
				len = pPacket.readGUShort();
				m_z = (len >> 1) / 16.0f;

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					m_z = -m_z;

				// Let pre-2.30+ clients see 2.30+ movement.
				levelBuff2 >> (char)PLPROP_Z << getProp(PLPROP_Z);

				m_status &= (~PLSTATUS_PAUSED);
				m_lastMovement = time(0);
				m_grMovementUpdated = true;

				// Do collision testing.
				doTouchTest = true;

				//// Let pre-2.30+ clients see 2.30+ movement.
				//m_z = (float)(int)(((float)z2 / 16.0f) + 0.5f);
				break;

			case PLPROP_UNKNOWN81:
			{
				auto val = pPacket.readGUChar();
				break;
			}

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
			}
				return;
		}

		if ((options & PLSETPROPS_FORWARD) && __sendLocal[propId])
			levelBuff >> (char)propId << getProp(propId);

		if ((options & PLSETPROPS_FORWARDSELF))
			selfBuff >> (char)propId << getProp(propId);
	}

	// Send Buffers Out
	if (isLoggedIn() && isLoaded())
	{
		if (globalBuff.length() > 0)
			m_server->sendPacketToAll({ PLO_OTHERPLPROPS, CString() >> (short)m_id << globalBuff }, { m_id });
		if (levelBuff.length() > 0)
		{
			// We need to arrange the props packet in a certain way depending
			// on if our client supports precise movement or not.  Versions 2.3+
			// support precise movement.
			bool MOVE_PRECISE = false;
			if (m_versionId >= CLVER_2_3) MOVE_PRECISE = true;

			m_server->sendPacketToLevelArea({ PLO_OTHERPLPROPS,
											  CString() >> (short)m_id << (!MOVE_PRECISE ? levelBuff : levelBuff2)
																	   << (!MOVE_PRECISE ? levelBuff2 : levelBuff) },
											shared_from_this(),
											{ m_id });
		}
		if (selfBuff.length() > 0)
			sendPacket({ PLO_PLAYERPROPS, CString() << selfBuff });

#ifdef V8NPCSERVER
		// Movement check.
		//if (options & PLSETPROPS_SETBYPLAYER)
		if (!rc)
		{
			if (doTouchTest)
			{
				if (m_sprite % 4 == 0)
					testSign();
				testTouch();
			}
		}
#endif
	}

	if (sentInvalid)
	{
		// If we are getting a whole bunch of invalid packets, something went wrong.  Disconnect the player.
		m_invalidPackets++;
		if (m_invalidPackets > 5)
		{
			serverlog.out("[%s] Player %s is sending invalid packets.\n", m_server->getName().text(),
						  m_nickName.text());
			sendPacket({ PLO_DISCMESSAGE, CString() << "Disconnected for sending invalid packets." });
			m_server->deletePlayer(shared_from_this());
		}
	}
}

void Player::sendProps(const bool* pProps, int pCount)
{
	// Definition
	CString propPacket;

	// Create Props
	if (isClient() && m_versionId < CLVER_2_1) pCount = 37;
	for (int i = 0; i < pCount; ++i)
	{
		if (pProps[i])
			propPacket >> (char)i << getProp(i);
	}

	// Send Packet
	sendPacket({ PLO_PLAYERPROPS, CString() << propPacket });
}

CString Player::getProps(const bool* pProps, int pCount)
{
	CString propPacket;

	// Start the prop packet.
	propPacket >> (short)m_id;

	if (pCount > 0)
	{
		// Check if PLPROP_JOINLEAVELVL is set.
		if (isClient() && pProps[PLPROP_JOINLEAVELVL])
			propPacket >> (char)PLPROP_JOINLEAVELVL >> (char)1;

		// Create Props
		if (isClient() && m_versionId < CLVER_2_1) pCount = 37;
		for (int i = 0; i < pCount; ++i)
		{
			if (i == PLPROP_JOINLEAVELVL) continue;

			if (i == PLPROP_ATTACHNPC && m_attachNPC != 0)
			{
				propPacket >> (char)i;
				getProp(propPacket, i);
			}

			if (pProps[i])
			{
				propPacket >> (char)i;
				getProp(propPacket, i);
			}
		}
	}

	if (m_isExternal)
		propPacket >> (char)81 << "!";

	return propPacket;
}
