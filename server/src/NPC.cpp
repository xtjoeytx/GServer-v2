#include "FileSystem.h"
#include "IDebug.h"
#include "IEnums.h"
#include "Level.h"
#include "Map.h"
#include "NPC.h"
#include "Server.h"
#include <math.h>
#include <time.h>
#include <vector>

#ifdef V8NPCSERVER
	#include "CScriptEngine.h"
	#include "Player.h"
#endif

const char __nSavePackets[10] = { 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };
const char __nAttrPackets[30] = { 36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 };

static CString toWeaponName(const CString& code);
static CString doJoins(const CString& code, CFileSystem* fs);

std::string minifyClientCode(const CString& src)
{
	std::string minified;
	if (!src.isEmpty())
	{
		auto tmp = removeComments(src, "\n");

		// Scripts should start with //#CLIENTSIDE since this is client code
		if (tmp.find("//#CLIENTSIDE") != 0)
			minified.append("//#CLIENTSIDE").append("\xa7");

		std::vector<CString> codeLines = tmp.tokenize("\n");
		for (const auto& line: codeLines)
			minified.append(line.trim().toString()).append("\xa7");
	}

	return minified;
}

TNPC::TNPC(const CString& pImage, std::string pScript, float pX, float pY, TServer* pServer, std::shared_ptr<TLevel> pLevel, NPCType type)
	: TNPC(pServer, type)
{
	setX(int(pX * 16));
	setY(int(pY * 16));
	image    = pImage.text();
	curlevel = pLevel;
#ifdef V8NPCSERVER
	origImage = image;
	origX     = x;
	origY     = y;
#endif

	// Keep a copy of the original level for resets
#ifdef V8NPCSERVER
	if (!curlevel.expired())
	{
		origLevel = getLevel()->getLevelName();
	}
#endif

	// TODO: Create plugin hook so NPCServer can acquire/format code.

	// Needs to be called so it creates a script-object
	//if (!pScript.isEmpty())
	setScriptCode(std::move(pScript));
}

TNPC::TNPC(TServer* pServer, NPCType type)
	: server(pServer), npcType(type), blockPositionUpdates(false),
	  x(int(30 * 16)), y(int(30.5 * 16)),
	  hurtX(32.0f), hurtY(32.0f), id(0), rupees(0),
	  darts(0), bombs(0), glovePower(0), bombPower(0), swordPower(0), shieldPower(0),
	  visFlags(1), blockFlags(0), sprite(2), power(0), ap(50),
	  gani("idle")
#ifdef V8NPCSERVER
	  ,
	  _scriptExecutionContext(pServer->getScriptEngine()), origX(x), origY(y), npcDeleteRequested(false), canWarp(NPCWarpType::None), width(32), height(32), timeout(0), _scriptEventsMask(0xFF)
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
	modTime[NPCPROP_IMAGE] = modTime[NPCPROP_SCRIPT] = modTime[NPCPROP_X] = modTime[NPCPROP_Y] = modTime[NPCPROP_VISFLAGS] = modTime[NPCPROP_ID] = modTime[NPCPROP_SPRITE] = modTime[NPCPROP_MESSAGE] = modTime[NPCPROP_GMAPLEVELX] = modTime[NPCPROP_GMAPLEVELY] = modTime[NPCPROP_X2] = modTime[NPCPROP_Y2] = time(0);

	// Needs to be called so it creates a script-object
	setScriptCode("");
}

TNPC::~TNPC()
{
#ifdef V8NPCSERVER
	freeScriptResources();
#endif
}

void TNPC::setScriptCode(std::string pScript)
{
	bool firstExecution = npcScript.empty();

#ifdef V8NPCSERVER
	// Clear any joined code
	classMap.clear();

	if (_scriptObject)
		freeScriptResources();
#endif
	bool gs2default = server->getSettings().getBool("gs2default", false);

	npcScript = SourceCode{ std::move(pScript), gs2default };

	bool levelModificationNPCHack = false;

	// NOTE: since we are not removing comments from the source, any comments at the start of the script
	// interferes with the starts_with check, so a temporary workaround is to check for it within the first 100 characters

	// All code is stored in clientside when building without an npc-server, and split as-expected with the npc-server
#ifdef V8NPCSERVER
	std::string_view npcScriptSearch = npcScript.getServerSide();
#else
	std::string_view npcScriptSearch = npcScript.getClientSide();
#endif

	// See if the NPC sets the level as a sparring zone.
	if (auto level = getLevel(); level)
	{
		if (npcScriptSearch.starts_with("sparringzone") || npcScriptSearch.find("sparringzone\n") < 100)
		{
			level->setSparringZone(true);
			levelModificationNPCHack = true;
		}
		// See if the NPC sets the level as singleplayer.
		else if (npcScriptSearch.starts_with("singleplayer") || npcScriptSearch.find("singleplayer\n") < 100)
		{
			level->setSingleplayer(true);
			levelModificationNPCHack = true;
		}
	}

	// Remove sparringzone / singleplayer from the server script
	if (levelModificationNPCHack)
	{
		// Clearing the entire script
		npcScript.clearServerSide();
	}

	// See if the NPC should block position updates from the level leader.
#ifdef V8NPCSERVER
	blockPositionUpdates = true;
#else
	if (npcScript.getClientGS1().find("//#BLOCKPOSITIONUPDATES") != std::string::npos)
		blockPositionUpdates = true;
#endif

#ifndef V8NPCSERVER
	// Search for toweapons in the clientside code and extract the name of the weapon.
	weaponName = toWeaponName(std::string{ npcScript.getClientGS1() });
#endif

	// Remove comments and trim the code if specified. Also changes line-endings
#ifdef V8NPCSERVER
	updateClientCode();
#else
	auto tmpScript        = doJoins(std::string{ npcScript.getClientGS1() }, server->getFileSystem());
	clientScriptFormatted = minifyClientCode(tmpScript);

	// Just a little warning for people who don't know.
	if (clientScriptFormatted.length() > 0x705F)
		printf("WARNING: Clientside script of NPC (%s) exceeds the limit of 28767 bytes.\n", (weaponName.length() != 0 ? weaponName.text() : image.c_str()));
#endif

#ifdef V8NPCSERVER
	// Compile and execute the script.
	bool executed = server->getScriptEngine()->ExecuteNpc(this);
	if (executed)
	{
		SCRIPTENV_D("SCRIPT COMPILED\n");
		this->queueNpcAction("npc.created");
	}
	else
		SCRIPTENV_D("Could not compile npc script\n");

	// Delete old npc, and send npc to level. Currently only doing this for database npcs, everything else
	//	would need "update level" to take changes.
	auto level = getLevel();
	if (!firstExecution && getType() != NPCType::LEVELNPC && level)
	{
		// this property forces showcharacter, preventing ganis to go back to images
		modTime[NPCPROP_GANI]  = 0;
		modTime[NPCPROP_IMAGE] = time(0);
		//image = "";

		// TODO(joey): refactor
		server->sendPacketToLevelArea(CString() >> (char)PLO_NPCDEL >> (int)getId(), level);

		CString packet = CString() >> (char)PLO_NPCPROPS >> (int)getId() << getProps(0);
		server->sendPacketToLevelArea(packet, level);
	}
#endif
}

