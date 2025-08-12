#ifndef GUILDMANAGER_H
#define GUILDMANAGER_H

#include <filesystem>
#include <optional>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>

#include <FileSystem.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct Guild
{
	std::string name;
	std::filesystem::path filePath;
	string_multimap<std::string> members;
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

private:
	void loadGuild(const std::filesystem::path& filePath);

private:
	string_map<Guild> m_guilds;
	FileSystem m_filesystem;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // GUILDMANAGER_H
