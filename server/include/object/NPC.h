#ifndef NPC_H
#define NPC_H

#include <CString.h>
#include <IUtil.h>

#include <common.h>

#include <object/Character.h>
#include <scripting/ScriptContainers.h>
#include <scripting/SourceCode.h>
#include <utilities/FlagContainer.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

inline constexpr std::array<uint8_t, 30> npcGaniAttrPackets = { 36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 };

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

//! NPC warp restrictions
enum class NPCWarpRestrictions
{
	ALLOWED,
	NOTALLOWED,
	ONLYOVERWORLD,
};

//! NPC type
enum class NPCType
{
	LEVELNPC, // npcs found in a level
	PUTNPC,   // npcs created via script (putnpc), is this needed still?
	DBNPC     // npcs created in RC (Database-NPCs)
};

//----------------------------

class NPC
{
	friend class FlatFileNPCLoader;

public:
	NPC(NPCID id, NPCType type);
	~NPC() = default;

	void setScript(std::string_view script);
	const SourceCode& getScript() const noexcept { return m_script; }

public:
	[[inline]] void recordCurrentPropModTime();

	[[inline]] void setProp(NPCProp prop, const auto& value)
		requires VariantContainsType<prop_access, std::remove_cvref_t<decltype(value)>>;

	CString getModifiedPropsPacket(int clientVersion = CLVER_2_17) const;

	// prop functions
	CString getPropPacket(NPCProp pId, int clientVersion = CLVER_2_17) const;
	CString getAllPropsPacket(time_t newTime, int clientVersion = CLVER_2_17) const;
	CString setPropsFromPacket(CString& pProps, int clientVersion = CLVER_2_17, bool pForward = false);
	//void setPropModTime(NPCProp pid, time_t time);

public:
	const std::string& getWeaponName() const noexcept { return m_weaponName; }
	bool isCharacter() const noexcept { return image == "#c#"; }

	// Records the current state as the initial state of the NPC.
	void recordInitialState()
	{
		m_initialImage = image;
		m_initialLevel = level;
		m_initialCharacter = character;
	}

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
	std::chrono::milliseconds timeout = 0ms;
	Character character;
	std::array<uint8_t, 10> saves;
	std::array<int64_t, NPCPROP_COUNT> modTime;
	NPCWarpRestrictions warpRestrictions = NPCWarpRestrictions::ALLOWED;
	FlagContainer flags;
	ScriptContainer scripting;

private:
	prop_access getPropAccess(NPCProp prop);

private:
	BabyDI_INJECT(Server, m_server);

	std::array<int64_t, NPCPROP_COUNT> m_savedModTime;
	bool m_blockPositionUpdates = false;

	SourceCode m_script;

	std::string m_initialImage;
	std::weak_ptr<Level> m_initialLevel;
	Character m_initialCharacter;

	std::string m_weaponName;
	std::string m_npcScripter;
	std::string m_npcScriptType;
	std::string m_npcClass;
};

using NPCPtr = std::shared_ptr<NPC>;
using NPCWeakPtr = std::weak_ptr<NPC>;

//----------------------------

inline void NPC::recordCurrentPropModTime()
{
	m_savedModTime = modTime;
}

void NPC::setProp(NPCProp prop, const auto& value)
	requires VariantContainsType<prop_access, std::remove_cvref_t<decltype(value)>>
{
	using value_type = std::remove_cvref_t<decltype(value)>;

	auto access = getPropAccess(prop);
	auto* ptr = std::get_if<value_type*>(&access);
	if (ptr == nullptr)
		return;

	**ptr = value;
	modTime[PROPID(prop)] = currentTimeInSeconds();

	if (prop == NPCProp::X2)
		modTime[PROPID(NPCProp::X)] = modTime[PROPID(prop)];
	if (prop == NPCProp::Y2)
		modTime[PROPID(NPCProp::Y)] = modTime[PROPID(prop)];
	if (prop == NPCProp::Z2)
		modTime[PROPID(NPCProp::Z)] = modTime[PROPID(prop)];
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // NPC_H
