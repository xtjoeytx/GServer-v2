#include "IDebug.h"
#include "CString.h"

#if defined(_WIN32) || defined(_WIN64)
	#ifdef _MSC_VER
		#define strncasecmp _strnicmp
		#define snprintf _snprintf
	#endif
#endif

/*
	Constructor ~ Deconstructor
*/

CString::CString()
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	clear(30);
}

CString::CString(const char *pString)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	if (pString == 0)
	{
		clear(30);
		return;
	}

	int length = strlen(pString);
	if (length != 0)
	{
		clear(length);
		write(pString, length);
	}
	else clear(30);
}

CString::CString(const CString& pString)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	clear(pString.length());
	write(pString.text(), pString.length());
}

CString::CString(char pChar)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	clear(sizeof(char));
	writeChar(pChar);
}

CString::CString(double pDouble)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	char tempBuff[32];
	sprintf(tempBuff, "%f", pDouble);
	*this = tempBuff;
}

CString::CString(float pFloat)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	char tempBuff[32];
	sprintf(tempBuff, "%.2f", pFloat);
	*this = tempBuff;
}

CString::CString(int pInteger)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	char tempBuff[32];
	sprintf(tempBuff, "%i", pInteger);
	*this = tempBuff;
}

CString::CString(unsigned int pUInteger)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	char tempBuff[32];
	sprintf(tempBuff, "%u", pUInteger);
	*this = tempBuff;
}

CString::CString(long pLInteger)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	char tempBuff[32];
	sprintf(tempBuff, "%ld", pLInteger);
	*this = tempBuff;
}

CString::CString(unsigned long pLUInteger)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	char tempBuff[32];
	sprintf(tempBuff, "%lu", pLUInteger);
	*this = tempBuff;
}

CString::CString(long long pLLInteger)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	char tempBuff[64];
	sprintf(tempBuff, "%lld", pLLInteger);
	*this = tempBuff;
}

CString::CString(unsigned long long pLLUInteger)
: buffer(0), buffc(0), sizec(0), readc(0), writec(0)
{
	char tempBuff[64];
	sprintf(tempBuff, "%llu", pLLUInteger);
	*this = tempBuff;
}

CString::~CString()
{
	if (buffer) free(buffer);
}

/*
	Data-Management
*/

bool CString::load(const CString& pString)
{
	char buff[65535];
	FILE *file = 0;

#if defined(WIN32) || defined(WIN64)
	wchar_t* wcstr = 0;

	// Determine if the filename is UTF-8 encoded.
	int wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pString.text(), pString.length(), 0, 0);
	if (wcsize != 0)
	{
		wcstr = new wchar_t[wcsize + 1];
		memset((void *)wcstr, 0, (wcsize + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, pString.text(), pString.length(), wcstr, wcsize);
	}
	else
	{
		wcstr = new wchar_t[pString.length() + 1];
		for (int i = 0; i < pString.length(); ++i)
			wcstr[i] = (unsigned char)pString[i];
		wcstr[pString.length()] = 0;
	}

	// Open the file now.
	file = _wfopen(wcstr, L"rb");
	delete[] wcstr;
	if (file == 0)
		return false;
#else
	// Linux uses UTF-8 filenames.
	if ((file = fopen(pString.text(), "rb")) == 0)
		return false;
#endif

	int size = 0;
	clear();
	while ((size = (int)fread(buff, 1, sizeof(buff), file)) > 0)
		write(buff, size);
	fclose(file);
	return true;
}

