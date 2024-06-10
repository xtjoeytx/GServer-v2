#ifdef V8NPCSERVER

	#include "ScriptEngine.h"
	#include "IUtil.h"
	#include "Level.h"
	#include "NPC.h"
	#include <algorithm>
	#include <cassert>
	#include <unordered_map>
	#include <v8.h>

	#include "V8ScriptFunction.h"
	#include "V8ScriptObject.h"

// Property: npc.id
void NPC_GetInt_Id(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	info.GetReturnValue().Set(npcObject->getId());
}

// Property: npc.name
void NPC_GetStr_Name(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Local<v8::String> npcName = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getName().c_str()).ToLocalChecked();
	info.GetReturnValue().Set(npcName);
}

// Property: npc.x
void NPC_GetNum_X(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	info.GetReturnValue().Set(npcObject->getX() / 16.0);
}

void NPC_SetNum_X(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int newValue = int(16 * value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked());
	npcObject->setX(newValue);
	//npcObject->updatePropModTime(NPCPROP_X);
	npcObject->updatePropModTime(NPCPROP_X2);
}

// Property: npc.y
void NPC_GetNum_Y(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	info.GetReturnValue().Set(npcObject->getY() / 16.0);
}

void NPC_SetNum_Y(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int newValue = int(16 * (float)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked());
	npcObject->setY(newValue);

	//npcObject->updatePropModTime(NPCPROP_Y);
	npcObject->updatePropModTime(NPCPROP_Y2);
}

// PROPERTY: Level
void NPC_GetObject_Level(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	if (!npcObject->getIsNpcDeleteRequested())
	{
		auto npcLevel = npcObject->getLevel();
		if (npcLevel != nullptr)
		{
			auto* v8_wrapped = static_cast<V8ScriptObject<Level>*>(npcLevel->getScriptObject());
			info.GetReturnValue().Set(v8_wrapped->handle(info.GetIsolate()));
			return;
		}
	}

	info.GetReturnValue().SetNull();
}

// PROPERTY: LevelName
void NPC_GetStr_LevelName(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	auto npcLevel = npcObject->getLevel();
	CString levelName("");
	if (npcLevel != nullptr)
		levelName = npcLevel->getLevelName();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), levelName.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

// PROPERTY: Timeout
void NPC_GetNum_Timeout(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	double timeout = npcObject->getTimeout() / 20;
	info.GetReturnValue().Set(timeout);
}

void NPC_SetNum_Timeout(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	double timeout = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setTimeout((int)(timeout * 20));
}

// PROPERTY: Rupees
void NPC_GetInt_Rupees(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	info.GetReturnValue().Set(npcObject->getRupees());
}

void NPC_SetInt_Rupees(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setRupees(newValue);
	npcObject->updatePropModTime(NPCPROP_RUPEES);
}

// PROPERTY: Bombs
void NPC_GetInt_Bombs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_BOMBS);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Bombs(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_BOMBS >> (char)clip(newValue, 0, 99), CLVER_2_17, true);
}

// PROPERTY: Darts
void NPC_GetInt_Darts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_ARROWS);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Darts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_ARROWS >> (char)clip(newValue, 0, 99), CLVER_2_17, true);
}

// PROPERTY: Hearts
void NPC_GetInt_Hearts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_POWER);
	info.GetReturnValue().Set((float)npcProp.readGUChar() / 2.0f);
}

void NPC_SetInt_Hearts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int newValue = (int)(value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked() * 2);
	npcObject->setProps(CString() >> (char)NPCPROP_POWER >> (char)clip(newValue, 0, 40), CLVER_2_17, true);
}

// PROPERTY: npc.height
void NPC_GetInt_Height(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	info.GetReturnValue().Set(npcObject->getHeight());
}

// PROPERTY: npc.width
void NPC_GetInt_Width(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	info.GetReturnValue().Set(npcObject->getWidth());
}

// PROPERTY: npc.glovepower
void NPC_GetInt_GlovePower(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_GLOVEPOWER);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_GlovePower(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setProps(CString() >> (char)NPCPROP_GLOVEPOWER >> (char)clip(newValue, 0, 3), CLVER_2_17, true);
}

// PROPERTY: npc.dir
void NPC_GetInt_Dir(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	info.GetReturnValue().Set(npcObject->getSprite());
}

void NPC_SetInt_Dir(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	npcObject->setSprite(newValue % 4);
	npcObject->updatePropModTime(NPCPROP_SPRITE);
}

// PROPERTY: npc.ap
void NPC_GetInt_Alignment(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	CString npcProp = npcObject->getProp(NPCPROP_ALIGNMENT);
	info.GetReturnValue().Set(npcProp.readGUChar());
}

void NPC_SetInt_Alignment(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int newValue = clip(value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked(), 0, 100);
	npcObject->setProps(CString() >> (char)NPCPROP_ALIGNMENT >> (char)newValue, CLVER_2_17, true);
}

