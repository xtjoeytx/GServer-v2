#ifndef ITRANSLATIONMANAGER_H
#define ITRANSLATIONMANAGER_H

#include <filesystem>
#include <string_view>

#include <FileSystem.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class ITranslationManager
{
public:
	virtual ~ITranslationManager() {};

public:
	virtual void loadTranslations(const std::filesystem::path& directory) = 0;
	virtual void saveTranslations() = 0;

public:
	virtual std::string_view getText(std::string_view language, std::string_view key) = 0;

protected:
	FileSystem m_filesystem;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // ITRANSLATIONMANAGER_H
