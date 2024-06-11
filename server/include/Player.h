#ifndef TPLAYER_H
#define TPLAYER_H

#include "Account.h"
#include "CEncryption.h"
#include "CFileQueue.h"
#include "CSocket.h"
#include "IEnums.h"
#include "TPacket.h"
#include <ctime>
#include <map>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

#ifdef V8NPCSERVER

	#include "ScriptBindings.h"
	#include "wolfssl/ssl.h"

#endif

class Level;

class Server;

class Map;

class Weapon;

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

class Player : public Account, public CSocketStub, public std::enable_shared_from_this<Player>
{
public:
	// Required by CSocketStub.
	bool onRecv();

	bool onSend();

	bool onRegister() { return true; }

	void onUnregister();

	SOCKET getSocketHandle() { return m_playerSock->getHandle(); }

	bool canRecv();

	bool canSend();

	// Constructor - Deconstructor
	Player(Server* pServer, CSocket* pSocket, uint16_t pId);

	~Player();

	void cleanup();

	// Manage Account
	bool isLoggedIn() const;

	bool sendLogin();

	// Get Properties
	CSocket* getSocket() { return m_playerSock; }

	std::shared_ptr<Level> getLevel() const;

	std::weak_ptr<Map> getMap() { return m_pmap; }

	CString getGroup() { return m_levelGroup; }

	uint16_t getId() const;

	time_t getLastData() const { return m_lastData; }

	CString getGuild() const { return m_guild; }

	int getVersion() const { return m_versionId; }

	CString getVersionStr() const { return m_version; }

	bool isUsingFileBrowser() const { return m_isFtp; }

	CString getServerName() const { return m_serverName; }

	const CString& getPlatform() const { return m_os; }

	std::pair<int, int> getMapPosition() const;

	// Set Properties
	void setChat(const CString& pChat);

	void setNick(CString pNickName, bool force = false);

	void setId(uint16_t pId);

	void setLoaded(bool loaded) { this->m_loaded = loaded; }

	void setGroup(CString group) { m_levelGroup = group; }

	void deleteFlag(const std::string& pFlagName, bool sendToPlayer = false);

	void setFlag(const std::string& pFlagName, const CString& pFlagValue, bool sendToPlayer = false);

	void setMap(std::shared_ptr<Map> map) { m_pmap = map; }

	void setServerName(CString& tmpServerName) { m_serverName = tmpServerName; }

	// Level manipulation
	bool warp(const CString& pLevelName, float pX, float pY, time_t modTime = 0);

	bool setLevel(const CString& pLevelName, time_t modTime = 0);

	bool sendLevel(std::shared_ptr<Level> pLevel, time_t modTime, bool fromAdjacent = false);

	bool sendLevel141(std::shared_ptr<Level> pLevel, time_t modTime, bool fromAdjacent = false);

	bool leaveLevel(bool resetCache = false);

	time_t getCachedLevelModTime(const Level* level) const;

	void resetLevelCache(const Level* level);

	// Prop-Manipulation
	inline CString getProp(int pPropId) const;

	void getProp(CString& buffer, int pPropId) const;

	CString getProps(const bool* pProps, int pCount);

	CString getPropsRC();

	void setProps(CString& pPacket, uint8_t options, Player* rc = 0);

	void sendProps(const bool* pProps, int pCount);

	void setPropsRC(CString& pPacket, Player* rc);

	// Socket-Functions
	bool doMain();

	void sendPacket(const PlayerOutPacket& packet, bool sendNow = false, bool appendNL = true)
	{
		if (m_newProtocol)
		{
			sendPacketNewProtocol((unsigned char)packet.Id, packet.Data, sendNow, appendNL);
		}
		else
		{
			sendPacketOldProtocol(CString() >> (char)packet.Id << packet.Data, appendNL);
		}
	};

	bool sendFile(const CString& pFile);

	bool sendFile(const CString& pPath, const CString& pFile);

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

	int getType() const { return m_type; }

	void setType(int val) { m_type = val; }

	// Misc functions.
	bool doTimedEvents();

	void disconnect();

	bool processChat(CString pChat);

	bool addWeapon(LevelItemType defaultWeapon);

	bool addWeapon(const std::string& name);

	bool addWeapon(std::shared_ptr<Weapon> weapon);

	bool deleteWeapon(LevelItemType defaultWeapon);

