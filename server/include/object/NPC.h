#ifndef NPC_H
#define NPC_H

#include <CString.h>
#include <IUtil.h>

#include "common.h"

#include "object/Character.h"
#include "scripting/SourceCode.h"
#include "utilities/FlagContainer.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class Server;
class Level;
class Player;
class ScriptClass;

enum class NPCProp : uint8_t
{
	IMAGE = 0,
	SCRIPT = 1,
	X = 2,
	Y = 3,
	POWER = 4,
	RUPEES = 5,
	ARROWS = 6,
	BOMBS = 7,
	GLOVEPOWER = 8,
	BOMBPOWER = 9,
	SWORDIMAGE = 10,
	SHIELDIMAGE = 11,
	GANI = 12, // NPCPROP_BOWGIF in pre-2.x
	VISFLAGS = 13,
	BLOCKFLAGS = 14,
	MESSAGE = 15,
	HURTDXDY = 16,
	ID = 17,
	SPRITE = 18,
	COLORS = 19,
	NICKNAME = 20,
	HORSEIMAGE = 21,
	HEADIMAGE = 22,
	SAVE0 = 23,
	SAVE1 = 24,
	SAVE2 = 25,
	SAVE3 = 26,
	SAVE4 = 27,
	SAVE5 = 28,
	SAVE6 = 29,
	SAVE7 = 30,
	SAVE8 = 31,
	SAVE9 = 32,
	ALIGNMENT = 33,
	IMAGEPART = 34,
	BODYIMAGE = 35,
	GATTRIB1 = 36,
	GATTRIB2 = 37,
	GATTRIB3 = 38,
	GATTRIB4 = 39,
	GATTRIB5 = 40,
	GMAPLEVELX = 41,
	GMAPLEVELY = 42,

	Z = 43,

	GATTRIB6 = 44,
	GATTRIB7 = 45,
	GATTRIB8 = 46,
	GATTRIB9 = 47,

	UNKNOWN48 = 48,
	SCRIPTER = 49, // My guess is UNKNOWN48 or this is the scripter's name
	NAME = 50,
	TYPE = 51,
	CURLEVEL = 52,

	GATTRIB10 = 53,
	GATTRIB11 = 54,
	GATTRIB12 = 55,
	GATTRIB13 = 56,
	GATTRIB14 = 57,
	GATTRIB15 = 58,
	GATTRIB16 = 59,
	GATTRIB17 = 60,
	GATTRIB18 = 61,
	GATTRIB19 = 62,
	GATTRIB20 = 63,
	GATTRIB21 = 64,
	GATTRIB22 = 65,
	GATTRIB23 = 66,
	GATTRIB24 = 67,
	GATTRIB25 = 68,
	GATTRIB26 = 69,
	GATTRIB27 = 70,
	GATTRIB28 = 71,
	GATTRIB29 = 72,
	GATTRIB30 = 73,

	CLASS = 74, // NPC-Server class.  Possibly also join scripts.
	X2 = 75,
	Y2 = 76,
	Z2 = 77,

	NPCPROP_COUNT
};
constexpr int NPCPROP_COUNT = static_cast<int>(NPCProp::NPCPROP_COUNT);

//! NPCPROP_VISFLAGS values.
enum class NPCVisFlags : uint8_t
{
	HIDDEN			= 0b0000'0000,
	VISIBLE			= 0b0000'0001,
	DRAWOVERPLAYER	= 0b0000'0010,
	DRAWUNDERPLAYER	= 0b0000'0100,
};

//! NPCPROP_BLOCKFLAGS values.
enum class NPCBlockFlags : uint8_t
{
	BLOCK	= 0b0000'0000,
	NOBLOCK	= 0b0000'0001,
};

//! NPCMOVE_FLAGS values
enum class NPCMoveFlags : uint8_t
{
	NOCACHE			= 0b0000'0000,
	CACHE			= 0b0000'0001,
	APPEND			= 0b0000'0010,
	BLOCKCHECK		= 0b0000'0100,
	EVENTWHENDONE	= 0b0000'1000,
	APPLYDIR		= 0b0001'0000,
};

enum class NPCType
{
	LEVELNPC, // npcs found in a level
	PUTNPC,   // npcs created via script (putnpc)
	DBNPC     // npcs created in RC (Database-NPCs)
};

class NPC
{
public:
	NPC(NPCID id, NPCType type);
	~NPC() = default;

	// prop functions
	CString getPropPacket(NPCProp pId, int clientVersion = CLVER_2_17) const;
	CString getAllPropsPacket(time_t newTime, int clientVersion = CLVER_2_17) const;
	CString setPropsFromPacket(CString& pProps, int clientVersion = CLVER_2_17, bool pForward = false);
	void setPropModTime(NPCProp pid, time_t time);

	void setScript(std::string_view script);
	const SourceCode& getScript() const noexcept { return m_script; }

	const std::string& getWeaponName() const noexcept { return m_weaponName; }

	// Records the current state as the initial state of the NPC.
	void recordInitialState()
	{
		m_initialImage = image;
		m_initialLevel = level;
		m_initialCharacter = character;
	}

	bool isCharacter() const noexcept { return image == "#c#"; }

public:
	const NPCID id;
	const NPCType type;
	std::string name;
	std::string image;
	std::weak_ptr<Level> level;
	Dimension<uint16_t> imageSize;
	Rectangle<uint16_t, uint8_t> imagePart;
	uint8_t visFlags = 1;
	uint8_t blockFlags = 0;
	float hurtX = 0.0f;
	float hurtY = 0.0f;
	int timeout = 0;
	Character character;
	std::array<uint8_t, 10> saves;
	std::array<time_t, NPCPROP_COUNT> modTime;
	FlagContainer flags;

private:
	BabyDI_INJECT(Server, m_server);

	SourceCode m_script;

	std::string m_initialImage;
	std::weak_ptr<Level> m_initialLevel;
	Character m_initialCharacter;

	bool m_blockPositionUpdates = false;
	std::string m_weaponName;

	CString m_npcScripter, m_npcScriptType;
};

using NPCPtr = std::shared_ptr<NPC>;
using NPCWeakPtr = std::weak_ptr<NPC>;

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // NPC_H
