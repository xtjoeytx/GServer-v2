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
		CString LoadAccount(const CString& accountName);
		bool SaveAccount(const CString& accountName, const CString& accountText);

	protected:
		TServer *mServer;
		std::vector<plugin_t *> mPlugins;
#ifdef _WIN32
		std::vector<HMODULE> mHandles;
#endif
};

// Used by the plugins to interface the gserver.
/*
class CPluginInterface
{
	public:
		const char* getServerString(const char* string);
};
*/

#endif
