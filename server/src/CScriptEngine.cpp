#ifdef V8NPCSERVER

#include "CScriptEngine.h"
#include "V8ScriptBindings.h"
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
	: _server(server), _env(nullptr), _bootstrapFunction(0), _environmentObject(0), _serverObject(0)
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
	
	return true;
}

void CScriptEngine::Cleanup()
{
	if (!_env) {
		return;
	}

	_updateNpcs.clear();
	_updateNpcsTimer.clear();
	_updateWeapons.clear();

	// Remove any registered callbacks
	for (auto it = _callbacks.begin(); it != _callbacks.end(); ++it) {
		delete it->second;
	}
	_callbacks.clear();

	// Remove cached scripts
	for (auto it = _cachedScripts.begin(); it != _cachedScripts.end(); ++it) {
		delete it->second;
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
	_env->Cleanup();
	delete _env;
	_env = 0;
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
	IScriptFunction *compiledScript = _env->Compile(std::to_string(SCRIPT_ID++), code);
	if (compiledScript == nullptr)
	{
		_server->reportScriptException(_env->getScriptError());
		return nullptr;
	}

	if (referenceCount)
		compiledScript->increaseReference();
	_cachedScripts[code] = compiledScript;

	SCRIPTENV_D("---COMPILED---\n");
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
	// Wrap object
	IScriptWrapped<TNPC> *wrappedObject = WrapObject(npc);

	// Wrap user code in a function-object, returning some useful symbols to call for events
	CString npcScript = npc->getServerScript();
	if (npcScript.isEmpty())
    {
		SCRIPTENV_D("Empty code, maybe we shouldn't execute\n");
    }

	std::string codeStr = WrapScript<TNPC>(npcScript.text());

	SCRIPTENV_D("---START SCRIPT---\n%s\n---END SCRIPT\n\n", codeStr.c_str());

	// Search the cache, or compile the script
	IScriptFunction *compiledScript = CompileCache(codeStr);
	if (compiledScript == nullptr)
	{
		// script failed to execute
		return false;
	}

	//
	// Execute the compiled script
	//

	// TODO(joey): Get rid of v8 usage here

	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(_env);

	// Fetch the v8 isolate and context
	v8::Isolate *isolate = env->Isolate();
	v8::Local<v8::Context> context = env->Context();
	assert(!context.IsEmpty());

	// Create a stack-allocated scope for v8 calls, and enter context
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);
	v8::Context::Scope context_scope(context);

	// Cast object
	V8ScriptWrapped<TNPC> *v8_wrappedObject = static_cast<V8ScriptWrapped<TNPC> *>(wrappedObject);
	v8::Local<v8::Object> wrappedObjectHandle = v8_wrappedObject->Handle(isolate);

	// Arguments to call function
	v8::Local<v8::Value> scriptFunctionArgs[1] = {
		wrappedObjectHandle
	};

	// Execute the compiled script with the instance from the newly-wrapped object
	V8ScriptFunction *v8_function = static_cast<V8ScriptFunction *>(compiledScript);

	v8::TryCatch try_catch(isolate);
	v8::Local<v8::Function> scriptFunction = v8_function->Function();
	v8::MaybeLocal<v8::Value> scriptTableRet = scriptFunction->Call(context, wrappedObjectHandle, 1, scriptFunctionArgs);
	if (scriptTableRet.IsEmpty())
	{
		SCRIPTENV_D("Failed when executing script\n");
		env->ParseErrors(&try_catch);
		_server->reportScriptException(_env->getScriptError());
	}

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
	{
		SCRIPTENV_D("Empty code, maybe we shouldn't execute\n");
	}

	std::string codeStr = WrapScript<TWeapon>(weaponScript.text());

	SCRIPTENV_D("---START SCRIPT---\n%s\n---END SCRIPT\n\n", codeStr.c_str());

	// Search the cache, or compile the script
	IScriptFunction *compiledScript = CompileCache(codeStr);
	if (compiledScript == nullptr)
	{
		// script failed to execute
		return false;
	}

	//
	// Execute the compiled script
	//

	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(_env);

	// Fetch the v8 isolate and context
	v8::Isolate *isolate = env->Isolate();
	v8::Local<v8::Context> context = env->Context();
	assert(!context.IsEmpty());

	// Create a stack-allocated scope for v8 calls, and enter context
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);
	v8::Context::Scope context_scope(context);

	// Cast object
	V8ScriptWrapped<TWeapon> *v8_wrappedObject = static_cast<V8ScriptWrapped<TWeapon> *>(wrappedObject);
	v8::Local<v8::Object> wrappedObjectHandle = v8_wrappedObject->Handle(isolate);

	// Arguments to call function
	v8::Local<v8::Value> scriptFunctionArgs[1] = {
		wrappedObjectHandle
	};

	// Execute the compiled script with the instance from the newly-wrapped object
	V8ScriptFunction *v8_function = static_cast<V8ScriptFunction *>(compiledScript);

	v8::TryCatch try_catch(isolate);
	v8::Local<v8::Function> scriptFunction = v8_function->Function();
	v8::MaybeLocal<v8::Value> scriptTableRet = scriptFunction->Call(context, wrappedObjectHandle, 1, scriptFunctionArgs);
	if (scriptTableRet.IsEmpty())
	{
		SCRIPTENV_D("Failed when executing weapon script\n");
		env->ParseErrors(&try_catch);
		_server->reportScriptException(_env->getScriptError());
	}

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

			if (!hasUpdates) {
				it = _updateNpcsTimer.erase(it);
			}
			else it++;
		}
	}
	
	if (!_updateNpcs.empty() || !_updateWeapons.empty())
	{
		_env->CallFunctionInScope([&]() -> void {
			// Iterate over npcs
			for (auto it = _updateNpcs.begin(); it != _updateNpcs.end(); ++it)
			{
				TNPC *npc = *it;
				npc->runScriptEvents();
			}
			_updateNpcs.clear();

			// Iterate over weapons
			for (auto it = _updateWeapons.begin(); it != _updateWeapons.end(); ++it)
			{
				TWeapon *weapon = *it;
				weapon->runScriptEvents();
			}
			_updateWeapons.clear();
		});
	}
}

#endif