std::shared_ptr<TLevel> TNPC::getLevel() const
{
	// TODO: Handle deleted level.
	// Delete level NPCs.

	return curlevel.lock();
}

CString TNPC::getProp(unsigned char pId, int clientVersion) const
{
	auto level = getLevel();
	switch (pId)
	{
		case NPCPROP_IMAGE:
			return CString() >> (char)image.length() << image;

		case NPCPROP_SCRIPT:
			// GS2 support
			if (clientVersion >= CLVER_4_0211)
			{
				// GS1 was disabled after this client version
				if (clientVersion > CLVER_5_07)
					return CString() >> (short)0;

				// If we have bytecode, don't send gs1 script
				if (!npcBytecode.isEmpty())
					return CString() >> (short)0;
			}

			return CString() >> (short)(clientScriptFormatted.length() > 0x3FFF ? 0x3FFF : clientScriptFormatted.length()) << clientScriptFormatted.substr(0, 0x3FFF);

		case NPCPROP_X:
			return CString() >> (char)(x / 8.0);

		case NPCPROP_Y:
			return CString() >> (char)(y / 8.0);

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
			return CString() >> (char)chatMsg.length() << chatMsg;

		case NPCPROP_HURTDXDY:
			return CString() >> (char)((hurtX * 32) + 32) >> (char)((hurtY * 32) + 32);

		case NPCPROP_ID:
			return CString() >> (int)id;

		// Sprite is deprecated and has been replaced by def.gani.
		// Sprite now holds the direction of the npc.  sprite % 4 gives backwards compatibility.
		case NPCPROP_SPRITE:
		{
			if (clientVersion < CLVER_2_1)
				return CString() >> (char)sprite;
			else
				return CString() >> (char)(sprite % 4);
		}

		case NPCPROP_COLORS:
			return CString() >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4];

		case NPCPROP_NICKNAME:
			return CString() >> (char)nickName.length() << nickName;

		case NPCPROP_HORSEIMAGE:
			return CString() >> (char)horseImage.length() << horseImage;

		case NPCPROP_HEADIMAGE:
			return CString() >> (char)(headImage.length() + 100) << headImage;

		case NPCPROP_SAVE0:
		case NPCPROP_SAVE1:
		case NPCPROP_SAVE2:
		case NPCPROP_SAVE3:
		case NPCPROP_SAVE4:
		case NPCPROP_SAVE5:
		case NPCPROP_SAVE6:
		case NPCPROP_SAVE7:
		case NPCPROP_SAVE8:
		case NPCPROP_SAVE9:
			return CString() >> (char)saves[pId - NPCPROP_SAVE0];

		case NPCPROP_ALIGNMENT:
			return CString() >> (char)ap;

		case NPCPROP_IMAGEPART:
			return CString() << imagePart;

		case NPCPROP_BODYIMAGE:
			return CString() >> (char)bodyImage.length() << bodyImage;

		case NPCPROP_GMAPLEVELX:
			return CString() >> (char)(level ? level->getMapX() : 0);

		case NPCPROP_GMAPLEVELY:
			return CString() >> (char)(level ? level->getMapY() : 0);

#ifdef V8NPCSERVER
		case NPCPROP_SCRIPTER:
			return CString() >> (char)npcScripter.length() << npcScripter;

		case NPCPROP_NAME:
			return CString() >> (char)npcName.length() << npcName;

		case NPCPROP_TYPE:
			return CString() >> (char)npcScriptType.length() << npcScriptType;

		case NPCPROP_CURLEVEL:
		{
			CString tmpLevelName = (level ? level->getLevelName() : "");
			return CString() >> (char)tmpLevelName.length() << tmpLevelName;
		}
#endif

		case NPCPROP_CLASS:
		{
			CString classList;

#ifdef V8NPCSERVER
			for (const auto& it: classMap)
				classList << it.first << ",";
#endif

			if (!classList.isEmpty())
				classList.removeI(classList.length() - 1);
			return CString() >> (short)classList.length() << classList;
		}

		case NPCPROP_X2:
		{
			uint16_t val = ((uint16_t)std::abs(x)) << 1;
			if (x < 0)
				val |= 0x0001;
			return CString().writeGShort(val);
		}

		case NPCPROP_Y2:
		{
			uint16_t val = ((uint16_t)std::abs(y)) << 1;
			if (y < 0)
				val |= 0x0001;
			return CString().writeGShort(val);
		}
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
	bool oldcreated = server->getSettings().getBool("oldcreated", "false");
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
		CString oldProp      = getProp(propId);
		//printf( "propId: %d\n", propId );
		switch (propId)
		{
			case NPCPROP_IMAGE:
				visFlags |= NPCVISFLAG_VISIBLE;
				image = pProps.readChars(pProps.readGUChar()).text();
				if (!image.empty() && clientVersion < CLVER_2_1 && getExtension(image).isEmpty())
					image.append(".gif");
				break;

			case NPCPROP_SCRIPT:
				pProps.readChars(pProps.readGUShort());

				// TODO(joey): is this used for putnpcs?
				//clientScript = pProps.readChars(pProps.readGUShort());
				break;

			case NPCPROP_X:
				if (blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				x        = pProps.readGChar() * 8;
				hasMoved = true;
				break;

			case NPCPROP_Y:
				if (blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				y        = pProps.readGChar() * 8;
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
					else
						swordImage = "";
					//swordPower = clip(sp, ((settings->getBool("healswords", false) == true) ? -(settings->getInt("swordlimit", 3)) : 0), settings->getInt("swordlimit", 3));
				}
				swordPower = sp;
				break;
			}

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
					else
						shieldImage = "";
				}
				shieldPower = sp;
				break;
			}

			case NPCPROP_GANI:
			{
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
				gani = pProps.readChars(pProps.readGUChar()).text();
				break;
			}

			case NPCPROP_VISFLAGS:
				visFlags = pProps.readGUChar();
				break;

			case NPCPROP_BLOCKFLAGS:
				blockFlags = pProps.readGUChar();
				break;

			case NPCPROP_MESSAGE:
				chatMsg = pProps.readChars(pProps.readGUChar()).text();
				break;

			case NPCPROP_HURTDXDY:
				hurtX = ((float)(pProps.readGUChar() - 32)) / 32;
				hurtY = ((float)(pProps.readGUChar() - 32)) / 32;
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
				nickName = pProps.readChars(pProps.readGUChar()).text();
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
				pProps.readGUChar();
				break;

			case NPCPROP_GMAPLEVELY:
				pProps.readGUChar();
				break;

			case NPCPROP_SCRIPTER:
				npcScripter = pProps.readChars(pProps.readGUChar());
				break;

			case NPCPROP_NAME:
				npcName = pProps.readChars(pProps.readGUChar()).text();
				break;

			case NPCPROP_TYPE:
				npcScriptType = pProps.readChars(pProps.readGUChar());
				break;

			case NPCPROP_CURLEVEL:
				pProps.readChars(pProps.readGUChar());
				break;

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

				len = pProps.readGUShort();
				x   = (len >> 1);

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					x = -x;

				hasMoved = true;
				break;

			case NPCPROP_Y2:
				if (blockPositionUpdates)
				{
					pProps.readGUShort();
					continue;
				}

				len = pProps.readGUShort();
				y   = (len >> 1);

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					y = -y;

				hasMoved = true;
				break;

			case NPCPROP_SAVE0:
				saves[0] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE1:
				saves[1] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE2:
				saves[2] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE3:
				saves[3] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE4:
				saves[4] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE5:
				saves[5] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE6:
				saves[6] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE7:
				saves[7] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE8:
				saves[8] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE9:
				saves[9] = pProps.readGUChar();
				break;

			case NPCPROP_GATTRIB1:
				gAttribs[0] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB2:
				gAttribs[1] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB3:
				gAttribs[2] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB4:
				gAttribs[3] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB5:
				gAttribs[4] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB6:
				gAttribs[5] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB7:
				gAttribs[6] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB8:
				gAttribs[7] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB9:
				gAttribs[8] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB10:
				gAttribs[9] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB11:
				gAttribs[10] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB12:
				gAttribs[11] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB13:
				gAttribs[12] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB14:
				gAttribs[13] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB15:
				gAttribs[14] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB16:
				gAttribs[15] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB17:
				gAttribs[16] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB18:
				gAttribs[17] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB19:
				gAttribs[18] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB20:
				gAttribs[19] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB21:
				gAttribs[20] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB22:
				gAttribs[21] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB23:
				gAttribs[22] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB24:
				gAttribs[23] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB25:
				gAttribs[24] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB26:
				gAttribs[25] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB27:
				gAttribs[26] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB28:
				gAttribs[27] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB29:
				gAttribs[28] = pProps.readChars(pProps.readGUChar());
				break;
			case NPCPROP_GATTRIB30:
				gAttribs[29] = pProps.readChars(pProps.readGUChar());
				break;

			default:
			{
				printf("NPC %ud (%.2f, %.2f): ", id, (float)x / 16.0f, (float)y / 16.0f);
				printf("Unknown prop: %ud, readPos: %d\n", propId, pProps.readPos());
				for (int i = 0; i < pProps.length(); ++i)
					printf("%02x ", (unsigned char)pProps[i]);
				printf("\n");
				return ret;
			}
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
		// Send the props.
		server->sendPacketToLevelArea(CString() >> (char)PLO_NPCPROPS >> (int)id << ret, curlevel);
	}

