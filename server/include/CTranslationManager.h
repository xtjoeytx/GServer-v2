#ifndef CTRANSLATIONMANAGER_H
#define CTRANSLATIONMANAGER_H

#include <map>
#include <string>
#include <vector>

typedef std::map<std::string, std::string> STRMAP;

// -- Class: CTranslationManager -- //
class CTranslationManager
{
	public:
		// -- Functions -> Get -- //
		inline std::map<std::string, STRMAP> * getTranslationList()
		{
			return &mTranslationList;
		}

		// -- Functions -> Translation Management -- //
		const char * translate(const std::string& pLanguage, const std::string& pKey);
		void add(const std::string& pLanguage, const std::string& pKey, const std::string& pTranslation);
		void remove(const std::string& pLanguage, const std::string& pKey);

		// -- Functions -> Language Management -- //
		void reset();
		void reset(const std::string& pLanguage);

	protected:
		std::map<std::string, STRMAP> mTranslationList;
};

#endif