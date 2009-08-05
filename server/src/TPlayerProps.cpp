#include <vector>
#include "ICommon.h"
#include "main.h"
#include "TPlayer.h"
#include "TAccount.h"
#include "TLevel.h"
#include "CWordFilter.h"

#define serverlog	server->getServerLog()
#define rclog		server->getRCLog()
extern bool __sendLocal[propscount];
extern int __attrPackets[30];

/*
	TPlayer: Prop-Manipulation
*/
CString TPlayer::getProp(int pPropId)
{
	// TODO: is there a reason for this?  Check serveroptions.
	CSettings* settings = server->getSettings();
	switch (pPropId)
	{
		case PLPROP_NICKNAME:
		{
			if (nickName.length() > 223) nickName = nickName.subString(0, 223);
			return CString() >> (char)nickName.length() << nickName;
		}

		case PLPROP_MAXPOWER:
		return CString() >> (char)maxPower;

		case PLPROP_CURPOWER:
		return CString() >> (char)(power * 2);

		case PLPROP_RUPEESCOUNT:
		return CString() >> (int)gralatc;

		case PLPROP_ARROWSCOUNT:
		return CString() >> (char)arrowc;

		case PLPROP_BOMBSCOUNT:
		return CString() >> (char)bombc;

		case PLPROP_GLOVEPOWER:
		return CString() >> (char)glovePower;

		case PLPROP_BOMBPOWER:
		return CString() >> (char)bombPower;

		case PLPROP_SWORDPOWER:
		{
			if (swordImg.length() > 223) swordImg = swordImg.subString(0, 223);
			return CString() >> (char)(swordPower+30) >> (char)swordImg.length() << swordImg;
		}

		case PLPROP_SHIELDPOWER:
		{
			if (shieldImg.length() > 223) shieldImg = shieldImg.subString(0, 223);
			return CString() >> (char)(shieldPower+10) >> (char)shieldImg.length() << shieldImg;
		}

		case PLPROP_GANI:
		{
			if (isClient() && versionID < CLVER_2_1)
			{
				if (!bowImage.isEmpty())
					return CString() >> (char)(10 + bowImage.length()) << bowImage;
				else
					return CString() >> (char)bowPower;
			}

			if (gani.length() > 223) gani = gani.subString(0, 223);
			return CString() >> (char)gani.length() << gani;
		}

		case PLPROP_HEADGIF:
		{
			if (headImg.length() > 123) headImg = headImg.subString(0, 123);
			return CString() >> (char)(headImg.length()+100) << headImg;
		}

		case PLPROP_CURCHAT:
		{
			if (chatMsg.length() > 223) chatMsg = chatMsg.subString(0, 223);
			return CString() >> (char)chatMsg.length() << chatMsg;
		}

		case PLPROP_COLORS:
		return CString() >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4];

		case PLPROP_ID:
		return CString() >> (short)id;

		case PLPROP_X:
		return CString() >> (char)(x * 2);

		case PLPROP_Y:
		return CString() >> (char)(y * 2);

		case PLPROP_Z:
		return CString() >> (char)((z + 25) * 2);

		case PLPROP_SPRITE:
		return CString() >> (char)sprite;

		case PLPROP_STATUS:
		return CString() >> (char)status;

		case PLPROP_CARRYSPRITE:
		return CString() >> (char)carrySprite;

		case PLPROP_CURLEVEL:
		{
			if (isClient())// || type == PLTYPE_AWAIT)
			{
				if (pmap && pmap->getType() == MAPTYPE_GMAP)
					return CString() >> (char)pmap->getMapName().length() << pmap->getMapName();
				else
				{
					if (level != 0 && level->isSingleplayer() == true)
						return CString() >> (char)(levelName.length() + 13) << levelName << ".singleplayer";
					//else if (level != 0 && level->isGroupLevel() == true)
					//	return CString() >> (char)(levelName.length() + levelGroup.length()) << levelName << levelGroup;
					else
						return CString() >> (char)levelName.length() << levelName;
				}
			}
			else
				return CString() >> (char)1 << " ";
		}

		case PLPROP_HORSEGIF:
		{
			if (horseImg.length() > 223) horseImg = horseImg.subString(0, 223);
			return CString() >> (char)horseImg.length() << horseImg;
		}

		case PLPROP_HORSEBUSHES:
		return CString() >> (char)horsec;

		case PLPROP_EFFECTCOLORS:
		return CString() >> (char)0;

		case PLPROP_CARRYNPC:
		return CString() >> (int)carryNpcId;

		case PLPROP_APCOUNTER:
		return CString() >> (short)(apCounter+1);

		case PLPROP_MAGICPOINTS:
		return CString() >> (char)mp;

		case PLPROP_KILLSCOUNT:
		return CString() >> (int)kills;

		case PLPROP_DEATHSCOUNT:
		return CString() >> (int)deaths;

		case PLPROP_ONLINESECS:
		return CString() >> (int)onlineTime;

		case PLPROP_IPADDR:
		return CString().writeGInt5(accountIp);

		case PLPROP_UDPPORT:
		return CString() >> (int)udpport;

		case PLPROP_ALIGNMENT:
		return CString() >> (char)ap;

		case PLPROP_ADDITFLAGS:
		return CString() >> (char)additionalFlags;

		case PLPROP_ACCOUNTNAME:
		return CString() >> (char)accountName.length() << accountName;

		case PLPROP_BODYIMG:
		{
			if (bodyImg.length() > 223) bodyImg = bodyImg.subString(0, 223);
			return CString() >> (char)bodyImg.length() << bodyImg;
		}

		case PLPROP_RATING:
		{
			int temp = (((int)rating & 0xFFF) << 9) | ((int)deviation & 0x1FF);
			return CString() >> (int)temp;
		}

		// Simplifies login.
		// Manually send prop if you are leaving the level.
		// 1 = join level, 0 = leave level.
		case PLPROP_JOINLEAVELVL:
		return CString() >> (char)1;

		case PLPROP_PCONNECTED:
		return CString();

		case PLPROP_PLANGUAGE:
		return CString() >> (char)language.length() << language;

		case PLPROP_PSTATUSMSG:
		{
			//if (id == -1)
			//	break;

			if (statusMsg > server->getStatusList()->size() - 1)
				return CString() >> (char)0;
			else
				return CString() >> (char)statusMsg;
		}

		// OS type.
		// Windows: wind
		case PLPROP_OSTYPE:
		return CString() >> (char)os.length() << os;

		// Text codepage.
		// Example: 1252
		case PLPROP_TEXTCODEPAGE:
		return CString().writeGInt(codepage);

		case PLPROP_X2:
		{
			unsigned short val = (x2 < 0 ? -x2 : x2);
			val <<= 1;
			if (x2 < 0) val |= 0x0001;
			return CString().writeGShort(val);
		}

		case PLPROP_Y2:
		{
			unsigned short val = (y2 < 0 ? -y2 : y2);
			val <<= 1;
			if (y2 < 0) val |= 0x0001;
			return CString().writeGShort((short)val);
		}

		case PLPROP_Z2:
		{
			unsigned short val = (z2 < 0 ? -z2 : z2) + (25 * 16);
			val <<= 1;
			if (z2 < 0) val |= 0x0001;
			return CString().writeGShort(val);
		}

		case PLPROP_GMAPLEVELX:
		return CString() >> (char)gmaplevelx;

		case PLPROP_GMAPLEVELY:
		return CString() >> (char)gmaplevely;

		case PLPROP_COMMUNITYNAME:
		return CString() >> (char)communityName.length() << communityName;
	}

	if (inrange(pPropId, 37, 41) || inrange(pPropId, 46, 49) || inrange(pPropId, 54, 74))
	{
		for (unsigned int i = 0; i < sizeof(__attrPackets) / sizeof(int); i++)
		{
			if (__attrPackets[i] == pPropId)
			{
				if (attrList[i].length() > 223) attrList[i] = attrList[i].subString(0, 223);
				return CString() >> (char)attrList[i].length() << attrList[i];
			}
		}
	}

	return CString();
}

