#ifndef MAIN_H
#define MAIN_H

/*
	Header-Includes
*/
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include <vector>

#include "ICommon.h"
#include "IUtil.h"
#include "CString.h"

/*
	Define Functions
*/
const CString getHomePath();
void shutdownServer(int sig);

#endif // MAIN_H
