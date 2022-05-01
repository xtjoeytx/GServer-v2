#pragma once

#ifndef SCRIPTFACTORY_H
#define SCRIPTFACTORY_H

#define SCRIPTSYS_HASV8

template<typename T>
struct ScriptConstructorId {
	static constexpr auto result = "";
};

#define SCRIPTFACTORY_CONSTRUCTOR(CLASS_NAME, KEY) \
class CLASS_NAME; \
template<> struct ScriptConstructorId<CLASS_NAME> { \
	static constexpr auto result = #KEY; \
};

SCRIPTFACTORY_CONSTRUCTOR(TLevel, level)
SCRIPTFACTORY_CONSTRUCTOR(TNPC, npc)
SCRIPTFACTORY_CONSTRUCTOR(TPlayer, player)
SCRIPTFACTORY_CONSTRUCTOR(TWeapon, weapon)

#undef SCRIPTFACTORY_CONSTRUCTOR

#ifdef SCRIPTSYS_HASV8
#include "V8ScriptArguments.h"
#endif

#include <memory>

struct ScriptFactory
{
	/*
		Create Script Arguments
	*/

	template <typename T, typename... Args>
	static inline ScriptArguments<Args...> * CreateArguments(T *env, Args&&... An) {
		return nullptr;
	}

	template <typename... Args>
	static inline ScriptArguments<Args...> * CreateArguments(IScriptEnv *env, Args&&... An) {
		switch (env->GetType()) {
#ifdef SCRIPTSYS_HASV8
			case 1: // v8
				return CreateArguments(static_cast<V8ScriptEnv *>(env), std::forward<Args>(An)...);
#endif

			default: // couldn't deduce type
				return nullptr;
		}
	}

#ifdef SCRIPTSYS_HASV8
	template <typename... Args>
	static inline ScriptArguments<Args...> * CreateArguments(V8ScriptEnv *env, Args&&... An) {
		return new V8ScriptArguments<Args...>(std::forward<Args>(An)...);
	}
#endif

	/*
		Wrap Object
	*/

	template <typename T, typename Cls>
	static inline std::unique_ptr<IScriptObject<Cls>> WrapObject(T *env, const std::string& ctor_name, Cls *obj) {
		return {};
	}

	template <typename Cls>
	static inline std::unique_ptr<IScriptObject<Cls>> WrapObject(IScriptEnv *env, const std::string& ctor_name, Cls *obj) {
		switch (env->GetType()) {
#ifdef SCRIPTSYS_HASV8
			case 1: // v8
				return WrapObject(static_cast<V8ScriptEnv *>(env), ctor_name, obj);
#endif

			default: // couldn't deduce type
				return {};
		}
	}

#ifdef SCRIPTSYS_HASV8
	template <typename Cls>
	static inline std::unique_ptr<IScriptObject<Cls>> WrapObject(V8ScriptEnv *env, const std::string& ctor_name, Cls *obj) {
		return env->Wrap(ctor_name, obj);
	}
#endif
};

#endif
