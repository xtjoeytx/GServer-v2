#ifndef MAIN_H
#define MAIN_H

#include "CString.h"

bool parseArgs(int argc, char* argv[]);
void printHelp(const char* pname);
const CString getHomePath();
void shutdownServer(int sig);

#endif // MAIN_H
