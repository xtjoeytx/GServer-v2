#include "IDebug.h"
#include <vector>
#include <time.h>
#include "TServer.h"
#include "TNPC.h"
#include "CFileSystem.h"
#include "TMap.h"
#include "TLevel.h"
#include "IEnums.h"

#ifdef V8NPCSERVER
#include "CScriptEngine.h"
#include "TPlayer.h"
#endif

const char __nSavePackets[10] = { 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };
const char __nAttrPackets[30] = { 36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 };

static CString toWeaponName(const CString& code);
static CString doJoins(const CString& code, CFileSystem* fs);

TNPC::TNPC(const CString& pImage, const CString& pScript, float pX, float pY, TServer* pServer, TLevel* pLevel, bool pLevelNPC)
	: TNPC(pServer, pLevelNPC)
{
	setX(pX);
	setY(pY);
	image = pImage;
	level = pLevel;
	originalScript = pScript;
#ifdef V8NPCSERVER
	origImage = image;
	origX = x;
	origY = y;
#endif

	// Set the gmap levels.
	if (level)
	{
		TMap *gmap = level->getMap();
		if (gmap && gmap->getType() == MAPTYPE_GMAP)
		{
			gmaplevelx = (unsigned char) gmap->getLevelX(level->getLevelName());
			gmaplevely = (unsigned char) gmap->getLevelY(level->getLevelName());
		}

#ifdef V8NPCSERVER
		origLevel = level->getLevelName();
#endif
	}

	// TODO: Create plugin hook so NPCServer can acquire/format code.
	
	// Needs to be called so it creates a script-object
	//if (!pScript.isEmpty())
		setScriptCode(pScript);
}

TNPC::TNPC(TServer *pServer, bool pLevelNPC)
	: server(pServer), levelNPC(pLevelNPC), blockPositionUpdates(false),
	x(30), y(30.5), x2((int)(x * 16)), y2((int)(y * 16)),
	gmaplevelx(0), gmaplevely(0),
	hurtX(32.0f), hurtY(32.0f), id(0), rupees(0),
	darts(0), bombs(0), glovePower(0), bombPower(0), swordPower(0), shieldPower(0),
	visFlags(1), blockFlags(0), sprite(2), power(0), ap(50),
	gani("idle"), level(nullptr)
#ifdef V8NPCSERVER
	, origX(x), origY(y), persistNpc(false), canWarp(false), width(32), height(32)
	, timeout(0), _scriptEventsMask(0xFF), _scriptObject(0)
#endif
{
	memset((void*)colors, 0, sizeof(colors));
	memset((void*)saves, 0, sizeof(saves));
	memset((void*)modTime, 0, sizeof(modTime));

	// bowImage for pre-2.x clients.
	bowImage >> (char)0;

	// imagePart needs to be Graal-packed.
	for (int i = 0; i < 6; i++)
		imagePart.writeGChar(0);

	// We need to alter the modTime of the following props as they should be always sent.
	// If we don't, they won't be sent until the prop gets modified.
	modTime[NPCPROP_IMAGE] = modTime[NPCPROP_SCRIPT] = modTime[NPCPROP_X] = modTime[NPCPROP_Y]
		= modTime[NPCPROP_VISFLAGS] = modTime[NPCPROP_ID] = modTime[NPCPROP_SPRITE] = modTime[NPCPROP_MESSAGE]
		= modTime[NPCPROP_GMAPLEVELX] = modTime[NPCPROP_GMAPLEVELY]
		= modTime[NPCPROP_X2] = modTime[NPCPROP_Y2] = time(0);

	// Needs to be called so it creates a script-object
	setScriptCode("");
}

TNPC::~TNPC()
{
#ifdef V8NPCSERVER
	freeScriptResources();
#endif
}

void TNPC::setScriptCode(const CString& pScript)
{
	bool firstExecution = originalScript.isEmpty();
	originalScript = pScript;

#ifdef V8NPCSERVER
	// Clear any joined code
	classMap.clear();

	if (!serverScript.isEmpty())
		freeScriptResources();
#endif

	bool levelModificationNPCHack = false;

	// See if the NPC sets the level as a sparring zone.
	if (pScript.subString(0, 12) == "sparringzone")
	{
		level->setSparringZone(true);
		levelModificationNPCHack = true;
	}

	// See if the NPC sets the level as singleplayer.
	if (pScript.subString(0, 12) == "singleplayer")
	{
		level->setSingleplayer(true);
		levelModificationNPCHack = true;
	}

	// Separate clientside and serverside scripts.
#ifdef V8NPCSERVER
	CString s = pScript;
	serverScript = s.readString("//#CLIENTSIDE");
	clientScript = s.readString("");
#else
	clientScript = pScript;
#endif

	// Do joins.
	if (!serverScript.isEmpty()) serverScript = doJoins(serverScript, server->getFileSystem());
	if (!clientScript.isEmpty()) clientScript = doJoins(clientScript, server->getFileSystem());

	// See if the NPC should block position updates from the level leader.
#ifdef V8NPCSERVER
		blockPositionUpdates = true;
#else
	if (clientScript.find("//#BLOCKPOSITIONUPDATES") != -1)
		blockPositionUpdates = true;
#endif

	// Remove comments and trim the code if specified.
	if (!clientScript.isEmpty())
	{
		clientScriptFormatted = removeComments(clientScript, "\n");
		std::vector<CString> code = clientScriptFormatted.tokenize("\n");
		clientScriptFormatted.clear();
		for (std::vector<CString>::iterator i = code.begin(); i != code.end(); ++i)
			clientScriptFormatted << (*i).trim() << "\xa7";
	}
	
	// Search for toweapons in the clientside code and extract the name of the weapon.
	weaponName = toWeaponName(clientScript);

	// Just a little warning for people who don't know.
	if (clientScriptFormatted.length() > 0x705F)
		printf("WARNING: Clientside script of NPC (%s) exceeds the limit of 28767 bytes.\n", (weaponName.length() != 0 ? weaponName.text() : image.text()));

#ifdef V8NPCSERVER
	// Delete old npc, and send npc to level. Currently only doing this for database npcs, everything else
	//	would need "update level" to take changes.
	if (!firstExecution && !npcName.empty() && level)
	{
		// TODO(joey): refactor
		TMap *map = level->getMap();
		server->sendPacketToLevel(CString() >> (char)PLO_NPCDEL >> (int)getId(), map, level, 0, true);

		CString packet = CString() >> (char)PLO_NPCPROPS >> (int)getId() << getProps(0);
		server->sendPacketToLevel(packet, map, level, 0, true);
	}

	// Compile and execute the script.
	bool executed = server->getScriptEngine()->ExecuteNpc(this);
	if (executed) {
		V8ENV_D("SCRIPT COMPILED\n");
		this->queueNpcAction("npc.created");
	}
	else
		V8ENV_D("Could not compile npc script\n");
#endif
}

