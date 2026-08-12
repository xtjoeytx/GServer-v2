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
	m_filesystem.categoryEventCallback[ENUM(fs::FileCategory::FILE)] = [this](const fs::FileEventCollection events, const fs::FileData& fileData)
	{
		if (events.test(fs::FileEvent::Deleted))
		{
			const auto guildName = string::replace(fs::getANSIFileName(fileData.file.stem()).substr(5), "_"sv, " "sv);
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
			if (const auto guild = loadGuild(fileData.file); guild != nullptr)
				log::printLine(log::server, "Guild '{}' loaded from filesystem.", guild->name);
		}
		if (events.test(fs::FileEvent::Modified))
		{
			if (const auto guild = loadGuild(fileData.file); guild != nullptr)
				log::printLine(log::server, "Guild '{}' modified in filesystem.", guild->name);
		}
	};

	m_filesystem.addFoldersConfigEntry(fs::FileCategory::FILE, directory / "guild*.txt");
	m_filesystem.bind(directory);
	m_filesystem.waitUntilFilesSearched();

	for (auto info : m_filesystem.info(fs::FileCategory::FILE) | toSharedPtr)
	{
		if (info == nullptr) continue;
		if (const auto guild = loadGuild(info->file); guild != nullptr)
			log::printLine(log::server, guild->name);
	}
}

void GuildManager::saveGuilds()
{
	for (const auto& guildName : m_guilds | std::views::keys)
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

	auto guildName = string::replace(fs::getANSIFileName(filePath.stem()).substr(5), "_"sv, " "sv);
	Guild guild{ .name = guildName, .filePath = filePath };

	std::string line;
	while (std::getline(file, line))
	{
		std::string_view lineView{line};
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

bool GuildManager::guildExists(const std::string_view guildName) const
{
	return m_guilds.contains(guildName);
}

bool GuildManager::verifyPlayerInGuild(const std::string_view guildName, const std::string_view account, const std::string_view nickName) const
{
	if (const auto it = m_guilds.find(guildName); it != m_guilds.end())
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

std::optional<GuildManager::string_map_pair> GuildManager::getPlayerNicknamesForGuild(const std::string_view guildName, const std::string_view account) const
{
	if (const auto it = m_guilds.find(guildName); it != m_guilds.end())
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
	guild.name = std::string{guildName};
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

bool GuildManager::deleteGuild(const std::string_view guildName)
{
	if (const auto it = m_guilds.find(guildName); it != m_guilds.end())
	{
		std::filesystem::remove(it->second.filePath);
		//m_guilds.erase(it);
		return true;
	}
	return false;
}

bool GuildManager::saveGuild(const std::string_view guildName)
{
	if (const auto it = m_guilds.find(guildName); it != m_guilds.end())
	{
		Guild& guild = it->second;
		if (!guild.modifiedSinceLastSave)
			return true;

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
		guild.modifiedSinceLastSave = false;

		// Update the file mod time so we don't get a file modified event.
		if (const auto fileInfo = m_filesystem.info(fs::FileCategory::FILE, guild.filePath.filename()); fileInfo != nullptr)
			fileInfo->refreshModTime();

		return true;
	}
	return false;
}

bool GuildManager::addPlayerToGuild(const std::string_view guildName, const std::string_view account, const std::string_view nickName)
{
	if (const auto it = m_guilds.find(guildName); it != m_guilds.end())
	{
		it->second.members.emplace(account, nickName);
		it->second.modifiedSinceLastSave = true;
		return true;
	}

	// Create the guild.
	if (createGuild(guildName))
	{
		auto& newGuild = m_guilds.at(std::string{guildName});
		newGuild.members.emplace(account, nickName);
		newGuild.modifiedSinceLastSave = true;

		saveGuild(guildName);
		return true;
	}

	return false;
}

bool GuildManager::removePlayerFromGuild(const std::string_view guildName, const std::string_view account, const std::string_view nickName)
{
	if (const auto it = m_guilds.find(guildName); it != m_guilds.end())
	{
		auto& members = it->second.members;
		auto [start, end] = members.equal_range(account);
		while (start != end)
		{
			if (start->second == nickName)
			{
				members.erase(start);
				it->second.modifiedSinceLastSave = true;
				return true;
			}
			++start;
		}
	}
	return false;
}

bool GuildManager::removePlayerEntirelyFromGuild(const std::string_view guildName, const std::string_view account)
{
	if (const auto it = m_guilds.find(guildName); it != m_guilds.end())
	{
		auto& members = it->second.members;
		const auto result = members.erase(std::string{account});
		it->second.modifiedSinceLastSave = true;
		return result > 0;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
