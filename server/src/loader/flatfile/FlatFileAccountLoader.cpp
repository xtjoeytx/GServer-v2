#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <limits>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Account.h>
#include <BabyDI.h>
#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <loader/flatfile/FlatFileAccountLoader.h>
#include <object/Player.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

using namespace std::string_view_literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

// Helper to avoid having to write uint8_t everywhere.
const auto& toByte = static_cast<uint8_t (*)(std::string_view)>(string::toNumber);
const auto& toSByte = static_cast<int8_t (*)(std::string_view)>(string::toNumber);

static bool setIfEmpty(std::string& str, const std::string_view value, const std::string_view defaultValue = {})
{
	if (!str.empty())
		return false;
	str = value.empty() ? defaultValue : value;
	return true;
}

static void writeLine(std::string& output, const std::string& section, const auto& value)
{
	output += section + " " + std::format("{}", value) + "\n";
}

static void writeLine(std::string& output, const std::string& section, const auto& value, const auto& defaultValue)
{
	if (value != defaultValue)
		writeLine(output, section, value);
}

///////////////////////////////////////////////////////////////////////////////

flagPair FlatFileAccountLoader::decomposeFlag(const std::string& flag)
{
	const auto server = BabyDI::Get<Server>();
	const auto sep = flag.find('=');
	flagPair result = (sep == std::string::npos) ? std::make_pair(flag, "") : std::make_pair(flag.substr(0, sep), flag.substr(sep + 1));
	if (server->cached.enableFlagCropping.getValue())
	{
		// If cropflags is enabled, crop the flag to 223 characters.
		// Subtract the length of the flag name and the = character from 223 to determine the space left for the flag value.
		const int fixedLength = result.first.length() < 223 ? (223 - 1) - static_cast<int>(result.first.length()) : 0;
		result.second = result.second.substr(0, fixedLength);
	}
	return result;
}

chestPair FlatFileAccountLoader::decomposeChest(const std::string_view chest)
{
	chestPair result;
	const auto tokens = string::splitToVector(chest, ":"sv);
	if (tokens.size() == 3)
	{
		result.second.x() = string::toNumber<uint8_t>(tokens[0]);
		result.second.y() = string::toNumber<uint8_t>(tokens[1]);
		result.first = string::trim(tokens[2]);
	}
	return result;
}

bool FlatFileAccountLoader::loadAccount(const std::string_view accountName, Account& account)
{
	return loadAccount(accountName, account, nullptr);
}

bool FlatFileAccountLoader::loadAccount(const std::string_view accountName, Player& player)
{
	return loadAccount(accountName, player.account, &player);
}

