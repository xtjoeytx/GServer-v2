#include "ICommon.h"
#include "CSettings.h"

/*
	Constructor - Deconstructor
*/
CSettings::CSettings()
{
}

CSettings::CSettings(const CString& pStr, const CString& pSeparator)
{
	strSep = pSeparator;
	opened = loadFile(pStr);
}

CSettings::~CSettings()
{
	clear();
}

/*
	File-Loading Functions
*/
bool CSettings::loadFile(const CString& pStr)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// definitions
	CString fileData;

	// Clear Keys
	clear();

	// Load File
	if (!fileData.load(pStr))
	{
		opened = false;
		return false;
	}

	// Parse Data
	fileData.removeAllI("\r");
	strList = fileData.tokenize("\n");
	for (unsigned int i = 0; i < strList.size(); i++)
	{
		// Skip Comments
		if (strList[i][0] == '#' || strList[i].find(strSep) == -1)
			continue;

		// Tokenize Line && Trim && Lowercase Key Name
		std::vector<CString> line = strList[i].tokenize(strSep);
		line[0].toLowerI();
		if (line.size() == 1) continue;

		// Fix problem involving settings with an = in the value.
		if (line.size() > 2)
		{
			for (unsigned int j = 2; j < line.size(); ++j)
				line[1] << "=" << line[j];
		}

		// Trim
		for (unsigned int j = 0; j < line.size(); j++)
			line[j].trimI();

		// Create Key
		CKey *key;
		if ((key = getKey(line[0])) == 0)
			keys.push_back(new CKey(line[0], line[1]));
		else
			key->value << "," << line[1];
	}

	opened = true;
	return true;
}

void CSettings::clear()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Clear Keys
	for (unsigned int i = 0; i < keys.size(); i++)
		delete keys[i];
	keys.clear();
}

/*
	Get Settings
*/
CKey * CSettings::getKey(const CString& pStr)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Lowercase Name
	CString strName = pStr.toLower();

	// Iterate key List
	for (std::vector<CKey *>::iterator i = keys.begin(); i != keys.end(); ++i)
	{
		if ((*i)->name == strName)
			return *i;
	}

	// None :(
	return 0;
}

const CKey* CSettings::getKey(const CString& pStr) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Lowercase Name
	CString strName = pStr.toLower();

	// Iterate key List
	for (std::vector<CKey *>::const_iterator i = keys.begin(); i != keys.end(); ++i)
	{
		if ((*i)->name == strName)
			return *i;
	}

	// None :(
	return 0;
}

bool CSettings::getBool(const CString& pStr, bool pDefault) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : (key->value == "true" || key->value == "1"));
}

float CSettings::getFloat(const CString& pStr, float pDefault) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : (float)strtofloat(key->value));
}

int CSettings::getInt(const CString& pStr, int pDefault) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : strtoint(key->value));
}

const CString CSettings::getStr(const CString& pStr, const CString& pDefault) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : key->value);
}

const CString CSettings::operator[](int pIndex) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	return strList[pIndex].trim();
}

std::vector<CString> CSettings::getFile() const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	std::vector<CString> newStrList;
	std::copy(strList.begin(), strList.end(), newStrList.begin());
	return newStrList;
}
