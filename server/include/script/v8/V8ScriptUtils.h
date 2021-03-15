#pragma once

#ifndef V8SCRIPTUTILS_H
#define V8SCRIPTUTILS_H

#include <v8.h>

// Throw an exception if the function was called with new Function();
#define V8ENV_THROW_CONSTRUCTOR(args, isolate)						\
	if (args.IsConstructCall()) {									\
		isolate->ThrowException(v8::String::NewFromUtf8Literal(isolate,	\
			"Cannot call function as a constructor.")); 			\
		return;														\
	}

// Throw an exception if a constructor was called with Function();
#define V8ENV_THROW_METHOD(args, isolate)							\
	if (!args.IsConstructCall()) {									\
		isolate->ThrowException(v8::String::NewFromUtf8Literal(isolate,	\
			"Cannot call constructor as a function.")); 			\
		return;														\
	}

// Throw an exception if we didn't receive the correct amount of arguments
#define V8ENV_THROW_ARGCOUNT(args, isolate, required_args)			\
	if (args.Length() != required_args) {							\
		isolate->ThrowException(v8::String::NewFromUtf8(isolate,	\
			std::string("Cannot call function with ")				\
				.append(std::to_string(args.Length()))				\
				.append(" arguments, required ## required_args ")	\
				.c_str()).ToLocalChecked()); 						\
		return;														\
	}

// Throw an exception if we receive less than the minimum amount of arguments
#define V8ENV_THROW_MINARGCOUNT(args, isolate, required_args)		\
	if (args.Length() < required_args) {							\
		isolate->ThrowException(v8::String::NewFromUtf8(isolate,	\
			std::string("Cannot call function with ")				\
			.append(std::to_string(args.Length()))					\
			.append(" arguments, required ## required_args ")		\
			.c_str()).ToLocalChecked()); 											\
		return;														\
	}

// Unwrap an object, and validate the pointer
#define V8ENV_SAFE_UNWRAP(ARGS, TYPE, VAR_NAME)						\
	TYPE * VAR_NAME = UnwrapObject<TYPE>(ARGS.This());				\
	if (!VAR_NAME) {												\
		return;														\
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

template <class TypeName>
inline v8::Local<TypeName> GlobalPersistentToLocal(v8::Isolate *isolate, const v8::Global<TypeName>& persistent)
{
	if (persistent.IsWeak()) {
		return v8::Local<TypeName>::New(isolate, persistent);
	} else {
		return *reinterpret_cast<v8::Local<TypeName> *>(const_cast<v8::Global<TypeName> *>(&persistent));
	}
}

template <class Type>
inline Type * UnwrapObject(v8::Local<v8::Object> self) {
	return static_cast<Type *>(self->GetAlignedPointerFromInternalField(0));
}

#endif
