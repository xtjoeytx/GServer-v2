#ifndef NPC_H
#define NPC_H

#include <CString.h>
#include <IUtil.h>

#include <common.h>

#include <object/Character.h>
#include <scripting/ScriptContainers.h>
#include <scripting/SourceCode.h>
#include <utilities/PropsContainer.h>

using namespace preagonal::props;

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

inline constexpr std::array<uint8_t, 30> NpcGaniAttrPackets = { 36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 };

class Server;
class Level;
class Player;
class ScriptClass;

using PlayerPtr = std::shared_ptr<Player>;

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
	UNKNOWNBIT4		= 0b0000'1000,
	UNKNOWNBIT5		= 0b0001'0000,
	UNKNOWNBIT6		= 0b0010'0000,
	MALE			= 0b0100'0000,
};

//! NPCPROP_BLOCKFLAGS values.
enum class NPCBlockFlags : uint8_t
{
	BLOCK			= 0b0000'0000,
	NOBLOCK			= 0b0000'0001,
	CANBECARRIED	= 0b0000'0010,
	CANBEPULLED		= 0b0000'0100,
	CANBEPUSHED		= 0b0000'1000,
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
	/// @brief Records the current modification time of all properties.
	[[inline]] void recordCurrentPropModTime();

	/// @brief Constructs a PropertyContainer for NPCProp P with the given values.
	/// @tparam P The NPCProp that determines the type of container to construct.
	/// @param ...values The values to pass to the container's constructor.
	/// @return A property container for the specified NPCProp P.
	template<NPCProp P, typename... Args>
	[[inline]] PropertyContainer auto constructPropFor(Args... values) const;

	/// @brief Constructs a PropertyContainer for NPCProp prop with the given values.
	/// @param prop The NPCProp that determines the type of container to construct.
	/// @return A shared pointer to the constructed property's base class.
	std::shared_ptr<PropertyBase> constructPropFor(NPCProp prop) const;

	/// @brief Gets the property container for NPCProp P.
	/// @tparam P The NPCProp that determines the type of container to get.
	/// @return A property container for the specified NPCProp P.
	template<NPCProp P>
	[[inline]] PropertyContainer auto getProp() const;

	/// @brief Gets the property container for NPCProp P.
	/// @param prop The NPCProp that determines the type of container to get.
	/// @return A shared pointer to the constructed property's base class.
	std::shared_ptr<PropertyBase> getProp(NPCProp prop) const;

	/// @brief Sets a property value for a player and returns the result of the operation.
	/// @tparam P The type of the player property to set.
	/// @param setBy Specifies who is setting the property. Defaults to SetBy::CLIENT.
	/// @param prop A property container that contains the value to set.
	/// @return A SetResults value indicating the outcome of the property set operation.
	template<NPCProp P>
	[[inline]] SetResults setProp(SetBy setBy, PropertyContainer auto prop);

	/// @brief Sets a property value for a player with the given values and returns the result of the operation.
	/// @tparam P The NPCProp that determines the type of property to set.
	/// @param setBy Specifies who is setting the property. Defaults to SetBy::CLIENT.
	/// @param ...values The values to pass to the property container's constructor.
	/// @return A SetResults value indicating the outcome of the property set operation.
	template<NPCProp P, typename... Args>
	[[inline]] SetResults setPropWith(SetBy setBy, Args... values);

	/// @brief Sets a property for a player and returns the result of the operation.
	/// @param prop The player property to set.
	/// @param setBy Indicates who is setting the property. Defaults to SetBy::CLIENT.
	/// @param base A shared pointer to the base property value to assign.
	/// @return A SetResults value indicating the outcome of the property set operation.
	SetResults setProp(NPCProp prop, SetBy setBy, std::shared_ptr<PropertyBase> base);

	/// @brief Sends the results of setting a property across the network.
	/// @param ...results A list of SetResults results to send.
	template<typename... Results> requires all_same_as<SetResults, Results...>
	[[inline]] void sendPropsFromResults(const Results&... results);

	/// @brief Sends the results of setting properties across the network.
	/// @param results A range of SetResults results to send.
	void sendPropsFromResults(std::ranges::forward_range auto&& results);

protected:
	SetResults setProp(NPCProp prop, SetBy setBy, PropertyBase* base);
	void sendPropsFromResults(PropertySendResults& results, PlayerPtr source = nullptr);

public:
	/// @brief Sets properties from a packet string.
	/// @param packet A packet that contains property data.
	/// @param source Indicates who is setting the properties.
	void setPropsFromPacket(CString& packet, PlayerPtr source = nullptr);

	CString getModifiedPropsPacket() const;
	CString getAllPropsPacket(clock::time_point newTime = clock::time_point::min()) const;

public:
	const std::string& getWeaponName() const noexcept { return m_weaponName; }
	bool isCharacter() const noexcept { return image == "#c#"; }
	std::string getLevelName() const;

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
	std::array<clock::time_point, NPCPROP_COUNT> modTime;
	NPCWarpRestrictions warpRestrictions = NPCWarpRestrictions::ALLOWED;
	ScriptContainer scripting;

private:
	BabyDI_INJECT(Server, m_server);

