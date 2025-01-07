#ifndef PLAYERCLIENT_H
#define PLAYERCLIENT_H

#include <string_view>

#include "object/Player.h"

using namespace std::literals::string_view_literals;

/*
	Default Files.
*/
template<class... Arrays>
consteval auto join_arrays(Arrays... arrays)
{
	return std::apply([](auto... args) { return std::array{ args... }; }, std::tuple_cat(arrays...));
}

constexpr std::array<std::string_view, 39> DefaultGanis = {
	"carried.gani", "carry.gani", "carrystill.gani", "carrypeople.gani", "dead.gani", "def.gani", "ghostani.gani", "grab.gani", "gralats.gani", "hatoff.gani",
	"haton.gani", "hidden.gani", "hiddenstill.gani", "hurt.gani", "idle.gani", "kick.gani", "lava.gani", "lift.gani", "maps1.gani", "maps2.gani",
	"maps3.gani", "pull.gani", "push.gani", "ride.gani", "rideeat.gani", "ridefire.gani", "ridehurt.gani", "ridejump.gani", "ridestill.gani", "ridesword.gani",
	"shoot.gani", "sit.gani", "skip.gani", "sleep.gani", "spin.gani", "swim.gani", "sword.gani", "walk.gani", "walkslow.gani"
};
constexpr std::array<std::string_view, 3> DefaultBodies = { "body.png", "body2.png", "body3.png" };
constexpr std::array<std::string_view, 2> DefaultSwords = { "sword?.png", "sword?.gif" };
constexpr std::array<std::string_view, 2> DefaultShields = { "shield?.png", "shield?.gif" };
constexpr std::array<std::string_view, 30> DefaultWavs = {
	"arrow.wav", "arrowon.wav", "axe.wav", "bomb.wav", "chest.wav", "compudead.wav", "crush.wav", "dead.wav", "extra.wav", "fire.wav",
	"frog.wav", "frog2.wav", "goal.wav", "horse.wav", "horse2.wav", "item.wav", "item2.wav", "jump.wav", "lift.wav", "lift2.wav",
	"nextpage.wav", "put.wav", "sign.wav", "steps.wav", "steps2.wav", "stonemove.wav", "sword.wav", "swordon.wav", "thunder.wav", "water.wav"
};
constexpr std::array<std::string_view, 1> DefaultPngs = { "pics1.png" };
constexpr std::array DefaultFiles = join_arrays(DefaultGanis, DefaultBodies, DefaultSwords, DefaultShields, DefaultWavs, DefaultPngs);


class PlayerClient : public Player
{
	// TODO: Need to refactor some Player functions like sendFile so this can be removed.
	friend class Player;

public:
	PlayerClient(CSocket* pSocket, uint16_t pId);
	virtual ~PlayerClient() {}
	virtual void cleanup() override;

public:
	// Main methods.
	virtual void doMain() override;
	virtual bool doTimedEvents() override;
	virtual bool handleLogin(CString& pPacket) override;
	virtual bool sendLogin() override;

	bool processChat(const CString& pChat);

	const CString& getGroup() const;
	void setGroup(const CString& group);

	std::weak_ptr<Map> getMap() const;
	void setMap(const std::shared_ptr<Map>& map);

	virtual CString getProp(int pPropId) const override;
	virtual bool getProp(CString& buffer, int pPropId) const override;

	// Level manipulation
	std::shared_ptr<Level> getLevel() const;
	std::pair<int, int> getMapPosition() const;
	bool warp(const CString& pLevelName, float pX, float pY, time_t modTime = 0);
	virtual bool setLevel(const CString& pLevelName, time_t modTime = 0) override;
	bool sendLevel(std::shared_ptr<Level> pLevel, time_t modTime, bool fromAdjacent = false);
	bool sendLevel141(std::shared_ptr<Level> pLevel, time_t modTime, bool fromAdjacent = false);
	bool leaveLevel(bool resetCache = false);
	time_t getCachedLevelModTime(const Level* level) const;
	void resetLevelCache(const Level* level);

	bool hasSeenFile(const std::string& file) const;

