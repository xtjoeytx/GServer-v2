#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <math.h>
#include <algorithm>
#include <unordered_map>
#include "CScriptEngine.h"
#include "V8ScriptFunction.h"
#include "V8ScriptObject.h"

#include "TLevel.h"
#include "TMap.h"
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

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), levelObject->getLevelName().text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

// PROPERTY: level.mapname
void Level_GetStr_MapName(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	auto map = levelObject->getMap();
	if (map)
	{
		v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), map->getMapName().c_str()).ToLocalChecked();
		info.GetReturnValue().Set(strText);
		return;
	}

	info.GetReturnValue().SetNull();
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
		V8ScriptObject<TNPC> *v8_wrapped = static_cast<V8ScriptObject<TNPC> *>((*it)->getScriptObject());
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
		V8ScriptObject<TPlayer> *v8_wrapped = static_cast<V8ScriptObject<TPlayer> *>((*it)->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	info.GetReturnValue().Set(result);
}

// PROPERTY: level.tiles
void Level_GetObject_Tiles(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalTiles = v8::String::NewFromUtf8(isolate, "_internalTiles", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalTiles).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalTiles).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	auto* scriptEngine = static_cast<CScriptEngine*>(data->Value());
	auto* env = dynamic_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("level.tiles");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, levelObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevel> *>(levelObject->getScriptObject());
	v8_wrapped->addChild("tiles", new_instance);

	auto propTiles = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalTiles, new_instance, propTiles).FromJust();
	info.GetReturnValue().Set(new_instance);
}


void Level_Tile_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	if ( index > 4096 )
		return;

	v8::Isolate* isolate = info.GetIsolate();

	auto tile = levelObject->getTiles()[index];

	v8::Local<v8::Integer> tileValue = v8::Integer::New(isolate, tile);
	info.GetReturnValue().Set(tileValue);
}

void Level_Tile_Setter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	if ( index > 4096 )
		return;

	v8::Isolate* isolate = info.GetIsolate();

	// Get new value
	if (value->IsUint32())
	{
		// Get new value
		unsigned int newValue = value->Uint32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();

		levelObject->modifyBoardDirect(index, (short)newValue);
	}

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
}

// Level Method: level.savelevel(levelname);
void Level_Function_SaveLevel(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

	// Create level from user input
	if (args[0]->IsString())
	{
		std::string filename = *v8::String::Utf8Value(isolate, args[0]->ToString(context).ToLocalChecked());

		if (levelObject != nullptr)
		{
			levelObject->saveLevel(filename);
		}
	}
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
	for (auto npc : npcList)
	{
		V8ScriptObject<TNPC> *v8_wrapped = static_cast<V8ScriptObject<TNPC> *>(npc->getScriptObject());
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

		for (auto pl : *playerList)
		{
			double distance = sqrt(pow(pl->getY() - targetY, 2) + pow(pl->getX() - targetX, 2));
			playerListSorted.emplace_back( distance, pl );
		}

		std::sort(playerListSorted.begin(), playerListSorted.end());

		// Create array of objects
		v8::Local<v8::String> key_distance = v8::String::NewFromUtf8(isolate, "distance", v8::NewStringType::kInternalized).ToLocalChecked();
		v8::Local<v8::String> key_player = v8::String::NewFromUtf8(isolate, "player", v8::NewStringType::kInternalized).ToLocalChecked();
		v8::Local<v8::Array> result = v8::Array::New(isolate, (int)playerListSorted.size());

		int idx = 0;
		for (auto & it : playerListSorted)
		{
			V8ScriptObject<TPlayer> *v8_wrapped = static_cast<V8ScriptObject<TPlayer> *>(it.second->getScriptObject());

			v8::Local<v8::Object> object = v8::Object::New(isolate);
			object->Set(context, key_distance, v8::Number::New(isolate, it.first)).Check();
			object->Set(context, key_player, v8_wrapped->Handle(isolate)).Check();
			result->Set(context, idx++, object).Check();
		}

		args.GetReturnValue().Set(result);
	}
}
// Level Method: level.putexplosion(radius, x, y);
void Level_Function_PutExplosion(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 3);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		TServer* server = levelObject->getServer();

		unsigned char eradius = args[0]->Int32Value(context).ToChecked();
		float loc[2] = {
			(float)(args[1]->NumberValue(context).ToChecked()),
			(float)(args[2]->NumberValue(context).ToChecked())
		};

		unsigned char epower = 1;

		// Send the packet out.
		CString packet = CString() >> (short)0 >> (char)eradius >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)epower;
		server->sendPacketToLevel(PLO_EXPLOSION, packet, nullptr, levelObject);
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
			npc->setScriptType("LOCALN");
			levelObject->addNPC(npc);

			V8ScriptObject<TNPC> *v8_wrapped = static_cast<V8ScriptObject<TNPC> *>(npc->getScriptObject());
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
		int npcX = int(16.0 * args[0]->NumberValue(context).ToChecked());
		int npcY = int(16.0 * args[1]->NumberValue(context).ToChecked());

		args.GetReturnValue().Set(levelObject->isOnWall(npcX, npcY));
	}
}