// PROPERTY: npc.bodyimg
void NPC_GetStr_BodyImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	auto& message = npcObject->getBodyImage();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), message.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_BodyImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	npcObject->setBodyImage(std::string(*newValue, newValue.length()));
	npcObject->updatePropModTime(NPCPROP_BODYIMAGE);
}

// PROPERTY: npc.headimg
void NPC_GetStr_HeadImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getHeadImage().text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_HeadImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	npcObject->setHeadImage(std::string(*newValue, newValue.length()));
	npcObject->updatePropModTime(NPCPROP_HEADIMAGE);
}

// PROPERTY: npc.horseimg
void NPC_GetStr_HorseImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getHorseImage().text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_HorseImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	npcObject->setHorseImage(std::string(*newValue, newValue.length()));
	npcObject->updatePropModTime(NPCPROP_HORSEIMAGE);
}

// PROPERTY: npc.image
void NPC_GetStr_Image(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getImage().c_str()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Image(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	npcObject->setImage(std::string(*newValue, newValue.length()));
	npcObject->updatePropModTime(NPCPROP_IMAGE);
}

// PROPERTY: npc.nick
void NPC_GetStr_Nickname(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getNickname().c_str()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Nickname(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	npcObject->setNickname(std::string(*newValue, newValue.length()));
	npcObject->updatePropModTime(NPCPROP_NICKNAME);
}

// PROPERTY: npc.shieldimg
void NPC_GetStr_ShieldImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getShieldImage().text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_ShieldImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	npcObject->setShieldImage(std::string(*newValue, newValue.length()));
	npcObject->updatePropModTime(NPCPROP_SHIELDIMAGE);
}

// PROPERTY: npc.swordimg
void NPC_GetStr_SwordImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), npcObject->getSwordImage().text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_SwordImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	npcObject->setSwordImage(std::string(*newValue, newValue.length()));
	npcObject->updatePropModTime(NPCPROP_SWORDIMAGE);
}

// PROPERTY: Message
void NPC_GetStr_Message(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	auto& message = npcObject->getChat();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), message.c_str()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Message(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	npcObject->setChat(std::string(*newValue, newValue.length()));
	npcObject->updatePropModTime(NPCPROP_MESSAGE);
}

// PROPERTY: Animation
void NPC_GetStr_Ani(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	auto& propValue = npcObject->getGani();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.c_str()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_SetStr_Ani(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	npcObject->setGani(std::string(*newValue, newValue.length()));
	npcObject->updatePropModTime(NPCPROP_GANI);
}

// NPC Method: npc.canwarp();
void NPC_Function_CanWarp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	npcObject->allowNpcWarping(NPCWarpType::AllLinks);
}

// NPC Method: npc.canwarp2();
void NPC_Function_CanWarp2(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	npcObject->allowNpcWarping(NPCWarpType::OverworldLinks);
}

// NPC Method: npc.cannotwarp();
void NPC_Function_CannotWarp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	npcObject->allowNpcWarping(NPCWarpType::None);
}

// NPC Method: npc.destroy();
void NPC_Function_Destroy(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	bool status = npcObject->deleteNPC();
	args.GetReturnValue().Set(status);
}

// NPC Method: npc.blockagain();
void NPC_Function_BlockAgain(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	npcObject->setBlockingFlags(NPCBLOCKFLAG_BLOCK);
	npcObject->updatePropModTime(NPCPROP_BLOCKFLAGS);
}

// NPC Method: npc.dontblock();
void NPC_Function_DontBlock(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	npcObject->setBlockingFlags(NPCBLOCKFLAG_NOBLOCK);
	npcObject->updatePropModTime(NPCPROP_BLOCKFLAGS);
}

// NPC Method: npc.drawoverplayer();
void NPC_Function_DrawOverPlayer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	// Toggle flags
	int flags = npcObject->getVisibleFlags();
	flags = (flags | NPCVISFLAG_DRAWOVERPLAYER) & ~(NPCVISFLAG_DRAWUNDERPLAYER);

	npcObject->setVisibleFlags(flags);
	npcObject->updatePropModTime(NPCPROP_VISFLAGS);
}

// NPC Method: npc.drawunderplayer();
void NPC_Function_DrawUnderPlayer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	// Toggle flags
	int flags = npcObject->getVisibleFlags();
	flags = (flags | NPCVISFLAG_DRAWUNDERPLAYER) & ~(NPCVISFLAG_DRAWOVERPLAYER);

	npcObject->setVisibleFlags(flags);
	npcObject->updatePropModTime(NPCPROP_VISFLAGS);
}

// NPC Method: npc.hide();
void NPC_Function_Hide(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	// Toggle flags
	int flags = npcObject->getVisibleFlags();
	npcObject->setVisibleFlags(flags & ~(NPCVISFLAG_VISIBLE));
	npcObject->updatePropModTime(NPCPROP_VISFLAGS);
}

// NPC Method: npc.show();
void NPC_Function_Show(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	// Toggle flags
	int flags = npcObject->getVisibleFlags();
	npcObject->setVisibleFlags(flags | NPCVISFLAG_VISIBLE);
	npcObject->updatePropModTime(NPCPROP_VISFLAGS);
}

