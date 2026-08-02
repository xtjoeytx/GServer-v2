#ifndef PLAYER_H
#define PLAYER_H

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <CSocket.h>

#include <CFileQueue.h>
#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <Account.h>
#include <limits>
#include <network/IPacketHandler.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/PropertySerializers.h>
#include <utilities/generator/IdGenerator.h>

using namespace preagonal::props;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Level;
struct SubLevel;
struct StaticLevelData;
class Map;
class Weapon;

//----------------------------

enum class LevelItemType;

enum
{
	PLSETPROPS_SETBYPLAYER = 0x01, // if set, do serverside checks to prevent attributes from being changed
	PLSETPROPS_FORWARD = 0x02,     // forward data to other players
	PLSETPROPS_FORWARDSELF = 0x04, // forward data back to the player
};

enum class CursorNumbers : uint8_t
{
	DEFAULT = 0,
	HIDDEN = 1,
	NORMAL = 2,
	CROSS = 3,
	TEXT = 4,
	HIDDEN_2 = 5,
	RESIZE_LL_UR = 6,
	RESIZE_UD = 7,
	RESIZE_UL_LR = 8,
	RESIZE_LR = 9,
	UP_ARROW = 10,
	HOURGLASS = 11,
	FILE = 12,
	NOT_ALLOWED = 13,
	BREAK_ADJUST_LR = 14,
	BREAK_ADJUST_UD = 15,
	MULTIPLE_FILES = 16,
	SQL_HOURGLASS = 17,
	NOT_ALLOWED_2 = 18,
	MOUSE_HOURGLASS = 19,
	MOUSE_QUESTION = 20,
	POINTING_HAND = 21,
	FOUR_DIR_ARROW = 22,

	COUNT
};

enum class GameFeatureFlags : uint32_t
{
	M_MAP = 0x0001,
	P_PAUSE = 0x0002,
	Q_WEAPONSELECT = 0x0004,
	R_SHOWRATING = 0x0008,
	SA_DROPITEM = 0x0010,
	SD_SWITCHWEAPON = 0x0020,
	TAB_CHAT = 0x0040,
	CHATMESSAGE = 0x0080,
	HEARTSOVERPLAYERS = 0x0100,
	NICKNAMES = 0x0200,
	TOALL_PM_BUBBLES = 0x0400,
	OPEN_PROFILE = 0x0800,
	EMOTICONS = 0x1000,
	LEVELSNAPSHOTS = 0x2000, // ALT+5
	LEVELZOOMING = 0x4000,   // ALT+8/9
	LOGFRAME = 0x8000,       // F2 (savelog() / echo())
	ALLFEATURES = 0xFFFF
};

enum class GameStatsFlags : uint32_t
{
	ASD_KEYS = 0x0001,
	ICONS = 0x0002, // (for gralats, bombs, arrows, etc.)
	GRALAT_COUNT = 0x0004,
	BOMB_COUNT = 0x0008,
	ARROW_COUNT = 0x0010,
	HEART_COUNT = 0x0020,
	ALIGNMENT = 0x0040,
	MAGIC = 0x0080,
	MINIMAP = 0x0100, // ALT+3
	INVENTORY = 0x0200,
	PLAYERS = 0x0400,
	OPEN_PROFILE = 0x0800,
	ALLSTATS = 0xFFFF
};

//----------------------------

struct ShootPacketWrapper
{
	NPCID source;
	PixelPosition position;
	int8_t offsetx;
	int8_t offsety;
	uint8_t sangle;
	uint8_t sanglez;
	uint8_t power;
	uint8_t gravity;
	std::string gani;
	std::string shootParams;

	CString constructShootV1() const;
	CString constructShootV2() const;
};

//----------------------------

class Server;
class Player : public CSocketStub, public IPacketHandler, public std::enable_shared_from_this<Player>
{
public:
	// Required by CSocketStub.
	virtual bool onRecv() override;
	virtual bool onSend() override;
	virtual bool onRegister() override { return true; }
	virtual void onUnregister() override;
	virtual SOCKET getSocketHandle() override { return m_playerSock->getHandle(); }
	virtual bool canRecv() override;
	virtual bool canSend() override;

	// Constructor - Deconstructor
	Player(CSocket* pSocket, PlayerID pId);
	virtual ~Player();
	virtual void cleanup();

	// Main methods.
	virtual void doMain();
	virtual bool doTimedEvents();

	// Manage Account
	bool isLoggedIn() const;
	virtual bool handleLogin(CString& pPacket);
	virtual bool sendLogin();