#ifdef V8NPCSERVER
	if (hasMoved) testTouch();
#endif

	return ret;
}

#ifdef V8NPCSERVER

void TNPC::testForLinks()
{
	auto level = getLevel();
	if (level == nullptr) return;

	// Overworld links
	if (auto map = level->getMap(); map)
	{
		// Gmaps are treated as one large map, and so (local?) npcs can freely walk
		// across maps without canwarp being enabled (source: post=1193766)
		if (map->isGmap() || canWarp != NPCWarpType::None)
		{
			int gmapX = x + 1024 * level->getMapX();
			int gmapY = y + 1024 * level->getMapY();
			int mapx  = gmapX / 1024;
			int mapy  = gmapY / 1024;

			if (level->getMapX() != mapx || level->getMapY() != mapy)
			{
				auto newLevel = server->getLevel(map->getLevelAt(mapx, mapy));
				if (newLevel != nullptr)
				{
					this->warpNPC(newLevel, gmapX % 1024, gmapY % 1024);
					return;
				}
			}
		}
	}

	if (canWarp == NPCWarpType::AllLinks)
	{
		static const int touchtestd[] = { 2, 1, 0, 2, 2, 4, 3, 2 };

		int dir = sprite % 4;

		auto linkTouched = level->getLink((int)(x / 16) + touchtestd[dir * 2], (int)(y / 16) + touchtestd[dir * 2 + 1]);
		if (linkTouched)
		{
			auto newLevel = server->getLevel(linkTouched.value()->getNewLevel().toString());
			if (newLevel != 0)
			{
				int newX = (linkTouched.value()->getNewX() == "playerx" ? x : int(16.0 * strtofloat(linkTouched.value()->getNewX())));
				int newY = (linkTouched.value()->getNewY() == "playery" ? y : int(16.0 * strtofloat(linkTouched.value()->getNewY())));
				this->warpNPC(newLevel, newX, newY);
			}
		}
	}
}

void TNPC::testTouch()
{
	if (curlevel.expired())
		return;

	testForLinks();
}

void TNPC::freeScriptResources()
{
	CScriptEngine* scriptEngine = server->getScriptEngine();

	// Clear cached script
	if (!npcScript.getServerSide().empty())
		scriptEngine->ClearCache<TNPC>(npcScript.getServerSide());

	// Reset script execution
	_scriptExecutionContext.resetExecution();

	// Clear any queued actions
	scriptEngine->UnregisterNpcUpdate(this);

	// Clear timeouts & scheduled events
	scriptEngine->UnregisterNpcTimer(this);
	_scriptTimers.clear();
	timeout = 0;

	// Clear triggeraction functions
	for (auto& _triggerAction: _triggerActions)
		delete _triggerAction.second;
	_triggerActions.clear();

	// Delete script object
	if (_scriptObject)
		_scriptObject.reset();
}

