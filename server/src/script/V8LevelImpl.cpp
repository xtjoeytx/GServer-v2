#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <unordered_map>
#include "CScriptEngine.h"
#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

#include "TLevel.h"
#include "TNPC.h"

// PROPERTY: level.name
void Level_GetStr_Name(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Local<v8::Object> self = info.This();
	TLevel *levelObject = UnwrapObject<TLevel>(self);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), levelObject->getLevelName().text());
	info.GetReturnValue().Set(strText);
}

// PROPERTY: level.npcs
void Level_GetArray_Npcs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	TLevel *levelObject = UnwrapObject<TLevel>(self);

	// Get npcs list
	auto npcList = levelObject->getLevelNPCs();

	v8::Local<v8::Array> result = v8::Array::New(isolate, npcList->size());

	int idx = 0;
	for (auto it = npcList->begin(); it != npcList->end(); ++it) {
		V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>((*it)->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	info.GetReturnValue().Set(result);
}

// Level Method: level.findareanpcs(x, y, width, height);
void Level_Function_FindAreaNpcs(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 4);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Argument parsing
	int startX = (int)(16 * args[0]->NumberValue(context).ToChecked());
	int startY = (int)(16 * args[1]->NumberValue(context).ToChecked());
	int endX = 16 * args[2]->Int32Value(context).ToChecked();
	int endY = 16 * args[3]->Int32Value(context).ToChecked();

	// Unwrap Object
	TLevel *levelObject = UnwrapObject<TLevel>(args.This());
	std::vector<TNPC *> npcList = levelObject->findAreaNpcs(startX, startY, endX, endY);

	// Create array of objects
	v8::Local<v8::Array> result = v8::Array::New(isolate, npcList.size());

	int idx = 0;
	for (auto it = npcList.begin(); it != npcList.end(); ++it)
	{
		V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>((*it)->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	args.GetReturnValue().Set(result);
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
	level_proto->Set(v8::String::NewFromUtf8(isolate, "findareanpcs"), v8::FunctionTemplate::New(isolate, Level_Function_FindAreaNpcs, engine_ref));

	// Properties
	// TODO(joey): implement
	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "name"), Level_GetStr_Name);
	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "npcs"), Level_GetArray_Npcs);
	//level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "players"), Level_GetStr_Name);

	// Persist the constructor
	env->SetConstructor(ScriptConstructorId<TLevel>::result, level_ctor);
}

#endif
