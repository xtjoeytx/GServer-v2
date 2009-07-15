#include "CTranslationManager.h"

// -- Function: Translate String -- //
const char * CTranslationManager::translate(const std::string& pLanguage, const std::string& pKey)
{
	if (mTranslationList[pLanguage][pKey].empty())
	{
		this->add(pLanguage, pKey, "");
		return pKey.c_str();
	}

	return mTranslationList[pLanguage][pKey].c_str();
}

// -- Function: Add Translation -- //
void CTranslationManager::add(const std::string& pLanguage, const std::string& pKey, const std::string& pTranslation)
{
	mTranslationList[pLanguage][pKey] = pTranslation;
}

// -- Function: Remove Translation -- //
void CTranslationManager::remove(const std::string& pLanguage, const std::string& pKey)
{
	mTranslationList[pLanguage].erase(pKey);
}

// -- Function: Reset Language -- //
void CTranslationManager::reset(const std::string& pLanguage)
{
	mTranslationList[pLanguage].clear();
}

// -- Function: Reset All Languages -- //
void CTranslationManager::reset()
{
	// Clear Languages..?
	for (std::map<std::string, STRMAP>::iterator i = mTranslationList.begin(); i != mTranslationList.end(); ++i)
		(*i).second.clear();

	/* -- unsure if this would be enough to free the sub-maps -- */
	// mTranslationList.clear();
}
