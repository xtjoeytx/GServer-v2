#include "CString.h"

#ifdef _WIN32
	#define strncasecmp _strnicmp
	#define snprintf _snprintf
#endif

/*
	Constructor ~ Deconstructor
*/

CString::CString(const char *pString)
{
	buffer = 0;

	int length = strlen(pString);
	clear(length);
	write(pString, length);
}

CString::CString(const CString& pString)
{
	buffer = 0;

	clear(pString.length());
	write(pString.text(), pString.length());
}

CString::CString(char pChar)
{
	buffer = 0;

	clear(sizeof(char));
	writeChar(pChar);
}

CString::CString(double pDouble)
{
	buffer = 0;

	char tempBuff[32];
	sprintf(tempBuff, "%f", pDouble);
	*this = tempBuff;
}

CString::CString(float pFloat)
{
	buffer = 0;

	char tempBuff[32];
	sprintf(tempBuff, "%.2f", pFloat);
	*this = tempBuff;
}

CString::CString(int pInteger)
{
	buffer = 0;

	char tempBuff[32];
	sprintf(tempBuff, "%i", pInteger);
	*this = tempBuff;
}

CString::CString(unsigned int pUInteger)
{
	buffer = 0;

	char tempBuff[32];
	sprintf(tempBuff, "%u", pUInteger);
	*this = tempBuff;
}

CString::CString(unsigned long int pLUInteger)
{
	buffer = 0;

	char tempBuff[32];
	sprintf(tempBuff, "%lu", pLUInteger);
	*this = tempBuff;
}

CString::CString(long long pLLInteger)
{
	buffer = 0;

	char tempBuff[64];
	sprintf(tempBuff, "%lld", pLLInteger);
	*this = tempBuff;
}

CString::CString(unsigned long long pLLUInteger)
{
	buffer = 0;

	char tempBuff[64];
	sprintf(tempBuff, "%llu", pLLUInteger);
	*this = tempBuff;
}

CString::~CString()
{
	free(buffer);
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
	free(buffer);
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
		if (buffer[i] > ' ')
			return subString(i, length() - i);
	}

	return CString();
	//return CString(*this);
}