// Set callbacks for triggeractions!
void TNPC::registerTriggerAction(const std::string& action, IScriptFunction* cbFunc)
{
	// clear old callback if it was set
	auto triggerIter = _triggerActions.find(action);
	if (triggerIter != _triggerActions.end())
		delete triggerIter->second;

	// register new trigger
	_triggerActions[action] = cbFunc;
}

void TNPC::queueNpcTrigger(const std::string& action, TPlayer* player, const std::string& data)
{
	assert(_scriptObject);

	// Check if we respond to this trigger
	auto triggerIter = _triggerActions.find(action);
	if (triggerIter == _triggerActions.end())
		return;

	CScriptEngine* scriptEngine = server->getScriptEngine();

	IScriptObject<TPlayer>* playerObject = nullptr;
	if (player != nullptr)
		playerObject = player->getScriptObject();

	if (playerObject)
	{
		_scriptExecutionContext.addAction(scriptEngine->CreateAction("npc.trigger", getScriptObject(), triggerIter->second, playerObject, data));
	}
	else
	{
		_scriptExecutionContext.addAction(scriptEngine->CreateAction("npc.trigger", getScriptObject(), triggerIter->second, nullptr, data));
	}

	scriptEngine->RegisterNpcUpdate(this);
}

TScriptClass* TNPC::joinClass(const std::string& className)
{
	auto found = classMap.find(className);
	if (found != classMap.end())
		return nullptr;

	auto classObj = server->getClass(className);
	if (!classObj)
		return nullptr;

	classMap[className] = classObj->getSource().getClientGS1();
	updateClientCode();
	modTime[NPCPROP_CLASS] = time(0);
	return classObj;
}

void TNPC::updateClientCode()
{
	// Skip servercode, and read client script
	CString tmpScript = std::string{ npcScript.getClientGS1() };

	// Iterate current classes, and add to end of code
	for (auto& it: classMap)
		tmpScript << "\n"
				  << it.second;

	// Remove comments and trim the code if specified.
	clientScriptFormatted = minifyClientCode(tmpScript);

	// Just a little warning for people who don't know.
	if (clientScriptFormatted.length() > 0x705F)
		printf("WARNING: Clientside script of NPC (%s) exceeds the limit of 28767 bytes.\n", (weaponName.length() != 0 ? weaponName.text() : image.c_str()));

	// Compile gs2
	if (!npcScript.getClientGS2().empty())
	{
		// Compile gs2 code
		server->compileGS2Script(this,
								 [this](const CompilerResponse& response)
								 {
									 if (response.success)
									 {
										 auto& byteCode = response.bytecode;
										 npcBytecode.clear(byteCode.length());
										 npcBytecode.write((const char*)byteCode.buffer(), (int)byteCode.length());
									 }
								 });
	}

	// Update prop for players
	this->updatePropModTime(NPCPROP_SCRIPT);
}

void TNPC::setTimeout(int newTimeout)
{
	timeout = newTimeout;

	if (hasTimerUpdates())
		server->getScriptEngine()->RegisterNpcTimer(this);
	else
		server->getScriptEngine()->UnregisterNpcTimer(this);
}

void TNPC::queueNpcAction(const std::string& action, TPlayer* player, bool registerAction)
{
	assert(_scriptObject);

	CScriptEngine* scriptEngine = server->getScriptEngine();

	IScriptObject<TPlayer>* playerObject = nullptr;
	if (player != nullptr)
		playerObject = player->getScriptObject();

	if (playerObject)
	{
		_scriptExecutionContext.addAction(scriptEngine->CreateAction(action, getScriptObject(), playerObject));
	}
	else
	{
		_scriptExecutionContext.addAction(scriptEngine->CreateAction(action, getScriptObject()));
	}

	if (registerAction)
		scriptEngine->RegisterNpcUpdate(this);
}

bool TNPC::runScriptTimer()
{
	// TODO(joey): Scheduled events, pass in delta, use milliseconds as an integer

	if (timeout > 0)
	{
		timeout--;
		if (timeout == 0)
			queueNpcAction("npc.timeout", 0, true);
	}

	// scheduled events
	bool queued = false;
	for (auto it = _scriptTimers.begin(); it != _scriptTimers.end();)
	{
		ScriptEventTimer& timer = *it;
		timer.timer--;
		if (timer.timer == 0)
		{
			_scriptExecutionContext.addAction(timer.action);
			it     = _scriptTimers.erase(it);
			queued = true;
			continue;
		}

		++it;
	}

	// Register for npc updates
	if (queued)
		server->getScriptEngine()->RegisterNpcUpdate(this);

	// return value dictates if this gets unregistered from timer updates
	return hasTimerUpdates();
}

NPCEventResponse TNPC::runScriptEvents()
{
	bool hasActions = false;
	if (!npcDeleteRequested)
		hasActions = _scriptExecutionContext.runExecution(); // Returns true if we still have actions to run

	// Send properties modified by scripts
	if (!propModified.empty())
	{
		if (propModified.contains(NPCPROP_X2) || propModified.contains(NPCPROP_Y2))
		{
			testTouch();
		}

		time_t newModTime = time(0);

		CString propPacket = CString() >> (char)PLO_NPCPROPS >> (int)id;
		for (unsigned char propId: propModified)
		{
			modTime[propId] = newModTime;
			propPacket >> (char)propId << getProp(propId);
		}
		propModified.clear();

		if (!curlevel.expired())
			server->sendPacketToLevelArea(propPacket, curlevel);
	}

	if (npcDeleteRequested)
	{
		npcDeleteRequested = false;
		return NPCEventResponse::Delete;
	}

	return (hasActions ? NPCEventResponse::PendingEvents : NPCEventResponse::NoEvents);
}