bool FlatFileAccountLoader::loadAccount(const std::string_view accountName, Account& account, Player* player)
{
	auto server = BabyDI::Get<Server>();

	// Find the account to load.
	bool loadedFromDefault = false;
	auto& accountFS = server->getFileSystemServer();
	auto path = accountFS.findi(fs::FileCategory::ACCOUNT, std::format("{}.txt", accountName));
	if (path.empty())
	{
		path = "accounts/(defaultaccount).txt";
		loadedFromDefault = true;
	}

	// Load the account data.
	fs::File fileData{path};
	if (!fileData.opened())
		return false;

	// Check for the header.
	if (fileData.readLine() != "GRACC001")
		return false;

	// Set the account name.
	account.name = accountName;

	// Modtime handling for players.
	const auto& updateTime = server->getFrameStartTime();
	PlayerModTimes* modTime = (player != nullptr ? &player->modTime : nullptr);

	auto updateModTime = [&](const PlayerProp prop)
	{
		if (modTime)
			(*modTime)[PROPID(prop)] = updateTime;
	};

	// Clear some data structures, just in case we are loading into an already existing player or account.
	account.savedChests.clear();
	account.folderList.clear();
	account.folderRights.clear();

	// Store weapons / flags.
	std::vector<std::string> flagList{};
	std::vector<std::string> weaponList{};

	// Parse File
	for (const auto& line : fileData.readAllLines())
	{
		// Trim Line
		std::string_view lineview{line};

		// Get the section and value.
		auto [section, val] = string::extractConfigParts(lineview);

		if (section == "NAME")
			continue;

		if (section == "NICK")
		{
			// Load the nickname only if it is not yet set.
			// Some clients, like RC, will send the nickname props immediately and not wait until the go-ahead to login.
			if (account.character.nickName.empty())
			{
				account.character.nickName = val.substr(0, 223);
				updateModTime(PlayerProp::NICKNAME);
			}
		}
		else if (section == "COMMUNITYNAME")
			account.communityName = val;
		else if (section == "LEVEL")
		{
			if (account.level != val)
			{
				account.level = val;
				updateModTime(PlayerProp::LEVEL);
			}
		}
		else if (section == "GROUPNAME") // GR
			account.groupName = val;
		else if (section == "X")
		{
			const auto num = static_cast<int16_t>(string::toFloat(val) * 16);
			if (account.character.localPixelX != num)
			{
				account.character.localPixelX = num;
				updateModTime(PlayerProp::X);
				updateModTime(PlayerProp::X2);
			}
		}
		else if (section == "Y")
		{
			const auto num = static_cast<int16_t>(string::toFloat(val) * 16);
			if (account.character.localPixelY != num)
			{
				account.character.localPixelY = static_cast<int16_t>(string::toFloat(val) * 16);
				updateModTime(PlayerProp::Y);
				updateModTime(PlayerProp::Y2);
			}
		}
		else if (section == "Z")
		{
			const auto num = static_cast<int16_t>(string::toFloat(val) * 16);
			if (account.character.localPixelZ != num)
			{
				account.character.localPixelZ = static_cast<int16_t>(string::toFloat(val) * 16);
				updateModTime(PlayerProp::Z);
				updateModTime(PlayerProp::Z2);
			}
		}
		else if (section == "MAPX")
		{
			const auto num = toByte(val);
			if (account.character.mapX != num)
			{
				account.character.mapX = num;
				updateModTime(PlayerProp::GMAPLEVELX);
			}
		}
		else if (section == "MAPY")
		{
			const auto num = toByte(val);
			if (account.character.mapY != num)
			{
				account.character.mapY = num;
				updateModTime(PlayerProp::GMAPLEVELY);
			}
		}
		else if (section == "MAXHP")
		{
			const auto num = toByte(val);
			if (account.maxHitpoints != num)
			{
				account.maxHitpoints = num;
				updateModTime(PlayerProp::FULLHEARTS);
			}
		}
		else if (section == "HP")
		{
			const auto num = static_cast<uint8_t>(string::toFloat(val) * 2);
			if (account.character.hitpointsInHalves != num)
			{
				account.character.hitpointsInHalves = num;
				updateModTime(PlayerProp::HALFHEARTS);
			}
		}
		else if (section == "GRALATS" || section == "RUPEES")
		{
			const auto num = string::toNumber<uint32_t>(val);
			if (account.character.gralats != num)
			{
				account.character.gralats = num;
				updateModTime(PlayerProp::GRALATS);
			}
		}
		else if (section == "ANI")
		{
			if (account.character.gani != val)
			{
				account.character.gani = val;
				updateModTime(PlayerProp::GANI);
			}
		}
		else if (section == "ARROWS")
		{
			const auto num = toByte(val);
			if (account.character.arrows != num)
			{
				account.character.arrows = num;
				updateModTime(PlayerProp::ARROWS);
			}
		}
		else if (section == "BOMBS")
		{
			const auto num = toByte(val);
			if (account.character.bombs != num)
			{
				account.character.bombs = num;
				updateModTime(PlayerProp::BOMBS);
			}
		}
		else if (section == "GLOVEP")
		{
			const auto num = toByte(val);
			if (account.character.glovePower != num)
			{
				account.character.glovePower = num;
				updateModTime(PlayerProp::GLOVEPOWER);
			}
		}
		else if (section == "SHIELDP")
		{
			const auto num = toByte(val);
			if (account.character.shieldPower != num)
			{
				account.character.shieldPower = num;
				updateModTime(PlayerProp::SHIELDIMAGE);
			}
		}
		else if (section == "SWORDP")
		{
			const auto num = toSByte(val);
			if (account.character.swordPower != num)
			{
				account.character.swordPower = num;
				updateModTime(PlayerProp::SWORDIMAGE);
			}
		}
		else if (section == "BOMBP")
		{
			const auto num = toByte(val);
			if (account.character.bombPower != num)
			{
				account.character.bombPower = num;
				updateModTime(PlayerProp::BOMBPOWER);
			}
		}
		else if (section == "BOWP")
		{
			const auto num = toByte(val);
			if (account.character.bowPower != num)
			{
				account.character.bowPower = num;
				updateModTime(PlayerProp::GANI);
			}
		}
		else if (section == "BOW")
		{
			if (account.character.bowImage != val)
			{
				account.character.bowImage = val;
				updateModTime(PlayerProp::GANI);
			}
		}
		else if (section == "HEAD")
		{
			if (account.character.headImage != val)
			{
				account.character.headImage = val;
				updateModTime(PlayerProp::HEADIMAGE);
			}
		}
		else if (section == "BODY")
		{
			if (account.character.bodyImage != val)
			{
				account.character.bodyImage = val;
				updateModTime(PlayerProp::BODYIMAGE);
			}
		}
		else if (section == "SWORD")
		{
			if (account.character.swordImage != val)
			{
				account.character.swordImage = val;
				updateModTime(PlayerProp::SWORDIMAGE);
			}
		}
		else if (section == "SHIELD")
		{
			if (account.character.shieldImage != val)
			{
				account.character.shieldImage = val;
				updateModTime(PlayerProp::SHIELDIMAGE);
			}
		}
		else if (section == "COLORS")
		{
			auto tokensAsNumbers = string::split(val, ","sv) | std::views::take(8) | std::views::transform([](const std::string_view& token)
			{
				return toByte(std::string{token});
			});
			std::ranges::copy(tokensAsNumbers, account.character.colors.begin());
			updateModTime(PlayerProp::COLORS);
		}
		else if (section == "SPRITE")
		{
			const auto num = toByte(val);
			const auto sprite = num >> 2;
			const auto dir = num & 0b11;
			if (account.character.sprite != sprite || account.character.direction != dir)
			{
				account.character.sprite = sprite;
				account.character.direction = dir;
				updateModTime(PlayerProp::SPRITE);
			}
		}
		else if (section == "STATUS")
		{
			const auto num = toByte(val);
			if (account.status != num)
			{
				account.status = num;
				updateModTime(PlayerProp::STATUS);
			}
		}
		else if (section == "MP")
		{
			const auto num = toByte(val);
			if (account.character.mp != num)
			{
				account.character.mp = num;
				updateModTime(PlayerProp::MAGICPOINTS);
			}
		}
		else if (section == "AP")
		{
			const auto num = toByte(val);
			if (account.character.ap != num)
			{
				account.character.ap = num;
				updateModTime(PlayerProp::ALIGNMENT);
			}
		}
		else if (section == "APCOUNTER")
		{
			const auto num = toByte(val);
			if (account.apCounter != num)
			{
				account.apCounter = num;
				updateModTime(PlayerProp::APCOUNTER);
			}
		}
		else if (section == "ONSECS")
		{
			const auto num = string::toNumber<uint32_t>(val);
			if (account.onlineSeconds != num)
			{
				account.onlineSeconds = num;
				updateModTime(PlayerProp::ONLINESECONDS);
				updateModTime(PlayerProp::ONLINESECONDS2);
			}
		}
		else if (section == "KILLS")
		{
			const auto num = string::toNumber<uint32_t>(val);
			if (account.kills != num)
			{
				account.kills = num;
				updateModTime(PlayerProp::KILLS);
			}
		}
		else if (section == "DEATHS")
		{
			const auto num = string::toNumber<uint32_t>(val);
			if (account.deaths != num)
			{
				account.deaths = num;
				updateModTime(PlayerProp::DEATHS);
			}
		}
		else if (section == "RATING")
		{
			const auto num = string::toFloat(val);
			if (account.eloRating != num)
			{
				account.eloRating = num;
				updateModTime(PlayerProp::RATING);
			}
		}
		else if (section == "DEVIATION")
		{
			const auto num = string::toFloat(val);
			if (account.eloDeviation != num)
			{
				account.eloDeviation = num;
				updateModTime(PlayerProp::RATING);
			}
		}
		else if (section == "LASTSPARTIME")
			account.lastSparTime = clock::from_time_t(string::toNumber<time_t>(val));
		else if (section == "IP")
			setIfEmpty(account.ipAddress, val);
		else if (section == "LANGUAGE")
			setIfEmpty(account.language, val, "English"sv);
		// PLATFORM - ignore
		// CODEPAGE - ignore
		else if (section == "FLAG")
		{
			flagList.emplace_back(val);
		}
		else if (section.starts_with("ATTR"))
		{
			if (auto idx = toByte(section.substr(4)); idx > 0 && idx <= 30)
			{
				if (account.character.ganiAttributes[idx - 1] != val)
				{
					account.character.ganiAttributes[idx - 1] = val;
					updateModTime(ENUM<PlayerProp>(GaniAttributePropList[idx - 1]));
				}
			}
		}
		else if (section == "WEAPON")
		{
			weaponList.emplace_back(val);
		}
		else if (section == "CHEST")
			account.savedChests.insert(decomposeChest(val));
		else if (section == "BANNED")
			account.banned = toByte(val) != 0;
		else if (section == "BANREASON")
			account.banReason = val;
		else if (section == "BANLENGTH")
			account.banLength = val;
		else if (section == "COMMENTS")
			account.comments = val;
		else if (section == "EMAIL")
			account.email = val;
		else if (section == "LOCALRIGHTS")
			account.adminRights = string::toNumber<uint32_t>(val);
		else if (section == "IPRANGE")
			account.adminIpRange = string::splitToVector(val, ","sv);
		else if (section == "LOADONLY")
			account.loadOnly = toByte(val) != 0;
		else if (section == "FOLDERRIGHT")
		{
			account.folderList.emplace_back(val);
			account.folderRights.addPermission(val);
		}
		else if (section == "LASTFOLDER")
			account.lastFolderAccessed = val;
	}

	// If this is a guest account, loadonly is set to true.
	if (string::equalsi(accountName, "guest"sv))
	{
		account.loadOnly = true;
		srand(static_cast<unsigned int>(time(nullptr)));

		// Try to create a unique account number.
		while (true)
		{
			int v = (rand() * rand()) % 9999999;
			if (server->getPlayer("pc:" + CString(v).subString(0, 6), PLTYPE_ANYPLAYER) == nullptr)
			{
				account.name = std::format("pc:{:6}", v);
				break;
			}
		}

		account.communityName = "guest";
	}

	// Default community name to account name if not set.
	if (account.communityName.empty())
		account.communityName = account.name;

	// Fix Z if we need to.
	if (account.character.localPixelZ.has_value() && (account.character.localPixelZ.value() < Character::ValidZRangePixels[0] || account.character.localPixelZ.value() > Character::ValidZRangePixels[1]))
		account.character.localPixelZ.reset();

	// Flag syncing.
	if (player != nullptr && player->isClient())
		player->synchronizeFlags(flagList);
	else
	{
		for (auto& val : flagList)
		{
			if (auto variable = GameVariable::deserialize(val); variable.has_value())
				account.variables.add(std::move(variable.value()));
		}
	}

	// Weapon syncing.
	if (player != nullptr && player->isClient())
		player->synchronizeWeapons(weaponList);
	else
		account.weapons = weaponList;

	// If we loaded from the default account, check if the settings is overriding the start level and position.
	// Also, save the account and add it to the file system.
	if (loadedFromDefault)
	{
		auto& settings = server->getSettings();

		// Check to see if we are overriding our start level and position.
		if (settings.exists("startlevel"))
			account.level = settings.get<std::string>("startlevel").value_or("onlinestartlocal.nw");

		if (settings.exists("startx"))
			account.character.localPixelX = static_cast<int16_t>(settings.get<float>("startx").value_or(30.0f) * 16);

		if (settings.exists("starty"))
			account.character.localPixelY = static_cast<int16_t>(settings.get<float>("starty").value_or(30.5f) * 16);

		// Save our account now and add it to the file system.
		if (!account.loadOnly)
			saveAccount(account);
	}

	return true;
}

