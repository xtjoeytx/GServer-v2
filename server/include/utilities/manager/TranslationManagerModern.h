#ifndef TRANSLATIONMANAGERMODERN_H
#define TRANSLATIONMANAGERMODERN_H

#include <filesystem>
#include <string_view>
#include <tuple>

#include <utilities/manager/ITranslationManager.h>
#include <utilities/std/generator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class TranslationManagerModern : public ITranslationManager
{
public:
	~TranslationManagerModern() override = default;

public:
	void loadTranslations(const std::filesystem::path& directory) override;
	void reloadTranslation(const std::filesystem::path& filePath) override;
	void saveTranslation(std::string_view domain) override;
	void saveTranslations() override;
	std::tuple<std::string_view, size_t, size_t> syncLanguageWithOriginal(std::string_view language) override;
	std::generator<std::tuple<std::string_view, size_t, size_t>> syncAllLanguagesWithOriginal() override;
	size_t generateAllLanguageStubs() override;
	void registerOriginalText(std::string_view key) override;

public:
	std::string_view getText(std::string_view language, std::string_view key) override;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // TRANSLATIONMANAGERMODERN_H
