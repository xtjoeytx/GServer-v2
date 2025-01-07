#include <IDebug.h>

#include <math.h>
#include <time.h>
#include <vector>

#include <IEnums.h>

#include "FileSystem.h"
#include "Server.h"
#include "object/NPC.h"
#include "level/Level.h"
#include "level/Map.h"
#include "scripting/SourceCode.h"

#ifdef V8NPCSERVER
	#include "scripting/ScriptEngine.h"
	#include "object/Player.h"
#endif

const char __nSavePackets[10] = { 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };
const char __nAttrPackets[30] = { 36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 };

static CString toWeaponName(const CString& code);
static CString doJoins(const CString& code, FileSystem* fs);

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

NPC::NPC(const CString& pImage, std::string pScript, float pX, float pY, std::shared_ptr<Level> pLevel, NPCType type)
	: NPC(type)
{
	setX(pX);
	setY(pY);
	m_image = pImage.text();
	m_curlevel = pLevel;
#ifdef V8NPCSERVER
	m_origImage = m_image;
#endif

	// Keep a copy of the original level for resets
#ifdef V8NPCSERVER
	if (!m_curlevel.expired())
	{
		m_origLevel = getLevel()->getLevelName();
	}
#endif

	// TODO: Create plugin hook so NPCServer can acquire/format code.

	// Needs to be called so it creates a script-object
	//if (!pScript.isEmpty())
	setScriptCode(std::move(pScript));
}

NPC::NPC(NPCType type)
	: m_npcType(type)
#ifdef V8NPCSERVER
	  ,
	  m_scriptExecutionContext(m_server->getScriptEngine()), m_origX(m_x), m_origY(m_y), m_origZ(m_z)
#endif
{
	memset((void*)m_saves, 0, sizeof(m_saves));
	memset((void*)m_modTime, 0, sizeof(m_modTime));

	// imagePart needs to be Graal-packed.
	for (int i = 0; i < 6; i++)
		m_imagePart.writeGChar(0);

	// We need to alter the modTime of the following props as they should be always sent.
	// If we don't, they won't be sent until the prop gets modified.
	m_modTime[NPCPROP_IMAGE] = m_modTime[NPCPROP_SCRIPT] = m_modTime[NPCPROP_X] = m_modTime[NPCPROP_Y] = m_modTime[NPCPROP_VISFLAGS] = m_modTime[NPCPROP_ID] = m_modTime[NPCPROP_SPRITE] = m_modTime[NPCPROP_MESSAGE] = m_modTime[NPCPROP_GMAPLEVELX] = m_modTime[NPCPROP_GMAPLEVELY] = m_modTime[NPCPROP_X2] = m_modTime[NPCPROP_Y2] = time(0);

	// Needs to be called so it creates a script-object
	setScriptCode("");
}

NPC::~NPC()
{
#ifdef V8NPCSERVER
	freeScriptResources();
#endif
}

void NPC::setScriptCode(std::string pScript)
{
	bool firstExecution = m_npcScript.empty();

#ifdef V8NPCSERVER
	// Clear any joined code
	m_classMap.clear();

	if (m_scriptObject)
		freeScriptResources();
#endif
	bool gs2default = m_server->getSettings().getBool("gs2default", false);

	m_npcScript = SourceCode{ std::move(pScript), gs2default };

	bool levelModificationNPCHack = false;

	// NOTE: since we are not removing comments from the source, any comments at the start of the script
	// interferes with the starts_with check, so a temporary workaround is to check for it within the first 100 characters

	// All code is stored in clientside when building without an npc-server, and split as-expected with the npc-server
#ifdef V8NPCSERVER
	std::string_view npcScriptSearch = m_npcScript.getServerSide();
#else
	std::string_view npcScriptSearch = m_npcScript.getClientSide();
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
		m_npcScript.clearServerSide();
	}

	// See if the NPC should block position updates from the level leader.
#ifdef V8NPCSERVER
	m_blockPositionUpdates = true;
#else
	if (m_npcScript.getClientGS1().find("//#BLOCKPOSITIONUPDATES") != std::string::npos)
		m_blockPositionUpdates = true;
#endif

#ifndef V8NPCSERVER
	// Search for toweapons in the clientside code and extract the name of the weapon.
	m_weaponName = toWeaponName(std::string{ m_npcScript.getClientGS1() });
#endif

	// Remove comments and trim the code if specified. Also changes line-endings
#ifdef V8NPCSERVER
	updateClientCode();
#else
	auto tmpScript = doJoins(std::string{ m_npcScript.getClientGS1() }, m_server->getFileSystem());
	m_clientScriptFormatted = minifyClientCode(tmpScript);

	// Just a little warning for people who don't know.
	if (m_clientScriptFormatted.length() > 0x705F)
		printf("WARNING: Clientside script of NPC (%s) exceeds the limit of 28767 bytes.\n", (m_weaponName.length() != 0 ? m_weaponName.text() : m_image.c_str()));
#endif

#ifdef V8NPCSERVER
	// Compile and execute the script.
	bool executed = m_server->getScriptEngine()->executeNpc(this);
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
		m_modTime[NPCPROP_GANI] = 0;
		m_modTime[NPCPROP_IMAGE] = time(0);
		//m_image = "";

		// TODO(joey): refactor
		m_server->sendPacketToLevelArea(CString() >> (char)PLO_NPCDEL >> (int)getId(), level);

		CString packet = CString() >> (char)PLO_NPCPROPS >> (int)getId() << getProps(0);
		m_server->sendPacketToLevelArea(packet, level);
	}
#endif
}

std::shared_ptr<Level> NPC::getLevel() const
{
	// TODO: Handle deleted level.
	// Delete level NPCs.

	return m_curlevel.lock();
}