CString TNPC::getVariableDump()
{
	static const char* const propNames[NPCPROP_COUNT] = {
		"image", "script", "x", "y", "power", "rupees",
		"arrows", "bombs", "glovepower", "bombpower", "sword", "shield",
		"animation", "visibility flags", "blocking flags", "message", "hurtdxdy",
		"id", "sprite", "colors", "nickname", "horse", "head",
		"save[0]", "save[1]", "save[2]", "save[3]", "save[4]", "save[5]",
		"save[6]", "save[7]", "save[8]", "save[9]",
		"alignment", "imagepart", "body",
		"ganiattr1", "ganiattr2", "ganiattr3", "ganiattr4", "ganiattr5",
		"mapx", "mapy", "UNKNOWN43", "ganiattr6", "ganiattr7", "ganiattr8",
		"ganiattr9", "UNKNOWN48", "scripter", "name", "type", "level",
		"ganiattr10", "ganiattr11", "ganiattr12", "ganiattr13", "ganiattr14",
		"ganiattr15", "ganiattr16", "ganiattr17", "ganiattr18", "ganiattr19",
		"ganiattr20", "ganiattr21", "ganiattr22", "ganiattr23", "ganiattr24",
		"ganiattr25", "ganiattr26", "ganiattr27", "ganiattr28", "ganiattr29",
		"ganiattr30", "joinedclasses", "xprecise", "yprecise"
	};

	static const char propList[] = {
		NPCPROP_ID, NPCPROP_IMAGE, NPCPROP_SCRIPT, NPCPROP_VISFLAGS, NPCPROP_BLOCKFLAGS,
		NPCPROP_HEADIMAGE, NPCPROP_BODYIMAGE, NPCPROP_SWORDIMAGE, NPCPROP_SHIELDIMAGE,
		NPCPROP_NICKNAME, NPCPROP_SPRITE, NPCPROP_GANI,
		NPCPROP_GATTRIB1, NPCPROP_GATTRIB2, NPCPROP_GATTRIB3, NPCPROP_GATTRIB4, NPCPROP_GATTRIB5,
		NPCPROP_GATTRIB6, NPCPROP_GATTRIB7, NPCPROP_GATTRIB8, NPCPROP_GATTRIB9, NPCPROP_GATTRIB10,
		NPCPROP_GATTRIB11, NPCPROP_GATTRIB12, NPCPROP_GATTRIB13, NPCPROP_GATTRIB14, NPCPROP_GATTRIB15,
		NPCPROP_GATTRIB16, NPCPROP_GATTRIB17, NPCPROP_GATTRIB18, NPCPROP_GATTRIB19, NPCPROP_GATTRIB20,
		NPCPROP_GATTRIB21, NPCPROP_GATTRIB22, NPCPROP_GATTRIB23, NPCPROP_GATTRIB24, NPCPROP_GATTRIB25,
		NPCPROP_GATTRIB26, NPCPROP_GATTRIB27, NPCPROP_GATTRIB28, NPCPROP_GATTRIB29, NPCPROP_GATTRIB30,
		NPCPROP_SAVE0, NPCPROP_SAVE1, NPCPROP_SAVE2, NPCPROP_SAVE3, NPCPROP_SAVE4,
		NPCPROP_SAVE5, NPCPROP_SAVE6, NPCPROP_SAVE7, NPCPROP_SAVE8, NPCPROP_SAVE9,
		NPCPROP_GMAPLEVELX, NPCPROP_GMAPLEVELY, NPCPROP_X2, NPCPROP_Y2
	};

	const int propsCount = sizeof(propList) / sizeof(char);

	// Create the npc dump...
	CString npcDump;
	CString npcNameStr = npcName;
	if (npcNameStr.isEmpty())
		npcNameStr = CString() << "npcs[" << CString(id) << "]";

	auto level = getLevel();

	npcDump << "Variables dump from npc " << npcNameStr << "\n\n";
	if (!npcScriptType.isEmpty()) npcDump << npcNameStr << ".type: " << npcScriptType << "\n";
	if (!npcScripter.isEmpty()) npcDump << npcNameStr << ".scripter: " << npcScripter << "\n";
	if (level) npcDump << npcNameStr << ".level: " << level->getLevelName() << "\n";

	npcDump << "\nAttributes:\n";
	for (int propId: propList)
	{
		CString prop = getProp(propId);

		switch (propId)
		{
			case NPCPROP_ID:
			{
				int id = prop.readGUInt();
				npcDump << npcNameStr << "." << propNames[propId] << ": " << CString((int)id) << "\n";
				break;
			}

			case NPCPROP_SCRIPT:
			{
				int len = prop.readGUShort();
				if (len > 0)
					npcDump << npcNameStr << "." << propNames[propId] << ": size: " << CString(len) << "\n";
				break;
			}

			case NPCPROP_SWORDIMAGE:
			{
				int power = prop.readGUChar();
				CString image;
				if (power > 30)
				{
					image = prop.readChars(prop.readGUChar());
					power -= 30;
				}
				else if (power > 0)
					image = CString() << "sword" << CString(power) << ".png";

				if (!image.isEmpty())
					npcDump << npcNameStr << "." << propNames[propId] << ": " << image << " (" << CString(power) << ")\n";

				break;
			}

			case NPCPROP_SHIELDIMAGE:
			{
				int power = prop.readGUChar();
				CString image;
				if (power > 10)
				{
					image = prop.readChars(prop.readGUChar());
					power -= 10;
				}
				else if (power > 0)
					image = CString() << "shield" << CString(power) << ".png";

				if (!image.isEmpty())
					npcDump << npcNameStr << "." << propNames[propId] << ": " << image << " (" << CString(power) << ")\n";

				break;
			}

			case NPCPROP_VISFLAGS:
			{
				char value = prop.readGUChar();
				npcDump << npcNameStr << "." << propNames[propId] << ": ";
				npcDump << ((value & NPCVISFLAG_VISIBLE) ? "visible" : "hidden");
				if (value & NPCVISFLAG_DRAWOVERPLAYER)
					npcDump << ", drawoverplayer";
				if (value & NPCVISFLAG_DRAWUNDERPLAYER)
					npcDump << ", drawunderplayer";
				npcDump << "\n";

				break;
			}

			case NPCPROP_BLOCKFLAGS:
			{
				char value = prop.readGUChar();
				if (value & NPCBLOCKFLAG_NOBLOCK)
					npcDump << npcNameStr << "." << propNames[propId] << ": "
							<< "dontblock"
							<< "\n";
				break;
			}

			case NPCPROP_SPRITE:
			{
				char value = prop.readGUChar();
				if (value > 0)
					npcDump << npcNameStr << "." << propNames[propId] << ": " << CString((int)value) << "\n";
				break;
			}

			case NPCPROP_GMAPLEVELX:
			case NPCPROP_GMAPLEVELY:
			{
				char value = prop.readGUChar();
				if (level && level->getMap() != nullptr)
					npcDump << npcNameStr << "." << propNames[propId] << ": " << CString((int)value) << "\n";
				break;
			}

			case NPCPROP_IMAGE:
			case NPCPROP_GANI:
			case NPCPROP_MESSAGE:
			case NPCPROP_NICKNAME:
			case NPCPROP_HORSEIMAGE:
			case NPCPROP_HEADIMAGE:
			case NPCPROP_BODYIMAGE:
			case NPCPROP_SCRIPTER:
			case NPCPROP_NAME:
			case NPCPROP_TYPE:
			case NPCPROP_CURLEVEL:
			case NPCPROP_GATTRIB1:
			case NPCPROP_GATTRIB2:
			case NPCPROP_GATTRIB3:
			case NPCPROP_GATTRIB4:
			case NPCPROP_GATTRIB5:
			case NPCPROP_GATTRIB6:
			case NPCPROP_GATTRIB7:
			case NPCPROP_GATTRIB8:
			case NPCPROP_GATTRIB9:
			case NPCPROP_GATTRIB10:
			case NPCPROP_GATTRIB11:
			case NPCPROP_GATTRIB12:
			case NPCPROP_GATTRIB13:
			case NPCPROP_GATTRIB14:
			case NPCPROP_GATTRIB15:
			case NPCPROP_GATTRIB16:
			case NPCPROP_GATTRIB17:
			case NPCPROP_GATTRIB18:
			case NPCPROP_GATTRIB19:
			case NPCPROP_GATTRIB20:
			case NPCPROP_GATTRIB21:
			case NPCPROP_GATTRIB22:
			case NPCPROP_GATTRIB23:
			case NPCPROP_GATTRIB24:
			case NPCPROP_GATTRIB25:
			case NPCPROP_GATTRIB26:
			case NPCPROP_GATTRIB27:
			case NPCPROP_GATTRIB28:
			case NPCPROP_GATTRIB29:
			case NPCPROP_GATTRIB30:
			{
				int len = prop.readGUChar();
				if (len > 0)
				{
					CString value = prop.readChars(len).trim();
					if (!value.isEmpty())
						npcDump << npcNameStr << "." << propNames[propId] << ": " << value << "\n";
				}
				break;
			}

			case NPCPROP_SAVE0:
			case NPCPROP_SAVE1:
			case NPCPROP_SAVE2:
			case NPCPROP_SAVE3:
			case NPCPROP_SAVE4:
			case NPCPROP_SAVE5:
			case NPCPROP_SAVE6:
			case NPCPROP_SAVE7:
			case NPCPROP_SAVE8:
			case NPCPROP_SAVE9:
			{
				unsigned char saveValue = prop.readGUChar();
				if (saveValue > 0)
					npcDump << npcNameStr << "." << propNames[propId] << ": " << CString((int)saveValue) << "\n";
				break;
			}

			case NPCPROP_X2:
			case NPCPROP_Y2:
			{
				short pos = prop.readGUShort();
				pos       = ((pos & 0x0001) ? -(pos >> 1) : pos >> 1);
				npcDump << npcNameStr << "." << propNames[propId] << ": " << CString((double)pos / 16.0) << "\n";
				break;
			}

			default:
				continue;
		}
	}

	if (timeout > 0)
		npcDump << npcNameStr << ".timeout: " << CString((float)(timeout * 0.05f)) << "\n";

	std::pair<unsigned int, double> executionData = _scriptExecutionContext.getExecutionData();
	npcDump << npcNameStr << ".scripttime (in the last min): " << CString(executionData.second) << "\n";
	npcDump << npcNameStr << ".scriptcalls: " << CString(executionData.first) << "\n";

	if (!flagList.empty())
	{
		npcDump << "\nnpc.Flags:\n";
		for (auto it = flagList.begin(); it != flagList.end(); ++it)
			npcDump << npcNameStr << ".flags[\"" << (*it).first << "\"]: " << (*it).second << "\n";
	}

	return npcDump;
}

