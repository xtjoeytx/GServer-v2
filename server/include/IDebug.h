#ifndef IDEBUG_H
#define IDEBUG_H

#if defined(_WIN32) || defined(_WIN64)
	#if defined(DEBUG) || defined(_DEBUG)
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
	#endif
#endif

#endif
