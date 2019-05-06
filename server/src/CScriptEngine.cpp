#ifdef V8NPCSERVER

#include "CScriptEngine.h"
#include "V8ScriptEnv.h"
#include "V8ScriptWrapped.h"
#include "TNPC.h"
#include "TPlayer.h"
#include "TServer.h"

extern void bindGlobalFunctions(CScriptEngine *scriptEngine);
extern void bindClass_NPC(CScriptEngine *scriptEngine);
extern void bindClass_Player(CScriptEngine *scriptEngine);
extern void bindClass_Server(CScriptEngine *scriptEngine);

CScriptEngine::CScriptEngine(TServer *server)
	: _server(server), _env(nullptr), _bootstrapFunction(0), _serverObject(0)
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
	V8ENV_D("---START SCRIPT---\n%s\n---END SCRIPT\n\n", bootstrapScript.text());

	// TODO(joey): Clean this the fuck up
	_env = new V8ScriptEnv();

	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(_env);
	env->Initialize();

	{
		// TODO(joey): get this out of here? possibly?
		// Fetch the v8 isolate, and create a stack-allocated scope for v8 calls
		v8::Isolate *isolate = env->Isolate();
		v8::Isolate::Scope isolate_scope(isolate);
		v8::HandleScope handle_scope(isolate);

		// Bind global functions
		bindGlobalFunctions(this);

		// Bind classes to be used for scripts
		bindClass_Server(this);
		bindClass_NPC(this);
		bindClass_Player(this);

		// Create a new context (occurs on initial compile)
		_bootstrapFunction = env->Compile("bootstrap", bootstrapScript.text());
		assert(_bootstrapFunction);

		v8::Context::Scope context_scope(env->Context());
		_serverObject = env->Wrap(ScriptConstructorId<TServer>::result, this->_server);

		IScriptArguments *args = ScriptArgumentsFactory::Create(env, _serverObject);
		args->Invoke(_bootstrapFunction);
		delete args;
	}

	return true;
}

void CScriptEngine::Cleanup()
{
	if (!_env) {
		return;
	}

	for (auto it = _callbacks.begin(); it != _callbacks.end(); ++it) {
		delete it->second;
	}
	_callbacks.clear();

	if (_bootstrapFunction) {
		delete _bootstrapFunction;
	}

	if (_serverObject) {
		delete _serverObject;
	}

	_env->Cleanup();
	delete _env;
	_env = 0;
}

IScriptFunction * CScriptEngine::CompileCache(const std::string& code)
{
	// TODO(joey): Temporary naming conventions, maybe pass an optional reference to an object which holds info for the compiler (name, ignore wrap code based off spaces/lines, and execution results?)
	static int SCRIPT_ID = 1;

	auto scriptFunctionIter = _cachedScripts.find(code);
	if (scriptFunctionIter != _cachedScripts.end())
	{
		scriptFunctionIter->second->increaseReference();
		return scriptFunctionIter->second;
	}

	// Compile script, handle errors
	IScriptFunction *compiledScript = _env->Compile(std::to_string(SCRIPT_ID++), code);
	if (compiledScript == nullptr)
	{
		// TODO(joey): Delete? Let the caller handle the errors
		auto error = _env->getScriptError();
		error.DebugPrint();
		return nullptr;
	}

	compiledScript->increaseReference();
	_cachedScripts[code] = compiledScript;

	V8ENV_D("---COMPILED---\n");
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

	// TODO(joey): All this ScriptRunError is temporary, will likely make a member variable that holds the last script error.
	V8ENV_D("Begin Global::ExecuteNPC()\n\n");
	

	// We always want to create an object for the npc
	// Wrap object
	IScriptWrapped<TNPC> *wrappedObject = WrapObject(npc);

	// Wrap user code in a function-object, returning some useful symbols to call for events
	CString npcScript = npc->getServerScript();
	//if (npcScript.isEmpty)
	std::string codeStr = WrapScript<TNPC>(npcScript.text());

	V8ENV_D("---START SCRIPT---\n%s\n---END SCRIPT\n\n", codeStr.c_str());

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
		V8ENV_D("Failed when executing script\n");
		if (try_catch.HasCaught())
		{
			v8::Handle<v8::Message> message = try_catch.Message();
			ScriptRunError scriptError;
			scriptError.filename  = *v8::String::Utf8Value(isolate, message->GetScriptResourceName());
			scriptError.error_line = *v8::String::Utf8Value(isolate, message->GetSourceLine(context).ToLocalChecked());
			V8ENV_D("Error Line: %s\n", scriptError.error_line.c_str());
			return false;
		}
	}

	V8ENV_D("End Global::ExecuteNPC()\n\n");
	return true;
}

void CScriptEngine::RunScripts(bool timedCall)
{
	if (timedCall && !_updateNpcsTimer.empty())
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

	if (!_updateNpcs.empty())
	{
		V8ScriptEnv *env = static_cast<V8ScriptEnv *>(_env);

		// Fetch the v8 isolate, and create a stack-allocated scope for v8 calls
		v8::Isolate *isolate = env->Isolate();
		v8::Isolate::Scope isolate_scope(isolate);
		v8::HandleScope handle_scope(isolate);

		// Enter context scope
		v8::Context::Scope context_scope(env->Context());

		// Iterate over npcs
		for (auto it = _updateNpcs.begin(); it != _updateNpcs.end(); ++it)
		{
			TNPC *npc = *it;
			npc->runScriptEvents();
		}
		_updateNpcs.clear();
	}
}

#endif
