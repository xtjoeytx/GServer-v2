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
#include "fmt/format.h"

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
	auto& npcList = levelObject->getLevelNPCs();
	auto server = levelObject->getServer();

	v8::Local<v8::Array> result = v8::Array::New(isolate, server ? (int)npcList.size() : 0);

	if (server)
	{
		int idx = 0;
		for (unsigned int it : npcList) {
			auto npc = server->getNPC(it);
			auto *v8_wrapped = dynamic_cast<V8ScriptObject<TNPC> *>(npc->getScriptObject());
			result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
		}
	}

	info.GetReturnValue().Set(result);
}

// PROPERTY: level.signs
void Level_GetObject_Signs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalSigns = v8::String::NewFromUtf8(isolate, "_internalSigns", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalSigns).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalSigns).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	auto* scriptEngine = static_cast<CScriptEngine*>(data->Value());
	auto* env = dynamic_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("level.signs");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, levelObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevel> *>(levelObject->getScriptObject());
	v8_wrapped->addChild("signs", new_instance);

	auto propLinks = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalSigns, new_instance, propLinks).FromJust();

	info.GetReturnValue().Set(new_instance);
}

void Level_Sign_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	v8::Isolate* isolate = info.GetIsolate();

	auto sign = levelObject->getLevelSigns()[index];

	auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevelSign> *>(sign->getScriptObject());

	info.GetReturnValue().Set(v8_wrapped->Handle(isolate));
}

void Level_Sign_Length(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	v8::Isolate* isolate = info.GetIsolate();

	auto signSize = levelObject->getLevelSigns().size();

	info.GetReturnValue().Set(v8::Number::New(isolate, signSize));
}

void Level_Sign_Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Get link list
	auto& levelSigns = levelObject->getLevelSigns();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)levelSigns.size());

	int idx = 0;
	for (auto & sign : levelSigns) {
		result->Set(context, idx, v8::Number::New(isolate, idx)).Check();
		idx++;
	}

	info.GetReturnValue().Set(result);
}

void Level_Sign_Next(const v8::FunctionCallbackInfo<v8::Value>& info) {
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Get a reference to the instance of "level.links".
	v8::Local<v8::Object> obj = info.This();

	// Get the current index from the iterator object.
	v8::Local<v8::Number> currentIndex = obj->GetInternalField(0).As<v8::Number>();
	v8::Local<v8::Array> items = obj->GetInternalField(1).As<v8::Array>();

	// Get the length of the array.
	uint32_t len = items->Length();

	// Check if we have reached the end of the iteration sequence.
	if (currentIndex->Value() >= len) {
		auto newObj = v8::Object::New(isolate);
		newObj->Set(context, v8::String::NewFromUtf8Literal(isolate, "done"), v8::True(isolate)).Check();
		info.GetReturnValue().Set(newObj);

		return;
	}

	// Get the value at the current index.
	v8::Local<v8::Value> value = items->Get(context, (uint32_t)currentIndex->Value()).ToLocalChecked();

	// Update the iterator's index.
	obj->SetInternalField(0, v8::Integer::New(isolate, (int32_t)currentIndex->Value() + 1));

	// Create the next() result object.
	v8::Local<v8::Object> result = v8::Object::New(isolate);
	result->Set(context, v8::String::NewFromUtf8Literal(isolate, "value"), value).Check();
	result->Set(context, v8::String::NewFromUtf8Literal(isolate, "done"), v8::False(isolate)).Check();

	info.GetReturnValue().Set(result);
}

