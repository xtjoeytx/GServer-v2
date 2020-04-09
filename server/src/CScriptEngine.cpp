#ifdef V8NPCSERVER

#include "CScriptEngine.h"
#include "TNPC.h"
#include "TPlayer.h"
#include "TServer.h"
#include "TWeapon.h"

extern void bindGlobalFunctions(CScriptEngine *scriptEngine);
extern void bindClass_Environment(CScriptEngine *scriptEngine);
extern void bindClass_Level(CScriptEngine *scriptEngine);
extern void bindClass_NPC(CScriptEngine *scriptEngine);
extern void bindClass_Player(CScriptEngine *scriptEngine);
extern void bindClass_Server(CScriptEngine *scriptEngine);
extern void bindClass_Weapon(CScriptEngine *scriptEngine);

CScriptEngine::CScriptEngine(TServer *server)
	: _server(server), _env(nullptr), _bootstrapFunction(nullptr), _environmentObject(nullptr), _serverObject(nullptr)
	, _scriptIsRunning(false), _scriptWatcherRunning(false), _scriptWatcherThread()
{

}

CScriptEngine::~CScriptEngine()
{
	this->Cleanup();
}

bool CScriptEngine::Initialize()
{
	if (_env)
		return true;

	CString bootstrapScript;
	if (!bootstrapScript.load(CString() << _server->getServerPath() << "bootstrap.js"))
	{
		// Failed to load file
		return false;
	}

	// bootstrap file print
	SCRIPTENV_D("---START SCRIPT---\n%s\n---END SCRIPT\n\n", bootstrapScript.text());

	// TODO(joey): Clean this the fuck up
	_env = new V8ScriptEnv();
	_env->Initialize();

	_env->CallFunctionInScope([&]() -> void {
		CScriptEngine *engine = this;

		// Bind global functions
		bindGlobalFunctions(engine);

		// Bind classes to be used for scripts
		bindClass_Environment(engine);
		bindClass_Server(engine);
		bindClass_Level(engine);
		bindClass_NPC(engine);
		bindClass_Player(engine);
		bindClass_Weapon(engine);
	});

	// Create a new context (occurs on initial compile)
	_bootstrapFunction = _env->Compile("bootstrap", bootstrapScript.text());
	assert(_bootstrapFunction);

	// Bind the server into two separate objects
	_environmentObject = ScriptFactory::WrapObject(_env, "environment", _server);
	_serverObject = ScriptFactory::WrapObject(_env, "server", _server);

	// Execute the bootstrap function
	_env->CallFunctionInScope([&]() -> void {
		IScriptArguments *args = ScriptFactory::CreateArguments(_env, _environmentObject);
		args->Invoke(_bootstrapFunction);
		delete args;
	});

	_scriptWatcherRunning.store(true);
	_scriptWatcherThread = std::thread(&CScriptEngine::ScriptWatcher, this);

	return true;
}

void CScriptEngine::ScriptWatcher()
{
	const std::chrono::milliseconds sleepTime(50);

	while (_scriptWatcherRunning.load())
	{
		if (_scriptIsRunning.load())
		{
			auto time_now = std::chrono::high_resolution_clock::now();
			std::chrono::milliseconds time_diff;
			{
				std::lock_guard<std::mutex> guard(_scriptWatcherLock);
				time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(time_now - _scriptStartTime);
			}

			if (time_diff.count() >= 500) {
				_env->TerminateExecution();
				_scriptIsRunning.store(false);
				//printf("Killed execution for running too long!\n");
			}
			else if (time_diff.count() < 450)
				std::this_thread::sleep_for(sleepTime);
		}
		else std::this_thread::sleep_for(sleepTime);
	}
}

void CScriptEngine::Cleanup(bool shutDown)
{
	if (!_env) {
		return;
	}

	// Kill script watcher
	_scriptWatcherRunning.store(false);
	_scriptWatcherThread.join();

	// Clear any registered scripts
	_updateNpcs.clear();
	_updateNpcsTimer.clear();
	_updateWeapons.clear();

	// Remove any registered callbacks
	for (auto & _callback : _callbacks) {
		delete _callback.second;
	}
	_callbacks.clear();

	// Remove cached scripts
	for (auto & _cachedScript : _cachedScripts) {
		delete _cachedScript.second;
	}
	_cachedScripts.clear();

	// Remove bootstrap function
	if (_bootstrapFunction) {
		delete _bootstrapFunction;
		_bootstrapFunction = nullptr;
	}

	// Remove script objects
	if (_environmentObject) {
	    delete _environmentObject;
		_environmentObject = nullptr;
	}

	if (_serverObject) {
		delete _serverObject;
		_serverObject = nullptr;
	}

	// Cleanup the Script Environment
	_env->Cleanup(shutDown);

	// Destroy the environment
	delete _env;
	_env = nullptr;
}