CString NPC::getProp(unsigned char pId, int clientVersion) const
{
	auto level = getLevel();
	switch (pId)
	{
		case NPCPROP_IMAGE:
			return CString() >> (char)m_image.length() << m_image;

		case NPCPROP_SCRIPT:
			// GS2 support
			if (clientVersion >= CLVER_4_0211)
			{
				// GS1 was disabled after this client version
				if (clientVersion > CLVER_5_07)
					return CString() >> (short)0;

				// If we have bytecode, don't send gs1 script
				if (!m_npcBytecode.isEmpty())
					return CString() >> (short)0;
			}

			return CString() >> (short)(m_clientScriptFormatted.length() > 0x3FFF ? 0x3FFF : m_clientScriptFormatted.length()) << m_clientScriptFormatted.substr(0, 0x3FFF);

		case NPCPROP_X:
			return CString() >> (char)(m_x / 8);

		case NPCPROP_Y:
			return CString() >> (char)(m_y / 8);

		case NPCPROP_Z:
			// range: -25 to 85
			return CString() >> (char)(std::min(85 * 2, std::max(-25 * 2, (m_z / 8))) + 50);

		case NPCPROP_POWER:
			return CString() >> (char)(m_character.hitpointsInHalves);

		case NPCPROP_RUPEES:
			return CString() >> (int)m_character.gralats;

		case NPCPROP_ARROWS:
			return CString() >> (char)m_character.arrows;

		case NPCPROP_BOMBS:
			return CString() >> (char)m_character.bombs;

		case NPCPROP_GLOVEPOWER:
			return CString() >> (char)m_character.glovePower;

		case NPCPROP_BOMBPOWER:
			return CString() >> (char)m_character.bombPower;

		case NPCPROP_SWORDIMAGE:
			if (m_character.swordPower == 0)
				return CString() >> (char)0;
			else
				return CString() >> (char)(m_character.swordPower + 30) >> (char)m_character.swordImage.length() << m_character.swordImage;

		case NPCPROP_SHIELDIMAGE:
			if (m_character.shieldPower + 10 > 10)
				return CString() >> (char)(m_character.shieldPower + 10) >> (char)m_character.shieldImage.length() << m_character.shieldImage;
			else
				return CString() >> (char)0;

		case NPCPROP_GANI:
			if (clientVersion < CLVER_2_1)
			{
				if (m_character.bowPower < 10)
					return CString() >> (char)m_character.bowPower;
				else
					return CString() >> (char)(m_character.bowImage.length() + 10) << m_character.bowImage;
			}
			return CString() >> (char)m_character.gani.length() << m_character.gani;

		case NPCPROP_VISFLAGS:
			return CString() >> (char)m_visFlags;

		case NPCPROP_BLOCKFLAGS:
			return CString() >> (char)m_blockFlags;

		case NPCPROP_MESSAGE:
			return CString() >> (char)m_character.chatMessage.length() << m_character.chatMessage;

		case NPCPROP_HURTDXDY:
			return CString() >> (char)((m_hurtX * 32) + 32) >> (char)((m_hurtY * 32) + 32);

		case NPCPROP_ID:
			return CString() >> (int)m_id;

		// Sprite is deprecated and has been replaced by def.gani.
		// Sprite now holds the direction of the npc.  sprite % 4 gives backwards compatibility.
		case NPCPROP_SPRITE:
		{
			if (clientVersion < CLVER_2_1)
				return CString() >> (char)m_character.sprite;
			else
				return CString() >> (char)(m_character.sprite % 4);
		}

		case NPCPROP_COLORS:
			return CString() >> (char)m_character.colors[0] >> (char)m_character.colors[1] >> (char)m_character.colors[2] >> (char)m_character.colors[3] >> (char)m_character.colors[4];

		case NPCPROP_NICKNAME:
			return CString() >> (char)m_character.nickName.length() << m_character.nickName;

		case NPCPROP_HORSEIMAGE:
			return CString() >> (char)m_character.horseImage.length() << m_character.horseImage;

		case NPCPROP_HEADIMAGE:
			return CString() >> (char)(m_character.headImage.length() + 100) << m_character.headImage;

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
			return CString() >> (char)m_saves[pId - NPCPROP_SAVE0];

		case NPCPROP_ALIGNMENT:
			return CString() >> (char)m_character.ap;

		case NPCPROP_IMAGEPART:
			return CString() << m_imagePart;

		case NPCPROP_BODYIMAGE:
			return CString() >> (char)m_character.bodyImage.length() << m_character.bodyImage;

		case NPCPROP_GMAPLEVELX:
			return CString() >> (char)(level ? level->getGmapX() : 0);

		case NPCPROP_GMAPLEVELY:
			return CString() >> (char)(level ? level->getGmapY() : 0);

#ifdef V8NPCSERVER
		case NPCPROP_SCRIPTER:
			return CString() >> (char)m_npcScripter.length() << m_npcScripter;

		case NPCPROP_NAME:
			return CString() >> (char)m_npcName.length() << m_npcName;

		case NPCPROP_TYPE:
			return CString() >> (char)m_npcScriptType.length() << m_npcScriptType;

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
			for (const auto& it: m_classMap)
				classList << it.first << ",";
#endif

			if (!classList.isEmpty())
				classList.removeI(classList.length() - 1);
			return CString() >> (short)classList.length() << classList;
		}

		case NPCPROP_X2:
		{
			uint16_t val = ((uint16_t)std::abs(m_x)) << 1;
			if (m_x < 0)
				val |= 0x0001;
			return CString().writeGShort(val);
		}

		case NPCPROP_Y2:
		{
			uint16_t val = ((uint16_t)std::abs(m_y)) << 1;
			if (m_y < 0)
				val |= 0x0001;
			return CString().writeGShort(val);
		}

		case NPCPROP_Z2:
		{
			// range: -25 to 85
			uint16_t val = std::min<int16_t>(85 * 16, std::max<int16_t>(-25 * 16, m_z));
			val = std::abs(val) << 1;
			if (m_z < 0)
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
				return CString() >> (char)m_character.ganiAttributes[i].length() << m_character.ganiAttributes[i];
		}
	}

	return CString();
}

