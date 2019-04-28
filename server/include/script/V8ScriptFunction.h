#pragma once

#include <cassert>
#include "ScriptArguments.h"
#include "ScriptFunction.h"
#include "V8Macros.h"
#include "V8ScriptEnv.h"

class V8ScriptFunction : public IScriptFunction
{
public:
	V8ScriptFunction(V8ScriptEnv *env, v8::Local<v8::Function> function)
		: IScriptFunction(), _env(env) {
		assert(env != 0);
		_function.Reset(env->Isolate(), function);
	}
	
	virtual ~V8ScriptFunction() {
		_function.Reset();
	}
	
	inline v8::Local<v8::Function> Function() const {
		return PersistentToLocal(_env->Isolate(), _function);
	}

	inline V8ScriptEnv * Env() const {
		return _env;
	}
	
private:
	v8::Persistent<v8::Function> _function;
	V8ScriptEnv *_env;
};