void Level_Sign_Iterator(const v8::FunctionCallbackInfo<v8::Value>& info) {
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Get link list
	auto& levelSigns = levelObject->getLevelSigns();

	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	v8::Local<v8::Object> obj = info.This();

	int current_index = 0;

	v8::Local<v8::FunctionTemplate> test_ctor = v8::FunctionTemplate::New(isolate);
	test_ctor->InstanceTemplate()->SetInternalFieldCount(2);

	v8::Local<v8::ObjectTemplate> test_proto = test_ctor->PrototypeTemplate();

	test_proto->Set(v8::String::NewFromUtf8Literal(isolate, "next"), v8::FunctionTemplate::New(isolate, Level_Sign_Next, obj));

	v8::Local<v8::Object> new_instance = test_ctor->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetInternalField(0, v8::Number::New(isolate, current_index));

	// Adds child property to the wrapped object, so it can clear the pointer when
	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)levelSigns.size());

	int idx = 0;
	for (auto & sign : levelSigns) {
		auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevelSign> *>(sign->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	new_instance->SetInternalField(1, result);

	info.GetReturnValue().Set(new_instance);
}

// Level Method: level.signs.add("dest.nw", x, y, width, height, newX, newY)
void Level_Function_AddLevelSign(const v8::FunctionCallbackInfo<v8::Value>& args)
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
		int levelX = (int)args[0]->NumberValue(context).ToChecked();
		int levelY = (int)args[1]->NumberValue(context).ToChecked();
		CString signText = *v8::String::Utf8Value(isolate, args[2]->ToString(context).ToLocalChecked());

		auto newSign = levelObject->addSign(levelX, levelY, signText);

		auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevelSign> *>(newSign->getScriptObject());
		args.GetReturnValue().Set(v8_wrapped->Handle(isolate));
	}
}

// Level Method: level.signs.remove(index)
void Level_Function_RemoveLevelSign(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		int index = (int)args[0]->NumberValue(context).ToChecked();

		args.GetReturnValue().Set(levelObject->removeSign(index));
	}

	args.GetReturnValue().Set(false);
}

// PROPERTY: level.signs
void Level_GetObject_Chests(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalChests = v8::String::NewFromUtf8(isolate, "_internalChests", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalChests).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalChests).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	auto* scriptEngine = static_cast<CScriptEngine*>(data->Value());
	auto* env = dynamic_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("level.chests");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, levelObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevel> *>(levelObject->getScriptObject());
	v8_wrapped->addChild("chests", new_instance);

	auto propLinks = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalChests, new_instance, propLinks).FromJust();

	info.GetReturnValue().Set(new_instance);
}

void Level_Chest_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	v8::Isolate* isolate = info.GetIsolate();

	auto chest = levelObject->getLevelChests()[index];

	auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevelSign> *>(chest->getScriptObject());

	info.GetReturnValue().Set(v8_wrapped->Handle(isolate));
}

void Level_Chest_Length(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	v8::Isolate* isolate = info.GetIsolate();

	auto chestSize = levelObject->getLevelChests().size();

	info.GetReturnValue().Set(v8::Number::New(isolate, chestSize));
}

void Level_Chest_Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Get link list
	auto& levelChests = levelObject->getLevelChests();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)levelChests.size());

	int idx = 0;
	for (auto & sign : levelChests) {
		result->Set(context, idx, v8::Number::New(isolate, idx)).Check();
		idx++;
	}

	info.GetReturnValue().Set(result);
}

void Level_Chest_Next(const v8::FunctionCallbackInfo<v8::Value>& info) {
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Get a reference to the instance of "level.links".
	v8::Local<v8::Object> obj = info.This();

	// Get the current index from the iterator object.
	v8::Local<v8::Number> currentIndex = obj->GetInternalField(0).As<v8::Number>();
	v8::Local<v8::Array> items = obj->GetInternalField(1).As<v8::Array>();

	// Get the length of the array.
	uint32_t len = items->Length();

	// Check if we have reached the end of the iteration sequence.
	if (currentIndex->Value() >= len) {
		auto newObj = v8::Object::New(isolate);
		newObj->Set(context, v8::String::NewFromUtf8Literal(isolate, "done"), v8::True(isolate)).Check();
		info.GetReturnValue().Set(newObj);

		return;
	}

	// Get the value at the current index.
	v8::Local<v8::Value> value = items->Get(context, (uint32_t)currentIndex->Value()).ToLocalChecked();

	// Update the iterator's index.
	obj->SetInternalField(0, v8::Integer::New(isolate, (int32_t)currentIndex->Value() + 1));

	// Create the next() result object.
	v8::Local<v8::Object> result = v8::Object::New(isolate);
	result->Set(context, v8::String::NewFromUtf8Literal(isolate, "value"), value).Check();
	result->Set(context, v8::String::NewFromUtf8Literal(isolate, "done"), v8::False(isolate)).Check();

	info.GetReturnValue().Set(result);
}