	bool deleteWeapon(const std::string& name);

	bool deleteWeapon(std::shared_ptr<Weapon> weapon);

	void disableWeapons();

	void enableWeapons();

	void freezePlayer();

	void unfreezePlayer();

	void sendRPGMessage(const CString& message);

	void sendSignMessage(const CString& message);

	void setAni(CString gani);

	bool hasSeenFile(const std::string& file) const;

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

	// Packet-Functions
	static bool created;

	static void createFunctions();

	bool msgPLI_NULL(CString& pPacket);

	bool msgPLI_LOGIN(CString& pPacket);

	bool msgPLI_LEVELWARP(CString& pPacket);

	bool msgPLI_BOARDMODIFY(CString& pPacket);

	bool msgPLI_REQUESTUPDATEBOARD(CString& pPacket);

	bool msgPLI_PLAYERPROPS(CString& pPacket);

	bool msgPLI_NPCPROPS(CString& pPacket);

	bool msgPLI_BOMBADD(CString& pPacket);

	bool msgPLI_BOMBDEL(CString& pPacket);

	bool msgPLI_TOALL(CString& pPacket);

	bool msgPLI_HORSEADD(CString& pPacket);

	bool msgPLI_HORSEDEL(CString& pPacket);

	bool msgPLI_ARROWADD(CString& pPacket);

	bool msgPLI_FIRESPY(CString& pPacket);

	bool msgPLI_THROWCARRIED(CString& pPacket);

	bool msgPLI_ITEMADD(CString& pPacket);

	bool msgPLI_ITEMDEL(CString& pPacket);

	bool msgPLI_CLAIMPKER(CString& pPacket);

	bool msgPLI_BADDYPROPS(CString& pPacket);

	bool msgPLI_BADDYHURT(CString& pPacket);

	bool msgPLI_BADDYADD(CString& pPacket);

	bool msgPLI_FLAGSET(CString& pPacket);

	bool msgPLI_FLAGDEL(CString& pPacket);

	bool msgPLI_OPENCHEST(CString& pPacket);

	bool msgPLI_PUTNPC(CString& pPacket);

	bool msgPLI_NPCDEL(CString& pPacket);

	bool msgPLI_WANTFILE(CString& pPacket);

	bool msgPLI_SHOWIMG(CString& pPacket);

	// PLI_UNKNOWN25
	bool msgPLI_HURTPLAYER(CString& pPacket);

	bool msgPLI_EXPLOSION(CString& pPacket);

	bool msgPLI_PRIVATEMESSAGE(CString& pPacket);

	bool msgPLI_NPCWEAPONDEL(CString& pPacket);

	bool msgPLI_PACKETCOUNT(CString& pPacket);

	bool msgPLI_WEAPONADD(CString& pPacket);

	bool msgPLI_UPDATEFILE(CString& pPacket);

	bool msgPLI_ADJACENTLEVEL(CString& pPacket);

	bool msgPLI_HITOBJECTS(CString& pPacket);

	bool msgPLI_LANGUAGE(CString& pPacket);

	bool msgPLI_TRIGGERACTION(CString& pPacket);

	bool msgPLI_MAPINFO(CString& pPacket);

	bool msgPLI_SHOOT(CString& pPacket);

	bool msgPLI_SHOOT2(CString& pPacket);

	bool msgPLI_SERVERWARP(CString& pPacket);

	bool msgPLI_PROCESSLIST(CString& pPacket);

	bool msgPLI_UNKNOWN46(CString& pPacket);

	bool msgPLI_VERIFYWANTSEND(CString& pPacket);

	bool msgPLI_UPDATECLASS(CString& pPacket);

	bool msgPLI_RAWDATA(CString& pPacket);

	bool msgPLI_RC_SERVEROPTIONSGET(CString& pPacket);

	bool msgPLI_RC_SERVEROPTIONSSET(CString& pPacket);

	bool msgPLI_RC_FOLDERCONFIGGET(CString& pPacket);

	bool msgPLI_RC_FOLDERCONFIGSET(CString& pPacket);

	bool msgPLI_RC_RESPAWNSET(CString& pPacket);

	bool msgPLI_RC_HORSELIFESET(CString& pPacket);

	bool msgPLI_RC_APINCREMENTSET(CString& pPacket);

	bool msgPLI_RC_BADDYRESPAWNSET(CString& pPacket);

