#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <unordered_map>
#include "TNPC.h"
#include "CScriptEngine.h"

#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

// Run macros
//V8ENV_ACCESSOR_NUM(NPC, timeout)
//V8ENV_ACCESSOR_NUM(TNPC, x)
//V8ENV_ACCESSOR_NUM(TNPC, y)

//V8ENV_ACCESSOR_INT32(TNPC, npc_id)
//V8ENV_ACCESSOR_INT32(TNPC, power)
//V8ENV_ACCESSOR_INT32(TNPC, maxpower)

//V8ENV_ACCESSOR_STRING(NPC, nickname)

void NPC_GetNum_timeout(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	void *ptr = self->GetAlignedPointerFromInternalField(0);
	TNPC *wrap = static_cast<TNPC *>(ptr);

	double timeout = wrap->timeout / 20;
	info.GetReturnValue().Set(timeout);
}

void NPC_SetNum_timeout(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	void *ptr = self->GetAlignedPointerFromInternalField(0);
	TNPC *wrap = static_cast<TNPC *>(ptr);
	
	double timeout = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	wrap->timeout = static_cast<int>(timeout * 20);
}

void NPC_GetStr_nickname(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	void *ptr = self->GetAlignedPointerFromInternalField(0);
	TNPC *wrap = static_cast<TNPC *>(ptr);

	char *nickname = wrap->getProp(NPCPROP_NICKNAME).remove(0, 1).text();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), nickname);
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_nickname(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	void *ptr = self->GetAlignedPointerFromInternalField(0);
	TNPC *wrap = static_cast<TNPC *>(ptr);

	v8::String::Utf8Value newValue = v8::String::Utf8Value(info.GetIsolate(), value);
	wrap->setProps(CString() >> (char)newValue.length() << *newValue);
}

// Called when javascript creates a new object
// js example: let jsNpc = new NPC();
//void Npc_Constructor(const v8::FunctionCallbackInfo<v8::Value>& args)
//{
//	// TODO(joey): more proof of concept, likely to get axed.
//	
//	// Called by V8. and should return an NPC object
//	V8ENV_D("Npc_Constructor called\n");
//
//	v8::Isolate *isolate = args.GetIsolate();
//	v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//	// Throw an exception on method functions for constructor calls
//	V8ENV_THROW_METHOD(args, isolate);
//	
//	// Retrieve external data for this call
//	v8::Local<v8::External> data = args.Data().As<v8::External>();
//	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
//	
//	TNPC *newNpc = new TNPC(scriptEngine->getServer());
//
//	assert(args.This()->InternalFieldCount() > 0);
//	
//	args.This()->SetAlignedPointerInInternalField(0, newNpc);
//	args.GetReturnValue().Set(args.This());
//}

// NPC Method: npc.test();
void Npc_testFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();
	
	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Retrieve external data for this call
	v8::Local<v8::External> data = args.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	
	// Check if the callback exists
	V8ScriptFunction *callback = static_cast<V8ScriptFunction *>(scriptEngine->getCallBack("onTest"));
	if (callback != 0)
	{
//		V8ScriptEnv *env = static_cast<V8ScriptEnv *>(server->getScriptEnv());
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Function> cbFunc = callback->Function();
		
		// Call callback
		v8::Local<v8::Value> newArgs[] = {
			args.This(),
			args[0]
		};
		v8::MaybeLocal<v8::Value> ret = cbFunc->Call(context, Null(isolate), 2, newArgs);
		if (!ret.IsEmpty())
			V8ENV_D(" - Returned Value from Callback: %s\n", *(v8::String::Utf8Value(isolate, ret.ToLocalChecked())));
	}

	V8ENV_D("Npc::testFunction: %s\n", *v8::String::Utf8Value(isolate, args[0]->ToString(isolate)));
}

// NPC Static Method: NPC::create();
void Npc_createFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();
	
	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	
	// Retrieve v8 environment
	v8::Local<v8::External> data = args.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Function> cbFunc = (static_cast<V8ScriptFunction *>(scriptEngine->getCallBack("onTest")))->Function();
	
	v8::Local<v8::ObjectTemplate> objTemplate = env->GetConstructor(ScriptConstructorId<NPC>::result)->InstanceTemplate();
	v8::Local<v8::Value> arg = objTemplate->NewInstance(context).ToLocalChecked();
	auto ret = cbFunc->Call(context, Null(isolate), 1, &arg);
	if (!ret.IsEmpty())
		V8ENV_D(" - EXT_Create_NPC Return: %s\n", *(v8::String::Utf8Value(isolate, ret.ToLocalChecked())));

	args.GetReturnValue().Set(arg);

	V8ENV_D("Npc::createFunction: %s\n", *v8::String::Utf8Value(isolate, args[0]->ToString(isolate)));
}

void bindClass_NPC(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();
	
	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "npc"
	v8::Local<v8::String> npcStr = v8::String::NewFromUtf8(isolate, "npc", v8::NewStringType::kInternalized).ToLocalChecked();
	
	// Create constructor for class
	v8::Local<v8::FunctionTemplate> npc_ctor = v8::FunctionTemplate::New(isolate, nullptr, engine_ref);
	v8::Local<v8::ObjectTemplate> npc_proto = npc_ctor->PrototypeTemplate();
	
	npc_ctor->SetClassName(npcStr);
	npc_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	
	// Static functions on the npc object
	//npc_ctor->Set(v8::String::NewFromUtf8(isolate, "create"), v8::FunctionTemplate::New(isolate, Npc_createFunction, engine_ref));
	
	// Method functions
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "test"), v8::FunctionTemplate::New(isolate, Npc_testFunction, engine_ref));
	
	// Properties...?
	//npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), NPC_GetInt32_npc_id, NPC_SetInt32_npc_id);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "timeout"), NPC_GetNum_timeout, NPC_SetNum_timeout);
	//npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "x"), NPC_GetNum_x, NPC_SetNum_x);
	//npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "y"), NPC_GetNum_y, NPC_SetNum_y);
	//npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "hearts"), NPC_GetInt32_power, NPC_SetInt32_power);
	//npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "maxhearts"), NPC_GetInt32_maxpower, NPC_SetInt32_maxpower);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "nick"), NPC_GetStr_nickname, NPC_SetStr_nickname);
	
	// Persist the npc constructor
	env->SetConstructor(ScriptConstructorId<NPC>::result, npc_ctor);

	// DISABLED: it would just allow scripts to construct npcs, better off disabled?
	// Set the npc constructor on the global object
	//v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();
	//global->Set(npcStr, npc_ctor);
}
