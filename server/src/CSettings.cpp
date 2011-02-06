#include "IDebug.h"
#ifndef NO_BOOST
#	include <boost/thread.hpp>
#endif
#include "CSettings.h"

/*
	Constructor - Deconstructor
*/
CSettings::CSettings()
{
#ifndef NO_BOOST
	m_preventChange = new boost::recursive_mutex();
#endif
}

CSettings::CSettings(const CString& pStr, const CString& pSeparator)
{
#ifndef NO_BOOST
	m_preventChange = new boost::recursive_mutex();
#endif
	strSep = pSeparator;
	opened = loadFile(pStr);
}

CSettings::~CSettings()
{
	clear();
#ifndef NO_BOOST
	delete m_preventChange;
#endif
}

/*
	File-Loading Functions
*/
bool CSettings::isOpened() const
{
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif
	return opened;
}

void CSettings::setSeparator(const CString& pSeparator)
{
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif
	strSep = pSeparator;
}

bool CSettings::loadFile(const CString& pStr)
{
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif

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
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif

	// Clear Keys
	for (unsigned int i = 0; i < keys.size(); i++)
		delete keys[i];
	keys.clear();
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
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif

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
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif

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
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif
	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : (key->value == "true" || key->value == "1"));
}

float CSettings::getFloat(const CString& pStr, float pDefault) const
{
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif
	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : (float)strtofloat(key->value));
}

int CSettings::getInt(const CString& pStr, int pDefault) const
{
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif
	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : strtoint(key->value));
}

const CString CSettings::getStr(const CString& pStr, const CString& pDefault) const
{
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif
	const CKey *key = getKey(pStr);
	return (key == 0 ? pDefault : key->value);
}

const CString CSettings::operator[](int pIndex) const
{
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif
	return strList[pIndex].trim();
}

std::vector<CString> CSettings::getFile() const
{
#ifndef NO_BOOST
	boost::recursive_mutex::scoped_lock lock(*m_preventChange);
#endif

	std::vector<CString> newStrList;
	std::copy(strList.begin(), strList.end(), newStrList.begin());
	return newStrList;
}
