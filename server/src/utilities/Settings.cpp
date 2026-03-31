#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <string>
#include <utility>

#include <BabyDI.h>

#include <Server.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/Settings.h>
#include <utilities/StringUtils.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

void Settings::load(const std::filesystem::path& file)
{
	auto server = BabyDI::Get<Server>();
	if (server == nullptr)
		throw std::runtime_error("Failed to get server instance in Settings::load.");

	auto serveroptions = server->getFileSystemServer().openi(fs::FileCategory::CONFIG, file);
	if (serveroptions == nullptr)
	{
		log::printLine(log::server, "[ERROR] Failed to load {}, the file may be missing or malformed.", file.string());
		return;
	}

	string_ordered_multimap<std::string> oldSettings = std::move(m_settings);
	m_settings.clear();

	// Read all of the new settings.
	for (const auto& curLine : serveroptions->readAllLines())
	{
		std::string_view line = string::trim(curLine);
		if (line.empty() || line.starts_with('#'))
			continue;

		auto sep = line.find('=');
		if (sep == std::string_view::npos)
			continue;

		auto comment = line.find('#');
		auto key = string::trim(line.substr(0, sep));
		auto value = string::trim(line.substr(sep + 1, comment - sep));

		m_settings.emplace(key, value);
	}

	string_set settingWasChanged;

	// Walk through the settings and assemble a list of all changed settings.
	for (const auto& [key, value] : m_settings)
	{
		// Check if this value is contained in oldSettings.
		auto [begin, end] = oldSettings.equal_range(key);
		auto oldValues = std::ranges::subrange(begin, end) | std::views::values;
		if (!std::ranges::contains(oldValues, value))
			settingWasChanged.insert(key);
	}

	// Find any deleted settings by walking through the old settings and checking if they are in the new settings.
	for (const auto& [key, value] : oldSettings)
	{
		if (m_settings.find(key) == m_settings.end())
			settingWasChanged.insert(key);
	}

	// Post update events for any changed settings.
	for (const auto& key : settingWasChanged)
	{
		if (auto eventIt = m_settingUpdateEvents.find(key); eventIt != m_settingUpdateEvents.end())
		{
			auto& event = eventIt->second;
			event.post();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