bool CString::save(const CString& pString) const
{
	FILE *file = 0;

#if defined(WIN32) || defined(WIN64)
	wchar_t* wcstr = 0;

	// Determine if the filename is UTF-8 encoded.
	int wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pString.text(), pString.length(), 0, 0);
	if (wcsize != 0)
	{
		wcstr = new wchar_t[wcsize + 1];
		memset((void *)wcstr, 0, (wcsize + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, pString.text(), pString.length(), wcstr, wcsize);
	}
	else
	{
		wcstr = new wchar_t[pString.length() + 1];
		for (int i = 0; i < pString.length(); ++i)
			wcstr[i] = (unsigned char)pString[i];
		wcstr[pString.length()] = 0;
	}

	// Open the file now.
	file = _wfopen(wcstr, L"wb");
	delete[] wcstr;
	if (file == 0)
		return false;
#else
	// Linux uses UTF-8 filenames.
	if ((file = fopen(pString.text(), "wb")) == 0)
		return false;
#endif

	fwrite(buffer, 1, sizec, file);
	fflush(file);
	fclose(file);
	return true;
}

CString CString::readChars(int pLength)
{
	CString retVal;
	pLength = clip(pLength, 0, sizec - readc);
	retVal.write(&buffer[readc], pLength);
	readc += pLength;
	return retVal;
}

CString CString::readString(const CString& pString)
{
	CString retVal;
	if (readc > sizec) return retVal;

	int len;
	len = (pString.isEmpty() ? -1 : find(pString, readc) - readc);
	len = (len < 0 ? bytesLeft() : len);
	len = (len > bytesLeft() ? bytesLeft() : len); //
	retVal.write(&buffer[readc], len);

	// If len was set to bytesLeft(), it MIGHT have pString.
	int len2;
	if ((len2 = retVal.find(pString)) != -1)
		retVal.removeI(len2, pString.length());

	readc += len + pString.length();
	return retVal;
}

int CString::read(char *pDest, int pSize)
{
	int length = (sizec - readc < pSize ? sizec - readc : pSize);
	if (length <= 0)
	{
		memset((void*)pDest, 0, pSize);
		return 0;
	}
	memcpy((void*)pDest, (void*)&buffer[readc], length);
	readc += length;
	return length;
}

int CString::write(const char *pSrc, int pSize)
{
	if (!pSize)
		return 0;
	if (buffer == 0)
		clear(pSize);

	if (writec + pSize >= buffc)
	{
		buffc = writec + pSize + 10 + (sizec / 3);
		buffer = (char*)realloc(buffer, buffc);
	}

	memcpy(&buffer[writec], pSrc, pSize);
	writec += pSize;
	buffer[writec] = 0;
	sizec = (writec > sizec ? writec : sizec);
	//buffer[sizec] = 0;
	return pSize;
}

int CString::write(const CString& pString)
{
	return write(pString.text(), pString.length());
}

void CString::clear(int pSize)
{
	if (buffer) free(buffer);
	sizec = readc = writec = 0;
	buffc = (pSize > 0 ? pSize : 1) + 1;
	buffer = (char*)malloc(buffc);
	buffer[0] = 0;
}

/*
	Functions
*/

CString CString::escape() const
{
	CString retVal;

	for (int i = 0; i < length(); i++)
	{
		if (buffer[i] == '\\')
			retVal << "\\\\";
		else if (buffer[i] == '\"')
			retVal << "\"\"";
		else if (buffer[i] == '\'')
			retVal << "\'\'";
		else
			retVal << buffer[i];
	}

	return retVal;
}

CString CString::unescape() const
{
	CString retVal;

	for (int i = 0; i < length() - 1; i++)
	{
		char cur = buffer[i];
		char nex = buffer[++i];
		
		if (cur == '\\' && nex == '\\')
			retVal << "\\";
		else if (cur == '\"' && nex == '\"')
			retVal << "\"";
		else if (cur == '\'' && nex == '\'')
			retVal << "\'";
		else
			retVal << buffer[--i];
	}

	return retVal;
}

CString CString::left(int pLength) const
{
	return subString(0, pLength);
}

CString CString::right(int pLength) const
{
	return subString(length() - pLength, pLength);
}