	// Get Properties
	CSocket* getSocket() { return m_playerSock; }
	[[inline]] PlayerID getId() const;
	clock::time_point getLastData() const { return m_lastData; }
	CString getGuild() const { return m_guild; }
	int getVersion() const { return m_versionId; }
	const std::string& getVersionStr() const { return m_version; }
	const std::string& getServerName() const { return m_serverName; }
	const std::string& getPlatform() const { return account.platform; }
	[[inline]] std::string_view getLanguage() const;
	int64_t getDeviceId() const { return m_deviceId; }
	NPCID getCarryNPC() const { return m_carryNPC; }
	NPCID getAttachedNPC() const { return m_attachNPC; }
	uint8_t getCarrySprite() const { return m_carrySprite; }
	[[inline]] bool isPaused() const noexcept;
	bool isInNoPkLevel() const noexcept;
	[[inline]] PixelRectangleArea getBoundingBox() const noexcept;
	[[inline]] PixelRectangleArea getCollisionBoundingBox() const noexcept;
	[[inline]] PixelPosition getGlobalPosition() const noexcept;
	[[inline]] LocalPixelPosition getLocalPosition() const noexcept;
	[[inline]] TilePosition getTilePosition() const noexcept;
	[[inline]] PixelPosition getSubLevelOrigin() const noexcept;
	[[inline]] MapPosition getMapPosition() const noexcept;
	virtual double getCalculatedTileZ() const noexcept;
	virtual std::string getLevelName() const { return account.level; }
	virtual std::shared_ptr<Level> getLevel() const;

	// Set Properties
	void setNick(CString pNickName, bool force = false);
	void setId(PlayerID pId);
	void setLoaded(bool loaded) { this->m_loaded = loaded; }
	void setServerName(CString& tmpServerName) { m_serverName = tmpServerName; }
	void setChat(const CString& pChat);
	void setDeviceId(int64_t newDeviceId) { m_deviceId = newDeviceId; }
	void setCarryNPC(NPCID id) { m_carryNPC = id; }

public:
	/// @brief Records the current modification time of all properties.
	[[inline]] void recordCurrentPropModTime();

	/// @brief Constructs a PropertyContainer for PlayerProp P with the given values.
	/// @tparam P The PlayerProp that determines the type of container to construct.
	/// @param ...values The values to pass to the container's constructor.
	/// @return A property container for the specified PlayerProp P.
	template<PlayerProp P, typename... Args>
	[[inline]] PropertyContainer auto constructPropFor(Args... values) const;

	/// @brief Constructs a PropertyContainer for PlayerProp prop with the given values.
	/// @param prop The PlayerProp that determines the type of container to construct.
	/// @return A shared pointer to the constructed property's base class.
	std::shared_ptr<PropertyBase> constructPropFor(PlayerProp prop) const;

	/// @brief Gets the property container for PlayerProp P.
	/// @tparam P The PlayerProp that determines the type of container to get.
	/// @return A property container for the specified PlayerProp P.
	template<PlayerProp P>
	[[inline]] PropertyContainer auto getProp() const;

	/// @brief Gets the property container for PlayerProp P.
	/// @param prop The PlayerProp that determines the type of container to get.
	/// @return A shared pointer to the constructed property's base class.
	std::shared_ptr<PropertyBase> getProp(PlayerProp prop) const;

	/// @brief Sets a property value for a player and returns the result of the operation.
	/// @tparam P The type of the player property to set.
	/// @param setBy Specifies who is setting the property.
	/// @param prop A property container that contains the value to set.
	/// @return A SetResults value indicating the outcome of the property set operation.
	template<PlayerProp P>
	[[inline]] SetResults setProp(SetBy setBy, PropertyContainer auto prop);

	/// @brief Sets a property for a player and returns the result of the operation.
	/// @param prop The player property to set.
	/// @param base A shared pointer to the base property value to assign.
	/// @param setBy Indicates who is setting the property.
	/// @return A SetResults value indicating the outcome of the property set operation.
	SetResults setProp(PlayerProp prop, SetBy setBy, std::shared_ptr<PropertyBase> base);

	/// @brief Sets a property value for a player with the given values and returns the result of the operation.
	/// @tparam P The PlayerProp that determines the type of property to set.
	/// @param setBy Specifies who is setting the property.
	/// @param ...values The values to pass to the property container's constructor.
	/// @return A SetResults value indicating the outcome of the property set operation.
	template<PlayerProp P, typename... Args>
	[[inline]] SetResults setPropWith(SetBy setBy, Args... values);

