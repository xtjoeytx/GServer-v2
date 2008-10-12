#include <signal.h>
#include <stdlib.h>
#include <map>
#include "ICommon.h"
#include "main.h"
#include "IUtil.h"
#include "CLog.h"
#include "CSocket.h"
#include "TServer.h"
#include "TPlayer.h"
#include "TServerList.h"

// Function pointer for signal handling.
typedef void (*sighandler_t)(int);

std::map<CString, TServer*> serverList;
std::map<CString, boost::thread*> serverThreads;
CLog serverlog("logs/serverlog.txt");

// Home path of the gserver.
CString homepath;
static void getBasePath();

int main(int argc, char* argv[])
{
	// Shut down the server if we get a kill signal.
	signal(SIGINT, (sighandler_t) shutdownServer);
	signal(SIGTERM, (sighandler_t) shutdownServer);
	signal(SIGBREAK, (sighandler_t) shutdownServer);
	signal(SIGABRT, (sighandler_t) shutdownServer);

	// Seed the random number generator with the current time.
	srand((unsigned int)time(0));

	// Grab the base path to the server executable.
	getBasePath();

	// Create Packet-Functions
	createPLFunctions();
	createSLFunctions();

	// Program announcements.
	serverlog.out("Graal Reborn GServer version %s\n", GSERVER_VERSION);
	serverlog.out("Programmed by Joey and Nalin.\n\n");

	// Load Server Settings
	serverlog.out(":: Loading servers.txt... ");
	CSettings serversettings(CString() << homepath << "servers.txt");
	if (!serversettings.isOpened())
	{
		serverlog.out("FAILED!\n");
		return ERR_SETTINGS;
	}
	serverlog.out("success\n");

	// Make sure we actually have a server.
	if (serversettings.getInt("servercount", 0) == 0)
	{
		serverlog.out("[Error] Incorrect settings.txt file.  servercount not found.\n");
		return ERR_SETTINGS;
	}

	// Load servers.
	for (int i = 1; i <= serversettings.getInt("servercount"); ++i)
	{
		CString name = serversettings.getStr(CString() << "server_" << CString(i), "default");
		TServer* server = new TServer(name);

		// Make sure doubles don't exist.
		if (serverList.find(name) != serverList.end())
		{
			serverlog.out("[WARNING] Server %s already found, deleting old server.\n", name.text());
			delete serverList[name];
		}

		// Initialize the server.
		serverlog.out(":: Starting server: %s...\n", name.text());
		if (server->init() != 0)
		{
			serverlog.out("[Error] Failed to start server: %s\n", name.text());
			delete server;
			continue;
		}
		serverList[name] = server;

		// Put the server in its own thread.
		serverThreads[name] = new boost::thread(boost::ref(*server));
	}

	// Announce that the program is now running.
	serverlog.out(":: Program started.\n");

	// Wait on each thread to end.
	// Once all threads have ended, the program has terminated.
	for (std::map<CString, boost::thread*>::iterator i = serverThreads.begin(); i != serverThreads.end();)
	{
		boost::thread* t = i->second;
		try
		{
			t->join();
			++i;
		}
		catch (boost::thread_interrupted e)
		{
			t->detach();
			delete t;
			i = serverThreads.erase(i);
		}
	}

	// Delete all the servers.
	for (std::map<CString, TServer*>::iterator i = serverList.begin(); i != serverList.end(); )
	{
		delete i->second;
		serverList.erase(i++);
	}

	// Destroy the sockets.
	CSocket::socketSystemDestroy();

	return ERR_SUCCESS;
}

/*
	Extra-Cool Functions :D
*/

const CString getHomePath()
{
	return homepath;
}

void shutdownServer(int signal)
{
	serverlog.out(":: The server is now shutting down...\n");

	// Interrupt each thread.  We are shutting down the server.
	// TODO: server shutdown function.
	for (std::map<CString, boost::thread*>::iterator i = serverThreads.begin(); i != serverThreads.end(); ++i)
	{
		boost::thread* t = i->second;
		t->interrupt();
	}
}

void getBasePath()
{
	#if defined(_WIN32) || defined(_WIN64)
	// Get the path.
	char path[ MAX_PATH ];
	GetModuleFileNameA(0, path, MAX_PATH);

	// Find the program exe and remove it from the path.
	// Assign the path to homepath.
	homepath = path;
	int pos = homepath.findl('\\');
	if (pos == -1) homepath.clear();
	else if (pos != (homepath.length() - 1))
		homepath.removeI(++pos, homepath.length());
#else
	// Get the path to the program.
	char path[260];
	memset((void*)path, 0, 260);
	readlink("/proc/self/exe", path, sizeof(path));

	// Assign the path to homepath.
	char* end = strrchr(path, '/');
	if (end != 0)
	{
		end++;
		if (end != 0) *end = '\0';
		homepath = path;
	}
#endif
}
