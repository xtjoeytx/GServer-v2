#include "TLevelSign.h"

const CString base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
const CString signText = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
				"0123456789!?-.,#>()#####\"####':/~&### <####;\n";
const CString signSymbols = "ABXYudlrhxyz#4.";
const int ctablen[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1};
const int ctabindex[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 16};
const int ctab[] = {91, 92, 93, 94, 77, 78, 79, 80, 74, 75, 71, 72, 73, 86, 87, 88, 67};

static CString getSignCode(CString& pText);

CString getSignCode(CString& pText)
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
				if (code >= 0)
				{
					for (int ii = 0; ii < ctablen[code]; ii++)
						retVal.writeGChar((char)ctab[ctabindex[code] + ii]);
				}
			}
		}
		else
		{
			int code = signText.find(letter);
			if (code >= 0)
				retVal.writeGChar((char)code);
		}
	}
	return retVal;
}

CString TLevelSign::getSignStr() const
{
	std::vector<CString> signText = text.tokenize("\n");
	CString outText;

	// Write the x and y location to the packet.
	outText.writeGChar(x);
	outText.writeGChar(y);

	// Format the sign text and add to the packet.
	for (std::vector<CString>::iterator i = signText.begin(); i != signText.end(); ++i)
		outText << getSignCode(CString() << *i << "\n");

	return outText;
}