bool TNPC::deleteNPC()
{
	if (getType() == NPCType::PUTNPC)
	{
		npcDeleteRequested = true;
		registerNpcUpdates();
	}

	return npcDeleteRequested;
}

void TNPC::reloadNPC()
{
	setScriptCode(npcScript.getSource());
}

void TNPC::resetNPC()
{
	// TODO(joey): reset script execution, clear flags.. unsure what else gets reset. TBD
	canWarp                = NPCWarpType::None;
	modTime[NPCPROP_IMAGE] = modTime[NPCPROP_SCRIPT] = modTime[NPCPROP_X] = modTime[NPCPROP_Y] = modTime[NPCPROP_VISFLAGS] = modTime[NPCPROP_ID] = modTime[NPCPROP_SPRITE] = modTime[NPCPROP_MESSAGE] = modTime[NPCPROP_GMAPLEVELX] = modTime[NPCPROP_GMAPLEVELY] = modTime[NPCPROP_X2] = modTime[NPCPROP_Y2] = time(0);
	headImage                                                                                                                                                                                                                                                                                                 = "";
	bodyImage                                                                                                                                                                                                                                                                                                 = "";
	image                                                                                                                                                                                                                                                                                                     = "";
	gani                                                                                                                                                                                                                                                                                                      = "idle";

	// Reset script execution
	setScriptCode(npcScript.getSource());

	if (!origLevel.isEmpty())
	{
		warpNPC(TLevel::findLevel(origLevel, server), origX, origY);
	}
}

void TNPC::moveNPC(int dx, int dy, double time, int options)
{
	// TODO(joey): Implement options? Or does the client handle them? TBD
	//	- If we want function callbacks we will need to handle time, can schedule an event once that is implemented

	int start_x = ((uint16_t)std::abs(x) << 1) | (x < 0 ? 0x0001 : 0x0000);
	int start_y = ((uint16_t)std::abs(y) << 1) | (y < 0 ? 0x0001 : 0x0000);
	int delta_x = ((uint16_t)std::abs(dx) << 1) | (dx < 0 ? 0x0001 : 0x0000);
	int delta_y = ((uint16_t)std::abs(dy) << 1) | (dy < 0 ? 0x0001 : 0x0000);
	short itime = (short)(time / 0.05);

	setX(x + dx);
	setY(y + dy);

	if (!curlevel.expired())
		server->sendPacketToLevelArea(CString() >> (char)PLO_MOVE2 >> (int)id >> (short)start_x >> (short)start_y >> (short)delta_x >> (short)delta_y >> (short)itime >> (char)options, curlevel);

	if (isWarpable())
		testTouch();
}

