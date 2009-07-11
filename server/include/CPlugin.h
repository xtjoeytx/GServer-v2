#ifndef GRAALRC_H
#define GRAALRC_H

#include <stddef.h>
#include "CString.h"

// Class Definitions
class TServer;

// Class: Plugin
class CPlugin
{
	public:
		// Functions -> Initiate
		virtual bool Initialize(TServer *pServer) = 0;

		// Functions -> Account Load/Save
		virtual void OnStart() = 0;

		//virtual TAccount * loadAccount(const CString& pAccount = "defaultaccount") = 0;
		//virtual bool saveAccount(TAccount *pAccount) = 0;

	protected:
		TServer *mServer;
};

struct plugin_t
{
	CPlugin		*lib;
	const char  *name;
	const char	*description;
	const char	*author;
	const char	*version;
};

typedef plugin_t *(*getplugin_t)(void);

/**
 * @brief Macro used by plugins to define the GetPluginI() method.
 *
 * The GetPluginI() method enables the plugin manager to get the plugin information.
 */
#define DEF_PLUGIN( PLUGIN, NAME, DESC, AUTHOR, VER ) \
	static plugin_t *g__plugin = NULL; \
	extern "C" __declspec(dllexport) plugin_t *GetPluginI(void) \
	{ \
		if ( g__plugin == NULL ) \
		{ \
			g__plugin = new plugin_t(); \
			g__plugin->lib = new PLUGIN(); \
			g__plugin->name = NAME; \
			g__plugin->description = DESC; \
			g__plugin->author = AUTHOR; \
			g__plugin->version = VER; \
		} \
		return g__plugin; \
	}

#define SELF g__plugin

#endif
