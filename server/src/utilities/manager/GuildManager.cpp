#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <ostream>
#include <string_view>
#include <string>
#include <utility>

#include <utilities/Log.h>
#include <utilities/manager/GuildManager.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

GuildManager::~GuildManager()
{
	saveGuilds();
}

//----------------------------

void GuildManager::loadGuilds(const std::filesystem::path& directory)
{
	auto indent = log::server.indent();
	m_filesystem.addDir(directory.string());

	for (const auto& [file, path] : m_filesystem.getFileList())
	{
		auto fileName = file.toStringView();

		// guildMy_Guild.txt
		if (!fileName.starts_with("guild") || !fileName.ends_with(".txt"))
			continue;

		loadGuild(path.toString());

		auto printedGuild = fileName.substr(5);
		printedGuild.remove_suffix(4);
		log::printLine(log::server, string::replace(printedGuild, "_"sv, " "sv));
	}
}

void GuildManager::saveGuilds()
{
	for (auto& [guildName, guild] : m_guilds)
		saveGuild(guildName);
}

//----------------------------

void GuildManager::loadGuild(const std::filesystem::path& filePath)
{
	std::ifstream file{ filePath, std::ios::in };
	if (!file.is_open())
	{
		log::printLine(log::server, "** [Error] Could not open guild file: {}", filePath.string());
		return;
	}

	Guild guild;
	guild.name = string::replace(filePath.stem().string().substr(5), "_"sv, " "sv);
	guild.filePath = filePath;

	std::string line;
	while (std::getline(file, line))
	{
		std::string_view lineView{ line };
		lineView = string::trim(lineView);
		if (lineView.empty())
			continue;

		if (auto pos = lineView.find(':'); pos == std::string_view::npos)
		{
			guild.members.emplace(lineView, std::string{});
		}
		else
		{
			auto account = string::trimRight(lineView.substr(0, pos));
			auto nickName = string::trimLeft(lineView.substr(pos + 1));
			if (nickName.starts_with('*'))
				nickName.remove_prefix(1);

			guild.members.emplace(account, nickName);
		}
	}

	m_guilds[guild.name] = std::move(guild);
	file.close();
}

//----------------------------

bool GuildManager::guildExists(std::string_view guildName) const
{
	return m_guilds.find(guildName) != m_guilds.end();
}

bool GuildManager::verifyPlayerInGuild(std::string_view guildName, std::string_view account, std::string_view nickName) const
{
	auto it = m_guilds.find(guildName);
	if (it != m_guilds.end())
	{
		const auto& members = it->second.members;
		auto memberIt = members.find(account);
		while (memberIt != members.end())
		{
			if (memberIt->second.empty() || memberIt->second == nickName)
				return true;
			++memberIt;
		}
	}
	return false;
}

std::optional<GuildManager::string_map_pair> GuildManager::getPlayerNicknamesForGuild(std::string_view guildName, std::string_view account) const
{
	auto it = m_guilds.find(guildName);
	if (it != m_guilds.end())
	{
		const auto& members = it->second.members;
		return members.equal_range(account);
	}
	return std::nullopt;
}

//----------------------------

bool GuildManager::createGuild(std::string_view guildName)
{
	if (guildExists(guildName))
	{
		log::printLine(log::server, "** [Error] Cannot create guild that already exists: {}", guildName);
		return false;
	}

	auto directories = m_filesystem.getDirList();
	if (directories == nullptr || directories->size() == 0)
	{
		log::printLine(log::server, "** [Error] No guilds directory found.");
		return false;
	}

	Guild guild;
	guild.name = std::string{ guildName };
	guild.filePath = std::filesystem::path{ directories->front().toString() } / std::format("guild{}.txt", string::replace(guildName, " "sv, "_"sv));

	std::ofstream file{ guild.filePath, std::ios::out | std::ios::trunc };
	if (!file.is_open())
	{
		log::printLine(log::server, "** [Error] Could not create guild file: {}", guild.filePath.string());
		return false;
	}

	m_guilds[guild.name] = std::move(guild);
	file.close();

	return true;
}

bool GuildManager::deleteGuild(std::string_view guildName)
{
	if (auto it = m_guilds.find(guildName); it != m_guilds.end())
	{
		std::filesystem::remove(it->second.filePath);
		m_guilds.erase(it);
		return true;
	}
	return false;
}

bool GuildManager::saveGuild(std::string_view guildName)
{
	if (auto it = m_guilds.find(guildName); it != m_guilds.end())
	{
		Guild& guild = it->second;

		std::ofstream file{ guild.filePath, std::ios::out | std::ios::trunc };
		if (!file.is_open())
		{
			log::printLine(log::server, "** [Error] Could not save guild: {}", guildName);
			return false;
		}

		for (const auto& [account, nickName] : it->second.members)
		{
			file << account;
			if (!nickName.empty())
				file << ':' << nickName;
			file << std::endl;
		}

		file.close();
		return true;
	}
	return false;
}

bool GuildManager::addPlayerToGuild(std::string_view guildName, std::string_view account, std::string_view nickName)
{
	auto it = m_guilds.find(guildName);
	if (it != m_guilds.end())
	{
		it->second.members.emplace(account, nickName);
		return true;
	}

	// Create the guild.
	if (createGuild(guildName))
	{
		m_guilds.at(std::string{ guildName }).members.emplace(account, nickName);
		saveGuild(guildName);
		return true;
	}

	return false;
}

bool GuildManager::removePlayerFromGuild(std::string_view guildName, std::string_view account, std::string_view nickName)
{
	auto it = m_guilds.find(guildName);
	if (it != m_guilds.end())
	{
		auto& members = it->second.members;
		auto membersIt = members.equal_range(account);
		while (membersIt.first != membersIt.second)
		{
			if (membersIt.first->second == nickName)
			{
				members.erase(membersIt.first);
				return true;
			}
			++membersIt.first;
		}
	}
	return false;
}

bool GuildManager::removePlayerEntirelyFromGuild(std::string_view guildName, std::string_view account)
{
	auto it = m_guilds.find(guildName);
	if (it != m_guilds.end())
	{
		auto& members = it->second.members;
		auto result = members.erase(std::string{ account });
		return result > 0;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
