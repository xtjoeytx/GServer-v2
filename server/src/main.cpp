#include "IDebug.h"
#include <atomic>
#include <csignal>
#include <filesystem>
#include <functional>

#include <cstdlib>
#include <map>

#include "Account.h"
#include "CLog.h"
#include "CSocket.h"
#include "CString.h"
#include "IConfig.h"
#include "IUtil.h"
#include "Server.h"
#include "main.h"

// Linux specific stuff.
#if !(defined(_WIN32) || defined(_WIN64))
	#include <unistd.h>
	#ifndef SIGBREAK
		#define SIGBREAK SIGQUIT
	#endif
#endif

// Function pointer for signal handling.
typedef void (*sighandler_t)(int);

// Home path of the gserver.
CString homePath;
static void getBasePath();
std::string getBaseHomePath()
{
	return homePath.text();
}

void getBasePath()
{
#if defined(_WIN32) || defined(_WIN64)
	// Get the path.
	char path[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, path);

	// Find the program exe and remove it from the path.
	// Assign the path to homepath.
	homePath = path;
	homePath += "\\";
	int pos = homePath.findl('\\');
	if (pos == -1) homePath.clear();
	else if (pos != (homePath.length() - 1))
		homePath.removeI(++pos, homePath.length());
#elif __APPLE__
	char path[255];
	if (!getcwd(path, sizeof(path)))
		printf("Error getting CWD\n");

	homePath = path;
	if (homePath[homePath.length() - 1] != '/')
		homePath << '/';
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
		homePath = path;
	}
#endif
}

CLog serverlog("startuplog.txt");
CString overrideServer;
CString overridePort;
CString overrideServerIp = nullptr;
CString overrideLocalIp = nullptr;
CString overrideServerInterface = nullptr;
CString overrideName = nullptr;
CString overrideStaff = nullptr;

std::atomic_bool shutdownProgram{ false };

#ifndef NOMAIN
int main(int argc, char* argv[])
{
	if (parseArgs(argc, argv))
		return 1;

	#if (defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)) && defined(_MSC_VER)
		#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		#endif
	#endif

	{
		// Shut down the server if we get a kill signal.
		signal(SIGINT, (sighandler_t)shutdownServer);
		signal(SIGTERM, (sighandler_t)shutdownServer);
		signal(SIGBREAK, (sighandler_t)shutdownServer);
		signal(SIGABRT, (sighandler_t)shutdownServer);

		// Seed the random number generator with the current time.
		srand((unsigned int)time(0));

		// Grab the base path to the server executable.
		getBasePath();

		// Program announcements.
		serverlog.out("%s %s version %s\n", APP_VENDOR, APP_NAME, APP_VERSION);
		serverlog.out("Programmed by %s.\n\n", APP_CREDITS);

		// Load Server Settings
		if (overrideServer.isEmpty())
		{
			serverlog.out(":: Determining the server to start... ");

			auto found_server = [](const std::string& why, const std::string& server)
			{
				serverlog.append("success! %s\n", why.c_str());
				overrideServer = server;
			};

			// startupserver.txt
			{
				CString startup;
				startup.load(CString(homePath) << "startupserver.txt");
				if (!startup.isEmpty())
					found_server("(startupserver.txt)", std::string{ startup.text() });
			}

			// Number of directories.
			if (overrideServer.isEmpty())
			{
				std::vector<std::filesystem::path> servers;

				std::filesystem::path base_dir{ homePath.text() };
				for (const auto& p: std::filesystem::directory_iterator{ base_dir / "servers" })
				{
					if (p.is_directory())
						servers.push_back(p.path().filename());
				}

				if (servers.size() == 1)
					found_server("(directory search)", servers.front().string());
			}

			// Failure.
			if (overrideServer.isEmpty())
			{
				serverlog.append("FAILED!\n");
				return ERR_SETTINGS;
			}
		}

		// Initialize the server.
		auto server = std::make_unique<Server>(overrideServer);
		serverlog.out(":: Starting server: %s.\n", overrideServer.text());
		if (server->init(overrideServerIp, overridePort, overrideLocalIp, overrideServerInterface) != 0)
		{
			serverlog.out("** [Error] Failed to start server: %s\n", overrideServer.text());
			return 1;
		}

		// Save override settings.
		{
			auto& settings = server->getSettings();

			if (!overrideName.isEmpty())
				settings.addKey("name", overrideName);

			if (!overrideStaff.isEmpty())
			{
				if (!server->isStaff(overrideStaff))
				{
					auto staff = settings.getStr("staff");
					settings.addKey("staff", staff << "," << overrideStaff);
				}

				Account accfs(server.get());
				accfs.loadAccount(overrideStaff, false);
				if (accfs.getOnlineTime() == 0)
				{
					accfs.loadAccount("YOURACCOUNT");
					accfs.setAccountName(overrideStaff);
					accfs.saveAccount();
				}
			}

			settings.saveFile();
			server->loadSettings();
		}

		// Announce that the program is now running.
		serverlog.out(":: Program started.\n");
	#if defined(WIN32) || defined(WIN64)
		serverlog.out(":: Press CTRL+C to close the program.  DO NOT CLICK THE X, you will LOSE data!\n");
	#endif

		// Run the server.
		(*server)();

		// Destroy the sockets.
		CSocket::socketSystemDestroy();
	}

	return ERR_SUCCESS;
}
#endif
/*
	Extra-Cool Functions :D
*/

