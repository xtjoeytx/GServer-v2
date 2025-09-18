#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <string_view>
#include <string>
#include <utility>

#include <tomcrypt.h>

#include <CString.h>

#include <utilities/StringUtils.h>
#include <utilities/manager/TranslationManagerClassic.h>
#include <utilities/Log.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

constexpr std::string_view filePrefix = "slanguage"sv;

void TranslationManagerClassic::loadTranslations(const std::filesystem::path& directory)
{
	auto indent = log::server.indent();
	m_filesystem.addDir(directory.string());

	for (const auto& [file, path] : m_filesystem.getFileList())
	{
		auto fileName = file.toStringView();

		// slanguageDomain.txt
		if (!fileName.starts_with(filePrefix) || !fileName.ends_with(".txt"))
			continue;

		loadDomain(path.toString());
	}

	// Always include the "Original" domain.
	if (!m_domains.contains("Original"))
		m_domains.emplace("Original", TranslationMap{ .filename = directory / "slanguageOriginal.txt" });
}

void TranslationManagerClassic::loadDomain(const std::filesystem::path& filePath)
{
	auto domain = filePath.stem().string().substr(filePrefix.length());
	if (domain.empty())
		return;

	TranslationMap translations{ .filename = filePath };

	// Translation file format:
	// md5: "text"

	auto lines = CString::loadToken(filePath.string(), "\n", true);
	for (auto& line : lines)
	{
		auto str = string::trim(line.toStringView());
		if (str.empty()) continue;

		auto md5 = str.substr(0, 32);
		if (md5.length() != 32) continue;

		auto separator = str.find(':');
		if (separator == std::string_view::npos) continue;

		auto start = str.find('"', separator);
		if (start == std::string_view::npos) continue;
		++start;

		auto end = str.find('"', start);
		if (end == std::string_view::npos) continue;

		auto value = string::unescapeQuotes(str.substr(start, end - start));
		translations.lines.emplace(md5, std::move(value));
	}

	m_domains.emplace(std::move(domain), std::move(translations));
}

void TranslationManagerClassic::saveTranslations()
{
	for (const auto& [domain, map] : m_domains)
	{
		if (map.filename.empty())
			continue;

		std::ofstream file{ map.filename };
		if (!file.is_open())
			continue;

		for (const auto& [key, value] : map.lines)
			file << key << ": \"" << string::escapeQuotes(value) << "\"\n";

		file.close();
	}
}

std::string_view TranslationManagerClassic::getText(std::string_view language, std::string_view key)
{
	auto hash = generateHash(key);

	auto findTranslation = [this](std::string_view language, std::string_view key) -> std::string*
	{
		auto domain = m_domains.find(language);
		if (domain == m_domains.end())
			return nullptr;
		if (auto line = domain->second.lines.find(key); line != domain->second.lines.end())
			return &line->second;
		return nullptr;
	};

	// Search the target language, then "Original".
	if (auto line = findTranslation(language, hash); line != nullptr)
		return *line;
	if (auto line = findTranslation("Original", hash); line != nullptr)
		return *line;

	// Not found, add to "Original" and return the key.
	if (auto domain = m_domains.find("Original"); domain != m_domains.end())
		domain->second.lines.emplace(hash, key);

	return key;
}

std::string TranslationManagerClassic::generateHash(std::string_view key) const
{
	hash_state md5;
	uint8_t output[16]{};

	md5_init(&md5);
	md5_process(&md5, reinterpret_cast<const unsigned char*>(key.data()), key.length());
	md5_done(&md5, output);

	// Convert output to a hex string.
	std::string hexString;
	for (const auto& byte : output)
		hexString += std::format("{:02x}", byte);

	return hexString;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
