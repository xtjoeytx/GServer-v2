#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include "IUtil.h"
#include "CScriptEngine.h"
#include "TLevel.h"
#include "TPlayer.h"
#include "TServer.h"

#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

// TODO(joey): Currently not cleaning this up
v8::Persistent<v8::FunctionTemplate> _persist_player_flags_ctor;

// PROPERTY: Id
void Player_GetInt_Id(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	info.GetReturnValue().Set(playerObject->getId());
}

// PROPERTY: player.type
void Player_GetInt_Type(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	info.GetReturnValue().Set(playerObject->isClient() ? 1 : 0);
}

// PROPERTY: X Position
void Player_GetNum_X(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	info.GetReturnValue().Set(playerObject->getX());
}

void Player_SetNum_X(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	int newValueInt = (int)(16 * newValue);
	if (newValueInt < 0) {
		newValueInt = (-newValueInt << 1) | 0x0001;
	}
	else newValueInt <<= 1;
	
	playerObject->setProps(CString() >> (char)PLPROP_X2 >> (short)newValueInt, true, true);
}

// PROPERTY: Y Position
void Player_GetNum_Y(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	info.GetReturnValue().Set(playerObject->getY());
}

void Player_SetNum_Y(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	int newValueInt = (int)(16 * newValue);
	if (newValueInt < 0) {
		newValueInt = (-newValueInt << 1) | 0x0001;
	}
	else newValueInt <<= 1;
	
	playerObject->setProps(CString() >> (char)PLPROP_Y2 >> (short)newValueInt, true, true);
}

// PROPERTY: Hearts
void Player_GetNum_Hearts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	info.GetReturnValue().Set(playerObject->getPower());
}

void Player_SetNum_Hearts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	int newValueInt = (int)(newValue * 2);
	playerObject->setProps(CString() >> (char)PLPROP_CURPOWER >> (char)newValueInt, true, true);
}

// PROPERTY: Maxhearts
void Player_GetInt_Maxhearts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getMaxPower());
}

void Player_SetInt_Maxhearts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	playerObject->setProps(CString() >> (char)PLPROP_MAXPOWER >> (char)newValue, true, true);
}

// PROPERTY: player.rupees
void Player_GetInt_Rupees(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	info.GetReturnValue().Set(playerObject->getRupees());
}

void Player_SetInt_Rupees(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	playerObject->setProps(CString() >> (char)PLPROP_RUPEESCOUNT >> (int)newValue, true, true);
}

// PROPERTY: player.account
void Player_GetStr_Account(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	CString accountName = playerObject->getAccountName();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), accountName.text());
	info.GetReturnValue().Set(strText);
}

// PROPERTY: player.isadmin
void Player_GetBool_IsAdmin(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	info.GetReturnValue().Set((playerObject->getType() & PLTYPE_ANYRC) != 0);
}

// PROPERTY: player.isclient
void Player_GetBool_IsClient(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	info.GetReturnValue().Set((playerObject->getType() & PLTYPE_ANYCLIENT) != 0);
}

// PROPERTY: player.level
void Player_GetObject_Level(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	TLevel *levelObject = playerObject->getLevel();

	if (levelObject != 0)
	{
		V8ScriptWrapped<TLevel> *v8_wrapped = static_cast<V8ScriptWrapped<TLevel> *>(levelObject->getScriptObject());
		info.GetReturnValue().Set(v8_wrapped->Handle(info.GetIsolate()));
		return;
	}

	info.GetReturnValue().SetNull();
}

// PROPERTY: player.levelname
void Player_GetStr_LevelName(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	TLevel *levelObject = playerObject->getLevel();

	CString levelName;
	if (levelObject != 0)
		levelName = levelObject->getLevelName();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), levelName.text());
	info.GetReturnValue().Set(strText);
}

// PROPERTY: player.nickname
void Player_GetStr_Chat(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	CString propValue = playerObject->getChatMsg();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text());
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_Chat(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::String::Utf8Value newValue = v8::String::Utf8Value(info.GetIsolate(), value);
	playerObject->setProps(CString() >> (char)PLPROP_CURCHAT >> (char)newValue.length() << *newValue, true, true);
}

// PROPERTY: player.nickname
void Player_GetStr_Nickname(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	CString propValue = playerObject->getNickname();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text());
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_Nickname(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

//	playerObject->setNick(*newValue, true);

	v8::String::Utf8Value newValue = v8::String::Utf8Value(info.GetIsolate(), value);
	playerObject->setProps(CString() >> (char)PLPROP_NICKNAME >> (char)newValue.length() << *newValue, true, true);
}

