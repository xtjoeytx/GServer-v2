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
bool parseArgs(int argc, char* argv[]);
void printHelp(const char* pname);
const CString getHomePath();
void shutdownServer(int sig);

#endif // MAIN_H
