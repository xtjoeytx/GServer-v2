#include "IDebug.h"
#include <thread>
#include <atomic>
#include <functional>
#include <signal.h>
#include <stdlib.h>
#include <map>

#include "main.h"
#include "IConfig.h"
#include "CString.h"
#include "IUtil.h"
#include "CLog.h"
#include "CSocket.h"
#include "TServer.h"
#include <TAccount.h>

// Linux specific stuff.
#if !(defined(_WIN32) || defined(_WIN64))
	#include <unistd.h>
	#ifndef SIGBREAK
		#define SIGBREAK SIGQUIT
	#endif
#endif

// Function pointer for signal handling.
typedef void (*sighandler_t)(int);

std::map<CString, TServer*> serverList;
std::map<CString, std::thread*> serverThreads;

CLog serverlog("startuplog.txt");
CString overrideServer;
CString overridePort;
CString overrideServerIp = nullptr;
CString overrideLocalIp = nullptr;
CString overrideServerInterface = nullptr;
CString overrideName = nullptr;
CString overrideStaff = nullptr;

// Home path of the gserver.
CString homepath;
static void getBasePath();

std::atomic_bool shutdownProgram{ false };

int main(int argc, char* argv[])
{
	if (parseArgs(argc, argv))
		return 1;

#if (defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)) && defined(_MSC_VER)
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
		serverlog.out("Programmed by %s.\n\n", GSERVER_CREDITS);

		// Load Server Settings
		if (overrideServer.isEmpty())
		{
			serverlog.out(":: Loading servers.txt... ");
			CSettings serversettings(CString(homepath) << "servers.txt");
			if (!serversettings.isOpened())
			{
				serverlog.append("FAILED!\n");
				return ERR_SETTINGS;
			}
			serverlog.append("success\n");

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
				CString serverinterface = serversettings.getStr(CString() << "server_" << CString(i) << "_interface");

				// Initialize the server.
				serverlog.out(":: Starting server: %s.\n", name.text());
				if (server->init(serverip, serverport, localip, serverinterface) != 0)
				{
					serverlog.out("** [Error] Failed to start server: %s\n", name.text());
					delete server;
					continue;
				}
				serverList[name] = server;

				// Put the server in its own thread.
				serverThreads[name] = new std::thread(std::ref(*server));
			}
		}
		else
		{
			TServer* server = new TServer(overrideServer);

			auto *settings = server->getSettings();

			serverlog.out(":: Starting server: %s.\n", overrideServer.text());
			if (server->init(overrideServerIp, overridePort, overrideLocalIp, overrideServerInterface ) != 0)
			{
				serverlog.out("** [Error] Failed to start server: %s\n", overrideServer.text());
				delete server;
				return 1;
			}

			if (!overrideName.isEmpty())
			{
				settings->addKey("name", overrideName);
				auto * sl = server->getServerList();
				if (sl)
					sl->setName(settings->getStr("name"));
			}

			if (!overrideStaff.isEmpty()) {
				settings->addKey("staff", overrideStaff);
				auto * accfs = new TAccount(server);
				accfs->loadAccount(overrideStaff, false);
				if (accfs->getNickname() == "default") {
					accfs->loadAccount("YOURACCOUNT", false);

					accfs->setAccountName(overrideStaff);
					accfs->saveAccount();
				}
				accfs = NULL;
			}

			settings->saveFile();

			serverList[overrideServer] = server;

			// Put the server in its own thread.
			serverThreads[overrideServer] = new std::thread(std::ref(*server));
		}

		// Announce that the program is now running.
		serverlog.out(":: Program started.\n");
	#if defined(WIN32) || defined(WIN64)
		serverlog.out(":: Press CTRL+C to close the program.  DO NOT CLICK THE X, you will LOSE data!\n");
	#endif

		// Wait on each thread to end.
		// Once all threads have ended, the program has terminated.
		for (auto i = serverThreads.begin(); i != serverThreads.end();)
		{
			std::thread* t = i->second;
			if (t == nullptr) serverThreads.erase(i++);
			else
			{
				t->join();
				++i;
			}
		}

		// Delete all the servers.
		for (auto i = serverList.begin(); i != serverList.end(); )
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

