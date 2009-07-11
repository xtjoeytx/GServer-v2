#include "CPluginManager.h"
#include "CLog.h"
#include "TServer.h"

#ifdef _WIN32
	#undef LoadLibrary
	#define LoadLibrary LoadLibraryA
#endif

// -- Function: Constructor -- //
CPluginManager::CPluginManager(TServer *pServer)
{
	this->mServer = pServer;
}

// -- Function: Destructor -- //
CPluginManager::~CPluginManager()
{
	// Remove Plugins
	for (std::vector<plugin_t *>::iterator i = mPlugins.begin(); i != mPlugins.end(); ++i)
		delete (*i);
	mPlugins.clear();

	// Release Plugins
#ifdef _WIN32
	for (std::vector<HMODULE>::iterator i = mHandles.begin(); i != mHandles.end(); ++i)
		delete (*i);
	mHandles.clear();
#endif
}

// -- Function: Load Plugin -- //
bool CPluginManager::LoadPlugin(const CString &pPluginName)
{
#ifdef _WIN32
	// Load Plugin
	HMODULE hPlugin = LoadLibrary(pPluginName.text());
	if (hPlugin == NULL)
	{
		mServer->getServerLog().out("[PLUGIN] %s, LoadLibrary() failed.\n", pPluginName.text());
		return false;
	}

	// Find the plugin GetPluginI() method
	getplugin_t pGetPlugin = (getplugin_t)GetProcAddress(hPlugin, "GetPluginI");
	if (pGetPlugin == NULL)
	{
		mServer->getServerLog().out("[PLUGIN] %s, GetProcAddress() failed.\n", pPluginName.text());
		return false;
	}
	mHandles.push_back(hPlugin);

	// Add Plugin to Manager
	plugin_t *plugin = pGetPlugin();
	mServer->getServerLog().out("[PLUGIN] Succesfully loaded %s v%s.\n", plugin->name, plugin->version);
	AddPlugin(plugin);
#endif
	return true;
}

// -- Function: Add Plugin -- //
void CPluginManager::AddPlugin(plugin_t *pPlugin)
{
	mPlugins.push_back(pPlugin);
	pPlugin->lib->Initialize(mServer);
}


void CPluginManager::LoadAccount(const char* accountName, const char** accountText)
{
	for (std::vector<plugin_t *>::iterator i = mPlugins.begin(); i != mPlugins.end(); ++i)
	{
		if ((*i)->lib->LoadAccount(accountName, accountText) == PLUGIN_STOP)
			return;
	}
}

void CPluginManager::SaveAccount(const char* accountName, const char* accountText)
{
	for (std::vector<plugin_t *>::iterator i = mPlugins.begin(); i != mPlugins.end(); ++i)
	{
		if ((*i)->lib->SaveAccount(accountName, accountText) == PLUGIN_STOP)
			return;
	}
}
