#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <cstdio>
#include <unordered_map>
#include "CScriptEngine.h"
#include "V8ScriptFunction.h"
#include "V8ScriptObject.h"

#include "TLevel.h"
#include "TNPC.h"
#include "TPlayer.h"

void Server_Function_FindLevel(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	TServer *serverObject = UnwrapObject<TServer>(args.This());

	// Find level from user input
	if (args[0]->IsString())
	{
		v8::String::Utf8Value levelName(isolate, args[0]->ToString(context).ToLocalChecked());
		TLevel *levelObject = serverObject->getLevel(*levelName);

		if (levelObject != nullptr)
		{
			V8ScriptObject<TLevel> *v8_wrapped = static_cast<V8ScriptObject<TLevel> *>(levelObject->getScriptObject());
			args.GetReturnValue().Set(v8_wrapped->Handle(isolate));
		}
	}
}

void Server_Function_CreateLevel(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	TServer *serverObject = UnwrapObject<TServer>(args.This());

	// Create level from user input
	if (args[0]->IsInt32() && args[1]->IsString())
	{
		short fillTile = args[0]->Uint32Value(context).ToChecked();
		std::string levelName = *v8::String::Utf8Value(isolate, args[1]->ToString(context).ToLocalChecked());

		TLevel *levelObject = TLevel::createLevel(serverObject, fillTile, levelName);

		if (levelObject != nullptr)
		{
			V8ScriptObject<TLevel> *v8_wrapped = static_cast<V8ScriptObject<TLevel> *>(levelObject->getScriptObject());
			args.GetReturnValue().Set(v8_wrapped->Handle(isolate));
		}
	}
}

void Server_Function_FindNPC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	TServer *serverObject = UnwrapObject<TServer>(args.This());

	// Find npc object from user input
	TNPC *npcObject = nullptr;
	if (args[0]->IsString())
	{
		v8::String::Utf8Value npcName(isolate, args[0]->ToString(context).ToLocalChecked());
		npcObject = serverObject->getNPCByName(*npcName);
	}
	else if (args[0]->IsInt32())
	{
		unsigned int npcId = args[0]->Uint32Value(context).ToChecked();
		npcObject = serverObject->getNPC(npcId);
	}

	// Set the return value as the handle from the wrapped object
	if (npcObject != nullptr)
	{
		V8ScriptObject<TNPC> *v8_wrapped = static_cast<V8ScriptObject<TNPC>*>(npcObject->getScriptObject());
		args.GetReturnValue().Set(v8_wrapped->Handle(isolate));
	}
}

void Server_Function_FindPlayer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// TODO(joey): second parameter could indicticate if it should skip rcs?

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	TServer *serverObject = UnwrapObject<TServer>(args.This());

	// Find player object from user input
	TPlayer *playerObject = nullptr;
	if (args[0]->IsString())
	{
		v8::String::Utf8Value accountName(isolate, args[0]->ToString(context).ToLocalChecked());
		playerObject = serverObject->getPlayer(*accountName, PLTYPE_ANYCLIENT);
	}
	else if (args[0]->IsInt32())
	{
		unsigned int playerId = args[0]->Uint32Value(context).ToChecked();
		playerObject = serverObject->getPlayer(playerId, PLTYPE_ANYPLAYER);
	}

	// Set the return value as the handle from the wrapped object
	if (playerObject != nullptr)
	{
		V8ScriptObject<TPlayer> *v8_wrapped = static_cast<V8ScriptObject<TPlayer>*>(playerObject->getScriptObject());
		args.GetReturnValue().Set(v8_wrapped->Handle(isolate));
	}
}

void Server_Function_SaveLog(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	if (args[0]->IsString() && args[1]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TServer, serverObject);

		v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
		v8::String::Utf8Value filename(isolate, args[0]->ToString(context).ToLocalChecked());
		v8::String::Utf8Value message(isolate, args[1]->ToString(context).ToLocalChecked());

		serverObject->logToFile(*filename, *message);
	}
}

void Server_Function_SendToNC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();

	std::string msg;
	for (int i = 0; i < args.Length(); i++)
	{
		v8::String::Utf8Value str(isolate, args[i]->ToString(context).ToLocalChecked());
		msg.append(*str).append(" ");
	}

	if (!msg.empty())
	{
		V8ENV_SAFE_UNWRAP(args, TServer, serverObject);
		serverObject->sendToNC(msg);
	}
}

void Server_Function_SendToRC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();

	std::string msg;
	for (int i = 0; i < args.Length(); i++)
	{
		v8::String::Utf8Value str(isolate, args[i]->ToString(context).ToLocalChecked());
		msg.append(" ").append(*str);
	}

	if (!msg.empty())
	{
		V8ENV_SAFE_UNWRAP(args, TServer, serverObject);
		serverObject->sendToRC(CString("[Server]:") << msg);
	}
}

// PROPERTY: server.timevar
void Server_Get_TimeVar(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TServer, serverObject);

	unsigned int timevar = serverObject->getNWTime();
	info.GetReturnValue().Set(timevar);
}

// PROPERTY: server.timevar2
void Server_Get_TimeVar2(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	unsigned int timevar = (unsigned int)time(0);
	info.GetReturnValue().Set(timevar);
}

