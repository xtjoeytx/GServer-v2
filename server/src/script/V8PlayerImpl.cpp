#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include "IUtil.h"
#include "CScriptEngine.h"
#include "TPlayer.h"
#include "TServer.h"

#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

// PROPERTY: Id
void Player_GetInt_Id(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	info.GetReturnValue().Set(playerObject->getId());
}

// PROPERTY: Nickname
void Player_GetStr_Nickname(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	
	CString propValue = playerObject->getNickname();;

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text());
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_Nickname(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	playerObject->setProps(CString() >> (char)PLPROP_NICKNAME >> (char)newValue.length() << *newValue, true, true);
}

// Called when javascript creates a new object
// js example: let jsNpc = new NPC();
//void Player_Constructor(const v8::FunctionCallbackInfo<v8::Value>& args)
//{
//	v8::Isolate *isolate = args.GetIsolate();
//	v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//	// Throw an exception on method functions for constructor calls
//	V8ENV_THROW_METHOD(args, isolate);
//	
//	// Called by V8. and should return an NPC object
//	V8ENV_D("Player_Constructor called\n");
//	
//	Player *newPlayer = new Player();
//	newPlayer->x = args[0]->NumberValue(context).ToChecked();
//	newPlayer->y = args[1]->NumberValue(context).ToChecked();
//	
//	V8ENV_D("\tTest Pos: %f, %f\n", newPlayer->x, newPlayer->y);
//	assert(args.This()->InternalFieldCount() > 0);
//	
//	args.This()->SetAlignedPointerInInternalField(0, newPlayer);
//	args.GetReturnValue().Set(args.This());
//}

void bindClass_Player(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "player"
	v8::Local<v8::String> className = v8::String::NewFromUtf8(isolate, "player", v8::NewStringType::kInternalized).ToLocalChecked();
	
	// Create constructor for class
	v8::Local<v8::FunctionTemplate> ctor = v8::FunctionTemplate::New(isolate, nullptr, engine_ref); // , Player_Constructor);
	v8::Local<v8::ObjectTemplate> proto = ctor->PrototypeTemplate();
	
	ctor->SetClassName(className);
	ctor->InstanceTemplate()->SetInternalFieldCount(1);
	
	// Static functions on the npc object
	//ctor->Set(v8::String::NewFromUtf8(isolate, "create"), v8::FunctionTemplate::New(isolate, Npc_createFunction));
	
	// Method functions
	//ctor->Set(v8::String::NewFromUtf8(isolate, "test"), v8::FunctionTemplate::New(isolate, Npc_testFunction));
	
	// Properties...?
	proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), Player_GetInt_Id);
	proto->SetAccessor(v8::String::NewFromUtf8(isolate, "nick"), Player_GetStr_Nickname, Player_SetStr_Nickname);
	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "timeout"), NPC_GetNum_timeout, NPC_SetNum_timeout);
	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "x"), NPC_GetNum_x, NPC_SetNum_x);
	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "y"), NPC_GetNum_y, NPC_SetNum_y);
	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "hearts"), NPC_GetInt32_power, NPC_SetInt32_power);
	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "maxhearts"), NPC_GetInt32_maxpower, NPC_SetInt32_maxpower);
	
	// Persist the npc constructor
	env->SetConstructor(ScriptConstructorId<TPlayer>::result, ctor);

	// Set the npc constructor on the global object
	v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();
	global->Set(className, ctor);
}

#endif
