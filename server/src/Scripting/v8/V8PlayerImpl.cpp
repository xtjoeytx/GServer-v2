#ifdef V8NPCSERVER

#include <cassert>
#include <v8.h>
#include <stdio.h>
#include "IUtil.h"
#include "CScriptEngine.h"
#include "TLevel.h"
#include "TNPC.h"
#include "TPlayer.h"
#include "TServer.h"

#include "V8ScriptFunction.h"
#include "V8ScriptObject.h"

// PROPERTY: player.id
void Player_GetInt_Id(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getId());
}

// PROPERTY: player.account
void Player_GetStr_Account(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	CString accountName = playerObject->getAccountName();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), accountName.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

// PROPERTY: player.ani
void Player_GetStr_Ani(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	const CString& animation = playerObject->getAnimation();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), animation.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_Ani(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)PLPROP_GANI >> (char)len;
	propPackage.write(*newValue, len);
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.ap
void Player_GetInt_Alignment(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getAlignment());
}

void Player_SetInt_Alignment(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	char newValue = (char)value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	playerObject->setProps(CString() >> (char)PLPROP_ALIGNMENT >> (char)newValue, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.bodyimg
void Player_GetStr_BodyImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	const CString& propValue = playerObject->getBodyImage();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_BodyImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)PLPROP_BODYIMG >> (char)len;
	propPackage.write(*newValue, len);
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.bombs
void Player_GetInt_Bombs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getBombCount());
}

void Player_SetInt_Bombs(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	char newValue = (char)value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	playerObject->setProps(CString() >> (char)PLPROP_BOMBSCOUNT >> (char)newValue, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.chat
void Player_GetStr_Chat(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	const CString& propValue = playerObject->getChatMsg();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_Chat(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)PLPROP_CURCHAT >> (char)len;
	propPackage.write(*newValue, len);
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.darts
void Player_GetInt_Darts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getArrowCount());
}

void Player_SetInt_Darts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	char newValue = (char)value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	playerObject->setProps(CString() >> (char)PLPROP_ARROWSCOUNT >> (char)newValue, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.dir
void Player_GetInt_Dir(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getSprite() % 4);
}

void Player_SetInt_Dir(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	char newValue = (char)(value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked() % 4);
	playerObject->setProps(CString() >> (char)PLPROP_SPRITE >> (char)newValue, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}


// PROPERTY: player.fullhearts
void Player_GetInt_Fullhearts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getMaxPower());
}

void Player_SetInt_Fullhearts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	int newValue = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	playerObject->setProps(CString() >> (char)PLPROP_MAXPOWER >> (char)clip(newValue, 0, 20), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.glovepower
void Player_GetInt_GlovePower(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getGlovePower());
}