CString TNPC::getProp(unsigned char pId, int clientVersion) const
{
	switch(pId)
	{
		case NPCPROP_IMAGE:
		return CString() >> (char)image.length() << image;

		case NPCPROP_SCRIPT:
			return CString() >> (short)(clientScriptFormatted.length() > 0x3FFF ? 0x3FFF : clientScriptFormatted.length()) << clientScriptFormatted.subString(0, 0x3FFF);

		case NPCPROP_X:
		return CString() >> (char)(x * 2);

		case NPCPROP_Y:
		return CString() >> (char)(y * 2);

		case NPCPROP_POWER:
		return CString() >> (char)power;

		case NPCPROP_RUPEES:
		return CString() >> (int)rupees;

		case NPCPROP_ARROWS:
		return CString() >> (char)darts;

		case NPCPROP_BOMBS:
		return CString() >> (char)bombs;

		case NPCPROP_GLOVEPOWER:
		return CString() >> (char)glovePower;

		case NPCPROP_BOMBPOWER:
		return CString() >> (char)bombPower;

		case NPCPROP_SWORDIMAGE:
		if (swordPower == 0)
			return CString() >> (char)0;
		else
			return CString() >> (char)(swordPower + 30) >> (char)swordImage.length() << swordImage;

		case NPCPROP_SHIELDIMAGE:
		if (shieldPower + 10 > 10)
			return CString() >> (char)(shieldPower + 10) >> (char)shieldImage.length() << shieldImage;
		else
			return CString() >> (char)0;

		case NPCPROP_GANI:
		if (clientVersion < CLVER_2_1)
			return bowImage;
		return CString() >> (char)gani.length() << gani;

		case NPCPROP_VISFLAGS:
		return CString() >> (char)visFlags;

		case NPCPROP_BLOCKFLAGS:
		return CString() >> (char)blockFlags;

		case NPCPROP_MESSAGE:
		return CString() >> (char)chatMsg.subString(0, 200).length() << chatMsg.subString(0, 200);

		case NPCPROP_HURTDXDY:
		return CString() >> (char)((hurtX*32)+32) >> (char)((hurtY*32)+32);

		case NPCPROP_ID:
		return CString() >> (int)id;

		// Sprite is deprecated and has been replaced by def.gani.
		// Sprite now holds the direction of the npc.  sprite % 4 gives backwards compatibility.
		case NPCPROP_SPRITE:
		{
			if (clientVersion < CLVER_2_1) return CString() >> (char)sprite;
			else return CString() >> (char)(sprite % 4);
		}

		case NPCPROP_COLORS:
		return CString() >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4];

		case NPCPROP_NICKNAME:
		return CString() >> (char)nickName.length() << nickName;

		case NPCPROP_HORSEIMAGE:
		return CString() >> (char)horseImage.length() << horseImage;

		case NPCPROP_HEADIMAGE:
		return CString() >> (char)(headImage.length() + 100) << headImage;

		case NPCPROP_ALIGNMENT:
		return CString() >> (char)ap;

		case NPCPROP_IMAGEPART:
		return CString() << imagePart;

		case NPCPROP_BODYIMAGE:
		return CString() >> (char)bodyImage.length() << bodyImage;

		case NPCPROP_GMAPLEVELX:
		return CString() >> (char)(level && level->getMap() ? level->getMap()->getLevelX(level->getActualLevelName()) : 0);

		case NPCPROP_GMAPLEVELY:
		return CString() >> (char)(level && level->getMap() ? level->getMap()->getLevelY(level->getActualLevelName()) : 0);

#ifdef V8NPCSERVER
		case NPCPROP_SCRIPTER:
		return CString() >> (char)scripterName.length() << scripterName;

		case NPCPROP_NAME:
		return CString() >> (char)npcName.length() << npcName;

		case NPCPROP_TYPE:
		return CString() >> (char)npcType.length() << npcType;

		case NPCPROP_CURLEVEL:
		{
			CString tmpLevelName = (level ? level->getLevelName() : "");
			return CString() >> (char)tmpLevelName.length() << tmpLevelName;
		}
#endif

		case NPCPROP_CLASS:
		return CString() >> (short)0;

		case NPCPROP_X2:
		{
			unsigned short val = (x2 < 0 ? -x2 : x2);
			val <<= 1;
			if (x2 < 0) val |= 0x0001;
			return CString().writeGShort(val);
		}

		case NPCPROP_Y2:
		{
			unsigned short val = (y2 < 0 ? -y2 : y2);
			val <<= 1;
			if (y2 < 0) val |= 0x0001;
			return CString().writeGShort((short)val);
		}
	}

	// Saves.
	if (inrange(pId, NPCPROP_SAVE0, NPCPROP_SAVE9))
	{
		return CString() >> (char)saves[NPCPROP_SAVE0 - pId];
	}

	// Gani attributes.
	if (inrange(pId, NPCPROP_GATTRIB1, NPCPROP_GATTRIB5) || inrange(pId, NPCPROP_GATTRIB6, NPCPROP_GATTRIB9) || inrange(pId, NPCPROP_GATTRIB10, NPCPROP_GATTRIB30))
	{
		// TODO(joey): Are we really looping every single possible attribute to find the one we want....??
		for (unsigned int i = 0; i < sizeof(__nAttrPackets); i++)
		{
			if (__nAttrPackets[i] == pId)
				return CString() >> (char)gAttribs[i].length() << gAttribs[i];
		}
	}

	return CString();
}

