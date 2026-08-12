#include <string_view>

#include <CString.h>

#include <BabyDI.h>
#include <level/LevelSign.h>
#include <object/Player.h>
#include <utilities/Extents.h>
#include <utilities/StringUtils.h>
#include <utilities/manager/ITranslationManager.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

static CString encodeSignCode(CString& pText);
static CString encodeSign(const CString& pSignText);
static CString decodeSignCode(CString pText);

const CString signCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
							   "0123456789!?-.,#>()#####\"####':/~&### <####;\n";
const CString signSymbols = "ABXYudlrhxyz#4.";
const int ctablen[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 2, 1};
const int ctabindex[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 15, 17};
const int ctab[] = {91, 92, 93, 94, 77, 78, 79, 80, 74, 75, 71, 72, 73, 86, 86, 87, 88, 67};

CString encodeSignCode(CString& pText)
{
	CString retVal;
	const int txtLen = pText.length();
	for (int i = 0; i < txtLen; i++)
	{
		char letter = pText[i];
		if (letter == '#')
		{
			i++;
			if (i < txtLen)
			{
				letter = pText[i];
				if (const int code = signSymbols.find(letter); code != -1)
				{
					for (int ii = 0; ii < ctablen[code]; ii++)
						retVal.writeGChar(static_cast<char>(ctab[ctabindex[code] + ii]));
					continue;
				}

				letter = pText[--i];
			}
		}

		int code = signCharacters.find(letter);
		if (letter == '#') code = 86;
		if (code != -1)
			retVal.writeGChar(static_cast<char>(code));
		else
		{
			if (letter != '\r')
			{
				// Write the character code directly into the sign.
				retVal >> (char)86 >> (char)10 >> (char)69; // #K(
				CString scode(static_cast<int>(letter));
				for (int j = 0; j < scode.length(); ++j)
				{
					const int c = signCharacters.find(scode[j]);
					if (scode != -1) retVal.writeGChar(static_cast<char>(c));
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
	const int txtLen = pText.length();
	for (int i = 0; i < txtLen; i++)
	{
		const unsigned char letter = pText.readGUChar();
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
			retVal << signCharacters[letter];
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

LevelSign::LevelSign(const LocalWholeTilePosition& position, const std::string_view signText, const bool signTextIsEncoded)
	: position(position)
{
	setText(signText, signTextIsEncoded);
}

CString LevelSign::getSignPacket(const Player* player) const
{
	CString outText;

	// Write the x and y location to the packet.
	outText.writeGChar(position.x());
	outText.writeGChar(position.y());

	// Write the text to the packet.
	outText.write(player ? encodeSign(player->translate(text)) : encodedText);

	return outText;
}

void LevelSign::setText(const std::string_view signText, const bool signTextIsEncoded)
{
	if (signTextIsEncoded)
	{
		encodedText = signText;
		text = decodeSignCode(signText);
	}
	else
	{
		encodedText = encodeSign(signText);
		text = signText;
	}

	if (const auto translations = BabyDI::Get<ITranslationManager>(); translations != nullptr)
		translations->registerOriginalText(string::replace(text, "\n"sv, "#b"sv));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
