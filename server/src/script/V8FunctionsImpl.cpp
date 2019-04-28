#ifdef V8NPCSERVER

#include <string>
#include <unordered_map>
#include "V8ScriptEnv.h"
#include "V8ScriptFunction.h"
#include "TPlayer.h"
#include "TNPC.h"
#include "CScriptEngine.h"

void Ext_TestFunc(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	if (args.Length() < 3)
	{
		isolate->ThrowException(v8::String::NewFromUtf8(isolate, "Not enough arguments."));
		return;
	}

	// MAY NOT NEED THIS
	v8::HandleScope handle_scope(isolate);

	v8::Local<v8::External> data = args.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

	v8::String::Utf8Value newNick(isolate, args[0]);

	TPlayer *pl = env->Unwrap<TPlayer>(args[1]);
	TNPC *npc = env->Unwrap<TNPC>(args[2]);

	if (pl == 0 || npc == 0)
	{
		printf("Error, received null for player or npc object!\n");
		return;
	}

	char *nickname = npc->getProp(NPCPROP_NICKNAME).remove(0, 1).text();

	printf("Player Id: %d\n", pl->getId());
	printf("Npc Nickname: %s\n", nickname);
	printf("set nickname to %s\n", *newNick);

	npc->setProps(CString() >> (char)newNick.length() << *newNick);
}

void Ext_PrintMessage(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	//V8ENV_D("Begin Global::Print()\n\t");

	// Print call
	bool first = true;
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope(isolate);
		if (first) {
			first = false;
		}
		else {
			printf(" ");
		}

		v8::String::Utf8Value str(isolate, args[i]);
		printf("%s", *str);
	}
	printf("\n");
	fflush(stdout);

	//V8ENV_D("End Global::Print()\n\n");
}

void bindGlobalFunctions(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);
	
	// Fetch global template
	v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();
	
	// Bind functions
	global->Set(v8::String::NewFromUtf8(isolate, "print"), v8::FunctionTemplate::New(isolate, Ext_PrintMessage));
	global->Set(v8::String::NewFromUtf8(isolate, "testFunc"), v8::FunctionTemplate::New(isolate, Ext_TestFunc, engine_ref));
}

#endif