void TNPC::warpNPC(std::shared_ptr<TLevel> pLevel, int pX, int pY)
{
	if (!pLevel)
		return;

	auto level = getLevel();
	if (level != nullptr)
	{
		// TODO(joey): NPCMOVED needs to be sent to everyone who potentially has this level cached or else the npc
		//  will stay visible when you come back to the level. Should this just be sent to everyone on the server? We do
		//  such for PLO_NPCDEL
		server->sendPacketToType(PLTYPE_ANYPLAYER, CString() >> (char)PLO_NPCMOVED >> (int)id);

		// Remove the npc from the old level
		level->removeNPC(id);
	}

	// Add to the new level
	pLevel->addNPC(id);
	level = pLevel;

	// Adjust the position of the npc
	x = pX;
	y = pY;

	updatePropModTime(NPCPROP_CURLEVEL);
	updatePropModTime(NPCPROP_GMAPLEVELX);
	updatePropModTime(NPCPROP_GMAPLEVELY);
	updatePropModTime(NPCPROP_X2);
	updatePropModTime(NPCPROP_Y2);

	// Send the properties to the players in the new level
	server->sendPacketToLevelArea(CString() >> (char)PLO_NPCPROPS >> (int)id << getProps(0), level);

	if (!npcName.empty())
		server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCADD >> (int)id >> (char)NPCPROP_CURLEVEL << getProp(NPCPROP_CURLEVEL));

	// Queue event
	this->queueNpcAction("npc.warped");
}

