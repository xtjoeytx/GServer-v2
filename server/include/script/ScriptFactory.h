#pragma once

#ifndef SCRIPTFACTORY_H
#define SCRIPTFACTORY_H

enum
{
	ScriptConstructorNone,
	ScriptConstructorServer,
	ScriptConstructorNpc,
	ScriptConstructorPlayer,
	ScriptConstructorCount
};

template<typename T>
struct ScriptConstructorId {
	enum {
		result = ScriptConstructorNone
	};
};

class TServer;
template<> struct ScriptConstructorId<TServer> {
	enum {
		result = ScriptConstructorServer
	};
};

class TNPC;
template<> struct ScriptConstructorId<TNPC> {
	enum {
		result = ScriptConstructorNpc
	};
};

class TPlayer;
template<> struct ScriptConstructorId<TPlayer> {
	enum {
		result = ScriptConstructorPlayer
	};
};

#include "V8ScriptArguments.h"

struct ScriptArgumentsFactory
{
	template <typename T, typename... Args>
	static inline ScriptArguments<Args...> * Create(T *env, Args&&... An) {
		return nullptr;
	}

	template <typename... Args>
	static inline ScriptArguments<Args...> * Create(IScriptEnv *env, Args&&... An) {
		switch (env->GetType()) {
		case 1: // v8
			return Create(static_cast<V8ScriptEnv *>(env), std::forward<Args>(An)...);

		default: // couldn't deduce type
			return nullptr;
		}
	}

	template <typename... Args>
	static inline ScriptArguments<Args...> * Create(V8ScriptEnv *env, Args&&... An) {
		return new V8ScriptArguments<Args...>(std::forward<Args>(An)...);
	}
};

#endif