void shutdownServer(int signal)
{
	serverlog.out(":: The server is now shutting down...\n-------------------------------------\n\n");

	shutdownProgram = true;
}

bool parseArgs(int argc, char* argv[])
{
	std::vector<CString> args;

	auto test_for_end = [&args](auto&& iterator, auto&& end)
	{
		if (iterator == end)
		{
			printHelp(args[0].text());
			return true;
		}
		return false;
	};

	bool use_env = getenv("USE_ENV");

	if (!use_env)
	{
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
				else
				{
					if (test_for_end(++i, args.end()))
						return true;

					if (key == "server")
						overrideServer = *i;
					else if (key == "port" && !overrideServer.isEmpty())
						overridePort = *i;
					else if (key == "localip" && !overrideServer.isEmpty())
						overrideLocalIp = *i;
					else if (key == "serverip" && !overrideServer.isEmpty())
						overrideServerIp = *i;
					else if (key == "interface" && !overrideServer.isEmpty())
						overrideServerInterface = *i;
					else if (key == "staff" && !overrideServer.isEmpty())
						overrideStaff = *i;
					else if (key == "name" && !overrideServer.isEmpty())
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
						if (test_for_end(++i, args.end()))
							return true;
						overrideServer = *i;
					}
					if ((*i)[j] == 'p' && !overrideServer.isEmpty())
					{
						if (test_for_end(++i, args.end()))
							return true;
						overridePort = *i;
					}
				}
			}
		}
	}
	else
	{
		if (getenv("SERVER"))
			overrideServer = getenv("SERVER");

		if (getenv("PORT") && !overrideServer.isEmpty())
			overridePort = getenv("PORT");

		if (getenv("LOCALIP") && !overrideServer.isEmpty())
			overrideLocalIp = getenv("LOCALIP");

		if (getenv("SERVERIP") && !overrideServer.isEmpty())
			overrideServerIp = getenv("SERVERIP");

		if (getenv("INTERFACE") && !overrideServer.isEmpty())
			overrideServerInterface = getenv("INTERFACE");

		if (getenv("STAFFACCOUNT") && !overrideServer.isEmpty())
			overrideStaff = getenv("STAFFACCOUNT");

		if (getenv("SERVERNAME") && !overrideServer.isEmpty())
			overrideName = getenv("SERVERNAME");
	}

	return false;
}

void printHelp(const char* pname)
{
	serverlog.out("%s %s version %s\n", APP_VENDOR, APP_NAME, APP_VERSION);
	serverlog.out("Programmed by %s.\n\n", APP_CREDITS);
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
