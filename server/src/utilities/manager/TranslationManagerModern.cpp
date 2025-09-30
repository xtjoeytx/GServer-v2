#include <filesystem>
#include <generator>
#include <string_view>
#include <tuple>

#include <utilities/manager/TranslationManagerModern.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

void TranslationManagerModern::loadTranslations(const std::filesystem::path& directory)
{
}

void TranslationManagerModern::reloadTranslation(const std::filesystem::path& filePath)
{
}

void TranslationManagerModern::saveTranslations()
{
}

std::tuple<std::string_view, size_t, size_t> TranslationManagerModern::syncLanguageWithOriginal(std::string_view language)
{
	return { "not implemented"sv, 0, 0};
}

std::generator<std::tuple<std::string_view, size_t, size_t>> TranslationManagerModern::syncAllLanguagesWithOriginal()
{
	co_return;
}

size_t TranslationManagerModern::generateAllLanguageStubs()
{
	return 0;
}

std::string_view TranslationManagerModern::getText(std::string_view language, std::string_view key)
{
	return key;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
