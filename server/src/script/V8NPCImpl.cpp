#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <unordered_map>
#include "IUtil.h"
#include "CScriptEngine.h"
#include "TLevel.h"
#include "TNPC.h"

#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

// Property: npc.id
void NPC_GetInt_Id(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	info.GetReturnValue().Set(npcObject->getId());
}

// Property: npc.name
void NPC_GetStr_Name(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	v8::Local<v8::String> npcName = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getName().c_str());
	info.GetReturnValue().Set(npcName);
}

// Property: npc.x
void NPC_GetNum_X(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	info.GetReturnValue().Set((double)npcObject->getPixelX() / 16.0f);
}

void NPC_SetNum_X(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setX(newValue);
	//npcObject->updatePropModTime(NPCPROP_X, time(0));
	npcObject->updatePropModTime(NPCPROP_X2, time(0));
}

// Property: npc.y
void NPC_GetNum_Y(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	info.GetReturnValue().Set((double)npcObject->getPixelY() / 16.0f);
}

void NPC_SetNum_Y(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setY(newValue);
	//npcObject->updatePropModTime(NPCPROP_Y, time(0));
	npcObject->updatePropModTime(NPCPROP_Y2, time(0));
}

// PROPERTY: Level
void NPC_GetObject_Level(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	TLevel *npcLevel = npcObject->getLevel();
	if (npcLevel != 0)
	{
		V8ScriptWrapped<TLevel> *v8_wrapped = static_cast<V8ScriptWrapped<TLevel> *>(npcLevel->getScriptObject());
		info.GetReturnValue().Set(v8_wrapped->Handle(info.GetIsolate()));
		return;
	}

	info.GetReturnValue().SetNull();
}

// PROPERTY: LevelName
void NPC_GetStr_LevelName(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	TLevel *npcLevel = npcObject->getLevel();
	CString levelName("");
	if (npcLevel != 0)
		levelName = npcLevel->getLevelName();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), levelName.text());
	info.GetReturnValue().Set(strText);
}

// PROPERTY: Timeout
void NPC_GetNum_Timeout(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	double timeout = npcObject->getTimeout() / 20;
	info.GetReturnValue().Set(timeout);
}

void NPC_SetNum_Timeout(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	double timeout = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setTimeout((int)(timeout * 20));
}

// PROPERTY: Rupees
void NPC_GetInt_Rupees(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	info.GetReturnValue().Set(npcObject->getRupees());
}

void NPC_SetInt_Rupees(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setRupees(newValue);
	npcObject->updatePropModTime(NPCPROP_RUPEES, time(0));
}

// PROPERTY: Bombs
void NPC_GetInt_Bombs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_BOMBS);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Bombs(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_BOMBS >> (char)clip(newValue, 0, 99), CLVER_2_17, true);
}

// PROPERTY: Darts
void NPC_GetInt_Darts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_ARROWS);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Darts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_ARROWS >> (char)clip(newValue, 0, 99), CLVER_2_17, true);
}

// PROPERTY: Hearts
void NPC_GetInt_Hearts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_POWER);
	info.GetReturnValue().Set((float)npcProp.readGUChar() / 2.0f);
}

void NPC_SetInt_Hearts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	int newValue = (int)(value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked() * 2);
	npcObject->setProps(CString() >> (char)NPCPROP_POWER >> (char)clip(newValue, 0, 40), CLVER_2_17, true);
}

// PROPERTY: Glove Power
void NPC_GetInt_GlovePower(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_GLOVEPOWER);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_GlovePower(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_GLOVEPOWER >> (char)clip(newValue, 0, 3), CLVER_2_17, true);
}

// PROPERTY: Dir
void NPC_GetInt_Dir(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_SPRITE);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Dir(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_SPRITE >> (char)(newValue % 4), CLVER_2_17, true);
}

// PROPERTY: Ap
void NPC_GetInt_Ap(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_ALIGNMENT);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Ap(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	int newValue = clip(value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked(), 0, 100);
	npcObject->setProps(CString() >> (char)NPCPROP_ALIGNMENT >> (char)newValue, CLVER_2_17, true);
}

// PROPERTY: npc.nick
void NPC_GetStr_Nickname(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);
	
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getNickname().text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Nickname(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	npcObject->setNickname(*newValue);
}

// PROPERTY: Message
void NPC_GetStr_Message(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_MESSAGE);
	CString propValue = npcProp.readChars(npcProp.readGUChar());

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Message(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	npcObject->setProps(CString() >> (char)NPCPROP_MESSAGE >> (char)newValue.length() << *newValue, CLVER_2_17, true);
}