CString NPC::getProps(time_t newTime, int clientVersion) const
{
	bool oldcreated = m_server->getSettings().getBool("oldcreated", "false");
	CString retVal;
	int pmax = NPCPROP_COUNT;
	if (clientVersion < CLVER_2_1) pmax = 36;

	for (int i = 0; i < pmax; i++)
	{
		if (m_modTime[i] != 0 && m_modTime[i] >= newTime)
		{
			if (oldcreated && i == NPCPROP_VISFLAGS && newTime == 0)
				retVal >> (char)i >> (char)(m_visFlags | NPCVISFLAG_VISIBLE);
			else
				retVal >> (char)i << getProp(i, clientVersion);
		}
	}
	if (clientVersion > CLVER_1_411)
	{
		if (m_modTime[NPCPROP_GANI] == 0 && m_image == "#c#")
			retVal >> (char)NPCPROP_GANI >> (char)4 << "idle";
	}

	return retVal;
}

CString NPC::setProps(CString& pProps, int clientVersion, bool pForward)
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
				m_visFlags |= NPCVISFLAG_VISIBLE;
				m_image = pProps.readChars(pProps.readGUChar()).text();
				if (!m_image.empty() && clientVersion < CLVER_2_1 && getExtension(m_image).isEmpty())
					m_image.append(".gif");
				break;

			case NPCPROP_SCRIPT:
				pProps.readChars(pProps.readGUShort());

				// TODO(joey): is this used for putnpcs?
				//clientScript = pProps.readChars(pProps.readGUShort());
				break;

			case NPCPROP_X:
				if (m_blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				m_x = pProps.readGChar() * 8;
				hasMoved = true;
				break;

			case NPCPROP_Y:
				if (m_blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				m_y = pProps.readGChar() * 8;
				hasMoved = true;
				break;
				
			case NPCPROP_Z:
				if (m_blockPositionUpdates)
				{
					pProps.readGChar();
					continue;
				}
				m_z = (pProps.readGChar() - 50) * 8;
				hasMoved = true;
				break;

			case NPCPROP_POWER:
				m_character.hitpointsInHalves = pProps.readGUChar();
				break;

			case NPCPROP_RUPEES:
				m_character.gralats = pProps.readGUInt();
				break;

			case NPCPROP_ARROWS:
				m_character.arrows = pProps.readGUChar();
				break;

			case NPCPROP_BOMBS:
				m_character.bombs = pProps.readGUChar();
				break;

			case NPCPROP_GLOVEPOWER:
				m_character.glovePower = pProps.readGUChar();
				break;

			case NPCPROP_BOMBPOWER:
				m_character.bombPower = pProps.readGUChar();
				break;

			case NPCPROP_SWORDIMAGE:
			{
				int sp = pProps.readGUChar();
				if (sp <= 4)
					m_character.swordImage = (CString() << "sword" << CString(sp) << (clientVersion < CLVER_2_1 ? ".gif" : ".png")).toString();
				else
				{
					sp -= 30;
					len = pProps.readGUChar();
					if (len > 0)
					{
						m_character.swordImage = pProps.readChars(len).toString();
						if (!m_character.swordImage.empty() && clientVersion < CLVER_2_1 && getExtension(m_character.swordImage).isEmpty())
							m_character.swordImage += ".gif";
					}
					else
						m_character.swordImage = "";
					//m_character.swordPower = clip(sp, ((settings->getBool("healswords", false) == true) ? -(settings->getInt("swordlimit", 3)) : 0), settings->getInt("swordlimit", 3));
				}
				m_character.swordPower = sp;
				break;
			}

			case NPCPROP_SHIELDIMAGE:
			{
				int sp = pProps.readGUChar();
				if (sp <= 3)
					m_character.shieldImage = (CString() << "shield" << CString(sp) << (clientVersion < CLVER_2_1 ? ".gif" : ".png")).toString();
				else
				{
					sp -= 10;
					len = pProps.readGUChar();
					if (len > 0)
					{
						m_character.shieldImage = pProps.readChars(len).toString();
						if (!m_character.shieldImage.empty() && clientVersion < CLVER_2_1 && getExtension(m_character.shieldImage).isEmpty())
							m_character.shieldImage += ".gif";
					}
					else
						m_character.shieldImage = "";
				}
				m_character.shieldPower = std::min<uint8_t>(sp, 3);
				break;
			}

			case NPCPROP_GANI:
			{
				if (clientVersion < CLVER_2_1)
				{
					// Older clients don't use ganis.  This is the bow power and image instead.
					m_character.bowPower = pProps.readGUChar();
					if (m_character.bowPower >= 10)
					{
						m_character.bowImage = pProps.readChars(m_character.bowPower - 10).toString();
						if (!m_character.bowImage.empty() && clientVersion < CLVER_2_1 && getExtension(m_character.bowImage).isEmpty())
							m_character.bowImage += ".gif";
					}
					break;
				}
				m_character.gani = pProps.readChars(pProps.readGUChar()).text();
				break;
			}

			case NPCPROP_VISFLAGS:
				m_visFlags = pProps.readGUChar();
				break;

			case NPCPROP_BLOCKFLAGS:
				m_blockFlags = pProps.readGUChar();
				break;

			case NPCPROP_MESSAGE:
				m_character.chatMessage = pProps.readChars(pProps.readGUChar()).text();
				break;

			case NPCPROP_HURTDXDY:
				m_hurtX = ((float)(pProps.readGUChar() - 32)) / 32;
				m_hurtY = ((float)(pProps.readGUChar() - 32)) / 32;
				break;

			case NPCPROP_ID:
				pProps.readGUInt();
				break;

			case NPCPROP_SPRITE:
				m_character.sprite = pProps.readGUChar();
				break;

			case NPCPROP_COLORS:
				for (int i = 0; i < 5; i++)
					m_character.colors[i] = pProps.readGUChar();
				break;

			case NPCPROP_NICKNAME:
				m_character.nickName = pProps.readChars(pProps.readGUChar()).text();
				break;

			case NPCPROP_HORSEIMAGE:
				m_character.horseImage = pProps.readChars(pProps.readGUChar()).toString();
				if (!m_character.horseImage.empty() && clientVersion < CLVER_2_1 && getExtension(m_character.horseImage).isEmpty())
					m_character.horseImage += ".gif";
				break;

			case NPCPROP_HEADIMAGE:
				len = pProps.readGUChar();
				if (len < 100)
					m_character.headImage = (CString() << "head" << CString(len) << (clientVersion < CLVER_2_1 ? ".gif" : ".png")).toString();
				else
				{
					m_character.headImage = pProps.readChars(len - 100).toString();
					if (!m_character.headImage.empty() && clientVersion < CLVER_2_1 && getExtension(m_character.headImage).isEmpty())
						m_character.headImage += ".gif";
				}
				break;

			case NPCPROP_ALIGNMENT:
				m_character.ap = pProps.readGUChar();
				m_character.ap = clip(m_character.ap, 0, 100);
				break;

			case NPCPROP_IMAGEPART:
				m_imagePart = pProps.readChars(6);
				break;

			case NPCPROP_BODYIMAGE:
				m_character.bodyImage = pProps.readChars(pProps.readGUChar()).toString();
				break;

			case NPCPROP_GMAPLEVELX:
				pProps.readGUChar();
				break;

			case NPCPROP_GMAPLEVELY:
				pProps.readGUChar();
				break;

			case NPCPROP_SCRIPTER:
				m_npcScripter = pProps.readChars(pProps.readGUChar());
				break;

			case NPCPROP_NAME:
				m_npcName = pProps.readChars(pProps.readGUChar()).text();
				break;

			case NPCPROP_TYPE:
				m_npcScriptType = pProps.readChars(pProps.readGUChar());
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
				if (m_blockPositionUpdates)
				{
					pProps.readGUShort();
					continue;
				}

				len = pProps.readGUShort();
				m_x = (len >> 1);

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					m_x = -m_x;

				hasMoved = true;
				break;

			case NPCPROP_Y2:
				if (m_blockPositionUpdates)
				{
					pProps.readGUShort();
					continue;
				}

				len = pProps.readGUShort();
				m_y = (len >> 1);

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					m_y = -m_y;

				hasMoved = true;
				break;

			case NPCPROP_Z2:
				if (m_blockPositionUpdates)
				{
					pProps.readGUShort();
					continue;
				}

				len = pProps.readGUShort();
				m_z = (len >> 1);

				// If the first bit is 1, our position is negative.
				if ((uint16_t)len & 0x0001)
					m_z = -m_z;

				hasMoved = true;
				break;

			case NPCPROP_SAVE0:
				m_saves[0] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE1:
				m_saves[1] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE2:
				m_saves[2] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE3:
				m_saves[3] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE4:
				m_saves[4] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE5:
				m_saves[5] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE6:
				m_saves[6] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE7:
				m_saves[7] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE8:
				m_saves[8] = pProps.readGUChar();
				break;
			case NPCPROP_SAVE9:
				m_saves[9] = pProps.readGUChar();
				break;

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
				int index = propId - NPCPROP_GATTRIB1;
				m_character.ganiAttributes[index] = pProps.readChars(pProps.readGUChar()).toString();
				break;
			}

			default:
			{
				printf("NPC %ud (%.2f, %.2f): ", m_id, (float)m_x / 16.0f, (float)m_y / 16.0f);
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
				m_modTime[propId] = time(0);
		}

		// Add to ret.
		ret >> (char)propId << getProp(propId, clientVersion);
	}

	if (pForward)
	{
		// Send the props.
		m_server->sendPacketToLevelArea(CString() >> (char)PLO_NPCPROPS >> (int)m_id << ret, m_curlevel);
	}

