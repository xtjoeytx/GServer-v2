#ifndef PLAYERCLIENT_H
#define PLAYERCLIENT_H

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <CSocket.h>

#include <CString.h>

#include <level/Level.h>
#include <level/LevelItem.h>
#include <network/IPacketHandler.h>
#include <object/Player.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/PropertySerializers.h>

using namespace std::literals::string_view_literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

template<class... Arrays>
consteval auto join_arrays(Arrays... arrays)
{
	return std::apply([](auto... args)
	{
		return std::array{args...};
	}, std::tuple_cat(arrays...));
}

constexpr std::array<std::string_view, 39> DefaultGanis = {
	"carried.gani", "carry.gani", "carrystill.gani", "carrypeople.gani", "dead.gani", "def.gani", "ghostani.gani", "grab.gani", "gralats.gani", "hatoff.gani",
	"haton.gani", "hidden.gani", "hiddenstill.gani", "hurt.gani", "idle.gani", "kick.gani", "lava.gani", "lift.gani", "maps1.gani", "maps2.gani",
	"maps3.gani", "pull.gani", "push.gani", "ride.gani", "rideeat.gani", "ridefire.gani", "ridehurt.gani", "ridejump.gani", "ridestill.gani", "ridesword.gani",
	"shoot.gani", "sit.gani", "skip.gani", "sleep.gani", "spin.gani", "swim.gani", "sword.gani", "walk.gani", "walkslow.gani"
};
constexpr std::array<std::string_view, 3> DefaultBodies = {"body.png", "body2.png", "body3.png"};
constexpr std::array<std::string_view, 2> DefaultSwords = {"sword?.png", "sword?.gif"};
constexpr std::array<std::string_view, 2> DefaultShields = {"shield?.png", "shield?.gif"};
constexpr std::array<std::string_view, 30> DefaultWavs = {
	"arrow.wav", "arrowon.wav", "axe.wav", "bomb.wav", "chest.wav", "compudead.wav", "crush.wav", "dead.wav", "extra.wav", "fire.wav",
	"frog.wav", "frog2.wav", "goal.wav", "horse.wav", "horse2.wav", "item.wav", "item2.wav", "jump.wav", "lift.wav", "lift2.wav",
	"nextpage.wav", "put.wav", "sign.wav", "steps.wav", "steps2.wav", "stonemove.wav", "sword.wav", "swordon.wav", "thunder.wav", "water.wav"
};
constexpr std::array<std::string_view, 1> DefaultPngs = {"pics1.png"};
constexpr std::array DefaultFiles = join_arrays(DefaultGanis, DefaultBodies, DefaultSwords, DefaultShields, DefaultWavs, DefaultPngs);

//----------------------------

template<typename T = StaticLevelData>
struct CachedLevel
{
	std::weak_ptr<T> level;
	clock::time_point lastEnteredTime;
};

//----------------------------

class PlayerClient : public Player
{
	// TODO: Need to refactor some Player functions like sendFile so this can be removed.
	friend class Player;

public:
	PlayerClient(CSocket* pSocket, PlayerID pId);
	virtual ~PlayerClient();
	virtual void cleanup() override;

public:
	std::shared_ptr<PlayerClient> self() { return std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); }

public:
	// Main methods.
	virtual void doMain() override;
	virtual bool doTimedEvents() override;
	virtual bool handleLogin(CString& pPacket) override;
	virtual bool sendLogin() override;

	bool processChat(const CString& pChat);

	[[inline]] const std::string& getGroup() const;
	void setGroup(std::string_view group);

	virtual double getCalculatedTileZ() const noexcept override;

	// Level manipulation
	virtual std::string getLevelName() const override;
	std::shared_ptr<Level> getLevel() const;
	std::shared_ptr<SubLevel> getSubLevel() const;

public:
	// Forcibly move a player (the client doesn't know it is transitioning levels).
	virtual bool warp(std::string_view levelName, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime = std::nullopt) override;
	virtual bool warp(std::shared_ptr<Level> level, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime = std::nullopt) override;

	// Place the player in a new level (the client knows it is transitioning levels).
	virtual bool enterLevel(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime = std::nullopt) override;
	using Player::enterLevel;

	virtual bool leaveLevel() override;
	virtual bool leaveSubLevel(std::shared_ptr<SubLevel> subLevel) override;

	virtual bool sendStaticLevelData(std::shared_ptr<StaticLevelData> staticLevelData, std::shared_ptr<SubLevel> subLevel, std::optional<clock::time_point> clientCachedTime = std::nullopt) override;
	virtual bool sendDynamicLevelData(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime = std::nullopt) override;

	void checkAndInformIfLevelLeader();
	void informPlayerIsLevelLeader();

public:
	std::optional<clock::time_point> getLevelLastEnteredTime(const StaticLevelData* level) const;
	std::optional<clock::time_point> getLevelLastEnteredTime(const SubLevel* level, std::string_view group = ""sv) const;
	void resetLevelCache(const StaticLevelData* level);
	void resetLevelCache(const SubLevel* level, std::string_view group = ""sv);
	void resetLevelCache(std::string_view group);

public:
	[[inline]] bool hasSeenFile(const std::string& file) const;
	[[inline]] void setLastChatTime(clock::time_point time);
	[[inline]] void setLastMovementTime(clock::time_point time);
	void dropItemsOnDeath();

public:
	void disableWeapons();
	void enableWeapons();
	void freezePlayer();
	void unfreezePlayer();
	void sendRPGMessage(std::string message);
	void sendSignMessage(std::string message);

