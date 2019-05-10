#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <unordered_map>
#include "CScriptEngine.h"
#include "TLevel.h"

#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

// PROPERTY: Name
void Level_GetStr_Name(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Local<v8::Object> self = info.This();
	TLevel *levelObject = UnwrapObject<TLevel>(self);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), levelObject->getLevelName().text());
	info.GetReturnValue().Set(strText);
}

void bindClass_Level(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "level"
	v8::Local<v8::String> levelStr = v8::String::NewFromUtf8(isolate, "level", v8::NewStringType::kInternalized).ToLocalChecked();

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> level_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> level_proto = level_ctor->PrototypeTemplate();
	level_ctor->SetClassName(levelStr);
	level_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	//level_proto->Set(v8::String::NewFromUtf8(isolate, "setCallBack"), v8::FunctionTemplate::New(isolate, Level_SetCallBack, engine_ref));

	// Properties
	// TODO(joey): implement
	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "name"), Level_GetStr_Name);
	//level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "npcs"), Level_GetStr_Name);
	//level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "players"), Level_GetStr_Name);

	// Persist the constructor
	env->SetConstructor(ScriptConstructorId<TLevel>::result, level_ctor);
}

#endif
