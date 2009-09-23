#include "IDebug.h"
#include <signal.h>
#include <stdlib.h>
#include <map>
#include <boost/thread.hpp>
#include "ICommon.h"
#include "main.h"
#include "IUtil.h"
#include "CLog.h"
#include "CSocket.h"
#include "TServer.h"
#include "TPlayer.h"
#include "TServerList.h"

// Linux specific stuff.
#if !(defined(_WIN32) || defined(_WIN64))
	#ifndef SIGBREAK
		#define SIGBREAK SIGQUIT
	#endif
#endif

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
#if defined(WIN32) || defined(WIN64)
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
#endif

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
			serverlog.out("** [Error] Incorrect settings.txt file.  servercount not found.\n");
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
				serverlog.out("-- [WARNING] Server %s already found, deleting old server.\n", name.text());
				delete serverList[name];
			}

			// See if an override was specified.
			CString serverip = serversettings.getStr(CString() << "server_" << CString(i) << "_ip");
			CString serverport = serversettings.getStr(CString() << "server_" << CString(i) << "_port");
			CString localip = serversettings.getStr(CString() << "server_" << CString(i) << "_localip");

			// Initialize the server.
			serverlog.out(":: Starting server: %s.\n", name.text());
			if (server->init(serverip, serverport, localip) != 0)
			{
				serverlog.out("** [Error] Failed to start server: %s\n", name.text());
				delete server;
				continue;
			}
			serverList[name] = server;

			// Put the server in its own thread.
			serverThreads[name] = new boost::thread(boost::ref(*server));
		}

		// Announce that the program is now running.
		serverlog.out(":: Program started.\n");
	#if defined(WIN32) || defined(WIN64)
		serverlog.out(":: Press CTRL+C to close the program.  DO NOT CLICK THE X, you will LOSE data!\n");
	#endif

		// Wait on each thread to end.
		// Once all threads have ended, the program has terminated.
		for (std::map<CString, boost::thread*>::iterator i = serverThreads.begin(); i != serverThreads.end();)
		{
			boost::thread* t = i->second;
			if (t == 0) serverThreads.erase(i++);
			else
			{
				t->join();
				++i;
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
	}

	return ERR_SUCCESS;
}

/*
	Extra-Cool Functions :D
*/

const CString getHomePath()
{
	return homepath;
}

void shutdownServer(int sig)
{
	serverlog.out(":: The server is now shutting down...\n");

	// Interrupt each thread.  We are shutting down the server.
	for (std::map<CString, boost::thread*>::iterator i = serverThreads.begin(); i != serverThreads.end(); ++i)
	{
		boost::thread* t = i->second;
		t->interrupt();
		t->join();
		t->detach();
		serverThreads[i->first] = 0;
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
