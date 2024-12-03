#ifndef TACCOUNT_H
#define TACCOUNT_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <CString.h>
#include "BabyDI.h"

#include "animation/Character.h"
#include "level/LevelChest.h"
#include "utilities/FilePermissions.h"

enum
{
	PLPERM_WARPTO = 0x00001,
	PLPERM_WARPTOPLAYER = 0x00002,
	PLPERM_SUMMON = 0x00004,
	PLPERM_UPDATELEVEL = 0x00008,
	PLPERM_DISCONNECT = 0x00010,
	PLPERM_VIEWATTRIBUTES = 0x00020,
	PLPERM_SETATTRIBUTES = 0x00040,
	PLPERM_SETSELFATTRIBUTES = 0x00080,
	PLPERM_RESETATTRIBUTES = 0x00100,
	PLPERM_ADMINMSG = 0x00200,
	PLPERM_SETRIGHTS = 0x00400,
	PLPERM_BAN = 0x00800,
	PLPERM_SETCOMMENTS = 0x01000,
	PLPERM_INVISIBLE = 0x02000,
	PLPERM_MODIFYSTAFFACCOUNT = 0x04000,
	PLPERM_SETSERVERFLAGS = 0x08000,
	PLPERM_SETSERVEROPTIONS = 0x10000,
	PLPERM_SETFOLDEROPTIONS = 0x20000,
	PLPERM_SETFOLDERRIGHTS = 0x40000,
	PLPERM_NPCCONTROL = 0x80000,

	PLPERM_ANYRIGHT = 0xFFFFFF
};

enum
{
	PLPROP_NICKNAME = 0,
	PLPROP_MAXPOWER = 1,
	PLPROP_CURPOWER = 2,
	PLPROP_RUPEESCOUNT = 3,
	PLPROP_ARROWSCOUNT = 4,
	PLPROP_BOMBSCOUNT = 5,
	PLPROP_GLOVEPOWER = 6,
	PLPROP_BOMBPOWER = 7,
	PLPROP_SWORDPOWER = 8,
	PLPROP_SHIELDPOWER = 9,
	PLPROP_GANI = 10, // PLPROP_BOWGIF in pre-2.x
	PLPROP_HEADGIF = 11,
	PLPROP_CURCHAT = 12,
	PLPROP_COLORS = 13,
	PLPROP_ID = 14,
	PLPROP_X = 15,
	PLPROP_Y = 16,
	PLPROP_SPRITE = 17,
	PLPROP_STATUS = 18,
	PLPROP_CARRYSPRITE = 19,
	PLPROP_CURLEVEL = 20,
	PLPROP_HORSEGIF = 21,
	PLPROP_HORSEBUSHES = 22,
	PLPROP_EFFECTCOLORS = 23,
	PLPROP_CARRYNPC = 24,
	PLPROP_APCOUNTER = 25,
	PLPROP_MAGICPOINTS = 26,
	PLPROP_KILLSCOUNT = 27,
	PLPROP_DEATHSCOUNT = 28,
	PLPROP_ONLINESECS = 29,
	PLPROP_IPADDR = 30,
	PLPROP_UDPPORT = 31,
	PLPROP_ALIGNMENT = 32,
	PLPROP_ADDITFLAGS = 33,
	PLPROP_ACCOUNTNAME = 34,
	PLPROP_BODYIMG = 35,
	PLPROP_RATING = 36,
	PLPROP_GATTRIB1 = 37,
	PLPROP_GATTRIB2 = 38,
	PLPROP_GATTRIB3 = 39,
	PLPROP_GATTRIB4 = 40,
	PLPROP_GATTRIB5 = 41,
	PLPROP_ATTACHNPC = 42,
	PLPROP_GMAPLEVELX = 43,
	PLPROP_GMAPLEVELY = 44,
	PLPROP_Z = 45,
	PLPROP_GATTRIB6 = 46,
	PLPROP_GATTRIB7 = 47,
	PLPROP_GATTRIB8 = 48,
	PLPROP_GATTRIB9 = 49,
	PLPROP_JOINLEAVELVL = 50,
	PLPROP_PCONNECTED = 51,
	PLPROP_PLANGUAGE = 52,
	PLPROP_PSTATUSMSG = 53,
	PLPROP_GATTRIB10 = 54,
	PLPROP_GATTRIB11 = 55,
	PLPROP_GATTRIB12 = 56,
	PLPROP_GATTRIB13 = 57,
	PLPROP_GATTRIB14 = 58,
	PLPROP_GATTRIB15 = 59,
	PLPROP_GATTRIB16 = 60,
	PLPROP_GATTRIB17 = 61,
	PLPROP_GATTRIB18 = 62,
	PLPROP_GATTRIB19 = 63,
	PLPROP_GATTRIB20 = 64,
	PLPROP_GATTRIB21 = 65,
	PLPROP_GATTRIB22 = 66,
	PLPROP_GATTRIB23 = 67,
	PLPROP_GATTRIB24 = 68,
	PLPROP_GATTRIB25 = 69,
	PLPROP_GATTRIB26 = 70,
	PLPROP_GATTRIB27 = 71,
	PLPROP_GATTRIB28 = 72,
	PLPROP_GATTRIB29 = 73,
	PLPROP_GATTRIB30 = 74,
	PLPROP_OSTYPE = 75,       // 2.19+
	PLPROP_TEXTCODEPAGE = 76, // 2.19+
	PLPROP_UNKNOWN77 = 77,
	PLPROP_X2 = 78,
	PLPROP_Y2 = 79,
	PLPROP_Z2 = 80,
	PLPROP_UNKNOWN81 = 81, // {GCHAR flag} - flag 0 places in playerlist, flag 1 places in servers tab, flag 3 places in channels tab (unconfirmed)