// PROPERTY: Animation
void NPC_GetStr_Ani(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_MESSAGE);
	CString propValue = npcProp.readChars(npcProp.readGUChar());

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Ani(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	npcObject->setProps(CString() >> (char)NPCPROP_GANI >> (char)newValue.length() << *newValue, CLVER_2_17, true);
}

// NPC Method: npc.blockagain()
void NPC_Function_BlockAgain(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Unwrap Object
	TNPC *npcObject = UnwrapObject<TNPC>(args.This());
	// TODO(joey): implement this
	//npcObject->allowNpcWarping(true);
}

// NPC Method: npc.canwarp();
void NPC_Function_CanWarp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Unwrap Object
	TNPC *npcObject = UnwrapObject<TNPC>(args.This());
	npcObject->allowNpcWarping(true);
}

// NPC Method: npc.cannotwarp();
void NPC_Function_CannotWarp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Unwrap Object
	TNPC *npcObject = UnwrapObject<TNPC>(args.This());
	npcObject->allowNpcWarping(false);
}

// NPC Method: npc.message();
void NPC_Function_Message(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Validate arguments
	if (args[0]->IsString())
	{
		// Retrieve external data
		//v8::Local<v8::External> data = args.Data().As<v8::External>();
		//CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
		//V8ScriptEnv *scriptEnv = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

		// Unwrap Object
		TNPC *npcObject = UnwrapObject<TNPC>(args.This());
		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate));
		npcObject->setProps(CString() >> (char)NPCPROP_MESSAGE >> (char)newValue.length() << *newValue, CLVER_2_17, true);
	}
}

// NPC Method: npc.move(x, y, time, options);
void NPC_Function_Move(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 4);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Argument parsing
	int delta_x = (int)(16 * args[0]->NumberValue(context).ToChecked());
	int delta_y = (int)(16 * args[1]->NumberValue(context).ToChecked());
	double time_fps = (int)(args[2]->NumberValue(context).ToChecked());
	int options = args[3]->Int32Value(context).ToChecked();

	// Unwrap Object
	TNPC *npcObject = UnwrapObject<TNPC>(args.This());
	npcObject->moveNPC(delta_x, delta_y, time_fps, options);
}

// NPC Method: npc.showcharacter();
void NPC_Function_ShowCharacter(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Unwrap Object
	TNPC *npcObject = UnwrapObject<TNPC>(args.This());
	npcObject->setProps(CString() >> (char)NPCPROP_IMAGE >> (char)3 << "#c#", CLVER_2_17, true);
	npcObject->setWidth(32);
	npcObject->setHeight(48);
}

// NPC Method: npc.setcharprop(code, value);
void NPC_Function_SetCharProp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	
	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	CString code = *v8::String::Utf8Value(isolate, args[0]->ToString(isolate));
	v8::String::Utf8Value newValue(isolate, args[1]->ToString(isolate));
	
	if (code[0] == '#')
	{
		// Unwrap Object
		TNPC *npcObject = UnwrapObject<TNPC>(args.This());

		switch (code[1])
		{
			case '1': // sword image
			{
				CString npcProp = npcObject->getProp(NPCPROP_SWORDIMAGE);
				unsigned char swordPower = npcProp.readGUChar();
				if (swordPower < 31)
					swordPower = 31;
				npcObject->setProps(CString() >> (char)NPCPROP_SWORDIMAGE >> (char)swordPower >> (char)newValue.length() << *newValue, CLVER_2_17, true);
				break;
			}

			case '2': // shield image
			{
				CString npcProp = npcObject->getProp(NPCPROP_SHIELDIMAGE);
				unsigned char shieldPower = npcProp.readGUChar();
				if (shieldPower < 11)
					shieldPower = 11;
				npcObject->setProps(CString() >> (char)NPCPROP_SHIELDIMAGE >> (char)shieldPower >> (char)newValue.length() << *newValue, CLVER_2_17, true);
				break;
			}

			case '3': // head image
				npcObject->setProps(CString() >> (char)NPCPROP_HEADIMAGE >> (char)(newValue.length() + 100) << *newValue, CLVER_2_17, true);
				break;

			case '5': // horse image (needs to be tested)
				npcObject->setProps(CString() >> (char)NPCPROP_HORSEIMAGE >> (char)newValue.length() << *newValue, CLVER_2_17, true);
				break;

			case '8': // body image
				npcObject->setProps(CString() >> (char)NPCPROP_BODYIMAGE >> (char)newValue.length() << *newValue, CLVER_2_17, true);
				break;

			case 'c': // chat
				npcObject->setProps(CString() >> (char)NPCPROP_MESSAGE >> (char)newValue.length() << *newValue, CLVER_2_17, true);
				break;

			case 'n': // nickname
				npcObject->setProps(CString() >> (char)NPCPROP_NICKNAME >> (char)newValue.length() << *newValue, CLVER_2_17, true);
				break;

			case 'C': // colors
			{
				if (code[2] >= '0' && code[2] < '5')
				{
					CString npcProp = npcObject->getProp(NPCPROP_COLORS);
					char colors[5];
					for (unsigned int i = 0; i < 5; i++)
						colors[i] = npcProp.readGUChar();

					colors[code[2] - '0'] = getColor(*newValue);
					npcObject->setProps(CString() >> (char)NPCPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4], CLVER_2_17, true);
				}
				break;
			}
		}
	}
}