public:
	void testForTouch(SetResults& result, uint8_t movementDirection);
	bool testForSigns(SetResults& result, uint8_t movementDirection);
	bool testForLinks(SetResults& result, uint8_t movementDirection);

protected:
	virtual HandlePacketResult handlePacket(std::optional<uint8_t> id, CString& packet) override;

public:
	HandlePacketResult msgPLI_LEVELWARP(CString& pPacket);
	HandlePacketResult msgPLI_BOARDMODIFY(CString& pPacket);
	HandlePacketResult msgPLI_REQUESTUPDATEBOARD(CString& pPacket);
	HandlePacketResult msgPLI_NPCPROPS(CString& pPacket);
	HandlePacketResult msgPLI_BOMBADD(CString& pPacket);
	HandlePacketResult msgPLI_BOMBDEL(CString& pPacket);
	HandlePacketResult msgPLI_HORSEADD(CString& pPacket);
	HandlePacketResult msgPLI_HORSEDEL(CString& pPacket);
	HandlePacketResult msgPLI_ARROWADD(CString& pPacket);
	HandlePacketResult msgPLI_FIRESPY(CString& pPacket);
	HandlePacketResult msgPLI_THROWCARRIED(CString& pPacket);
	HandlePacketResult msgPLI_ITEMADD(CString& pPacket);
	HandlePacketResult msgPLI_ITEMDEL(CString& pPacket);
	HandlePacketResult msgPLI_CLAIMPKER(CString& pPacket);
	HandlePacketResult msgPLI_BADDYPROPS(CString& pPacket);
	HandlePacketResult msgPLI_BADDYHURT(CString& pPacket);
	HandlePacketResult msgPLI_BADDYADD(CString& pPacket);
	HandlePacketResult msgPLI_FLAGSET(CString& pPacket);
	HandlePacketResult msgPLI_FLAGDEL(CString& pPacket);
	HandlePacketResult msgPLI_OPENCHEST(CString& pPacket);
	HandlePacketResult msgPLI_PUTNPC(CString& pPacket);
	HandlePacketResult msgPLI_NPCDEL(CString& pPacket);
	HandlePacketResult msgPLI_WANTFILE(CString& pPacket);
	HandlePacketResult msgPLI_SHOWIMGPLAYER(CString& pPacket);
	HandlePacketResult msgPLI_HURTPLAYER(CString& pPacket);
	HandlePacketResult msgPLI_EXPLOSION(CString& pPacket);
	HandlePacketResult msgPLI_PRIVATEMESSAGE(CString& pPacket);
	HandlePacketResult msgPLI_NPCWEAPONDEL(CString& pPacket);
	HandlePacketResult msgPLI_WEAPONADD(CString& pPacket);
	HandlePacketResult msgPLI_UPDATEFILE(CString& pPacket);
	HandlePacketResult msgPLI_ADJACENTLEVEL(CString& pPacket);
	HandlePacketResult msgPLI_HITOBJECTS(CString& pPacket);
	HandlePacketResult msgPLI_TRIGGERACTION(CString& pPacket);
	HandlePacketResult msgPLI_TAMPERCHECK(CString& pPacket);
	HandlePacketResult msgPLI_SHOOT(CString& pPacket);
	HandlePacketResult msgPLI_SHOOT2(CString& pPacket);
	HandlePacketResult msgPLI_SERVERWARP(CString& pPacket);
	HandlePacketResult msgPLI_PROCESSLIST(CString& pPacket);
	HandlePacketResult msgPLI_ENTERLEVEL(CString& pPacket);
	HandlePacketResult msgPLI_UPDATECLASS(CString& pPacket);
	HandlePacketResult msgPLI_UPDATEGANI(CString& pPacket);
	HandlePacketResult msgPLI_UPDATESCRIPT(CString& pPacket);
	HandlePacketResult msgPLI_VERIFYWANTSEND(CString& pPacket);
	HandlePacketResult msgPLI_UPDATEPACKAGEREQUESTFILE(CString& pPacket);

protected:
	bool dropItem(const PixelPosition& position, LevelItemType item);
	bool removeItem(LevelItemType itemType);
	props::SetResults addItem(LevelItemType itemType, props::SetBy setBy = props::SetBy::SERVER);
	void addItem(inform_client_t, LevelItemType itemType, props::SetBy setBy = props::SetBy::SERVER);

protected:
	clock::time_point m_lastMovement, m_lastSave, m_last1m;
	clock::time_point m_lastChat;
	clock::time_point m_lastMessage;
	clock::time_point m_lastNick;
	std::vector<std::unique_ptr<CachedLevel<StaticLevelData>>> m_cachedStaticLevels;
	string_map<std::vector<std::unique_ptr<CachedLevel<SubLevel>>>> m_cachedDynamicLevels;
	std::map<CString, std::shared_ptr<Level>> m_singleplayerLevels;
	std::weak_ptr<Level> m_currentLevel;

	std::unordered_set<std::string> m_knownFiles;

	bool m_grMovementUpdated = false;
	CString m_grMovementPackets;
	CString m_grExecParameterList;
};

using PlayerClientPtr = std::shared_ptr<PlayerClient>;

//----------------------------

inline const std::string& PlayerClient::getGroup() const
{
	return account.groupName;
}

inline bool PlayerClient::hasSeenFile(const std::string& file) const
{
	return m_knownFiles.find(file) != m_knownFiles.end();
}

inline void PlayerClient::setLastChatTime(clock::time_point time)
{
	m_lastChat = time;
}

inline void PlayerClient::setLastMovementTime(clock::time_point time)
{
	m_lastMovement = time;
	m_grMovementUpdated = true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // PLAYERCLIENT_H
