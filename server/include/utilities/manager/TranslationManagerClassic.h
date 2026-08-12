#ifndef TRANSLATIONMANAGERCLASSIC_H
#define TRANSLATIONMANAGERCLASSIC_H

#include <filesystem>
#include <string_view>
#include <string>
#include <tuple>
#include <unordered_map>

#include <utilities/CommonTypes.h>
#include <utilities/manager/ITranslationManager.h>
#include <utilities/std/generator.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class TranslationManagerClassic : public ITranslationManager
{
public:
	struct TranslationMap
	{
		std::filesystem::path filename;
		string_map<std::string> lines;
	};

public:
	~TranslationManagerClassic() override = default;

public:
	void loadTranslations(const std::filesystem::path& directory) override;
	void reloadTranslation(const std::filesystem::path& filePath) override;
	void saveTranslation(std::string_view domain) override;
	void saveTranslations() override;
	std::tuple<std::string_view, size_t, size_t> syncLanguageWithOriginal(std::string_view language) override;
	std::generator<std::tuple<std::string_view, size_t, size_t>> syncAllLanguagesWithOriginal() override;
	size_t generateAllLanguageStubs() override;
	void registerOriginalText(std::string_view key) override;

protected:
	void loadDomain(const std::filesystem::path& filePath);
	static std::string generateHash(std::string_view key);

public:
	std::string_view getText(std::string_view language, std::string_view key) override;

protected:
	std::unordered_map<std::string, TranslationMap, string::string_hash, string::string_hash_equal> m_domains;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // TRANSLATIONMANAGERCLASSIC_H
