#include "IDebug.h"
#include "CString.h"

#ifdef _WIN32
	#define strncasecmp _strnicmp
	#define snprintf _snprintf
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
	if ((file = fopen(pString.text(), "rb")) == 0)
		return false;

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
	if ((file = fopen(pString.text(), "wb")) == 0)
		return false;
	fwrite(buffer, 1, sizec, file);
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
	sizec = (writec > sizec ? writec : sizec);
	buffer[sizec] = 0;
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
		temp.replaceAllI( "\"", "\"\"" );	// Change all " to ""
		temp.removeAllI("\r");
		if (temp.length() != 0)
			retVal << "\"" << temp << "\",";
		else
			retVal << ",";
		pos[1] = pos[0] + 1;
	}

	// Kill the trailing comma and return our new string.
	retVal.removeI(retVal.length() - 1, 1);
	return retVal;
}

CString CString::guntokenize() const
{
	CString retVal;
	std::vector<CString> temp;
	int pos[] = {0, 1};

	// Copy the buffer data to a working copy and trim it.
	CString nData(*this);
	nData.trimI();

	// Check to see if it starts with a quotation mark.  If not, set pos[1] to 0.
	if (nData[0] != '\"') pos[1] = 0;

	// Untokenize.
	while ((pos[0] = nData.find(",", pos[1])) != -1)
	{
		// Empty blocks are blank lines.
		if (pos[0] == pos[1])
		{
			pos[1]++;
			temp.push_back(CString("\r"));	// Workaround strtok() limitation.
			continue;
		}

		// ,"", blank lines.
		if (pos[0] - pos[1] == 1 && nData[pos[1]] == '\"')
		{
			pos[1] += 2;
			temp.push_back(CString("\r"));
			continue;
		}

		// Check for ,,"""blah"
		if (nData[pos[1]] == '\"' && nData[pos[1]+1] != '\"')
		{
			// Check to make sure it isn't ,"",
			if (!(pos[1] + 2 < nData.length() && nData[pos[1]+2] == ','))
				pos[1]++;
		}

		// Check and see if the comma is outside or inside of the thing string.
		// If pos[1] points to a quotation mark we have to find the closing quotation mark.
		if (pos[1] > 0 && nData[pos[1] - 1] == '\"')
		{
			while (true)
			{
				if ( pos[0] == -1 ) break;
				if ((nData[pos[0]-1] != '\"') ||
					(nData[pos[0]-1] == '\"' && nData[pos[0]-2] == '\"') )
					pos[0] = nData.find( ",", pos[0] + 1 );
				else
					break;
			}
		}

		// Exit out if we previously failed to find the end.
		if (pos[0] == -1) break;

		// "test",test
		CString t2;
		if (pos[0] > 0 && nData[pos[0] - 1] == '\"')
			t2 = nData.subString(pos[1], pos[0] - pos[1] - 1);
		else
			t2 = nData.subString(pos[1], pos[0] - pos[1]);

		// Check if the string is valid and if it is, copy it.
		t2.replaceAllI( "\"\"", "\"" );
		t2.removeAllI("\n");
		t2.removeAllI("\r");

		// Add it.
		temp.push_back(t2);

		// Move forward the correct number of spaces.
		if (pos[0] + 1 != nData.length() && nData[pos[0] + 1] == '\"')
			pos[1] = pos[0] + (int)strlen(",\"");	// test,"test
		else
			pos[1] = pos[0] + (int)strlen(",");		// test,test
	}

	// Try and grab the very last element.
	if (pos[1] < nData.length())
	{
		// If the end is a quotation mark, remove it.
		if (nData[nData.length() - 1] == '\"')
			nData.removeI(nData.length() - 1, 1);

		// Sanity check.
		if (pos[1] != nData.length())
		{
			CString buf(nData.subString(pos[1]));
			buf.replaceAllI("\"\"", "\"");	// Replace "" with "
			buf.removeAllI("\n");
			buf.removeAllI("\r");
			temp.push_back(buf);
		}
	}

	// Write the correct string out.
	for ( unsigned int i = 0; i < temp.size(); ++i )
		retVal << temp[i] << "\n";

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

bool CString::isNumber()
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
CString& CString::writeGChar(const char pData)
{
	char val = pData+32;
	write((char*)&val, 1);
	return *this;
}

CString& CString::writeGShort(const short pData)
{
	char val[2];
	val[0] = ((pData >> 7) & 0x7F)+32;
	val[1] = (pData & 0x7F)+32;
	write((char*)&val, 2);
	return *this;
}

CString& CString::writeGInt(const int pData)
{
	char val[3];
	val[0] = ((pData >> 14) & 0x7F)+32;
	val[1] = ((pData >> 7) & 0x7F)+32;
	val[2] = (pData & 0x7F)+32;
	write((char *)&val, 3);
	return *this;
}

CString& CString::writeGInt4(const int pData)
{
	char val[4];
	val[0] = ((pData >> 21) & 0x7F)+32;
	val[1] = ((pData >> 14) & 0x7F)+32;
	val[2] = ((pData >> 7) & 0x7F)+32;
	val[3] = (pData & 0x7F)+32;
	write((char *)&val, 4);
	return *this;
}

CString& CString::writeGInt5(const long long pData)
{
	char val[5];
	val[0] = ((pData >> 28) & 0x7F)+32;
	val[1] = ((pData >> 21) & 0x7F)+32;
	val[2] = ((pData >> 14) & 0x7F)+32;
	val[3] = ((pData >> 7) & 0x7F)+32;
	val[4] = (pData & 0x7F)+32;
	write((char *)&val, 5);
	return *this;
}

char CString::readGChar()
{
	char val;
	read(&val, 1);
	return val-32;
}

short CString::readGShort()
{
	unsigned char val[2];
	read((char*)val, 2);
	return ((val[0]-32) << 7) + val[1]-32;
}

int CString::readGInt()
{
	unsigned char val[3];
	read((char*)val, 3);
	return ((val[0]-32) << 14) + ((val[1]-32) << 7) + val[2]-32;
}

int CString::readGInt4()
{
	unsigned char val[4];
	read((char*)val, 4);
	return ((val[0]-32) << 21) + ((val[1]-32) << 14) + ((val[2]-32) << 7) + val[3]-32;
}

int CString::readGInt5()
{
	unsigned char val[5];
	read((char*)val, 5);
	return ((val[0]-32) << 28) + ((val[1]-32) << 21) + ((val[2]-32) << 14) + ((val[3]-32) << 7) + val[4]-32;
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