// PROPERTY: Server Flags
void Server_GetObject_Flags(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Isolate *isolate = info.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Object> self = info.This();

    v8::Local<v8::String> internalProperty = v8::String::NewFromUtf8Literal(isolate, "_internalFlags", v8::NewStringType::kInternalized);
    if (self->HasRealNamedProperty(context, internalProperty).ToChecked())
    {
        info.GetReturnValue().Set(self->Get(context, internalProperty).ToLocalChecked());
        return;
    }

	V8ENV_SAFE_UNWRAP(info, TServer, serverObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("server.flags");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
    v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
    new_instance->SetAlignedPointerInInternalField(0, serverObject);

    v8::PropertyAttribute propAttr = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
    self->DefineOwnProperty(context, internalProperty, new_instance, propAttr).FromJust();
    info.GetReturnValue().Set(new_instance);
}

void Server_Flags_Getter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.This();
	TServer *serverObject = UnwrapObject<TServer>(self);

	// Get property name
    v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get server flag with the property
	CString flagValue = serverObject->getFlag(*utf8);
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, flagValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Server_Flags_Setter(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.This();
	TServer *serverObject = UnwrapObject<TServer>(self);

	// Get property name
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get new value
	v8::String::Utf8Value newValue(isolate, value);
	serverObject->setFlag(*utf8, *newValue, true);
}

void Server_Flags_Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	TServer *serverObject = UnwrapObject<TServer>(self);

	// Get flags list
	auto flagList = serverObject->getServerFlags();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)flagList->size());

	int idx = 0;
	for (auto it = flagList->begin(); it != flagList->end(); ++it)
		result->Set(context, idx++, v8::String::NewFromUtf8(isolate, it->first.c_str()).ToLocalChecked()).Check();

	info.GetReturnValue().Set(result);
}

// PROPERTY: server.npcs
void Server_GetArray_Npcs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	TServer *serverObject = UnwrapObject<TServer>(self);

	// Get npcs list
	auto npcList = serverObject->getNPCList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)npcList->size());

	int idx = 0;
	for (auto it = npcList->begin(); it != npcList->end(); ++it) {
		V8ScriptObject<TNPC> *v8_wrapped = static_cast<V8ScriptObject<TNPC> *>((*it)->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	info.GetReturnValue().Set(result);
}

// PROPERTY: server.players
void Server_GetArray_Players(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	TServer *serverObject = UnwrapObject<TServer>(self);

	// Get npcs list
	auto playerList = serverObject->getPlayerList();

	v8::Local<v8::Array> result = v8::Array::New(isolate);

	int idx = 0;
	for (auto it = playerList->begin(); it != playerList->end(); ++it) {
		TPlayer *pl = *it;
		if (pl->isHiddenClient())
			continue;

		V8ScriptObject<TPlayer> *v8_wrapped = static_cast<V8ScriptObject<TPlayer> *>((*it)->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	info.GetReturnValue().Set(result);
}

// PROPERTY: server.serverlist
void Server_GetArray_Serverlist(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	TServer *serverObject = UnwrapObject<TServer>(self);

	// Get npcs list
	auto listserver = serverObject->getServerList();
	auto serverList = listserver->getServerList();

	v8::Local<v8::Object> result = v8::Object::New(isolate);

	for (auto it = serverList.begin(); it != serverList.end(); ++it) {
		v8::Local<v8::String> key_servername = v8::String::NewFromUtf8(isolate, it->first.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
		result->Set(context, key_servername, v8::Number::New(isolate, it->second)).Check();
	}

	info.GetReturnValue().Set(result);
}

void bindClass_Server(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "server"
	v8::Local<v8::String> serverStr = v8::String::NewFromUtf8Literal(isolate, "server", v8::NewStringType::kInternalized);

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> server_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> server_proto = server_ctor->PrototypeTemplate();

	server_ctor->SetClassName(serverStr);
	server_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "findlevel"), v8::FunctionTemplate::New(isolate, Server_Function_FindLevel, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "createlevel"), v8::FunctionTemplate::New(isolate, Server_Function_CreateLevel, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "findnpc"), v8::FunctionTemplate::New(isolate, Server_Function_FindNPC, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "findplayer"), v8::FunctionTemplate::New(isolate, Server_Function_FindPlayer, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "savelog"), v8::FunctionTemplate::New(isolate, Server_Function_SaveLog, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "sendtonc"), v8::FunctionTemplate::New(isolate, Server_Function_SendToNC, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8Literal(isolate, "sendtorc"), v8::FunctionTemplate::New(isolate, Server_Function_SendToRC, engine_ref));

	// Properties
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "flags"), Server_GetObject_Flags, nullptr, engine_ref);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "npcs"), Server_GetArray_Npcs);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "players"), Server_GetArray_Players);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "serverlist"), Server_GetArray_Serverlist);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "timevar"), Server_Get_TimeVar);
	server_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "timevar2"), Server_Get_TimeVar2);

    // Create the server flags template
	v8::Local<v8::FunctionTemplate> server_flags_ctor = v8::FunctionTemplate::New(isolate);
	server_flags_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "flags"));
	server_flags_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	server_flags_ctor->InstanceTemplate()->SetHandler(v8::NamedPropertyHandlerConfiguration(
			Server_Flags_Getter, Server_Flags_Setter, nullptr, nullptr, Server_Flags_Enumerator, v8::Local<v8::Value>(),
			v8::PropertyHandlerFlags::kOnlyInterceptStrings));
	env->SetConstructor("server.flags", server_flags_ctor);

	// Persist the constructor
	env->SetConstructor("server", server_ctor);
}

#endif
