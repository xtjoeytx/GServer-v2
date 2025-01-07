#ifndef TPLAYER_H
#define TPLAYER_H

#include <map>
#include <memory>
#include <set>
#include <time.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <ranges>
#include <optional>

#include <CEncryption.h>
#include <CFileQueue.h>
#include <CSocket.h>
#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>
#include "BabyDI.h"

#include "Account.h"
#include "Server.h"
#include "player/PlayerProps.h"
#include "network/IPacketHandler.h"
#include "utilities/IdGenerator.h"

#ifdef V8NPCSERVER
	#include "scripting/interface/ScriptBindings.h"
#endif

class Level;
class Map;
class Weapon;

constexpr uint16_t EXTERNALPLAYERID_INIT = 16000;

enum class LevelItemType;

enum
{
	PLSETPROPS_SETBYPLAYER = 0x01, // if set, do serverside checks to prevent attributes from being changed
	PLSETPROPS_FORWARD = 0x02,     // forward data to other players
	PLSETPROPS_FORWARDSELF = 0x04, // forward data back to the player
};

struct ShootPacketNew
{
	// shoot(float x, float y, float z, float angle, float zangle, float strength, str ani, str aniparams)
	int16_t pixelx;
	int16_t pixely;
	int16_t pixelz;
	int8_t offsetx;
	int8_t offsety;
	int8_t sangle;
	int8_t sanglez;
	int8_t speed;
	int8_t gravity;
	CString gani;
	CString ganiArgs;
	CString shootParams;

	CString constructShootV1() const;
	CString constructShootV2() const;

	void debug();
};

struct CachedLevel
{
	CachedLevel(std::weak_ptr<Level> pLevel, time_t pModTime) : level(pLevel), modTime(pModTime) {}
	std::weak_ptr<Level> level;
	time_t modTime;
};

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
	Player(CSocket* pSocket, uint16_t pId);
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
	uint16_t getId() const;
	time_t getLastData() const { return m_lastData; }
	CString getGuild() const { return m_guild; }
	int getVersion() const { return m_versionId; }
	CString getVersionStr() const { return m_version; }
	CString getServerName() const { return m_serverName; }
	const CString& getPlatform() const { return m_os; }
	int64_t getDeviceId() const { return m_deviceId; }
	float getX() const { return account.character.pixelX / 16.0f; }
	float getY() const { return account.character.pixelY / 16.0f; }
	float getZ() const { return account.character.pixelZ / 16.0f; }

	// Set Properties
	void setNick(CString pNickName, bool force = false);
	void setId(uint16_t pId);
	void setLoaded(bool loaded) { this->m_loaded = loaded; }
	void setServerName(CString& tmpServerName) { m_serverName = tmpServerName; }
	void setChat(const CString& pChat);
	void setDeviceId(int64_t newDeviceId) { m_deviceId = newDeviceId; }

	// Prop-Manipulation
	virtual CString getProp(int pPropId) const;
	virtual bool getProp(CString& buffer, int pPropId) const;
	CString getProps(const PropList& props);
	CString getPropsRC();
	void setProps(CString& pPacket, uint8_t options, Player* rc = 0);
	void setPropsRC(CString& pPacket, Player* rc);
	void sendProps(const PropList& props);
	void exchangeMyPropsWithOthers();

	void deleteFlag(const std::string& pFlagName, bool sendToPlayer = false);
	void setFlag(const std::string& pFlagName, const CString& pFlagValue, bool sendToPlayer = false);
	CString getFlag(const std::string& pFlagName) const;

	virtual bool setLevel(const CString& pLevelName, time_t modTime = 0);

	// Socket-Functions
	void sendPacket(CString pPacket, bool appendNL = true);
	bool sendFile(const CString& pFile);
	bool sendFile(const CString& pPath, const CString& pFile);
	void setReceivedBuffer(const CString& buffer) { m_recvBuffer = buffer; }

	// Type of player
	bool isAdminIp();
	bool isStaff();
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
	bool addWeapon(const std::string& name);
	bool addWeapon(std::shared_ptr<Weapon> weapon);
	bool deleteWeapon(LevelItemType defaultWeapon);
	bool deleteWeapon(const std::string& name);
	bool deleteWeapon(std::shared_ptr<Weapon> weapon);

	CString translate(const CString& pKey) const;

	// Misc functions.
	void disconnect();

	bool addPMServer(CString& option);
	bool remPMServer(CString& option);
	bool inChatChannel(const std::string& channel) const;
	bool addChatChannel(const std::string& channel);
	bool removeChatChannel(const std::string& channel);
	bool updatePMPlayers(CString& servername, CString& players);
	bool pmExternalPlayer(CString servername, CString account, CString& pmMessage);
	std::vector<CString> getPMServerList();
	std::shared_ptr<Player> getExternalPlayer(const uint16_t id, bool includeRC = true) const;
	std::shared_ptr<Player> getExternalPlayer(const CString& account, bool includeRC = true) const;

#ifdef V8NPCSERVER
	bool isProcessed() const { return m_processRemoval; }
	void setProcessed() { m_processRemoval = true; }

	// NPC-Server Functionality
	void sendNCAddr();

	inline IScriptObject<Player>* getScriptObject() const
	{
		return m_scriptObject.get();
	}

	inline void setScriptObject(std::unique_ptr<IScriptObject<Player>> object)
	{
		m_scriptObject = std::move(object);
	}