// PROPERTY: Player Flags
void Player_GetObject_Flags(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();
	
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
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	v8::Isolate *isolate = info.GetIsolate();
	
	// TODO(joey): playerObject is not getting unset here.

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
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	v8::Isolate *isolate = info.GetIsolate();
	
	// Get property name
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get new value
	v8::String::Utf8Value newValue(isolate, value);
	playerObject->setFlag(*utf8, *newValue, true);
}

void Player_Flags_Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);
	
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	
	// Get flags list
	auto flagList = playerObject->getFlagList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, flagList->size());

	int idx = 0;
	for (auto it = flagList->begin(); it != flagList->end(); ++it)
		result->Set(context, idx++, v8::String::NewFromUtf8(isolate, it->first.c_str())).Check();

	info.GetReturnValue().Set(result);
}

// Player Function: player.addweapon("weaponName");
void Player_Function_AddWeapon(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// Validate arguments
	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);
		
		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate));

		bool result = playerObject->addWeapon(*newValue);
		args.GetReturnValue().Set(result);
		return;
	}

	args.GetReturnValue().Set(false);
}

// Player Function: player.hasweapon("weaponName");
void Player_Function_HasWeapon(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// Validate arguments
	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate));

		bool result = playerObject->hasWeapon(*newValue);
		args.GetReturnValue().Set(result);
		return;
	}

	args.GetReturnValue().Set(false);
}

// Player Function: player.removeweapon("weaponName");
void Player_Function_RemoveWeapon(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// Validate arguments
	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);
		
		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate));

		bool result = playerObject->deleteWeapon(*newValue);
		args.GetReturnValue().Set(result);
		return;
	}

	args.GetReturnValue().Set(false);
}

// Player Function: player.disableweapons();
void Player_Function_DisableWeapons(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);
	
	playerObject->disableWeapons();
}

// Player Function: player.enableweapons();
void Player_Function_EnableWeapons(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);
	
	playerObject->enableWeapons();
}

// Player Function: player.say("message");
void Player_Function_Say(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// Validate arguments
	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate));
		playerObject->sendSignMessage(*newValue);
	}
}

// Player Function: player.sendrpgmessage("message");
void Player_Function_SendRPGMessage(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// Validate arguments
	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate));
		playerObject->sendRPGMessage(*newValue);
	}
}

// Player Function: player.setlevel2("levelname", x, y);
void Player_Function_SetLevel2(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 3);

	// Validate arguments
	if (args[0]->IsString() && args[1]->IsNumber() && args[2]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);
		
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		
		v8::String::Utf8Value levelName(isolate, args[0]->ToString(isolate));
		double newX = args[1]->NumberValue(context).ToChecked();
		double newY = args[2]->NumberValue(context).ToChecked();

		bool result = playerObject->warp(*levelName, newX, newY);
		args.GetReturnValue().Set(result);
		return;
	}

	args.GetReturnValue().Set(false);
}

// Player Function: player.join("class");
void Player_Function_Join(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	if (args[0]->IsString())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::External> data = args.Data().As<v8::External>();
		CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());

		std::string className = *v8::String::Utf8Value(isolate, args[0]->ToString(context).ToLocalChecked());

		TServer *server = scriptEngine->getServer();
		std::string classCode = server->getClass(className);

		if (!classCode.empty())
		{
			// Wrap code
			std::string classCodeWrap = CScriptEngine::WrapScript<TPlayer>(classCode);

			// TODO(joey): maybe we shouldn't cache this using this method, since classes can be used with
			// multiple wrappers.
			IScriptFunction *function = scriptEngine->CompileCache(classCodeWrap, false);
			V8ScriptFunction *v8_function = static_cast<V8ScriptFunction *>(function);
			v8::Local<v8::Value> newArgs[] = { args.This() };

			// Execute
			v8::TryCatch try_catch(isolate);
			v8::Local<v8::Function> scriptFunction = v8_function->Function();
			v8::MaybeLocal<v8::Value> scriptTableRet = scriptFunction->Call(context, args.This(), 1, newArgs);
			if (!scriptTableRet.IsEmpty())
			{
				args.GetReturnValue().Set(scriptTableRet.ToLocalChecked());
				return;
			}

			// TODO(joey): error handling

		}
	}
}