void Player_SetInt_GlovePower(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	char newValue = (char)value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	playerObject->setProps(CString() >> (char)PLPROP_GLOVEPOWER >> (char)newValue, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.guild
void Player_GetStr_Guild(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	const CString& propValue = playerObject->getGuild();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_Guild(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::String::Utf8Value newValue = v8::String::Utf8Value(info.GetIsolate(), value);

	CString playerNick = playerObject->getNickname();
	int pos = playerNick.find("(", 0);
	if (pos != -1)
		playerNick = playerNick.readChars(pos).trimRight();
	if (newValue.length() > 0)
		playerNick << " (" << *newValue << ")";

	int len = playerNick.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)PLPROP_NICKNAME >> (char)len;
	propPackage.write(playerNick.text(), len);
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.hearts
void Player_GetNum_Hearts(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getPower());
}

void Player_SetNum_Hearts(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	int newValue = (int)(value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked() * 2);
	playerObject->setProps(CString() >> (char)PLPROP_CURPOWER >> (char)clip(newValue, 0, 40), PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.headimg
void Player_GetStr_HeadImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	const CString& propValue = playerObject->getHeadImage();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_HeadImage(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 123)
		len = 123;

	CString propPackage;
	propPackage >> (char)PLPROP_HEADGIF >> (char)(len + 100);
	propPackage.write(*newValue, len);
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
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

// PROPERTY: player.isstaff
void Player_GetBool_IsStaff(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->isStaff());
}

// PROPERTY: player.level
void Player_GetObject_Level(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	TLevel *levelObject = playerObject->getLevel();

	if (levelObject != 0)
	{
		V8ScriptObject<TLevel> *v8_wrapped = static_cast<V8ScriptObject<TLevel> *>(levelObject->getScriptObject());
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

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), levelName.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

// PROPERTY: player.mp
void Player_GetInt_MagicPower(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getMagicPower());
}

void Player_SetInt_MagicPower(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	unsigned char newValue = (unsigned char)value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	playerObject->setProps(CString() >> (char)PLPROP_MAGICPOINTS >> (char)newValue, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.nickname
void Player_GetStr_Nickname(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	const CString& propValue = playerObject->getNickname();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_Nickname(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::String::Utf8Value newValue = v8::String::Utf8Value(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)PLPROP_NICKNAME >> (char)len;
	propPackage.write(*newValue, len);
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.platform
void Player_GetString_Platform(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	const CString& propValue = playerObject->getPlatform();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
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
	playerObject->setProps(CString() >> (char)PLPROP_RUPEESCOUNT >> (int)newValue, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.shieldimg
void Player_GetStr_ShieldImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	const CString& propValue = playerObject->getShieldImage();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_ShieldImage(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::String::Utf8Value newValue = v8::String::Utf8Value(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)PLPROP_SHIELDPOWER >> (char)(playerObject->getShieldPower() + 10) >> (char)len;
	propPackage.write(*newValue, len);
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.shieldpower
void Player_GetInt_ShieldPower(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getShieldPower());
}

void Player_SetInt_ShieldPower(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	char newValue = (char)value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	const CString& shieldImg = playerObject->getShieldImage();

	CString propPackage;
	propPackage >> (char)PLPROP_SHIELDPOWER >> (char)(newValue + 10);
	propPackage >> (char)shieldImg.length() << shieldImg;
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.swordimg
void Player_GetStr_SwordImage(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	const CString& propValue = playerObject->getSwordImage();

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), propValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Player_SetStr_SwordImage(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::String::Utf8Value newValue = v8::String::Utf8Value(info.GetIsolate(), value);
	int len = newValue.length();
	if (len > 223)
		len = 223;

	CString propPackage;
	propPackage >> (char)PLPROP_SWORDPOWER >> (char)(playerObject->getSwordPower() + 30) >> (char)len;
	propPackage.write(*newValue, len);
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.swordpower
void Player_GetInt_SwordPower(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	info.GetReturnValue().Set(playerObject->getSwordPower());
}

void Player_SetInt_SwordPower(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	char newValue = (char)value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
	const CString& swordImg = playerObject->getSwordImage();

	CString propPackage;
	propPackage >> (char)PLPROP_SWORDPOWER >> (char)(newValue + 30);
	propPackage >> (char)swordImg.length() << swordImg;
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.x
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

	playerObject->setProps(CString() >> (char)PLPROP_X2 >> (short)newValueInt, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.y
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

	playerObject->setProps(CString() >> (char)PLPROP_Y2 >> (short)newValueInt, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

// PROPERTY: player.attr
void Player_GetObject_Attrs(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
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

	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	CScriptEngine* scriptEngine = static_cast<CScriptEngine*>(data->Value());
	V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("player.attr");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, playerObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptObject<TPlayer> *v8_wrapped = static_cast<V8ScriptObject<TPlayer> *>(playerObject->getScriptObject());
	v8_wrapped->addChild("attr", new_instance);

	v8::PropertyAttribute propAttr = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalAttr, new_instance, propAttr).FromJust();
	info.GetReturnValue().Set(new_instance);
}

const char __pAttrPackets[30] = { 37, 38, 39, 40, 41, 46, 47, 48, 49, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74 };

void Player_Attr_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index < 1 || index > 30)
		return;
	index--;

	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::Isolate* isolate = info.GetIsolate();

	CString playerAttr = playerObject->getProp(__pAttrPackets[index]);
	CString playerAttrValue = playerAttr.readChars(playerAttr.readGUChar());

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, playerAttrValue.text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Player_Attr_Setter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index < 1 || index > 30)
		return;
	index--;

	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::Isolate* isolate = info.GetIsolate();

	// Get new value
	v8::String::Utf8Value newValue(isolate, value);
	int strLength = newValue.length();
	if (strLength > 223)
		strLength = 223;

	CString propPackage;
	propPackage.writeGChar(__pAttrPackets[index]);
	propPackage.writeGChar(strLength);
	propPackage.write(*newValue, strLength);
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
}

// PROPERTY: player.colors
void Player_GetObject_Colors(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> self = info.This();

	v8::Local<v8::String> internalName = v8::String::NewFromUtf8(isolate, "_internalColors", v8::NewStringType::kInternalized).ToLocalChecked();
	if (self->HasRealNamedProperty(context, internalName).ToChecked())
	{
		info.GetReturnValue().Set(self->Get(context, internalName).ToLocalChecked());
		return;
	}

	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("player.colors");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, playerObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptObject<TPlayer> *v8_wrapped = static_cast<V8ScriptObject<TPlayer> *>(playerObject->getScriptObject());
	v8_wrapped->addChild("colors", new_instance);

	v8::PropertyAttribute propAttributes = static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum);
	self->DefineOwnProperty(context, internalName, new_instance, propAttributes).FromJust();
	info.GetReturnValue().Set(new_instance);
}

void Player_Colors_Getter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index > 4)
		return;

	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	int colorValue = playerObject->getColorId(index);
	info.GetReturnValue().Set(colorValue);
}

void Player_Colors_Setter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	if (index > 4)
		return;

	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	CString playerProp = playerObject->getProp(PLPROP_COLORS);
	char colors[5];
	for (unsigned int i = 0; i < 5; i++)
		colors[i] = playerProp.readGUChar();

	if (value->IsUint32())
	{
		// Get new value
		unsigned int newValue = value->Uint32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
		if (newValue > 32) // Unsure how many colors exist, capping at 32 for now
			newValue = 32;

		colors[index] = newValue;
	}
	else // if (value->IsString())
	{
		v8::String::Utf8Value newValue(info.GetIsolate(), value);
		colors[index] = getColor(*newValue);
		if (colors[index] < 0)
			return;
	}

	CString propPackage;
	propPackage >> (char)PLPROP_COLORS >> (char)colors[0] >> (char)colors[1] >> (char)colors[2] >> (char)colors[3] >> (char)colors[4];
	playerObject->setProps(propPackage, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
}

// PROPERTY: Player Flags
void Player_GetObject_Flags(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
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

	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	// Grab external data
	v8::Local<v8::External> data = info.Data().As<v8::External>();
	CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());

	// Find constructor
	v8::Local<v8::FunctionTemplate> ctor_tpl = env->GetConstructor("player.flags");
	assert(!ctor_tpl.IsEmpty());

	// Create new instance
	v8::Local<v8::Object> new_instance = ctor_tpl->InstanceTemplate()->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, playerObject);

	// Adds child property to the wrapped object, so it can clear the pointer when the parent is destroyed
	V8ScriptObject<TPlayer> *v8_wrapped = static_cast<V8ScriptObject<TPlayer> *>(playerObject->getScriptObject());
	v8_wrapped->addChild("flags", new_instance);

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
	v8::Local<v8::String> strText = v8::String::NewFromUtf8(isolate, flagValue.text()).ToLocalChecked();
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
	if (newValue.length() == 0) {
		playerObject->deleteFlag(*utf8, true);
	} else {
		playerObject->setFlag(*utf8, *newValue, true);
	}

	// Needed to indicate we handled the request
	info.GetReturnValue().Set(value);
}

void Player_Flags_Enumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Get flags list
	auto flagList = playerObject->getFlagList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)flagList->size());

	int idx = 0;
	for (auto it = flagList->begin(); it != flagList->end(); ++it)
		result->Set(context, idx++, v8::String::NewFromUtf8(isolate, it->first.c_str()).ToLocalChecked()).Check();

	info.GetReturnValue().Set(result);
}

void Player_GetArray_Weapons(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TPlayer, playerObject);

	v8::Isolate *isolate = info.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	// Get npcs list
	auto weaponList = playerObject->getWeaponList();

	v8::Local<v8::Array> result = v8::Array::New(isolate, (int)weaponList->size());

	// TODO(joey): We don't store the weapon objects on the player, maybe we should so we can use the object directly
	//	           in scripts.
	int idx = 0;
	for (auto it = weaponList->begin(); it != weaponList->end(); ++it) {
		//V8ScriptObject<TWeapon> *v8_wrapped = static_cast<V8ScriptObject<TWeapon> *>((*it)->getScriptObject());
		v8::Local<v8::String> weaponName = v8::String::NewFromUtf8(info.GetIsolate(), (*it).text()).ToLocalChecked();
		result->Set(context, idx++, weaponName).Check();
	}

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

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());

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

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());

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

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());

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