// NPC Method: npc.setshape(type, pixelWidth, pixelHeight);
void NPC_Function_SetShape(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 3);

	if (args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsInt32())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();

		// Unwrap Object
		TNPC *npcObject = UnwrapObject<TNPC>(args.This());

		int width = args[1]->Int32Value(context).ToChecked();
		int height = args[2]->Int32Value(context).ToChecked();
		npcObject->setWidth(width);
		npcObject->setHeight(height);
	}
}

// NPC Method: npc.registerAction(string, function);
void NPC_Function_RegisterTrigger(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	V8ENV_D("Begin NPC::registerAction()\n");

	if (args[0]->IsString() && args[1]->IsFunction())
	{
		V8ENV_D(" - Register npc action %s with: %s\n",
			*v8::String::Utf8Value(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked()),
			*v8::String::Utf8Value(isolate, args[1]->ToString(isolate->GetCurrentContext()).ToLocalChecked()));

		v8::Local<v8::External> data = args.Data().As<v8::External>();
		CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());

		V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

		// Callback name
		std::string eventName = *v8::String::Utf8Value(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());

		// Persist the callback function so we can retrieve it later on
		v8::Local<v8::Function> cbFunc = args[1].As<v8::Function>();
		V8ScriptFunction *cbFuncWrapper = new V8ScriptFunction(env, cbFunc);

		// Unwrap Object
		TNPC *npcObject = UnwrapObject<TNPC>(args.This());
		npcObject->registerTriggerAction(eventName, cbFuncWrapper);
	}

	V8ENV_D("End NPC::registerAction()\n");
}

// Called when javascript creates a new object
// js example: let jsNpc = new NPC();
//void Npc_Constructor(const v8::FunctionCallbackInfo<v8::Value>& args)
//{
//	// TODO(joey): more proof of concept, likely to get axed.
//	
//	// Called by V8. and should return an NPC object
//	V8ENV_D("Npc_Constructor called\n");
//
//	v8::Isolate *isolate = args.GetIsolate();
//	v8::Local<v8::Context> context = isolate->GetCurrentContext();
//
//	// Throw an exception on method functions for constructor calls
//	V8ENV_THROW_METHOD(args, isolate);
//	
//	// Retrieve external data for this call
//	v8::Local<v8::External> data = args.Data().As<v8::External>();
//	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
//	
//	TNPC *newNpc = new TNPC(scriptEngine->getServer());
//
//	assert(args.This()->InternalFieldCount() > 0);
//	
//	args.This()->SetAlignedPointerInInternalField(0, newNpc);
//	args.GetReturnValue().Set(args.This());
//}

// NPC Static Method: NPC::create();
void Npc_createFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();
	
	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	
	// Retrieve v8 environment
	v8::Local<v8::External> data = args.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

	args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, "test str"));

	if (args.Length() > 0)
		V8ENV_D("Npc::createFunction: %s\n", *v8::String::Utf8Value(isolate, args[0]->ToString(isolate)));
}

// NPC Method: npc.test();
void Npc_testFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Retrieve external data for this call
	v8::Local<v8::External> data = args.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());

	// Check if the callback exists
	V8ScriptFunction *callback = static_cast<V8ScriptFunction *>(scriptEngine->getCallBack("onTest"));
	if (callback != 0)
	{
		//		V8ScriptEnv *env = static_cast<V8ScriptEnv *>(server->getScriptEnv());
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Function> cbFunc = callback->Function();

		// Call callback
		v8::Local<v8::Value> newArgs[] = {
			args.This(),
			args[0]
		};
		v8::MaybeLocal<v8::Value> ret = cbFunc->Call(context, Null(isolate), 2, newArgs);
		if (!ret.IsEmpty())
			V8ENV_D(" - Returned Value from Callback: %s\n", *(v8::String::Utf8Value(isolate, ret.ToLocalChecked())));
	}

	V8ENV_D("Npc::testFunction: %s\n", *v8::String::Utf8Value(isolate, args[0]->ToString(isolate)));
}

