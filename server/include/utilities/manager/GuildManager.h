#ifndef GUILDMANAGER_H
#define GUILDMANAGER_H

#include <filesystem>
#include <optional>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>

#include <filesystem/FileSystem.h>
#include <utilities/CommonTypes.h>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

// Ranks inside a guild: a leader can do everything, an officer can add and remove plain members.
enum GuildRank : int { GUILD_MEMBER = 0, GUILD_OFFICER = 1, GUILD_LEADER = 2 };

struct Guild
{
	std::string name;
	std::filesystem::path filePath;
	string_multimap<std::string> members;
	string_map<int> ranks;	// account -> GuildRank (leaders and officers; everyone else is a member). Lines "#leader Account" / "#officer Account" in the file.
	bool modifiedSinceLastSave = false;
};

class GuildManager
{
public:
	using string_map_pair = std::pair<string_multimap<std::string>::const_iterator, string_multimap<std::string>::const_iterator>;

public:
	~GuildManager();

public:
	void loadGuilds(const std::filesystem::path& directory);
	void saveGuilds();
	[[a::inline]] void update();

public:
	bool guildExists(std::string_view guildName) const;
	bool verifyPlayerInGuild(std::string_view guildName, std::string_view account, std::string_view nickName = {}) const;
	std::optional<string_map_pair> getPlayerNicknamesForGuild(std::string_view guildName, std::string_view account) const;

public:
	bool createGuild(std::string_view guildName);
	bool deleteGuild(std::string_view guildName);
	bool saveGuild(std::string_view guildName);
	bool addPlayerToGuild(std::string_view guildName, std::string_view account, std::string_view nickName = {});
	bool removePlayerFromGuild(std::string_view guildName, std::string_view account, std::string_view nickName = {});
	bool removePlayerEntirelyFromGuild(std::string_view guildName, std::string_view account);

public:
	// Ranks (leader / officer / member) and lookups for the in-game guild window (gr.guild trigger commands).
	const Guild* getGuild(std::string_view guildName) const;
	int getRank(std::string_view guildName, std::string_view account) const;	// -1 if not a member
	bool setRank(std::string_view guildName, std::string_view account, int rank);
	std::vector<const Guild*> guildsOfAccount(std::string_view account) const;
	bool setMemberNick(std::string_view guildName, std::string_view account, std::string_view nickName);	// the one name the account may wear with the tag ("" = any)
	std::string getMemberNick(std::string_view guildName, std::string_view account) const;

private:
	Guild* loadGuild(const std::filesystem::path& filePath);

private:
	string_map<Guild> m_guilds;
	fs::FileSystem m_filesystem;
};

//----------------------------

inline void GuildManager::update()
{
	m_filesystem.update();
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // GUILDMANAGER_H
