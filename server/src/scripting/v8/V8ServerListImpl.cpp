#ifdef V8NPCSERVER

	#include <cassert>
	#include <httplib.h>
	#include <v8.h>

	#include "CString.h"
	#include "NPC.h"
	#include "Player.h"
	#include "scripting/ScriptEngine.h"
	#include "scripting/v8/V8ScriptFunction.h"

CString HandleServerTextObject(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& object)
{
	CString ret;
	if (object->IsString())
	{
		ret << CString(*v8::String::Utf8Value(isolate, object->ToString(context).ToLocalChecked())).gtokenizeI() << "\n";
	}
	else if (object->IsArray())
	{
		const auto subArray = object.As<v8::Array>();
		auto subText = CString("");
		for (int j = 0; j < subArray->Length(); j++)
		{
			v8::Local<v8::Value> item;
			if (!subArray->Get(context, j).ToLocal(&item)) continue;

			subText << HandleServerTextObject(isolate, context, item);
		}
		ret << subText.gtokenizeI() << "\n";
	}

	return ret;
}

void ServerList_Function_SendText(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	auto* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 1);

	const auto context = args.GetIsolate()->GetCurrentContext();

	auto serverText = CString("");
	for (int i = 0; i < args.Length(); i++)
	{
		serverText << HandleServerTextObject(isolate, context, args[i]);
	}

	if (!serverText.isEmpty())
	{
		V8ENV_SAFE_UNWRAP(args, ServerList, serverListObject);
		serverListObject->sendText(serverText.gtokenizeI());
	}
}

void bindClass_ServerList(ScriptEngine* scriptEngine)
{
	// Retrieve v8 environment
	auto* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());
	auto* isolate = env->isolate();

	// External pointer
	const auto engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "server"
	const auto serverListStr = v8::String::NewFromUtf8Literal(isolate, "serverlist", v8::NewStringType::kInternalized);

	// Create constructor for class
	const auto serverList_ctor = v8::FunctionTemplate::New(isolate);
	const auto serverList_proto = serverList_ctor->PrototypeTemplate();

	serverList_ctor->SetClassName(serverListStr);
	serverList_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	serverList_proto->Set(v8::String::NewFromUtf8Literal(isolate, "sendtext"), v8::FunctionTemplate::New(isolate, ServerList_Function_SendText, engine_ref));

	// Properties


	// Persist the constructor
	env->setConstructor("serverlist", serverList_ctor);
}

#endif