// NPC Method: npc.message();
void NPC_Function_Message(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Validate arguments
	if (args.Length() == 0)
	{
		V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

		npcObject->setChat("");
		npcObject->updatePropModTime(NPCPROP_MESSAGE);
	}
	else if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());

		npcObject->setChat(std::string(*newValue, newValue.length()));
		npcObject->updatePropModTime(NPCPROP_MESSAGE);
	}
}

// NPC Method: npc.move(x, y, time, options);
void NPC_Function_Move(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 4);

	// Unwrap object
	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Argument parsing
	int dx = int(16.0 * args[0]->NumberValue(context).ToChecked());
	int dy = int(16.0 * args[1]->NumberValue(context).ToChecked());
	double time_fps = args[2]->NumberValue(context).ToChecked();
	int options = args[3]->Int32Value(context).ToChecked();

	npcObject->moveNPC(dx, dy, time_fps, options);
}

// NPC Method: npc.setimg(image);
void NPC_Function_SetImg(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// Unwrap object
	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	if (args[0]->IsString())
	{
		v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
		v8::String::Utf8Value image(isolate, args[0]->ToString(context).ToLocalChecked());

		npcObject->setImage(*image);
		npcObject->updatePropModTime(NPCPROP_IMAGE);
		npcObject->updatePropModTime(NPCPROP_IMAGEPART);
	}
}

// NPC Method: npc.setimgpart(filename,offsetx,offsety,width,height)
void NPC_Function_SetImgPart(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 5);

	// Unwrap object
	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	if (args[0]->IsString())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::String::Utf8Value image(isolate, args[0]->ToString(context).ToLocalChecked());

		// TODO(joey): may need to check the types individually
		int offsetx = args[1]->Int32Value(context).ToChecked();
		int offsety = args[2]->Int32Value(context).ToChecked();
		int width = args[3]->Int32Value(context).ToChecked();
		int height = args[4]->Int32Value(context).ToChecked();

		npcObject->setImage(std::string(*image, image.length()), offsetx, offsety, width, height);
		npcObject->updatePropModTime(NPCPROP_IMAGE);
		npcObject->updatePropModTime(NPCPROP_IMAGEPART);
	}
}

// NPC Method: npc.showcharacter();
void NPC_Function_ShowCharacter(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Unwrap object
	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	npcObject->setImage("#c#");
	npcObject->setHeadImage("head0.png");
	npcObject->setBodyImage("body.png");
	npcObject->setShieldImage("shield1.png");
	npcObject->setSwordImage("sword1.png");
	npcObject->setColorId(0, 2);  // orange
	npcObject->setColorId(1, 5);  // dark red
	npcObject->setColorId(2, 21); // black
	npcObject->setColorId(3, 5);  // dark red
	npcObject->setColorId(4, 21); // black
	npcObject->setWidth(32);
	npcObject->setHeight(48);

	npcObject->updatePropModTime(NPCPROP_IMAGE);
	npcObject->updatePropModTime(NPCPROP_IMAGEPART);
	npcObject->updatePropModTime(NPCPROP_HEADIMAGE);
	npcObject->updatePropModTime(NPCPROP_BODYIMAGE);
	npcObject->updatePropModTime(NPCPROP_SHIELDIMAGE);
	npcObject->updatePropModTime(NPCPROP_SWORDIMAGE);
	npcObject->updatePropModTime(NPCPROP_COLORS);
}

// NPC Method: npc.setani("walk", "ani", "params");
void NPC_Function_SetAni(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 1);

	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());

		CString animation(*newValue);
		for (int i = 1; i < args.Length(); i++)
		{
			if (args[i]->IsString() || args[i]->IsNumber())
			{
				v8::String::Utf8Value aniParam(isolate, args[i]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
				animation << "," << *aniParam;
			}
		}

		npcObject->setGani(std::string(animation.text(), animation.length()));
		npcObject->updatePropModTime(NPCPROP_GANI);
	}
}