void Level_Chest_Iterator(const v8::FunctionCallbackInfo<v8::Value>& info) {
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Get link list
	auto& levelChests = levelObject->getLevelChests();

	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	v8::Local<v8::Object> obj = info.This();

	int current_index = 0;

	v8::Local<v8::FunctionTemplate> test_ctor = v8::FunctionTemplate::New(isolate);
	test_ctor->InstanceTemplate()->SetInternalFieldCount(2);

	v8::Local<v8::ObjectTemplate> test_proto = test_ctor->PrototypeTemplate();

	test_proto->Set(v8::String::NewFromUtf8Literal(isolate, "next"), v8::FunctionTemplate::New(isolate, Level_Chest_Next, obj));

	v8::Local<v8::Object> new_instance = test_ctor->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetInternalField(0, v8::Number::New(isolate, current_index));

	// Adds child property to the wrapped object, so it can clear the pointer when
	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)levelChests.size());

	int idx = 0;
	for (auto & chest : levelChests) {
		auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevelChest> *>(chest->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	new_instance->SetInternalField(1, result);

	info.GetReturnValue().Set(new_instance);
}

// Level Method: level.signs.add("dest.nw", x, y, width, height, newX, newY)
void Level_Function_AddLevelChest(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 4);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber() && args[3]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		// Argument parsing
		int levelX = (int)args[0]->NumberValue(context).ToChecked();
		int levelY = (int)args[1]->NumberValue(context).ToChecked();
		LevelItemType levelItemType = (LevelItemType)args[2]->NumberValue(context).ToChecked();
		int signId = (int)args[3]->NumberValue(context).ToChecked();

		auto newSign = levelObject->addChest(levelX, levelY, levelItemType, signId);

		auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevelChest> *>(newSign->getScriptObject());
		args.GetReturnValue().Set(v8_wrapped->Handle(isolate));
	}
}

// Level Method: level.signs.remove(index)
void Level_Function_RemoveLevelChest(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		int index = (int)args[0]->NumberValue(context).ToChecked();

		args.GetReturnValue().Set(levelObject->removeChest(index));
	}

	args.GetReturnValue().Set(false);
}

// PROPERTY: level.players
void Level_GetArray_Players(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Get npcs list
	auto& playerList = levelObject->getPlayerList();
	auto server = levelObject->getServer();

	v8::Local<v8::Array> result = v8::Array::New(isolate, server ? (int)playerList.size() : 0);

	if (server) {
		int idx = 0;
		for (auto it = playerList.begin(); it != playerList.end(); ++it) {
			auto player = server->getPlayer(*it);
			V8ScriptObject<TPlayer> *v8_wrapped = static_cast<V8ScriptObject<TPlayer> *>(player->getScriptObject());
			result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
		}
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

// PROPERTY: level.links
void Level_GetObject_Links(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalLinks = v8::String::NewFromUtf8(isolate, "_internalLinks", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalLinks).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalLinks).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	auto* scriptEngine = static_cast<CScriptEngine*>(data->Value());
	auto* env = dynamic_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("level.links");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, levelObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevel> *>(levelObject->getScriptObject());
	v8_wrapped->addChild("links", new_instance);

	auto propLinks = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalLinks, new_instance, propLinks).FromJust();

	info.GetReturnValue().Set(new_instance);
}