bool parseArgs(int argc, char* argv[])
{
	std::vector<CString> args;
	for (int i = 0; i < argc; ++i)
		args.push_back(CString(argv[i]));

	for (auto i = args.begin(); i != args.end(); ++i)
	{
		if ((*i).find("--") == 0)
		{
			CString key((*i).subString(2));
			if (key == "help")
			{
				printHelp(args[0].text());
				return true;
			}
			else if (key == "server")
			{
				++i;
				if (i == args.end())
				{
					printHelp(args[0].text());
					return true;
				}
				overrideServer = *i;
			}
			else if (key == "port" && !overrideServer.isEmpty())
			{
				++i;
				if (i == args.end())
				{
					printHelp(args[0].text());
					return true;
				}
				overridePort = *i;
			}
			else if (key == "localip" && !overrideServer.isEmpty())
			{
				++i;
				if (i == args.end())
				{
					printHelp(args[0].text());
					return true;
				}
				overrideLocalIp = *i;
			}
			else if (key == "serverip" && !overrideServer.isEmpty())
			{
				++i;
				if (i == args.end())
				{
					printHelp(args[0].text());
					return true;
				}
				overrideServerIp = *i;
			}
			else if (key == "interface" && !overrideServer.isEmpty())
			{
				++i;
				if (i == args.end())
				{
					printHelp(args[0].text());
					return true;
				}
				overrideServerInterface = *i;
			}
			else if (key == "staff" && !overrideServer.isEmpty())
			{
				++i;
				if (i == args.end())
				{
					printHelp(args[0].text());
					return true;
				}
				overrideStaff = *i;
			}
			else if (key == "name" && !overrideServer.isEmpty())
			{
				++i;
				if (i == args.end())
				{
					printHelp(args[0].text());
					return true;
				}
				overrideName = *i;
			}
		}
		else if ((*i)[0] == '-')
		{
			for (int j = 1; j < (*i).length(); ++j)
			{
				if ((*i)[j] == 'h')
				{
					printHelp(args[0].text());
					return true;
				}
				if ((*i)[j] == 's')
				{
					++i;
					if (i == args.end())
					{
						printHelp(args[0].text());
						return true;
					}
					overrideServer = *i;
				}
				if ((*i)[j] == 'p' && !overrideServer.isEmpty())
				{
					++i;
					if (i == args.end())
					{
						printHelp(args[0].text());
						return true;
					}
					overridePort = *i;
				}
			}
		}
	}
	return false;
}

void printHelp(const char* pname)
{
	serverlog.out("Graal Reborn GServer version %s\n", GSERVER_VERSION);
	serverlog.out("Programmed by %s.\n\n", GSERVER_CREDITS);
	serverlog.out("USAGE: %s [options]\n\n", pname);
	serverlog.out("Commands:\n\n");
	serverlog.out(" -h, --help\t\tPrints out this help text.\n");
	serverlog.out(" -s, --server DIR\tOverride the servers.txt by specifying which server directory to use.\n");
	serverlog.out(" -p, --port PORT\tSpecify which port to use when using servers.txt override.\n");
	serverlog.out(" --localip IP\tSpecify which IP to retrieve when on the same network as the server.\n");
	serverlog.out(" --serverip IP\tSpecify which IP that the listserver should deliver to clients.\n");
	serverlog.out(" --interface IP\tSpecify which IP to bind the server to.\n");

	serverlog.out("\n");
}

const CString getHomePath()
{
	return homepath;
}

void shutdownServer(int sig)
{
	serverlog.out(":: The server is now shutting down...\n-------------------------------------\n\n");

	shutdownProgram = true;
}

void getBasePath()
{
	#if defined(_WIN32) || defined(_WIN64)
	// Get the path.
	char path[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH,path);

	// Find the program exe and remove it from the path.
	// Assign the path to homepath.
	homepath = path;
	homepath += "\\";
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
