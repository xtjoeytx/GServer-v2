#include "IUtil.h"
#include "CString.h"

static const char* colors[] =
{
	"white",
	"yellow",
	"orange",
	"pink",
	"red",
	"darkred",
	"lightgreen",
	"green",
	"darkgreen",
	"lightblue",
	"blue",
	"darkblue",
	"brown",
	"cynober",
	"purple",
	"darkpurple",
	"lightgray",
	"gray",
	"black",
	"transparent",
	0
};

static const char* const clientVersions[] =
{
	"SERV009",		// CLVER_1_25,
	"SERV011",		// CLVER_1_27,
	"SERV013",		// CLVER_1_28,
	"SERV016",		// CLVER_1_31,
	"SERV018",		// CLVER_1_32,
	"",		// CLVER_1_322,
	"",		// CLVER_1_323,
	"",		// CLVER_1_324,
	"NW20020",		// CLVER_1_33,
	"",		// CLVER_1_331,
	"NW22030",		// CLVER_1_34,
	"",		// CLVER_1_341,
	"NW13040",		// CLVER_1_35,
	"NW10050",		// CLVER_1_36,
	"NW21050",		// CLVER_1_37,
	"",		// CLVER_1_371,
	"NW22060",		// CLVER_1_38,
	"",		// CLVER_1_381,
	"NW07080",		// CLVER_1_39,
	"",		// CLVER_1_391,
	"",		// CLVER_1_392,
	"",		// CLVER_1_4,
	"",		// CLVER_1_41,
	"GNW13110",		// CLVER_1_411,
	"GNW31101",		// CLVER_2_1,
	"GNW01012",		// CLVER_2_12,
	"GNW23012",		// CLVER_2_13,
	"GNW30042",		// CLVER_2_14,
	"GNW19052",		// CLVER_2_15,
	"GNW20052",		// CLVER_2_151,
	"GNW12102",		// CLVER_2_16,
	"GNW22122",		// CLVER_2_17,
	"GNW21033",		// CLVER_2_18,
	"GNW15053",		// CLVER_2_19,
	"GNW28063",		// CLVER_2_2,
	"GNW01113",		// CLVER_2_21,
	"GNW03014",		// CLVER_2_22,
	"GNW14015",		// CLVER_2_3,
	"GNW28015",		// CLVER_2_31,

	"G3D16053",		// CLVER_3,
	"G3D27063",		// CLVER_3_01,
	"G3D03014",		// CLVER_3_041,

	"G3D28095",		// CLVER_4_0211,
	"G3D09125",		// CLVER_4_034,
	"G3D17026",		// CLVER_4_042,
	"G3D26076",		// CLVER_4_110,
	"G3D20126",		// CLVER_4_208,

	"G3D22067",		// CLVER_5_07,
	"G3D14097",		// CLVER_5_12,
	0
};

static const char* const rcVersions[] =
{
	"GSERV023",		// RCVER_1_010,
	"GSERV024",		// RCVER_1_1,
	"GSERV025",		// RCVER_2,
	0
};

// ncVersions
// NCL11012

int getVersionID(const CString& version)
{
	int i = 0;
	while (clientVersions[i] != 0)
	{
		if (version == CString(clientVersions[i]))
			return i;
		++i;
	}

	i = 0;
	while (rcVersions[i] != 0)
	{
		if (version == CString(rcVersions[i]))
			return i;
		++i;
	}
	return -1;
}

char getColor(const CString& color)
{
	int i = 0;
	while (colors[i] != 0)
	{
		if (color.comparei(CString(colors[i])))
			return i;
		++i;
	}
	return -1;
}

CString removeComments(const CString& code, const CString& newLine)
{
	CString ret(code);
	int pos = -1;

	// Remove // comments.
	while ((pos = ret.find("//", pos + 1)) != -1)
	{
		//check for urls (http://)
		if (pos > 0 && ret[pos - 1] == ':')
			continue;

		// check for //#CLIENTSIDE
		if (ret.subString(pos, 13) == "//#CLIENTSIDE")
			continue;

		int len = ret.find(newLine, pos) - pos;
		if (len < 0) len = -1;
		ret.removeI(pos, len);
	}

	// Remove /* ... */ comments.
	while ((pos = ret.find("/*", pos + 1)) != -1)
	{
		int len = ret.find("*/", pos) - pos;
		if (len < 0) len = -1;
		ret.removeI(pos, len + 2);
	}

	return ret;
}

CString removeExtension(const CString& ext)
{
	int ePos = ext.findl('.');
	if (ePos == -1) return ext;

	return ext.subString(0, ePos);
}