	/// @brief Sends the results of setting a property across the network.
	/// @param ...results A list of SetResults results to send.
	template<typename... Results>
		requires AllSameAs<SetResults, Results...>
	[[inline]] void sendPropsFromResults(const Results&... results);

	/// @brief Sends the results of setting properties across the network.
	/// @param results A range of SetResults results to send.
	void sendPropsFromResults(std::ranges::forward_range auto&& results);

public:
	/// @brief Sets properties from a packet string.
	/// @param packet A packet that contains property data.
	/// @param setBy Indicates who is setting the properties, either the client or server.
	/// @param originator Who is the originator of the property set request, if applicable.
	void setPropsFromPacket(CString& packet, SetBy setBy, Player* originator = nullptr);

	/// @brief Sets properties from a remote control packet.
	/// @param packet A packet that contains property data.
	/// @param rc The remote control player, if applicable.
	void setPropsFromRCPacket(CString& packet, Player* rc = nullptr);

	/// @brief Retrieves a packet containing properties from a list of properties.
	/// @param props A list of properties to include in the packet.
	/// @return A packet of properties.
	CString getPropsPacketFromList(const PropList& props) const;

	/// @brief Gets a packet containing properties for remote control profile viewing.
	/// @return A packet of properties.
	CString getPropsForRCPacket();

	/// @brief Exchanges the properties of the current player with other players.
	void exchangeMyPropsWithOthers();

	/// @brief Gets a packet containing modified properties.
	/// @return A packet of modified properties.
	CString getModifiedPropsPacket() const;

public:
	void constructScriptParameters();
	string_map<GameVariable> scriptParameters;

public:
	bool deleteFlag(std::string_view flagName, const SetBy setBy);
	bool setFlag(std::string_view flagPair, const SetBy setBy);
	bool setFlag(std::string_view flagName, std::optional<std::string> flagValue, const SetBy setBy);

public:
	virtual void setPosition(const PixelPosition& position);
	virtual bool warp(std::string_view levelName, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime = std::nullopt);
	virtual bool warp(std::shared_ptr<Level> level, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime = std::nullopt);
	virtual bool enterLevel(std::shared_ptr<Level> level, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime = std::nullopt);
	virtual bool enterLevel(std::shared_ptr<Level> level, const MapPosition& mapPosition, const LocalPixelPosition& position, std::optional<clock::time_point> clientCachedTime = std::nullopt);
	virtual bool enterLevel(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime = std::nullopt);
	virtual bool leaveLevel(bool keepLevelReference = false);
	virtual bool leaveSubLevel(std::shared_ptr<SubLevel> subLevel);
	virtual bool sendStaticLevelData(std::shared_ptr<StaticLevelData> staticLevelData, std::shared_ptr<SubLevel> subLevel, std::optional<clock::time_point> clientCachedTime = std::nullopt);
	virtual bool sendDynamicLevelData(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime = std::nullopt);
	virtual bool sendNearbyObjects(std::shared_ptr<Level> level);

public:
	// Socket-Functions
	void sendPacket(CString pPacket, bool appendNL = true);
	std::pair<bool, bool> sendFile(const std::filesystem::path& file);
	void setReceivedBuffer(const CString& buffer) { m_recvBuffer = buffer; }

	// Type of player
	bool isAdminIp();
	bool isStaff();
	bool isJailed();
	bool isNC() const { return (m_type & PLTYPE_ANYNC) != 0; }
	bool isRC() const { return (m_type & PLTYPE_ANYRC) != 0; }
	bool isClient() const { return (m_type & PLTYPE_ANYCLIENT) != 0; }
	bool isNPCServer() const { return (m_type & PLTYPE_NPCSERVER) != 0; }
	bool isControlClient() const { return (m_type & PLTYPE_ANYCONTROL) != 0; }
	bool isHiddenClient() const { return (m_type & PLTYPE_NONITERABLE) != 0; }
	bool isLoaded() const { return m_loaded; }
	bool isGuest() const { return account.loadOnly && account.communityName == "guest"; }
	int getType() const { return m_type; }
	void setType(int val) { m_type = val; }
	void setExternal(bool val) { m_isExternal = val; }

	bool addWeapon(LevelItemType defaultWeapon);
	bool addWeapon(std::string_view name);
	bool addWeapon(std::shared_ptr<Weapon> weapon);
	bool deleteWeapon(LevelItemType defaultWeapon);
	bool deleteWeapon(std::string_view name);
	bool deleteWeapon(std::shared_ptr<Weapon> weapon);