// Player Function: player.freezeplayer();
void Player_Function_FreezePlayer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

	playerObject->freezePlayer();
}

// Player Function: player.unfreezeplayer();
void Player_Function_UnfreezePlayer(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

	playerObject->unfreezePlayer();
}

// Player Function: player.say("message"); or player.say(index) for signs in a level
void Player_Function_Say(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// Validate arguments
	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
		playerObject->sendSignMessage(*newValue);
	}
	else if (args[0]->IsInt32())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

		int signIndex = args[0]->Int32Value(isolate->GetCurrentContext()).ToChecked();

		auto level = playerObject->getLevel();
		if (level != nullptr) {
			auto& signs = level->getLevelSigns();
			if (signIndex < signs.size())
				playerObject->sendSignMessage(signs[signIndex].getUText().replaceAll("\n", "#b"));

		}
	}
}

// Player Function: player.sendpm("message");
void Player_Function_SendPM(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	// Validate arguments
	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

		// TODO(joey): Function this like TServer::sendPM(fromPlayer, toPlayer, message);

		// Get server
		v8::Local<v8::External> data = args.Data().As<v8::External>();
		CScriptEngine *scriptEngine = static_cast<CScriptEngine *>(data->Value());
		TServer *server = scriptEngine->getServer();

		// Get npc-server
		TPlayer *npcServer = server->getNPCServer();
		assert(npcServer);

		// Parse argument
		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());

		// PM message
		CString pmMessage(*newValue);
		playerObject->sendPacket(CString() >> (char)PLO_PRIVATEMESSAGE >> (short)npcServer->getId() << "\"\"," << pmMessage.gtokenize());
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

		v8::String::Utf8Value newValue(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
		playerObject->sendRPGMessage(*newValue);
	}
}

