#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <unordered_map>
#include "CScriptEngine.h"
#include "TNPC.h"

#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

// TODO(joey): Currently not cleaning this up
v8::Persistent<v8::FunctionTemplate> _persist_server_flags_ctor;

void Server_SendToNC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1)

	if (args[0]->IsString())
	{
		v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
		v8::String::Utf8Value message(isolate, args[0]->ToString(context).ToLocalChecked());

		TServer *serverObject = UnwrapObject<TServer>(args.This());
		serverObject->sendToNC(CString("[NPC-Server]: ") << *message);
	}
}

void Server_SendToRC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1)

	if (args[0]->IsString())
	{
		v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
		v8::String::Utf8Value message(isolate, args[0]->ToString(context).ToLocalChecked());

		TServer *serverObject = UnwrapObject<TServer>(args.This());
		serverObject->sendToRC(CString("[Server]: ") << *message);
	}
}

// PROPERTY: Server Flags
void Server_GetObject_Flags(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Isolate *isolate = info.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Object> self = info.This();
    TServer *serverObject = UnwrapObject<TServer>(self);

    v8::Local<v8::String> internalProperty = v8::String::NewFromUtf8(isolate, "_internalFlags", v8::NewStringType::kInternalized).ToLocalChecked();
    if (self->HasRealNamedProperty(context, internalProperty).ToChecked())
    {
        info.GetReturnValue().Set(self->Get(context, internalProperty).ToLocalChecked());
        return;
    }

    v8::Local<v8::FunctionTemplate> ctor_tpl = PersistentToLocal(isolate, _persist_server_flags_ctor);
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
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, flagValue.text());
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

	v8::Local<v8::Array> result = v8::Array::New(isolate, flagList->size());

	int idx = 0;
	for (auto it = flagList->begin(); it != flagList->end(); ++it)
		result->Set(context, idx++, v8::String::NewFromUtf8(isolate, it->first.c_str())).Check();

	info.GetReturnValue().Set(result);
}

// PROPERTY: Server Npcs
void Server_GetArray_Npcs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	TServer *serverObject = UnwrapObject<TServer>(self);

	// Get npcs list
	auto npcList = serverObject->getNPCList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, npcList->size());

	int idx = 0;
	for (auto it = npcList->begin(); it != npcList->end(); ++it) {
		V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>((*it)->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
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
	v8::Local<v8::String> serverStr = v8::String::NewFromUtf8(isolate, "server", v8::NewStringType::kInternalized).ToLocalChecked();

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> server_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> server_proto = server_ctor->PrototypeTemplate();

	server_ctor->SetClassName(serverStr);
	server_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	server_proto->Set(v8::String::NewFromUtf8(isolate, "sendtonc"), v8::FunctionTemplate::New(isolate, Server_SendToNC, engine_ref));
	server_proto->Set(v8::String::NewFromUtf8(isolate, "sendtorc"), v8::FunctionTemplate::New(isolate, Server_SendToRC, engine_ref));

	// Properties
	server_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "flags"), Server_GetObject_Flags);
	server_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "npcs"), Server_GetArray_Npcs);

    // Create the server flags template
	v8::Local<v8::FunctionTemplate> server_flags_ctor = v8::FunctionTemplate::New(isolate);
	server_flags_ctor->SetClassName(v8::String::NewFromUtf8(isolate, "flags"));
	server_flags_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	server_flags_ctor->InstanceTemplate()->SetHandler(v8::NamedPropertyHandlerConfiguration(
			Server_Flags_Getter, Server_Flags_Setter, nullptr, nullptr, Server_Flags_Enumerator, v8::Local<v8::Value>(),
			v8::PropertyHandlerFlags::kOnlyInterceptStrings));
	_persist_server_flags_ctor.Reset(isolate, server_flags_ctor);

	// Properties...?
	//server_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), NPC_GetInt32_npc_id, NPC_SetInt32_npc_id);

	// Persist the npc constructor
	env->SetConstructor(ScriptConstructorId<TServer>::result, server_ctor);
}

#endif
