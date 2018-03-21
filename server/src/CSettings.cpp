#include "IDebug.h"
#include "CSettings.h"

/*
	Constructor - Deconstructor
*/
CSettings::CSettings()
{
	m_preventChange = new std::recursive_mutex();
}

CSettings::CSettings(const CString& pStr, const CString& pSeparator)
{
	m_preventChange = new std::recursive_mutex();

	strSep = pSeparator;
	opened = loadFile(pStr);
}

CSettings::~CSettings()
{
	clear();
	delete m_preventChange;
}

/*
	File-Loading Functions
*/
bool CSettings::isOpened() const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);
	return opened;
}

void CSettings::setSeparator(const CString& pSeparator)
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);
	strSep = pSeparator;
}

bool CSettings::loadFile(const CString& pStr)
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

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
		// Strip out comments.
		int comment_pos = strList[i].find("#");
		if (comment_pos != -1)
			strList[i].removeI(comment_pos);

		// Skip invalid or blank lines.
		if (strList[i].isEmpty() || strList[i].find(strSep) == -1)
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
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	// Clear Keys
	for (unsigned int i = 0; i < keys.size(); i++)
		delete keys[i];
	keys.clear();
}

bool CSettings::exists(const CString& pKey) const
{
	// Lowercase Name
	CString strName = pKey.toLower();

	// Iterate key List
	for (std::vector<CKey *>::const_iterator i = keys.begin(); i != keys.end(); ++i)
	{
		if ((*i)->name == strName)
			return true;
	}

	return false;
}

CKey* CSettings::addKey(const CString& pKey, const CString& pValue)
{
	CKey* k = getKey(pKey);
	if (k)
	{
		k->value = pValue;
		return k;
	}

	k = new CKey(pKey, pValue);
	keys.push_back(k);
	return k;
}

/*
	Get Settings
*/
CKey * CSettings::getKey(const CString& pStr)
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

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
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

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
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : (key->value == "true" || key->value == "1"));
}

float CSettings::getFloat(const CString& pStr, float pDefault) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : (float)strtofloat(key->value));
}

int CSettings::getInt(const CString& pStr, int pDefault) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : strtoint(key->value));
}

const CString CSettings::getStr(const CString& pStr, const CString& pDefault) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : key->value);
}

const CString CSettings::operator[](int pIndex) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);
	return strList[pIndex].trim();
}

std::vector<CString> CSettings::getFile() const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	std::vector<CString> newStrList;
	std::copy(strList.begin(), strList.end(), newStrList.begin());
	return newStrList;
}