	std::string translate(std::string_view key) const;

	virtual void sendPrivateMessage(PlayerID from, std::string_view message);

	// Misc functions.
	void disconnect(std::string_view message = ""sv);

	bool addPMServer(CString& option);
	bool remPMServer(CString& option);
	bool inChatChannel(const std::string& channel) const;
	bool addChatChannel(const std::string& channel);
	bool removeChatChannel(const std::string& channel);
	bool updatePMPlayers(CString& servername, CString& players);
	bool pmExternalPlayer(CString servername, CString account, CString& pmMessage);
	std::vector<CString> getPMServerList();
	std::shared_ptr<Player> getExternalPlayer(const PlayerID id, bool includeRC = true) const;
	std::shared_ptr<Player> getExternalPlayer(const CString& account, bool includeRC = true) const;

public:
	Account account;
	std::array<std::optional<clock::time_point>, PLAYERPROP_COUNT> modTime;
	uint32_t loginTime = 0;
	uint32_t lastDeadTime = 0;

protected:
	SetResults setProp(PlayerProp prop, SetBy setBy, PropertyBase* base);
	bool checkPropSetAccess(PlayerProp prop, SetBy setBy, Player* originator) const;
	void sendPropsFromResults(PropertySendResults& results);

protected:
	virtual std::string_view whoAmI() const noexcept override { return account.name; }
	virtual HandlePacketResult handlePacket(std::optional<uint8_t> id, CString& packet) override;

public:
	// Packet-Functions
	HandlePacketResult msgPLI_NULL(CString& pPacket);
	HandlePacketResult msgPLI_LOGIN(CString& pPacket);
	HandlePacketResult msgWebSocketInit(CString& pPacket);

	//HandlePacketResult msgPLI_LEVELWARP(CString& pPacket);
	//HandlePacketResult msgPLI_BOARDMODIFY(CString& pPacket);
	//HandlePacketResult msgPLI_REQUESTUPDATEBOARD(CString& pPacket);
	HandlePacketResult msgPLI_PLAYERPROPS(CString& pPacket);
	//HandlePacketResult msgPLI_NPCPROPS(CString& pPacket);
	//HandlePacketResult msgPLI_BOMBADD(CString& pPacket);
	//HandlePacketResult msgPLI_BOMBDEL(CString& pPacket);
	HandlePacketResult msgPLI_TOALL(CString& pPacket);
	//HandlePacketResult msgPLI_HORSEADD(CString& pPacket);
	//HandlePacketResult msgPLI_HORSEDEL(CString& pPacket);
	//HandlePacketResult msgPLI_ARROWADD(CString& pPacket);
	//HandlePacketResult msgPLI_FIRESPY(CString& pPacket);
	//HandlePacketResult msgPLI_THROWCARRIED(CString& pPacket);
	//HandlePacketResult msgPLI_ITEMADD(CString& pPacket);
	//HandlePacketResult msgPLI_ITEMDEL(CString& pPacket);
	//HandlePacketResult msgPLI_CLAIMPKER(CString& pPacket);
	//HandlePacketResult msgPLI_BADDYPROPS(CString& pPacket);
	//HandlePacketResult msgPLI_BADDYHURT(CString& pPacket);
	//HandlePacketResult msgPLI_BADDYADD(CString& pPacket);
	//HandlePacketResult msgPLI_FLAGSET(CString& pPacket);
	//HandlePacketResult msgPLI_FLAGDEL(CString& pPacket);
	//HandlePacketResult msgPLI_OPENCHEST(CString& pPacket);
	//HandlePacketResult msgPLI_PUTNPC(CString& pPacket);
	//HandlePacketResult msgPLI_NPCDEL(CString& pPacket);
	//HandlePacketResult msgPLI_WANTFILE(CString& pPacket);
	//HandlePacketResult msgPLI_SHOWIMGPLAYER(CString& pPacket);
	// PLI_UNKNOWN25
	//HandlePacketResult msgPLI_HURTPLAYER(CString& pPacket);
	//HandlePacketResult msgPLI_EXPLOSION(CString& pPacket);
	HandlePacketResult msgPLI_PRIVATEMESSAGE(CString& pPacket);
	//HandlePacketResult msgPLI_NPCWEAPONDEL(CString& pPacket);
	HandlePacketResult msgPLI_PACKETCOUNT(CString& pPacket);
	//HandlePacketResult msgPLI_WEAPONADD(CString& pPacket);
	//HandlePacketResult msgPLI_UPDATEFILE(CString& pPacket);
	//HandlePacketResult msgPLI_ADJACENTLEVEL(CString& pPacket);
	//HandlePacketResult msgPLI_HITOBJECTS(CString& pPacket);
	HandlePacketResult msgPLI_LANGUAGE(CString& pPacket);
	//HandlePacketResult msgPLI_TRIGGERACTION(CString& pPacket);
	//HandlePacketResult msgPLI_TAMPERCHECK(CString& pPacket);
	//HandlePacketResult msgPLI_SHOOT(CString& pPacket);
	//HandlePacketResult msgPLI_SHOOT2(CString& pPacket);
	//HandlePacketResult msgPLI_SERVERWARP(CString& pPacket);
	//HandlePacketResult msgPLI_PROCESSLIST(CString& pPacket);
	//HandlePacketResult msgPLI_ENTERLEVEL(CString& pPacket);
	//HandlePacketResult msgPLI_VERIFYWANTSEND(CString& pPacket);
	//HandlePacketResult msgPLI_UPDATECLASS(CString& pPacket);
	//HandlePacketResult msgPLI_RAWDATA(CString& pPacket);