void Level_Link_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	v8::Isolate* isolate = info.GetIsolate();

	if (levelObject->getLevelLinks().empty()) {
		return;
	}

	auto link = levelObject->getLevelLinks()[index];

	auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevelLink> *>(link->getScriptObject());

	info.GetReturnValue().Set(v8_wrapped->Handle(isolate));
}

void Level_Link_Length(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	v8::Isolate* isolate = info.GetIsolate();

	auto linkSize = levelObject->getLevelLinks().size();

	info.GetReturnValue().Set(v8::Number::New(isolate, linkSize));
}

void Level_Link_Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Get link list
	auto& levelLinks = levelObject->getLevelLinks();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)levelLinks.size());

	int idx = 0;
	for (auto & link : levelLinks) {
		result->Set(context, idx, v8::Number::New(isolate, idx)).Check();
		idx++;
	}

	info.GetReturnValue().Set(result);
}

void Level_Link_Next(const v8::FunctionCallbackInfo<v8::Value>& info) {
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Get a reference to the instance of "level.links".
	v8::Local<v8::Object> obj = info.This();

	// Get the current index from the iterator object.
	v8::Local<v8::Number> currentIndex = obj->GetInternalField(0).As<v8::Number>();
	v8::Local<v8::Array> items = obj->GetInternalField(1).As<v8::Array>();

	// Get the length of the array.
	uint32_t len = items->Length();

	// Check if we have reached the end of the iteration sequence.
	if (currentIndex->Value() >= len) {
		auto newObj = v8::Object::New(isolate);
		newObj->Set(context, v8::String::NewFromUtf8Literal(isolate, "done"), v8::True(isolate)).Check();
		info.GetReturnValue().Set(newObj);

		return;
	}

	// Get the value at the current index.
	v8::Local<v8::Value> value = items->Get(context, (uint32_t)currentIndex->Value()).ToLocalChecked();

	// Update the iterator's index.
	obj->SetInternalField(0, v8::Integer::New(isolate, (int32_t)currentIndex->Value() + 1));

	// Create the next() result object.
	v8::Local<v8::Object> result = v8::Object::New(isolate);
	result->Set(context, v8::String::NewFromUtf8Literal(isolate, "value"), value).Check();
	result->Set(context, v8::String::NewFromUtf8Literal(isolate, "done"), v8::False(isolate)).Check();

	info.GetReturnValue().Set(result);
}

void Level_Link_Iterator(const v8::FunctionCallbackInfo<v8::Value>& info) {
	V8ENV_SAFE_UNWRAP(info, TLevel, levelObject);

	// Get link list
	auto& levelLinks = levelObject->getLevelLinks();

	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	v8::Local<v8::Object> obj = info.This();

	int current_index = 0;

	v8::Local<v8::FunctionTemplate> test_ctor = v8::FunctionTemplate::New(isolate);
	test_ctor->InstanceTemplate()->SetInternalFieldCount(2);

	v8::Local<v8::ObjectTemplate> test_proto = test_ctor->PrototypeTemplate();

	test_proto->Set(v8::String::NewFromUtf8Literal(isolate, "next"), v8::FunctionTemplate::New(isolate, Level_Link_Next, obj));

	v8::Local<v8::Object> new_instance = test_ctor->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetInternalField(0, v8::Number::New(isolate, current_index));

	// Adds child property to the wrapped object, so it can clear the pointer when
	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)levelLinks.size());

	int idx = 0;
	for (auto & link : levelLinks) {
		auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevelLink> *>(link->getScriptObject());
		result->Set(context, idx++, v8_wrapped->Handle(isolate)).Check();
	}

	new_instance->SetInternalField(1, result);

	info.GetReturnValue().Set(new_instance);
}


