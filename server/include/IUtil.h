#ifndef IUTIL_H
#define IUTIL_H

#include "CString.h"
#include <vector>

#define inrange(a, b, c) ((a) >= (b) && (a) <= (c))

#if defined(_WIN32) || defined(_WIN64)
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
	for (unsigned int i = 0; i < a.size(); ++i)
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
	CLVER_1_25,
	CLVER_1_27,
	CLVER_1_28,
	CLVER_1_31,
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
	CLVER_2_1,
	CLVER_2_12,
	CLVER_2_13,
	CLVER_2_14,
	CLVER_2_15,
	CLVER_2_151,
	CLVER_2_16,
	CLVER_2_17,
	CLVER_2_18,
	CLVER_2_19,
	CLVER_2_2,
	CLVER_2_21,
	CLVER_2_22,
	CLVER_2_3,
	CLVER_2_31,

	CLVER_3,
	CLVER_3_01,
	CLVER_3_041,

	CLVER_4_0211,
	CLVER_4_034,
	CLVER_4_042,
	CLVER_4_110,
	CLVER_4_208,

	CLVER_5_07,
	CLVER_5_12,
};

enum
{
	RCVER_1_010,
	RCVER_1_1,
};

int getVersionID(const CString& version);
unsigned char getColor(const CString& color);
CString removeComments(const CString& code, const CString& newLine = "\n");
CString removeExtension(const CString& ext);

#endif
