#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <string_view>
#include <string>
#include <tuple>
#include <utility>

#include <tomcrypt.h>

#include <CString.h>

#include <filesystem/File.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>
#include <utilities/manager/ITranslationManager.h>
#include <utilities/manager/TranslationManagerClassic.h>
#include <utilities/std/generator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

// https://r12a.github.io/app-conversion/
constexpr std::array<std::string_view, 9> supportedLanguages =
{
	"Deutsch"sv,
	"English"sv,
	"Espa\u00F1ol"sv,
	"Fran\u00E7ais"sv,
	"Italiano"sv,
	"Nederlands"sv,
	"Norsk"sv,
	"Portugu\u00EAs"sv,
	"Svenska"sv,
};

constexpr auto filePrefix = "slanguage"sv;
constexpr auto originalLanguage = "Original"sv;

//----------------------------

static void saveTranslationFromMap(const TranslationManagerClassic::TranslationMap& map)
{
	if (map.filename.empty())
		return;

	std::ofstream file{map.filename};
	if (!file.is_open())
		return;

	for (const auto& [key, value] : map.lines)
		file << key << ": \"" << string::escapeQuotes(value) << "\"\n";
}

//----------------------------

void TranslationManagerClassic::loadTranslations(const std::filesystem::path& directory)
{
	auto indent = log::server.indent();

	std::filesystem::directory_iterator dirSearch{directory, std::filesystem::directory_options::follow_directory_symlink | std::filesystem::directory_options::skip_permission_denied};
	for (const auto& entry : dirSearch)
	{
		if (!entry.is_regular_file())
			continue;

		// slanguageDomain.txt
		const auto fileName = fs::getANSIFileName(entry.path());
		if (!fileName.starts_with(filePrefix) || !fileName.ends_with(".txt"))
			continue;

		loadDomain(entry.path());
	}

	// Always include the "Original" domain.
	if (!m_domains.contains(originalLanguage))
		m_domains.emplace(originalLanguage, TranslationMap{.filename = directory / "slanguageOriginal.txt"});
}

void TranslationManagerClassic::reloadTranslation(const std::filesystem::path& filePath)
{
	// slanguageDomain.txt
	const auto file = fs::getANSIFileName(filePath);
	if (!file.starts_with(filePrefix) || !file.ends_with(".txt"))
		return;

	loadDomain(filePath);
}

void TranslationManagerClassic::loadDomain(const std::filesystem::path& filePath)
{
	auto domain = fs::getANSIFileName(filePath.stem()).substr(filePrefix.length());
	if (domain.empty())
		return;

	TranslationMap translations{.filename = filePath};

	// Translation file format:
	// md5: "text"

	const auto lines = CString::loadToken(filePath.string(), "\n", true);
	for (auto& line : lines)
	{
		auto str = string::trim(line.toStringView());
		if (str.empty()) continue;

		auto md5 = str.substr(0, 32);
		if (md5.length() != 32) continue;

		const auto separator = str.find(':');
		if (separator == std::string_view::npos) continue;

		auto start = str.find('"', separator);
		if (start == std::string_view::npos) continue;
		++start;

		const auto end = str.find('"', start);
		if (end == std::string_view::npos) continue;

		auto value = string::unescapeQuotes(str.substr(start, end - start));
		translations.lines.emplace(md5, std::move(value));
	}

	m_domains.emplace(std::move(domain), std::move(translations));
}

void TranslationManagerClassic::saveTranslation(const std::string_view domain)
{
	const auto iter = m_domains.find(domain);
	if (iter == m_domains.end())
		return;

	saveTranslationFromMap(iter->second);
}

void TranslationManagerClassic::saveTranslations()
{
	for (const auto& map : m_domains | std::views::values)
	{
		if (map.filename.empty())
			continue;

		saveTranslationFromMap(map);
	}
}