// Level Method: level.links.add("dest.nw", x, y, width, height, newX, newY)
void Level_Function_AddLevelLink(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 7);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsString() && args[1]->IsNumber() && args[2]->IsNumber() && args[3]->IsNumber() && args[4]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		// Argument parsing
		CString destination = *v8::String::Utf8Value(isolate, args[0]->ToString(context).ToLocalChecked());
		int levelX = (int)args[1]->NumberValue(context).ToChecked();
		int levelY = (int)args[2]->NumberValue(context).ToChecked();
		int width = (int)args[3]->NumberValue(context).ToChecked();
		int height = (int)args[4]->NumberValue(context).ToChecked();
		CString newX = *v8::String::Utf8Value(isolate, args[5]->ToString(context).ToLocalChecked());
		CString newY = *v8::String::Utf8Value(isolate, args[6]->ToString(context).ToLocalChecked());


		auto newLevelLink = levelObject->addLink();
		newLevelLink->setNewLevel(destination);
		newLevelLink->setX(levelX);
		newLevelLink->setY(levelY);
		newLevelLink->setWidth(width);
		newLevelLink->setHeight(height);
		newLevelLink->setNewX(newX);
		newLevelLink->setNewY(newY);

		auto *v8_wrapped = dynamic_cast<V8ScriptObject<TLevelLink> *>(newLevelLink->getScriptObject());
		args.GetReturnValue().Set(v8_wrapped->Handle(isolate));
	}
}

// Level Method: level.links.remove(index)
void Level_Function_RemoveLevelLink(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		int index = (int)args[0]->NumberValue(context).ToChecked();

		args.GetReturnValue().Set(levelObject->removeLink(index));
	}

	args.GetReturnValue().Set(false);
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

		auto &playerList = levelObject->getPlayerList();
		auto server = levelObject->getServer();
		if (server == nullptr) [[unlikely]] {
			v8::Local<v8::Array> result = v8::Array::New(isolate, (int)0);
			args.GetReturnValue().Set(result);
			return;
		}

		// Get distance for each player in the level, and sort it
		std::vector<std::pair<double, std::shared_ptr<TPlayer>>> playerListSorted;

		for (auto plId : playerList)
		{
			auto pl = server->getPlayer(plId);
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
			auto *v8_wrapped = static_cast<V8ScriptObject<TPlayer> *>(it.second->getScriptObject());

			v8::Local<v8::Object> object = v8::Object::New(isolate);
			object->Set(context, key_distance, v8::Number::New(isolate, it.first)).Check();
			object->Set(context, key_player, v8_wrapped->Handle(isolate)).Check();
			result->Set(context, idx++, object).Check();
		}

		args.GetReturnValue().Set(result);
	}
}

