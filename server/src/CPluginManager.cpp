#include "IDebug.h"
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


CString CPluginManager::LoadAccount(const CString& accountName)
{
	CString ret;
	for (std::vector<plugin_t *>::iterator i = mPlugins.begin(); i != mPlugins.end(); ++i)
	{
		const char* accountText = 0;
		PLUGINRET val = (*i)->lib->LoadAccount(accountName.text(), &accountText);
		ret.clear();
		ret << accountText;

		if (val == PLUGIN_STOP || val == PLUGIN_STOP_EAT)
			return ret;
	}
	return ret;
}

bool CPluginManager::SaveAccount(const CString& accountName, const CString& accountText)
{
	bool eat = false;
	for (std::vector<plugin_t *>::iterator i = mPlugins.begin(); i != mPlugins.end(); ++i)
	{
		PLUGINRET ret = (*i)->lib->SaveAccount(accountName.text(), accountText.text());
		if (ret == PLUGIN_CONTINUE_EAT || ret == PLUGIN_STOP_EAT) eat = true;
		if (ret == PLUGIN_STOP || ret == PLUGIN_STOP_EAT)
			return eat;
	}
	return eat;
}