CString CString::remove(int pStart, int pLength) const
{
	CString retVal(*this);
	if (pLength == 0 || pStart < 0)
		return retVal;
	if (pStart >= sizec)
		return retVal;
	if (pLength == -1)
		pLength = retVal.length();

	pLength = clip(pLength, 0, retVal.length()-pStart);
	memmove(retVal.text() + pStart, retVal.text() + pStart + pLength, retVal.length() - pStart - (pLength - 1));
	retVal.setSize(retVal.length() - pLength);
	retVal[retVal.length()] = 0;
	return retVal;
}

CString CString::removeAll(const CString& pString) const
{
	// Sanity checks.
	if (pString.length() == 0 || length() == 0) return *this;

	// First check and see if we even have the string to remove.
	// pLoc will be initially set here.
	int pLoc = find(pString);
	if (pLoc == -1) return *this;

	// Resize retVal to the current length of the class.
	// This will prevent unnecessary realloc() calls.
	CString retVal;
	retVal.clear(sizec);

	// Construct retVal.
	const int pLen = pString.length();
	int pStart = 0;
	do
	{
		int pRead = pLoc - pStart;
		retVal << subString(pStart, pRead);
		pStart += (pRead + pLen);
	}
	while ((pLoc = find(pString, pStart)) != -1);
	retVal << subString(pStart);

	// Done!
	return retVal;
}

CString CString::subString(int pStart, int pLength) const
{
	// Read Entire String?
	if (pLength == -1)
		pLength = length();

	if (pStart >= length())
		return CString();

	CString retVal;
	pStart = clip(pStart, 0, length());
	pLength = clip(pLength, 0, length()-pStart);

	if (pLength > 0)
		retVal.write(&buffer[pStart], pLength);
	return retVal;
}

CString CString::toLower() const
{
	CString retVal(*this);
	for (int i = 0; i < retVal.length(); i++)
	{
		if (inrange(retVal[i], 'A', 'Z'))
			retVal[i] += 32;
	}

	return retVal;
}

CString CString::toUpper() const
{
	CString retVal(*this);
	for (int i = 0; i < retVal.length(); i++)
	{
		if (inrange(retVal[i], 'a', 'z'))
			retVal[i] -= 32;
	}

	return retVal;
}

CString CString::trim() const
{
	return trimLeft().trimRight();
}

CString CString::trimLeft() const
{
	for (int i = 0; i < length(); ++i)
	{
		if ((unsigned char)buffer[i] > (unsigned char)' ')
			return subString(i, length() - i);
	}

	return CString();
	//return CString(*this);
}

CString CString::trimRight() const
{
	for (int i = length() - 1; i >= 0; --i)
	{
		if ((unsigned char)buffer[i] > (unsigned char)' ')
			return subString(0, i+1);
	}

	return CString(*this);
}

