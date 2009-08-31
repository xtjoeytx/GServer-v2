#ifndef CSETTINGS_H
#define CSETTINGS_H

#include <vector>
#include "ICommon.h"
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
		bool isOpened() const;
		bool loadFile(const CString& pStr);
		void setSeparator(const CString& pSeparator);
		void clear();

		CKey* addKey(const CString& pKey, const CString& pValue);

		// Get Type
		CKey *getKey(const CString& pStr);
		const CKey *getKey(const CString& pStr) const;
		bool getBool(const CString& pStr, bool pDefault = true) const;
		float getFloat(const CString& pStr, float pDefault = 1.00) const;
		int getInt(const CString& pStr, int pDefault = 1) const;
		const CString getStr(const CString& pStr, const CString& pDefault = "") const;

		const CString operator[](int pIndex) const;
		std::vector<CString> getFile() const;

	private:
		bool opened;
		CString strSep;
		std::vector<CKey *> keys;
		std::vector<CString> strList;

		mutable boost::recursive_mutex m_preventChange;
};

inline bool CSettings::isOpened() const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return opened;
}

inline void CSettings::setSeparator(const CString& pSeparator)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	strSep = pSeparator;
}

#endif // CSETTINGS_H