	HandlePacketResult msgPLI_PROFILEGET(CString& pPacket);
	HandlePacketResult msgPLI_PROFILESET(CString& pPacket);

	//HandlePacketResult msgPLI_NPCSERVERQUERY(CString& pPacket);

	HandlePacketResult msgPLI_REQUESTTEXT(CString& pPacket);
	HandlePacketResult msgPLI_SENDTEXT(CString& pPacket);

	//HandlePacketResult msgPLI_UPDATEGANI(CString& pPacket);
	//HandlePacketResult msgPLI_UPDATESCRIPT(CString& pPacket);
	//HandlePacketResult msgPLI_UPDATEPACKAGEREQUESTFILE(CString& pPacket);

protected:
	// Cyclic, we have to create in the constructor.
	// BabyDI_INJECT(Server, m_server);
	Server* m_server = nullptr;

	// Socket Variables
	CSocket* m_playerSock;
	CString m_recvBuffer;

	// Variables
	PlayerID m_id = 0;
	int m_type = PLTYPE_AWAIT;
	int m_versionId = CLVER_UNKNOWN;
	std::string m_version;
	std::string m_serverName;
	uint8_t m_statusMsg = 0;
	uint8_t m_additionalFlags = 0;
	std::set<std::string> m_channelList;
	clock::time_point m_lastData;
	uint8_t m_encryptionKey = 0;
	int64_t m_accountIp = 0;
	uint16_t m_udpport = 0;
	int64_t m_deviceId = 0;
	std::array<std::optional<clock::time_point>, PLAYERPROP_COUNT> m_savedModTime;

	uint8_t m_horseBombCount = 0;
	uint8_t m_carrySprite = 0xFF;
	PlayerListCategory m_playerListCategory = PlayerListCategory::PLAYERLIST;
	NPCID m_attachNPC = 0;
	NPCID m_carryNPC = 0;
	std::array<uint8_t, 5> m_effectColors{0, 0, 0, 0, 0};

	std::vector<CString> m_privateMessageServerList;
	std::unordered_map<PlayerID, std::shared_ptr<Player>> m_externalPlayers;
	IdGenerator<PlayerID> m_externalPlayerIdGenerator{PLAYERID_GEN_EXTERNAL};

	bool m_loaded = false;
	bool m_isExternal = false;
	CString m_npcserverPort;
	CString m_guild;

	// File queue.
	CFileQueue m_fileQueue;

	int getVersionIDByVersion(const CString& versionInput) const;
};

using PlayerPtr = std::shared_ptr<Player>;
using PlayerWeakPtr = std::weak_ptr<Player>;

//----------------------------

template<typename T>
concept DerivedFromPlayer = std::is_base_of_v<Player, T>;

template<DerivedFromPlayer P>
auto players_of_type(const std::unordered_map<PlayerID, PlayerPtr>& range)
{
	// clang-format off
	using newpair = std::pair<const PlayerID, std::shared_ptr<P>>;
	return range
		| std::views::filter([](auto& kvp) { return dynamic_cast<P*>(kvp.second.get()) != nullptr; })
		| std::views::transform([](auto& kvp) { return newpair(kvp.first, std::dynamic_pointer_cast<P>(kvp.second)); });
	// clang-format on
}

inline bool Player::isLoggedIn() const
{
	return (m_type != PLTYPE_AWAIT && m_id > 0);
}

