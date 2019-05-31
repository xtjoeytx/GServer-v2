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
void NPC_GetInt_Id(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	info.GetReturnValue().Set(npcObject->getId());
}

// Property: npc.name
void NPC_GetStr_Name(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Local<v8::String> npcName = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getName().c_str());
	info.GetReturnValue().Set(npcName);
}

// Property: npc.x
void NPC_GetNum_X(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	info.GetReturnValue().Set((double)npcObject->getPixelX() / 16.0f);
}

void NPC_SetNum_X(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	float newValue = (float)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setX(newValue);
	//npcObject->updatePropModTime(NPCPROP_X);
	npcObject->updatePropModTime(NPCPROP_X2);
}

// Property: npc.y
void NPC_GetNum_Y(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	info.GetReturnValue().Set((double)npcObject->getPixelY() / 16.0f);
}

void NPC_SetNum_Y(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	float newValue = (float)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setY(newValue);

	//npcObject->updatePropModTime(NPCPROP_Y);
	npcObject->updatePropModTime(NPCPROP_Y2);
}

// PROPERTY: Level
void NPC_GetObject_Level(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

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
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	TLevel *npcLevel = npcObject->getLevel();
	CString levelName("");
	if (npcLevel != 0)
		levelName = npcLevel->getLevelName();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), levelName.text());
	info.GetReturnValue().Set(strText);
}

// PROPERTY: Timeout
void NPC_GetNum_Timeout(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	double timeout = npcObject->getTimeout() / 20;
	info.GetReturnValue().Set(timeout);
}

void NPC_SetNum_Timeout(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	double timeout = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setTimeout((int)(timeout * 20));
}

// PROPERTY: Rupees
void NPC_GetInt_Rupees(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	info.GetReturnValue().Set(npcObject->getRupees());
}

void NPC_SetInt_Rupees(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setRupees(newValue);
	npcObject->updatePropModTime(NPCPROP_RUPEES);
}

// PROPERTY: Bombs
void NPC_GetInt_Bombs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_BOMBS);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Bombs(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_BOMBS >> (char)clip(newValue, 0, 99), CLVER_2_17, true);
}

// PROPERTY: Darts
void NPC_GetInt_Darts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_ARROWS);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Darts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_ARROWS >> (char)clip(newValue, 0, 99), CLVER_2_17, true);
}

// PROPERTY: Hearts
void NPC_GetInt_Hearts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_POWER);
	info.GetReturnValue().Set((float)npcProp.readGUChar() / 2.0f);
}

void NPC_SetInt_Hearts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	int newValue = (int)(value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked() * 2);
	npcObject->setProps(CString() >> (char)NPCPROP_POWER >> (char)clip(newValue, 0, 40), CLVER_2_17, true);
}

// PROPERTY: npc.height
void NPC_GetInt_Height(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	info.GetReturnValue().Set(npcObject->getHeight());
}

// PROPERTY: npc.width
void NPC_GetInt_Width(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	info.GetReturnValue().Set(npcObject->getWidth());
}

// PROPERTY: Glove Power
void NPC_GetInt_GlovePower(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_GLOVEPOWER);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_GlovePower(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_GLOVEPOWER >> (char)clip(newValue, 0, 3), CLVER_2_17, true);
}

// PROPERTY: Dir
void NPC_GetInt_Dir(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_SPRITE);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Dir(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_SPRITE >> (char)(newValue % 4), CLVER_2_17, true);
}

// PROPERTY: Ap
void NPC_GetInt_Alignment(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_ALIGNMENT);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Alignment(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	int newValue = clip(value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked(), 0, 100);
	npcObject->setProps(CString() >> (char)NPCPROP_ALIGNMENT >> (char)newValue, CLVER_2_17, true);
}

// PROPERTY: npc.bodyimg
void NPC_GetStr_BodyImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getBodyImage().text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_BodyImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)NPCPROP_BODYIMAGE >> (char)len;
	propPackage.write(*newValue, len);
	npcObject->setProps(propPackage, CLVER_2_17, true);
}

// PROPERTY: npc.headimg
void NPC_GetStr_HeadImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getHeadImage().text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_HeadImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)NPCPROP_HEADIMAGE >> (char)len;
	propPackage.write(*newValue, len);
	npcObject->setProps(propPackage, CLVER_2_17, true);
}

// PROPERTY: npc.horseimg
void NPC_GetStr_HorseImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getHorseImage().text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_HorseImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)NPCPROP_HORSEIMAGE >> (char)len;
	propPackage.write(*newValue, len);
	npcObject->setProps(propPackage, CLVER_2_17, true);
}