#ifdef V8NPCSERVER
	if (hasMoved) testTouch();
#endif

	return ret;
}

#ifdef V8NPCSERVER

void NPC::testForLinks()
{
	auto level = getLevel();
	if (level == nullptr) return;

	// Overworld links
	if (auto map = level->getMap(); map)
	{
		// Gmaps are treated as one large map, and so (local?) npcs can freely walk
		// across maps without canwarp being enabled (source: post=1193766)
		if (map->isGmap() || m_canWarp != NPCWarpType::None)
		{
			int gmapX = m_x + 1024 * level->getMapX();
			int gmapY = m_y + 1024 * level->getMapY();
			int mapx = gmapX / 1024;
			int mapy = gmapY / 1024;

			if (level->getMapX() != mapx || level->getMapY() != mapy)
			{
				auto newLevel = m_server->getLevel(map->getLevelAt(mapx, mapy));
				if (newLevel != nullptr)
				{
					this->warpNPC(newLevel, gmapX % 1024, gmapY % 1024);
					return;
				}
			}
		}
	}

	if (m_canWarp == NPCWarpType::AllLinks)
	{
		static const int touchtestd[] = { 2, 1, 0, 2, 2, 4, 3, 2 };

		int dir = m_character.sprite % 4;

		auto linkTouched = level->getLink((int)(m_x / 16) + touchtestd[dir * 2], (int)(m_y / 16) + touchtestd[dir * 2 + 1]);
		if (linkTouched)
		{
			auto newLevel = m_server->getLevel(linkTouched.value()->getNewLevel().toString());
			if (newLevel != 0)
			{
				int newX = (linkTouched.value()->getNewX() == "playerx" ? m_x : int(16.0 * strtofloat(linkTouched.value()->getNewX())));
				int newY = (linkTouched.value()->getNewY() == "playery" ? m_y : int(16.0 * strtofloat(linkTouched.value()->getNewY())));
				this->warpNPC(newLevel, newX, newY);
			}
		}
	}
}

void NPC::testTouch()
{
	if (m_curlevel.expired())
		return;

	testForLinks();
}

void NPC::freeScriptResources()
{
	ScriptEngine* scriptEngine = m_server->getScriptEngine();

	// Clear cached script
	if (!m_npcScript.getServerSide().empty())
		scriptEngine->clearCache<NPC>(m_npcScript.getServerSide());

	// Reset script execution
	m_scriptExecutionContext.resetExecution();

	// Clear any queued actions
	scriptEngine->unregisterNpcUpdate(this);

	// Clear timeouts & scheduled events
	scriptEngine->unregisterNpcTimer(this);
	m_scriptTimers.clear();
	m_timeout = 0;

	// Clear triggeraction functions
	for (auto& _triggerAction: m_triggerActions)
		delete _triggerAction.second;
	m_triggerActions.clear();

	// Delete script object
	if (m_scriptObject)
		m_scriptObject.reset();
}