// NPC Method: npc.setcharprop(code, value);
void NPC_Function_SetCharProp(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	CString code = *v8::String::Utf8Value(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
	v8::String::Utf8Value newValue(isolate, args[1]->ToString(isolate->GetCurrentContext()).ToLocalChecked());

	auto len = newValue.length();
	if (len > 223)
		len = 223;

	if (code[0] == '#')
	{
		// Unwrap object
		V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

		switch (code[1])
		{
			case '1': // sword image
			{
				npcObject->setSwordImage(std::string(*newValue, newValue.length()));
				npcObject->updatePropModTime(NPCPROP_SWORDIMAGE);
				break;
			}

			case '2': // shield image
			{
				npcObject->setShieldImage(std::string(*newValue, newValue.length()));
				npcObject->updatePropModTime(NPCPROP_SHIELDIMAGE);
				break;
			}

			case '3': // head image
			{
				npcObject->setHeadImage(std::string(*newValue, newValue.length()));
				npcObject->updatePropModTime(NPCPROP_HEADIMAGE);
				break;
			}

			case '5': // horse image (needs to be tested)
				npcObject->setHorseImage(std::string(*newValue, newValue.length()));
				npcObject->updatePropModTime(NPCPROP_HORSEIMAGE);
				break;

			case '8': // body image
				npcObject->setBodyImage(std::string(*newValue, newValue.length()));
				npcObject->updatePropModTime(NPCPROP_BODYIMAGE);
				break;

			case 'c': // chat
				npcObject->setChat(std::string(*newValue, newValue.length()));
				npcObject->updatePropModTime(NPCPROP_MESSAGE);
				break;

			case 'n': // nickname
				npcObject->setNickname(std::string(*newValue, newValue.length()));
				npcObject->updatePropModTime(NPCPROP_NICKNAME);
				break;

			case 'C': // colors
			{
				if (code[2] >= '0' && code[2] < '5')
				{
					npcObject->setColorId(code[2] - '0', getColor(*newValue));
					npcObject->updatePropModTime(NPCPROP_COLORS);
				}
				break;
			}
		}
	}
}

// NPC Method: npc.settimer(time);
void NPC_Function_SetTimer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	if (args[0]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

		double timeout = args[0]->NumberValue(isolate->GetCurrentContext()).ToChecked();
		npcObject->setTimeout((int)(timeout * 20));
	}
}

// NPC Method: npc.setshape(type, pixelWidth, pixelHeight);
void NPC_Function_SetShape(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	// Throw an exception on constructor calls for method functions
	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	// Throw an exception if we don't receive the specified arguments
	V8ENV_THROW_ARGCOUNT(args, isolate, 3);

	if (args[0]->IsInt32() && args[1]->IsInt32() && args[2]->IsInt32())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();

		// Unwrap Object
		NPC* npcObject = unwrapObject<NPC>(args.This());

		int width = args[1]->Int32Value(context).ToChecked();
		int height = args[2]->Int32Value(context).ToChecked();
		npcObject->setWidth(width);
		npcObject->setHeight(height);
	}
}

// NPC Method: npc.registerAction(string, function);
void NPC_Function_RegisterTrigger(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 2);

	SCRIPTENV_D("Begin NPC::registerAction()\n");

	if (args[0]->IsString() && args[1]->IsFunction())
	{
		SCRIPTENV_D(" - Register npc action %s with: %s\n",
					*v8::String::Utf8Value(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked()),
					*v8::String::Utf8Value(isolate, args[1]->ToString(isolate->GetCurrentContext()).ToLocalChecked()));

		v8::Local<v8::External> data = args.Data().As<v8::External>();
		ScriptEngine* scriptEngine = static_cast<ScriptEngine*>(data->Value());

		V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

		// Callback name
		std::string eventName = *v8::String::Utf8Value(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
		std::transform(eventName.begin(), eventName.end(), eventName.begin(), ::tolower);

		// Persist the callback function so we can retrieve it later on
		v8::Local<v8::Function> cbFunc = args[1].As<v8::Function>();
		V8ScriptFunction* cbFuncWrapper = new V8ScriptFunction(env, cbFunc);

		// Unwrap Object
		NPC* npcObject = unwrapObject<NPC>(args.This());
		npcObject->registerTriggerAction(eventName, cbFuncWrapper);
	}

	SCRIPTENV_D("End NPC::registerAction()\n");
}

// NPC Method: npc.scheduleevent(time, function);
void NPC_Function_ScheduleEvent(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 2);

	SCRIPTENV_D("Begin NPC::registerAction()\n");

	if (args[0]->IsNumber() && args[1]->IsFunction())
	{
		V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

		SCRIPTENV_D(" - Register npc schedule event %s with: %s\n",
					*v8::String::Utf8Value(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked()),
					*v8::String::Utf8Value(isolate, args[1]->ToString(isolate->GetCurrentContext()).ToLocalChecked()));

		v8::Local<v8::Context> context = isolate->GetCurrentContext();

		v8::Local<v8::External> data = args.Data().As<v8::External>();
		ScriptEngine* scriptEngine = static_cast<ScriptEngine*>(data->Value());

		V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

		// Callback name
		double time_til = args[0]->NumberValue(context).ToChecked();
		int timer_frames = (int)(time_til * 20);

		// Persist the callback function so we can retrieve it later on
		v8::Local<v8::Function> cbFunc = args[1].As<v8::Function>();
		V8ScriptFunction* cbFuncWrapper = new V8ScriptFunction(env, cbFunc);

		IScriptArguments* v8args;
		if (args.Length() > 2)
		{
			v8::Local<v8::Object> paramData = args[2]->ToObject(context).ToLocalChecked();

			auto v8ScriptData = std::make_shared<V8ScriptData>(env, paramData);
			v8args = ScriptFactory::createArguments(env, npcObject->getScriptObject(), std::move(v8ScriptData));
		}
		else
			v8args = ScriptFactory::createArguments(env, npcObject->getScriptObject());

		ScriptAction action(cbFuncWrapper, v8args, "_scheduleevent");
		npcObject->scheduleEvent(timer_frames, action);
		scriptEngine->registerNpcTimer(npcObject);
	}

	SCRIPTENV_D("End NPC::registerAction()\n");
}

	#include "ScriptClass.h"
	#include "V8ScriptWrappers.h"