// Level Method: level.onwall2(x, y, w, h);
void Level_Function_OnWall2(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 4);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber() && args[3]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		// Argument parsing
		auto npcX = args[0]->NumberValue(context).ToChecked();
		auto npcY = args[1]->NumberValue(context).ToChecked();
		auto width = args[2]->Int32Value(context).ToChecked();
		auto height = args[3]->Int32Value(context).ToChecked();

		if (std::lround(npcX) != int(npcX))
		{
			width++;
		}

		if (std::lround(npcY) != int(npcY))
		{
			height++;
		}

		args.GetReturnValue().Set(levelObject->isOnWall2(int(npcX), int(npcY), width, height));
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
	v8::Local<v8::String> levelStr = v8::String::NewFromUtf8Literal(isolate, "level", v8::NewStringType::kInternalized);

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> level_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> level_proto = level_ctor->PrototypeTemplate();
	level_ctor->SetClassName(levelStr);
	level_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
//	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "clone"), v8::FunctionTemplate::New(isolate, Level_Function_Clone, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "savelevel"), v8::FunctionTemplate::New(isolate, Level_Function_SaveLevel, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "findareanpcs"), v8::FunctionTemplate::New(isolate, Level_Function_FindAreaNpcs, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "findnearestplayers"), v8::FunctionTemplate::New(isolate, Level_Function_FindNearestPlayers, engine_ref));
//	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "reload"), v8::FunctionTemplate::New(isolate, Level_Function_Reload, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "putexplosion"), v8::FunctionTemplate::New(isolate, Level_Function_PutExplosion, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "putnpc"), v8::FunctionTemplate::New(isolate, Level_Function_PutNPC, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "onwall"), v8::FunctionTemplate::New(isolate, Level_Function_OnWall, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "onwall2"), v8::FunctionTemplate::New(isolate, Level_Function_OnWall2, engine_ref));

	// Properties
//	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "isnopkzone"), Level_GetBool_IsNoPkZone);		// TODO(joey): must be missing a status flag or something
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "issparringzone"), Level_GetBool_IsSparringZone);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "name"), Level_GetStr_Name);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "mapname"), Level_GetStr_MapName);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "npcs"), Level_GetArray_Npcs);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "players"), Level_GetArray_Players);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "tiles"), Level_GetObject_Tiles, nullptr, engine_ref);

	// Create the player attr template
	v8::Local<v8::FunctionTemplate> level_tiles_ctor = v8::FunctionTemplate::New(isolate);
	level_tiles_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "tiles"));
	level_tiles_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	level_tiles_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			Level_Tile_Getter, Level_Tile_Setter, nullptr, nullptr, nullptr, v8::Local<v8::Value>(),
			v8::PropertyHandlerFlags::kNone));
	env->SetConstructor("level.tiles", level_tiles_ctor);


	// Persist the constructor
	env->SetConstructor(ScriptConstructorId<TLevel>::result, level_ctor);
}

#endif