CString CString::bzcompress(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned int clen = buffSize;

	if ((error = BZ2_bzBuffToBuffCompress(buf, &clen, buffer, length(), 1, 0, 30)) != BZ_OK)
	{
		delete [] buf;
		return retVal;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

CString CString::bzuncompress(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned int clen = buffSize;

	if ((error = BZ2_bzBuffToBuffDecompress(buf, &clen, buffer, length(), 0, 0)) != BZ_OK)
	{
		delete [] buf;
		return retVal;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

CString CString::zcompress(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned long clen = buffSize;

	if ((error = compress((Bytef *)buf, (uLongf *)&clen, (const Bytef *)buffer, length())) != Z_OK)
	{
		delete [] buf;
		return retVal;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

CString CString::zuncompress(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned long clen = buffSize;

	if ((error = uncompress((Bytef *)buf, (uLongf *)&clen, (const Bytef *)buffer, length())) != Z_OK)
	{
		switch (error)
		{
			case Z_MEM_ERROR:
				printf("[zlib] Not enough memory to decompress.\n");
				break;
			case Z_BUF_ERROR:
				printf("[zlib] Not enough room in the output buffer to decompress.\n");
				break;
			case Z_DATA_ERROR:
				printf("[zlib] The input data was corrupted.\n");
				break;
		}
		delete [] buf;
		return retVal;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

int CString::find(const CString& pString, int pStart) const
{
	const char* obuffer = pString.text();
	const int olen = pString.length();
	for (int i = pStart; i <= sizec - olen; ++i)
	{
		if (buffer[i] == 0)
		{
			if (olen == 0) return i;
			else continue;
		}
		if (memcmp(buffer + i, obuffer, olen) == 0)
			return i;
	}
	return -1;
}

int CString::findi(const CString& pString, int pStart) const
{
	for (int i = pStart; i <= length() - pString.length(); ++i)
	{
		if (strncasecmp(&buffer[i], pString.text(), pString.length()) == 0)
			return i;
	}
	return -1;
}

int CString::findl(char pChar) const
{
	char* loc = strrchr(buffer, (int)pChar);
	if (loc == 0) return -1;
	return (int)(loc - buffer);
}

std::vector<CString> CString::tokenize(const CString& pString) const
{
	CString retVal(*this);
	std::vector<CString> strList;
	char *tok = strtok(retVal.text(), pString.text());

	while (tok != 0)
	{
		strList.push_back(tok);
		tok = strtok(0, pString.text());
	}

	return strList;
}

std::vector<CString> CString::tokenizeConsole() const
{
	std::vector<CString> strList;
	bool quotes = false;
	CString str;
	for (int i = 0; i < sizec; ++i)
	{
		switch (buffer[i])
		{
			case ' ':
				if (!quotes)
				{
					strList.push_back(str);
					str.clear(30);
				}
				else str.write(buffer + i, 1);
				break;
			case '\"':
				quotes = !quotes;
				break;
			case '\\':
				if (i + 1 < sizec)
				{
					switch (buffer[i + 1])
					{
						case '\"':
							str.write("\"", 1);
							++i;
							break;
						case '\\':
							str.write("\\", 1);
							++i;
							break;
						default:
							str.write(buffer + i, 1);
							break;
					}
				}
				else str.write(buffer + i, 1);
				break;
			default:
				str.write(buffer + i, 1);
				break;
		}
	}
	strList.push_back(str);
	return strList;
}

std::vector<CString> CString::loadToken(const CString& pFile, const CString& pToken, bool removeCR)
{
	CString fileData;
	if (!fileData.load(pFile))
		return std::vector<CString>();

	if (removeCR) fileData.removeAllI("\r");

	// parse file
	std::vector<CString> result;
	while (fileData.bytesLeft())
		result.push_back(fileData.readString(pToken));
	
	// return
	return result;
}

CString CString::replaceAll(const CString& pString, const CString& pNewString) const
{
	CString retVal(*this);
	int pos = 0;
	int len = pString.length();
	int len2 = pNewString.length();

	while (true)
	{
		pos = retVal.find(pString, pos);
		if (pos == -1) break;

		// Remove the string.
		retVal.removeI(pos, len);

		// Add the new string where the removed data used to be.
		retVal = CString() << retVal.subString(0, pos) << pNewString << retVal.subString(pos);

		pos += len2;
	}
	return retVal;
}

CString CString::gtokenize() const
{
	CString self(*this);
	CString retVal;
	int pos[] = {0, 0};

	// Add a trailing \n to the line if one doesn't already exist.
	if (buffer[sizec - 1] != '\n') self.writeChar('\n');

	// Do the tokenization stuff.
	while ((pos[0] = self.find("\n", pos[1])) != -1)
	{
		CString temp(self.subString(pos[1], pos[0] - pos[1]));
		temp.removeAllI("\r");
		if (!temp.isEmpty())
		{
			// Check for a complex word.
			bool complex = false;
			if (temp[0] == '"') complex = true;
			for (int i = 0; i < temp.length() && !complex; ++i)
			{
				if (temp[i] < 33 || temp[i] > 126 || temp[i] == 44)
					complex = true;
			}

			// Put complex words inside quotation marks.
			if (complex)
			{
				temp.replaceAllI( "\"", "\"\"" );	// Change all " to ""
				retVal << "\"" << temp << "\",";
			}
			else retVal << temp << ",";
		}
		else retVal << ",";
		pos[1] = pos[0] + 1;
	}

	// Kill the trailing comma and return our new string.
	retVal.removeI(retVal.length() - 1, 1);
	return retVal;
}

CString CString::guntokenize() const
{
	CString retVal;
	retVal.clear(length() + 5);
	bool is_paren = false;

	// Check to see if we are starting with a quotation mark.
	int i = 0;
	if (buffer[0] == '"')
	{
		is_paren = true;
		++i;
	}

	// Untokenize.
	for (; i < length(); ++i)
	{
		// If we encounter a comma not inside a quoted string, we are encountering
		// a new index.  Replace the comma with a newline.
		if (buffer[i] == ',' && !is_paren)
		{
			retVal << "\n";
			
			// Check to see if the next string is quoted.
			if (i + 1 < length() && buffer[i + 1] == '"')
			{
				is_paren = true;
				++i;
			}
		}
		// We need to handle quotation marks as they have different behavior in quoted strings.
		else if (buffer[i] == '"')
		{
			// If we are encountering a quotation mark in a quoted string, we are either
			// ending the quoted string or escaping a quotation mark.
			if (is_paren)
			{
				if (i + 1 < length())
				{
					// Escaping a quotation mark.
					if (buffer[i + 1] == '"')
					{
						retVal << "\"";
						++i;
					}
					// Ending the quoted string.
					else if (buffer[i + 1] == ',')
						is_paren = false;
				}
			}
			// A quotation mark in a non-quoted string.
			else retVal << buffer[i];
		}
		// Anything else gets put to the output.
		else retVal << buffer[i];
	}

	return retVal;
}

bool CString::match(const CString& pMask) const
{
	char stopstring[1];
	*stopstring = 0;
	const char* matchstring = buffer;
	const char* wildstring = pMask.text();

	while (*matchstring)
	{
		if (*wildstring == '*')
		{
			if (!*++wildstring)
				return true;
			else *stopstring = *wildstring;
		}

		if (*stopstring)
		{
			if (*stopstring == *matchstring)
			{
				wildstring++;
				matchstring++;
				*stopstring = 0;
			}
			else matchstring++;
		}
		else if ((*wildstring == *matchstring) || (*wildstring == '?'))
		{
			wildstring++;
			matchstring++;
		}
		else return false;

		if (!*matchstring && *wildstring && *wildstring != '*')
		{
			// matchstring too short
			return false;
		}
	}
	return true;
}

bool CString::comparei(const CString& pOther) const
{
	if (strncasecmp(buffer, pOther.text(), MAX(sizec, pOther.length())) == 0)
		return true;
	return false;
}

bool CString::contains(const CString& characters) const
{
	for (int i = 0; i < sizec; ++i)
	{
		for (int j = 0; j < characters.length(); ++j)
		{
			if (buffer[i] == characters[j])
				return true;
		}
	}
	return false;
}

bool CString::onlyContains(const CString& characters) const
{
	for (int i = 0; i < sizec; ++i)
	{
		bool test = false;
		for (int j = 0; j < characters.length(); ++j)
		{
			if (buffer[i] == characters[j])
			{
				test = true;
				j = characters.length();
			}
		}
		if (test == false)
			return false;
	}
	return true;
}

bool CString::isNumber() const
{
	const char numbers[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.' };
	char periodCount = 0;
	for (int i = 0; i < sizec; ++i)
	{
		bool isNum = false;
		for (int j = 0; j < 11; ++j)
		{
			if (buffer[i] == numbers[j])
			{
				if (j == 10) periodCount++;
				isNum = true;
				j = 11;
			}
		}
		if (isNum == false || periodCount > 1)
			return false;
	}
	return true;
}

bool CString::isAlphaNumeric() const
{
	return onlyContains("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}


/*
	Operators
*/
CString& CString::operator=(const CString& pString)
{
	if (this == &pString)
		return *this;

	if (pString.length() != 0)
	{
		clear(pString.length() + 10);
		write((char*)pString.text(), pString.length());
	}
	else clear(30);
	return *this;
}

CString& CString::operator<<(const CString& pString)
{
	if (this == &pString)
		return *this << CString(pString);

	write(pString.text(), pString.length());
	return *this;
}

CString& CString::operator+=(const CString& pString)
{
	return *this << pString;
}

CString CString::operator+(const CString& pString)
{
	return CString(*this) << pString;
}

//CString::operator const char*() const
//{
//	return buffer;
//}

char& CString::operator[](int pIndex)
{
	return buffer[pIndex];
}

char CString::operator[](int pIndex) const
{
	return buffer[pIndex];
}

bool operator==(const CString& pString1, const CString& pString2)
{
	if (pString1.length() == pString2.length())
	{
		if (memcmp(pString1.text(), pString2.text(), pString1.length()) == 0)
			return true;
	}

	return false;
}

bool operator!=(const CString& pString1, const CString& pString2)
{
	return !(pString1 == pString2);
}

bool operator<(const CString& pString1, const CString& pString2)
{
	if (strcmp(pString1.text(), pString2.text()) < 0) return true;
	return false;
	/*
	int len = (pString1.length() > pString2.length() ? pString2.length() : pString1.length());
	return memcmp(pString1.text(), pString2.text(), len) < 0;
	*/
}

bool operator>(const CString& pString1, const CString& pString2)
{
	int len = (pString1.length() > pString2.length() ? pString2.length() : pString1.length());
	return memcmp(pString1.text(), pString2.text(), len) > 0;
}

bool operator<=(const CString& pString1, const CString& pString2)
{
	int len = (pString1.length() > pString2.length() ? pString2.length() : pString1.length());
	return memcmp(pString1.text(), pString2.text(), len) <= 0;
}

bool operator>=(const CString& pString1, const CString& pString2)
{
	int len = (pString1.length() > pString2.length() ? pString2.length() : pString1.length());
	return memcmp(pString1.text(), pString2.text(), len) >= 0;
}

CString operator+(const CString& pString1, const CString& pString2)
{
	return CString(pString1) << pString2;
}

/*
	Additional Functions for Data-Packing
*/
CString& CString::writeChar(const char pData)
{
	write((char*)&pData, 1);
	return *this;
}

CString& CString::writeShort(const short pData)
{
	char val[2];
	val[0] = ((pData >> 8) & 0xFF);
	val[1] = (pData & 0xFF);
	write((char*)&val, 2);
	return *this;
}

CString& CString::writeInt(const int pData)
{
	char val[4];
	val[0] = ((pData >> 24) & 0xFF);
	val[1] = ((pData >> 16) & 0xFF);
	val[2] = ((pData >> 8) & 0xFF);
	val[3] = (pData & 0xFF);
	write((char *)&val, 4);
	return *this;
}

char CString::readChar()
{
	char val;
	read(&val, 1);
	return val;
}

short CString::readShort()
{
	unsigned char val[2];
	read((char*)val, 2);
	return (val[0] << 8) + val[1];
}

int CString::readInt()
{
	unsigned char val[4];
	read((char*)val, 4);
	return (val[0] << 24) + (val[1] << 16) + (val[2] << 8) + val[3];
}

/*
	Additional Functions for Data-Packing (GRAAL)
*/
CString& CString::writeGChar(const unsigned char pData)
{
	unsigned char val = (unsigned char)(pData < 223 ? pData : 223) + 32;
	write((char*)&val, 1);
	return *this;
}

CString& CString::writeGShort(const unsigned short pData)
{
	unsigned short t = pData;
	if (t > 28767) t = 28767;

	unsigned char val[2];
	val[0] = t >> 7;
	if (val[0] > 223) val[0] = 223;
	val[1] = t - (val[0] << 7);

	val[0] += 32;
	val[1] += 32;
	write((char*)&val, 2);

	return *this;
}

CString& CString::writeGInt(const unsigned int pData)
{
	unsigned int t = pData;
	if (t > 3682399) t = 3682399;

	unsigned char val[3];
	val[0] = t >> 14;
	if (val[0] > 223) val[0] = 223;
	t -= val[0] << 14;
	val[1] = t >> 7;
	if (val[1] > 223) val[1] = 223;
	val[2] = t - (val[1] << 7);

	for (int a = 0;a < 3;++a) val[a] += 32;
	write((char *)&val,3);

	return *this;
}

CString& CString::writeGInt4(const unsigned int pData)
{
	unsigned int t = pData;
	if (t > 471347295) t = 471347295;

	unsigned char val[4];
	val[0] = t >> 21;
	if (val[0] > 223) val[0] = 223;
	t -= val[0] << 21;
	val[1] = t >> 14;
	if (val[1] > 223) val[1] = 223;
	t -= val[1] << 14;
	val[2] = t >> 7;
	if (val[2] > 223) val[2] = 223;
	val[3] = t - (val[2] << 7);

	for (int a = 0;a < 4;++a) val[a] += 32;
	write((char *)&val,4);

	return *this;
}

CString& CString::writeGInt5(const unsigned long long pData)
{
	unsigned long long t = pData;
	if (t > 0xFFFFFFFF) t = 0xFFFFFFFF;

	unsigned char val[5];
	val[0] = t >> 28;
	if (val[0] > 15) val[0] = 15; //This is capped low because higher just results in values over 0xFFFFFFFF.
	t -= val[0] << 28;
	val[1] = t >> 21;
	if (val[1] > 223) val[1] = 223;
	t -= val[1] << 21;
	val[2] = t >> 14;
	if (val[2] > 223) val[2] = 223;
	t -= val[2] << 14;
	val[3] = t >> 7;
	if (val[3] > 223) val[3] = 223;
	val[4] = t - (val[3] << 7);

	for (int a = 1;a < 5;++a) val[a] += 32; //For whatever reason, the client doesn't subtract 32 from the MSB here.
	write((char *)&val,5);

	return *this;
}

// max: 0xDF 223
char CString::readGChar()
{
	char val;
	read(&val, 1);
	return val - 32;
}

// max: 0x705F 28767
short CString::readGShort()
{
	unsigned char val[2];
	read((char*)val, 2);
	return (val[0] << 7) + val[1] - 0x1020;
}

// max: 0x38305F 3682399
int CString::readGInt()
{
	unsigned char val[3];
	read((char*)val, 3);
	return ((val[0] << 7) + val[1] << 7) + val[2] - 0x81020;
}

// max: 0x1C18305F 471347295
int CString::readGInt4()
{
	unsigned char val[4];
	read((char*)val, 4);
	return (((val[0] << 7) + val[1] << 7) + val[2] << 7) + val[3] - 0x4081020;
}

// max: 0xFFFFFFFF 4294967295
//The client doesn't store these in multiple registers, so the maximum value is capped.
unsigned int CString::readGInt5()
{
	unsigned char val[5];
	read((char*)val, 5);
	return ((((val[0] << 7) + val[1] << 7) + val[2] << 7) + val[3] << 7) + val[4] - 0x4081020;
}

/*
	Friend Functions
*/
CString getExtension(const CString& pStr)
{
	int pos = pStr.findl('.');
	if (pos >= 0)
		return pStr.subString(pos);
	return CString();
}
