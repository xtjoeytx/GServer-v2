#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <unordered_map>
#include "main.h"
#include "CScriptEngine.h"
#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

// NPC Static Method: NPC::create();
void Server_createFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Retrieve v8 environment
//	v8::Local<v8::External> data = args.Data().As<v8::External>();
//	Server *server = static_cast<Server *>(data->Value());
//	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(server->getScriptEnv());

	//v8::Local<v8::Context> context = isolate->GetCurrentContext();
	//v8::Local<v8::Function> cbFunc = (static_cast<V8ScriptFunction *>(server->getCallBack("onTest")))->Function();

	//v8::Local<v8::ObjectTemplate> objTemplate = env->GetConstructor(ScriptConstructorId<NPC>::result)->InstanceTemplate();
	//v8::Local<v8::Value> arg = objTemplate->NewInstance();
	//auto ret = cbFunc->Call(context, Null(isolate), 1, &arg);
	//if (!ret.IsEmpty())
	//	V8ENV_D(" - EXT_Create_NPC Return: %s\n", *(v8::String::Utf8Value(isolate, ret.ToLocalChecked())));

	args.GetReturnValue().Set(v8::True(isolate));

	V8ENV_D("Server::createFunction: %s\n", *v8::String::Utf8Value(isolate, args[0]->ToString(isolate)));
}

void Server_SetCallBack(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();
	
	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	
	V8ENV_D("Begin Server::setCallBack()\n");
	
	if (args[0]->IsString() && args[1]->IsFunction())
	{
		V8ENV_D(" - Set callback for %s with: %s\n",
				*v8::String::Utf8Value(isolate, args[0]->ToString(isolate)),
				*v8::String::Utf8Value(isolate, args[1]->ToString(isolate)));
		
		v8::Local<v8::External> data = args.Data().As<v8::External>();
		CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
		
		V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
		
		// Callback name
		std::string eventName = *v8::String::Utf8Value(isolate, args[0]->ToString(isolate));
		
		// Persist the callback function so we can retrieve it later on
		v8::Local<v8::Function> cbFunc = args[1].As<v8::Function>();
		V8ScriptFunction *cbFuncWrapper = new V8ScriptFunction(env, cbFunc);
		
		scriptEngine->setCallBack(eventName, cbFuncWrapper);
	}
	
	V8ENV_D("End Server::setCallBack()\n\n");
}


void bindClass_Server(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "server"
	v8::Local<v8::String> serverStr = v8::String::NewFromUtf8(isolate, "server", v8::NewStringType::kInternalized).ToLocalChecked();

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> server_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> server_proto = server_ctor->PrototypeTemplate();

	server_ctor->SetClassName(serverStr);
	server_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	server_proto->Set(v8::String::NewFromUtf8(isolate, "createmethod"), v8::FunctionTemplate::New(isolate, Server_createFunction, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8(isolate, "setCallBack"), v8::FunctionTemplate::New(isolate, Server_SetCallBack, engine_ref));
	
	// Properties...?
	//server_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), NPC_GetInt32_npc_id, NPC_SetInt32_npc_id);

	// Persist the npc constructor
	env->SetConstructor(ScriptConstructorId<Server>::result, server_ctor);
}

#endif
