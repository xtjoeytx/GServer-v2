#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <unordered_map>
#include "CScriptEngine.h"
#include "TNPC.h"
#include "TServer.h"

#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

void Environment_ReportException(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1)

	V8ENV_D("Begin Environment::reportException()\n");

	if (args[0]->IsString())
	{
		v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();

		// Unwrap Object
		TServer *serverObject = UnwrapObject<TServer>(args.This());

		// Message
		CString message = *v8::String::Utf8Value(isolate, args[0]->ToString(isolate));

		// Report exception to NC / file
		serverObject->sendToNC(message);
		serverObject->getScriptLog().out(message << "\n");
	}

	V8ENV_D("End Environment::reportException()\n\n");
}

void Environment_SetCallBack(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 2)

	V8ENV_D("Begin Environment::setCallBack()\n");

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

	V8ENV_D("End Environment::setCallBack()\n\n");
}

void Environment_SetNpcEvents(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 2)

	V8ENV_D("Begin Environment::setNpcEvents()\n");

	if (args[0]->IsObject() && args[1]->IsInt32())
	{
		v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
		v8::Local<v8::Object> obj = args[0]->ToObject(context).ToLocalChecked();

		std::string npcConstructor = *v8::String::Utf8Value(isolate, obj->GetConstructorName());
		if (npcConstructor == "npc")
		{
			TNPC *npcObject = UnwrapObject<TNPC>(obj);
			npcObject->setScriptEvents(args[1]->Int32Value(context).ToChecked());
		}
	}

	V8ENV_D("End Environment::setNpcEvents()\n\n");
}

void bindClass_Environment(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "Environment"
	v8::Local<v8::String> envStr = v8::String::NewFromUtf8(isolate, "Environment", v8::NewStringType::kInternalized).ToLocalChecked();

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> environment_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> environment_proto = environment_ctor->PrototypeTemplate();
	environment_ctor->SetClassName(envStr);
	environment_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	environment_proto->Set(v8::String::NewFromUtf8(isolate, "reportException"), v8::FunctionTemplate::New(isolate, Environment_ReportException, engine_ref));
	environment_proto->Set(v8::String::NewFromUtf8(isolate, "setCallBack"), v8::FunctionTemplate::New(isolate, Environment_SetCallBack, engine_ref));
	environment_proto->Set(v8::String::NewFromUtf8(isolate, "setNpcEvents"), v8::FunctionTemplate::New(isolate, Environment_SetNpcEvents, engine_ref));

	// Persist the constructor
	env->SetConstructor(ScriptConstructorId<IScriptEnv>::result, environment_ctor);
}

#endif
