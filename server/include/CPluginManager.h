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
		void OnStart()
		{
			for (std::vector<plugin_t *>::iterator i = mPlugins.begin(); i != mPlugins.end(); ++i)
				(*i)->lib->OnStart();
		}

	protected:
		TServer *mServer;
		std::vector<plugin_t *> mPlugins;
#ifdef _WIN32
		std::vector<HMODULE> mHandles;
#endif

};

#endif
