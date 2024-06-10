#ifdef V8NPCSERVER

	#include "ScriptEngine.h"
	#include "V8ScriptEnv.h"
	#include "V8ScriptFunction.h"
	#include <string>
	#include <unordered_map>

// Global Method: print(arg0, arg1, arg2, arg3) [no format atm];
void Global_Function_Print(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Print call
	for (int i = 0; i < args.Length(); i++)
	{
		v8::HandleScope handle_scope(isolate);
		if (i > 0)
			printf(" ");

		v8::String::Utf8Value str(isolate, args[i]);
		printf("%s", *str);
	}
	printf("\n");
	fflush(stdout);
}

// PROPERTY: server object
void Global_GetObject_Server(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	CScriptEngine* scriptEngine = static_cast<CScriptEngine*>(data->Value());

	V8ScriptObject<Server>* v8_serverObject = static_cast<V8ScriptObject<Server>*>(scriptEngine->getServerObject());
	info.GetReturnValue().Set(v8_serverObject->Handle(info.GetIsolate()));
}

void bindGlobalFunctions(CScriptEngine* scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());
	v8::Isolate* isolate = env->Isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Fetch global template
	v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();

	// Global functions
	global->Set(v8::String::NewFromUtf8Literal(isolate, "print"), v8::FunctionTemplate::New(isolate, Global_Function_Print));
	//global->Set(v8::String::NewFromUtf8(isolate, "testFunc"), v8::FunctionTemplate::New(isolate, Ext_TestFunc, engine_ref));

	// Global properties
	global->Set(v8::String::NewFromUtf8Literal(isolate, "global"), v8::ObjectTemplate::New(isolate), static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete));
	global->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "server"), Global_GetObject_Server, nullptr, engine_ref);
}

#endif
