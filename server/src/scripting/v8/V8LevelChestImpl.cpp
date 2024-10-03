#ifdef V8NPCSERVER

	#include <algorithm>
	#include <cassert>
	#include <math.h>
	#include <unordered_map>
	#include <v8.h>

	#include "object/NPC.h"
	#include "object/Player.h"
	#include "level/Level.h"
	#include "level/LevelChest.h"
	#include "level/Map.h"
	#include "scripting/ScriptEngine.h"
	#include "scripting/v8/V8ScriptFunction.h"
	#include "scripting/v8/V8ScriptObject.h"

// PROPERTY: chest.x
void Chest_GetNum_X(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelChest, chestObject);

	info.GetReturnValue().Set(chestObject->getX());
}

void Chest_SetNum_X(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelChest, chestObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	chestObject->setX(newValue);
}

// PROPERTY: chest.y
void Chest_GetNum_Y(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelChest, chestObject);

	info.GetReturnValue().Set(chestObject->getY());
}

void Chest_SetNum_Y(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelChest, chestObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	chestObject->setY(newValue);
}

// PROPERTY: chest.itemtype
void Chest_GetNum_ItemType(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelChest, chestObject);

	info.GetReturnValue().Set((int)chestObject->getItemIndex());
}

void Chest_SetNum_ItemType(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelChest, chestObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	chestObject->setItemIndex(newValue);
}

// PROPERTY: chest.signid
void Chest_GetNum_SignId(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelChest, chestObject);

	info.GetReturnValue().Set((int)chestObject->getSignIndex());
}

void Chest_SetNum_SignId(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelChest, chestObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	chestObject->setSignIndex(newValue);
}

void bindClass_LevelChest(ScriptEngine* scriptEngine)
{
	// Retrieve v8 environment
	auto* env = dynamic_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());
	v8::Isolate* isolate = env->isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "level"
	v8::Local<v8::String> chestStr = v8::String::NewFromUtf8Literal(isolate, "chest", v8::NewStringType::kInternalized);

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> chest_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> chest_proto = chest_ctor->PrototypeTemplate();
	chest_ctor->SetClassName(chestStr);
	chest_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	//link_proto->Set(v8::String::NewFromUtf8Literal(isolate, "clone"), v8::FunctionTemplate::New(isolate, Level_Function_Clone, engine_ref));

	// Properties
	chest_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "x"), Chest_GetNum_X, Chest_SetNum_X);
	chest_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "y"), Chest_GetNum_Y, Chest_SetNum_Y);
	chest_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "itemtype"), Chest_GetNum_ItemType, Chest_SetNum_ItemType);
	chest_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "signid"), Chest_GetNum_SignId, Chest_SetNum_SignId);

	// Persist the constructor
	env->setConstructor(ScriptConstructorId<LevelChest>::result, chest_ctor);
}

#endif
