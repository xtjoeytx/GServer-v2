#ifndef V8SCRIPTARGUMENTS_H
#define V8SCRIPTARGUMENTS_H

#include <cassert>
#include <string>
#include <unordered_map>
#include <v8.h>

#include "scripting/interface/ScriptBindings.h"
#include "scripting/v8/V8ScriptEnv.h"
#include "scripting/v8/V8ScriptFunction.h"
#include "scripting/v8/V8ScriptObject.h"

namespace detail
{
	inline v8::Handle<v8::Value> toBinding(V8ScriptEnv* env, std::nullptr_t val)
	{
		return v8::Null(env->isolate());
	}

	inline v8::Handle<v8::Value> toBinding(V8ScriptEnv* env, double val)
	{
		return v8::Number::New(env->isolate(), val);
	}

	inline v8::Handle<v8::Value> toBinding(V8ScriptEnv* env, int val)
	{
		return v8::Integer::New(env->isolate(), val);
	}

	inline v8::Handle<v8::Value> toBinding(V8ScriptEnv* env, const std::string& val)
	{
		return v8::String::NewFromUtf8(env->isolate(), val.c_str()).ToLocalChecked();
	}

	inline v8::Handle<v8::Value> toBinding(V8ScriptEnv* env, const std::shared_ptr<V8ScriptData>& object)
	{
		return object.get()->object();
	}

	inline v8::Handle<v8::Value> toBinding(V8ScriptEnv* env, IScriptFunction* function)
	{
		return static_cast<V8ScriptFunction*>(function)->function();
	}

	template<typename T>
	inline v8::Handle<v8::Value> toBinding(V8ScriptEnv* env, IScriptObject<T>* val)
	{
		V8ScriptObject<T>* wrappedVal = static_cast<V8ScriptObject<T>*>(val);
		wrappedVal->decreaseReference();
		return wrappedVal->handle(env->isolate());
	}
}; // namespace detail

template<typename... Ts>
class V8ScriptArguments : public ScriptArguments<Ts...>
{
	typedef ScriptArguments<Ts...> base;

public:
	template<typename... Args>
	explicit V8ScriptArguments(Args&&... An)
		: ScriptArguments<Ts...>(std::forward<Args>(An)...)
	{
	}

	~V8ScriptArguments() = default;

	virtual bool invoke(IScriptFunction* func, bool catchExceptions = false) override
	{
		assert(base::m_argc > 0);
		SCRIPTENV_D("Invoke Script Argument: %d args\n", base::m_argc);

		if (!base::m_resolved)
		{
			V8ScriptFunction* v8_func = static_cast<V8ScriptFunction*>(func);
			V8ScriptEnv* v8_env = static_cast<V8ScriptEnv*>(v8_func->env());

			v8::Isolate* isolate = v8_env->isolate();
			v8::Local<v8::Context> context = v8_env->context();

			// get a v8 handle for the function to be executed
			v8::Local<v8::Function> cbFunc = v8_func->function();
			assert(!cbFunc.IsEmpty());

			// sort arguments into array
			resolveArgs(v8_env, std::index_sequence_for<Ts...>{});
			base::m_resolved = true;

			// TODO(joey): This will probably not stay like this. Needed the trycatch for executing
			//	new objects for the first time only. Will figure something out.

			// call function
			if (catchExceptions)
			{
				v8::TryCatch try_catch(isolate);
				v8::MaybeLocal<v8::Value> ret = cbFunc->Call(context, m_args[0], base::m_argc, m_args);
				static_cast<void>(ret);

				if (try_catch.HasCaught())
				{
					v8_env->parseErrors(&try_catch);
					return false;
				}
			}
			else
			{
				v8::MaybeLocal<v8::Value> ret = cbFunc->Call(context, m_args[0], base::m_argc, m_args); // base::Argc - 1, m_args + 1);
				static_cast<void>(ret);
				//ret.IsEmpty();
			}
		}

		SCRIPTENV_D("Finish Script Argument\n");
		return true;
	}

private:
	v8::Local<v8::Value> m_args[base::m_argc];

	template<std::size_t... Is>
	inline void resolveArgs(V8ScriptEnv* env, std::index_sequence<Is...>)
	{
		if constexpr (sizeof...(Is) > 0)
		{
			int unused[] = { ((m_args[Is] = detail::toBinding(env, std::get<Is>(base::m_tuple))), void(), 0)... };
			static_cast<void>(unused); // Avoid warning for unused variable
		}
	}
};

#endif
