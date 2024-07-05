#ifndef TACCOUNT_H
#define TACCOUNT_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <CString.h>

#include "level/LevelChest.h"

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
	Account(Server* pServer);
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

	CString translate(const CString& pKey);
	bool hasRight(int mask) { return (m_adminRights & mask) ? true : false; }

	// get functions
	bool getGuest() { return m_isGuest; }
	float getX() const { return m_x; }
	float getY() const { return m_y; }
	float getPower() const { return m_hitpoints; }
	int getAlignment() const { return m_ap; }
	int getArrowCount() const { return m_arrowCount; }
	int getBombCount() const { return m_bombCount; }
	int getGlovePower() const { return m_glovePower; }
	int getMagicPower() const { return m_mp; }
	int getMaxPower() const { return m_maxHitpoints; }
	int getRupees() const { return m_gralatCount; }
	int getSwordPower() const { return m_swordPower; }
	int getShieldPower() const { return m_shieldPower; }
	int getSprite() const { return m_sprite; }
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
	const CString& getNickname() const { return m_nickName; }
	const CString& getLevelName() const { return m_levelName; }
	const CString& getBodyImage() const { return m_bodyImage; }
	const CString& getHeadImage() const { return m_headImage; }
	//Level * getLevel()						{ return nullptr; }
	const CString& getShieldImage() const { return m_shieldImage; }
	const CString& getSwordImage() const { return m_swordImage; }
	const CString& getAnimation() const { return m_gani; }
	const CString& getAdminIp() const { return m_adminIp; }
	const CString& getBanReason() const { return m_banReason; }
	const CString& getBanLength() const { return m_banLength; }
	const CString& getChatMsg() const { return m_chatMessage; }
	const CString& getEmail() const { return m_email; }
	const CString& getIpStr() const { return m_accountIpStr; }
	const CString& getComments() const { return m_accountComments; }
	std::unordered_map<std::string, CString>& getFlagList() { return m_flagList; }
	std::vector<CString>& getFolderList() { return m_folderList; }
	std::vector<CString>& getWeaponList() { return m_weaponList; }

	// set functions
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

protected:
	Server* m_server;

	// Player-Account
	bool m_isBanned, m_isLoadOnly, m_isGuest;
	bool m_isExternal;
	CString m_adminIp, m_accountComments, m_accountName, m_communityName, m_banReason, m_banLength, m_lastFolder, m_email;
	CString m_accountIpStr;
	unsigned long m_accountIp;
	int m_adminRights;
	int64_t m_deviceId;

	// Player-Attributes
	CString m_attrList[30], m_bodyImage, m_chatMessage, m_headImage, m_horseImage, m_gani, m_bowImage, m_language;
	CString m_levelName, m_nickName, m_shieldImage, m_swordImage;
	float m_eloDeviation, m_hitpoints, m_eloRating, m_x, m_y, m_z;
	int m_additionalFlags, m_ap, m_apCounter, m_arrowCount, m_bombCount, m_bombPower, m_carrySprite;
	unsigned char m_colors[5];
	int m_deaths, m_glovePower, m_bowPower, m_gralatCount, m_horseBombCount, m_kills, m_mp, m_maxHitpoints;
	int m_onlineTime, m_shieldPower, m_sprite, m_status, m_swordPower, m_udpport;
	unsigned int m_attachNPC;
	time_t m_lastSparTime;
	unsigned char m_statusMsg;
	std::unordered_map<std::string, CString> m_flagList;
	std::vector<CString> m_chestList, m_folderList, m_weaponList, m_privateMessageServerList;
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
	if (idx < 5) return m_colors[idx];
	return 0;
}

inline void Account::setPower(float newPower)
{
	m_hitpoints = clip(newPower, 0.0f, (float)m_maxHitpoints);
}

inline void Account::setShieldImage(const CString& newImage)
{
	m_shieldImage = newImage.subString(0, 223);
}

inline void Account::setSwordImage(const CString& newImage)
{
	m_swordImage = newImage.subString(0, 223);
}

inline void Account::setGani(const CString& newGani)
{
	m_gani = newGani.subString(0, 223);
}

inline void Account::setBodyImage(const CString& newImage)
{
	m_bodyImage = newImage.subString(0, 223);
}

inline void Account::setHeadImage(const CString& newImage)
{
	m_headImage = newImage.subString(0, 123);
}

#endif // TACCOUNT_H