// Set callbacks for triggeractions!
void NPC::registerTriggerAction(const std::string& action, IScriptFunction* cbFunc)
{
	// clear old callback if it was set
	auto triggerIter = m_triggerActions.find(action);
	if (triggerIter != m_triggerActions.end())
		delete triggerIter->second;

	// register new trigger
	m_triggerActions[action] = cbFunc;
}

void NPC::queueNpcTrigger(const std::string& action, Player* player, const std::string& data)
{
	assert(m_scriptObject);

	// Check if we respond to this trigger
	auto triggerIter = m_triggerActions.find(action);
	if (triggerIter == m_triggerActions.end())
		return;

	ScriptEngine* scriptEngine = m_server->getScriptEngine();

	IScriptObject<Player>* playerObject = nullptr;
	if (player != nullptr)
		playerObject = player->getScriptObject();

	if (playerObject)
	{
		m_scriptExecutionContext.addAction(scriptEngine->createAction("npc.trigger", getScriptObject(), triggerIter->second, playerObject, data));
	}
	else
	{
		m_scriptExecutionContext.addAction(scriptEngine->createAction("npc.trigger", getScriptObject(), triggerIter->second, nullptr, data));
	}

	scriptEngine->registerNpcUpdate(this);
}

ScriptClass* NPC::joinClass(const std::string& className)
{
	auto found = m_classMap.find(className);
	if (found != m_classMap.end())
		return nullptr;

	auto classObj = m_server->getClass(className);
	if (!classObj)
		return nullptr;

	m_classMap[className] = classObj->getSource().getClientGS1();
	updateClientCode();
	m_modTime[NPCPROP_CLASS] = time(0);
	return classObj;
}

void NPC::updateClientCode()
{
	// Skip servercode, and read client script
	CString tmpScript = std::string{ m_npcScript.getClientGS1() };

	// Iterate current classes, and add to end of code
	for (auto& it: m_classMap)
		tmpScript << "\n"
				  << it.second;

	// Remove comments and trim the code if specified.
	m_clientScriptFormatted = minifyClientCode(tmpScript);

	// Just a little warning for people who don't know.
	if (m_clientScriptFormatted.length() > 0x705F)
		printf("WARNING: Clientside script of NPC (%s) exceeds the limit of 28767 bytes.\n", (m_weaponName.length() != 0 ? m_weaponName.text() : m_image.c_str()));

	// Compile gs2
	if (!m_npcScript.getClientGS2().empty())
	{
		// Compile gs2 code
		m_server->compileGS2Script(this,
								   [this](const CompilerResponse& response)
								   {
									   if (response.success)
									   {
										   auto& byteCode = response.bytecode;
										   m_npcBytecode.clear(byteCode.length());
										   m_npcBytecode.write((const char*)byteCode.buffer(), (int)byteCode.length());
									   }
								   });
	}

	// Update prop for players
	this->updatePropModTime(NPCPROP_SCRIPT);
}

void NPC::setTimeout(int newTimeout)
{
	m_timeout = newTimeout;

	if (hasTimerUpdates())
		m_server->getScriptEngine()->registerNpcTimer(this);
	else
		m_server->getScriptEngine()->unregisterNpcTimer(this);
}

void NPC::queueNpcAction(const std::string& action, Player* player, bool registerAction)
{
	assert(m_scriptObject);

	ScriptEngine* scriptEngine = m_server->getScriptEngine();

	IScriptObject<Player>* playerObject = nullptr;
	if (player != nullptr)
		playerObject = player->getScriptObject();

	if (playerObject)
	{
		m_scriptExecutionContext.addAction(scriptEngine->createAction(action, getScriptObject(), playerObject));
	}
	else
	{
		m_scriptExecutionContext.addAction(scriptEngine->createAction(action, getScriptObject()));
	}

	if (registerAction)
		scriptEngine->registerNpcUpdate(this);
}

bool NPC::runScriptTimer()
{
	// TODO(joey): Scheduled events, pass in delta, use milliseconds as an integer

	if (m_timeout > 0)
	{
		m_timeout--;
		if (m_timeout == 0)
			queueNpcAction("npc.timeout", 0, true);
	}

	// scheduled events
	bool queued = false;
	for (auto it = m_scriptTimers.begin(); it != m_scriptTimers.end();)
	{
		ScriptEventTimer& timer = *it;
		timer.timer--;
		if (timer.timer == 0)
		{
			m_scriptExecutionContext.addAction(timer.action);
			it = m_scriptTimers.erase(it);
			queued = true;
			continue;
		}

		++it;
	}

	// Register for npc updates
	if (queued)
		m_server->getScriptEngine()->registerNpcUpdate(this);

	// return value dictates if this gets unregistered from timer updates
	return hasTimerUpdates();
}

NPCEventResponse NPC::runScriptEvents()
{
	bool hasActions = false;
	if (!m_npcDeleteRequested)
		hasActions = m_scriptExecutionContext.runExecution(); // Returns true if we still have actions to run

	// Send properties modified by scripts
	if (!m_propModified.empty())
	{
		if (m_propModified.contains(NPCPROP_X2) || m_propModified.contains(NPCPROP_Y2))
		{
			testTouch();
		}

		time_t newModTime = time(0);

		CString propPacket = CString() >> (char)PLO_NPCPROPS >> (int)m_id;
		for (unsigned char propId: m_propModified)
		{
			m_modTime[propId] = newModTime;
			propPacket >> (char)propId << getProp(propId);
		}
		m_propModified.clear();

		if (!m_curlevel.expired())
			m_server->sendPacketToLevelArea(propPacket, m_curlevel);
	}

	if (m_npcDeleteRequested)
	{
		m_npcDeleteRequested = false;
		return NPCEventResponse::Delete;
	}

	return (hasActions ? NPCEventResponse::PendingEvents : NPCEventResponse::NoEvents);
}

