#ifdef V8NPCSERVER

	#include "ScriptEngine.h"
	#include "EmbeddedBootstrapScript.h"
	#include "NPC.h"
	#include "Player.h"
	#include "Server.h"
	#include "V8ScriptWrappers.h"
	#include "Weapon.h"

extern const unsigned char JSBOOTSTRAPSCRIPT[];
extern const size_t JSBOOTSTRAPSCRIPT_SIZE;

extern void bindGlobalFunctions(ScriptEngine* scriptEngine);

extern void bindClass_Environment(ScriptEngine* scriptEngine);

extern void bindClass_Level(ScriptEngine* scriptEngine);

extern void bindClass_LevelLink(ScriptEngine* scriptEngine);

extern void bindClass_LevelSign(ScriptEngine* scriptEngine);

extern void bindClass_LevelChest(ScriptEngine* scriptEngine);

extern void bindClass_NPC(ScriptEngine* scriptEngine);

extern void bindClass_Player(ScriptEngine* scriptEngine);

extern void bindClass_Server(ScriptEngine* scriptEngine);

extern void bindClass_Weapon(ScriptEngine* scriptEngine);

ScriptEngine::ScriptEngine(Server* server)
	: m_server(server), m_env(nullptr), m_bootstrapFunction(nullptr), m_environmentObject(nullptr),
	  m_serverObject(nullptr), m_scriptIsRunning(false), m_scriptWatcherRunning(false), m_scriptWatcherThread()
{
	m_accumulator = std::chrono::nanoseconds(0);
	m_lastScriptTimer = std::chrono::high_resolution_clock::now();
}

ScriptEngine::~ScriptEngine()
{
	this->cleanup();
}

bool ScriptEngine::initialize()
{
	if (m_env)
		return true;

	CString bootstrapScript;
	bootstrapScript.write((const char*)JSBOOTSTRAPSCRIPT, JSBOOTSTRAPSCRIPT_SIZE);

	// bootstrap file print
	SCRIPTENV_D("---START SCRIPT---\n%s\n---END SCRIPT\n\n", bootstrapScript.text());

	// TODO(joey): Clean this the fuck up
	m_env = new V8ScriptEnv();
	m_env->initialize();

	m_env->callFunctionInScope([&]() -> void
							   {
								   ScriptEngine* engine = this;

								   // Bind global functions
								   bindGlobalFunctions(engine);

								   // Bind classes to be used for scripts
								   bindClass_Environment(engine);
								   bindClass_Server(engine);
								   bindClass_Level(engine);
								   bindClass_LevelLink(engine);
								   bindClass_LevelSign(engine);
								   bindClass_LevelChest(engine);
								   bindClass_NPC(engine);
								   bindClass_Player(engine);
								   bindClass_Weapon(engine);
							   });

	// Create a new context (occurs on initial compile)
	m_bootstrapFunction = m_env->compile("bootstrap", bootstrapScript.text());
	assert(m_bootstrapFunction);

	// Bind the server into two separate objects
	m_environmentObject = ScriptFactory::wrapObject(m_env, "environment", m_server);
	m_serverObject = ScriptFactory::wrapObject(m_env, "server", m_server);

	// Execute the bootstrap function
	m_env->callFunctionInScope([&]() -> void
							   {
								   IScriptArguments* args = ScriptFactory::createArguments(m_env, m_environmentObject.get());
								   args->invoke(m_bootstrapFunction);
								   delete args;
							   });

	m_scriptWatcherRunning.store(true);
	m_scriptWatcherThread = std::thread(&ScriptEngine::scriptWatcher, this);

	return true;
}

void ScriptEngine::scriptWatcher()
{
	const std::chrono::milliseconds sleepTime(50);

	while (m_scriptWatcherRunning.load())
	{
		if (m_scriptIsRunning.load())
		{
			auto time_now = std::chrono::high_resolution_clock::now();
			std::chrono::milliseconds time_diff;
			{
				std::lock_guard<std::mutex> guard(m_scriptWatcherLock);
				time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(time_now - m_scriptStartTime);
			}

			if (time_diff.count() >= 500)
			{
				m_env->terminateExecution();
				m_scriptIsRunning.store(false);
				//printf("Killed execution for running too long!\n");
			}
			else if (time_diff.count() < 450)
				std::this_thread::sleep_for(sleepTime);
		}
		else
			std::this_thread::sleep_for(sleepTime);
	}
}

