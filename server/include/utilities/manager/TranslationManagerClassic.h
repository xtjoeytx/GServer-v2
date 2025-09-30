#ifndef TRANSLATIONMANAGERCLASSIC_H
#define TRANSLATIONMANAGERCLASSIC_H

#include <filesystem>
#include <generator>
#include <string_view>
#include <string>
#include <tuple>
#include <unordered_map>

#include <utilities/manager/ITranslationManager.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class TranslationManagerClassic : public ITranslationManager
{
	struct TranslationMap
	{
		std::filesystem::path filename;
		string_map<std::string> lines;
	};

public:
	virtual ~TranslationManagerClassic() override {}

public:
	virtual void loadTranslations(const std::filesystem::path& directory) override;
	virtual void reloadTranslation(const std::filesystem::path& filePath) override;
	virtual void saveTranslations() override;
	virtual std::tuple<std::string_view, size_t, size_t> syncLanguageWithOriginal(std::string_view language) override;
	virtual std::generator<std::tuple<std::string_view, size_t, size_t>> syncAllLanguagesWithOriginal() override;
	virtual size_t generateAllLanguageStubs() override;

protected:
	void loadDomain(const std::filesystem::path& filePath);
	std::string generateHash(std::string_view key) const;

public:
	virtual std::string_view getText(std::string_view language, std::string_view key) override;

protected:
	std::unordered_map<std::string, TranslationMap, string::string_hash, string::string_hash_equal> m_domains;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // TRANSLATIONMANAGERCLASSIC_H
