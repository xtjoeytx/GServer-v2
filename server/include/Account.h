#ifndef TACCOUNT_H
#define TACCOUNT_H

#include <chrono>
#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <ranges>

#include <CString.h>
#include "BabyDI.h"

#include "object/Character.h"
#include "level/LevelChest.h"
#include "player/PlayerProps.h"
#include "utilities/FilePermissions.h"

enum PlayerPermissions
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

struct SavedChest
{
	int8_t x;
	int8_t y;
	std::string level;
};

struct Account
{
	std::string name;
	std::string nickname;
	std::string communityName;
	std::string level;
	Character character;
	uint8_t maxHitpoints = 3;
	uint8_t status = 20;
	uint8_t apCounter = 0;			// GR only?
	uint32_t onlineSeconds = 0;		// GR only?
	std::string ipAddress;
	std::string language{ "English" };
	uint32_t kills = 0;
	uint32_t deaths = 0;
	float eloRating = 1500.0f;
	float eloDeviation = 350.0f;
	std::chrono::system_clock::time_point lastSparTime;
	std::array<std::string, 30> ganiAttributes;
	std::vector<std::string> weapons;
	std::unordered_multimap<std::string, std::pair<int8_t, int8_t>> savedChests;
	std::unordered_map<std::string, std::string> flags;
	bool banned = false;
	std::string banReason;
	std::string banLength;
	std::string comments;
	std::string email;
	uint32_t adminRights;
	std::vector<std::string> adminIpRange;
	bool loadOnly = false;
	FilePermissions folderRights;
	std::vector<std::string> folderList;
	std::string lastFolderAccessed;

	bool hasRight(uint32_t right) const { return (adminRights & right); }
	bool hasChest(std::string_view level, int8_t x, int8_t y) const;
	bool hasWeapon(std::string_view weapon) const { return std::ranges::contains(weapons, weapon); }
};

///////////////////////////////////////////////////////////////////////////////

using flagPair = std::pair<std::string, std::string>;
using chestPair = std::pair<std::string, std::pair<int8_t, int8_t>>;

class Server;
class IAccountLoader
{
public:
	virtual bool loadAccount(std::string_view accountName, Account& account) = 0;
	virtual bool saveAccount(const Account& account) = 0;

public:
	virtual bool checkSearchConditions(std::string_view account, const std::vector<std::string>& searches) const = 0;

protected:
	BabyDI_INJECT(Server, m_server);
};

class PlainTextAccountLoader : public IAccountLoader
{
public:
	bool loadAccount(std::string_view accountName, Account& account) override;
	bool saveAccount(const Account& account) override;

public:
	bool checkSearchConditions(std::string_view account, const std::vector<std::string>& searches) const override;

protected:
	flagPair decomposeFlag(const std::string& flag) const;
	chestPair decomposeChest(const std::string& chest) const;
};

#endif // TACCOUNT_H
