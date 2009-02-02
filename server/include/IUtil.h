#ifndef IUTIL_H
#define IUTIL_H

#include "CString.h"
#include <vector>

#define inrange(a, b, c) ((a) >= (b) && (a) <= (c))

#if defined(_WIN32)
	#define sleep(a) Sleep(a)
#else
	#define sleep(a) usleep(a*1000)
#endif

/*
	Vector-Functions
*/
template <class T>
int vecSearch(std::vector<T>& a, T b)
{
	for (unsigned int i = 0; i < a.size(); i++)
	{
		if (a[i] == b)
			return i;
	}

	return -1;
}

template <class T>
void vecRemove(std::vector<T>& a, T b)
{
	typename std::vector<T>::iterator i;
	for (i = a.begin(); i != a.end(); ++i)
	{
		if (*i == b)
		{
			a.erase(i);
			return;
		}
	}
}

template <class T>
int vecReplace(std::vector<T>& a, void *b, void *c)
{
	for (unsigned int i = 0; i < a.size(); i++)
	{
		if (a[i] == b)
		{
			a[i] = (T)c;
			return i;
		}
	}

	return -1;
}

/*
	Definitions
*/
enum // Return-Errors
{
	ERR_SUCCESS = 0,
	ERR_SETTINGS = -1,
	ERR_SOCKETS = -2,
	ERR_MYSQL = -3,
	ERR_LISTEN = -4,
};

enum
{
	CLIENTTYPE_CLIENT		= 0,
	CLIENTTYPE_RC			= 1,
	CLIENTTYPE_AWAIT		= 2,
	CLIENTTYPE_NPCCONTROL	= 3,
	CLIENTTYPE_CLIENT2		= 5,
	CLIENTTYPE_RC2			= 6,
};

enum
{
	COLOR_WHITE			= 0,
	COLOR_YELLOW		= 1,
	COLOR_ORANGE		= 2,
	COLOR_PINK			= 3,
	COLOR_RED			= 4,
	COLOR_DARKRED		= 5,
	COLOR_LIGHTGREEN	= 6,
	COLOR_GREEN			= 7,
	COLOR_DARKGREEN		= 8,
	COLOR_LIGHTBLUE		= 9,
	COLOR_BLUE			= 10,
	COLOR_DARKBLUE		= 11,
	COLOR_BROWN			= 12,
	COLOR_CYNOBER		= 13,
	COLOR_PURPLE		= 14,
	COLOR_DARKPURPLE	= 15,
	COLOR_LIGHTGRAY		= 16,
	COLOR_GRAY			= 17,
	COLOR_BLACK			= 18,
	COLOR_TRANSPARENT	= 19,
};

enum
{
	CLVER_1_32,
	CLVER_1_322,
	CLVER_1_323,
	CLVER_1_324,
	CLVER_1_33,
	CLVER_1_331,
	CLVER_1_34,
	CLVER_1_341,
	CLVER_1_35,
	CLVER_1_36,
	CLVER_1_37,
	CLVER_1_371,
	CLVER_1_38,
	CLVER_1_381,
	CLVER_1_39,
	CLVER_1_391,
	CLVER_1_392,
	CLVER_1_4,
	CLVER_1_41,
	CLVER_1_411,
	CLVER_2,
	CLVER_2_01,
	CLVER_2_02,
	CLVER_2_021,
	CLVER_2_03,
	CLVER_2_04,
	CLVER_2_05,
	CLVER_2_052,
	CLVER_2_1,
	CLVER_2_101,
	CLVER_2_102,
	CLVER_2_103,
	CLVER_2_12,
	CLVER_2_121,
	CLVER_2_13,
	CLVER_2_133,
	CLVER_2_134,
	CLVER_2_14,
	CLVER_2_144,
	CLVER_2_145,
	CLVER_2_146,
	CLVER_2_147,
	CLVER_2_15,
	CLVER_2_152,
	CLVER_2_16,
	CLVER_2_161,
	CLVER_2_162,
	CLVER_2_163,
	CLVER_2_164,
	CLVER_2_165,
	CLVER_2_166,
	CLVER_2_167,
	CLVER_2_17,
	CLVER_2_171,
	CLVER_2_18,
	CLVER_2_181,
	CLVER_2_19,
	CLVER_2_191,
	CLVER_2_2,
	CLVER_2_21,
	CLVER_2_22,
	CLVER_2_3,
	CLVER_2_31,

	CLVER_5_12,
};

int getVersionID(const CString& version);
unsigned char getColor(const CString& color);
CString removeComments(const CString& code, const CString& newLine = "\n");

#endif