void Player_Function_TriggerAction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 4);

	if (args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsString() && args[3]->IsString())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		char trigx = (char)(args[0]->NumberValue(context).ToChecked() * 2);
		char trigy = (char)(args[1]->NumberValue(context).ToChecked() * 2);
		
		CString trigaction = *v8::String::Utf8Value(isolate, args[2]->ToString(context).ToLocalChecked());
		for (int i = 3; i < args.Length(); i++)
			trigaction << "," << *v8::String::Utf8Value(isolate, args[i]->ToString(context).ToLocalChecked());

		// Unwrap Object
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);
		
		playerObject->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)trigx >> (char)trigy << trigaction);
	}
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
	player_proto->Set(v8::String::NewFromUtf8(isolate, "addweapon"), v8::FunctionTemplate::New(isolate, Player_Function_AddWeapon));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "disableweapons"), v8::FunctionTemplate::New(isolate, Player_Function_DisableWeapons));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "enableweapons"), v8::FunctionTemplate::New(isolate, Player_Function_EnableWeapons));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "hasweapon"), v8::FunctionTemplate::New(isolate, Player_Function_HasWeapon));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "removeweapon"), v8::FunctionTemplate::New(isolate, Player_Function_RemoveWeapon));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "say"), v8::FunctionTemplate::New(isolate, Player_Function_Say, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8(isolate, "sendpm"), v8::FunctionTemplate::New(isolate, Player_Function_SendPM, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "sendrpgmessage"), v8::FunctionTemplate::New(isolate, Player_Function_SendRPGMessage, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8(isolate, "setani"), v8::FunctionTemplate::New(isolate, Player_Function_SetAni, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8(isolate, "setgender"), v8::FunctionTemplate::New(isolate, Player_Function_SetGender, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "setlevel2"), v8::FunctionTemplate::New(isolate, Player_Function_SetLevel2, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8(isolate, "setplayerprop"), v8::FunctionTemplate::New(isolate, Player_Function_SetPlayerProp, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "join"), v8::FunctionTemplate::New(isolate, Player_Function_Join, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8(isolate, "triggeraction"), v8::FunctionTemplate::New(isolate, Player_Function_TriggerAction, engine_ref));

	// Properties
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), Player_GetInt_Id);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "account"), Player_GetStr_Account);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "ani"), Player_GetStr_Ani, Player_SetStr_Ani);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "ap"), Player_GetInt_AP, Player_SetInt_AP);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "bombs"), Player_GetInt_Bombs, Player_SetInt_Bombs);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "chat"), Player_GetStr_Chat, Player_SetStr_Chat);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "darts"), Player_GetInt_Darts, Player_SetInt_Darts);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "hearts"), Player_GetNum_Hearts, Player_SetNum_Hearts);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "fullhearts"), Player_GetInt_Maxhearts, Player_SetInt_Maxhearts);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "isadmin"), Player_GetBool_IsAdmin);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "isclient"), Player_GetBool_IsClient);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "glovepower"), Player_GetInt_GlovePower, Player_SetInt_GlovePower);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "guild"), Player_GetStr_Guild, Player_SetStr_Guild);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "level"), Player_GetObject_Level);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "levelname"), Player_GetStr_LevelName);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "mp"), Player_GetInt_MP, Player_SetInt_MP);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "nick"), Player_GetStr_Nickname, Player_SetStr_Nickname);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "platform"), Player_GetString_Platform);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "bodyimg"), Player_GetStr_BodyImg, Player_SetStr_BodyImg);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "headimg"), Player_GetStr_HeadImg, Player_SetStr_HeadImg);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "shieldimg"), Player_GetStr_ShieldPower, Player_SetStr_ShieldPower);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "shieldpower"), Player_GetInt_SwordPower, Player_SetInt_SwordPower);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "swordimg"), Player_GetStr_SwordImage, Player_SetStr_SwordImage);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "swordpower"), Player_GetInt_SwordPower, Player_SetInt_SwordPower);
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "x"), Player_GetNum_X, Player_SetNum_X);
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "y"), Player_GetNum_Y, Player_SetNum_Y);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "colors"), Player_GetObject_Colors);
    player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "flags"), Player_GetObject_Flags);
//	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "weapons"), Player_GetObject_Weapons);
	player_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "rupees"), Player_GetInt_Rupees, Player_SetInt_Rupees);

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
