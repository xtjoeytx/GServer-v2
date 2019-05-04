#pragma once

#include <v8.h>

#ifndef NDEBUG
	#define V8ENV_D(...) do {} while(0)
#else
	#define V8ENV_D(...) printf(__VA_ARGS__)
#endif

// TODO(joey): might be able to get rid of this check does it really matter if a user uses new print("asd");
// Throw an exception on constructor calls for method functions
#define V8ENV_THROW_CONSTRUCTOR(args, isolate)						\
	if (args.IsConstructCall()) {									\
		isolate->ThrowException(v8::String::NewFromUtf8(isolate,	\
			"Cannot call function as a constructor.")); 			\
		return;														\
	}

// Throw an exception on method functions for constructor calls
#define V8ENV_THROW_METHOD(args, isolate)							\
	if (!args.IsConstructCall()) {									\
		isolate->ThrowException(v8::String::NewFromUtf8(isolate,	\
			"Cannot call constructor as a function.")); 			\
		return;														\
	}

// Throw an exception if we didn't receive the correct amount of arguments
#define V8ENV_THROW_ARGCOUNT(args, isolate, required_args)				\
	if (args.Length() != required_args) {							\
		isolate->ThrowException(v8::String::NewFromUtf8(isolate,	\
			std::string("Cannot call function with ").append(std::to_string(args.Length())).append(" arguments, required ## required_args ").c_str())); 			\
		return;														\
	}

// Accessor macros
#define V8ENV_ACCESSOR_INT32(OBJ, VAR)								\
	void OBJ ## _GetInt32_ ## VAR(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) { \
		v8::Local<v8::Object> self = info.This();					\
		void *ptr = self->GetAlignedPointerFromInternalField(0);	\
		OBJ *wrap = static_cast<OBJ *>(ptr);						\
		info.GetReturnValue().Set(wrap->VAR);						\
	}																\
	void OBJ ## _SetInt32_ ## VAR(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) { \
		v8::Local<v8::Object> self = info.This();					\
		void *ptr = self->GetAlignedPointerFromInternalField(0);	\
		OBJ *wrap = static_cast<OBJ *>(ptr);						\
		wrap->VAR = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked(); \
	}

#define V8ENV_ACCESSOR_NUM(OBJ, VAR)								\
	void OBJ ## _GetNum_ ## VAR(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) { \
		v8::Local<v8::Object> self = info.This();					\
		void *ptr = self->GetAlignedPointerFromInternalField(0);	\
		OBJ *wrap = static_cast<OBJ *>(ptr);						\
		info.GetReturnValue().Set(wrap->VAR);						\
	}																\
	void OBJ ## _SetNum_ ## VAR(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) { \
		v8::Local<v8::Object> self = info.This();					\
		void *ptr = self->GetAlignedPointerFromInternalField(0);	\
		OBJ *wrap = static_cast<OBJ *>(ptr);						\
		wrap->VAR = value->NumberValue(info.GetIsolate()->GetCurrentContext()).ToChecked(); \
	}

#define V8ENV_ACCESSOR_STRING(OBJ, VAR)								\
	void OBJ ## _GetStr_ ## VAR(v8::Local<v8::String> prop, const v8::PropertyCallbackInfo<v8::Value>& info) { \
		v8::Local<v8::Object> self = info.This();					\
		void *ptr = self->GetAlignedPointerFromInternalField(0);	\
		OBJ *wrap = static_cast<OBJ *>(ptr);						\
		v8::Local<v8::String> strText = v8::String::NewFromUtf8(	\
			v8::Isolate::GetCurrent(), wrap->VAR.c_str());			\
		info.GetReturnValue().Set(strText);							\
	}																\
	void OBJ ## _SetStr_ ## VAR(v8::Local<v8::String> prop, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) { \
		v8::Local<v8::Object> self = info.This();					\
		void *ptr = self->GetAlignedPointerFromInternalField(0);	\
		OBJ *wrap = static_cast<OBJ *>(ptr);						\
		wrap->VAR = *v8::String::Utf8Value(value);					\
	}

template <class TypeName>
inline v8::Local<TypeName> PersistentToLocal(v8::Isolate *isolate, const v8::Persistent<TypeName>& persistent)
{
	if (persistent.IsWeak()) {
		return v8::Local<TypeName>::New(isolate, persistent);
	} else {
		return *reinterpret_cast<v8::Local<TypeName> *>(const_cast<v8::Persistent<TypeName> *>(&persistent));
	}
}

template <class Type>
inline Type * UnwrapObject(v8::Local<v8::Object> self) {
	return static_cast<Type *>(self->GetAlignedPointerFromInternalField(0));
}