// Level Method: level.shoot(float x, float y, float z, float angle, float zangle, float strength, str ani, str aniparams);
void Level_Function_Shoot(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the minimum 8 arguments
	V8ENV_THROW_MINARGCOUNT(args, isolate, 8);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber() && args[3]->IsNumber() && args[4]->IsNumber() && args[5]->IsNumber() && args[6]->IsString() && args[7]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TLevel, levelObject);

		TServer* server = levelObject->getServer();
		if (server == nullptr) return;
		auto level = server->getLevel(levelObject->getLevelName().toString());

		auto x = (float)args[0]->NumberValue(context).ToChecked();
		auto y = (float)args[1]->NumberValue(context).ToChecked();
		auto z = (float)args[2]->NumberValue(context).ToChecked();
		auto angle = (float)args[3]->NumberValue(context).ToChecked();
		auto zangle = (float)args[4]->NumberValue(context).ToChecked();
		auto strength = (float)args[5]->NumberValue(context).ToChecked();
		std::string ani = *v8::String::Utf8Value(isolate, args[6]->ToString(context).ToLocalChecked());

		CString aniArgs;
		for (int i = 7; i < args.Length(); i++) {
			aniArgs << (std::string)*v8::String::Utf8Value(isolate, args[i]->ToString(context).ToLocalChecked()) << "\n";
		}
		aniArgs.gtokenizeI();

		// Send the packet out.
		server->sendShootToOneLevel(level, x, y, z, angle, zangle, strength, ani, aniArgs.text());
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
		if (server == nullptr) return;
		auto level = server->getLevel(levelObject->getLevelName().toString());

		unsigned char eradius = args[0]->Int32Value(context).ToChecked();
		float loc[2] = {
			(float)(args[1]->NumberValue(context).ToChecked()),
			(float)(args[2]->NumberValue(context).ToChecked())
		};

		unsigned char epower = 1;

		// Send the packet out.
		CString packet = CString() >> (char)PLO_EXPLOSION >> (short)0 >> (char)eradius >> (char)(loc[0] * 2) >> (char)(loc[1] * 2) >> (char)epower;
		server->sendPacketToOneLevel(packet, level);
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
		auto level = server->getLevel(levelObject->getLevelName().toString());

		auto npc = server->addNPC("", script, npcX, npcY, level, false, true);
		if (npc != nullptr)
		{
			npc->setScriptType("LOCALN");
			levelObject->addNPC(npc);

			V8ScriptObject<TNPC> *v8_wrapped = static_cast<V8ScriptObject<TNPC> *>(npc->getScriptObject());
			args.GetReturnValue().Set(v8_wrapped->Handle(isolate));
		}
	}
}

// Level Method: level.addnpc(x, y, script, options);
void Level_Function_AddNPC(const v8::FunctionCallbackInfo<v8::Value>& args)
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
		auto level = server->getLevel(levelObject->getLevelName().toString());

		auto npc = server->addNPC("", script, npcX, npcY, level, true, true);
		if (npc != nullptr)
		{
			npc->setScriptType("LOCALN");
			levelObject->addNPC(npc);

			auto *v8_wrapped = dynamic_cast<V8ScriptObject<TNPC> *>(npc->getScriptObject());
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
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "shoot"), v8::FunctionTemplate::New(isolate, Level_Function_Shoot, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "putexplosion"), v8::FunctionTemplate::New(isolate, Level_Function_PutExplosion, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "putnpc"), v8::FunctionTemplate::New(isolate, Level_Function_PutNPC, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "addnpc"), v8::FunctionTemplate::New(isolate, Level_Function_AddNPC, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "onwall"), v8::FunctionTemplate::New(isolate, Level_Function_OnWall, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "onwall2"), v8::FunctionTemplate::New(isolate, Level_Function_OnWall2, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "addlevelsign"), v8::FunctionTemplate::New(isolate, Level_Function_AddLevelSign, engine_ref));
	level_proto->Set(v8::String::NewFromUtf8Literal(isolate, "removelevelsign"), v8::FunctionTemplate::New(isolate, Level_Function_RemoveLevelSign, engine_ref));

	// Properties
