#include "IDebug.h"
#include "IEnums.h"
#include "IUtil.h"

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
	"",				// CLVER_UNKNOWN,
	"",				// CLVER_NPCSERVER,
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
	"G3D26090",		// CLVER_5_31x,

	"G3D04048",		// CLVER_IPHONE_1_1
	"G3D18010",		// CLVER_IPHONE_1_5
	"G3D29090",		// CLVER_IPHONE_1_11

	"",				// NSVER_UNKNOWN,
	"GRNS0000",		// NSVER_GENERIC,
	"LNX00001",		// NSVER_LNXMAD,
	0
};

static const char* const rcVersions[] =
{
	"",				// RCVER_UNKNOWN,
	"GSERV023",		// RCVER_1_010,
	"GSERV024",		// RCVER_1_1,
	"GSERV025",		// RCVER_2,
	0
};

static const char* const clientVersionString[] =
{
	"",			// CLVER_UNKNOWN,
	"",			// CLVER_NPCSERVER,
	"1.25",		// CLVER_1_25,
	"1.27",		// CLVER_1_27,
	"1.28",		// CLVER_1_28,
	"1.31",		// CLVER_1_31,
	"1.32",		// CLVER_1_32,
	"",		// CLVER_1_322,
	"",		// CLVER_1_323,
	"",		// CLVER_1_324,
	"1.33",		// CLVER_1_33,
	"",		// CLVER_1_331,
	"1.34",		// CLVER_1_34,
	"",		// CLVER_1_341,
	"1.35",		// CLVER_1_35,
	"1.36",		// CLVER_1_36,
	"1.37",		// CLVER_1_37,
	"",		// CLVER_1_371,
	"1.38",		// CLVER_1_38,
	"",		// CLVER_1_381,
	"1.39",		// CLVER_1_39,
	"",		// CLVER_1_391,
	"",		// CLVER_1_392,
	"",		// CLVER_1_4,
	"",		// CLVER_1_41,
	"1.41r1",	// CLVER_1_411,
	"2.1",		// CLVER_2_1,
	"2.12",		// CLVER_2_12,
	"2.13",		// CLVER_2_13,
	"2.14",		// CLVER_2_14,
	"2.15",		// CLVER_2_15,
	"2.151",	// CLVER_2_151,
	"2.16",		// CLVER_2_16,
	"2.17",		// CLVER_2_17,
	"2.18",		// CLVER_2_18,
	"2.19",		// CLVER_2_19,
	"2.2",		// CLVER_2_2,
	"2.21",		// CLVER_2_21,
	"2.22",		// CLVER_2_22,
	"2.3",		// CLVER_2_3,
	"2.31",		// CLVER_2_31,

	"3.0",		// CLVER_3,
	"3.01",		// CLVER_3_01,
	"3.041",	// CLVER_3_041,

	"4.0211",	// CLVER_4_0211,
	"4.034",	// CLVER_4_034,
	"4.042",	// CLVER_4_042,
	"4.110",	// CLVER_4_110,
	"4.208",	// CLVER_4_208,

	"5.07",		// CLVER_5_07,
	"5.12",		// CLVER_5_12,
	"5.31x",	// CLVER_5_31x,

	"iPhone 1.1",	// CLVER_IPHONE_1_1
	"iPhone 1.5",	// CLVER_IPHONE_1_5
	"iPhone 1.11",	// CLVER_IPHONE_1_11

	"[NPC-Server]",				// NSVER_UNKNOWN,
	"[NPC-Server] Generic",		// NSVER_GENERIC,
	"[NPC-Server] lnxmad",		// NSVER_LNXMAD,
	0
};

static const char* const rcVersionString[] =
{
	"",				// RCVER_UNKNOWN,
	"1.010",		// RCVER_1_010,
	"1.1",			// RCVER_1_1,
	"2.0",			// RCVER_2,
	0
};



int getVersionID(const CString& version)
{
	int i = 0;
	while (clientVersions[i] != 0)
	{
		if (version == CString(clientVersions[i]))
			return i;
		++i;
	}
	return CLVER_UNKNOWN;
}

const char* getVersionString(const CString& version, const int type)
{
	int i = 0;
	while (clientVersions[i] != 0)
	{
		if ((type & PLTYPE_ANYCLIENT) != 0)
		{
			if (version == CString(clientVersions[i]))
				return clientVersionString[i];
		}
		else if ((type & PLTYPE_ANYRC) != 0)
		{
			if (version == CString(rcVersions[i]))
				return rcVersionString[i];
		}
		else if ((type & PLTYPE_NPCSERVER) != 0)
		{
			if (version == CString(clientVersions[i]))
				return clientVersionString[i];
		}
		++i;
	}
	return 0;
}

int getRCVersionID(const CString& version)
{
	int i = 0;
	while (rcVersions[i] != 0)
	{
		if (version == CString(rcVersions[i]))
			return i;
		++i;
	}
	return RCVER_UNKNOWN;
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

CString removeExtension(const CString& file)
{
	int ePos = file.findl('.');
	if (ePos == -1) return file;

	return file.subString(0, ePos);
}
