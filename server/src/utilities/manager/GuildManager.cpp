#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <ostream>
#include <string_view>
#include <string>
#include <utility>

#include <BabyDI.h>
#include <Server.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/manager/GuildManager.h>
#include <utilities/PropertySerializers.h>
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
	m_filesystem.categoryEventCallback[ENUM(fs::FileCategory::FILE)] = [this](fs::FileEventCollection events, fs::FileData& fileData)
	{
		if (events.test(fs::FileEvent::Deleted))
		{
			auto guildName = string::replace(fs::getFileNameAsANSI(fileData.file.stem()).substr(5), "_"sv, " "sv);
			m_guilds.erase(guildName);
			log::printLine(log::server, "Guild '{}' removed from filesystem.", guildName);

			auto server = BabyDI::Get<Server>();
			for (const auto& [playerId, player] : players_of_type<PlayerClient>(server->getPlayerList()))
			{
				// If the player was in the guild, set their nickname prop so it gets stripped.
				if (player->getGuild() == guildName)
				{
					player->setNick(player->account.character.nickName);
					player->sendPropsFromResults(player->setPropWith<PlayerProp::NICKNAME>(props::SetBy::SERVER, player->account.character.nickName));
				}
			}
		}
		if (events.test(fs::FileEvent::Added))
		{
			if (auto guild = loadGuild(fileData.file); guild != nullptr)
				log::printLine(log::server, "Guild '{}' loaded from filesystem.", guild->name);
		}
		if (events.test(fs::FileEvent::Modified))
		{
			if (auto guild = loadGuild(fileData.file); guild != nullptr)
				log::printLine(log::server, "Guild '{}' modified in filesystem.", guild->name);
		}
	};

	m_filesystem.addFoldersConfigEntry(fs::FileCategory::FILE, directory / "guild*.txt");
	m_filesystem.bind(directory);
	m_filesystem.waitUntilFilesSearched();

	for (const auto& info : m_filesystem.info(fs::FileCategory::FILE))
	{
		if (auto guild = loadGuild(info.file); guild != nullptr)
			log::printLine(log::server, guild->name);
	}
}

void GuildManager::saveGuilds()
{
	for (auto& [guildName, guild] : m_guilds)
		saveGuild(guildName);
}

//----------------------------

Guild* GuildManager::loadGuild(const std::filesystem::path& filePath)
{
	std::ifstream file{ filePath, std::ios::in };
	if (!file.is_open())
	{
		log::printLine(log::server, "** [Error] Could not open guild file: {}", filePath.generic_string());
		return nullptr;
	}

	auto guildName = string::replace(fs::getFileNameAsANSI(filePath.stem()).substr(5), "_"sv, " "sv);
	Guild guild{ .name = guildName, .filePath = filePath };

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
	m_guilds[guildName] = std::move(guild);
	file.close();

	return &m_guilds.at(guildName);
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

	auto directories = m_filesystem.getManagedDirectories();
	auto directory = directories.begin();
	if (directory == directories.end())
	{
		log::printLine(log::server, "** [Error] No guilds directory found.");
		return false;
	}

	Guild guild;
	guild.name = std::string{ guildName };
	guild.filePath = *directory / std::format("guild{}.txt", string::replace(guildName, " "sv, "_"sv));

	std::ofstream file{ guild.filePath, std::ios::out | std::ios::trunc };
	if (!file.is_open())
	{
		log::printLine(log::server, "** [Error] Could not create guild file: {}", guild.filePath.generic_string());
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
		//m_guilds.erase(it);
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

		// Update the file mod time so we don't get a file modified event.
		if (auto fileInfo = m_filesystem.info(fs::FileCategory::FILE, guild.filePath.filename()); fileInfo != nullptr)
			fileInfo->refreshModTime();

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