//	level_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "isnopkzone"), Level_GetBool_IsNoPkZone);		// TODO(joey): must be missing a status flag or something
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "issparringzone"), Level_GetBool_IsSparringZone);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "name"), Level_GetStr_Name);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "mapname"), Level_GetStr_MapName);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "npcs"), Level_GetArray_Npcs);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "links"), Level_GetObject_Links, nullptr, engine_ref);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "signs"), Level_GetObject_Signs, nullptr, engine_ref);
	level_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "chests"), Level_GetObject_Chests, nullptr, engine_ref);
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

	// Create the level link template
	v8::Local<v8::FunctionTemplate> level_links_ctor = v8::FunctionTemplate::New(isolate);
	level_links_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "links"));
	level_links_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	level_links_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			Level_Link_Getter, nullptr, nullptr, nullptr, Level_Link_Enumerator, v8::Local<v8::Value>(),
			v8::PropertyHandlerFlags::kNone));
	v8::Local<v8::ObjectTemplate> level_links_proto = level_links_ctor->PrototypeTemplate();

	level_links_proto->Set(v8::String::NewFromUtf8Literal(isolate, "add"), v8::FunctionTemplate::New(isolate, Level_Function_AddLevelLink, engine_ref));
	level_links_proto->Set(v8::String::NewFromUtf8Literal(isolate, "remove"), v8::FunctionTemplate::New(isolate, Level_Function_RemoveLevelLink, engine_ref));
	level_links_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "length"), Level_Link_Length);

	// Define the Symbol.iterator method on the prototype to make "level.links" iterable
	v8::Local<v8::FunctionTemplate> level_links_iterator = v8::FunctionTemplate::New(isolate);
	level_links_iterator->SetCallHandler(Level_Link_Iterator);
	level_links_proto->Set(v8::Symbol::GetIterator(isolate), level_links_iterator);

	env->SetConstructor("level.links", level_links_ctor);

	// Create the level signs template
	v8::Local<v8::FunctionTemplate> level_signs_ctor = v8::FunctionTemplate::New(isolate);
	level_signs_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "signs"));
	level_signs_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	level_signs_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			Level_Sign_Getter, nullptr, nullptr, nullptr, Level_Sign_Enumerator, v8::Local<v8::Value>(),
			v8::PropertyHandlerFlags::kNone));
	v8::Local<v8::ObjectTemplate> level_signs_proto = level_signs_ctor->PrototypeTemplate();

	level_signs_proto->Set(v8::String::NewFromUtf8Literal(isolate, "add"), v8::FunctionTemplate::New(isolate, Level_Function_AddLevelSign, engine_ref));
	level_signs_proto->Set(v8::String::NewFromUtf8Literal(isolate, "remove"), v8::FunctionTemplate::New(isolate, Level_Function_RemoveLevelSign, engine_ref));
	level_signs_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "length"), Level_Sign_Length);

	// Define the Symbol.iterator method on the prototype to make "level.links" iterable
	v8::Local<v8::FunctionTemplate> level_signs_iterator = v8::FunctionTemplate::New(isolate);
	level_signs_iterator->SetCallHandler(Level_Sign_Iterator);
	level_signs_proto->Set(v8::Symbol::GetIterator(isolate), level_signs_iterator);

	env->SetConstructor("level.signs", level_signs_ctor);

	// Create the level chests template
	v8::Local<v8::FunctionTemplate> level_chests_ctor = v8::FunctionTemplate::New(isolate);
	level_chests_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "chests"));
	level_chests_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	level_chests_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			Level_Chest_Getter, nullptr, nullptr, nullptr, Level_Chest_Enumerator, v8::Local<v8::Value>(),
			v8::PropertyHandlerFlags::kNone));
	v8::Local<v8::ObjectTemplate> level_chests_proto = level_chests_ctor->PrototypeTemplate();

	level_chests_proto->Set(v8::String::NewFromUtf8Literal(isolate, "add"), v8::FunctionTemplate::New(isolate, Level_Function_AddLevelChest, engine_ref));
	level_chests_proto->Set(v8::String::NewFromUtf8Literal(isolate, "remove"), v8::FunctionTemplate::New(isolate, Level_Function_RemoveLevelChest, engine_ref));
	level_chests_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "length"), Level_Chest_Length);

	// Define the Symbol.iterator method on the prototype to make "level.links" iterable
	v8::Local<v8::FunctionTemplate> level_chests_iterator = v8::FunctionTemplate::New(isolate);
	level_chests_iterator->SetCallHandler(Level_Chest_Iterator);
	level_chests_proto->Set(v8::Symbol::GetIterator(isolate), level_chests_iterator);

	env->SetConstructor("level.chests", level_chests_ctor);

	// Persist the constructor
	env->SetConstructor(ScriptConstructorId<TLevel>::result, level_ctor);
}

#endif
