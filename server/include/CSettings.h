#ifndef CSETTINGS_H
#define CSETTINGS_H

#include <vector>
#include "CString.h"

struct CKey
{
	CKey(const CString& pName, const CString& pValue)
	{
		name  = pName;
		value = pValue;
	}

	CString name;
	CString value;
};

class CSettings
{
	public:
		// Constructor-Destructor
		CSettings();
		CSettings(const CString& pStr, const CString& pSeparator = "=");
		~CSettings();

		// File-Loading Functions
		bool isOpened();
		bool loadFile(const CString& pStr);
		void setSeparator(const CString& pSeparator);
		void clear();

		// Get Type
		CKey *getKey(CString pStr);
		bool getBool(const CString& pStr, bool pDefault = true);
		float getFloat(const CString& pStr, float pDefault = 1.00);
		int getInt(const CString& pStr, int pDefault = 1);
		const CString& getStr(const CString& pStr, const CString& pDefault = "");

		const CString& operator[](int pIndex);
		std::vector<CString> getFile();

	private:
		bool opened;
		CString strSep;
		std::vector<CKey *> keys;
		std::vector<CString> strList;
};

inline bool CSettings::isOpened()
{
	return opened;
}

inline void CSettings::setSeparator(const CString& pSeparator)
{
	strSep = pSeparator;
}

#endif // CSETTINGS_H