// PROPERTY: npc.image
void NPC_GetStr_Image(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getImage().text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Image(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	npcObject->setProps(CString() >> (char)NPCPROP_IMAGE >> (char)newValue.length() << *newValue, CLVER_2_17, true);
}

// PROPERTY: npc.nick
void NPC_GetStr_Nickname(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getNickname().text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Nickname(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	npcObject->setNickname(*newValue);
}

// PROPERTY: npc.shieldimg
void NPC_GetStr_ShieldImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getShieldImage().text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_ShieldImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)NPCPROP_SHIELDIMAGE >> (char)len;
	propPackage.write(*newValue, len);
	npcObject->setProps(propPackage, CLVER_2_17, true);
}

// PROPERTY: npc.swordimg
void NPC_GetStr_SwordImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getSwordImage().text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_SwordImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)NPCPROP_SWORDIMAGE >> (char)len;
	propPackage.write(*newValue, len);
	npcObject->setProps(propPackage, CLVER_2_17, true);
}

// PROPERTY: Message
void NPC_GetStr_Message(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_MESSAGE);
	CString propValue = npcProp.readChars(npcProp.readGUChar());

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Message(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	npcObject->setProps(CString() >> (char)NPCPROP_MESSAGE >> (char)newValue.length() << *newValue, CLVER_2_17, true);
}

// PROPERTY: Animation
void NPC_GetStr_Ani(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_GANI);
	CString propValue = npcProp.readChars(npcProp.readGUChar());

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text());
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Ani(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	npcObject->setProps(CString() >> (char)NPCPROP_GANI >> (char)newValue.length() << *newValue, CLVER_2_17, true);
}

// NPC Method: npc.canwarp();
void NPC_Function_CanWarp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	npcObject->allowNpcWarping(true);
}

// NPC Method: npc.cannotwarp();
void NPC_Function_CannotWarp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	npcObject->allowNpcWarping(false);
}

// NPC Method: npc.destroy();
void NPC_Function_Destroy(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	bool status = npcObject->deleteNPC();
	args.GetReturnValue().Set(status);
}

// NPC Method: npc.blockagain();
void NPC_Function_BlockAgain(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	npcObject->setBlockingFlags(NPCBLOCKFLAG_BLOCK);
	npcObject->updatePropModTime(NPCPROP_BLOCKFLAGS);
}

// NPC Method: npc.dontblock();
void NPC_Function_DontBlock(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	npcObject->setBlockingFlags(NPCBLOCKFLAG_NOBLOCK);
	npcObject->updatePropModTime(NPCPROP_BLOCKFLAGS);
}

// NPC Method: npc.drawoverplayer();
void NPC_Function_DrawOverPlayer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	// Toggle flags
	int flags = npcObject->getVisibleFlags();
	flags = (flags | NPCVISFLAG_DRAWOVERPLAYER) & ~(NPCVISFLAG_DRAWUNDERPLAYER);
	
	npcObject->setVisibleFlags(flags);
	npcObject->updatePropModTime(NPCPROP_VISFLAGS);
}

// NPC Method: npc.drawunderplayer();
void NPC_Function_DrawUnderPlayer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	// Toggle flags
	int flags = npcObject->getVisibleFlags();
	flags = (flags | NPCVISFLAG_DRAWUNDERPLAYER) & ~(NPCVISFLAG_DRAWOVERPLAYER);

	npcObject->setVisibleFlags(flags);
	npcObject->updatePropModTime(NPCPROP_VISFLAGS);
}

// NPC Method: npc.hide();
void NPC_Function_Hide(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	// Toggle flags
	int flags = npcObject->getVisibleFlags();
	npcObject->setVisibleFlags(flags & ~(NPCVISFLAG_VISIBLE));
	npcObject->updatePropModTime(NPCPROP_VISFLAGS);
}

// NPC Method: npc.show();
void NPC_Function_Show(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	// Toggle flags
	int flags = npcObject->getVisibleFlags();
	npcObject->setVisibleFlags(flags | NPCVISFLAG_VISIBLE);
	npcObject->updatePropModTime(NPCPROP_VISFLAGS);
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
		V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

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

	// Unwrap object
	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Argument parsing
	int delta_x = (int)(16 * args[0]->NumberValue(context).ToChecked());
	int delta_y = (int)(16 * args[1]->NumberValue(context).ToChecked());
	double time_fps = (int)(args[2]->NumberValue(context).ToChecked());
	int options = args[3]->Int32Value(context).ToChecked();

	npcObject->moveNPC(delta_x, delta_y, time_fps, options);
}