#endif

public:
	Account account;

protected:
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
	//HandlePacketResult msgPLI_SHOWIMG(CString& pPacket);
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
	//HandlePacketResult msgPLI_MAPINFO(CString& pPacket);
	//HandlePacketResult msgPLI_SHOOT(CString& pPacket);
	//HandlePacketResult msgPLI_SHOOT2(CString& pPacket);
	//HandlePacketResult msgPLI_SERVERWARP(CString& pPacket);
	//HandlePacketResult msgPLI_PROCESSLIST(CString& pPacket);
	//HandlePacketResult msgPLI_UNKNOWN46(CString& pPacket);
	//HandlePacketResult msgPLI_VERIFYWANTSEND(CString& pPacket);
	//HandlePacketResult msgPLI_UPDATECLASS(CString& pPacket);
	//HandlePacketResult msgPLI_RAWDATA(CString& pPacket);

	HandlePacketResult msgPLI_PROFILEGET(CString& pPacket);
	HandlePacketResult msgPLI_PROFILESET(CString& pPacket);
	
	//HandlePacketResult msgPLI_NPCSERVERQUERY(CString& pPacket);

#ifdef V8NPCSERVER
	HandlePacketResult msgPLI_NC_NPCGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCDELETE(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCRESET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCSCRIPTGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCWARP(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCFLAGSGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCSCRIPTSET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCFLAGSSET(CString& pPacket);
	HandlePacketResult msgPLI_NC_NPCADD(CString& pPacket);
	HandlePacketResult msgPLI_NC_CLASSEDIT(CString& pPacket);
	HandlePacketResult msgPLI_NC_CLASSADD(CString& pPacket);
	HandlePacketResult msgPLI_NC_LOCALNPCSGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_WEAPONLISTGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_WEAPONGET(CString& pPacket);
	HandlePacketResult msgPLI_NC_WEAPONADD(CString& pPacket);
	HandlePacketResult msgPLI_NC_WEAPONDELETE(CString& pPacket);
	HandlePacketResult msgPLI_NC_CLASSDELETE(CString& pPacket);
	HandlePacketResult msgPLI_NC_LEVELLISTGET(CString& pPacket);
#endif

	HandlePacketResult msgPLI_REQUESTTEXT(CString& pPacket);
	HandlePacketResult msgPLI_SENDTEXT(CString& pPacket);

	//HandlePacketResult msgPLI_UPDATEGANI(CString& pPacket);
	//HandlePacketResult msgPLI_UPDATESCRIPT(CString& pPacket);
	//HandlePacketResult msgPLI_UPDATEPACKAGEREQUESTFILE(CString& pPacket);

protected:
	BabyDI_INJECT(Server, m_server);

	// Login functions.
	//bool sendLoginClient();
	bool sendLoginNC();
	//bool sendLoginRC();

	// Socket Variables
	CSocket* m_playerSock;
	CString m_recvBuffer;

	// Variables
	uint16_t m_id = 0;
	int m_type = PLTYPE_AWAIT;
	int m_versionId = CLVER_UNKNOWN;
	CString m_version;
	CString m_os{ "wind" };
	CString m_serverName;
	uint8_t m_statusMsg = 0;
	int m_envCodePage = 1252;
	std::set<std::string> m_channelList;
	time_t m_lastData;
	unsigned char m_encryptionKey = 0;
	unsigned long m_accountIp = 0;
	int64_t m_deviceId = 0;

	std::vector<CString> m_privateMessageServerList;
	std::unordered_map<PlayerID, std::shared_ptr<Player>> m_externalPlayers;
	IdGenerator<PlayerID> m_externalPlayerIdGenerator{ EXTERNALPLAYERID_INIT };

	bool m_loaded = false;
	bool m_isExternal = false;
	CString m_npcserverPort;
	CString m_guild;

	// File queue.
	CFileQueue m_fileQueue;

#ifdef V8NPCSERVER
	bool m_processRemoval = false;
	std::unique_ptr<IScriptObject<Player>> m_scriptObject;
#endif

	int getVersionIDByVersion(const CString& versionInput) const;
};

using PlayerPtr = std::shared_ptr<Player>;
using PlayerWeakPtr = std::weak_ptr<Player>;

template<typename T>
concept DerivedFromPlayer = std::is_base_of_v<Player, T>;

template<DerivedFromPlayer P>
auto players_of_type(const std::unordered_map<uint16_t, PlayerPtr>& range)
{
	using oldpair = std::unordered_map<uint16_t, PlayerPtr>::value_type;
	using newpair = std::pair<const uint16_t, std::shared_ptr<P>>;
	return range
		| std::views::filter([](auto& kvp) { return dynamic_cast<P*>(kvp.second.get()) != nullptr; })
		| std::views::transform([](auto& kvp) { return newpair(kvp.first, std::dynamic_pointer_cast<P>(kvp.second)); });
}

inline bool Player::isLoggedIn() const
{
	return (m_type != PLTYPE_AWAIT && m_id > 0);
}

inline uint16_t Player::getId() const
{
	return m_id;
}

inline void Player::setId(uint16_t pId)
{
	m_id = pId;
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

#endif // TPLAYER_H
