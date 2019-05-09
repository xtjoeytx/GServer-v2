#pragma once

#ifndef SCRIPTFACTORY_H
#define SCRIPTFACTORY_H

enum
{
	ScriptConstructorNone           = 0,
	ScriptConstructorEnvironment    = 1,
	ScriptConstructorServer         = 2,
	ScriptConstructorLevel         	= 3,
	ScriptConstructorNpc            = 4,
	ScriptConstructorPlayer         = 5,
	ScriptConstructorCount,
};

template<typename T>
struct ScriptConstructorId {
	enum {
		result = ScriptConstructorNone
	};
};

#define SCRIPTFACTORY_CONSTRUCTOR(CLASS_NAME, KEY) \
class CLASS_NAME; \
template<> struct ScriptConstructorId<CLASS_NAME> { \
	enum { \
		result = ScriptConstructor ## KEY \
	}; \
};

SCRIPTFACTORY_CONSTRUCTOR(IScriptEnv, Environment)
SCRIPTFACTORY_CONSTRUCTOR(TServer, Server)
SCRIPTFACTORY_CONSTRUCTOR(TLevel, Level)
SCRIPTFACTORY_CONSTRUCTOR(TNPC, Npc)
SCRIPTFACTORY_CONSTRUCTOR(TPlayer, Player)

#undef SCRIPTFACTORY_CONSTRUCTOR

#include "V8ScriptArguments.h"

struct ScriptArgumentsFactory
{
	template <typename T, typename... Args>
	static inline ScriptArguments<Args...> * Create(T *env, Args&&... An)
	{
		return nullptr;
	}

	template <typename... Args>
	static inline ScriptArguments<Args...> * Create(IScriptEnv *env, Args&&... An)
	{
		switch (env->GetType())
		{
			case 1: // v8
				return Create(static_cast<V8ScriptEnv *>(env), std::forward<Args>(An)...);

			default: // couldn't deduce type
				return nullptr;
		}
	}

	template <typename... Args>
	static inline ScriptArguments<Args...> * Create(V8ScriptEnv *env, Args&&... An)
	{
		return new V8ScriptArguments<Args...>(std::forward<Args>(An)...);
	}
};

#endif