CString NPC::getVariableDump()
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
	CString npcNameStr = m_npcName;
	if (npcNameStr.isEmpty())
		npcNameStr = CString() << "npcs[" << CString(m_id) << "]";

	auto level = getLevel();

	npcDump << "Variables dump from npc " << npcNameStr << "\n\n";
	if (!m_npcScriptType.isEmpty()) npcDump << npcNameStr << ".type: " << m_npcScriptType << "\n";
	if (!m_npcScripter.isEmpty()) npcDump << npcNameStr << ".scripter: " << m_npcScripter << "\n";
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
				int sp = prop.readGUChar();
				CString image;
				if (sp > 30)
				{
					image = prop.readChars(prop.readGUChar());
					sp -= 30;
				}
				else if (sp > 0)
					image = CString() << "sword" << CString(sp) << ".png";

				if (!image.isEmpty())
					npcDump << npcNameStr << "." << propNames[propId] << ": " << image << " (" << CString(sp) << ")\n";

				break;
			}

			case NPCPROP_SHIELDIMAGE:
			{
				int sp = prop.readGUChar();
				CString image;
				if (sp > 10)
				{
					image = prop.readChars(prop.readGUChar());
					sp -= 10;
				}
				else if (sp > 0)
					image = CString() << "shield" << CString(sp) << ".png";

				if (!image.isEmpty())
					npcDump << npcNameStr << "." << propNames[propId] << ": " << image << " (" << CString(sp) << ")\n";

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
				pos = ((pos & 0x0001) ? -(pos >> 1) : pos >> 1);
				npcDump << npcNameStr << "." << propNames[propId] << ": " << CString((double)pos / 16.0) << "\n";
				break;
			}

			default:
				continue;
		}
	}

	if (m_timeout > 0)
		npcDump << npcNameStr << ".timeout: " << CString((float)(m_timeout * 0.05f)) << "\n";

	std::pair<unsigned int, double> executionData = m_scriptExecutionContext.getExecutionData();
	npcDump << npcNameStr << ".scripttime (in the last min): " << CString(executionData.second) << "\n";
	npcDump << npcNameStr << ".scriptcalls: " << CString(executionData.first) << "\n";

	if (!m_flagList.empty())
	{
		npcDump << "\nnpc.Flags:\n";
		for (auto it = m_flagList.begin(); it != m_flagList.end(); ++it)
			npcDump << npcNameStr << ".flags[\"" << (*it).first << "\"]: " << (*it).second << "\n";
	}

	return npcDump;
}

bool NPC::deleteNPC()
{
	if (getType() == NPCType::PUTNPC)
	{
		m_npcDeleteRequested = true;
		registerNpcUpdates();
	}

	return m_npcDeleteRequested;
}

void NPC::reloadNPC()
{
	setScriptCode(m_npcScript.getSource());
}

void NPC::resetNPC()
{
	// TODO(joey): reset script execution, clear flags.. unsure what else gets reset. TBD
	m_canWarp = NPCWarpType::None;
	m_modTime[NPCPROP_IMAGE] = m_modTime[NPCPROP_SCRIPT] = m_modTime[NPCPROP_X] = m_modTime[NPCPROP_Y] = m_modTime[NPCPROP_VISFLAGS] = m_modTime[NPCPROP_ID] = m_modTime[NPCPROP_SPRITE] = m_modTime[NPCPROP_MESSAGE] = m_modTime[NPCPROP_GMAPLEVELX] = m_modTime[NPCPROP_GMAPLEVELY] = m_modTime[NPCPROP_X2] = m_modTime[NPCPROP_Y2] = time(0);
	m_character.headImage = "";
	m_character.bodyImage = "";
	m_image = "";
	m_character.gani = "idle";

	// Reset script execution
	setScriptCode(m_npcScript.getSource());

	if (!m_origLevel.isEmpty())
	{
		warpNPC(Level::findLevel(m_origLevel, m_server), m_origX, m_origY);
	}
}

void NPC::moveNPC(int dx, int dy, double time, int options)
{
	// TODO(joey): Implement options? Or does the client handle them? TBD
	//	- If we want function callbacks we will need to handle time, can schedule an event once that is implemented

	int start_x = ((uint16_t)std::abs(m_x) << 1) | (m_x < 0 ? 0x0001 : 0x0000);
	int start_y = ((uint16_t)std::abs(m_y) << 1) | (m_y < 0 ? 0x0001 : 0x0000);
	int delta_x = ((uint16_t)std::abs(dx) << 1) | (dx < 0 ? 0x0001 : 0x0000);
	int delta_y = ((uint16_t)std::abs(dy) << 1) | (dy < 0 ? 0x0001 : 0x0000);
	short itime = (short)(time / 0.05);

	setPixelX(m_x + dx);
	setPixelY(m_y + dy);

	if (!m_curlevel.expired())
		m_server->sendPacketToLevelArea(CString() >> (char)PLO_MOVE2 >> (int)m_id >> (short)start_x >> (short)start_y >> (short)delta_x >> (short)delta_y >> (short)itime >> (char)options, m_curlevel);

	if (isWarpable())
		testTouch();
}

