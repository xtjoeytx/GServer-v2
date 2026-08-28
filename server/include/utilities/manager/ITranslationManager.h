#ifndef ITRANSLATIONMANAGER_H
#define ITRANSLATIONMANAGER_H

#include <filesystem>
#include <string_view>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <utilities/std/generator.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

inline namespace language
{

using namespace std::literals::string_literals;

static std::unordered_map<std::string, std::string_view, string::string_hash, string::string_hash_equal> languageAliasesToClassic =
{
	{ "de"s, "Deutsch"sv },
	{ "german"s, "Deutsch"sv },
	//
	{ "en"s, "English"sv },
	{ "english"s, "English"sv },
	//
	{ "es"s, "Espa\u00F1ol"sv },
	{ "spanish"s, "Espa\u00F1ol"sv },
	{ "espanol"s, "Espa\u00F1ol"sv },
	//
	{ "fr"s, "Fran\u00E7ais"sv },
	{ "french"s, "Fran\u00E7ais"sv },
	{ "francais"s, "Fran\u00E7ais"sv },
	//
	{ "it"s, "Italiano"sv },
	{ "italian"s, "Italiano"sv },
	//
	{ "nl"s, "Nederlands"sv },
	{ "dutch"s, "Nederlands"sv },
	//
	{ "no"s, "Norsk"sv },
	{ "norwegian"s, "Norsk"sv },
	//
	{ "pt"s, "Portugu\u00EAs"sv },
	{ "portuguese"s, "Portugu\u00EAs"sv },
	{ "portugues"s, "Portugu\u00EAs"sv },
	//
	{ "sv"s, "Svenska"sv },
	{ "swedish"s, "Svenska"sv },
};

static std::unordered_map<std::string, std::string_view, string::string_hash, string::string_hash_equal> languageAliasesClassicToModern =
{
	{ "Deutsch"s, "de"sv },
	{ "English"s, "en"sv },
	{ "Espa\u00F1ol"s, "es"sv },
	{ "Fran\u00E7ais"s, "fr"sv },
	{ "Italiano"s, "it"sv },
	{ "Nederlands"s, "nl"sv },
	{ "Norsk"s, "no"sv },
	{ "Portugu\u00EAs"s, "pt"sv },
	{ "Svenska"s, "sv"sv },
};

inline constexpr auto originalLanguage = "Original"sv;

inline static std::string_view mapToClassic(const std::string_view language) noexcept
{
	if (const auto it = languageAliasesToClassic.find(language); it != languageAliasesToClassic.end())
		return it->second;
	return language;
}

} // end namespace language

//----------------------------

/// @brief Interface for managing translations.
class ITranslationManager
{
public:
	virtual ~ITranslationManager() = default;

public:
	/// @brief Loads all translations from a given directory.
	/// @param directory The directory to load translations from.
	virtual void loadTranslations(const std::filesystem::path& directory) = 0;

	/// @brief Reloads the translation data from the specified file.
	/// @param filePath The path to the translation file to reload.
	virtual void reloadTranslation(const std::filesystem::path& filePath) = 0;

	/// @brief Saves a specified translation in memory to the disk.
	virtual void saveTranslation(std::string_view domain) = 0;

	/// @brief Saves all translations in memory to the disk.
	virtual void saveTranslations() = 0;

	/// @brief Synchronizes the specified language with the original.
	/// @param language A view of the language string to synchronize.
	/// @return A std::tuple containing the name of the language (0), how many entries were added (1), and how many were removed (2).
	virtual std::tuple<std::string_view, size_t, size_t> syncLanguageWithOriginal(std::string_view language) = 0;

	/// @brief Generator that, when iterated, synchronizes all loaded languages with the original.
	/// @return A std::tuple containing the name of the language (0), how many entries were added (1), and how many were removed (2).
	virtual std::generator<std::tuple<std::string_view, size_t, size_t>> syncAllLanguagesWithOriginal() = 0;

	/// @brief Generates stubs for supported languages.
	/// @return The number of language stubs generated.
	virtual size_t generateAllLanguageStubs() = 0;

	/// @brief Registers an original text string with a given key.
	/// @param key The key identifying the original text string to register.
	virtual void registerOriginalText(std::string_view key) = 0;

public:
	/// @brief Adds a translation for a given language and key.
	/// @param language The language code specifying the desired localization.
	/// @param key The key identifying the text string to translate.
	/// @param translation The translated text string.
	/// @return True if the translation was successfully added, false otherwise.
	virtual bool addTranslation(std::string_view language, std::string_view key, std::string_view translation) = 0;

	/// @brief Retrieves a localized text string for a given language and key.
	/// @param language The language code specifying the desired localization.
	/// @param key The key identifying the text string to retrieve.
	/// @return A string view containing the localized text corresponding to the specified key and language.
	virtual std::string_view getText(std::string_view language, std::string_view key) = 0;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // ITRANSLATIONMANAGER_H
