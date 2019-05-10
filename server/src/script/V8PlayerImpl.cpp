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

// TODO(joey): Currently not cleaning this up
v8::Persistent<v8::FunctionTemplate> _persist_player_flags_ctor;

// PROPERTY: Id
void Player_GetInt_Id(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	info.GetReturnValue().Set(playerObject->getId());
}

// PROPERTY: X Position
void Player_GetNum_X(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	info.GetReturnValue().Set(playerObject->getX());
}

void Player_SetNum_X(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	
	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	int newValueInt = 16 * (int)newValue;
	if (newValueInt < 0) {
		newValueInt = (-newValueInt << 1) | 0x0001;
	}
	else newValueInt <<= 1;
	
	playerObject->setProps(CString() >> (char)PLPROP_X2 >> (short)newValueInt, true, true);
}

// PROPERTY: Y Position
void Player_GetNum_Y(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	info.GetReturnValue().Set(playerObject->getY());
}

void Player_SetNum_Y(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	
	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	int newValueInt = 16 * (int)newValue;
	if (newValueInt < 0) {
		newValueInt = (-newValueInt << 1) | 0x0001;
	}
	else newValueInt <<= 1;
	
	playerObject->setProps(CString() >> (char)PLPROP_Y2 >> (short)newValueInt, true, true);
}

// PROPERTY: Hearts
void Player_GetNum_Hearts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	info.GetReturnValue().Set(playerObject->getPower());
}

void Player_SetNum_Hearts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	
	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	int newValueInt = (int)newValue * 2;
	playerObject->setProps(CString() >> (char)PLPROP_CURPOWER >> (char)newValueInt, true, true);
}

// PROPERTY: Maxhearts
void Player_GetInt_Maxhearts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	info.GetReturnValue().Set(playerObject->getMaxPower());
}

void Player_SetInt_Maxhearts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);
	
	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	playerObject->setProps(CString() >> (char)PLPROP_MAXPOWER >> (char)newValue, true, true);
}

// PROPERTY: Account
void Player_GetStr_Account(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);

	CString propValue = playerObject->getAccountName();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text());
	info.GetReturnValue().Set(strText);
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

	v8::String::Utf8Value newValue = v8::String::Utf8Value(info.GetIsolate(), value);
	playerObject->setProps(CString() >> (char)PLPROP_NICKNAME >> (char)newValue.length() << *newValue, true, true);
}


// PROPERTY: Player Flags
void Player_GetObject_Flags(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);

	v8::Local<v8::String> internalFlags = v8::String::NewFromUtf8(isolate, "_internalFlags", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalFlags).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalFlags).ToLocalChecked());
		return;
	}

	v8::Local<v8::FunctionTemplate> ctor_tpl = PersistentToLocal(isolate, _persist_player_flags_ctor);
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, playerObject);

	v8::PropertyAttribute propAttr = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalFlags, new_instance, propAttr).FromJust();
	info.GetReturnValue().Set(new_instance);
}

void Player_Flags_Getter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);

	// Get property name
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get server flag with the property
	CString flagValue = playerObject->getFlag(*utf8);
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, flagValue.text());
	info.GetReturnValue().Set(strText);
}

void Player_Flags_Setter(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);

	// Get property name
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get new value
	v8::String::Utf8Value newValue(isolate, value);
	playerObject->setFlag(*utf8, *newValue, true);
}

void Player_Flags_Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	TPlayer *playerObject = UnwrapObject<TPlayer>(self);

	// Get flags list
	auto flagList = playerObject->getFlagList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, flagList->size());

	int idx = 0;
	for (auto it = flagList->begin(); it != flagList->end(); ++it)
		result->Set(context, idx++, v8::String::NewFromUtf8(isolate, it->first.text())).Check();

	info.GetReturnValue().Set(result);
}

// NPC Method: player.addweapon("weaponName");
void Player_Function_AddWeapon(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Validate arguments
	if (args[0]->IsString())
	{
		// Unwrap Object
		TPlayer *playerObject = UnwrapObject<TPlayer>(args.This());
		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate));

		bool result = playerObject->addWeapon(*newValue);
		args.GetReturnValue().Set(result);
		return;
	}

	args.GetReturnValue().Set(false);
}

