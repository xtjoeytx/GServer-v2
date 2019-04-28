#pragma once

#ifndef CSCRIPTENGINE_H
#define CSCRIPTENGINE_H

#include <cassert>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "ScriptAction.h"
#include "ScriptFactory.h"
#include "ScriptWrapped.h"

class IScriptEnv;
class IScriptFunction;

class TNPC;
class TServer;

class CScriptEngine
{
public:
	CScriptEngine(TServer *server);
	~CScriptEngine();

	bool Initialize();
	void Cleanup();
	void RunScripts(bool timedCall = false);

	inline TServer * getServer() const {
		return _server;
	}

	inline IScriptEnv * getScriptEnv() const {
		return _env;
	}

	//
	inline void UnregisterNpcTimer(TNPC *npc) {
		_updateNpcsTimer.erase(npc);
	}

	inline void RegisterNpcTimer(TNPC *npc) {
		_updateNpcsTimer.insert(npc);
	}

	//
	inline std::unordered_set<TNPC *> * GetNpcUpdateList() {
		return &_updateNpcs;
	}

	inline void UnregisterNpcUpdate(TNPC *npc) {
		_updateNpcs.erase(npc);
	}

	inline void RegisterNpcUpdate(TNPC *npc) {
		_updateNpcs.insert(npc);
	}

	inline IScriptFunction * getCallBack(const std::string& callback) const {
		auto it = _callbacks.find(callback);
		if (it != _callbacks.end())
			return it->second;

		return 0;
	}

	inline void setCallBack(const std::string& callback, IScriptFunction *cbFunc) {
		_callbacks[callback] = cbFunc;
	}

	// Script Compile / Cache
	IScriptFunction * CompileCache(const std::string& code);
	bool ClearCache(const std::string& code);

	bool CreatePlayer(TPlayer *player);
	bool ExecuteNpc(TNPC *npc);
	inline std::string WrapNPCScript(const std::string& code) const;

	// TODO(joey): Need to figure out how I should make this generic
	template<class... Args>
	ScriptAction * CreateAction(const std::string& action, Args... An);

protected:
	std::unordered_map<std::string, IScriptFunction *> _cachedScripts;
	std::unordered_map<std::string, IScriptFunction *> _callbacks;
	std::unordered_set<TNPC *> _updateNpcs;
	std::unordered_set<TNPC *> _updateNpcsTimer;

private:
	IScriptEnv *_env;
	IScriptFunction *_bootstrapFunction;
	IScriptWrapped<TServer> *_serverObject;
	TServer *_server;
};

std::string CScriptEngine::WrapNPCScript(const std::string& code) const
{
	static const char *prefixNpcString = "(function(npc) {" \
		"var onCreated, onPlayerEnters, onPlayerLeaves, onPlayerTouchsMe, onTimeout;" \
		"const self = npc;" \
		"self.onCreated = onCreated;" \
		"self.onPlayerEnters = onPlayerEnters;" \
		"self.onPlayerLeaves = onPlayerLeaves;" \
		"self.onPlayerTouchsMe = onPlayerTouchsMe;" \
		"self.onTimeout = onTimeout;";

	std::string wrappedCode = std::string(prefixNpcString);
	wrappedCode.append(code);
	wrappedCode.append("});");
	return wrappedCode;
}

template<class... Args>
ScriptAction * CScriptEngine::CreateAction(const std::string& action, Args... An)
{
	// TODO(joey): This just creates an action, and leaves it up to the user to do something with this. Most-likely will be renamed, or changed.
	constexpr size_t Argc = (sizeof...(Args));
	assert(Argc > 0);

	V8ENV_D("Server_RegisterAction:\n");
	V8ENV_D("\tAction: %s\n", action.c_str());
	V8ENV_D("\tArguments: %zu\n", Argc);

	auto funcIt = _callbacks.find(action);
	if (funcIt == _callbacks.end())
	{
		V8ENV_D("Global::Server_RegisterAction: Callback not registered for %s\n", action.c_str());
		return 0;
	}

	// total temp
	V8ScriptEnv *_env = 0;

	IScriptArguments *args = ScriptArgumentsFactory::Create(_env, std::forward<Args>(An)...);

	ScriptAction *newScriptAction = new ScriptAction(funcIt->second, args, action);
	return newScriptAction;
}

#endif