// NPC Function: NPC.join("class");
void NPC_Function_Join(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	if (args[0]->IsString())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::External> data = args.Data().As<v8::External>();
		ScriptEngine* scriptEngine = static_cast<ScriptEngine*>(data->Value());

		std::string className = *v8::String::Utf8Value(isolate, args[0]->ToString(context).ToLocalChecked());

		V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

		ScriptClass* classObject = npcObject->joinClass(className);
		if (classObject != nullptr)
		{
			auto classCodeWrap = wrapScript<NPC>(classObject->getSource().getServerSide());
			auto scriptFunction = scriptEngine->compileCache(classCodeWrap, false);

			if (scriptFunction != nullptr)
			{
				V8ScriptFunction* v8_function = static_cast<V8ScriptFunction*>(scriptFunction);
				v8::Local<v8::Value> newArgs[] = { args.This() };

				// Execute
				v8::TryCatch try_catch(isolate);
				v8::Local<v8::Function> localFunction = v8_function->function();
				v8::MaybeLocal<v8::Value> scriptTableRet = localFunction->Call(context, args.This(), 1, newArgs);
				if (!scriptTableRet.IsEmpty())
				{
					args.GetReturnValue().Set(scriptTableRet.ToLocalChecked());
					return;
				}
			}
		}

		//Server *server = scriptEngine->getServer();
		//auto classObj = server->getClass(className);

		//if (classObj && !classObj->source().empty())
		//{
		//	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

		//	// Split the code
		//	std::string serverCode = classObj->serverCode();
		//	std::string clientCode = classObj->clientCode();

		//	//auto currentClass = server->getClassObject(className);
		//	//if (currentClass == nullptr)
		//	//	currentClass = server->addClass(className, clientCode);
		//	//npcObject->addClassCode(className, clientCode);
		//	// Add class to npc
		//	//npcObject->addClassCode(className, clientCode);
		//
		//	// Wrap code
		//	std::string classCodeWrap = ScriptEngine::wrapScript<NPC>(serverCode);

		//	// TODO(joey): maybe we shouldn't cache this using this method, since classes can be used with
		//	// multiple wrappers.
		//	IScriptFunction *function = scriptEngine->compileCache(classCodeWrap, false);
		//	if (function == nullptr)
		//		return;

		//	V8ScriptFunction *v8_function = static_cast<V8ScriptFunction *>(function);
		//	v8::Local<v8::Value> newArgs[] = { args.This() };

		//	// Execute
		//	v8::TryCatch try_catch(isolate);
		//	v8::Local<v8::Function> scriptFunction = v8_function->Function();
		//	v8::MaybeLocal<v8::Value> scriptTableRet = scriptFunction->Call(context, args.This(), 1, newArgs);
		//	if (!scriptTableRet.IsEmpty())
		//	{
		//		args.GetReturnValue().Set(scriptTableRet.ToLocalChecked());
		//		return;
		//	}

		//	// TODO(joey): error handling
		//}
	}
}

void NPC_Function_SetPM(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

	// Exclusive for Control-NPC
	if (npcObject->getName() != "Control-NPC")
		return;

	v8::Local<v8::External> data = args.Data().As<v8::External>();
	ScriptEngine* scriptEngine = static_cast<ScriptEngine*>(data->Value());

	if (args[0]->IsFunction())
	{
		V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

		// Persist the callback function so we can retrieve it later on
		v8::Local<v8::Function> cbFunc = args[0].As<v8::Function>();
		V8ScriptFunction* cbFuncWrapper = new V8ScriptFunction(env, cbFunc);

		// Set pm function
		scriptEngine->getServer()->setPMFunction(npcObject->getId(), cbFuncWrapper);
	}
	else
	{
		scriptEngine->getServer()->setPMFunction(0);
	}
}

