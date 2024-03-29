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
#include "TLevelLink.h"
#include "TMap.h"
#include "TNPC.h"
#include "TPlayer.h"

// PROPERTY: sign.text
void Sign_GetStr_Text(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevelSign, signObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(), signObject->getUText().text()).ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Sign_SetStr_Text(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevelSign, signObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	signObject->setUText(*newValue);
}

// PROPERTY: sign.x
void Sign_GetNum_X(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevelSign, signObject);

	info.GetReturnValue().Set(signObject->getX());
}

void Sign_SetNum_X(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevelSign, signObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	signObject->setX(newValue);
}

// PROPERTY: sign.y
void Sign_GetNum_Y(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevelSign, signObject);

	info.GetReturnValue().Set(signObject->getY());
}

void Sign_SetNum_Y(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, TLevelSign, signObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	signObject->setY(newValue);
}

void bindClass_LevelSign(CScriptEngine *scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
	v8::Isolate *isolate = env->Isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "level"
	v8::Local<v8::String> signStr = v8::String::NewFromUtf8Literal(isolate, "sign", v8::NewStringType::kInternalized);

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> sign_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> sign_proto = sign_ctor->PrototypeTemplate();
	sign_ctor->SetClassName(signStr);
	sign_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	//link_proto->Set(v8::String::NewFromUtf8Literal(isolate, "clone"), v8::FunctionTemplate::New(isolate, Level_Function_Clone, engine_ref));

	// Properties
	sign_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "text"), Sign_GetStr_Text, Sign_SetStr_Text);
	sign_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "x"), Sign_GetNum_X, Sign_SetNum_X);
	sign_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "y"), Sign_GetNum_Y, Sign_SetNum_Y);

	// Persist the constructor
	env->SetConstructor(ScriptConstructorId<TLevelSign>::result, sign_ctor);
}

#endif