void TPlayer::setProps(CString& pPacket, bool pForward, bool pForwardToSelf, TPlayer *rc)
{
	// Forward to NPC-Server if the npc-server isn't setting them
	if (server->hasNPCServer() && server->getNPCServer() != rc)
	{
		pPacket.removeI(0, pPacket.readPos());
		pPacket.setRead(0);
		server->getNPCServer()->sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->id << pPacket);
	}

	CSettings *settings = server->getSettings();
	CString globalBuff, levelBuff, levelBuff2, selfBuff;
	bool doOverride = (server->hasNPCServer() && server->getNPCServer() == rc);
	bool doSignCheck = false;
	int len = 0;
	bool sentInvalid = false;

	while (pPacket.bytesLeft() > 0)
	{
		unsigned char propId = pPacket.readGUChar();
		
		switch (propId)
		{
			case PLPROP_NICKNAME:
			{
				CString nick = pPacket.readChars(pPacket.readGUChar());

				// Word filter.
				int filter = server->getWordFilter()->apply(this, nick, FILTER_CHECK_NICK);
				if (filter & FILTER_ACTION_WARN)
				{
					if (nickName.isEmpty())
						setNick("unknown");
				}
				else setNick(nick, doOverride);

				globalBuff >> (char)propId << getProp(propId);
				if (!pForwardToSelf)
					selfBuff >> (char)propId << getProp(propId);
			}
			break;

			case PLPROP_MAXPOWER:
				maxPower = pPacket.readGUChar();
				maxPower = clip(maxPower, 0, settings->getInt("heartlimit", 20));
				maxPower = clip(maxPower, 0, 20);
			break;

			case PLPROP_CURPOWER:
				power = (float)pPacket.readGUChar() / 2.0f;
				power = clip(power, 0, (float)maxPower);
			break;

			case PLPROP_RUPEESCOUNT:
				if (rc != 0)
				{
					if (server->getSettings()->getBool("normaladminscanchangegralats", true) || (rc->isStaff() && rc->hasRight(PLPERM_MODIFYSTAFFACCOUNT)))
					{
						gralatc = pPacket.readGUInt();
						gralatc = clip(gralatc, 0, 9999999);
					}
					else
						pPacket.readGUInt();

				}
				else
				{
					gralatc = pPacket.readGUInt();
					gralatc = clip(gralatc, 0, 9999999);
				}
			break;

			case PLPROP_ARROWSCOUNT:
				arrowc = pPacket.readGUChar();
				arrowc = clip(arrowc, 0, 99);
			break;

			case PLPROP_BOMBSCOUNT:
				bombc = pPacket.readGUChar();
				bombc = clip(bombc, 0, 99);
			break;

			case PLPROP_GLOVEPOWER:
				glovePower = pPacket.readGUChar();
				glovePower = clip(glovePower, 0, 3);
			break;

			case PLPROP_BOMBPOWER:
				bombPower = pPacket.readGUChar();
				bombPower = clip(bombPower, 0, 3);
			break;

			case PLPROP_SWORDPOWER:
			{
				int sp = pPacket.readGUChar();
				if (sp <= 4)
					swordImg = CString() << "sword" << CString(sp) << (versionID < CLVER_2_1 ? ".gif" : ".png");
				else
				{
					sp -= 30;
					len = pPacket.readGUChar();
					if (len > 0)
					{
						swordImg = pPacket.readChars(len);
						if (versionID < CLVER_2_1 && getExtension(swordImg).isEmpty())
							swordImg << ".gif";
					}
					else swordImg = "";
				}
				swordPower = clip(sp, ((settings->getBool("healswords", false) == true) ? -(settings->getInt("swordlimit", 3)) : 0), settings->getInt("swordlimit", 3));
			}
			break;

			case PLPROP_SHIELDPOWER:
			{
				int sp = pPacket.readGUChar();
				if (sp <= 3)
					shieldImg = CString() << "shield" << CString(sp) << (versionID < CLVER_2_1 ? ".gif" : ".png");
				else
				{
					sp -= 10;
					if (sp < 0) break;
					len = pPacket.readGUChar();
					if (len > 0)
					{
						shieldImg = pPacket.readChars(len);
						if (versionID < CLVER_2_1 && getExtension(shieldImg).isEmpty())
							shieldImg << ".gif";
					}
					else shieldImg = "";
				}
				shieldPower = clip(sp, 0, settings->getInt("shieldlimit", 3));
			}
			break;

			case PLPROP_GANI:
			{
				if (isClient() && versionID < CLVER_2_1)
				{
					int sp = pPacket.readGUChar();
					if (sp < 10)
					{
						bowPower = sp;
						bowImage.clear();
					}
					else
					{
						bowPower = 10;
						sp -= 10;
						if (sp < 0) break;
						bowImage = CString() << pPacket.readChars(sp);
						if (versionID < CLVER_2_1 && getExtension(bowImage).isEmpty())
							bowImage << ".gif";
					}
					break;
				}

				gani = pPacket.readChars(pPacket.readGUChar());
				if (gani == "spin")
				{
					CString nPacket;
					nPacket >> (char)PLO_HITOBJECTS >> (short)id >> (char)swordPower;
					char hx = (char)((x + 1.5f) * 2);
					char hy = (char)((y + 2.0f) * 2);
					server->sendPacketToLevel(CString() << nPacket >> (char)(hx) >> (char)(hy - 4), 0, level, this);
					server->sendPacketToLevel(CString() << nPacket >> (char)(hx) >> (char)(hy + 4), 0, level, this);
					server->sendPacketToLevel(CString() << nPacket >> (char)(hx - 4) >> (char)(hy), 0, level, this);
					server->sendPacketToLevel(CString() << nPacket >> (char)(hx + 4) >> (char)(hy), 0, level, this);
				}
			}
			break;

			case PLPROP_HEADGIF:
				len = pPacket.readGUChar();
				if (len < 100)
				{
					headImg = CString() << "head" << CString(len) << (versionID < CLVER_2_1 ? ".gif" : ".png");
					globalBuff >> (char)propId << getProp(propId);
				}
				else if (len > 100)
				{
					headImg = pPacket.readChars(len-100);
					if (versionID < CLVER_2_1 && getExtension(headImg).isEmpty())
						headImg << ".gif";

					globalBuff >> (char)propId << getProp(propId);
				}
			break;

			case PLPROP_CURCHAT:
			{
				chatMsg = pPacket.readChars(pPacket.readGUChar());

				// Try to process the chat.  If it wasn't processed, apply the word filter to it.
				if (!processChat(chatMsg))
				{
					int found = server->getWordFilter()->apply(this, chatMsg, FILTER_CHECK_CHAT);
					if (pForwardToSelf == false && ((found & FILTER_ACTION_REPLACE) || (found & FILTER_ACTION_WARN)))
						selfBuff >> (char)propId << getProp(propId);
				}
			}
			break;

			case PLPROP_COLORS:
				for (unsigned int i = 0; i < sizeof(colors) / sizeof(unsigned char); ++i)
					colors[i] = pPacket.readGUChar();
			break;

			case PLPROP_ID:
				pPacket.readGUShort();
			break;

			case PLPROP_X:
				x = (float)(pPacket.readGUChar() / 2.0f);
				status &= (~PLSTATUS_PAUSED);
				lastMovement = time(0);
				grMovementUpdated = true;

				// Let 2.30+ clients see pre-2.30 movement.
				x2 = (int)(x * 16);
				levelBuff2 >> (char)PLPROP_X2 << getProp(PLPROP_X2);
			break;

			case PLPROP_Y:
				y = (float)(pPacket.readGUChar() / 2.0f);
				status &= (~PLSTATUS_PAUSED);
				lastMovement = time(0);
				grMovementUpdated = true;

				// Let 2.30+ clients see pre-2.30 movement.
				y2 = (int)(y * 16);
				levelBuff2 >> (char)PLPROP_Y2 << getProp(PLPROP_Y2);

				// Do collision testing.
				doSignCheck = true;
			break;

			case PLPROP_Z:
				z = (float)(pPacket.readGUChar() / 2.0f) - 25.0f;
				status &= (~PLSTATUS_PAUSED);
				lastMovement = time(0);
				grMovementUpdated = true;

				// Let 2.30+ clients see pre-2.30 movement.
				z2 = (int)(z * 16);
				levelBuff2 >> (char)PLPROP_Z2 << getProp(PLPROP_Z2);
			break;

			case PLPROP_SPRITE:
				sprite = pPacket.readGUChar();

				// Do collision testing.
				doSignCheck = true;
			break;

			case PLPROP_STATUS:
			{
				int oldStatus = status;
				status = pPacket.readGUChar();
				//printf("%s: status: %d, oldStatus: %d\n", accountName.text(), status, oldStatus );

				if (id == -1) break;

				// When they come back to life, give them hearts.
				if ((oldStatus & PLSTATUS_DEAD) > 0 && (status & PLSTATUS_DEAD) == 0)
				{
					power = clip((ap < 20 ? 3 : (ap < 40 ? 5 : maxPower)), 0.0f, maxPower);

					// If we are the leader of the level, call warp().  This will fix NPCs not
					// working again after we respawn.
					if (level->getPlayer(0) == this)
						warp(levelName, x, y, time(0));
				}

				// When they die, increase deaths and make somebody else level leader.
				if ((oldStatus & PLSTATUS_DEAD) == 0 && (status & PLSTATUS_DEAD) > 0)
				{
					if (level->isSparringZone() == false)
					{
						deaths++;
						dropItemsOnDeath();
					}

					// If we are the leader and there are more players on the level, we want to remove
					// ourself from the leader position and tell the new leader that they are the leader.
					if (level->getPlayer(0) == this && level->getPlayer(1) != 0)
					{
						level->removePlayer(this);
						level->addPlayer(this);
						level->getPlayer(0)->sendPacket(CString() >> (char)PLO_ISLEADER);
					}
				}
			}
			break;

			case PLPROP_CARRYSPRITE:
				carrySprite = pPacket.readGUChar();
			break;

			case PLPROP_CURLEVEL:
				len = pPacket.readGUChar();
				levelName = pPacket.readChars(len);
			break;

			case PLPROP_HORSEGIF:
				len = pPacket.readGUChar();
				horseImg = pPacket.readChars(len);
				if (versionID < CLVER_2_1 && getExtension(horseImg).isEmpty())
					horseImg << ".gif";
			break;

			case PLPROP_HORSEBUSHES:
				horsec = pPacket.readGUChar();
			break;

			case PLPROP_EFFECTCOLORS:
				len = pPacket.readGUChar();
				if (len > 0)
					pPacket.readGInt4();
			break;

			case PLPROP_CARRYNPC:
			{
				carryNpcId = pPacket.readGUInt();

				// TODO: Remove when an npcserver is created.
				if (server->getSettings()->getBool("duplicatecanbecarried", false) == false)
				{
					bool isOwner = true;
					{
						std::vector<TPlayer*>* playerList = server->getPlayerList();
						for (std::vector<TPlayer*>::iterator i = playerList->begin(); i != playerList->end(); ++i)
						{
							TPlayer* other = *i;
							if (other == this) continue;
							if (other->getProp(PLPROP_CARRYNPC).readGUInt() == carryNpcId)
							{
								// Somebody else got this NPC first.  Force the player to throw his down
								// and tell the player to remove the NPC from memory.
								carryNpcId = 0;
								isOwner = false;
								sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_CARRYNPC >> (int)0);
								sendPacket(CString() >> (char)PLO_NPCDEL2 >> (char)level->getLevelName().length() << level->getLevelName() >> (int)carryNpcId);
								server->sendPacketToLevel(CString() >> (char)PLO_OTHERPLPROPS >> (short)id >> (char)PLPROP_CARRYNPC >> (int)0, pmap, this);
								break;
							}
						}
					}
					if (isOwner)
					{
						// We own this NPC now so remove it from the level and have everybody else delete it.
						TNPC* npc = server->getNPC(carryNpcId);
						level->removeNPC(npc);
						server->sendPacketToAll(CString() >> (char)PLO_NPCDEL2 >> (char)level->getLevelName().length() << level->getLevelName() >> (int)carryNpcId);
					}
				}
			}
			break;

			case PLPROP_APCOUNTER:
				apCounter = pPacket.readGUShort();
			break;

			case PLPROP_MAGICPOINTS:
				mp = pPacket.readGUChar();
				mp = clip(mp, 0, 100);
			break;

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
				udpport = pPacket.readGInt();
				if (id != -1 && loaded)
					server->sendPacketTo(PLTYPE_ANYCLIENT, CString() >> (char)PLO_OTHERPLPROPS >> (short)id >> (char)PLPROP_UDPPORT >> (int)udpport, this);
				printf("udp_port: %d\n", udpport);
				// TODO: udp support.
			break;

			case PLPROP_ALIGNMENT:
				ap = pPacket.readGUChar();
				ap = clip(ap, 0, 100);
			break;

			case PLPROP_ADDITFLAGS:
				additionalFlags = pPacket.readGUChar();
			break;

			case PLPROP_ACCOUNTNAME:
				len = pPacket.readGUChar();
				pPacket.readChars(len);
			break;

			case PLPROP_BODYIMG:
				len = pPacket.readGUChar();
				bodyImg = pPacket.readChars(len);
			break;

			case PLPROP_RATING:
				len = pPacket.readGInt();
				//rating = (float)((len >> 9) & 0xFFF);
				//oldDeviation = deviation = (float)(len & 0x1FF);
			break;

			case PLPROP_UNKNOWN42:
				len = pPacket.readGInt4();
				printf( "PLPROP_UNKNOWN42: %d\n", len );
				break;
/*
			case PLPROP_UNKNOWN50:
				break;
*/
			case PLPROP_PCONNECTED:
			break;

			case PLPROP_PLANGUAGE:
				len = pPacket.readGUChar();
				language = pPacket.readChars(len);
			break;

			case PLPROP_PSTATUSMSG:
				statusMsg = pPacket.readGUChar();
				if (id == -1 || !loaded)
					break;

				server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)id >> (char)PLPROP_PSTATUSMSG >> (char)statusMsg, this);
			break;

			case PLPROP_GATTRIB1:  attrList[0]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB2:  attrList[1]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB3:  attrList[2]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB4:  attrList[3]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB5:  attrList[4]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB6:  attrList[5]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB7:  attrList[6]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB8:  attrList[7]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB9:  attrList[8]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB10: attrList[9]  = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB11: attrList[10] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB12: attrList[11] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB13: attrList[12] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB14: attrList[13] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB15: attrList[14] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB16: attrList[15] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB17: attrList[16] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB18: attrList[17] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB19: attrList[18] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB20: attrList[19] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB21: attrList[20] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB22: attrList[21] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB23: attrList[22] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB24: attrList[23] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB25: attrList[24] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB26: attrList[25] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB27: attrList[26] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB28: attrList[27] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB29: attrList[28] = pPacket.readChars(pPacket.readGUChar()); break;
			case PLPROP_GATTRIB30: attrList[29] = pPacket.readChars(pPacket.readGUChar()); break;

			// OS type.
			// Windows: wind
			case PLPROP_OSTYPE:
				os = pPacket.readChars(pPacket.readGUChar());
				break;

			// Text codepage.
			// Example: 1252
			case PLPROP_TEXTCODEPAGE:
				codepage = pPacket.readGInt();
				break;

			// Location, in pixels, of the player on the level in 2.30+ clients.
			// Bit 0x0001 controls if it is negative or not.
			// Bits 0xFFFE are the actual value.
			case PLPROP_X2:
				x2 = len = pPacket.readGUShort();
				status &= (~PLSTATUS_PAUSED);
				lastMovement = time(0);
				grMovementUpdated = true;

				// If the first bit is 1, our position is negative.
				x2 >>= 1;
				if ((short)len & 0x0001) x2 = -x2;

				// Let pre-2.30+ clients see 2.30+ movement.
				x = (float)x2 / 16.0f;
				levelBuff2 >> (char)PLPROP_X << getProp(PLPROP_X);
				break;

			case PLPROP_Y2:
				y2 = len = pPacket.readGUShort();
				status &= (~PLSTATUS_PAUSED);
				lastMovement = time(0);
				grMovementUpdated = true;

				// If the first bit is 1, our position is negative.
				y2 >>= 1;
				if ((short)len & 0x0001) y2 = -y2;

				// Let pre-2.30+ clients see 2.30+ movement.
				y = (float)y2 / 16.0f;
				levelBuff2 >> (char)PLPROP_Y << getProp(PLPROP_Y);

				// Do collision testing.
				doSignCheck = true;
				break;

			case PLPROP_Z2:
				z2 = len = pPacket.readGUShort();
				status &= (~PLSTATUS_PAUSED);
				lastMovement = time(0);
				grMovementUpdated = true;

				// If the first bit is 1, our position is negative.
				z2 >>= 1;
				if ((short)len & 0x0001) z2 = -z2;

				// Let pre-2.30+ clients see 2.30+ movement.
				z2 -= 25 * 16;
				z = (float)z2 / 16.0f;
				levelBuff2 >> (char)PLPROP_Z << getProp(PLPROP_Z);
				break;

			case PLPROP_GMAPLEVELX:
			{
				gmaplevelx = pPacket.readGUChar();
				if (pmap)
				{
					levelName = pmap->getLevelAt(gmaplevelx, gmaplevely);
					leaveLevel();
					setLevel(levelName, -1);
				}
				printf("gmap level x: %d\n", gmaplevelx);
				break;
			}

			case PLPROP_GMAPLEVELY:
			{
				gmaplevely = pPacket.readGUChar();
				if (pmap)
				{
					levelName = pmap->getLevelAt(gmaplevelx, gmaplevely);
					leaveLevel();
					setLevel(levelName, -1);
				}
				printf("gmap level y: %d\n", gmaplevely);
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

		if (pForward && __sendLocal[propId] == true)
			levelBuff >> (char)propId << getProp(propId);

		if (pForwardToSelf)
			selfBuff >> (char)propId << getProp(propId);
	}

	// Send Buffers Out
	if (isLoggedIn() && isLoaded())
	{
		if (globalBuff.length() > 0)
			server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->id << globalBuff, this, true);
		if (levelBuff.length() > 0)
		{
			// We need to arrange the props packet in a certain way depending
			// on if our client supports precise movement or not.  Versions 2.3+
			// support precise movement.
			bool MOVE_PRECISE = false;
			if (versionID >= CLVER_2_3) MOVE_PRECISE = true;

			server->sendPacketToLevel(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->id << (!MOVE_PRECISE ? levelBuff : levelBuff2) << (!MOVE_PRECISE ? levelBuff2 : levelBuff), pmap, this, false);
		}
		if (selfBuff.length() > 0)
			this->sendPacket(CString() >> (char)PLO_PLAYERPROPS << selfBuff);

		// Movement check.
		if (doSignCheck) testSign();
	}

	if (sentInvalid)
	{
		// If we are getting a whole bunch of invalid packets, something went wrong.  Disconnect the player.
		invalidPackets++;
		if (invalidPackets > 5)
			server->deletePlayer(this);
	}
}

void TPlayer::sendProps(const bool *pProps, int pCount)
{
	// Definition
	CString propPacket;

	// Create Props
	if (isClient() && versionID < CLVER_2_1) pCount = 37;
	for (int i = 0; i < pCount; ++i)
	{
		if (pProps[i])
			propPacket >> (char)i << getProp(i);
	}

	// Send Packet
	sendPacket(CString() >> (char)PLO_PLAYERPROPS << propPacket);
}

CString TPlayer::getProps(const bool *pProps, int pCount)
{
	CString propPacket;

	// Start the prop packet.
	propPacket >> (char)PLO_OTHERPLPROPS >> (short)this->id;

	if (pCount > 0)
	{
		// Check if PLPROP_JOINLEAVELVL is set.
		if (isClient() && pProps[PLPROP_JOINLEAVELVL])
			propPacket >> (char)PLPROP_JOINLEAVELVL >> (char)1;

		// Create Props
		if (isClient() && versionID < CLVER_2_1) pCount = 37;
		for (int i = 0; i < pCount; ++i)
		{
			if (i == PLPROP_JOINLEAVELVL) continue;
			if (pProps[i])
				propPacket >> (char)i << getProp(i);
		}
	}

	return propPacket;
}
