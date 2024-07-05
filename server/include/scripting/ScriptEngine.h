#ifndef CSCRIPTENGINE_H
#define CSCRIPTENGINE_H

#include <atomic>
#include <cassert>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "scripting/ScriptAction.h"
#include "scripting/ScriptFactory.h"
#include "scripting/SourceCode.h"
#include "scripting/interface/ScriptBindings.h"

#ifdef V8NPCSERVER
	#include "scripting/v8/V8ScriptWrappers.h"
#endif

class IScriptEnv;
class IScriptFunction;

class NPC;
class Server;
class Weapon;

class ScriptEngine
{
public:
	ScriptEngine(Server* server);
	~ScriptEngine();

	bool initialize();
	void cleanup(bool shutDown = false);
	void runScripts(const std::chrono::high_resolution_clock::time_point& time);

	void scriptWatcher();
	void startScriptExecution(const std::chrono::high_resolution_clock::time_point& startTime);
	bool stopScriptExecution();

	Server* getServer() const;
	IScriptEnv* getScriptEnv() const;
	IScriptObject<Server>* getServerObject() const;

	bool executeNpc(NPC* npc);
	bool executeWeapon(Weapon* weapon);

	void registerNpcTimer(NPC* npc);
	void registerNpcUpdate(NPC* npc);
	void registerWeaponUpdate(Weapon* weapon);

	void unregisterNpcTimer(NPC* npc);
	void unregisterNpcUpdate(NPC* npc);
	void unregisterWeaponUpdate(Weapon* weapon);

	// callbacks
	IScriptFunction* getCallBack(const std::string& callback) const;
	void removeCallBack(const std::string& callback);
	void setCallBack(const std::string& callback, IScriptFunction* cbFunc);

	// Compile script into a ScriptFunction
	IScriptFunction* compileCache(const std::string& code, bool referenceCount = true);

	// Clear cache for code
	bool clearCache(const std::string& code);

	// Clear cache for code, with a WrapperScript of Type T
	template<typename T>
	bool clearCache(const std::string& code);

	template<typename T>
	bool clearCache(const std::string_view& code);

	template<class... Args>
	ScriptAction createAction(const std::string& action, Args... An);

	template<class T>
	void wrapScriptObject(T* obj) const;

	const ScriptRunError& getScriptError() const;

	void reportScriptException(const ScriptRunError& error);
	void reportScriptException(const std::string& error_message);

private:
	void runTimers(const std::chrono::high_resolution_clock::time_point& time);

	IScriptEnv* m_env;
	IScriptFunction* m_bootstrapFunction;
	std::unique_ptr<IScriptObject<Server>> m_environmentObject;
	std::unique_ptr<IScriptObject<Server>> m_serverObject;
	Server* m_server;

	std::chrono::high_resolution_clock::time_point m_lastScriptTimer;
	std::chrono::nanoseconds m_accumulator;

	// Script watcher
	std::atomic<bool> m_scriptIsRunning;
	std::atomic<bool> m_scriptWatcherRunning;
	std::chrono::high_resolution_clock::time_point m_scriptStartTime;
	std::mutex m_scriptWatcherLock;
	std::thread m_scriptWatcherThread;

	std::unordered_map<std::string, IScriptFunction*> m_cachedScripts;
	std::unordered_map<std::string, IScriptFunction*> m_callbacks;
	std::unordered_set<NPC*> m_updateNpcs;
	std::unordered_set<NPC*> m_updateNpcsTimer;
	std::unordered_set<Weapon*> m_updateWeapons;
	std::unordered_set<IScriptFunction*> m_deletedCallbacks;
};

inline void ScriptEngine::startScriptExecution(const std::chrono::high_resolution_clock::time_point& startTime)
{
	{
		std::lock_guard<std::mutex> guard(m_scriptWatcherLock);
		m_scriptStartTime = startTime;
	}
	m_scriptIsRunning.store(true);
}

inline bool ScriptEngine::stopScriptExecution()
{
	bool res = m_scriptIsRunning.load();
	if (res)
		m_scriptIsRunning.store(false);
	return res;
}

// Getters

inline Server* ScriptEngine::getServer() const
{
	return m_server;
}

inline IScriptEnv* ScriptEngine::getScriptEnv() const
{
	return m_env;
}

inline IScriptObject<Server>* ScriptEngine::getServerObject() const
{
	return m_serverObject.get();
}

inline IScriptFunction* ScriptEngine::getCallBack(const std::string& callback) const
{
	auto it = m_callbacks.find(callback);
	if (it != m_callbacks.end())
		return it->second;

	return nullptr;
}

inline const ScriptRunError& ScriptEngine::getScriptError() const
{
	return m_env->getScriptError();
}

// Register scripts for processing

inline void ScriptEngine::registerNpcTimer(NPC* npc)
{
	m_updateNpcsTimer.insert(npc);
}

inline void ScriptEngine::registerNpcUpdate(NPC* npc)
{
	m_updateNpcs.insert(npc);
}

inline void ScriptEngine::registerWeaponUpdate(Weapon* weapon)
{
	m_updateWeapons.insert(weapon);
}

// Unregister scripts from processing

inline void ScriptEngine::unregisterWeaponUpdate(Weapon* weapon)
{
	m_updateWeapons.erase(weapon);
}

inline void ScriptEngine::unregisterNpcUpdate(NPC* npc)
{
	m_updateNpcs.erase(npc);
}

inline void ScriptEngine::unregisterNpcTimer(NPC* npc)
{
	m_updateNpcsTimer.erase(npc);
}

//

template<class... Args>
ScriptAction ScriptEngine::createAction(const std::string& action, Args... An)
{
	constexpr size_t Argc = (sizeof...(Args));
	assert(Argc > 0);

	SCRIPTENV_D("Server_RegisterAction:\n");
	SCRIPTENV_D("\tAction: %s\n", action.c_str());
	SCRIPTENV_D("\tArguments: %zu\n", Argc);

	auto funcIt = m_callbacks.find(action);
	if (funcIt == m_callbacks.end())
	{
		SCRIPTENV_D("Global::Server_RegisterAction: Callback not registered for %s\n", action.c_str());
		return ScriptAction{};
	}

	// Create an arguments object, and pass it to ScriptAction
	IScriptArguments* args = ScriptFactory::createArguments(m_env, std::forward<Args>(An)...);
	assert(args);

	return ScriptAction(funcIt->second, args, action);
}

template<class T>
inline void ScriptEngine::wrapScriptObject(T* obj) const
{
	SCRIPTENV_D("Begin Global::wrapScriptObject()\n");

	// Wrap the object, and set the new script object on the original object
	auto wrappedObject = ScriptFactory::wrapObject(m_env, ScriptConstructorId<T>::result, obj);
	obj->setScriptObject(std::move(wrappedObject));

	SCRIPTENV_D("End Global::wrapScriptObject()\n\n");
}

template<typename T>
inline bool ScriptEngine::clearCache(const std::string& code)
{
	return clearCache(wrapScript<T>(code));
}

template<typename T>
inline bool ScriptEngine::clearCache(const std::string_view& code)
{
	return clearCache(wrapScript<T>(code));
}

#endif
