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
#include "utilities/Log.h"

using namespace graal::utilities;

const char __nSavePackets[10] = { 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };
const char __nAttrPackets[30] = { 36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 };

static CString toWeaponName(const CString& code);
static CString doJoins(const CString& code, FileSystem* fs);

static std::string minifyClientCode(const CString& src)
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

	// TODO: Create plugin hook so NPCServer can acquire/format code.

	// Needs to be called so it creates a script-object
	//if (!pScript.isEmpty())
	setScriptCode(std::move(pScript));
}

NPC::NPC(NPCType type)
	: m_npcType(type)
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
}

void NPC::setScriptCode(std::string pScript)
{
	bool firstExecution = m_npcScript.empty();
	bool gs2default = m_server->getSettings().getBool("gs2default", false);

	m_npcScript = SourceCode{ std::move(pScript), gs2default };

	bool levelModificationNPCHack = false;

	// NOTE: since we are not removing comments from the source, any comments at the start of the script
	// interferes with the starts_with check, so a temporary workaround is to check for it within the first 100 characters

	// All code is stored in clientside when building without an npc-server, and split as-expected with the npc-server
	std::string_view npcScriptSearch = m_npcScript.getClientSide();

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
	if (m_npcScript.getClientGS1().find("//#BLOCKPOSITIONUPDATES") != std::string::npos)
		m_blockPositionUpdates = true;

	// Search for toweapons in the clientside code and extract the name of the weapon.
	m_weaponName = toWeaponName(std::string{ m_npcScript.getClientGS1() });

	// Remove comments and trim the code if specified. Also changes line-endings
	auto tmpScript = doJoins(std::string{ m_npcScript.getClientGS1() }, m_server->getFileSystem());
	m_clientScriptFormatted = minifyClientCode(tmpScript);

	// Just a little warning for people who don't know.
	if (m_clientScriptFormatted.length() > 0x705F)
		log::printLine(log::server, "WARNING: Clientside script of NPC ({}) exceeds the limit of 28767 bytes.", (m_weaponName.length() != 0 ? m_weaponName : m_image));
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

		case NPCPROP_CLASS:
		{
			CString classList;
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
			{
				auto sprite = pProps.readGUChar();
				if (clientVersion < CLVER_2_1)
					m_character.sprite = sprite;
				else m_character.sprite = sprite % 4;
				break;
			}

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
				int index = propId - NPCPROP_SAVE0;
				m_saves[index] = pProps.readGUChar();
				break;
			}

			case NPCPROP_GATTRIB1:
			case NPCPROP_GATTRIB2:
			case NPCPROP_GATTRIB3:
			case NPCPROP_GATTRIB4:
			case NPCPROP_GATTRIB5:
			{
				int index = propId - NPCPROP_GATTRIB1;
				m_character.ganiAttributes[index] = pProps.readChars(pProps.readGUChar()).toString();
				break;
			}

			case NPCPROP_GATTRIB6:
			case NPCPROP_GATTRIB7:
			case NPCPROP_GATTRIB8:
			case NPCPROP_GATTRIB9:
			{
				int index = 5 + propId - NPCPROP_GATTRIB6;
				m_character.ganiAttributes[index] = pProps.readChars(pProps.readGUChar()).toString();
				break;
			}

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
				int index = 9 + propId - NPCPROP_GATTRIB10;
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

	return ret;
}

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
