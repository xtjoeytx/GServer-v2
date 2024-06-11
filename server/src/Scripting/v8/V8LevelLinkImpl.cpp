#ifdef V8NPCSERVER

	#include "ScriptEngine.h"
	#include "V8ScriptFunction.h"
	#include "V8ScriptObject.h"
	#include <algorithm>
	#include <cassert>
	#include <math.h>
	#include <unordered_map>
	#include <v8.h>

	#include "Level.h"
	#include "LevelLink.h"
	#include "Map.h"
	#include "NPC.h"
	#include "Player.h"

// PROPERTY: link.newlevel
void Link_GetStr_NewLevel(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(),
															linkObject->getNewLevel().text())
										.ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Link_SetStr_NewLevel(v8::Local<v8::String> props, v8::Local<v8::Value> value,
						  const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	linkObject->setNewLevel(*newValue);
}

// PROPERTY: link.x
void Link_GetNum_X(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	info.GetReturnValue().Set(linkObject->getX());
}

void Link_SetNum_X(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	linkObject->setX(newValue);
}

// PROPERTY: link.y
void Link_GetNum_Y(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	info.GetReturnValue().Set(linkObject->getY());
}

void Link_SetNum_Y(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	linkObject->setY(newValue);
}

// PROPERTY: link.width
void Link_GetNum_Width(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	info.GetReturnValue().Set(linkObject->getWidth());
}

void Link_SetNum_Width(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	linkObject->setWidth(newValue);
}

// PROPERTY: link.height
void Link_GetNum_Height(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	info.GetReturnValue().Set(linkObject->getHeight());
}

void Link_SetNum_Height(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	int newValue = (int)value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked();

	linkObject->setHeight(newValue);
}

// PROPERTY: link.newx
void Link_GetStr_NewX(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(),
															linkObject->getNewX().text())
										.ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Link_SetStr_NewX(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	linkObject->setNewX(*newValue);
}

// PROPERTY: link.newx
void Link_GetStr_NewY(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	v8::Local<v8::String> strText = v8::String::NewFromUtf8(info.GetIsolate(),
															linkObject->getNewY().text())
										.ToLocalChecked();
	info.GetReturnValue().Set(strText);
}

void Link_SetStr_NewY(v8::Local<v8::String> props, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
	V8ENV_SAFE_UNWRAP(info, LevelLink, linkObject);

	v8::String::Utf8Value newValue(info.GetIsolate(), value);

	linkObject->setNewY(*newValue);
}

void bindClass_LevelLink(ScriptEngine* scriptEngine)
{
	// Retrieve v8 environment
	V8ScriptEnv* env = static_cast<V8ScriptEnv*>(scriptEngine->getScriptEnv());
	v8::Isolate* isolate = env->isolate();

	// External pointer
	v8::Local<v8::External> engine_ref = v8::External::New(isolate, scriptEngine);

	// Create V8 string for "level"
	v8::Local<v8::String> linkStr = v8::String::NewFromUtf8Literal(isolate, "link", v8::NewStringType::kInternalized);

	// Create constructor for class
	v8::Local<v8::FunctionTemplate> link_ctor = v8::FunctionTemplate::New(isolate);
	v8::Local<v8::ObjectTemplate> link_proto = link_ctor->PrototypeTemplate();
	link_ctor->SetClassName(linkStr);
	link_ctor->InstanceTemplate()->SetInternalFieldCount(1);

	// Method functions
	//link_proto->Set(v8::String::NewFromUtf8Literal(isolate, "clone"), v8::FunctionTemplate::New(isolate, Level_Function_Clone, engine_ref));

	// Properties
	link_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "newlevel"), Link_GetStr_NewLevel,
							Link_SetStr_NewLevel);
	link_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "x"), Link_GetNum_X, Link_SetNum_X);
	link_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "y"), Link_GetNum_Y, Link_SetNum_Y);
	link_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "width"), Link_GetNum_Width, Link_SetNum_Width);
	link_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "height"), Link_GetNum_Height, Link_SetNum_Height);
	link_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "newx"), Link_GetStr_NewX, Link_SetStr_NewX);
	link_proto->SetAccessor(v8::String::NewFromUtf8Literal(isolate, "newy"), Link_GetStr_NewY, Link_SetStr_NewY);

	// Persist the constructor
	env->setConstructor(ScriptConstructorId<LevelLink>::result, link_ctor);
}

#endif
