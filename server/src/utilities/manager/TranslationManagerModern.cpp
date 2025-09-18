#include <filesystem>
#include <string>
#include <string_view>

#include <utilities/manager/TranslationManagerModern.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

void TranslationManagerModern::loadTranslations(const std::filesystem::path& directory)
{
}

void TranslationManagerModern::saveTranslations()
{
}

std::string_view TranslationManagerModern::getText(std::string_view language, std::string_view key)
{
	return key;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
