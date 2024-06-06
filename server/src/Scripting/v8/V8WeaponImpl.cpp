#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <unordered_map>
#include "CScriptEngine.h"
#include "Weapon.h"

#include "V8ScriptFunction.h"
#include "V8ScriptObject.h"

// PROPERTY: Name
void Weapon_GetStr_Name(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Local<v8::Object> self = info.This();
	TWeapon *weaponObject = UnwrapObject<TWeapon>(self);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), weaponObject->getName().c_str()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

// PROPERTY: Image
void Weapon_GetStr_Image(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Local<v8::Object> self = info.This();
	TWeapon *weaponObject = UnwrapObject<TWeapon>(self);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), weaponObject->getImage().c_str()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void bindClass_Weapon(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();

	// External pointer
	// v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "weapon"
	v8::Local<v8::String> weaponStr = v8::String::NewFromUtf8Literal(isolate, "weapon", v8::NewStringType::kInternalized);

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> weapon_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> weapon_proto = weapon_ctor->PrototypeTemplate();
	weapon_ctor->SetClassName(weaponStr);
	weapon_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	//weapon_proto->Set(v8::String::NewFromUtf8(isolate, "setCallBack"), v8::FunctionTemplate::New(isolate, Weapon_SetCallBack, engine_ref));

	// Properties
	weapon_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "name"), Weapon_GetStr_Name);
	weapon_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "image"), Weapon_GetStr_Image);

	// Persist the constructor
	env->SetConstructor(ScriptConstructorId<TWeapon>::result, weapon_ctor);
}

#endif