inline PlayerID Player::getId() const
{
	return m_id;
}

inline void Player::setId(PlayerID pId)
{
	m_id = pId;
}

inline bool Player::isPaused() const noexcept
{
	return (account.status & PLSTATUS_PAUSED);
}

inline PixelRectangleArea Player::getBoundingBox() const noexcept
{
	return {getGlobalPosition(), {48, 48, 48}};
}

inline PixelRectangleArea Player::getCollisionBoundingBox() const noexcept
{
	// Check if we are in a state that can't be collided with.
	if (isPaused() || isInNoPkLevel())
		return {{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), 0}, {0, 0, 0}};

	return {getGlobalPosition().translate(8, 16), {32, 32, 48}};
}

inline PixelPosition Player::getGlobalPosition() const noexcept
{
	auto pos = account.character.getGlobalPosition();
	pos.z() = static_cast<int32_t>(getCalculatedTileZ() * 16);
	return pos;
}

inline LocalPixelPosition Player::getLocalPosition() const noexcept
{
	auto pos = account.character.getLocalPosition();
	pos.z() = static_cast<int16_t>(getCalculatedTileZ() * 16);
	return pos;
}

inline TilePosition Player::getTilePosition() const noexcept
{
	auto pos = account.character.getTilePosition();
	pos.z() = getCalculatedTileZ();
	return pos;
}

inline PixelPosition Player::getSubLevelOrigin() const noexcept
{
	return PixelPosition{account.character.mapX * 1024, account.character.mapY * 1024, 0};
}

inline MapPosition Player::getMapPosition() const noexcept
{
	return MapPosition{account.character.mapX, account.character.mapY, 0};
}

inline bool Player::inChatChannel(const std::string& channel) const
{
	return m_channelList.find(channel) != m_channelList.end();
}

inline bool Player::addChatChannel(const std::string& channel)
{
	auto res = m_channelList.insert(channel);
	return res.second;
}

inline bool Player::removeChatChannel(const std::string& channel)
{
	m_channelList.erase(channel);
	return false;
}

inline std::string_view Player::getLanguage() const
{
	return account.language;
}

//----------------------------

inline void Player::recordCurrentPropModTime()
{
	m_savedModTime = modTime;
}

//----------------------------