void NPC_Function_Warpto(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 3);

	// warpto levelname,x,y;
	if (args[0]->IsString() && args[1]->IsNumber() && args[2]->IsNumber())
	{
		V8ENV_SAFE_UNWRAP(args, NPC, npcObject);

		if (npcObject->isWarpable())
		{
			v8::Local<v8::Context> context = isolate->GetCurrentContext();

			v8::String::Utf8Value levelName(isolate, args[0]->ToString(context).ToLocalChecked());
			int newX = int(16.0 * args[1]->NumberValue(context).ToChecked());
			int newY = int(16.0 * args[2]->NumberValue(context).ToChecked());

			v8::Local<v8::External> data = args.Data().As<v8::External>();
			ScriptEngine* scriptEngine = static_cast<ScriptEngine*>(data->Value());
			Server* server = scriptEngine->getServer();

			auto level = server->getLevel(*levelName);
			if (level != nullptr)
			{
				npcObject->warpNPC(level, newX, newY);
				args.GetReturnValue().Set(true);
				return;
			}
		}
	}

	args.GetReturnValue().Set(false);
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
//	ScriptEngine *scriptEngine = static_cast<ScriptEngine *>(data->Value());
//
//	NPC *newNpc = new NPC(scriptEngine->getServer());
//
//	assert(args.This()->InternalFieldCount() > 0);
//
//	args.This()->SetAlignedPointerInInternalField(0, newNpc);
//	args.GetReturnValue().Set(args.This());
//}

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
//	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);
//
//	v8::Local<v8::String> internalAttr = v8::String::NewFromUtf8(isolate, "_internalAttr", v8::NewStringType::kInternalized).ToLocalChecked();
//
//	v8::Local<v8::FunctionTemplate> ctor_tpl = persistentToLocal(isolate, _persist_npc_attrs_ctor);
//	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
//	new_instance->SetAlignedPointerInInternalField(0, npcObject);
//
//	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
//	V8ScriptObject<NPC> *v8_wrapped = static_cast<V8ScriptObject<NPC> *>(npcObject->getScriptObject());
//	v8_wrapped->addChild("attr", new_instance);
//
//	v8::PropertyAttribute propAttr = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
//	self->DefineOwnProperty(context, internalAttr, new_instance, propAttr).FromJust();
//	info.GetReturnValue().Set(new_instance);
//}

void NPC_GetObject_Attrs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalAttr = v8::String::NewFromUtf8(isolate, "_internalAttr", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalAttr).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalAttr).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	ScriptEngine* scriptEngine = static_cast<ScriptEngine*>(data->Value());
	V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->getConstructor("npc.attr");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, npcObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptObject<NPC>* v8_wrapped = static_cast<V8ScriptObject<NPC>*>(npcObject->getScriptObject());
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

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Isolate* isolate = info.GetIsolate();

	// TODO(joey): Object is not getting unset here.

	CString npcAttr = npcObject->getProp(__nAttrPackets[index]);
	CString npcAttrValue = npcAttr.readChars(npcAttr.readGUChar());

	// Get server flag with the property
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, npcAttrValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_Attrs_Setter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index < 1 || index > 30)
		return;
	index--;

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Isolate* isolate = info.GetIsolate();

	// Get new value
	v8::String::Utf8Value newValue(isolate, value);
	int strLength = newValue.length();
	if (strLength > 223)
		strLength = 223;

	npcObject->setProps(CString() >> (char)__nAttrPackets[index] >> (char)strLength << *newValue, CLVER_2_17, true);

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
}

// PROPERTY: npc.colors
void NPC_GetObject_Colors(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalName = v8::String::NewFromUtf8(isolate, "_internalColors", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalName).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalName).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	ScriptEngine* scriptEngine = static_cast<ScriptEngine*>(data->Value());
	V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->getConstructor("npc.colors");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, npcObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptObject<NPC>* v8_wrapped = static_cast<V8ScriptObject<NPC>*>(npcObject->getScriptObject());
	v8_wrapped->addChild("colors", new_instance);

	v8::PropertyAttribute propAttributes = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalName, new_instance, propAttributes).FromJust();
	info.GetReturnValue().Set(new_instance);
}

void NPC_Colors_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index > 4)
		return;

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int colorValue = npcObject->getColorId(index);
	info.GetReturnValue().Set(colorValue);
}

void NPC_Colors_Setter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index > 4)
		return;

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	char colorIndex;

	if (value->IsUint32())
	{
		// Get new value
		unsigned int newValue = value->Uint32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
		if (newValue > 32) // Unsure how many colors exist, capping at 32 for now
			newValue = 32;

		colorIndex = static_cast<char>(newValue);
	}
	else // if (value->IsString())
	{
		v8::String::Utf8Value newValue(info.GetIsolate(), value);
		colorIndex = getColor(*newValue);
		if (colorIndex < 0)
			return;
	}

	npcObject->setColorId(index, colorIndex);
	npcObject->updatePropModTime(NPCPROP_COLORS);

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
}

// PROPERTY: npc.flags
void NPC_GetObject_Flags(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalFlags = v8::String::NewFromUtf8(isolate, "_internalFlags", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalFlags).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalFlags).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	ScriptEngine* scriptEngine = static_cast<ScriptEngine*>(data->Value());
	V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->getConstructor("npc.flags");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, npcObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptObject<NPC>* v8_wrapped = static_cast<V8ScriptObject<NPC>*>(npcObject->getScriptObject());
	v8_wrapped->addChild("flags", new_instance);

	v8::PropertyAttribute propAttr = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalFlags, new_instance, propAttr).FromJust();
	info.GetReturnValue().Set(new_instance);
}

void NPC_Flags_Getter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Isolate* isolate = info.GetIsolate();

	// Get property name
	v8::Local<v8::String> name = v8::Local<v8::String>::Cast(property);
	v8::String::Utf8Value utf8(isolate, name);

	// Get server flag with the property
	CString flagValue = npcObject->getFlag(*utf8);
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, flagValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void NPC_Flags_Setter(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Isolate* isolate = info.GetIsolate();

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
	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Get flags list
	auto& flagList = npcObject->getFlagList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)flagList.size());

	int idx = 0;
	for (auto& flag: flagList)
		result->Set(context, idx++, v8::String::NewFromUtf8(isolate, flag.first.c_str()).ToLocalChecked()).Check();

	info.GetReturnValue().Set(result);
}