CString TNPC::getProps(time_t newTime, int clientVersion) const
{
	bool oldcreated = server->getSettings()->getBool("oldcreated", "false");
	CString retVal;
	int pmax = NPCPROP_COUNT;
	if (clientVersion < CLVER_2_1) pmax = 36;

	for (int i = 0; i < pmax; i++)
	{
		if (modTime[i] != 0 && modTime[i] >= newTime)
		{
			if (oldcreated && i == NPCPROP_VISFLAGS && newTime == 0)
				retVal >> (char)i >> (char)(visFlags | NPCVISFLAG_VISIBLE);
			else
				retVal >> (char)i << getProp(i, clientVersion);
		}
	}
	if (clientVersion > CLVER_1_411)
	{
		if (modTime[NPCPROP_GANI] == 0 && image == "#c#")
			retVal >> (char)NPCPROP_GANI >> (char)4 << "idle";
	}

	return retVal;
}

CString TNPC::setProps(CString& pProps, int clientVersion, bool pForward)
{
	bool hasMoved = false;

	// TODO(joey): Most of these props will eventually be ignored

	CString ret;
	int len = 0;
	while (pProps.bytesLeft() > 0)
	{
		unsigned char propId = pProps.readGUChar();
		CString oldProp = getProp(propId);
		//printf( "propId: %d\n", propId );
		switch (propId)
		{
			case NPCPROP_IMAGE:
				visFlags |= NPCVISFLAG_VISIBLE;
				image = pProps.readChars(pProps.readGUChar());
				if (!image.isEmpty() && clientVersion < CLVER_2_1 && getExtension(image).isEmpty())
					image << ".gif";
			break;

			case NPCPROP_SCRIPT:
				clientScript = pProps.readChars(pProps.readGUShort());
			break;

			case NPCPROP_X:
				if (blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				x = (float)(pProps.readGChar()) / 2.0f;
				x2 = (int)(x * 16);
				hasMoved = true;
			break;

			case NPCPROP_Y:
				if (blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				y = (float)(pProps.readGChar()) / 2.0f;
				y2 = (int)(y * 16);
				hasMoved = true;
				break;

			case NPCPROP_POWER:
				power = pProps.readGUChar();
			break;

			case NPCPROP_RUPEES:
				rupees = pProps.readGUInt();
			break;

			case NPCPROP_ARROWS:
				darts = pProps.readGUChar();
			break;

			case NPCPROP_BOMBS:
				bombs = pProps.readGUChar();
			break;

			case NPCPROP_GLOVEPOWER:
				glovePower = pProps.readGUChar();
			break;

			case NPCPROP_BOMBPOWER:
				bombPower = pProps.readGUChar();
			break;

			case NPCPROP_SWORDIMAGE:
			{
				int sp = pProps.readGUChar();
				if (sp <= 4)
					swordImage = CString() << "sword" << CString(sp) << (clientVersion < CLVER_2_1 ? ".gif" : ".png");
				else
				{
					sp -= 30;
					len = pProps.readGUChar();
					if (len > 0)
					{
						swordImage = pProps.readChars(len);
						if (!swordImage.isEmpty() && clientVersion < CLVER_2_1 && getExtension(swordImage).isEmpty())
							swordImage << ".gif";
					}
					else swordImage = "";
					//swordPower = clip(sp, ((settings->getBool("healswords", false) == true) ? -(settings->getInt("swordlimit", 3)) : 0), settings->getInt("swordlimit", 3));
				}
				swordPower = sp;
			}
			break;

			case NPCPROP_SHIELDIMAGE:
			{
				int sp = pProps.readGUChar();
				if (sp <= 3)
					shieldImage = CString() << "shield" << CString(sp) << (clientVersion < CLVER_2_1 ? ".gif" : ".png");
				else
				{
					sp -= 10;
					len = pProps.readGUChar();
					if (len > 0)
					{
						shieldImage = pProps.readChars(len);
						if (!shieldImage.isEmpty() && clientVersion < CLVER_2_1 && getExtension(shieldImage).isEmpty())
							shieldImage << ".gif";
					}
					else shieldImage = "";
				}
				shieldPower = sp;
			}
			break;

			case NPCPROP_GANI:
				if (clientVersion < CLVER_2_1)
				{
					// Older clients don't use ganis.  This is the bow power and image instead.
					int sp = pProps.readGUChar();
					if (sp < 10)
						bowImage = CString() >> (char)sp;
					else
					{
						sp -= 10;
						if (sp < 0) break;
						bowImage = pProps.readChars(sp);
						if (!bowImage.isEmpty() && clientVersion < CLVER_2_1 && getExtension(bowImage).isEmpty())
							bowImage << ".gif";
						bowImage = CString() >> (char)(10 + bowImage.length()) << bowImage;
					}
					break;
				}
				gani = pProps.readChars(pProps.readGUChar());
			break;

			case NPCPROP_VISFLAGS:
				visFlags = pProps.readGUChar();
			break;

			case NPCPROP_BLOCKFLAGS:
				blockFlags = pProps.readGUChar();
			break;

			case NPCPROP_MESSAGE:
				chatMsg = pProps.readChars(pProps.readGUChar());
			break;

			case NPCPROP_HURTDXDY:
				hurtX = ((float)(pProps.readGUChar()-32))/32;
				hurtY = ((float)(pProps.readGUChar()-32))/32;
			break;

			case NPCPROP_ID:
				pProps.readGUInt();
			break;

			case NPCPROP_SPRITE:
				sprite = pProps.readGUChar();
			break;

			case NPCPROP_COLORS:
				for (int i = 0; i < 5; i++)
					colors[i] = pProps.readGUChar();
			break;

			case NPCPROP_NICKNAME:
				nickName = pProps.readChars(pProps.readGUChar());
			break;

			case NPCPROP_HORSEIMAGE:
				horseImage = pProps.readChars(pProps.readGUChar());
				if (!horseImage.isEmpty() && clientVersion < CLVER_2_1 && getExtension(horseImage).isEmpty())
					horseImage << ".gif";
			break;

			case NPCPROP_HEADIMAGE:
				len = pProps.readGUChar();
				if (len < 100)
					headImage = CString() << "head" << CString(len) << (clientVersion < CLVER_2_1 ? ".gif" : ".png");
				else
				{
					headImage = pProps.readChars(len - 100);
					if (!headImage.isEmpty() && clientVersion < CLVER_2_1 && getExtension(headImage).isEmpty())
						headImage << ".gif";
				}
			break;

			case NPCPROP_ALIGNMENT:
				ap = pProps.readGUChar();
				ap = clip(ap, 0, 100);
			break;

			case NPCPROP_IMAGEPART:
				imagePart = pProps.readChars(6);
			break;

			case NPCPROP_BODYIMAGE:
				bodyImage = pProps.readChars(pProps.readGUChar());
			break;

			case NPCPROP_GMAPLEVELX:
				gmaplevelx = pProps.readGUChar();
			break;

			case NPCPROP_GMAPLEVELY:
				gmaplevely = pProps.readGUChar();
			break;

#ifdef V8NPCSERVER
			case NPCPROP_SCRIPTER:
				scripterName = pProps.readChars(pProps.readGUChar());
				break;

			case NPCPROP_NAME:
				npcName = pProps.readChars(pProps.readGUChar()).text();
				break;

			case NPCPROP_TYPE:
				npcType = pProps.readChars(pProps.readGUChar());
				break;

			case NPCPROP_CURLEVEL:
				// TODO(joey): We don't set the level like this, maybe we should? TBD
				pProps.readChars(pProps.readGUChar());
				break;
#endif

			case NPCPROP_CLASS:
				pProps.readChars(pProps.readGShort());
			break;

			// Location, in pixels, of the npc on the level in 2.3+ clients.
			// Bit 0x0001 controls if it is negative or not.
			// Bits 0xFFFE are the actual value.
			case NPCPROP_X2:
				if (blockPositionUpdates)
				{
					pProps.readGUShort();
					continue;
				}

				x2 = len = pProps.readGUShort();

				// If the first bit is 1, our position is negative.
				x2 >>= 1;
				if ((short)len & 0x0001) x2 = -x2;
				
				// Let pre-2.3+ clients see 2.3+ movement.
				x = (float)x2 / 16.0f;

				hasMoved = true;
				break;

			case NPCPROP_Y2:
				if (blockPositionUpdates)
				{
					pProps.readGUShort();
					continue;
				}

				y2 = len = pProps.readGUShort();

				// If the first bit is 1, our position is negative.
				y2 >>= 1;
				if ((short)len & 0x0001) y2 = -y2;

				// Let pre-2.3+ clients see 2.3+ movement.
				y = (float)y2 / 16.0f;

				hasMoved = true;
				break;

			case NPCPROP_SAVE0: saves[0] = pProps.readGUChar(); break;
			case NPCPROP_SAVE1: saves[1] = pProps.readGUChar(); break;
			case NPCPROP_SAVE2: saves[2] = pProps.readGUChar(); break;
			case NPCPROP_SAVE3: saves[3] = pProps.readGUChar(); break;
			case NPCPROP_SAVE4: saves[4] = pProps.readGUChar(); break;
			case NPCPROP_SAVE5: saves[5] = pProps.readGUChar(); break;
			case NPCPROP_SAVE6: saves[6] = pProps.readGUChar(); break;
			case NPCPROP_SAVE7: saves[7] = pProps.readGUChar(); break;
			case NPCPROP_SAVE8: saves[8] = pProps.readGUChar(); break;
			case NPCPROP_SAVE9: saves[9] = pProps.readGUChar(); break;

			case NPCPROP_GATTRIB1:  gAttribs[0]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB2:  gAttribs[1]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB3:  gAttribs[2]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB4:  gAttribs[3]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB5:  gAttribs[4]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB6:  gAttribs[5]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB7:  gAttribs[6]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB8:  gAttribs[7]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB9:  gAttribs[8]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB10: gAttribs[9]  = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB11: gAttribs[10] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB12: gAttribs[11] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB13: gAttribs[12] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB14: gAttribs[13] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB15: gAttribs[14] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB16: gAttribs[15] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB17: gAttribs[16] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB18: gAttribs[17] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB19: gAttribs[18] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB20: gAttribs[19] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB21: gAttribs[20] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB22: gAttribs[21] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB23: gAttribs[22] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB24: gAttribs[23] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB25: gAttribs[24] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB26: gAttribs[25] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB27: gAttribs[26] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB28: gAttribs[27] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB29: gAttribs[28] = pProps.readChars(pProps.readGUChar()); break;
			case NPCPROP_GATTRIB30: gAttribs[29] = pProps.readChars(pProps.readGUChar()); break;

			default:
			{
				printf("NPC %d (%.2f, %.2f): ", id, x, y);
				printf("Unknown prop: %ud, readPos: %d\n", propId, pProps.readPos());
				for (int i = 0; i < pProps.length(); ++i)
					printf("%02x ", (unsigned char)pProps[i]);
				printf("\n");
			}
			return ret;
		}

		// If a prop changed, adjust its mod time.
		if (propId < NPCPROP_COUNT)
		{
			if (oldProp != getProp(propId))
				modTime[propId] = time(0);
		}

		// Add to ret.
		ret >> (char)propId << getProp(propId, clientVersion);
	}

	if (pForward)
	{
		// Find the level.
		TMap* map = 0;
		if (level != 0) map = level->getMap();

		// Send the props.
		server->sendPacketToLevel(CString() >> (char)PLO_NPCPROPS >> (int)id << ret, map, level, 0, true);
	}

#ifdef V8NPCSERVER
	if (hasMoved) testTouch();
#endif

	return ret;
}

#ifdef V8NPCSERVER

void TNPC::testTouch()
{
	if (level == 0)
		return;

	// 2, 3
	static int touchtestd[] = { 2,1, 0,2, 2,4, 3,2 };
	int dir = sprite % 4;

	TLevelLink *linkTouched = level->isOnLink((int)x + touchtestd[dir*2], (int)y + touchtestd[dir*2+1]);
	if (linkTouched != 0)
	{
		printf("Touched a link! Warp npc!\n");
		TLevel *newLevel = server->getLevel(linkTouched->getNewLevel());
		if (newLevel != 0)
		{
			float newX = (linkTouched->getNewX() == "playerx" ? x : strtofloat(linkTouched->getNewX()));
			float newY = (linkTouched->getNewY() == "playery" ? y : strtofloat(linkTouched->getNewY()));
			this->warpNPC(newLevel, newX, newY);
		}
	}
}

void TNPC::freeScriptResources()
{
	CScriptEngine *scriptEngine = server->getScriptEngine();

	scriptEngine->ClearCache(CScriptEngine::WrapScript<TNPC>(serverScript.text()));

	// Clear any queued actions
	if (!_actions.empty())
	{
		// Unregister npc from any queued event calls
		scriptEngine->UnregisterNpcUpdate(this);

		for (auto it = _actions.begin(); it != _actions.end(); ++it)
			delete *it;
		_actions.clear();
	}

	// Clear timeouts
	if (timeout > 0)
	{
		scriptEngine->UnregisterNpcTimer(this);
		timeout = 0;
	}

	// Clear triggeraction functions
	for (auto it = _triggerActions.begin(); it != _triggerActions.end(); ++it)
		delete it->second;
	_triggerActions.clear();

	// Delete script object
	if (_scriptObject)
		delete _scriptObject;
}

// Set callbacks for triggeractions!
void TNPC::registerTriggerAction(const std::string& action, IScriptFunction *cbFunc)
{
	// clear old callback if it was set
	auto triggerIter = _triggerActions.find(action);
	if (triggerIter != _triggerActions.end())
		delete triggerIter->second;

	// register new trigger
	_triggerActions[action] = cbFunc;
}

void TNPC::queueNpcTrigger(const std::string& action, const std::string& data)
{
	// Check if we respond to this trigger
	auto triggerIter = _triggerActions.find(action);
	if (triggerIter == _triggerActions.end())
		return;

	CScriptEngine *scriptEngine = server->getScriptEngine();

	ScriptAction *scriptAction = scriptEngine->CreateAction("npc.trigger", _scriptObject, triggerIter->second, data);
	_actions.push_back(scriptAction);
	scriptEngine->RegisterNpcUpdate(this);
}

void TNPC::addClassCode(const std::string & className, const std::string & classCode)
{
	classMap[className] = classCode;

	// TODO(joey): We are not readding old join code back into here, just an fyi.

	// Skip servercode, and read client script
	CString script = originalScript;
	script.readString("//#CLIENTSIDE");
	clientScript = script.readString("");

	// Iterate current classes, and add to end of code
	for (auto it = classMap.begin(); it != classMap.end(); ++it)
		clientScript << "\n" << (*it).second;

	// Remove comments and trim the code if specified.
	if (!clientScript.isEmpty())
	{
		clientScriptFormatted = removeComments(clientScript, "\n");
		std::vector<CString> code = clientScriptFormatted.tokenize("\n");
		clientScriptFormatted.clear();
		for (std::vector<CString>::iterator i = code.begin(); i != code.end(); ++i)
			clientScriptFormatted << (*i).trim() << "\xa7";
	}

	// Just a little warning for people who don't know.
	if (clientScriptFormatted.length() > 0x705F)
		printf("WARNING: Clientside script of NPC (%s) exceeds the limit of 28767 bytes.\n", (weaponName.length() != 0 ? weaponName.text() : image.text()));

	// Update prop for players
	this->updatePropModTime(NPCPROP_SCRIPT);
}

void TNPC::setTimeout(int newTimeout)
{
	if (newTimeout > 0)
		server->getScriptEngine()->RegisterNpcTimer(this);
	else if (timeout <= 0)
		server->getScriptEngine()->UnregisterNpcTimer(this);
	timeout = newTimeout;
}

void TNPC::queueNpcAction(const std::string& action, TPlayer *player, bool registerAction)
{
	ScriptAction *scriptAction = 0;
	CScriptEngine *scriptEngine = server->getScriptEngine();

	IScriptWrapped<TPlayer> *playerObject = 0;
	if (player != 0)
	{
		auto playerObject = player->getScriptObject();
		if (playerObject != 0)
			scriptAction = scriptEngine->CreateAction(action, _scriptObject, playerObject);
	}
	
	if (!scriptAction)
		scriptAction = scriptEngine->CreateAction(action, _scriptObject);

	_actions.push_back(scriptAction);
	if (registerAction)
		scriptEngine->RegisterNpcUpdate(this);
}

bool TNPC::runScriptTimer()
{
	// TODO(joey): Scheduled events

	if (timeout > 0)
	{
		timeout--;
		if (timeout == 0)
			queueNpcAction("npc.timeout", 0, true);
	}

	// return value dictates if this gets unregistered from updates
	return (timeout > 0);
}

void TNPC::runScriptEvents()
{
	// TODO(joey): deadlocking if an action invokes another action (ex. an action invoking setprops which invokes playertouchsme)
	
	// iterate over queued actions
	for (auto it = _actions.begin(); it != _actions.end();)
	{
		ScriptAction *action = *it;
		if (action != 0)
		{
			V8ENV_D("Running action: %s\n", action->getAction().c_str());
			action->Invoke();
			it = _actions.erase(it);
			delete action;
		}
		else ++it;
	}

	// Send properties modified by scripts
	if (!propModified.empty())
	{
		if (canWarp)
			testTouch();
		
		time_t newModTime = time(0);

		CString propPacket = CString() >> (char)PLO_NPCPROPS >> (int)id;
		for (auto it = propModified.begin(); it != propModified.end(); ++it)
		{
			char propId = *it;
			modTime[propId] = newModTime;
			propPacket >> (char)(propId) << getProp(propId);
		}
		propModified.clear();

		if (level != nullptr)
			server->sendPacketToLevel(propPacket, level->getMap(), level, nullptr, true);
	}
}

void TNPC::moveNPC(int dx, int dy, double time, int options)
{
	// TODO(joey): Implement options? Or does the client handle them? TBD

	int start_x = (abs(x2) << 1) | (x2 < 0 ? 0x0001 : 0x0000);
	int start_y = (abs(y2) << 1) | (y2 < 0 ? 0x0001 : 0x0000);
	int delta_x = (abs(dx) << 1) | (dx < 0 ? 0x0001 : 0x0000);
	int delta_y = (abs(dy) << 1) | (dy < 0 ? 0x0001 : 0x0000);
	short itime = (short)(time / 0.05);

	setX(x + ((float)dx / 16));
	setY(y + ((float)dy / 16));

	if (level != nullptr)
		server->sendPacketToLevel(CString() >> (char)PLO_MOVE2 >> (int)id >> (short)start_x >> (short)start_y >> (short)delta_x >> (short)delta_y >> (short)itime >> (char)options, level->getMap(), level);
}

void TNPC::resetNPC()
{
	canWarp = false;

	if (!origLevel.isEmpty())
		warpNPC(TLevel::findLevel(origLevel, server), origX, origY);
}

void TNPC::warpNPC(TLevel *pLevel, float pX, float pY)
{
	if (!pLevel)
		return;

	if (level != nullptr)
	{
		// TODO(joey): NPCMOVED needs to be sent to everyone who potentially has this level cached or else the npc
		//  will stay visible when you come back to the level. Should this just be sent to everyone on the server? We do
		//  such for PLO_NPCDEL
		server->sendPacketToLevel(CString() >> (char)PLO_NPCMOVED >> (int) id, level->getMap(), level);

		// Remove the npc from the old level
		level->removeNPC(this);
	}

	// Add to the new level
	pLevel->addNPC(this);
	level = pLevel;

	// Adjust the position of the npc
	x = pX;
	x2 = 16 * pX;

	y = pY;
	y2 = 16 * pY;

	// Send the properties to the players in the new level
	server->sendPacketToLevel(CString() >> (char)PLO_NPCPROPS >> (int)id << getProps(0), level->getMap(), level, 0, true);
}

void TNPC::saveNPC() const
{
	static const char *NL = "\r\n";
	CString fileName = server->getServerPath() << "npcs/npc" << npcName << ".txt";
	CString fileData = CString("GRNPC001") << NL;
	fileData << "NAME " << npcName << NL;
	fileData << "ID " << CString(id) << NL;
	fileData << "TYPE " << npcType << NL;
	fileData << "SCRIPTER " << scripterName << NL;
	fileData << "IMAGE " << image << NL;
	fileData << "STARTLEVEL " << origLevel << NL;
	fileData << "STARTX " << CString(origX) << NL;
	fileData << "STARTY " << CString(origY) << NL;
	if (level)
	{
		fileData << "LEVEL " << level->getLevelName() << NL;
		fileData << "X " << CString(x) << NL;
		fileData << "Y " << CString(y) << NL;
	}
	fileData << "NICK " << nickName << NL;
	fileData << "ANI " << gani << NL;
	fileData << "HP " << CString(power) << NL;
	fileData << "GRALATS " << CString(rupees) << NL;
	fileData << "ARROWS " << CString(darts) << NL;
	fileData << "BOMBS " << CString(bombs) << NL;
	fileData << "GLOVEP " << CString(glovePower) << NL;
	fileData << "SWORDP " << CString(swordPower) << NL;
	fileData << "SHIELDP " << CString(shieldPower) << NL;
	fileData << "HEAD " << headImage << NL;
	fileData << "BODY " << bodyImage << NL;
	fileData << "SWORD " << swordImage << NL;
	fileData << "SHIELD " << shieldImage << NL;
	fileData << "HORSE " << horseImage << NL;
	fileData << "COLORS " << CString((int)colors[0]) << "," << CString((int)colors[1]) << "," << CString((int)colors[2]) << "," << CString((int)colors[3]) << "," << CString((int)colors[4]) << NL;
	fileData << "SPRITE " << CString(sprite) << NL;
	fileData << "AP " << CString(ap) << NL;
	fileData << "TIMEOUT " << CString(timeout / 20) << NL;
	fileData << "LAYER 0" << NL;
	fileData << "SHAPETYPE 0" << NL;
	fileData << "SHAPE " << CString(width) << " " << CString(height) << NL;
	fileData << "SAVEARR " << CString((int)saves[0]) << "," << CString((int)saves[1]) << "," << CString((int)saves[2]) << ","
			 << CString((int)saves[3]) << "," << CString((int)saves[4]) << "," << CString((int)saves[5]) << ","
			 << CString((int)saves[6]) << "," << CString((int)saves[7]) << "," << CString((int)saves[8]) << ","
			 << CString((int)saves[9]) << NL;
	
	for (int i = 0; i < 30; i++)
	{
		if (!gAttribs[i].isEmpty())
			fileData << "ATTR" << std::to_string(i+1) << " " << gAttribs[i] << NL;
	}

	for (auto it = flagList.begin(); it != flagList.end(); ++it)
		fileData << "FLAG " << (*it).first << "=" << (*it).second << NL;

	fileData << "NPCSCRIPT" << NL << originalScript.replaceAll("\n", NL) << NL << "NPCSCRIPTEND" << NL;
	fileData.save(fileName);
}

bool TNPC::loadNPC(const CString& fileName)
{
	// Load file
	CString fileData;
	if (!fileData.load(fileName))
		return false;

	fileData.replaceAllI("\r", "");

	CString headerLine = fileData.readString("\n");
	if (headerLine != "GRNPC001")
		return false;

	// TODO(joey): implement
	//	SAVEARR 0,0,0,0,0,0,0,0,0,0
	//	FLAG car="4,1.6,-0.7,0"
	// 	JOINEDCLASSES staffblock
	//	DONTBLOCK 1
	//	ATTR11 ci_hair-2-4.png [not sure where this ends, i got up to 11 on one]

	CString npcScript, npcLevel;

	// Parse File
	while (fileData.bytesLeft())
	{
		CString curLine = fileData.readString("\n");

		// Find Command
		CString curCommand = curLine.readString();

		// Parse Line
		if (curCommand == "NAME")
			npcName = curLine.readString("").text();
		else if (curCommand == "ID")
			id = strtoint(curLine.readString(""));
		else if (curCommand == "TYPE")
			npcType = curLine.readString("");
		else if (curCommand == "SCRIPTER")
			scripterName = curLine.readString("");
		else if (curCommand == "IMAGE")
			image = curLine.readString("");
		else if (curCommand == "STARTLEVEL")
			origLevel = curLine.readString("");
		else if (curCommand == "STARTX")
			origX = strtofloat(curLine.readString(""));
		else if (curCommand == "STARTY")
			origY = strtofloat(curLine.readString(""));
		else if (curCommand == "LEVEL")
			npcLevel = curLine.readString("");
		else if (curCommand == "X")
			setX(strtofloat(curLine.readString("")));
		else if (curCommand == "Y")
			setY(strtofloat(curLine.readString("")));
		else if (curCommand == "MAPX")
			gmaplevelx = strtoint(curLine.readString(""));
		else if (curCommand == "MAPY")
			gmaplevely = strtoint(curLine.readString(""));
		else if (curCommand == "NICK")
			nickName = curLine.readString("");
		else if (curCommand == "ANI")
			gani = curLine.readString("");
		else if (curCommand == "HP")
			power = (int)(strtofloat(curLine.readString("")) * 2);
		else if (curCommand == "GRALATS")
			rupees = strtoint(curLine.readString(""));
		else if (curCommand == "ARROWS")
			darts = strtoint(curLine.readString(""));
		else if (curCommand == "BOMBS")
			bombs = strtoint(curLine.readString(""));
		else if (curCommand == "GLOVEP")
			glovePower = strtoint(curLine.readString(""));
		else if (curCommand == "SWORDP")
			swordPower = strtoint(curLine.readString(""));
		else if (curCommand == "SHIELDP")
			shieldPower = strtoint(curLine.readString(""));
		else if (curCommand == "HEAD")
			headImage = curLine.readString("");
		else if (curCommand == "BODY")
			bodyImage = curLine.readString("");
		else if (curCommand == "SWORD")
			swordImage = curLine.readString("");
		else if (curCommand == "SHIELD")
			shieldImage = curLine.readString("");
		else if (curCommand == "HORSE")
			horseImage = curLine.readString("");
		else if (curCommand == "SPRITE")
			sprite = strtoint(curLine.readString(""));
		else if (curCommand == "AP")
			ap = strtoint(curLine.readString(""));
		else if (curCommand == "COLORS")
		{
			auto tokens = curLine.readString("").tokenize(",");
			for (int colorIdx = 0; colorIdx < std::min((int) tokens.size(), 5); colorIdx++)
				colors[colorIdx] = strtoint(tokens[colorIdx]);
		}
		else if (curCommand == "SHAPE")
		{
			width = strtoint(curLine.readString(" "));
			height = strtoint(curLine.readString(" "));
		}
		else if (curCommand == "CANWARP")
			canWarp = strtoint(curLine.readString("")) != 0;
		//else if (curCommand == "CANWARP2")
		//	canWarp = strtoint(curLine.readString("")) != 0;
		else if (curCommand == "TIMEOUT")
			timeout = strtoint(curLine.readString("")) * 20;
		else if (curCommand == "FLAG")
		{
			CString flagName = curLine.readString("=");
			CString flagValue = curLine.readString("");
			setFlag(flagName.text(), flagValue);
		}
		else if (curCommand.subString(0, 4) == "ATTR")
		{
			CString attrIdStr = curCommand.subString(5);
			int attrId = strtoint(attrIdStr);
			if (attrId > 0 && attrId < 30)
				gAttribs[attrId - 1] = curLine.readString("");
		}
		else if (curCommand == "NPCSCRIPT")
		{
			curLine = fileData.readString("\n");
			while (fileData.bytesLeft())
			{
				if (curLine == "NPCSCRIPTEND")
					break;

				npcScript << curLine << "\n";
				curLine = fileData.readString("\n");
			}
		}
	}

	setScriptCode(npcScript);

	if (npcLevel.isEmpty())
		npcLevel = origLevel;

	if (!npcLevel.isEmpty())
		level = TLevel::findLevel(npcLevel, server);

	persistNpc = true;
	return true;
}

#endif

CString toWeaponName(const CString& code)
{
	int name_start = code.find("toweapons ");
	if (name_start == -1) return CString();
	name_start += 10;	// 10 = strlen("toweapons ")

	int name_end[2] = { code.find(";", name_start), code.find("}", name_start) };
	if (name_end[0] == -1 && name_end[1] == -1) return CString();

	int name_pos = -1;
	if (name_end[0] == -1) name_pos = name_end[1];
	if (name_end[1] == -1) name_pos = name_end[0];
	if (name_pos == -1) name_pos = (name_end[0] < name_end[1]) ? name_end[0] : name_end[1];

	return code.subString(name_start, name_pos - name_start).trim();
}

CString doJoins(const CString& code, CFileSystem* fs)
{
	CString ret;
	CString c(code);
	std::vector<CString> joinList;

	// Parse out all the joins.
	while (c.bytesLeft())
	{
		ret << c.readString("join ");

		int pos = c.readPos();
		int loc = c.find(";", pos);
		if (loc != -1)
		{
			CString spacecheck = c.subString(pos, loc - pos);
			if (!spacecheck.contains(" \t") && c.bytesLeft())
			{
				ret << ";\xa7";
				joinList.push_back(CString() << c.readString(";") << ".txt");
			}
		}
	}

	// Add the files now.
	for (std::vector<CString>::iterator i = joinList.begin(); i != joinList.end(); ++i)
	{
		//printf("file: %s\n", (*i).text());
		c = fs->load(*i);
		c.removeAllI("\r");
		c.replaceAllI("\n", "\xa7");
		ret << c;
		//ret << removeComments(c, "\xa7");
	}

	return ret;
}
