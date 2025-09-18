#ifndef TRANSLATIONMANAGERCLASSIC_H
#define TRANSLATIONMANAGERCLASSIC_H

#include <filesystem>
#include <string_view>
#include <string>

#include <utilities/manager/ITranslationManager.h>
#include <utilities/CommonTypes.h>

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
	virtual void loadTranslations(const std::filesystem::path& directory);
	virtual void saveTranslations();

protected:
	void loadDomain(const std::filesystem::path& filePath);
	std::string generateHash(std::string_view key) const;

public:
	virtual std::string_view getText(std::string_view language, std::string_view key);

protected:
	string_map<TranslationMap> m_domains;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // TRANSLATIONMANAGERCLASSIC_H
