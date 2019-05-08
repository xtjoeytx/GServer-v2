#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include <unordered_map>
#include "IUtil.h"
#include "CScriptEngine.h"
#include "TNPC.h"

#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

// PROPERTY: Id
void NPC_GetInt_id(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);
	info.GetReturnValue().Set(npcObject->getId());
}

// PROPERTY: X Position
void NPC_GetNum_X(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);
	info.GetReturnValue().Set((double)npcObject->getPixelX() / 16.0f);
}

void NPC_SetNum_X(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	int newValueInt = 16 * (int)newValue;
	if (newValueInt < 0) {
		newValueInt = (-newValueInt << 1) | 0x0001;
	}
	else newValueInt <<= 1;

	npcObject->setProps(CString() >> (char)NPCPROP_X2 >> (short)newValueInt, CLVER_2_17, true);
}

// PROPERTY: Y Position
void NPC_GetNum_Y(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);
	info.GetReturnValue().Set((double)npcObject->getPixelY() / 16.0f);
}

void NPC_SetNum_Y(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	double newValue = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	int newValueInt = 16 * (int)newValue;
	if (newValueInt < 0) {
		newValueInt = (-newValueInt << 1) | 0x0001;
	}
	else newValueInt <<= 1;

	npcObject->setProps(CString() >> (char)NPCPROP_Y2 >> (short)newValueInt, CLVER_2_17, true);
}

// PROPERTY: Timeout
void NPC_GetNum_timeout(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	double timeout = npcObject->getTimeout() / 20;
	info.GetReturnValue().Set(timeout);
}

void NPC_SetNum_timeout(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	double timeout = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setTimeout((int)(timeout * 20));
}

// PROPERTY: Rupees
void NPC_GetInt_Rupees(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_RUPEES);
	info.GetReturnValue().Set(npcProp.readGUInt());
}

void NPC_SetInt_Rupees(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_RUPEES >> (int)newValue, CLVER_2_17, true);
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

// PROPERTY: Nickname
void NPC_GetStr_Nickname(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	CString npcProp = npcObject->getProp(NPCPROP_NICKNAME);
	CString propValue = npcProp.readChars(npcProp.readGUChar());
	
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Nickname(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
	v8::Local<v8::Object> self = info.This();
	TNPC *npcObject = UnwrapObject<TNPC>(self);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	npcObject->setProps(CString() >> (char)NPCPROP_NICKNAME >> (char)newValue.length() << *newValue, CLVER_2_17, true);
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

// NPC Method: npc.showcharacter();
void NPC_Function_ShowCharacter(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Unwrap Object
	TNPC *npcObject = UnwrapObject<TNPC>(args.This());
	npcObject->setProps(CString() >> (char)NPCPROP_IMAGE >> (char)3 << "#c#", CLVER_2_17, true);
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
	npc_ctor->Set(v8::String::NewFromUtf8(isolate, "create"), v8::FunctionTemplate::New(isolate, Npc_createFunction, engine_ref));
	
	// Method functions
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "test"), v8::FunctionTemplate::New(isolate, Npc_testFunction, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "canwarp"), v8::FunctionTemplate::New(isolate, NPC_Function_CanWarp, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "cannotwarp"), v8::FunctionTemplate::New(isolate, NPC_Function_CannotWarp, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "message"), v8::FunctionTemplate::New(isolate, NPC_Function_Message, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "showcharacter"), v8::FunctionTemplate::New(isolate, NPC_Function_ShowCharacter, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "setcharprop"), v8::FunctionTemplate::New(isolate, NPC_Function_SetCharProp, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "registerTrigger"), v8::FunctionTemplate::New(isolate, NPC_Function_RegisterTrigger, engine_ref));

	// Properties...?
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), NPC_GetInt_id);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "x"), NPC_GetNum_X, NPC_SetNum_X);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "y"), NPC_GetNum_Y, NPC_SetNum_Y);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "timeout"), NPC_GetNum_timeout, NPC_SetNum_timeout);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "rupees"), NPC_GetInt_Rupees, NPC_SetInt_Rupees);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "bombs"), NPC_GetInt_Bombs, NPC_SetInt_Bombs);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "darts"), NPC_GetInt_Darts, NPC_SetInt_Darts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "hearts"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "glovepower"), NPC_GetInt_GlovePower, NPC_SetInt_GlovePower);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "dir"), NPC_GetInt_Dir, NPC_SetInt_Dir);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "ap"), NPC_GetInt_Ap, NPC_SetInt_Ap);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "chat"), NPC_GetStr_Message, NPC_SetStr_Message);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "nick"), NPC_GetStr_Nickname, NPC_SetStr_Nickname);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "ani"), NPC_GetStr_Ani, NPC_SetStr_Ani);
	
	// Persist the npc constructor
	env->SetConstructor(ScriptConstructorId<TNPC>::result, npc_ctor);

	// DISABLED: it would just allow scripts to construct npcs, better off disabled?
	// Set the npc constructor on the global object
	//v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();
	//global->Set(npcStr, npc_ctor);
}

#endif