void ScriptEngine::cleanup(bool shutDown)
{
	if (!m_env)
	{
		return;
	}

	// Kill script watcher
	m_scriptWatcherRunning.store(false);
	if (m_scriptWatcherThread.joinable())
		m_scriptWatcherThread.join();

	// Clear any registered scripts
	m_updateNpcs.clear();
	m_updateNpcsTimer.clear();
	m_updateWeapons.clear();

	// Remove any registered callbacks
	for (auto& _callback: m_callbacks)
	{
		delete _callback.second;
	}
	m_callbacks.clear();

	// Remove cached scripts
	for (auto& _cachedScript: m_cachedScripts)
	{
		delete _cachedScript.second;
	}
	m_cachedScripts.clear();

	// Remove bootstrap function
	if (m_bootstrapFunction)
	{
		delete m_bootstrapFunction;
		m_bootstrapFunction = nullptr;
	}

	// Remove script objects
	if (m_environmentObject)
	{
		m_environmentObject.reset();
	}

	if (m_serverObject)
	{
		m_serverObject.reset();
	}

	// Cleanup the Script Environment
	m_env->cleanup(shutDown);

	// Destroy the environment
	delete m_env;
	m_env = nullptr;
}

IScriptFunction* ScriptEngine::compileCache(const std::string& code, bool referenceCount)
{
	// TODO(joey): Temporary naming conventions, maybe pass an optional reference to an object which holds info for the compiler (name, ignore wrap code based off spaces/lines, and execution results?)
	static int SCRIPT_ID = 1;

	auto scriptFunctionIter = m_cachedScripts.find(code);
	if (scriptFunctionIter != m_cachedScripts.end())
	{
		if (referenceCount)
			scriptFunctionIter->second->increaseReference();
		return scriptFunctionIter->second;
	}

	// Compile script, send errors to server
	SCRIPTENV_D("Compiling script:\n---\n%s\n---\n", code.c_str());

	IScriptFunction* compiledScript = m_env->compile(std::to_string(SCRIPT_ID++), code);
	if (compiledScript == nullptr)
	{
		reportScriptException(m_env->getScriptError());
		SCRIPTENV_D("Error Compiling: %s\n", m_env->getScriptError().getErrorString().c_str());
		return nullptr;
	}

	SCRIPTENV_D("Successfully Compiled\n");

	// Increase reference count to compiled script, and cache it.
	if (referenceCount)
		compiledScript->increaseReference();
	m_cachedScripts[code] = compiledScript;
	return compiledScript;
}

bool ScriptEngine::clearCache(const std::string& code)
{
	auto scriptFunctionIter = m_cachedScripts.find(code);
	if (scriptFunctionIter == m_cachedScripts.end())
		return false;

	IScriptFunction* scriptFunction = scriptFunctionIter->second;
	scriptFunction->decreaseReference();
	if (!scriptFunction->isReferenced())
	{
		m_cachedScripts.erase(scriptFunctionIter);
		delete scriptFunction;
	}

	return true;
}

	#include "Level.h"

bool ScriptEngine::executeNpc(NPC* npc)
{
	SCRIPTENV_D("Begin Global::ExecuteNPC()\n\n");

	// We always want to create an object for the npc
	wrapScriptObject(npc);

	// No script, nothing to execute.
	auto npcScript = npc->getSource().getServerSide();
	if (npcScript.empty())
		return false;

	// Wrap user code in a function-object, returning some useful symbols to call for events
	std::string codeStr = wrapScript<NPC>(npcScript);

	// Search the cache, or compile the script
	IScriptFunction* compiledScript = compileCache(codeStr);

	// Script failed to compile
	if (compiledScript == nullptr)
		return false;

	//
	// Execute the compiled script
	//
	m_env->callFunctionInScope([&]() -> void
							   {
								   IScriptArguments* args = ScriptFactory::createArguments(m_env, npc->getScriptObject());
								   bool result = args->invoke(compiledScript, true);
								   if (!result)
								   {
									   auto level = npc->getLevel();
									   if (level)
									   {
										   std::string exceptionMsg("NPC Exception at ");

										   exceptionMsg.append(level->getLevelName().text());
										   exceptionMsg.append(",");
										   exceptionMsg.append(std::to_string(npc->getX() / 16.0));
										   exceptionMsg.append(",");
										   exceptionMsg.append(std::to_string(npc->getY() / 16.0));
										   exceptionMsg.append(": ");
										   if (!npc->getName().empty())
										   {
											   exceptionMsg.append(npc->getName());
											   exceptionMsg.append(" - ");
										   }
										   exceptionMsg.append(m_env->getScriptError().getErrorString());
										   reportScriptException(exceptionMsg);
									   }
									   else
										   reportScriptException(m_env->getScriptError());
								   }
								   delete args;
							   });

	SCRIPTENV_D("End Global::ExecuteNPC()\n\n");
	return true;
}