std::tuple<std::string_view, size_t, size_t> TranslationManagerClassic::syncLanguageWithOriginal(const std::string_view language)
{
	std::tuple<std::string_view, size_t, size_t> result{""sv, 0, 0};
	constexpr size_t addIndex = 1;
	constexpr size_t removeIndex = 2;

	// Don't sync original with original.
	// Instead, just save.
	if (string::equalsi(language, originalLanguage))
	{
		saveTranslation(originalLanguage);
		std::get<0>(result) = originalLanguage;
		return result;
	}

	// Find the original domain.
	auto original = m_domains.find(originalLanguage);
	if (original == m_domains.end())
		return result;

	// Get our calculated language domain.
	auto calculatedLanguage = language::mapToClassic(language);
	auto domain = m_domains.find(calculatedLanguage);
	if (domain == m_domains.end())
	{
		if (std::ranges::find(supportedLanguages, calculatedLanguage) == std::ranges::end(supportedLanguages))
			return result;

		std::string languageFile{filePrefix};
		languageFile.append(calculatedLanguage).append(".txt");
		domain = m_domains.emplace(calculatedLanguage, TranslationMap{.filename = original->second.filename.parent_path() / languageFile}).first;
	}

	// Can't find a language, return failure.
	if (domain == m_domains.end())
		return result;

	// Record the calculated language.
	std::get<0>(result) = calculatedLanguage;

	// File for unused translations.
	std::filesystem::path unusedFileName = domain->second.filename;
	unusedFileName.replace_extension(".unused");
	{
		std::ofstream unusedFile;

		// Check for any translations that aren't in the original file anymore.
		for (auto iter = domain->second.lines.begin(); iter != domain->second.lines.end();)
		{
			const auto& [key, value] = *iter;
			if (!original->second.lines.contains(key))
			{
				if (!unusedFile.is_open())
					unusedFile.open(unusedFileName, std::ios::out | std::ios::app);
				if (unusedFile.is_open())
				{
					unusedFile << key << ": \"" << string::escapeQuotes(value) << "\"\n";
					iter = domain->second.lines.erase(iter);
					++std::get<removeIndex>(result);
					continue;
				}
			}
			++iter;
		}
	}

	// Add any translations from the original that aren't in this language file.
	for (const auto& [key, value] : original->second.lines)
	{
		if (!domain->second.lines.contains(key))
		{
			domain->second.lines.emplace(key, value);
			++std::get<addIndex>(result);
		}
	}

	// Save the translations.
	std::ofstream file{domain->second.filename};
	if (file.is_open())
	{
		for (const auto& [key, value] : domain->second.lines)
			file << key << ": \"" << string::escapeQuotes(value) << "\"\n";
	}

	return result;
}

std::generator<std::tuple<std::string_view, size_t, size_t>> TranslationManagerClassic::syncAllLanguagesWithOriginal()
{
	for (const auto& domain : m_domains | std::views::keys)
	{
		if (domain == originalLanguage)
			continue;

		co_yield syncLanguageWithOriginal(domain);
	}
}

size_t TranslationManagerClassic::generateAllLanguageStubs()
{
	size_t count = 0;
	for (const auto& language : supportedLanguages)
	{
		auto result = syncLanguageWithOriginal(language);
		if (std::get<1>(result) != 0)
			++count;
	}
	return count;
}

void TranslationManagerClassic::registerOriginalText(const std::string_view key)
{
	auto hash = generateHash(key);

	const auto domain = m_domains.find(originalLanguage);
	if (domain == m_domains.end())
		return;

	if (const auto line = domain->second.lines.find(key); line != domain->second.lines.end())
		return;

	// Not found, add to "Original" and return the key.
	domain->second.lines.emplace(hash, key);
}

std::string_view TranslationManagerClassic::getText(const std::string_view language, const std::string_view key)
{
	auto hash = generateHash(key);

	auto findTranslation = [this](const std::string_view lang, const std::string_view k) -> std::string*
	{
		const auto domain = m_domains.find(lang);
		if (domain == m_domains.end())
			return nullptr;
		if (const auto line = domain->second.lines.find(k); line != domain->second.lines.end())
			return &line->second;
		return nullptr;
	};

	// Search the target language, then "Original".
	if (const auto line = findTranslation(language, hash); line != nullptr)
		return *line;
	if (const auto line = findTranslation(originalLanguage, hash); line != nullptr)
		return *line;

	// Not found, add to "Original" and return the key.
	if (const auto domain = m_domains.find(originalLanguage); domain != m_domains.end())
		domain->second.lines.emplace(hash, key);

	return key;
}

std::string TranslationManagerClassic::generateHash(const std::string_view key)
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