	bool msgPLI_RC_PLAYERPROPSGET(CString& pPacket);

	bool msgPLI_RC_PLAYERPROPSSET(CString& pPacket);

	bool msgPLI_RC_DISCONNECTPLAYER(CString& pPacket);

	bool msgPLI_RC_UPDATELEVELS(CString& pPacket);

	bool msgPLI_RC_ADMINMESSAGE(CString& pPacket);

	bool msgPLI_RC_PRIVADMINMESSAGE(CString& pPacket);

	bool msgPLI_RC_LISTRCS(CString& pPacket);

	bool msgPLI_RC_DISCONNECTRC(CString& pPacket);

	bool msgPLI_RC_APPLYREASON(CString& pPacket);

	bool msgPLI_RC_SERVERFLAGSGET(CString& pPacket);

	bool msgPLI_RC_SERVERFLAGSSET(CString& pPacket);

	bool msgPLI_RC_ACCOUNTADD(CString& pPacket);

	bool msgPLI_RC_ACCOUNTDEL(CString& pPacket);

	bool msgPLI_RC_ACCOUNTLISTGET(CString& pPacket);

	bool msgPLI_RC_PLAYERPROPSGET2(CString& pPacket);

	bool msgPLI_RC_PLAYERPROPSGET3(CString& pPacket);

	bool msgPLI_RC_PLAYERPROPSRESET(CString& pPacket);

	bool msgPLI_RC_PLAYERPROPSSET2(CString& pPacket);

	bool msgPLI_RC_ACCOUNTGET(CString& pPacket);

	bool msgPLI_RC_ACCOUNTSET(CString& pPacket);

	bool msgPLI_RC_CHAT(CString& pPacket);

	bool msgPLI_PROFILEGET(CString& pPacket);

	bool msgPLI_PROFILESET(CString& pPacket);

	bool msgPLI_RC_WARPPLAYER(CString& pPacket);

	bool msgPLI_RC_PLAYERRIGHTSGET(CString& pPacket);

	bool msgPLI_RC_PLAYERRIGHTSSET(CString& pPacket);

	bool msgPLI_RC_PLAYERCOMMENTSGET(CString& pPacket);

	bool msgPLI_RC_PLAYERCOMMENTSSET(CString& pPacket);

	bool msgPLI_RC_PLAYERBANGET(CString& pPacket);

	bool msgPLI_RC_PLAYERBANSET(CString& pPacket);

	bool msgPLI_RC_FILEBROWSER_START(CString& pPacket);

	bool msgPLI_RC_FILEBROWSER_CD(CString& pPacket);

	bool msgPLI_RC_FILEBROWSER_END(CString& pPacket);

	bool msgPLI_RC_FILEBROWSER_DOWN(CString& pPacket);

	bool msgPLI_RC_FILEBROWSER_UP(CString& pPacket);

	bool msgPLI_NPCSERVERQUERY(CString& pPacket);

	bool msgPLI_RC_FILEBROWSER_MOVE(CString& pPacket);

	bool msgPLI_RC_FILEBROWSER_DELETE(CString& pPacket);

	bool msgPLI_RC_FILEBROWSER_RENAME(CString& pPacket);

	bool msgPLI_RC_LARGEFILESTART(CString& pPacket);

	bool msgPLI_RC_LARGEFILEEND(CString& pPacket);

	bool msgPLI_RC_FOLDERDELETE(CString& pPacket);

#ifdef V8NPCSERVER

	bool msgPLI_NC_NPCGET(CString& pPacket);

	bool msgPLI_NC_NPCDELETE(CString& pPacket);

	bool msgPLI_NC_NPCRESET(CString& pPacket);

	bool msgPLI_NC_NPCSCRIPTGET(CString& pPacket);

	bool msgPLI_NC_NPCWARP(CString& pPacket);

	bool msgPLI_NC_NPCFLAGSGET(CString& pPacket);

	bool msgPLI_NC_NPCSCRIPTSET(CString& pPacket);

	bool msgPLI_NC_NPCFLAGSSET(CString& pPacket);

	bool msgPLI_NC_NPCADD(CString& pPacket);

	bool msgPLI_NC_CLASSEDIT(CString& pPacket);

	bool msgPLI_NC_CLASSADD(CString& pPacket);

	bool msgPLI_NC_LOCALNPCSGET(CString& pPacket);