// Defines the mapping of PlayerProp to PropertyContainer.
#define FOR_LIST_OF_PLAYER_PROPS(DO)                                                                                           \
	DO(PlayerProp::NICKNAME, PropertyString, account.character.nickName)                                                       \
	DO(PlayerProp::FULLHEARTS, PropertyNumeric<GBYTE1>, account.maxHitpoints)                                                  \
	DO(PlayerProp::HALFHEARTS, PropertyNumeric<GBYTE1>, account.character.hitpointsInHalves)                                   \
	DO(PlayerProp::GRALATS, PropertyNumeric<GBYTE3>, account.character.gralats)                                                \
	DO(PlayerProp::ARROWS, PropertyNumeric<GBYTE1>, account.character.arrows)                                                  \
	DO(PlayerProp::BOMBS, PropertyNumeric<GBYTE1>, account.character.bombs)                                                    \
	DO(PlayerProp::GLOVEPOWER, PropertyNumeric<GBYTE1>, account.character.glovePower)                                          \
	DO(PlayerProp::BOMBPOWER, PropertyNumeric<GBYTE1>, account.character.bombPower)                                            \
	DO(PlayerProp::SWORDIMAGE, PropertySwordPower, account.character.swordImage, account.character.swordPower)                 \
	DO(PlayerProp::SHIELDIMAGE, PropertyShieldPower, account.character.shieldImage, account.character.shieldPower)             \
	DO(PlayerProp::GANI, PropertyGaniOrBowGif, account.character.gani, account.character.bowPower, account.character.bowImage) \
	DO(PlayerProp::HEADIMAGE, PropertyHeadGif, account.character.headImage)                                                    \
	DO(PlayerProp::MESSAGE, PropertyString, account.character.chatMessage)                                                     \
	DO(PlayerProp::COLORS, PropertyColors, account.character.colors)                                                           \
	DO(PlayerProp::ID, PropertyNumeric<GBYTE2>, m_id)                                                                          \
	DO(PlayerProp::X, PropertyTileCoordinate, account.character.localPixelX)                                                   \
	DO(PlayerProp::Y, PropertyTileCoordinate, account.character.localPixelY)                                                   \
	DO(PlayerProp::SPRITE, PropertySprite, account.character.sprite, account.character.direction)                              \
	DO(PlayerProp::STATUS, PropertyNumeric<GBYTE1>, account.status)                                                            \
	DO(PlayerProp::CARRYSPRITE, PropertyUnsafeByte, m_carrySprite)                                                             \
	DO(PlayerProp::LEVEL, PropertyString, getLevelName())                                                                      \
	DO(PlayerProp::HORSEIMAGE, PropertyString, account.character.horseImage)                                                   \
	DO(PlayerProp::HORSEBUSHES, PropertyNumeric<GBYTE1>, m_horseBombCount)                                                     \
	DO(PlayerProp::EFFECTCOLORS, PropertyEffectColors, m_effectColors)                                                         \
	DO(PlayerProp::CARRYNPCID, PropertyNumeric<GBYTE3>, m_carryNPC)                                                            \
	DO(PlayerProp::APCOUNTER, PropertyNumeric<GBYTE2>, account.apCounter)                                                      \
	DO(PlayerProp::MAGICPOINTS, PropertyNumeric<GBYTE1>, account.character.mp)                                                 \
	DO(PlayerProp::KILLS, PropertyNumeric<GBYTE3>, account.kills)                                                              \
	DO(PlayerProp::DEATHS, PropertyNumeric<GBYTE3>, account.deaths)                                                            \
	DO(PlayerProp::ONLINESECONDS, PropertyNumeric<GBYTE3>, account.onlineSeconds)                                              \
	DO(PlayerProp::IPADDR, PropertyNumeric<GBYTE5>, m_accountIp)                                                               \
	DO(PlayerProp::UDPPORT, PropertyNumeric<GBYTE3>, m_udpport)                                                                \
	DO(PlayerProp::ALIGNMENT, PropertyNumeric<GBYTE1>, account.character.ap)                                                   \
	DO(PlayerProp::ADDITFLAGS, PropertyNumeric<GBYTE1>, m_additionalFlags)                                                     \
	DO(PlayerProp::ACCOUNTNAME, PropertyString, account.name)                                                                  \
	DO(PlayerProp::BODYIMAGE, PropertyString, account.character.bodyImage)                                                     \
	DO(PlayerProp::RATING, PropertyEloRating, account.eloRating, account.eloDeviation)                                         \
	DO(PlayerProp::GATTRIB1, PropertyString, account.character.ganiAttributes[0])                                              \
	DO(PlayerProp::GATTRIB2, PropertyString, account.character.ganiAttributes[1])                                              \
	DO(PlayerProp::GATTRIB3, PropertyString, account.character.ganiAttributes[2])                                              \
	DO(PlayerProp::GATTRIB4, PropertyString, account.character.ganiAttributes[3])                                              \
	DO(PlayerProp::GATTRIB5, PropertyString, account.character.ganiAttributes[4])                                              \
	DO(PlayerProp::ATTACHNPCID, PropertyAttachNPC, m_attachNPC)                                                                \
	DO(PlayerProp::GMAPLEVELX, PropertyNumeric<GBYTE1>, account.character.mapX)                                                \
	DO(PlayerProp::GMAPLEVELY, PropertyNumeric<GBYTE1>, account.character.mapY)                                                \
	DO(PlayerProp::Z, PropertyTileCoordinateZ, account.character.localPixelZ)                                                  \
	DO(PlayerProp::GATTRIB6, PropertyString, account.character.ganiAttributes[5])                                              \
	DO(PlayerProp::GATTRIB7, PropertyString, account.character.ganiAttributes[6])                                              \
	DO(PlayerProp::GATTRIB8, PropertyString, account.character.ganiAttributes[7])                                              \
	DO(PlayerProp::GATTRIB9, PropertyString, account.character.ganiAttributes[8])                                              \
	DO(PlayerProp::JOINLEAVELVL, PropertyNumeric<GBYTE1>, 1_ui8)                                                               \
	DO(PlayerProp::DISCONNECT, PropertyVoid)                                                                                   \
	DO(PlayerProp::LANGUAGE, PropertyString, account.language)                                                                 \
	DO(PlayerProp::PLAYERLISTSTATUS, PropertyNumeric<GBYTE1>, m_statusMsg)                                                     \
	DO(PlayerProp::GATTRIB10, PropertyString, account.character.ganiAttributes[9])                                             \
	DO(PlayerProp::GATTRIB11, PropertyString, account.character.ganiAttributes[10])                                            \
	DO(PlayerProp::GATTRIB12, PropertyString, account.character.ganiAttributes[11])                                            \
	DO(PlayerProp::GATTRIB13, PropertyString, account.character.ganiAttributes[12])                                            \
	DO(PlayerProp::GATTRIB14, PropertyString, account.character.ganiAttributes[13])                                            \
	DO(PlayerProp::GATTRIB15, PropertyString, account.character.ganiAttributes[14])                                            \
	DO(PlayerProp::GATTRIB16, PropertyString, account.character.ganiAttributes[15])                                            \
	DO(PlayerProp::GATTRIB17, PropertyString, account.character.ganiAttributes[16])                                            \
	DO(PlayerProp::GATTRIB18, PropertyString, account.character.ganiAttributes[17])                                            \
	DO(PlayerProp::GATTRIB19, PropertyString, account.character.ganiAttributes[18])                                            \
	DO(PlayerProp::GATTRIB20, PropertyString, account.character.ganiAttributes[19])                                            \
	DO(PlayerProp::GATTRIB21, PropertyString, account.character.ganiAttributes[20])                                            \
	DO(PlayerProp::GATTRIB22, PropertyString, account.character.ganiAttributes[21])                                            \
	DO(PlayerProp::GATTRIB23, PropertyString, account.character.ganiAttributes[22])                                            \
	DO(PlayerProp::GATTRIB24, PropertyString, account.character.ganiAttributes[23])                                            \
	DO(PlayerProp::GATTRIB25, PropertyString, account.character.ganiAttributes[24])                                            \
	DO(PlayerProp::GATTRIB26, PropertyString, account.character.ganiAttributes[25])                                            \
	DO(PlayerProp::GATTRIB27, PropertyString, account.character.ganiAttributes[26])                                            \
	DO(PlayerProp::GATTRIB28, PropertyString, account.character.ganiAttributes[27])                                            \
	DO(PlayerProp::GATTRIB29, PropertyString, account.character.ganiAttributes[28])                                            \
	DO(PlayerProp::GATTRIB30, PropertyString, account.character.ganiAttributes[29])                                            \
	DO(PlayerProp::OSTYPE, PropertyString, account.platform)                                                                   \
	DO(PlayerProp::TEXTCODEPAGE, PropertyNumeric<GBYTE3>, account.codePage)                                                    \
	DO(PlayerProp::ONLINESECONDS2, PropertyNumeric<GBYTE5>)                                                                    \
	DO(PlayerProp::X2, PropertyPixelCoordinate, account.character.localPixelX)                                                 \
	DO(PlayerProp::Y2, PropertyPixelCoordinate, account.character.localPixelY)                                                 \
	DO(PlayerProp::Z2, PropertyPixelCoordinate, account.character.localPixelZ)                                                 \
	DO(PlayerProp::PLAYERLISTCATEGORY, PropertyNumeric<GBYTE1>, (uint8_t)m_playerListCategory)                                 \
	DO(PlayerProp::COMMUNITYNAME, PropertyString, account.communityName)

