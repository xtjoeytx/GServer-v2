#ifndef MAIN_H
#define MAIN_H

#include <filesystem>

bool parseArgs(int argc, char* argv[]);
void printHelp(const char* pname);
std::filesystem::path getBaseHomePath();
void shutdownServer(int signal);

#endif // MAIN_H
