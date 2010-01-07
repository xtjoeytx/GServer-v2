#ifndef CSTRING_H
#define CSTRING_H

#define _WINSOCKAPI_
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include "bzlib.h"
#include "zlib.h"

#ifndef MAX
	#define MAX(a, b)	((a) > (b) ? (a) : (b))
	#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif
#define clip(a, b, c) MAX(MIN((a), (c)), (b))
#define inrange(a, b, c) ((a) >= (b) && (a) <= (c))

#define strtofloat(a) atof(a.text())
#define strtoint(a) atoi(a.text())
#define strtolong(a) atol(a.text())

class CString
{
	public:
		/* Constructor ~ Deconstructor */
		CString();
		CString(const char *pString);
		CString(const CString& pString);
		CString(char pChar);
		CString(double pDouble);
		CString(float pFloat);
		CString(int pInteger);
		CString(unsigned int pUInteger);
		CString(long pLInteger);
		CString(unsigned long pLUInteger);
		CString(long long pLLInteger);
		CString(unsigned long long pLLUInteger);
		~CString();

		/* Retrieve Information */
		inline bool isEmpty() const;
		inline char * text();
		inline const char * text() const;
		inline int bytesLeft() const;
		inline int length() const;
		inline int readPos() const;
		inline int writePos() const;
		inline void setRead(int pRead);
		inline void setSize(int pSize);
		inline void setWrite(int pWrite);

		/* Data-Management */
		bool load(const CString& pString);
		bool save(const CString& pString) const;
		CString readChars(int pLength);
		CString readString(const CString& pString = " ");
		int read(char *pDest, int pSize);
		int write(const char *pSrc, int pSize);
		int write(const CString& pString);
		void clear(int pCount = 10);

		/* Functions */
		CString escape() const;
		CString unescape() const;
		CString left(int pLength) const;
		CString right(int pLength) const;
		CString remove(int pStart, int pLength = -1) const;
		CString removeAll(const CString& pString) const;
		CString subString(int pStart, int pLength = -1) const;
		CString toLower() const;
		CString toUpper() const;
		CString trim() const;
		CString trimLeft() const;
		CString trimRight() const;
		CString bzcompress(unsigned int buffSize = 65536) const;
		CString bzuncompress(unsigned int buffSize = 65536) const;
		CString zcompress(unsigned int buffSize = 65536) const;
		CString zuncompress(unsigned int buffSize = 65536) const;
		int find(const CString& pString, int pStart = 0) const;
		int findi(const CString& pString, int pStart = 0) const;
		int findl(char pChar) const;
		std::vector<CString> tokenize(const CString& pString = " ") const;
		std::vector<CString> tokenizeConsole() const;
		static std::vector<CString> loadToken(const CString& pFile, const CString& pToken = "\n", bool removeCR = false);
		CString replaceAll(const CString& pString, const CString& pNewString) const;
		CString gtokenize() const;
		CString guntokenize() const;
		bool match(const CString& pMask) const;
		bool comparei(const CString& pOther) const;
		bool isNumber() const;

		/* In-Functions */
		inline CString& escapeI();
		inline CString& unescapeI();
		inline CString& removeI(int pStart, int pLength = -1);
		inline CString& removeAllI(const CString& pString);
		inline CString& toLowerI();
		inline CString& toUpperI();
		inline CString& trimI();
		inline CString& bzcompressI(unsigned int buffSize = 65536);
		inline CString& bzuncompressI(unsigned int buffSize = 65536);
		inline CString& zcompressI(unsigned int buffSize = 65536);
		inline CString& zuncompressI(unsigned int buffSize = 65536);
		inline CString& replaceAllI(const CString& pString, const CString& pNewString);
		inline CString& gtokenizeI();
		inline CString& guntokenizeI();

		/* Operators */
		char& operator[](int pIndex);
		char operator[](int pIndex) const;
		CString& operator=(const CString& pString);
		CString& operator<<(const CString& pString);
		CString& operator+=(const CString& pString);
		CString operator+(const CString& pString);
		//operator const char*() const;

		friend bool operator==(const CString& pString1, const CString& pString2);
		friend bool operator!=(const CString& pString1, const CString& pString2);
		friend bool operator<(const CString& pString1, const CString& pString2);
		friend bool operator>(const CString& pString1, const CString& pString2);
		friend bool operator<=(const CString& pString1, const CString& pString2);
		friend bool operator>=(const CString& pString1, const CString& pString2);
		friend CString operator+(const CString& pString1, const CString& pString2);