// NPC Method: npc.showcharacter();
void NPC_Function_ShowCharacter(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Unwrap object
	V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

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
		// Unwrap object
		V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

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
					CString propPackage;
					propPackage >> (char)NPCPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4];
					npcObject->setProps(propPackage, CLVER_2_17, true);
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

	SCRIPTENV_D("Begin NPC::registerAction()\n");

	if (args[0]->IsString() && args[1]->IsFunction())
	{
		SCRIPTENV_D(" - Register npc action %s with: %s\n",
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

	SCRIPTENV_D("End NPC::registerAction()\n");
}

void NPC_Function_Join(const v8::FunctionCallbackInfo<v8::Value>& args)
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
		CString classCode = server->getClass(className);

		if (!classCode.isEmpty())
		{
			V8ENV_SAFE_UNWRAP(args, TNPC, npcObject);

			// Split the code
			std::string serverCode = classCode.readString("//#CLIENTSIDE").text();
			std::string clientCode = classCode.readString("").text();

			// Add class to npc
			npcObject->addClassCode(className, clientCode);

			// Wrap code
			std::string classCodeWrap = CScriptEngine::WrapScript<TNPC>(serverCode);

			// TODO(joey): maybe we shouldn't cache this using this method, since classes can be used with
			// multiple wrappers.
			IScriptFunction *function = scriptEngine->CompileCache(classCodeWrap, false);
			if (function == nullptr)
				return;

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

// Called when javascript creates a new object
// js example: let jsNpc = new NPC();
//void Npc_Constructor(const v8::FunctionCallbackInfo<v8::Value>& args)
//{
//	// TODO(joey): more proof of concept, likely to get axed.
//	
//	// Called by V8. and should return an NPC object
//	SCRIPTENV_D("Npc_Constructor called\n");
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
		SCRIPTENV_D("Npc::createFunction: %s\n", *v8::String::Utf8Value(isolate, args[0]->ToString(isolate)));
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
			SCRIPTENV_D(" - Returned Value from Callback: %s\n", *(v8::String::Utf8Value(isolate, ret.ToLocalChecked())));
	}

	SCRIPTENV_D("Npc::testFunction: %s\n", *v8::String::Utf8Value(isolate, args[0]->ToString(isolate)));
}

// PROPERTY: NPC Attributes
// TODO(joey): use lazy property instead? TBD
//void NPC_GetObject_Attrs2(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
//{
//	//printf("Called getattr\n");
//
//	v8::Isolate *isolate = info.GetIsolate();
//	v8::Local<v8::Context> context = isolate->GetCurrentContext();
//	v8::Local<v8::Object> self = info.This();
//
//	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);
//
//	v8::Local<v8::String> internalAttr = v8::String::NewFromUtf8(isolate, "_internalAttr", v8::NewStringType::kInternalized).ToLocalChecked();
//
//	v8::Local<v8::FunctionTemplate> ctor_tpl = PersistentToLocal(isolate, _persist_npc_attrs_ctor);
//	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
//	new_instance->SetAlignedPointerInInternalField(0, npcObject);
//
//	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
//	V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>(npcObject->getScriptObject());
//	v8_wrapped->addChild("attr", new_instance);
//
//	v8::PropertyAttribute propAttr = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
//	self->DefineOwnProperty(context, internalAttr, new_instance, propAttr).FromJust();
//	info.GetReturnValue().Set(new_instance);
//}

void NPC_GetObject_Attrs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalAttr = v8::String::NewFromUtf8(isolate, "_internalAttr", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalAttr).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalAttr).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("npc.attr");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, npcObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>(npcObject->getScriptObject());
	v8_wrapped->addChild("attr", new_instance);

	v8::PropertyAttribute propAttr = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalAttr, new_instance, propAttr).FromJust();
	info.GetReturnValue().Set(new_instance);
}

const char __nAttrPackets[30] = { 36, 37, 38, 39, 40, 44, 45, 46, 47, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 };

void NPC_Attrs_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index < 1 || index > 30)
		return;
	index--;

	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Isolate *isolate = info.GetIsolate();

	// TODO(joey): Object is not getting unset here.

	CString npcAttr = npcObject->getProp(__nAttrPackets[index]);
	CString npcAttrValue = npcAttr.readChars(npcAttr.readGUChar());

	// Get server flag with the property
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, npcAttrValue.text());
	info.GetReturnValue().Set(strText);
}

void NPC_Attrs_Setter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index < 1 || index > 30)
		return;
	index--;

	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Isolate *isolate = info.GetIsolate();

	// Get new value
	v8::String::Utf8Value newValue(isolate, value);
	int strLength = newValue.length();
	if (strLength > 223)
		strLength = 223;

	npcObject->setProps(CString() >> (char)__nAttrPackets[index] >> (char)strLength << *newValue, CLVER_2_17, true);

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
}