IScriptFunction * CScriptEngine::CompileCache(const std::string& code, bool referenceCount)
{
	// TODO(joey): Temporary naming conventions, maybe pass an optional reference to an object which holds info for the compiler (name, ignore wrap code based off spaces/lines, and execution results?)
	static int SCRIPT_ID = 1;

	auto scriptFunctionIter = _cachedScripts.find(code);
	if (scriptFunctionIter != _cachedScripts.end())
	{
		if (referenceCount)
			scriptFunctionIter->second->increaseReference();
		return scriptFunctionIter->second;
	}

	// Compile script, send errors to server
	SCRIPTENV_D("Compiling script:\n---\n%s\n---\n", code.c_str());

	IScriptFunction *compiledScript = _env->Compile(std::to_string(SCRIPT_ID++), code);
	if (compiledScript == nullptr)
	{
		auto scriptError = _env->getScriptError();
		_server->reportScriptException(scriptError);
		SCRIPTENV_D("Error Compiling: %s\n", scriptError.getErrorString().c_str());
		return nullptr;
	}

	SCRIPTENV_D("Successfully Compiled\n");

	// Increase reference count to compiled script, and cache it.
	if (referenceCount)
		compiledScript->increaseReference();
	_cachedScripts[code] = compiledScript;
	return compiledScript;
}

bool CScriptEngine::ClearCache(const std::string& code)
{
	auto scriptFunctionIter = _cachedScripts.find(code);
	if (scriptFunctionIter == _cachedScripts.end())
		return false;

	IScriptFunction *scriptFunction = scriptFunctionIter->second;
	scriptFunction->decreaseReference();
	if (!scriptFunction->isReferenced())
	{
		_cachedScripts.erase(scriptFunctionIter);
		delete scriptFunction;
	}

	return true;
}

bool CScriptEngine::ExecuteNpc(TNPC *npc)
{
	SCRIPTENV_D("Begin Global::ExecuteNPC()\n\n");

	// We always want to create an object for the npc
	IScriptWrapped<TNPC> *wrappedObject = WrapObject(npc);

	// No script, nothing to execute.
	CString npcScript = npc->getServerScript();
	if (npcScript.isEmpty())
		return false;

	// Wrap user code in a function-object, returning some useful symbols to call for events
	std::string codeStr = WrapScript<TNPC>(npcScript.text());

	// Search the cache, or compile the script
	IScriptFunction *compiledScript = CompileCache(codeStr);

	// Script failed to compile
	if (compiledScript == nullptr)
		return false;

	//
	// Execute the compiled script
	//
	_env->CallFunctionInScope([&]() -> void {
		IScriptArguments *args = ScriptFactory::CreateArguments(_env, wrappedObject);
		bool result = args->Invoke(compiledScript, true);
		if (!result)
			_server->reportScriptException(_env->getScriptError());
		delete args;
	});

	SCRIPTENV_D("End Global::ExecuteNPC()\n\n");
	return true;
}

bool CScriptEngine::ExecuteWeapon(TWeapon *weapon)
{
	SCRIPTENV_D("Begin Global::ExecuteWeapon()\n\n");

	// We always want to create an object for the weapon
	// Wrap object
	IScriptWrapped<TWeapon> *wrappedObject = WrapObject(weapon);

	// Wrap user code in a function-object, returning some useful symbols to call for events
	CString weaponScript = weapon->getServerScript();
	if (weaponScript.isEmpty())
		return false;

	std::string codeStr = WrapScript<TWeapon>(weaponScript.text());

	// Search the cache, or compile the script
	IScriptFunction *compiledScript = CompileCache(codeStr);

	// Script failed to compile
	if (compiledScript == nullptr)
		return false;

	//
	// Execute the compiled script
	//
	_env->CallFunctionInScope([&]() -> void {
		IScriptArguments *args = ScriptFactory::CreateArguments(_env, wrappedObject);
		bool result = args->Invoke(compiledScript, true);
		if (!result)
			_server->reportScriptException(_env->getScriptError());
		delete args;
	});

	SCRIPTENV_D("End Global::ExecuteWeapon()\n\n");
	return true;
}

void CScriptEngine::RunScripts(bool timedCall)
{
	if (timedCall)
	{
		for (auto it = _updateNpcsTimer.begin(); it != _updateNpcsTimer.end(); )
		{
			TNPC *npc = *it;
			bool hasUpdates = npc->runScriptTimer();

			if (!hasUpdates)
				it = _updateNpcsTimer.erase(it);
			else
				it++;
		}
	}

	if (!_updateNpcs.empty() || !_updateWeapons.empty())
	{
		_env->CallFunctionInScope([&]() -> void {
			// Iterate over npcs
			for (auto it = _updateNpcs.begin(); it != _updateNpcs.end(); )
			{
				TNPC *npc = *it;
				bool hasActions = npc->runScriptEvents();

				if (!hasActions)
					it = _updateNpcs.erase(it);
				else
					it++;
			}

			// Iterate over weapons
			for (auto it = _updateWeapons.begin(); it != _updateWeapons.end(); ++it)
			{
				TWeapon *weapon = *it;
				weapon->runScriptEvents();
			}
			_updateWeapons.clear();
		});
	}

	// No actions are queued, so we can assume no functions are cached here. May need to invalidate functions?
	if (!_deletedFunctions.empty())
	{
		for (auto it = _deletedFunctions.begin(); it != _deletedFunctions.end();)
		{
			IScriptFunction *func = *it;
			if (!func->isReferenced())
			{
				delete func;
				it = _deletedFunctions.erase(it);
			}
			else ++it;
		}
	}
}

#endif
