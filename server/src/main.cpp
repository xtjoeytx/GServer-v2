#include <atomic>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <map>
#include <iostream>
#include <string_view>

#include <CSocket.h>
#include <CString.h>
#include <IUtil.h>
#include "BabyDI.h"

#include "IConfig.h"

#include "Account.h"
#include "Server.h"
#include "main.h"

#include "utilities/Log.h"

using namespace preagonal;

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
static CString getBasePath()
{
	CString homePath;
#if defined(_WIN32) || defined(_WIN64)
	// Get the path.
	char path[MAX_PATH];
	DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
	std::string_view path_view{ path, length };
	if (auto pos = path_view.find_last_of("\\"); pos != std::string_view::npos)
		path_view = path_view.substr(0, pos);
	homePath = path_view;
#elif __APPLE__
	char path[1024];
	uint32_t size = sizeof(path);
	int result = _NSGetExecutablePath(&path[0], &size);
	if (result == -1)
		printf("Error getting executable path\n");

	std::string_view path_view{ path, size };
	homePath = path_view;
#else
	// Get the path to the program.
	char path[1024];
	memset((void*)path, 0, 1024);
	readlink("/proc/self/exe", path, sizeof(path));

	// Assign the path to homepath.
	char* end = strrchr(path, '/');
	if (end != 0)
	{
		*end = '\0';
		homePath = path;
	}
#endif
	printf("Calculated home path: %s\n", homePath.text());
	return homePath;
}
std::filesystem::path getBaseHomePath()
{
	static std::filesystem::path homePath{ getBasePath().toString() };
	return homePath;
}

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

	{
		// Shut down the server if we get a kill signal.
		signal(SIGINT, (sighandler_t)shutdownServer);
		signal(SIGTERM, (sighandler_t)shutdownServer);
		signal(SIGBREAK, (sighandler_t)shutdownServer);
		signal(SIGABRT, (sighandler_t)shutdownServer);

		// Seed the random number generator with the current time.
		srand((unsigned int)time(0));

		// Load Server Settings
		std::string discovery_mode;
		if (overrideServer.isEmpty())
		{
			std::cout << ":: Determining the server to start... ";

			auto found_server = [&discovery_mode](const std::string& why, std::string_view server, const std::filesystem::path& working_directory)
			{
				std::cout << "success! " << why << std::endl;
				discovery_mode = why;
				overrideServer = server;
				if (!working_directory.empty() && std::filesystem::exists(working_directory))
					std::filesystem::current_path(working_directory);
			};

			// Current working directory.
			if (overrideServer.isEmpty())
			{
				std::filesystem::path cwd = std::filesystem::current_path();
				if (std::filesystem::exists(cwd / "config" / "serveroptions.txt"))
					found_server("(current working directory)", cwd.filename().string(), {});
			}

			// startupserver.txt
			if (overrideServer.isEmpty())
			{
				CString startup;
				startup.load("startupserver.txt");
				if (!startup.isEmpty())
					found_server("(startupserver.txt)", startup.text(), std::filesystem::path{ "servers" } / startup.text());
			}

			// Number of directories.
			if (overrideServer.isEmpty())
			{
				std::vector<std::filesystem::path> servers;

				for (const auto& p: std::filesystem::directory_iterator{ "servers" })
				{
					if (p.is_directory())
						servers.push_back(p.path().filename());
				}

				if (servers.size() == 1)
					found_server("(directory search)", servers.front().string(), std::filesystem::path{ "servers" } / servers.front());
			}

			// Failure.
			if (overrideServer.isEmpty())
			{
				std::cout << "FAILED!" << std::endl;
				std::cerr << "Failed to start server: no server specified and no default server found." << std::endl;
				return ERR_SETTINGS;
			}
		}

		// Create the server.
		auto* server = BabyDI_PROVIDE(Server, new Server(overrideServer));

		// Program announcements.
		log::printLine(log::server, "{} {} version {}", APP_VENDOR, APP_NAME, APP_VERSION);
		log::printLine(log::server, "Programmed by {}.", APP_CREDITS);
		log::printLine(log::server, "");

		// Initialize the server.
		log::printLine(log::server, ":: Starting server: {}.", overrideServer);
		log::printLine(log::server, "     {}: {}", discovery_mode, std::filesystem::current_path().string());
		if (server->init(overrideServerIp, overridePort, overrideLocalIp, overrideServerInterface) != 0)
		{
			log::printLine(log::server, "** [Error] Failed to start server: {}", overrideServer);
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

				Account accfs;
				server->getAccountLoader().loadAccount(overrideStaff.toStringView(), accfs);
				if (accfs.onlineSeconds == 0)
				{
					server->getAccountLoader().loadAccount("YOURACCOUNT", accfs);
					accfs.name = overrideStaff.toStringView();
					server->getAccountLoader().saveAccount(accfs);
				}
			}

			settings.saveFile();
			server->loadSettings();
		}

		// Announce that the program is now running.
		log::print(log::server, ":: Started server {}", server->getName());
		if (server->getSettings().exists("name"))
			log::printLine(log::server, " ({})", server->getSettings().getStr("name"));
		else log::printLine(log::server, "");

	#if defined(WIN32) || defined(WIN64)
		log::printLine(log::server, ":: Press CTRL+C to close the program.  DO NOT CLICK THE X, you will LOSE data!");
	#endif

		// Run the server.
		(*server)();

		// Destroy the sockets.
		CSocket::socketSystemDestroy();

		BabyDI_RELEASE(Server);
	}

	return ERR_SUCCESS;
}
#endif
/*
	Extra-Cool Functions :D
*/

void shutdownServer(int signal)
{
	log::printLine(log::server, ":: The server is now shutting down...");
	log::printLine(log::server, "-------------------------------------");

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
	printf("%s %s version %s\n", APP_VENDOR, APP_NAME, APP_VERSION);
	printf("Programmed by %s.\n\n", APP_CREDITS);
	printf("USAGE: %s [options]\n\n", pname);
	printf("Commands:\n\n");
	printf(" -h, --help\t\tPrints out this help text.\n");
	printf(" -s, --server DIR\tOverride the servers.txt by specifying which server directory to use.\n");
	printf(" -p, --port PORT\tSpecify which port to use when using servers.txt override.\n");
	printf(" --localip IP\tSpecify which IP to retrieve when on the same network as the server.\n");
	printf(" --serverip IP\tSpecify which IP that the listserver should deliver to clients.\n");
	printf(" --interface IP\tSpecify which IP to bind the server to.\n");

	printf("\n");
}
