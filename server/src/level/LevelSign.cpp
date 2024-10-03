#include <IDebug.h>

#include "object/Player.h"
#include "level/LevelSign.h"

static CString encodeSignCode(CString& pText);
static CString encodeSign(const CString& pSignText);
static CString decodeSignCode(CString pText);

const CString signText = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
						 "0123456789!?-.,#>()#####\"####':/~&### <####;\n";
const CString signSymbols = "ABXYudlrhxyz#4.";
const int ctablen[] = { 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 2, 1 };
const int ctabindex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 15, 17 };
const int ctab[] = { 91, 92, 93, 94, 77, 78, 79, 80, 74, 75, 71, 72, 73, 86, 86, 87, 88, 67 };

CString encodeSignCode(CString& pText)
{
	CString retVal;
	int txtLen = pText.length();
	for (int i = 0; i < txtLen; i++)
	{
		char letter = pText[i];
		if (letter == '#')
		{
			i++;
			if (i < txtLen)
			{
				letter = pText[i];
				int code = signSymbols.find(letter);
				if (code != -1)
				{
					for (int ii = 0; ii < ctablen[code]; ii++)
						retVal.writeGChar((char)ctab[ctabindex[code] + ii]);
					continue;
				}
				else
					letter = pText[--i];
			}
		}

		int code = signText.find(letter);
		if (letter == '#') code = 86;
		if (code != -1)
			retVal.writeGChar((char)code);
		else
		{
			if (letter != '\r')
			{
				// Write the character code directly into the sign.
				retVal >> (char)86 >> (char)10 >> (char)69; // #K(
				CString scode((int)letter);
				for (int i = 0; i < scode.length(); ++i)
				{
					int c = signText.find(scode[i]);
					if (scode != -1) retVal.writeGChar((char)c);
				}
				retVal >> (char)70; // )
			}
		}
	}
	return retVal;
}

CString decodeSignCode(CString pText)
{
	CString retVal;
	int txtLen = pText.length();
	for (int i = 0; i < txtLen; i++)
	{
		unsigned char letter = pText.readGUChar();
		bool isCode = false;
		int codeID = -1;
		for (int j = 0; j < 16; ++j) // ctab length
		{
			if (letter == ctab[j])
			{
				codeID = j;
				isCode = true;
				break;
			}
		}

		if (isCode)
		{
			int codeIndex = -1;
			for (int j = 0; j < 14; ++j) // ctabindex
			{
				if (ctabindex[j] == codeID)
				{
					codeIndex = j;
					break;
				}
			}
			if (codeIndex != -1)
				retVal << "#" << signSymbols[codeIndex];
		}
		else
			retVal << signText[letter];
	}
	retVal.removeAllI("#K(13)");
	return retVal;
}

CString encodeSign(const CString& pSignText)
{
	CString retVal;
	CString signText(pSignText);
	while (signText.bytesLeft())
		retVal << encodeSignCode(CString() << signText.readString("\n") << "\n");
	return retVal;
}

LevelSign::LevelSign(const int pX, const int pY, const CString& pSign, bool encoded)
	: m_x(pX), m_y(pY), m_unformattedText(pSign)
{
	if (encoded)
	{
		m_text = m_unformattedText;
		m_unformattedText = decodeSignCode(m_unformattedText);
	}
	else
		m_text = encodeSign(m_unformattedText);
}

CString LevelSign::getSignStr(Player* pPlayer) const
{
	CString outText;

	// Write the x and y location to the packet.
	outText.writeGChar(m_x);
	outText.writeGChar(m_y);

	// Write the text to the packet.
	outText.write(pPlayer ? encodeSign(pPlayer->translate(m_unformattedText)) : m_text);

	return outText;
}

void LevelSign::setText(const CString& value)
{
	m_text = value;
	m_unformattedText = decodeSignCode(value);
}

void LevelSign::setUText(const CString& value)
{
	m_text = encodeSign(value);
	m_unformattedText = value;
}
