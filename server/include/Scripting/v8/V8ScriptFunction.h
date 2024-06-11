#ifndef V8SCRIPTFUNCTION_H
#define V8SCRIPTFUNCTION_H

#include "ScriptBindings.h"
#include "V8ScriptEnv.h"
#include "V8ScriptUtils.h"
#include <cassert>

class V8ScriptData
{
public:
	V8ScriptData(V8ScriptEnv* env, v8::Local<v8::Object> object)
		: m_env(env)
	{
		assert(env != nullptr);
		assert(object->IsObject());
		m_object.Reset(env->isolate(), object);
	}

	~V8ScriptData()
	{
		m_object.Reset();
	}

	v8::Local<v8::Object> object() const
	{
		return persistentToLocal(m_env->isolate(), m_object);
	}

private:
	V8ScriptEnv* m_env;
	v8::Persistent<v8::Object> m_object;
};

class V8ScriptFunction : public IScriptFunction
{
public:
	V8ScriptFunction(V8ScriptEnv* env, v8::Local<v8::Function> function)
		: IScriptFunction(), m_env(env)
	{
		assert(env != nullptr);
		assert(function->IsFunction());
		m_function.Reset(env->isolate(), function);
	}

	virtual ~V8ScriptFunction()
	{
		m_function.Reset();
	}

	inline v8::Local<v8::Function> function() const
	{
		return persistentToLocal(m_env->isolate(), m_function);
	}

	inline V8ScriptEnv* env() const
	{
		return m_env;
	}

private:
	v8::Persistent<v8::Function> m_function;
	V8ScriptEnv* m_env;
};

#endif
