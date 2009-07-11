#ifndef CPLUGINMANAGER_H
#define CPLUGINMANAGER_H

// Headers
#include <vector>
#include "CPlugin.h"
#include "CString.h"

// Classes
class TServer;

// -- Class: CPluginManager -- //
class CPluginManager
{
	public:
		// -- Constructor | Destructor -- //
		CPluginManager(TServer *pServer);
		~CPluginManager();

		// -- Functions -> Plugin Loading -- //
		bool LoadPlugin(const CString& pPluginName);
		void AddPlugin(plugin_t *pPlugin);

		// -- Functions -> Events -- //
		void LoadAccount(const char* accountName, const char** accountText);
		void SaveAccount(const char* accountName, const char* accountText);

	protected:
		TServer *mServer;
		std::vector<plugin_t *> mPlugins;
#ifdef _WIN32
		std::vector<HMODULE> mHandles;
#endif

};

#endif