	std::array<clock::time_point, NPCPROP_COUNT> m_savedModTime;
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

//----------------------------

// Defines the mapping of NPCProp to PropertyContainer.
#define FOR_LIST_OF_NPC_PROPS(DO) \
	DO(NPCProp::IMAGE,		PropertyString,				image) \
	DO(NPCProp::SCRIPT,		PropertyGS1Script,			m_script.getClientSide()) \
	DO(NPCProp::X,			PropertyTileCoordinate,		character.pixelX) \
	DO(NPCProp::Y,			PropertyTileCoordinate,		character.pixelY) \
	DO(NPCProp::POWER,		PropertyNumeric<GBYTE1>,	character.hitpointsInHalves) \
	DO(NPCProp::RUPEES,		PropertyNumeric<GBYTE3>,	character.gralats) \
	DO(NPCProp::ARROWS,		PropertyNumeric<GBYTE1>,	character.arrows) \
	DO(NPCProp::BOMBS,		PropertyNumeric<GBYTE1>,	character.bombs) \
	DO(NPCProp::GLOVEPOWER,	PropertyNumeric<GBYTE1>,	character.glovePower) \
	DO(NPCProp::BOMBPOWER,	PropertyNumeric<GBYTE1>,	character.bombPower) \
	DO(NPCProp::SWORDIMAGE,	PropertySwordPower,			character.swordImage, character.swordPower) \
	DO(NPCProp::SHIELDIMAGE,PropertyShieldPower,		character.shieldImage, character.shieldPower) \
	DO(NPCProp::GANI,		PropertyGaniOrBowGif,		character.gani, character.bowPower, character.bowImage) \
	DO(NPCProp::VISFLAGS,	PropertyNumeric<GBYTE1>,	visFlags) \
	DO(NPCProp::BLOCKFLAGS, PropertyNumeric<GBYTE1>,	blockFlags) \
	DO(NPCProp::MESSAGE,	PropertyString,				character.chatMessage) \
	DO(NPCProp::HURTDXDY,	PropertyHurtDxDy,			hurtX, hurtY) \
	DO(NPCProp::ID,			PropertyNumeric<GBYTE3>,	id) \
	DO(NPCProp::SPRITE,		PropertySprite,				character.sprite, character.direction) \
	DO(NPCProp::COLORS,		PropertyColors,				character.colors) \
	DO(NPCProp::NICKNAME,	PropertyString,				character.nickName) \
	DO(NPCProp::HORSEIMAGE,	PropertyString,				character.horseImage) \
	DO(NPCProp::HEADIMAGE,	PropertyHeadGif,			character.headImage) \
	DO(NPCProp::SAVE0,		PropertyNumeric<GBYTE1>,	saves[0]) \
	DO(NPCProp::SAVE1,		PropertyNumeric<GBYTE1>,	saves[1]) \
	DO(NPCProp::SAVE2,		PropertyNumeric<GBYTE1>,	saves[2]) \
	DO(NPCProp::SAVE3,		PropertyNumeric<GBYTE1>,	saves[3]) \
	DO(NPCProp::SAVE4,		PropertyNumeric<GBYTE1>,	saves[4]) \
	DO(NPCProp::SAVE5,		PropertyNumeric<GBYTE1>,	saves[5]) \
	DO(NPCProp::SAVE6,		PropertyNumeric<GBYTE1>,	saves[6]) \
	DO(NPCProp::SAVE7,		PropertyNumeric<GBYTE1>,	saves[7]) \
	DO(NPCProp::SAVE8,		PropertyNumeric<GBYTE1>,	saves[8]) \
	DO(NPCProp::SAVE9,		PropertyNumeric<GBYTE1>,	saves[9]) \
	DO(NPCProp::ALIGNMENT,	PropertyNumeric<GBYTE1>,	character.ap) \
	DO(NPCProp::IMAGEPART,	PropertyImagePart,			imagePart) \
	DO(NPCProp::BODYIMAGE,	PropertyString,				character.bodyImage) \
	DO(NPCProp::GATTRIB1,	PropertyString,				character.ganiAttributes[0]) \
	DO(NPCProp::GATTRIB2,	PropertyString,				character.ganiAttributes[1]) \
	DO(NPCProp::GATTRIB3,	PropertyString,				character.ganiAttributes[2]) \
	DO(NPCProp::GATTRIB4,	PropertyString,				character.ganiAttributes[3]) \
	DO(NPCProp::GATTRIB5,	PropertyString,				character.ganiAttributes[4]) \
	DO(NPCProp::GMAPLEVELX,	PropertyNumeric<GBYTE1>,	(level.expired() ? 0 : (level.lock()->getGmapX()))) \
	DO(NPCProp::GMAPLEVELY,	PropertyNumeric<GBYTE1>,	(level.expired() ? 0 : (level.lock()->getGmapY()))) \
	DO(NPCProp::Z,			PropertyTileCoordinateZ,	character.pixelZ) \
	DO(NPCProp::GATTRIB6,	PropertyString,				character.ganiAttributes[5]) \
	DO(NPCProp::GATTRIB7,	PropertyString,				character.ganiAttributes[6]) \
	DO(NPCProp::GATTRIB8,	PropertyString,				character.ganiAttributes[7]) \
	DO(NPCProp::GATTRIB9,	PropertyString,				character.ganiAttributes[8]) \
	DO(NPCProp::UNKNOWN48,	PropertyVoid) \
	DO(NPCProp::SCRIPTER,	PropertyString,				m_npcScripter) \
	DO(NPCProp::NAME,		PropertyString,				name) \
	DO(NPCProp::TYPE,		PropertyString,				m_npcScriptType) \
	DO(NPCProp::CURLEVEL,	PropertyString,				getLevelName()) \
	DO(NPCProp::GATTRIB10,	PropertyString,				character.ganiAttributes[9]) \
	DO(NPCProp::GATTRIB11,	PropertyString,				character.ganiAttributes[10]) \
	DO(NPCProp::GATTRIB12,	PropertyString,				character.ganiAttributes[11]) \
	DO(NPCProp::GATTRIB13,	PropertyString,				character.ganiAttributes[12]) \
	DO(NPCProp::GATTRIB14,	PropertyString,				character.ganiAttributes[13]) \
	DO(NPCProp::GATTRIB15,	PropertyString,				character.ganiAttributes[14]) \
	DO(NPCProp::GATTRIB16,	PropertyString,				character.ganiAttributes[15]) \
	DO(NPCProp::GATTRIB17,	PropertyString,				character.ganiAttributes[16]) \
	DO(NPCProp::GATTRIB18,	PropertyString,				character.ganiAttributes[17]) \
	DO(NPCProp::GATTRIB19,	PropertyString,				character.ganiAttributes[18]) \
	DO(NPCProp::GATTRIB20,	PropertyString,				character.ganiAttributes[19]) \
	DO(NPCProp::GATTRIB21,	PropertyString,				character.ganiAttributes[20]) \
	DO(NPCProp::GATTRIB22,	PropertyString,				character.ganiAttributes[21]) \
	DO(NPCProp::GATTRIB23,	PropertyString,				character.ganiAttributes[22]) \
	DO(NPCProp::GATTRIB24,	PropertyString,				character.ganiAttributes[23]) \
	DO(NPCProp::GATTRIB25,	PropertyString,				character.ganiAttributes[24]) \
	DO(NPCProp::GATTRIB26,	PropertyString,				character.ganiAttributes[25]) \
	DO(NPCProp::GATTRIB27,	PropertyString,				character.ganiAttributes[26]) \
	DO(NPCProp::GATTRIB28,	PropertyString,				character.ganiAttributes[27]) \
	DO(NPCProp::GATTRIB29,	PropertyString,				character.ganiAttributes[28]) \
	DO(NPCProp::GATTRIB30,	PropertyString,				character.ganiAttributes[29]) \
	DO(NPCProp::CLASS,		PropertyString,				m_npcClass) \
	DO(NPCProp::X2,			PropertyPixelCoordinate,	character.pixelX) \
	DO(NPCProp::Y2,			PropertyPixelCoordinate,	character.pixelY) \
	DO(NPCProp::Z2,			PropertyPixelCoordinate,	character.pixelZ)

//----------------------------

template<NPCProp P, typename... Args>
PropertyContainer auto NPC::constructPropFor(Args... values) const
{
#define RETURN_CONSTRUCTPROPSFOR_CONSTEXPR(prop, type, ...) if constexpr (P == prop) return type{ values... };
	FOR_LIST_OF_NPC_PROPS(RETURN_CONSTRUCTPROPSFOR_CONSTEXPR);

	throw std::invalid_argument("Invalid NPCProp type in constructPropFor");
}

template<NPCProp P>
PropertyContainer auto NPC::getProp() const
{
#define RETURN_GETPROP_CONSTEXPR(prop, type, ...) if constexpr (P == prop) return type{ __VA_ARGS__ };
	FOR_LIST_OF_NPC_PROPS(RETURN_GETPROP_CONSTEXPR);

	throw std::invalid_argument("Invalid NPCProp type in getProp");
}

template<NPCProp P>
SetResults NPC::setProp(SetBy setBy, PropertyContainer auto prop)
{
	return setProp(P, setBy, &prop);
}

template<NPCProp P, typename... Args>
SetResults NPC::setPropWith(SetBy setBy, Args... values)
{
	return setProp<P>(setBy, constructPropFor<P>(values...));
}

template<typename... Results> requires all_same_as<SetResults, Results...>
void NPC::sendPropsFromResults(const Results&... results)
{
	PropertySendResults send_results;
	(send_results.emplace_back(results, nullptr), ...);
	sendPropsFromResults(send_results);
}

void NPC::sendPropsFromResults(std::ranges::forward_range auto&& results)
{
	PropertySendResults send_results;
	auto results_range = results | std::views::transform([](const SetResults& results) { return std::make_pair(results, nullptr); });
	for (auto& r : results_range)
		send_results.emplace_back(r);

	sendPropsFromResults(send_results);
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // NPC_H