void NPC::warpNPC(std::shared_ptr<Level> pLevel, int pX, int pY)
{
	if (!pLevel)
		return;

	auto level = getLevel();
	if (level != nullptr)
	{
		// TODO(joey): NPCMOVED needs to be sent to everyone who potentially has this level cached or else the npc
		//  will stay visible when you come back to the level. Should this just be sent to everyone on the server? We do
		//  such for PLO_NPCDEL
		m_server->sendPacketToType(PLTYPE_ANYPLAYER, CString() >> (char)PLO_NPCMOVED >> (int)m_id);

		// Remove the npc from the old level
		level->removeNPC(m_id);
	}

	// Add to the new level
	pLevel->addNPC(m_id);
	level = pLevel;

	// Adjust the position of the npc
	m_x = pX;
	m_y = pY;

	updatePropModTime(NPCPROP_CURLEVEL);
	updatePropModTime(NPCPROP_GMAPLEVELX);
	updatePropModTime(NPCPROP_GMAPLEVELY);
	updatePropModTime(NPCPROP_X2);
	updatePropModTime(NPCPROP_Y2);

	// Send the properties to the players in the new level
	m_server->sendPacketToLevelArea(CString() >> (char)PLO_NPCPROPS >> (int)m_id << getProps(0), level);

	if (!m_npcName.empty())
		m_server->sendPacketToType(PLTYPE_ANYNC, CString() >> (char)PLO_NC_NPCADD >> (int)m_id >> (char)NPCPROP_CURLEVEL << getProp(NPCPROP_CURLEVEL));

	// Queue event
	this->queueNpcAction("npc.warped");
}