// PROPERTY: npc.flags
void NPC_GetObject_Flags(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalFlags = v8::String::NewFromUtf8(isolate, "_internalFlags", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalFlags).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalFlags).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("npc.flags");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, npcObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>(npcObject->getScriptObject());
	v8_wrapped->addChild("flags", new_instance);

	v8::PropertyAttribute propAttr = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalFlags, new_instance, propAttr).FromJust();
	info.GetReturnValue().Set(new_instance);
}

void NPC_Flags_Getter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Isolate *isolate = info.GetIsolate();

	// Get property name
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get server flag with the property
	CString flagValue = npcObject->getFlag(*utf8);
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, flagValue.text());
	info.GetReturnValue().Set(strText);
}

void NPC_Flags_Setter(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Isolate *isolate = info.GetIsolate();

	// Get property name
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get new value
	v8::String::Utf8Value newValue(isolate, value);
	npcObject->setFlag(*utf8, *newValue);

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
}

void NPC_Flags_Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Get flags list
	auto flagList = npcObject->getFlagList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)flagList->size());

	int idx = 0;
	for (auto it = flagList->begin(); it != flagList->end(); ++it)
		result->Set(context, idx++, v8::String::NewFromUtf8(isolate, it->first.c_str())).Check();

	info.GetReturnValue().Set(result);
}

// PROPERTY: npc.save
void NPC_GetObject_Save(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalSave = v8::String::NewFromUtf8(isolate, "_internalSave", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalSave).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalSave).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("npc.save");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, npcObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptWrapped<TNPC> *v8_wrapped = static_cast<V8ScriptWrapped<TNPC> *>(npcObject->getScriptObject());
	v8_wrapped->addChild("save", new_instance);

	v8::PropertyAttribute propSave = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalSave, new_instance, propSave).FromJust();
	info.GetReturnValue().Set(new_instance);
}

void NPC_Save_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index > 9)
		return;

	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	int npcSaveValue = npcObject->getSave(index);
	info.GetReturnValue().Set(npcSaveValue);
}