bool ScriptEngine::executeWeapon(Weapon* weapon)
{
	SCRIPTENV_D("Begin Global::ExecuteWeapon()\n\n");

	// We always want to create an object for the weapon
	wrapScriptObject(weapon);

	auto weaponScript = weapon->getServerScript();
	if (!weaponScript.empty())
	{
		// Wrap user code in a function-object, returning some useful symbols to call for events
		std::string codeStr = wrapScript<Weapon>(weaponScript);

		// Search the cache, or compile the script
		IScriptFunction* compiledScript = compileCache(codeStr);

		// Script failed to compile
		if (compiledScript == nullptr)
			return false;

		//
		// Execute the compiled script
		//
		m_env->callFunctionInScope([&]() -> void
								   {
									   IScriptArguments* args = ScriptFactory::createArguments(m_env, weapon->getScriptObject());
									   bool result = args->invoke(compiledScript, true);
									   if (!result)
										   reportScriptException(m_env->getScriptError());

									   delete args;
								   });
	}

	SCRIPTENV_D("End Global::ExecuteWeapon()\n\n");
	return true;
}

void ScriptEngine::runTimers(const std::chrono::high_resolution_clock::time_point& time)
{
	auto delta_time = time - m_lastScriptTimer;
	m_lastScriptTimer = time;

	// Run scripts every 0.05 seconds
	constexpr std::chrono::nanoseconds timestep(std::chrono::milliseconds(50));
	m_accumulator += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);
	while (m_accumulator >= timestep)
	{
		m_accumulator -= timestep;

		for (auto it = m_updateNpcsTimer.begin(); it != m_updateNpcsTimer.end();)
		{
			NPC* npc = *it;
			bool hasUpdates = npc->runScriptTimer();

			if (!hasUpdates)
				it = m_updateNpcsTimer.erase(it);
			else
				it++;
		}
	}
}

void ScriptEngine::runScripts(const std::chrono::high_resolution_clock::time_point& time)
{
	runTimers(time);

	if (!m_updateNpcs.empty() || !m_updateWeapons.empty())
	{
		m_env->callFunctionInScope([&]() -> void
								   {
									   std::map<int, NPC*> deleteNpcs;

									   // Iterate over npcs
									   for (auto it = m_updateNpcs.begin(); it != m_updateNpcs.end();)
									   {
										   NPC* npc = *it;
										   auto response = npc->runScriptEvents();

										   if (response == NPCEventResponse::PendingEvents)
										   {
											   it++;
											   continue;
										   }

										   if (response == NPCEventResponse::Delete)
											   deleteNpcs.emplace(npc->getId(), npc);

										   it = m_updateNpcs.erase(it);
									   }

									   // Iterate over weapons
									   for (auto weapon: m_updateWeapons)
										   weapon->runScriptEvents();
									   m_updateWeapons.clear();

									   // Delete any npcs
									   for (auto n: deleteNpcs)
										   m_server->deleteNPC(n.first);
								   });
	}

	// No actions are queued, so we can assume no functions are cached here.
	if (!m_deletedCallbacks.empty())
	{
		for (auto it = m_deletedCallbacks.begin(); it != m_deletedCallbacks.end();)
		{
			IScriptFunction* func = *it;
			if (!func->isReferenced())
			{
				delete func;
				it = m_deletedCallbacks.erase(it);
			}
			else
				++it;
		}
	}
}

void ScriptEngine::removeCallBack(const std::string& callback)
{
	auto it = m_callbacks.find(callback);
	if (it != m_callbacks.end())
	{
		it->second->decreaseReference();
		m_deletedCallbacks.insert(it->second);
		m_callbacks.erase(it);
	}
}

void ScriptEngine::setCallBack(const std::string& callback, IScriptFunction* cbFunc)
{
	removeCallBack(callback);

	m_callbacks[callback] = cbFunc;
	cbFunc->increaseReference();
}

void ScriptEngine::reportScriptException(const ScriptRunError& error)
{
	m_server->reportScriptException(error);
}

void ScriptEngine::reportScriptException(const std::string& error_message)
{
	m_server->reportScriptException(error_message);
}

#endif