bool FlatFileAccountLoader::saveAccount(const Account& account)
{
	auto server = BabyDI::Get<Server>();

	// Don't save 'Load Only' or RC accounts.
	if (account.loadOnly)
		return false;

#ifdef DEBUG
	assert(account.level.empty() == false);
#endif

	std::string colorStr = std::format("{},{},{},{},{}", account.character.colors[0], account.character.colors[1], account.character.colors[2], account.character.colors[3], account.character.colors[4]);
	std::string colorStrEx = std::format("{},{},{},{}", colorStr, account.character.colors[5], account.character.colors[6], account.character.colors[7]);
	std::string defaultColorStr = "2,0,10,4,18";
	std::string defaultColorStrEx = "2,0,10,4,18,18,18,18";

	std::string newFile = "GRACC001\r\n";
	writeLine(newFile, "NAME", account.name);
	writeLine(newFile, "NICK", account.character.nickName);
	writeLine(newFile, "COMMUNITYNAME", account.communityName, account.name);
	writeLine(newFile, "LEVEL", account.level);

	// GR extension
	if (!account.groupName.empty())
		writeLine(newFile, "GROUPNAME", account.groupName);

	writeLine(newFile, "X", static_cast<float>(account.character.localPixelX) / 16.0f);
	writeLine(newFile, "Y", static_cast<float>(account.character.localPixelY) / 16.0f);
	if (account.character.localPixelZ.has_value())
		writeLine(newFile, "Z", static_cast<float>(account.character.localPixelZ.value()) / 16.0f, 0.0f);

	if (account.character.mapX != 0)
		writeLine(newFile, "MAPX", account.character.mapX);
	if (account.character.mapY != 0)
		writeLine(newFile, "MAPY", account.character.mapY);

	writeLine(newFile, "MAXHP", account.maxHitpoints);
	writeLine(newFile, "HP", static_cast<float>(account.character.hitpointsInHalves) / 2.0f);
	if (server->Generation != ServerGeneration::CLASSIC)
	{
		writeLine(newFile, "ANI", account.character.gani);
	}
	writeLine(newFile, "SPRITE", (account.character.sprite << 2 | account.character.direction), 2);
	writeLine(newFile, "GRALATS", account.character.gralats);
	writeLine(newFile, "ARROWS", account.character.arrows);
	writeLine(newFile, "BOMBS", account.character.bombs);
	writeLine(newFile, "GLOVEP", account.character.glovePower);
	writeLine(newFile, "SWORDP", account.character.swordPower);
	writeLine(newFile, "SHIELDP", account.character.shieldPower);
	if (server->Generation == ServerGeneration::CLASSIC)
	{
		writeLine(newFile, "BOMBP", account.character.bombPower, 1_ui8);
		writeLine(newFile, "BOWP", account.character.bowPower, 1_ui8);
		writeLine(newFile, "BOW", account.character.bowImage, "");
	}
	writeLine(newFile, "HEAD", account.character.headImage);
	writeLine(newFile, "BODY", account.character.bodyImage);
	writeLine(newFile, "SWORD", account.character.swordImage);
	writeLine(newFile, "SHIELD", account.character.shieldImage);

	if (server->isNewWorldMode())
		writeLine(newFile, "COLORS", colorStrEx, defaultColorStrEx);
	else writeLine(newFile, "COLORS", colorStr, defaultColorStr);

	writeLine(newFile, "STATUS", account.status);
	writeLine(newFile, "MP", account.character.mp, 0_ui8);
	writeLine(newFile, "AP", account.character.ap);
	writeLine(newFile, "APCOUNTER", account.apCounter, 0_ui8);
	writeLine(newFile, "ONSECS", account.onlineSeconds, static_cast<uint32_t>(0));
	writeLine(newFile, "KILLS", account.kills, static_cast<uint32_t>(0));
	writeLine(newFile, "DEATHS", account.deaths, static_cast<uint32_t>(0));
	writeLine(newFile, "RATING", account.eloRating, 1500.0f);
	writeLine(newFile, "DEVIATION", account.eloDeviation, 350.0f);
	writeLine(newFile, "LASTSPARTIME", clock::to_time_t(account.lastSparTime), static_cast<time_t>(0));
	writeLine(newFile, "IP", account.ipAddress);
	writeLine(newFile, "LANGUAGE", account.language, "English"sv); // TODO: Also accept "en" and other two-character language codes.
	writeLine(newFile, "PLATFORM", account.platform);
	writeLine(newFile, "CODEPAGE", account.codePage);

	// Attributes
	for (size_t i = 0; i < 30; i++)
		writeLine(newFile, "ATTR" + std::to_string(i + 1), account.character.ganiAttributes[i], "");

	// Chests
	for (const auto& [level, pos] : account.savedChests)
		writeLine(newFile, "CHEST", std::format("{}:{}:{}", pos.x(), pos.y(), level));

	// Weapons
	for (const auto& weapon : account.weapons)
		writeLine(newFile, "WEAPON", weapon);

	// Flags
	for (const auto& [variable, value] : account.variables.store | variables::only_flags | variables::serializable)
	{
		if (auto serialized = account.variables.serializeModern(variable); serialized.has_value())
			writeLine(newFile, "FLAG", serialized.value());
	}

	// Account Settings
	newFile += "\r\n";
	writeLine(newFile, "BANNED", account.banned ? 1 : 0, 0);
	writeLine(newFile, "BANREASON", account.banReason, "");
	writeLine(newFile, "BANLENGTH", account.banLength, "");
	writeLine(newFile, "COMMENTS", account.comments, "");
	writeLine(newFile, "EMAIL", account.email, "");
	writeLine(newFile, "LOCALRIGHTS", account.adminRights, static_cast<uint32_t>(0));
	writeLine(newFile, "IPRANGE", string::join(account.adminIpRange), "");
	writeLine(newFile, "LOADONLY", account.loadOnly ? 1 : 0, 0);

	// Folder Rights
	for (const auto& perm : account.folderList)
		writeLine(newFile, "FOLDERRIGHT", perm);

	// Last Folder Accessed
	writeLine(newFile, "LASTFOLDER", account.lastFolderAccessed, "");

	// Get the file name for the account.
	auto accountFileName = std::format("{}.txt", account.name);
	auto accountPath = server->getFileSystemServer().findi(fs::FileCategory::ACCOUNT, accountFileName);
	if (accountPath.empty())
		accountPath = std::filesystem::path{"accounts"} / accountFileName;

	// Save the account now.
	if (fs::FileIO writer(accountPath, true); writer.opened())
	{
		writer.write(newFile);
	}
	else
	{
		log::printLine(log::rc, "** Error saving account: {}", account.name);
	}

	return true;
}