	// In Graal v5, where players have the Graal######## accounts, this is their chosen account alias (community name.)
	PLPROP_COMMUNITYNAME = 82,
};
#define propscount 83

class Server;
class Account
{
public:
	// Constructor - Deconstructor
	Account();
	~Account();

	static bool meetsConditions(CString fileName, CString conditions);

	// Load/Save Account
	void reset();
	bool loadAccount(const CString& pAccount, bool ignoreNickname = false);
	bool saveAccount();

	// Attribute-Managing
	bool hasChest(const CString& pChest);
	bool hasWeapon(const CString& pWeapon);

	// Flag-Managing
	CString getFlag(const std::string& pFlagName) const;
	void setFlag(CString pFlag);
	void setFlag(const std::string& pFlagName, const CString& pFlagValue);
	void deleteFlag(const std::string& pFlagName);

	CString translate(const CString& pKey) const;
	bool hasRight(int mask) const { return (m_adminRights & mask) ? true : false; }

	// get functions
	bool getGuest() const { return m_isGuest; }
	float getX() const { return m_x / 16.0f; }
	float getY() const { return m_y / 16.0f; }
	float getZ() const { return m_z / 16.0f; }
	int16_t getPixelX() const { return m_x; }
	int16_t getPixelY() const { return m_y; }
	int16_t getPixelZ() const { return m_z; }
	float getPower() const { return m_character.hitpoints; }
	int getAlignment() const { return m_character.ap; }
	int getArrowCount() const { return m_character.arrows; }
	int getBombCount() const { return m_character.bombs; }
	int getGlovePower() const { return m_character.glovePower; }
	int getMagicPower() const { return m_mp; }
	int getMaxPower() const { return m_maxHitpoints; }
	int getRupees() const { return m_character.gralats; }
	int getSwordPower() const { return m_character.swordPower; }
	int getShieldPower() const { return m_character.shieldPower; }
	int getSprite() const { return m_character.sprite; }
	int getStatus() const { return m_status; }
	int getOnlineTime() const { return m_onlineTime; }
	int getKills() const { return m_kills; }
	int getDeaths() const { return m_deaths; }
	int getAdminRights() const { return m_adminRights; }
	int64_t getDeviceId() const { return m_deviceId; }
	bool getBanned() const { return m_isBanned; }
	bool getLoadOnly() const { return m_isLoadOnly; }
	unsigned char getColorId(unsigned int idx) const;
	unsigned int getAttachedNPC() const { return m_attachNPC; }

	const CString& getAccountName() const { return m_accountName; }
	const std::string& getNickname() const { return m_character.nickName; }
	const CString& getLevelName() const { return m_levelName; }
	const CString& getBodyImage() const { return m_character.bodyImage; }
	const CString& getHeadImage() const { return m_character.headImage; }
	//Level * getLevel()						{ return nullptr; }
	const CString& getShieldImage() const { return m_character.shieldImage; }
	const CString& getSwordImage() const { return m_character.swordImage; }
	const CString& getAnimation() const { return m_character.gani; }
	const CString& getAdminIp() const { return m_adminIp; }
	const CString& getBanReason() const { return m_banReason; }
	const CString& getBanLength() const { return m_banLength; }
	const CString& getChatMsg() const { return m_character.chatMessage; }
	const CString& getEmail() const { return m_email; }
	const CString& getIpStr() const { return m_accountIpStr; }
	const CString& getComments() const { return m_accountComments; }
	std::unordered_map<std::string, CString>& getFlagList() { return m_flagList; }
	const std::vector<CString>& getFolderList() const { return m_folderList; }
	const FilePermissions& getFolderRights() const { return m_folderRights; }
	std::vector<CString>& getWeaponList() { return m_weaponList; }

