//#include <cassert>
//#include <v8.h>
//#include <stdio.h>
//#include "main.h"
//#include "Player.h"
//#include "Server.h"
//
//#include "V8ScriptFunction.h"
//#include "V8ScriptWrapped.h"
//
//V8ENV_ACCESSOR_NUM(Player, x)
//V8ENV_ACCESSOR_NUM(Player, y)
//V8ENV_ACCESSOR_INT32(Player, player_id)
//
//// Called when javascript creates a new object
//// js example: let jsNpc = new NPC();
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
//
//void bindClass_Player(ScriptEngine *scriptEngine)
//{
//	// Retrieve v8 environment
//	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
//	v8::Isolate *isolate = env->Isolate();
//
//	// External pointer
//	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);
//
//	// Create V8 string for "npc"
//	v8::Local<v8::String> className = v8::String::NewFromUtf8(isolate, "player");
//	
//	// Create constructor for class
//	v8::Local<v8::FunctionTemplate> ctor = v8::FunctionTemplate::New(isolate, Player_Constructor);
//	v8::Local<v8::ObjectTemplate> proto = ctor->PrototypeTemplate();
//	
//	ctor->SetClassName(className);
//	ctor->InstanceTemplate()->SetInternalFieldCount(1);
//	
//	// Static functions on the npc object
//	//ctor->Set(v8::String::NewFromUtf8(isolate, "create"), v8::FunctionTemplate::New(isolate, Npc_createFunction));
//	
//	// Method functions
//	//ctor->Set(v8::String::NewFromUtf8(isolate, "test"), v8::FunctionTemplate::New(isolate, Npc_testFunction));
//	
//	// Properties...?
//	proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), Player_GetInt32_player_id, Player_SetInt32_player_id);
//	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "timeout"), NPC_GetNum_timeout, NPC_SetNum_timeout);
//	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "x"), NPC_GetNum_x, NPC_SetNum_x);
//	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "y"), NPC_GetNum_y, NPC_SetNum_y);
//	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "hearts"), NPC_GetInt32_power, NPC_SetInt32_power);
//	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "maxhearts"), NPC_GetInt32_maxpower, NPC_SetInt32_maxpower);
//	//proto->SetAccessor(v8::String::NewFromUtf8(isolate, "nick"), NPC::NPC_GetStr_nickname, NPC::NPC_SetStr_nickname);
//	
//	// Persist the npc constructor
//	env->SetConstructor(ScriptConstructorId<Player>::result, ctor);
//
//	// Set the npc constructor on the global object
//	v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();
//	global->Set(className, ctor);
//}
//
