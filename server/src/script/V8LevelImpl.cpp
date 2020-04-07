#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <unordered_map>
#include "CScriptEngine.h"
#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

#include "TLevel.h"
#include "TNPC.h"
#include "TPlayer.h"

// PROPERTY: level.issparringzone
void Level_GetBool_IsSparringZone(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	info.GetReturnValue().Set(levelObject->isSparringZone());
}

// PROPERTY: level.name
void Level_GetStr_Name(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), levelObject->getLevelName().text());
	info.GetReturnValue().Set(strText);
}

// PROPERTY: level.npcs
void Level_GetArray_Npcs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Get npcs list
	auto npcList = levelObject->getLevelNPCs();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)npcList->size());

	int idx = 0;
	for (auto it = npcList->begin(); it != npcList->end(); ++it) {
		V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>((*it)->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	info.GetReturnValue().Set(result);
}

// PROPERTY: level.players
void Level_GetArray_Players(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Get npcs list
	auto playerList = levelObject->getPlayerList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)playerList->size());

	int idx = 0;
	for (auto it = playerList->begin(); it != playerList->end(); ++it) {
		V8ScriptWrapped<TPlayer> *v8_wrapped = static_cast<V8ScriptWrapped<TPlayer> *>((*it)->getScriptObject());
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

	// Unwrap Object
	V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Argument parsing
	int startX = (int)(16 * args[0]->NumberValue(context).ToChecked());
	int startY = (int)(16 * args[1]->NumberValue(context).ToChecked());
	int endX = 16 * args[2]->Int32Value(context).ToChecked();
	int endY = 16 * args[3]->Int32Value(context).ToChecked();

	std::vector<TNPC *> npcList = levelObject->findAreaNpcs(startX, startY, endX, endY);

	// Create array of objects
	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)npcList.size());

	int idx = 0;
	for (auto it = npcList.begin(); it != npcList.end(); ++it)
	{
		V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>((*it)->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	args.GetReturnValue().Set(result);
}

// Level Method: level.findnearestplayers(x, y);
void Level_Function_FindNearestPlayers(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber() && args[1]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		// Argument parsing
		float targetX = (float)args[0]->NumberValue(context).ToChecked();
		float targetY = (float)args[1]->NumberValue(context).ToChecked();

		// Get distance for each player in the level, and sort it
		std::vector<TPlayer *> *playerList = levelObject->getPlayerList();
		std::vector<std::pair<double, TPlayer *>> playerListSorted;

		for (auto it = playerList->begin(); it != playerList->end(); ++it)
		{
			TPlayer *pl = *it;
			double distance = sqrt(pow(pl->getY() - targetY, 2) + pow(pl->getX() - targetX, 2));
			playerListSorted.push_back({ distance, pl });
		}

		std::sort(playerListSorted.begin(), playerListSorted.end());

		// Create array of objects
		v8::Local<v8::String> key_distance = v8::String::NewFromUtf8(isolate, "distance", v8::NewStringType::kInternalized).ToLocalChecked();
		v8::Local<v8::String> key_player = v8::String::NewFromUtf8(isolate, "player", v8::NewStringType::kInternalized).ToLocalChecked();
		v8::Local<v8::Array> result = v8::Array::New(isolate, (int)playerListSorted.size());

		int idx = 0;
		for (auto it = playerListSorted.begin(); it != playerListSorted.end(); ++it)
		{
			V8ScriptWrapped<TPlayer> *v8_wrapped = static_cast<V8ScriptWrapped<TPlayer> *>((*it).second->getScriptObject());

			v8::Local<v8::Object> object = v8::Object::New(isolate);
			object->Set(key_distance, v8::Number::New(isolate, (*it).first));
			object->Set(key_player, v8_wrapped->Handle(isolate));
			result->Set(context, idx++, object).Check();
		}

		args.GetReturnValue().Set(result);
	}
}

// Level Method: level.putnpc(x, y, script, options);
void Level_Function_PutNPC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 3);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		// Argument parsing
		float npcX = (float)args[0]->NumberValue(context).ToChecked();
		float npcY = (float)args[1]->NumberValue(context).ToChecked();
		CString script = *v8::String::Utf8Value(isolate, args[2]->ToString(context).ToLocalChecked());

		// TODO(joey): additional options parsing
		if (args.Length() == 4)
		{

		}

		TServer *server = levelObject->getServer();
		TNPC *npc = server->addNPC("", script, npcX, npcY, levelObject, false, true);

		if (npc != nullptr)
		{
			npc->setType("LOCALN");
			levelObject->addNPC(npc);

			V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>(npc->getScriptObject());
			args.GetReturnValue().Set(v8_wrapped->Handle(isolate));
		}
	}
}

// Level Method: level.onwall(x, y);
void Level_Function_OnWall(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber() && args[1]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		// Argument parsing
		double npcX = (float)args[0]->NumberValue(context).ToChecked();
		double npcY = (float)args[1]->NumberValue(context).ToChecked();

		args.GetReturnValue().Set(levelObject->isOnWall(npcX, npcY));
	}
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
//	level_proto->Set(v8::String::NewFromUtf8(isolate, "clone"), v8::FunctionTemplate::New(isolate, Level_Function_Clone, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8(isolate, "findareanpcs"), v8::FunctionTemplate::New(isolate, Level_Function_FindAreaNpcs, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8(isolate, "findnearestplayers"), v8::FunctionTemplate::New(isolate, Level_Function_FindNearestPlayers, engine_ref));
//	level_proto->Set(v8::String::NewFromUtf8(isolate, "reload"), v8::FunctionTemplate::New(isolate, Level_Function_Reload, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8(isolate, "putnpc"), v8::FunctionTemplate::New(isolate, Level_Function_PutNPC, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8(isolate, "onwall"), v8::FunctionTemplate::New(isolate, Level_Function_OnWall, engine_ref));

	// Properties
//	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "isnopkzone"), Level_GetBool_IsNoPkZone);		// TODO(joey): must be missing a status flag or something
	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "issparringzone"), Level_GetBool_IsSparringZone);
	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "name"), Level_GetStr_Name);
	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "npcs"), Level_GetArray_Npcs);
	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "players"), Level_GetArray_Players);
//	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "tiles"), Level_GetObject_Tiles);

	// Persist the constructor
	env->SetConstructor(ScriptConstructorId<TLevel>::result, level_ctor);
}

#endif