void NPC::saveNPC()
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
	//m_scriptExecutionContext.getExecutionData();

	/*
	CString saveDir;
	CString saveName;
	if (getType() == NPCType::DBNPC)
	{
		saveDir = "npcs/";
		saveName = m_npcName;
	}
	else if (getType() == NPCType::PUTNPC)
	{
		saveDir = "npcprops/";
		saveName = CString("localnpc_");

		if (level && level->getMap())
		{
			saveName << removeExtension(m_origLevel) << "_" << level->getMapX() << "_" << level->getMapY();
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
	CString fileName = m_server->getServerPath() << "npcs/npc" << m_npcName << ".txt";
	CString fileData = CString("GRNPC001") << NL;
	fileData << "NAME " << m_npcName << NL;
	fileData << "ID " << CString(m_id) << NL;
	fileData << "TYPE " << m_npcScriptType << NL;
	fileData << "SCRIPTER " << m_npcScripter << NL;
	fileData << "IMAGE " << m_image << NL;
	fileData << "STARTLEVEL " << m_origLevel << NL;
	fileData << "STARTX " << CString((float)m_origX / 16.0f) << NL;
	fileData << "STARTY " << CString((float)m_origY / 16.0f) << NL;
	fileData << "STARTZ " << CString((float)m_origY / 16.0f) << NL;
	if (level)
	{
		fileData << "LEVEL " << level->getLevelName() << NL;
		fileData << "X " << CString(getX()) << NL;
		fileData << "Y " << CString(getY()) << NL;
		fileData << "Z " << CString(getZ()) << NL;
	}
	fileData << "NICK " << m_character.nickName << NL;
	fileData << "ANI " << m_character.gani << NL;
	fileData << "HP " << CString(m_character.hitpoints) << NL;
	fileData << "GRALATS " << CString(m_character.gralats) << NL;
	fileData << "ARROWS " << CString(m_character.arrows) << NL;
	fileData << "BOMBS " << CString(m_character.bombs) << NL;
	fileData << "GLOVEP " << CString(m_character.glovePower) << NL;
	fileData << "SWORDP " << CString(m_character.swordPower) << NL;
	fileData << "SHIELDP " << CString(m_character.shieldPower) << NL;
	fileData << "BOWP" << CString(m_character.bowPower) << NL;
	fileData << "BOW" << m_character.bowImage << NL;
	fileData << "HEAD " << m_character.headImage << NL;
	fileData << "BODY " << m_character.bodyImage << NL;
	fileData << "SWORD " << m_character.swordImage << NL;
	fileData << "SHIELD " << m_character.shieldImage << NL;
	fileData << "HORSE " << m_character.horseImage << NL;
	fileData << "COLORS " << CString((int)m_character.colors[0]) << "," << CString((int)m_character.colors[1]) << "," << CString((int)m_character.colors[2]) << "," << CString((int)m_character.colors[3]) << "," << CString((int)m_character.colors[4]) << NL;
	fileData << "SPRITE " << CString(m_character.sprite) << NL;
	fileData << "AP " << CString(m_character.ap) << NL;
	fileData << "TIMEOUT " << CString(m_timeout / 20) << NL;
	fileData << "LAYER 0" << NL;
	fileData << "SHAPETYPE 0" << NL;
	fileData << "SHAPE " << CString(m_width) << " " << CString(m_height) << NL;

	if (m_blockFlags & NPCBLOCKFLAG_NOBLOCK)
		fileData << "DONTBLOCK 1" << NL;

	fileData << "SAVEARR " << CString((int)m_saves[0]) << "," << CString((int)m_saves[1]) << "," << CString((int)m_saves[2]) << ","
			 << CString((int)m_saves[3]) << "," << CString((int)m_saves[4]) << "," << CString((int)m_saves[5]) << ","
			 << CString((int)m_saves[6]) << "," << CString((int)m_saves[7]) << "," << CString((int)m_saves[8]) << ","
			 << CString((int)m_saves[9]) << NL;

	for (int i = 0; i < 30; i++)
	{
		if (!m_character.ganiAttributes[i].isEmpty())
			fileData << "ATTR" << std::to_string(i + 1) << " " << m_character.ganiAttributes[i] << NL;
	}

	for (auto it = m_flagList.begin(); it != m_flagList.end(); ++it)
		fileData << "FLAG " << (*it).first << "=" << (*it).second << NL;

	fileData << "NPCSCRIPT" << NL << CString(m_npcScript.getSource()).replaceAll("\n", NL);
	if (fileData[fileData.length() - 1] != '\n')
		fileData << NL;
	fileData << "NPCSCRIPTEND" << NL;
	fileData.save(fileName);
}

bool NPC::loadNPC(const CString& fileName)
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
			m_npcName = curLine.readString("").text();
			m_modTime[NPCPROP_NAME] = updateTime;
		}
		else if (curCommand == "ID")
			m_id = strtoint(curLine.readString(""));
		else if (curCommand == "TYPE")
			m_npcScriptType = curLine.readString("");
		else if (curCommand == "SCRIPTER")
		{
			m_npcScripter = curLine.readString("");
			m_modTime[NPCPROP_SCRIPTER] = updateTime;
		}
		else if (curCommand == "IMAGE")
		{
			m_image = curLine.readString("").text();
			m_modTime[NPCPROP_IMAGE] = updateTime;
		}
		else if (curCommand == "STARTLEVEL")
			m_origLevel = curLine.readString("");
		else if (curCommand == "STARTX")
			m_origX = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "STARTY")
			m_origY = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "STARTZ")
			m_origZ = int(strtofloat(curLine.readString("")) * 16);
		else if (curCommand == "LEVEL")
			npcLevel = curLine.readString("");
		else if (curCommand == "X")
			setX(strtofloat(curLine.readString("")));
		else if (curCommand == "Y")
			setY(strtofloat(curLine.readString("")));
		else if (curCommand == "Z")
			setZ(strtofloat(curLine.readString("")));
		else if (curCommand == "MAPX")
		{
			//gmaplevelx = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_GMAPLEVELX] = updateTime;
		}
		else if (curCommand == "MAPY")
		{
			//gmaplevely = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_GMAPLEVELY] = updateTime;
		}
		else if (curCommand == "NICK")
		{
			m_character.nickName = curLine.readString("").text();
			m_modTime[NPCPROP_NICKNAME] = updateTime;
		}
		else if (curCommand == "ANI")
		{
			m_character.gani = curLine.readString("").text();
			m_modTime[NPCPROP_GANI] = updateTime;
		}
		else if (curCommand == "HP")
		{
			m_character.hitpoints = (int)strtofloat(curLine.readString(""));
			m_modTime[NPCPROP_POWER] = updateTime;
		}
		else if (curCommand == "GRALATS")
		{
			m_character.gralats = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_RUPEES] = updateTime;
		}
		else if (curCommand == "ARROWS")
		{
			m_character.arrows = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_ARROWS] = updateTime;
		}
		else if (curCommand == "BOMBS")
		{
			m_character.bombs = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_BOMBS] = updateTime;
		}
		else if (curCommand == "GLOVEP")
		{
			m_character.glovePower = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_GLOVEPOWER] = updateTime;
		}
		else if (curCommand == "SWORDP")
		{
			m_character.swordPower = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_SWORDIMAGE] = updateTime;
		}
		else if (curCommand == "SHIELDP")
		{
			m_character.shieldPower = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_SHIELDIMAGE] = updateTime;
		}
		else if (curCommand == "BOWP")
		{
			m_character.bowPower = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_GANI] = updateTime;
		}
		else if (curCommand == "BOW")
		{
			m_character.bowImage = curLine.readString("");
			m_modTime[NPCPROP_GANI] = updateTime;
		}
		else if (curCommand == "HEAD")
		{
			m_character.headImage = curLine.readString("");
			m_modTime[NPCPROP_HEADIMAGE] = updateTime;
		}
		else if (curCommand == "BODY")
		{
			m_character.bodyImage = curLine.readString("");
			m_modTime[NPCPROP_BODYIMAGE] = updateTime;
		}
		else if (curCommand == "SWORD")
		{
			m_character.swordImage = curLine.readString("");
			m_modTime[NPCPROP_SWORDIMAGE] = updateTime;
		}
		else if (curCommand == "SHIELD")
		{
			m_character.shieldImage = curLine.readString("");
			m_modTime[NPCPROP_SHIELDIMAGE] = updateTime;
		}
		else if (curCommand == "HORSE")
		{
			m_character.horseImage = curLine.readString("");
			m_modTime[NPCPROP_HORSEIMAGE] = updateTime;
		}
		else if (curCommand == "SPRITE")
		{
			m_character.sprite = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_SPRITE] = updateTime;
		}
		else if (curCommand == "AP")
		{
			m_character.ap = strtoint(curLine.readString(""));
			m_modTime[NPCPROP_ALIGNMENT] = updateTime;
		}
		else if (curCommand == "COLORS")
		{
			auto tokens = curLine.readString("").tokenize(",");
			for (int idx = 0; idx < std::min((int)tokens.size(), 5); idx++)
				m_character.colors[idx] = strtoint(tokens[idx]);
			m_modTime[NPCPROP_COLORS] = updateTime;
		}
		else if (curCommand == "SAVEARR")
		{
			auto tokens = curLine.readString("").tokenize(",");
			for (int idx = 0; idx < std::min(tokens.size(), sizeof(m_saves) / sizeof(unsigned char)); idx++)
			{
				m_saves[idx] = (unsigned char)strtoint(tokens[idx]);
				m_modTime[NPCPROP_SAVE0 + idx] = updateTime;
			}
		}
		else if (curCommand == "SHAPE")
		{
			m_width = strtoint(curLine.readString(" "));
			m_height = strtoint(curLine.readString(" "));
		}
		else if (curCommand == "CANWARP")
		{
			m_canWarp = strtoint(curLine.readString("")) != 0 ? NPCWarpType::AllLinks : m_canWarp;
		}
		else if (curCommand == "CANWARP2")
		{
			m_canWarp = strtoint(curLine.readString("")) != 0 ? NPCWarpType::OverworldLinks : m_canWarp;
		}
		else if (curCommand == "TIMEOUT")
			m_timeout = strtoint(curLine.readString("")) * 20;
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
			{
				int idx = attrId - 1;
				m_character.ganiAttributes[idx] = curLine.readString("");
				m_modTime[__nAttrPackets[idx]] = updateTime;
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

			m_modTime[NPCPROP_SCRIPT] = updateTime;
		}
	}

	setScriptCode(std::move(script));

	if (npcLevel.isEmpty())
		npcLevel = m_origLevel;

	if (!npcLevel.isEmpty())
		m_curlevel = Level::findLevel(npcLevel, m_server);

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

CString doJoins(const CString& code, FileSystem* fs)
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
				ret << ";\n";
				joinList.push_back(CString() << c.readString(";") << ".txt");
			}
		}
	}

	// Add the files now.
	for (auto& fileName: joinList)
	{
		c = fs->load(fileName);
		c.removeAllI("\r");
		ret << removeComments(c);
	}

	return ret;
}