// PROPERTY: npc.save
void NPC_GetObject_Save(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate* isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalSave = v8::String::NewFromUtf8(isolate, "_internalSave", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalSave).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalSave).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	ScriptEngine* scriptEngine = static_cast<ScriptEngine*>(data->Value());
	V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->getConstructor("npc.save");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, npcObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptObject<NPC>* v8_wrapped = static_cast<V8ScriptObject<NPC>*>(npcObject->getScriptObject());
	v8_wrapped->addChild("save", new_instance);

	v8::PropertyAttribute propSave = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalSave, new_instance, propSave).FromJust();
	info.GetReturnValue().Set(new_instance);
}

void NPC_Save_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index > 9)
		return;

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	int npcSaveValue = npcObject->getSave(index);
	info.GetReturnValue().Set(npcSaveValue);
}

void NPC_Save_Setter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index > 9)
		return;

	V8ENV_SAFE_UNWRAP(info, NPC, npcObject);

	// Get new value
	unsigned int newValue = value->Uint32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	if (newValue > 223)
		newValue = 223;

	npcObject->setSave(index, static_cast<unsigned char>(newValue));
	npcObject->updatePropModTime(NPCPROP_SAVE0 + index);

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
}

void bindClass_NPC(ScriptEngine* scriptEngine)
{
	// Retrieve v8 environment
	auto* env = dynamic_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());
	v8::Isolate* isolate = env->isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "npc"
	v8::Local<v8::String> npcStr = v8::String::NewFromUtf8Literal(isolate, "npc", v8::NewStringType::kInternalized);

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> npc_ctor = v8::FunctionTemplate::New(isolate, nullptr, engine_ref);
	v8::Local<v8::ObjectTemplate> npc_proto = npc_ctor->PrototypeTemplate();

	npc_ctor->SetClassName(npcStr);
	npc_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Static functions on the npc object
	//npc_ctor->Set(v8::String::NewFromUtf8(isolate, "create"), v8::FunctionTemplate::New(isolate, Npc_createFunction, engine_ref));

	// Method functions
	// TODO(joey): Implement these functions
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "blockagain"), v8::FunctionTemplate::New(isolate, NPC_Function_BlockAgain, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "canwarp"), v8::FunctionTemplate::New(isolate, NPC_Function_CanWarp, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "canwarp2"), v8::FunctionTemplate::New(isolate, NPC_Function_CanWarp2, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "cannotwarp"), v8::FunctionTemplate::New(isolate, NPC_Function_CannotWarp, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "destroy"), v8::FunctionTemplate::New(isolate, NPC_Function_Destroy, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "dontblock"), v8::FunctionTemplate::New(isolate, NPC_Function_DontBlock, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "drawoverplayer"), v8::FunctionTemplate::New(isolate, NPC_Function_DrawOverPlayer, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "drawunderplayer"), v8::FunctionTemplate::New(isolate, NPC_Function_DrawUnderPlayer, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "hide"), v8::FunctionTemplate::New(isolate, NPC_Function_Hide, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "message"), v8::FunctionTemplate::New(isolate, NPC_Function_Message, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "move"), v8::FunctionTemplate::New(isolate, NPC_Function_Move, engine_ref));
	//	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "noplayeronwall"), v8::FunctionTemplate::New(isolate, NPC_Function_NoPlayerOnWall, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setimg"), v8::FunctionTemplate::New(isolate, NPC_Function_SetImg, engine_ref));         // setimg(filename);
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setimgpart"), v8::FunctionTemplate::New(isolate, NPC_Function_SetImgPart, engine_ref)); // setimgpart(filename,offsetx,offsety,width,height);
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "showcharacter"), v8::FunctionTemplate::New(isolate, NPC_Function_ShowCharacter, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setani"), v8::FunctionTemplate::New(isolate, NPC_Function_SetAni, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setcharani"), v8::FunctionTemplate::New(isolate, NPC_Function_SetAni, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setcharprop"), v8::FunctionTemplate::New(isolate, NPC_Function_SetCharProp, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "settimer"), v8::FunctionTemplate::New(isolate, NPC_Function_SetTimer, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setshape"), v8::FunctionTemplate::New(isolate, NPC_Function_SetShape, engine_ref)); // setshape(1, pixelWidth, pixelHeight)
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "show"), v8::FunctionTemplate::New(isolate, NPC_Function_Show, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "warpto"), v8::FunctionTemplate::New(isolate, NPC_Function_Warpto, engine_ref)); // warpto levelname,x,y;

	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "join"), v8::FunctionTemplate::New(isolate, NPC_Function_Join, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "registerTrigger"), v8::FunctionTemplate::New(isolate, NPC_Function_RegisterTrigger, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setpm"), v8::FunctionTemplate::New(isolate, NPC_Function_SetPM, engine_ref));
	npc_proto->Set(v8::String::NewFromUtf8Literal(isolate, "scheduleevent"), v8::FunctionTemplate::New(isolate, NPC_Function_ScheduleEvent, engine_ref));

	// Properties
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "ani"), NPC_GetStr_Ani, NPC_SetStr_Ani);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "ap"), NPC_GetInt_Alignment, NPC_SetInt_Alignment);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "bodyimg"), NPC_GetStr_BodyImage, NPC_SetStr_BodyImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "bombs"), NPC_GetInt_Bombs, NPC_SetInt_Bombs);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "chat"), NPC_GetStr_Message, NPC_SetStr_Message);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "darts"), NPC_GetInt_Darts, NPC_SetInt_Darts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "dir"), NPC_GetInt_Dir, NPC_SetInt_Dir);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "glovepower"), NPC_GetInt_GlovePower, NPC_SetInt_GlovePower);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "headimg"), NPC_GetStr_HeadImage, NPC_SetStr_HeadImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "hearts"), NPC_GetInt_Hearts, NPC_SetInt_Hearts);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "height"), NPC_GetInt_Height);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "horseimg"), NPC_GetStr_HorseImage, NPC_SetStr_HorseImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "id"), NPC_GetInt_Id);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "image"), NPC_GetStr_Image, NPC_SetStr_Image);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "level"), NPC_GetObject_Level);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "levelname"), NPC_GetStr_LevelName);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "name"), NPC_GetStr_Name);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "nick"), NPC_GetStr_Nickname, NPC_SetStr_Nickname);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "rupees"), NPC_GetInt_Rupees, NPC_SetInt_Rupees);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "shieldimg"), NPC_GetStr_ShieldImage, NPC_SetStr_ShieldImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "swordimg"), NPC_GetStr_SwordImage, NPC_SetStr_SwordImage);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "timeout"), NPC_GetNum_Timeout, NPC_SetNum_Timeout);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "width"), NPC_GetInt_Width);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "x"), NPC_GetNum_X, NPC_SetNum_X);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "y"), NPC_GetNum_Y, NPC_SetNum_Y);

	//npc_ctor->InstanceTemplate()->SetLazyDataProperty(v8::String::NewFromUtf8(isolate, "attr"), NPC_GetObject_Attrs2, v8::Local<v8::Value>(), static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete));
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "attr"), NPC_GetObject_Attrs, nullptr, engine_ref);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "colors"), NPC_GetObject_Colors, nullptr, engine_ref);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "flags"), NPC_GetObject_Flags, nullptr, engine_ref);
	npc_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "save"), NPC_GetObject_Save, nullptr, engine_ref);

	// Create the npc-attributes flags template
	v8::Local<v8::FunctionTemplate> npc_attrs_ctor = v8::FunctionTemplate::New(isolate);
	npc_attrs_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "attr"));
	npc_attrs_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	npc_attrs_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
		NPC_Attrs_Getter, NPC_Attrs_Setter, nullptr, nullptr, nullptr, v8::Local<v8::Value>(),
		v8::PropertyHandlerFlags::kNone));
	env->setConstructor("npc.attr", npc_attrs_ctor);

	// Create the npc colors template
	v8::Local<v8::FunctionTemplate> npc_colors_ctor = v8::FunctionTemplate::New(isolate);
	npc_colors_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "colors"));
	npc_colors_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	npc_colors_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
		NPC_Colors_Getter, NPC_Colors_Setter, nullptr, nullptr, nullptr, v8::Local<v8::Value>(),
		v8::PropertyHandlerFlags::kNone));
	env->setConstructor("npc.colors", npc_colors_ctor);

	// Create the npc flags template
	v8::Local<v8::FunctionTemplate> npc_flags_ctor = v8::FunctionTemplate::New(isolate);
	npc_flags_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "flags"));
	npc_flags_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	npc_flags_ctor->InstanceTemplate()->SetHandler(v8::NamedPropertyHandlerConfiguration(
		NPC_Flags_Getter, NPC_Flags_Setter, nullptr, nullptr, NPC_Flags_Enumerator, v8::Local<v8::Value>(),
		v8::PropertyHandlerFlags::kOnlyInterceptStrings));
	env->setConstructor("npc.flags", npc_flags_ctor);

	// Create the npc saves template
	v8::Local<v8::FunctionTemplate> npc_save_ctor = v8::FunctionTemplate::New(isolate);
	npc_save_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "save"));
	npc_save_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	npc_save_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
		NPC_Save_Getter, NPC_Save_Setter, nullptr, nullptr, nullptr, v8::Local<v8::Value>(),
		v8::PropertyHandlerFlags::kNone));
	env->setConstructor("npc.save", npc_save_ctor);

	// Persist the npc constructor
	env->setConstructor(ScriptConstructorId<NPC>::result, npc_ctor);

	// DISABLED: it would just allow scripts to construct npcs, better off disabled?
	// Set the npc constructor on the global object
	//v8::Local<v8::ObjectTemplate> global = env->globalTemplate();
	//global->Set(npcStr, npc_ctor);
}

#endif
