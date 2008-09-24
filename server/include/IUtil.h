#ifndef IUTIL_H
#define IUTIL_H

#include <vector>

#define inrange(a, b, c) ((a) >= (b) && (a) <= (c))

#if defined(_WIN32)
	#define wait(a) Sleep(a)
#else
	#define wait(a) usleep(a*1000)
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
	CLIENTTYPE_CLIENT,
	CLIENTTYPE_RC,
	CLIENTTYPE_AWAIT,
	CLIENTTYPE_CLIENT22 = 5,
};


#endif
