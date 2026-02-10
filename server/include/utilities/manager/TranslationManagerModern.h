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
	virtual ~TranslationManagerModern() override {};

public:
	virtual void loadTranslations(const std::filesystem::path& directory) override;
	virtual void reloadTranslation(const std::filesystem::path& filePath) override;
	virtual void saveTranslations() override;
	virtual std::tuple<std::string_view, size_t, size_t> syncLanguageWithOriginal(std::string_view language) override;
	virtual std::generator<std::tuple<std::string_view, size_t, size_t>> syncAllLanguagesWithOriginal() override;
	virtual size_t generateAllLanguageStubs() override;

public:
	virtual std::string_view getText(std::string_view language, std::string_view key) override;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // TRANSLATIONMANAGERMODERN_H