void NPC_Save_Setter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index < 0 || index > 9)
		return;

	V8ENV_SAFE_UNWRAP(info, TNPC, npcObject);

	// Get new value
	unsigned int newValue = value->Uint32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	if (newValue > 223)
		newValue = 223;

	npcObject->setSave(index, newValue);
	npcObject->updatePropModTime(NPCPROP_SAVE0 + index);

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
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
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "destroy"), v8::FunctionTemplate::New(isolate, NPC_Function_Destroy, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "dontblock"), v8::FunctionTemplate::New(isolate, NPC_Function_DontBlock, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "drawoverplayer"), v8::FunctionTemplate::New(isolate, NPC_Function_DrawOverPlayer, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "drawunderplayer"), v8::FunctionTemplate::New(isolate, NPC_Function_DrawUnderPlayer, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "hide"), v8::FunctionTemplate::New(isolate, NPC_Function_Hide, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "message"), v8::FunctionTemplate::New(isolate, NPC_Function_Message, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "move"), v8::FunctionTemplate::New(isolate, NPC_Function_Move, engine_ref));
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "noplayeronwall"), v8::FunctionTemplate::New(isolate, NPC_Function_NoPlayerOnWall, engine_ref));
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "setimg"), v8::FunctionTemplate::New(isolate, NPC_Function_SetImg, engine_ref)); // setimg(filename);
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "setimgpart"), v8::FunctionTemplate::New(isolate, NPC_Function_SetImgPart, engine_ref)); // setimgpart(filename,offsetx,offsety,width,height);
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "showcharacter"), v8::FunctionTemplate::New(isolate, NPC_Function_ShowCharacter, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "setcharprop"), v8::FunctionTemplate::New(isolate, NPC_Function_SetCharProp, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "setshape"), v8::FunctionTemplate::New(isolate, NPC_Function_SetShape, engine_ref)); // setshape(1, pixelWidth, pixelHeight)
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "show"), v8::FunctionTemplate::New(isolate, NPC_Function_Show, engine_ref));
//	npc_proto->Set(v8::String::NewFromUtf8(isolate, "warpto"), v8::FunctionTemplate::New(isolate, NPC_Function_Warpto, engine_ref)); // warpto levelname,x,y;

	npc_proto->Set(v8::String::NewFromUtf8(isolate, "join"), v8::FunctionTemplate::New(isolate, NPC_Function_Join, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8(isolate, "registerTrigger"), v8::FunctionTemplate::New(isolate, NPC_Function_RegisterTrigger, engine_ref));

	// Properties
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "ani"), NPC_GetStr_Ani, NPC_SetStr_Ani);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "ap"), NPC_GetInt_Alignment, NPC_SetInt_Alignment);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "bodyimg"), NPC_GetStr_BodyImage, NPC_SetStr_BodyImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "bombs"), NPC_GetInt_Bombs, NPC_SetInt_Bombs);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "chat"), NPC_GetStr_Message, NPC_SetStr_Message);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "darts"), NPC_GetInt_Darts, NPC_SetInt_Darts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "dir"), NPC_GetInt_Dir, NPC_SetInt_Dir);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "glovepower"), NPC_GetInt_GlovePower, NPC_SetInt_GlovePower);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "headimg"), NPC_GetStr_HeadImage, NPC_SetStr_HeadImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "hearts"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "height"), NPC_GetInt_Height);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "horseimg"), NPC_GetStr_HorseImage, NPC_SetStr_HorseImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "id"), NPC_GetInt_Id);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "image"), NPC_GetStr_Image, NPC_SetStr_Image);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "level"), NPC_GetObject_Level);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "levelname"), NPC_GetStr_LevelName);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "name"), NPC_GetStr_Name);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "nick"), NPC_GetStr_Nickname, NPC_SetStr_Nickname);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "rupees"), NPC_GetInt_Rupees, NPC_SetInt_Rupees);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "shieldimg"), NPC_GetStr_ShieldImage, NPC_SetStr_ShieldImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "swordimg"), NPC_GetStr_SwordImage, NPC_SetStr_SwordImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "timeout"), NPC_GetNum_Timeout, NPC_SetNum_Timeout);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "width"), NPC_GetInt_Width);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "x"), NPC_GetNum_X, NPC_SetNum_X);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "y"), NPC_GetNum_Y, NPC_SetNum_Y);

	//npc_ctor->InstanceTemplate()->SetLazyDataProperty(v8::String::NewFromUtf8(isolate, "attr"), NPC_GetObject_Attrs2, v8::Local<v8::Value>(), static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete));
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "attr"), NPC_GetObject_Attrs, nullptr, engine_ref);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "flags"), NPC_GetObject_Flags, nullptr, engine_ref);
	npc_proto->SetAccessor(v8::String::NewFromUtf8(isolate, "save"), NPC_GetObject_Save, nullptr, engine_ref);

	// Create the npc-attributes flags template
	v8::Local<v8::FunctionTemplate> npc_attrs_ctor = v8::FunctionTemplate::New(isolate);
	npc_attrs_ctor->SetClassName(v8::String::NewFromUtf8(isolate, "attr"));
	npc_attrs_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	npc_attrs_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
		NPC_Attrs_Getter, NPC_Attrs_Setter, nullptr, nullptr, nullptr, v8::Local<v8::Value>(),
		v8::PropertyHandlerFlags::kNone));
	env->SetConstructor("npc.attr", npc_attrs_ctor);

	// Create the npc flags template
	v8::Local<v8::FunctionTemplate> npc_flags_ctor = v8::FunctionTemplate::New(isolate);
	npc_flags_ctor->SetClassName(v8::String::NewFromUtf8(isolate, "flags"));
	npc_flags_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	npc_flags_ctor->InstanceTemplate()->SetHandler(v8::NamedPropertyHandlerConfiguration(
		NPC_Flags_Getter, NPC_Flags_Setter, nullptr, nullptr, NPC_Flags_Enumerator, v8::Local<v8::Value>(),
		v8::PropertyHandlerFlags::kOnlyInterceptStrings));
	env->SetConstructor("npc.flags", npc_flags_ctor);

	// Create the npc saves template
	v8::Local<v8::FunctionTemplate> npc_save_ctor = v8::FunctionTemplate::New(isolate);
	npc_save_ctor->SetClassName(v8::String::NewFromUtf8(isolate, "save"));
	npc_save_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	npc_save_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
			NPC_Save_Getter, NPC_Save_Setter, nullptr, nullptr, nullptr, v8::Local<v8::Value>(),
			v8::PropertyHandlerFlags::kNone));
	env->SetConstructor("npc.save", npc_save_ctor);

	// Persist the npc constructor
	env->SetConstructor(ScriptConstructorId<TNPC>::result, npc_ctor);

	// DISABLED: it would just allow scripts to construct npcs, better off disabled?
	// Set the npc constructor on the global object
	//v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();
	//global->Set(npcStr, npc_ctor);
}

#endif