	bool msgPLI_NC_WEAPONLISTGET(CString& pPacket);

	bool msgPLI_NC_WEAPONGET(CString& pPacket);

	bool msgPLI_NC_WEAPONADD(CString& pPacket);

	bool msgPLI_NC_WEAPONDELETE(CString& pPacket);

	bool msgPLI_NC_CLASSDELETE(CString& pPacket);

	bool msgPLI_NC_LEVELLISTGET(CString& pPacket);

#endif

	bool msgPLI_REQUESTTEXT(CString& pPacket);

	bool msgPLI_SENDTEXT(CString& pPacket);

	bool msgPLI_UPDATEGANI(CString& pPacket);

	bool msgPLI_UPDATESCRIPT(CString& pPacket);

	bool msgPLI_UPDATEPACKAGEREQUESTFILE(CString& pPacket);

	bool msgPLI_RC_UNKNOWN162(CString& pPacket);

	bool m_newProtocol;

	// SSL Stuff
	static int serverRecv(WOLFSSL* ssl, char* buf, int sz, void* ctx);

	static int serverSend(WOLFSSL* ssl, char* buf, int sz, void* ctx);

private:
	WOLFSSL* ssl = nullptr;
	WOLFSSL_CTX* ctx = nullptr;

	// SendPacket functions.
	void
	sendPacketNewProtocol(unsigned char packetId, const CString& pPacket, bool sendNow = false, bool appendNL = true);

	void sendPacketOldProtocol(CString pPacket, bool appendNL = true);

	// Login functions.
	bool sendLoginClient();

	bool sendLoginNC();

	bool sendLoginRC();

	// Packet functions.
	bool parsePacket(CString& pPacket);

	void decryptPacket(CString& pPacket);

	// Collision detection stuff.
	bool testSign();

	void testTouch();

	// Misc.
	void dropItemsOnDeath();

	bool spawnLevelItem(CString& pPacket, bool playerDrop = true);

	bool removeItem(LevelItemType itemType);

	// Socket Variables
	CSocket* m_playerSock;
	CString m_recvBuffer;
	// SSL Stuff
	CString iBuffer;

	// Encryption
	unsigned char m_encryptionKey;
	CEncryption m_encryptionCodecIn;

	// Variables
	CString m_version, m_os, m_serverName;
	int m_envCodePage;
	std::weak_ptr<Level> m_currentLevel;
	uint16_t m_id;
	int m_type, m_versionId;
	time_t m_lastData, m_lastMovement, m_lastChat, m_lastNick, m_lastMessage, m_lastSave, m_last1m;
	std::vector<std::unique_ptr<CachedLevel>> m_cachedLevels;
	std::map<CString, CString> m_rcLargeFiles;
	std::map<CString, std::shared_ptr<Level>> m_singleplayerLevels;
	std::set<std::string> m_channelList;
	std::unordered_set<std::string> m_knownFiles;
	std::weak_ptr<Map> m_pmap;

	std::unordered_map<uint16_t, std::shared_ptr<Player>> m_externalPlayers;
	std::set<uint16_t> m_freeExternalPlayerIds;
	uint16_t m_nextExternalPlayerId;

	unsigned int m_carryNpcId;
	bool m_carryNpcThrown;
	CString m_guild;
	bool m_loaded;
	bool m_nextIsRaw;
	int m_rawPacketSize;
	bool m_isFtp;
	bool m_grMovementUpdated;
	CString m_grMovementPackets;
	CString m_npcserverPort;
	int m_packetCount;
	bool m_firstLevel;
	CString m_levelGroup;
	int m_invalidPackets;

	CString m_grExecParameterList;

	// File queue.
	CFileQueue m_fileQueue;

#ifdef V8NPCSERVER
	bool m_processRemoval;
	std::unique_ptr<IScriptObject<Player>> m_scriptObject;
#endif

	int getVersionIDByVersion(const CString& versionInput) const;
};

using PlayerPtr = std::shared_ptr<Player>;
using PlayerWeakPtr = std::weak_ptr<Player>;

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

inline bool Player::hasSeenFile(const std::string& file) const
{
	return m_knownFiles.find(file) != m_knownFiles.end();
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

inline CString Player::getProp(int pPropId) const
{
	CString packet;
	getProp(packet, pPropId);
	return packet;
}

#endif // TPLAYER_H