	// set functions
	void setX(float val) { m_x = static_cast<int16_t>(val * 16); }
	void setY(float val) { m_y = static_cast<int16_t>(val * 16); }
	void setZ(float val) { m_z = static_cast<int16_t>(val * 16); }
	void setPixelX(int16_t val) { m_x = val; }
	void setPixelY(int16_t val) { m_y = val; }
	void setPixelZ(int16_t val) { m_z = val; }
	void setDeviceId(int64_t newDeviceId) { m_deviceId = newDeviceId; }
	void setLastSparTime(time_t newTime) { m_lastSparTime = newTime; }
	void setApCounter(int newTime) { m_apCounter = newTime; }
	void setKills(int newKills) { m_kills = newKills; }
	void setRating(int newRate, int newDeviate)
	{
		m_eloRating = (float)newRate;
		m_eloDeviation = (float)newDeviate;
	}
	void setAccountName(CString account) { m_accountName = account; }
	void setExternal(bool external) { m_isExternal = external; }
	void setBanned(bool banned) { m_isBanned = banned; }
	void setBanReason(CString reason) { m_banReason = reason; }
	void setBanLength(CString length) { m_banLength = length; }
	void setLoadOnly(bool loadOnly) { m_isLoadOnly = loadOnly; }
	void setEmail(CString email) { m_email = email; }
	void setAdminRights(int rights) { m_adminRights = rights; }
	void setAdminIp(CString ip) { m_adminIp = ip; }
	void setComments(CString comments) { m_accountComments = comments; }

	void setBodyImage(const CString& newImage);
	void setHeadImage(const CString& newImage);
	void setMaxPower(int newMaxPower);
	void setPower(float newPower);
	void setShieldImage(const CString& newImage);
	void setShieldPower(int newPower);
	void setSwordImage(const CString& newImage);
	void setSwordPower(int newPower);
	void setGani(const CString& newGani);
	void setFolderRights(const std::vector<CString>& folderRights);

protected:
	BabyDI_INJECT(Server, m_server);

	// Player-Account
	bool m_isBanned = false;
	bool m_isLoadOnly = false;
	bool m_isGuest = false;
	bool m_isExternal = false;
	CString m_adminIp{ "0.0.0.0" };
	CString m_accountComments, m_accountName, m_communityName, m_banReason, m_banLength, m_lastFolder, m_email;
	CString m_accountIpStr;
	unsigned long m_accountIp = 0;
	int m_adminRights = 0;
	int64_t m_deviceId = 0;

	// Player-Attributes
	Character m_character;

	CString m_language{ "English" };
	CString m_levelName;
	int16_t m_x = 0, m_y = 0, m_z = 0;
	float m_eloDeviation = 350.0f;
	float m_eloRating = 1500.0f;
	uint8_t m_maxHitpoints = 3;
	uint8_t m_mp = 0;
	uint8_t m_apCounter = 0;
	uint8_t m_horseBombCount = 0;
	uint32_t m_kills = 0;
	uint32_t m_deaths = 0;
	uint8_t m_carrySprite = -1;
	int m_additionalFlags = 0;
	int m_onlineTime = 0;
	int m_status = 20;
	int m_udpport = 0;
	time_t m_lastSparTime = 0;
	uint32_t m_attachNPC = 0;
	uint8_t m_statusMsg = 0;
	std::unordered_map<std::string, CString> m_flagList;
	std::vector<CString> m_chestList, m_folderList, m_weaponList, m_privateMessageServerList;
	FilePermissions m_folderRights;
};

inline CString Account::getFlag(const std::string& pFlagName) const
{
	auto it = m_flagList.find(pFlagName);
	if (it != m_flagList.end())
		return it->second;
	return "";
}

inline void Account::deleteFlag(const std::string& pFlagName)
{
	m_flagList.erase(pFlagName);
}

inline unsigned char Account::getColorId(unsigned int idx) const
{
	if (idx < 5) return m_character.colors[idx];
	return 0;
}

inline void Account::setPower(float newPower)
{
	m_character.hitpoints = clip(newPower, 0.0f, (float)m_maxHitpoints);
}

inline void Account::setShieldImage(const CString& newImage)
{
	m_character.shieldImage = newImage.subString(0, 223);
}

inline void Account::setSwordImage(const CString& newImage)
{
	m_character.swordImage = newImage.subString(0, 223);
}

inline void Account::setGani(const CString& newGani)
{
	m_character.gani = newGani.subString(0, 223);
}

inline void Account::setBodyImage(const CString& newImage)
{
	m_character.bodyImage = newImage.subString(0, 223);
}

inline void Account::setHeadImage(const CString& newImage)
{
	m_character.headImage = newImage.subString(0, 123);
}

#endif // TACCOUNT_H