// NPC Method: player.hasweapon("weaponName");
void Player_Function_HasWeapon(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Validate arguments
	if (args[0]->IsString())
	{
		// Unwrap Object
		TPlayer *playerObject = UnwrapObject<TPlayer>(args.This());
		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate));

		bool result = playerObject->hasWeapon(*newValue);
		args.GetReturnValue().Set(result);
		return;
	}

	args.GetReturnValue().Set(false);
}

// NPC Method: player.removeweapon("weaponName");
void Player_Function_RemoveWeapon(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Validate arguments
	if (args[0]->IsString())
	{
		// Unwrap Object
		TPlayer *playerObject = UnwrapObject<TPlayer>(args.This());
		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate));

		bool result = playerObject->deleteWeapon(*newValue);
		args.GetReturnValue().Set(result);
		return;
	}

	args.GetReturnValue().Set(false);
}

// NPC Method: player.disableweapons();
void Player_Function_DisableWeapons(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Unwrap Object
	TPlayer *playerObject = UnwrapObject<TPlayer>(args.This());
	playerObject->disableWeapons();
}

// NPC Method: player.enableweapons();
void Player_Function_EnableWeapons(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Unwrap Object
	TPlayer *playerObject = UnwrapObject<TPlayer>(args.This());
	playerObject->enableWeapons();
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
	v8::Local<v8::FunctionTemplate> player_ctor = v8::FunctionTemplate::New(isolate, nullptr, engine_ref); // , Player_Constructor);
	v8::Local<v8::ObjectTemplate> player_proto = player_ctor->PrototypeTemplate();

    player_ctor->SetClassName(className);
    player_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	// TODO(joey): Implement these functions
	player_proto->Set(v8::String::NewFromUtf8(isolate, "addweapon"), v8::FunctionTemplate::New(isolate, Player_Function_AddWeapon, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "disableweapons"), v8::FunctionTemplate::New(isolate, Player_Function_DisableWeapons, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "enableweapons"), v8::FunctionTemplate::New(isolate, Player_Function_EnableWeapons, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "hasweapon"), v8::FunctionTemplate::New(isolate, Player_Function_HasWeapon, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "removeweapon"), v8::FunctionTemplate::New(isolate, Player_Function_RemoveWeapon, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8(isolate, "setani"), v8::FunctionTemplate::New(isolate, Player_Function_SetAni, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8(isolate, "setgender"), v8::FunctionTemplate::New(isolate, Player_Function_SetGender, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8(isolate, "setlevel2"), v8::FunctionTemplate::New(isolate, Player_Function_SetLevel2, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8(isolate, "setplayerprop"), v8::FunctionTemplate::New(isolate, Player_Function_SetPlayerProp, engine_ref));

	// Properties
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), Player_GetInt_Id);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "account"), Player_GetStr_Account);
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "nick"), Player_GetStr_Nickname, Player_SetStr_Nickname);
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "x"), Player_GetNum_X, Player_SetNum_X);
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "y"), Player_GetNum_Y, Player_SetNum_Y);
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "hearts"), Player_GetNum_Hearts, Player_SetNum_Hearts);
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "fullhearts"), Player_GetInt_Maxhearts, Player_SetInt_Maxhearts);
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "flags"), Player_GetObject_Flags);

	// Create the player flags template
    v8::Local<v8::FunctionTemplate> player_flags_ctor = v8::FunctionTemplate::New(isolate);
    player_flags_ctor->SetClassName(v8::String::NewFromUtf8(isolate, "flags"));
    player_flags_ctor->InstanceTemplate()->SetInternalFieldCount(1);
    player_flags_ctor->InstanceTemplate()->SetHandler(v8::NamedPropertyHandlerConfiguration(
            Player_Flags_Getter, Player_Flags_Setter, nullptr, nullptr, Player_Flags_Enumerator, v8::Local<v8::Value>(),
            v8::PropertyHandlerFlags::kOnlyInterceptStrings));
    _persist_player_flags_ctor.Reset(isolate, player_flags_ctor);

	// Persist the player constructor
	env->SetConstructor(ScriptConstructorId<TPlayer>::result, player_ctor);

	// Set the player constructor on the global object
	//v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();
	//global->Set(className, player_ctor);
}

#endif