void TNPC::saveNPC()
{
	// TODO(joey): save localnpcs aka putnpcs to a localnpcs folder, as of now
	// we are only saving database npcs.

	if (getType() != NPCType::DBNPC)
	{
		return;
	}

	// TODO(joey): check if properties have been modified before deciding to save
	// enumerate scriptObject variables, to save into file and load later..?

	// Clean up old samples
	//_scriptExecutionContext.getExecutionData();

	/*
	CString saveDir;
	CString saveName;
	if (getType() == NPCType::DBNPC)
	{
		saveDir = "npcs/";
		saveName = npcName;
	}
	else if (getType() == NPCType::PUTNPC)
	{
		saveDir = "npcprops/";
		saveName = CString("localnpc_");

		if (level && level->getMap())
		{
			saveName << removeExtension(origLevel) << "_" << level->getMapX() << "_" << level->getMapY();
		}
	}

	// Level npcs shouldn't be saved
	if (saveDir.isEmpty())
	{
		return;
	}
	*/

	auto level = getLevel();

	static const char* NL = "\r\n";
	CString fileName      = server->getServerPath() << "npcs/npc" << npcName << ".txt";
	CString fileData      = CString("GRNPC001") << NL;
	fileData << "NAME " << npcName << NL;
	fileData << "ID " << CString(id) << NL;
	fileData << "TYPE " << npcScriptType << NL;
	fileData << "SCRIPTER " << npcScripter << NL;
	fileData << "IMAGE " << image << NL;
	fileData << "STARTLEVEL " << origLevel << NL;
	fileData << "STARTX " << CString((float)origX / 16.0) << NL;
	fileData << "STARTY " << CString((float)origY / 16.0) << NL;
	if (level)
	{
		fileData << "LEVEL " << level->getLevelName() << NL;
		fileData << "X " << CString((float)x / 16.0) << NL;
		fileData << "Y " << CString((float)y / 16.0) << NL;
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

	if (blockFlags & NPCBLOCKFLAG_NOBLOCK)
		fileData << "DONTBLOCK 1" << NL;

	fileData << "SAVEARR " << CString((int)saves[0]) << "," << CString((int)saves[1]) << "," << CString((int)saves[2]) << ","
			 << CString((int)saves[3]) << "," << CString((int)saves[4]) << "," << CString((int)saves[5]) << ","
			 << CString((int)saves[6]) << "," << CString((int)saves[7]) << "," << CString((int)saves[8]) << ","
			 << CString((int)saves[9]) << NL;

	for (int i = 0; i < 30; i++)
	{
		if (!gAttribs[i].isEmpty())
			fileData << "ATTR" << std::to_string(i + 1) << " " << gAttribs[i] << NL;
	}

	for (auto it = flagList.begin(); it != flagList.end(); ++it)
		fileData << "FLAG " << (*it).first << "=" << (*it).second << NL;

	fileData << "NPCSCRIPT" << NL << CString(npcScript.getSource()).replaceAll("\n", NL);
	if (fileData[fileData.length() - 1] != '\n')
		fileData << NL;
	fileData << "NPCSCRIPTEND" << NL;
	fileData.save(fileName);
}

bool TNPC::loadNPC(const CString& fileName)
{
	// Load file
	CString fileData;
	if (!fileData.load(fileName))
		return false;

	fileData.removeAllI("\r");

	CString headerLine = fileData.readString("\n");
	if (headerLine != "GRNPC001")
		return false;

	// TODO(joey): implement
	// 	JOINEDCLASSES staffblock (not really needed for us, so)
	//	DONTBLOCK 1
	//	modtime is not being updated for these properties

	time_t updateTime = time(0);
	CString npcLevel;
	std::string script;

	CString propPacket;

	// Parse File
	while (fileData.bytesLeft())
	{
		CString curLine = fileData.readString("\n");

		// Find Command
		CString curCommand = curLine.readString();

		// Parse Line
		if (curCommand == "NAME")
		{
			npcName               = curLine.readString("").text();
			modTime[NPCPROP_NAME] = updateTime;
		}
		else if (curCommand == "ID")
			id = strtoint(curLine.readString(""));
		else if (curCommand == "TYPE")
			npcScriptType = curLine.readString("");
		else if (curCommand == "SCRIPTER")
		{
			npcScripter               = curLine.readString("");
			modTime[NPCPROP_SCRIPTER] = updateTime;
		}
		else if (curCommand == "IMAGE")
		{
			image                  = curLine.readString("").text();
			modTime[NPCPROP_IMAGE] = updateTime;
		}
		else if (curCommand == "STARTLEVEL")
			origLevel = curLine.readString("");
		else if (curCommand == "STARTX")
			origX = int(strtofloat(curLine.readString("")) * 16.0);
		else if (curCommand == "STARTY")
			origY = int(strtofloat(curLine.readString("")) * 16.0);
		else if (curCommand == "LEVEL")
			npcLevel = curLine.readString("");
		else if (curCommand == "X")
			setX(int(strtofloat(curLine.readString("")) * 16.0));
		else if (curCommand == "Y")
			setY(int(strtofloat(curLine.readString("")) * 16.0));
		else if (curCommand == "MAPX")
		{
			//gmaplevelx = strtoint(curLine.readString(""));
			modTime[NPCPROP_GMAPLEVELX] = updateTime;
		}
		else if (curCommand == "MAPY")
		{
			//gmaplevely = strtoint(curLine.readString(""));
			modTime[NPCPROP_GMAPLEVELY] = updateTime;
		}
		else if (curCommand == "NICK")
		{
			nickName                  = curLine.readString("").text();
			modTime[NPCPROP_NICKNAME] = updateTime;
		}
		else if (curCommand == "ANI")
		{
			gani                  = curLine.readString("").text();
			modTime[NPCPROP_GANI] = updateTime;
		}
		else if (curCommand == "HP")
		{
			power                  = (int)(strtofloat(curLine.readString("")) * 2);
			modTime[NPCPROP_POWER] = updateTime;
		}
		else if (curCommand == "GRALATS")
		{
			rupees                  = strtoint(curLine.readString(""));
			modTime[NPCPROP_RUPEES] = updateTime;
		}
		else if (curCommand == "ARROWS")
		{
			darts                   = strtoint(curLine.readString(""));
			modTime[NPCPROP_ARROWS] = updateTime;
		}
		else if (curCommand == "BOMBS")
		{
			bombs                  = strtoint(curLine.readString(""));
			modTime[NPCPROP_BOMBS] = updateTime;
		}
		else if (curCommand == "GLOVEP")
		{
			glovePower                  = strtoint(curLine.readString(""));
			modTime[NPCPROP_GLOVEPOWER] = updateTime;
		}
		else if (curCommand == "SWORDP")
		{
			swordPower                  = strtoint(curLine.readString(""));
			modTime[NPCPROP_SWORDIMAGE] = updateTime;
		}
		else if (curCommand == "SHIELDP")
		{
			shieldPower                  = strtoint(curLine.readString(""));
			modTime[NPCPROP_SHIELDIMAGE] = updateTime;
		}
		else if (curCommand == "HEAD")
		{
			headImage                  = curLine.readString("");
			modTime[NPCPROP_HEADIMAGE] = updateTime;
		}
		else if (curCommand == "BODY")
		{
			bodyImage                  = curLine.readString("");
			modTime[NPCPROP_BODYIMAGE] = updateTime;
		}
		else if (curCommand == "SWORD")
		{
			swordImage                  = curLine.readString("");
			modTime[NPCPROP_SWORDIMAGE] = updateTime;
		}
		else if (curCommand == "SHIELD")
		{
			shieldImage                  = curLine.readString("");
			modTime[NPCPROP_SHIELDIMAGE] = updateTime;
		}
		else if (curCommand == "HORSE")
		{
			horseImage                  = curLine.readString("");
			modTime[NPCPROP_HORSEIMAGE] = updateTime;
		}
		else if (curCommand == "SPRITE")
		{
			sprite                  = strtoint(curLine.readString(""));
			modTime[NPCPROP_SPRITE] = updateTime;
		}
		else if (curCommand == "AP")
		{
			ap                         = strtoint(curLine.readString(""));
			modTime[NPCPROP_ALIGNMENT] = updateTime;
		}
		else if (curCommand == "COLORS")
		{
			auto tokens = curLine.readString("").tokenize(",");
			for (int idx = 0; idx < std::min((int)tokens.size(), 5); idx++)
				colors[idx] = strtoint(tokens[idx]);
			modTime[NPCPROP_COLORS] = updateTime;
		}
		else if (curCommand == "SAVEARR")
		{
			auto tokens = curLine.readString("").tokenize(",");
			for (int idx = 0; idx < std::min(tokens.size(), sizeof(saves) / sizeof(unsigned char)); idx++)
			{
				saves[idx]                   = (unsigned char)strtoint(tokens[idx]);
				modTime[NPCPROP_SAVE0 + idx] = updateTime;
			}
		}
		else if (curCommand == "SHAPE")
		{
			width  = strtoint(curLine.readString(" "));
			height = strtoint(curLine.readString(" "));
		}
		else if (curCommand == "CANWARP")
		{
			canWarp = strtoint(curLine.readString("")) != 0 ? NPCWarpType::AllLinks : canWarp;
		}
		else if (curCommand == "CANWARP2")
		{
			canWarp = strtoint(curLine.readString("")) != 0 ? NPCWarpType::OverworldLinks : canWarp;
		}
		else if (curCommand == "TIMEOUT")
			timeout = strtoint(curLine.readString("")) * 20;
		else if (curCommand == "FLAG")
		{
			CString flagName  = curLine.readString("=");
			CString flagValue = curLine.readString("");
			setFlag(flagName.text(), flagValue);
		}
		else if (curCommand.subString(0, 4) == "ATTR")
		{
			CString attrIdStr = curCommand.subString(5);
			int attrId        = strtoint(attrIdStr);
			if (attrId > 0 && attrId < 30)
			{
				int idx                      = attrId - 1;
				gAttribs[idx]                = curLine.readString("");
				modTime[__nAttrPackets[idx]] = updateTime;
			}
		}
		else if (curCommand == "NPCSCRIPT")
		{
			do {
				curLine = fileData.readString("\n");
				if (curLine == "NPCSCRIPTEND")
					break;

				script.append(curLine.text(), curLine.length()).append(1, '\n');
			}
			while (fileData.bytesLeft());

			modTime[NPCPROP_SCRIPT] = updateTime;
		}
	}

	setScriptCode(std::move(script));

	if (npcLevel.isEmpty())
		npcLevel = origLevel;

	if (!npcLevel.isEmpty())
		curlevel = TLevel::findLevel(npcLevel, server);

	return true;
}

#endif

CString toWeaponName(const CString& code)
{
	int name_start = code.find("toweapons ");
	if (name_start == -1) return CString();
	name_start += 10; // 10 = strlen("toweapons ")

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
	for (auto& fileName: joinList)
	{
		c = fs->load(fileName);
		c.removeAllI("\r");
		c.replaceAllI("\n", "\xa7");
		ret << removeComments(c, "\xa7");
	}

	return ret;
}