// Player Function: player.setani("walk", "ani", "params");
void Player_Function_SetAni(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 1);

	// Validate arguments
	if (args[0]->IsString())
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

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

		playerObject->setAni(animation);
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

		v8::String::Utf8Value levelName(isolate, args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
		double newX = args[1]->NumberValue(context).ToChecked();
		double newY = args[2]->NumberValue(context).ToChecked();

		bool result = playerObject->warp(*levelName, (float)newX, (float)newY);
		args.GetReturnValue().Set(result);
		return;
	}

	args.GetReturnValue().Set(false);
}

// Player function: player.attached(npcId|npcObject)
void Player_Function_Attached(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	unsigned int npcId = 0;

	// Validate arguments
	if (args[0]->IsInt32())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		npcId = args[0]->Int32Value(context).ToChecked();
	}
	else if (args[0]->IsObject())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Object> obj = args[0]->ToObject(context).ToLocalChecked();

		std::string npcConstructor = *v8::String::Utf8Value(isolate, obj->GetConstructorName());
		if (npcConstructor == "npc")
		{
			TNPC* npcObject = UnwrapObject<TNPC>(obj);
			if (npcObject)
				npcId = npcObject->getId();
		}
	}

	//
	if (npcId > 0)
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);
		args.GetReturnValue().Set(playerObject->getAttachedNPC() == npcId);
		return;
	}

	args.GetReturnValue().Set(false);
}

// Player Function: player.attachnpc(npcid);
void Player_Function_AttachNpc(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_ARGCOUNT(args, isolate, 1);

	unsigned int npcId = 0;

	// Validate arguments
	if (args[0]->IsInt32())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		npcId = args[0]->Int32Value(context).ToChecked();
	}
	else if (args[0]->IsObject())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Object> obj = args[0]->ToObject(context).ToLocalChecked();

		std::string npcConstructor = *v8::String::Utf8Value(isolate, obj->GetConstructorName());
		if (npcConstructor == "npc")
		{
			TNPC *npcObject = UnwrapObject<TNPC>(obj);
			if (npcObject)
				npcId = npcObject->getId();
		}
	}

	//
	if (npcId > 0)
	{
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

		CString propPacket;
		propPacket >> (char)PLPROP_ATTACHNPC >> (char)0 >> (int)npcId;
		playerObject->setProps(propPacket, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
		args.GetReturnValue().Set(true);
		return;
	}

	args.GetReturnValue().Set(false);
}