CString CString::trimRight() const
{
	for (int i = length() - 1; i >= 0; --i)
	{
		if (buffer[i] > ' ')
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
		delete [] buf;
		return retVal;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

int CString::find(const CString& pString, int pStart) const
{
	for (int i = pStart; i <= sizec - pString.length(); ++i)
	{
		if (buffer[i] == 0)
		{
			if (pString.length() == 0) return i;
			else continue;
		}
		if (strncmp(buffer + i, pString.text(), pString.length()) == 0)
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

std::vector<CString> CString::loadToken(const CString& pFile, const CString& pToken, bool removeCR)
{
	CString fileData;
	if (!fileData.load(pFile))
		return std::vector<CString>();
	if (removeCR) fileData.removeAllI("\r");
	return fileData.tokenize(pToken);
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
	int sloc = 0, mloc = 0;

	// Check to see if they are equal.
	if (length() == pMask.length())
	{
		if (memcmp(buffer, pMask.text(), length()) == 0)
			return true;
	}

	// Check for a blank string.
	if (buffer[sloc] == 0)
	{
		if ((pMask.text())[mloc] != '*') return false;
		else return true;
	}

	// Check just for a single *.
	if (pMask == "*") return true;

	// Do the match now.
	while (buffer[sloc] != 0)
	{
		// ? only wildcards a single character.
		// Jump to the next character and try again.
		if ((pMask.text())[mloc] == '?')
		{
			sloc++;
			mloc++;
			continue;
		}

		// Find the next * or ?.
		int loc = pMask.find("*", mloc);
		int loc2 = pMask.find("?", mloc);
		if (loc == -1 && loc2 == -1)
		{
			// If neither * or ? was found, see if the rest of the string matches.
			if (subString(sloc) == pMask.subString(mloc)) return true;
			else
			{
				// If they don't match, let's make sure the last mask value wasn't a *.
				if ((pMask.text())[pMask.length() - 1] == '*') return true;
				return false;
			}
		}

		// Grab the string to search.
		// Only read up to the first * or ? so choose the correct one.
		if (loc == -1) loc = loc2;
		if (loc2 != -1 && loc2 < loc) loc = loc2;
		CString search = pMask.subString(mloc, loc - mloc);

		// This should only happen if the mask starts with a *.
		if (search.length() == 0)
		{
			mloc++;
			int loc3 = pMask.find("*", mloc);
			int loc4 = pMask.find("?", mloc);
			if (loc4 < loc3) loc3 = loc4;
			sloc = find(pMask.subString(mloc, loc3));
			continue;
		}

		// See if we can find the search string.  If not, we don't match.
		if ((loc2 = find(search, sloc)) == -1) return false;

		// Update our locations.
		sloc = loc2 + search.length();
		mloc = loc + 1;
	}

	// See if any non-wildcard characters are left in pMask.
	// If not, return true.
	CString search = pMask.subString(mloc);
	search.removeAllI("*");
	search.removeAllI("?");
	if (search.length() == 0) return true;
	return false;
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

	clear(pString.length());
	write((char*)pString.text(), pString.length());
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

// 2002-05-07 by Markus Ewald
CString CString::B64_Encode()
{
	static const char *EncodeTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	CString retVal;

	for (int i = 0; i < sizec; i++)
	{
		char pCode;

		pCode = (buffer[i] >> 2) & 0x3f;
		retVal.writeChar(EncodeTable[pCode]);

		pCode = (buffer[i] << 4) & 0x3f;
		if (i++ < sizec)
			pCode |= (buffer[i] >> 4) & 0x0f;
		retVal.writeChar(EncodeTable[pCode]);

		if (i < sizec)
		{
			pCode = (buffer[i] << 2) & 0x3f;
			if (i++ < sizec)
				pCode |= (buffer[i] >> 6) & 0x03;
			retVal.writeChar(EncodeTable[pCode]);
		}
		else
		{
			i++;
			retVal.writeChar('=');
		}

		if (i < sizec)
		{
			pCode = buffer[i] & 0x3f;
			retVal.writeChar(EncodeTable[pCode]);
		}
		else
		{
			retVal.writeChar('=');
		}
	}

	return retVal;
}

CString CString::B64_Decode()
{
	static const int DecodeTable[] = {
	// 0   1   2   3   4   5   6   7   8   9
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //   0 -   9
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  10 -  19
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  20 -  29
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  30 -  39
	-1, -1, -1, 62, -1, -1, -1, 63, 52, 53,  //  40 -  49
	54, 55, 56, 57, 58, 59, 60, 61, -1, -1,  //  50 -  59
	-1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  //  60 -  69
	 5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  //  70 -  79
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24,  //  80 -  89
	25, -1, -1, -1, -1, -1, -1, 26, 27, 28,  //  90 -  99
	29, 30, 31, 32, 33, 34, 35, 36, 37, 38,  // 100 - 109
	39, 40, 41, 42, 43, 44, 45, 46, 47, 48,  // 110 - 119
	49, 50, 51, -1, -1, -1, -1, -1, -1, -1,  // 120 - 129
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 130 - 139
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 140 - 149
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 150 - 159
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 160 - 169
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 170 - 179
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 180 - 189
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 190 - 199
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 200 - 209
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 210 - 219
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 220 - 229
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 230 - 239
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 240 - 249
	-1, -1, -1, -1, -1, -1				   // 250 - 256
	};

	CString retVal;

	for (int i = 0; i < sizec; i++)
	{
		unsigned char c1, c2;

		c1 = (char)DecodeTable[(unsigned char)buffer[i]];
		i++;
		c2 = (char)DecodeTable[(unsigned char)buffer[i]];
		c1 = (c1 << 2) | ((c2 >> 4) & 0x3);
		retVal.writeChar(c1);

		if (i++ < sizec)
		{
			c1 = buffer[i];
			if (c1 == '=')
				break;

			c1 = (char)DecodeTable[(unsigned char)buffer[i]];
			c2 = ((c2 << 4) & 0xf0) | ((c1 >> 2) & 0xf);
			retVal.writeChar(c2);
		}

		if (i++ < sizec)
		{
			c2 = buffer[i];
			if (c2 == '=')
				break;

			c2 = (char)DecodeTable[(unsigned char)buffer[i]];
			c1 = ((c1 << 6) & 0xc0) | c2;
			retVal.writeChar(c1);
		}
	}

	return retVal;
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