void bindClass_NPC(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();
	
	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "npc"
	v8::Local<v8::String> npcStr = v8::String::NewFromUtf8(isolate, "npc", v8::NewStringType::kInternalized).ToLocalChecked();
	
	// Create constructor for class
	v8::Local<v8::FunctionTemplate> npc_ctor = v8::FunctionTemplate::New(isolate, nullptr, engine_ref);
	v8::Local<v8::ObjectTemplate> npc_proto = npc_ctor->PrototypeTemplate();
	
	npc_ctor->SetClassName(npcStr);
	npc_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	
	// Static functions on the npc object
	//npc_ctor->Set(v8::String::NewFromUtf8(isolate, "create"), v8::FunctionTemplate::New(isolate, Npc_createFunction, engine_ref));
	
	// Method functions
	// TODO(joey): Implement these functions
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "blockagain"), v8::FunctionTemplate::New(isolate, NPC_Function_BlockAgain, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "canwarp"), v8::FunctionTemplate::New(isolate, NPC_Function_CanWarp, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "cannotwarp"), v8::FunctionTemplate::New(isolate, NPC_Function_CannotWarp, engine_ref));
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "destroy"), v8::FunctionTemplate::New(isolate, NPC_Function_Destroy, engine_ref));
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "dontblock"), v8::FunctionTemplate::New(isolate, NPC_Function_DontBlock, engine_ref));
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "hide"), v8::FunctionTemplate::New(isolate, NPC_Function_Hide, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "message"), v8::FunctionTemplate::New(isolate, NPC_Function_Message, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "move"), v8::FunctionTemplate::New(isolate, NPC_Function_Move, engine_ref));
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "noplayeronwall"), v8::FunctionTemplate::New(isolate, NPC_Function_NoPlayerOnWall, engine_ref));
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "setimg"), v8::FunctionTemplate::New(isolate, NPC_Function_SetImg, engine_ref)); // setimg(filename);
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "setimgpart"), v8::FunctionTemplate::New(isolate, NPC_Function_SetImgPart, engine_ref)); // setimgpart(filename,offsetx,offsety,width,height);
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "showcharacter"), v8::FunctionTemplate::New(isolate, NPC_Function_ShowCharacter, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "setcharprop"), v8::FunctionTemplate::New(isolate, NPC_Function_SetCharProp, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "setshape"), v8::FunctionTemplate::New(isolate, NPC_Function_SetShape, engine_ref)); // setshape(1, pixelWidth, pixelHeight)
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "show"), v8::FunctionTemplate::New(isolate, NPC_Function_Show, engine_ref));
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "warpto"), v8::FunctionTemplate::New(isolate, NPC_Function_Warpto, engine_ref)); // warpto levelname,x,y;
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "registerTrigger"), v8::FunctionTemplate::New(isolate, NPC_Function_RegisterTrigger, engine_ref));

	// Properties
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "ani"), NPC_GetStr_Ani, NPC_SetStr_Ani);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "ap"), NPC_GetInt_Ap, NPC_SetInt_Ap);
	//npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "body"), NPC_GetInt_Ap, NPC_SetInt_Ap);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "bombs"), NPC_GetInt_Bombs, NPC_SetInt_Bombs);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "chat"), NPC_GetStr_Message, NPC_SetStr_Message);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "darts"), NPC_GetInt_Darts, NPC_SetInt_Darts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "dir"), NPC_GetInt_Dir, NPC_SetInt_Dir);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "glovepower"), NPC_GetInt_GlovePower, NPC_SetInt_GlovePower);
	//npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "head"), NPC_GetInt_Ap, NPC_SetInt_Ap);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "hearts"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
//	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "height"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
//	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "horseimg"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), NPC_GetInt_Id);
//	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "image"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "level"), NPC_GetObject_Level);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "levelname"), NPC_GetStr_LevelName);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "name"), NPC_GetStr_Name);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "nick"), NPC_GetStr_Nickname, NPC_SetStr_Nickname);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "rupees"), NPC_GetInt_Rupees, NPC_SetInt_Rupees);
//	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "save"), NPC_GetObject_Save);
//	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "shieldimg"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
//	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "swordimg"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "timeout"), NPC_GetNum_Timeout, NPC_SetNum_Timeout);
//	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "width"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "x"), NPC_GetNum_X, NPC_SetNum_X);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "y"), NPC_GetNum_Y, NPC_SetNum_Y);

	// Persist the npc constructor
	env->SetConstructor(ScriptConstructorId<TNPC>::result, npc_ctor);

	// DISABLED: it would just allow scripts to construct npcs, better off disabled?
	// Set the npc constructor on the global object
	//v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();
	//global->Set(npcStr, npc_ctor);
}

#endif