//----------------------------

template<PlayerProp P, typename... Args>
PropertyContainer auto Player::constructPropFor(Args... values) const
{
	FOR_LIST_OF_PLAYER_PROPS(RETURN_CONSTRUCTPROPSFOR_CONSTEXPR);
	throw std::invalid_argument("Invalid PlayerProp type in constructPropFor");
}

template<PlayerProp P>
PropertyContainer auto Player::getProp() const
{
	FOR_LIST_OF_PLAYER_PROPS(RETURN_GETPROP_CONSTEXPR);
	throw std::invalid_argument("Invalid PlayerProp type in getProp");
}

template<PlayerProp P>
SetResults Player::setProp(SetBy setBy, PropertyContainer auto prop)
{
	return setProp(P, setBy, &prop);
}

template<PlayerProp P, typename... Args>
SetResults Player::setPropWith(SetBy setBy, Args... values)
{
	return setProp<P>(setBy, constructPropFor<P>(values...));
}

template<typename... Results>
	requires AllSameAs<SetResults, Results...>
void Player::sendPropsFromResults(const Results&... results)
{
	PropertySendResults send_results;
	(send_results.emplace_back(results, nullptr), ...);
	sendPropsFromResults(send_results);
}

void Player::sendPropsFromResults(std::ranges::forward_range auto&& results)
{
	PropertySendResults send_results;
	auto results_range = results | std::views::transform([](const SetResults& results)
	{
		return std::make_pair(results, nullptr);
	});
	for (auto&& r : results_range)
		send_results.emplace_back(r);

	sendPropsFromResults(send_results);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // PLAYER_H
