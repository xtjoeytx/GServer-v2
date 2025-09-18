#ifndef TRANSLATIONMANAGERMODERN_H
#define TRANSLATIONMANAGERMODERN_H

#include <filesystem>
#include <string_view>

#include <utilities/manager/ITranslationManager.h>

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
	virtual void saveTranslations() override;

public:
	virtual std::string_view getText(std::string_view language, std::string_view key) override;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // TRANSLATIONMANAGERMODERN_H
