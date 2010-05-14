#ifndef IDEBUG_H
#define IDEBUG_H

#if (defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)) && defined(_MSC_VER)
	#if defined(DEBUG) || defined(_DEBUG)
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif
#endif

#endif