	void disableWeapons();
	void enableWeapons();
	void freezePlayer();
	void unfreezePlayer();
	void sendRPGMessage(const CString& message);
	void sendSignMessage(const CString& message);
	void setAni(CString gani);
	void setLastChatTime(time_t time) { m_lastChat = time; }
	void setLastMovementTime(time_t time);
	void dropItemsOnDeath();
	NPCID getCarryNpcId() const { return m_carryNpcId; }
	void setCarryNpcId(NPCID id) { m_carryNpcId = id; }

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
	HandlePacketResult msgPLI_SHOWIMG(CString& pPacket);
	HandlePacketResult msgPLI_HURTPLAYER(CString& pPacket);
	HandlePacketResult msgPLI_EXPLOSION(CString& pPacket);
	HandlePacketResult msgPLI_PRIVATEMESSAGE(CString& pPacket);
	HandlePacketResult msgPLI_NPCWEAPONDEL(CString& pPacket);
	HandlePacketResult msgPLI_WEAPONADD(CString& pPacket);
	HandlePacketResult msgPLI_UPDATEFILE(CString& pPacket);
	HandlePacketResult msgPLI_ADJACENTLEVEL(CString& pPacket);
	HandlePacketResult msgPLI_HITOBJECTS(CString& pPacket);
	HandlePacketResult msgPLI_TRIGGERACTION(CString& pPacket);
	HandlePacketResult msgPLI_MAPINFO(CString& pPacket);
	HandlePacketResult msgPLI_SHOOT(CString& pPacket);
	HandlePacketResult msgPLI_SHOOT2(CString& pPacket);
	HandlePacketResult msgPLI_SERVERWARP(CString& pPacket);
	HandlePacketResult msgPLI_PROCESSLIST(CString& pPacket);
	HandlePacketResult msgPLI_UNKNOWN46(CString& pPacket);
	HandlePacketResult msgPLI_UPDATECLASS(CString& pPacket);
	HandlePacketResult msgPLI_UPDATEGANI(CString& pPacket);
	HandlePacketResult msgPLI_UPDATESCRIPT(CString& pPacket);
	HandlePacketResult msgPLI_VERIFYWANTSEND(CString& pPacket);
	HandlePacketResult msgPLI_UPDATEPACKAGEREQUESTFILE(CString& pPacket);

protected:
	// Collision detection stuff.
	bool testSign();
	void testTouch();

	// Misc.
	bool spawnLevelItem(CString& pPacket, bool playerDrop = true);
	bool removeItem(LevelItemType itemType);

protected:
	time_t m_lastMovement, m_lastSave, m_last1m;
	time_t m_lastChat = 0;
	time_t m_lastMessage = 0;
	time_t m_lastNick = 0;
	std::vector<std::unique_ptr<CachedLevel>> m_cachedLevels;
	std::map<CString, std::shared_ptr<Level>> m_singleplayerLevels;
	std::weak_ptr<Map> m_pmap;
	std::weak_ptr<Level> m_currentLevel;

	uint16_t m_udpport = 0;

	uint8_t m_horseBombCount = 0;
	uint8_t m_carrySprite = -1;		// TODO: Make sure this is correct.
	NPCID m_attachNPC = 0;
	NPCID m_carryNpcId = 0;
	bool m_carryNpcThrown = false;
	std::unordered_set<std::string> m_knownFiles;

	bool m_grMovementUpdated = false;
	bool m_firstLevel = true;
	CString m_grMovementPackets;
	CString m_levelGroup;
	CString m_grExecParameterList;

};

// So stupid that I have to do this.
inline CString PlayerClient::getProp(int pPropId) const
{
	return Player::getProp(pPropId);
}

inline const CString& PlayerClient::getGroup() const
{
	return m_levelGroup;
}

inline void PlayerClient::setGroup(const CString& group)
{
	m_levelGroup = group;
}

inline std::weak_ptr<Map> PlayerClient::getMap() const
{
	return m_pmap;
}

inline void PlayerClient::setMap(const std::shared_ptr<Map>& map)
{
	m_pmap = map;
}

inline bool PlayerClient::hasSeenFile(const std::string& file) const
{
	return m_knownFiles.find(file) != m_knownFiles.end();
}

inline void PlayerClient::setLastMovementTime(time_t time)
{
	m_lastMovement = time;
	m_grMovementUpdated = true;
}

#endif // PLAYERCL_H