		/* Data-Packing Functions */
		CString& operator>>(const char pData);
		CString& operator>>(const short pData);
		CString& operator>>(const int pData);
		CString& operator>>(const long long pData);
		CString& operator<<(const char pData);
		CString& operator<<(const short pData);
		CString& operator<<(const int pData);

		CString& writeChar(const char pData);
		CString& writeShort(const short pData);
		CString& writeInt(const int pData);
		char readChar();
		short readShort();
		int readInt();

		// Graal Packing <.<.
		CString& writeGChar(const char pData);
		CString& writeGShort(const short pData);
		CString& writeGInt(const int pData);
		CString& writeGInt4(const int pData);
		CString& writeGInt5(const long long pData);
		char readGChar();
		short readGShort();
		int readGInt();
		int readGInt4();
		int readGInt5();

		inline unsigned char readGUChar();
		inline unsigned short readGUShort();
		inline unsigned int readGUInt();
		inline unsigned int readGUInt4();
		inline unsigned int readGUInt5();

	protected:
		char *buffer;
		int buffc, sizec, readc, writec;
};

/*
	Inline Functions
*/

inline bool CString::isEmpty() const
{
	return (length() < 1);
}

inline CString& CString::operator>>(const char pData)
{
	return writeGChar(pData);
}

inline CString& CString::operator>>(const short pData)
{
	return writeGShort(pData);
}

inline CString& CString::operator>>(const int pData)
{
	return writeGInt(pData);
}

inline CString& CString::operator>>(const long long pData)
{
	return writeGInt5(pData);
}

inline CString& CString::operator<<(const char pData)
{
	return writeChar(pData);
}

inline CString& CString::operator<<(const short pData)
{
	return writeShort(pData);
}

inline CString& CString::operator<<(const int pData)
{
	return writeInt(pData);
}

inline char * CString::text()
{
	return buffer;
}

inline const char * CString::text() const
{
	return buffer;
}

inline int CString::bytesLeft() const
{
	return MAX(0, length()-readPos());
}

inline int CString::length() const
{
	return sizec;
}

inline int CString::readPos() const
{
	return readc;
}

inline int CString::writePos() const
{
	return writec;
}

inline void CString::setRead(int pRead)
{
	readc = clip(pRead, 0, sizec);
}

inline void CString::setSize(int pSize)
{
	sizec = MAX(0, pSize);
}

inline void CString::setWrite(int pWrite)
{
	writec = clip(pWrite, 0, sizec);
}

inline unsigned char CString::readGUChar()
{
	return (unsigned char)readGChar();
}

inline unsigned short CString::readGUShort()
{
	return (unsigned short)readGShort();
}

inline unsigned int CString::readGUInt()
{
	return (unsigned int)readGInt();
}

inline unsigned int CString::readGUInt4()
{
	return (unsigned int)readGInt4();
}

inline unsigned int CString::readGUInt5()
{
	return (unsigned int)readGInt5();
}

/*
	Inline Inside-Functions
*/
inline CString& CString::escapeI()
{
	*this = escape();
	return *this;
}

inline CString& CString::unescapeI()
{
	*this = unescape();
	return *this;
}

inline CString& CString::removeI(int pStart, int pLength)
{
	*this = remove(pStart, pLength);
	return *this;
}

inline CString& CString::removeAllI(const CString& pString)
{
	*this = removeAll(pString);
	return *this;
}

inline CString& CString::toLowerI()
{
	*this = toLower();
	return *this;
}

inline CString& CString::toUpperI()
{
	*this = toUpper();
	return *this;
}

inline CString& CString::trimI()
{
	*this = trim();
	return *this;
}

inline CString& CString::bzcompressI(unsigned int buffSize)
{
	*this = bzcompress(buffSize);
	return *this;
}

inline CString& CString::bzuncompressI(unsigned int buffSize)
{
	*this = bzuncompress(buffSize);
	return *this;
}

inline CString& CString::zcompressI(unsigned int buffSize)
{
	*this = zcompress(buffSize);
	return *this;
}

inline CString& CString::zuncompressI(unsigned int buffSize)
{
	*this = zuncompress(buffSize);
	return *this;
}

inline CString& CString::replaceAllI(const CString& pString, const CString& pNewString)
{
	*this = replaceAll(pString, pNewString);
	return *this;
}

inline CString& CString::gtokenizeI()
{
	*this = gtokenize();
	return *this;
}

inline CString& CString::guntokenizeI()
{
	*this = guntokenize();
	return *this;
}

/*
	Friend Functions
*/
CString getExtension(const CString& pStr);

#endif // CSTRING_H