bool FlatFileAccountLoader::checkSearchConditions(const std::string_view account, const std::vector<std::string>& searches) const
{
	constexpr std::array<std::string_view, 6> conditions = {">=", "<=", "!=", "=", ">", "<"};

	// Load the account data.
	std::string file;
	{
		CString fileData;
		fileData.load(account);
		if (fileData.isEmpty() || fileData.subString(0, 8) != "GRACC001")
			return false;
		file = fileData.toString();
	}

	// Go through each search and check if the conditions are met.
	for (const auto& search : searches)
	{
		// Find the condition.
		size_t condition = std::numeric_limits<size_t>::max();
		for (size_t i = 0; i < (size_t)conditions.size(); ++i)
		{
			if (search.find(conditions[i]) != std::string::npos)
			{
				condition = i;
				break;
			}
		}

		// If we didn't find a condition, fail out completely.
		if (condition == std::numeric_limits<size_t>::max())
			return false;

		// Split the search up into the components.
		std::string searchSection = search.substr(0, search.find(conditions[condition]));
		std::string searchValue = search.substr(search.find(conditions[condition]) + conditions[condition].size());

		// Check if the search value is a number.
		float searchValueNumber = 0.0f;
		const bool searchValueIsNumber = string::toFloat(searchValue, searchValueNumber);

		// Search for all matching sections.
		bool matched = false;
		size_t pos = 0;
		while (pos < file.length() && (pos = string::findi(file, searchSection, pos)) != std::string::npos)
		{
			// Get the value for this line.
			const auto start = file.find(' ', pos);
			const auto end = file.find('\n', start);
			std::string fileValue;
			{
				std::string_view value_view(file.data() + start + 1, end - start - 1);
				fileValue = string::trim(value_view);
			}

			// Check if the value is a number.
			if (float valueNum = 0.0f; string::toFloat(fileValue, valueNum) && searchValueIsNumber)
			{
				switch (condition)
				{
					case 0:
						matched |= valueNum >= searchValueNumber;
						break;
					case 1:
						matched |= valueNum <= searchValueNumber;
						break;
					case 2:
						matched |= valueNum != searchValueNumber;
						break;
					case 3:
						matched |= valueNum == searchValueNumber;
						break;
					case 4:
						matched |= valueNum > searchValueNumber;
						break;
					case 5:
						matched |= valueNum < searchValueNumber;
						break;
					default:;
				}
			}
			else
			{
				switch (condition)
				{
					case 0:
						matched |= string::comparei(fileValue, searchValue) >= 0;
						break;
					case 1:
						matched |= string::comparei(fileValue, searchValue) <= 0;
						break;
					case 2:
						matched |= string::comparei(fileValue, searchValue) != 0;
						break;
					case 3:
						matched |= string::comparei(fileValue, searchValue) == 0;
						break;
					case 4:
						matched |= string::comparei(fileValue, searchValue) > 0;
						break;
					case 5:
						matched |= string::comparei(fileValue, searchValue) < 0;
						break;
					default:;
				}
			}

			pos = end + 1;
		}

		if (!matched)
			return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