void Player_Function_DetachNpc(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);

	V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

	CString propPacket;
	propPacket >> (char)PLPROP_ATTACHNPC >> (char)0 >> (int)0;
	playerObject->setProps(propPacket, PLSETPROPS_FORWARD | PLSETPROPS_FORWARDSELF);
}

#include "TScriptClass.h"
#include "V8ScriptWrappers.h"

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
		auto classObj = server->getClass(className);

		if (classObj && !classObj->getSource().empty())
		{
			auto &classCode = classObj->getSource();

			// Wrap code
			std::string classCodeWrap = WrapScript<TPlayer>(classCode.getServerSide());

			// TODO(joey): maybe we shouldn't cache this using this method, since classes can be used with
			//             multiple wrappers.
			IScriptFunction *function = scriptEngine->CompileCache(classCodeWrap, false);
			if (function != nullptr) {
				V8ScriptFunction* v8_function = static_cast<V8ScriptFunction*>(function);

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

// Player Function : player.triggerclient(str, wep, args) -> onActionClientSide
void Player_Function_TriggerClient(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate *isolate = args.GetIsolate();

	V8ENV_THROW_CONSTRUCTOR(args, isolate);
	V8ENV_THROW_MINARGCOUNT(args, isolate, 2);

	if (args[0]->IsString() && args[1]->IsString())
	{
		v8::Local<v8::Context> context = isolate->GetCurrentContext();

		CString trigaction = CString("clientside");
		for (int i = 1; i < args.Length(); i++)
			trigaction << "," << *v8::String::Utf8Value(isolate, args[i]->ToString(context).ToLocalChecked());

		// Unwrap Object
		V8ENV_SAFE_UNWRAP(args, TPlayer, playerObject);

		playerObject->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << trigaction);
	}
}

void bindClass_Player(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "player"
	v8::Local<v8::String> className = v8::String::NewFromUtf8Literal(isolate, "player", v8::NewStringType::kInternalized);

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> player_ctor = v8::FunctionTemplate::New(isolate, nullptr, engine_ref); // , Player_Constructor);
	v8::Local<v8::ObjectTemplate> player_proto = player_ctor->PrototypeTemplate();

	player_ctor->SetClassName(className);
	player_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "addweapon"), v8::FunctionTemplate::New(isolate, Player_Function_AddWeapon));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "disableweapons"), v8::FunctionTemplate::New(isolate, Player_Function_DisableWeapons));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "enableweapons"), v8::FunctionTemplate::New(isolate, Player_Function_EnableWeapons));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "freezeplayer"), v8::FunctionTemplate::New(isolate, Player_Function_FreezePlayer, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "unfreezeplayer"), v8::FunctionTemplate::New(isolate, Player_Function_UnfreezePlayer, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "hasweapon"), v8::FunctionTemplate::New(isolate, Player_Function_HasWeapon));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "removeweapon"), v8::FunctionTemplate::New(isolate, Player_Function_RemoveWeapon));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "say"), v8::FunctionTemplate::New(isolate, Player_Function_Say, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "sendpm"), v8::FunctionTemplate::New(isolate, Player_Function_SendPM, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "sendrpgmessage"), v8::FunctionTemplate::New(isolate, Player_Function_SendRPGMessage, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setani"), v8::FunctionTemplate::New(isolate, Player_Function_SetAni, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setgender"), v8::FunctionTemplate::New(isolate, Player_Function_SetGender, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setlevel2"), v8::FunctionTemplate::New(isolate, Player_Function_SetLevel2, engine_ref));
	//player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "setplayerprop"), v8::FunctionTemplate::New(isolate, Player_Function_SetPlayerProp, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "attached"), v8::FunctionTemplate::New(isolate, Player_Function_Attached, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "attachnpc"), v8::FunctionTemplate::New(isolate, Player_Function_AttachNpc, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "detachnpc"), v8::FunctionTemplate::New(isolate, Player_Function_DetachNpc, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "join"), v8::FunctionTemplate::New(isolate, Player_Function_Join, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "triggeraction"), v8::FunctionTemplate::New(isolate, Player_Function_TriggerAction, engine_ref));
	player_proto->Set(v8::String::NewFromUtf8Literal(isolate, "triggerclient"), v8::FunctionTemplate::New(isolate, Player_Function_TriggerClient, engine_ref));

	// Properties
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "id"), Player_GetInt_Id);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "account"), Player_GetStr_Account);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "ani"), Player_GetStr_Ani, Player_SetStr_Ani);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "ap"), Player_GetInt_Alignment, Player_SetInt_Alignment);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "bodyimg"), Player_GetStr_BodyImage, Player_SetStr_BodyImage);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "bombs"), Player_GetInt_Bombs, Player_SetInt_Bombs);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "chat"), Player_GetStr_Chat, Player_SetStr_Chat);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "darts"), Player_GetInt_Darts, Player_SetInt_Darts);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "dir"), Player_GetInt_Dir, Player_SetInt_Dir);
	//player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "hatimg"), Player_GetStr_HatImage, Player_SetStr_HatImage);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "hearts"), Player_GetNum_Hearts, Player_SetNum_Hearts);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "headimg"), Player_GetStr_HeadImage, Player_SetStr_HeadImage);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "fullhearts"), Player_GetInt_Fullhearts, Player_SetInt_Fullhearts);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "glovepower"), Player_GetInt_GlovePower, Player_SetInt_GlovePower);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "guild"), Player_GetStr_Guild, Player_SetStr_Guild);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "isadmin"), Player_GetBool_IsAdmin);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "isclient"), Player_GetBool_IsClient);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "isstaff"), Player_GetBool_IsStaff);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "level"), Player_GetObject_Level);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "levelname"), Player_GetStr_LevelName);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "mp"), Player_GetInt_MagicPower, Player_SetInt_MagicPower);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "nick"), Player_GetStr_Nickname, Player_SetStr_Nickname);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "platform"), Player_GetString_Platform);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "rupees"), Player_GetInt_Rupees, Player_SetInt_Rupees);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "shieldimg"), Player_GetStr_ShieldImage, Player_SetStr_ShieldImage);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "shieldpower"), Player_GetInt_ShieldPower, Player_SetInt_ShieldPower);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "swordimg"), Player_GetStr_SwordImage, Player_SetStr_SwordImage);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "swordpower"), Player_GetInt_SwordPower, Player_SetInt_SwordPower);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "x"), Player_GetNum_X, Player_SetNum_X);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "y"), Player_GetNum_Y, Player_SetNum_Y);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "attr"), Player_GetObject_Attrs, nullptr, engine_ref);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "colors"), Player_GetObject_Colors, nullptr, engine_ref);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "flags"), Player_GetObject_Flags, nullptr, engine_ref);
	player_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "weapons"), Player_GetArray_Weapons);

	// Create the player attr template
	v8::Local<v8::FunctionTemplate> player_attr_ctor = v8::FunctionTemplate::New(isolate);
	player_attr_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "attr"));
	player_attr_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	player_attr_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
		Player_Attr_Getter, Player_Attr_Setter, nullptr, nullptr, nullptr, v8::Local<v8::Value>(),
		v8::PropertyHandlerFlags::kNone));
	env->SetConstructor("player.attr", player_attr_ctor);

	// Create the player colors template
	v8::Local<v8::FunctionTemplate> player_colors_ctor = v8::FunctionTemplate::New(isolate);
	player_colors_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "colors"));
	player_colors_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	player_colors_ctor->InstanceTemplate()->SetHandler(v8::IndexedPropertyHandlerConfiguration(
		Player_Colors_Getter, Player_Colors_Setter, nullptr, nullptr, nullptr, v8::Local<v8::Value>(),
		v8::PropertyHandlerFlags::kNone));
	env->SetConstructor("player.colors", player_colors_ctor);

	// Create the player flags template
	v8::Local<v8::FunctionTemplate> player_flags_ctor = v8::FunctionTemplate::New(isolate);
	player_flags_ctor->SetClassName(v8::String::NewFromUtf8Literal(isolate, "flags"));
	player_flags_ctor->InstanceTemplate()->SetInternalFieldCount(1);
	player_flags_ctor->InstanceTemplate()->SetHandler(v8::NamedPropertyHandlerConfiguration(
			Player_Flags_Getter, Player_Flags_Setter, nullptr, nullptr, Player_Flags_Enumerator, v8::Local<v8::Value>(),
			v8::PropertyHandlerFlags::kHasNoSideEffect));
	env->SetConstructor("player.flags", player_flags_ctor);

	// Persist the player constructor
	env->SetConstructor(ScriptConstructorId<TPlayer>::result, player_ctor);

	// Set the player constructor on the global object
	//v8::Local<v8::ObjectTemplate> global = env->GlobalTemplate();
	//global->Set(className, player_ctor);
}

#endif
