#pragma once

#include <cassert>
#include <string>
#include <unordered_map>
#include <v8.h>
#include "ScriptArguments.h"
#include "V8ScriptEnv.h"
#include "V8ScriptFunction.h"
#include "V8ScriptWrapped.h"

namespace detail
{
	inline v8::Handle<v8::Value> ToBinding(V8ScriptEnv *env, double val) {
		return v8::Number::New(env->Isolate(), val);
	}

	inline v8::Handle<v8::Value> ToBinding(V8ScriptEnv *env, int val) {
		return v8::Integer::New(env->Isolate(), val);
	}

	inline v8::Handle<v8::Value> ToBinding(V8ScriptEnv *env, const std::string& val) {
		return v8::String::NewFromUtf8(env->Isolate(), val.c_str());
	}

	inline v8::Handle<v8::Value> ToBinding(V8ScriptEnv *env, IScriptFunction *function) {
		return static_cast<V8ScriptFunction *>(function)->Function();
	}

	template<typename T>
	inline v8::Handle<v8::Value> ToBinding(V8ScriptEnv *env, IScriptWrapped<T> *val) {
		V8ScriptWrapped<T> *wrappedVal = static_cast<V8ScriptWrapped<T> *>(val);
		wrappedVal->decreaseReference();
		return wrappedVal->Handle(env->Isolate());
	}
};

template <typename... Ts>
class V8ScriptArguments : public ScriptArguments<Ts...>
{
	typedef ScriptArguments<Ts...> base;

public:
	template <typename... Args>
	V8ScriptArguments(Args&&... An)
		: ScriptArguments<Ts...>(An...) {
	}

	~V8ScriptArguments() = default;

	virtual void Invoke(IScriptFunction *func) override
	{
		V8ENV_D("Invoke Script Argument: %d args\n", this->Argc);

		if (!base::_resolved)
		{
			V8ScriptFunction *v8_func = static_cast<V8ScriptFunction *>(func);
			V8ScriptEnv *v8_env = static_cast<V8ScriptEnv *>(v8_func->Env());
			
			v8::Isolate *isolate = v8_env->Isolate();
			v8::Local<v8::Context> context = v8_env->Context();

			// get a v8 handle for the function to be executed
			v8::Local<v8::Function> cbFunc = v8_func->Function();
			assert(!cbFunc.IsEmpty());

			// sort arguments into array
			resolve_args(v8_env, std::index_sequence_for<Ts...>{});
			base::_resolved = true;

			// call function
			v8::MaybeLocal<v8::Value> ret = cbFunc->Call(context, v8::Null(isolate), base::Argc, _args);
			if (!ret.IsEmpty())
				V8ENV_D(" - Returned Value from Callback: %s\n", *(v8::String::Utf8Value(isolate, ret.ToLocalChecked())));
		}

		V8ENV_D("Finish Script Argument\n");
	}

private:
	v8::Local<v8::Value> _args[base::Argc];

	template <std::size_t...Is>
	inline void resolve_args(V8ScriptEnv *env, std::index_sequence<Is...>) {
		if constexpr (sizeof...(Is) > 0) {
			int unused[] = { ((_args[Is] = detail::ToBinding(env, std::get<Is>(base::_tuple))), void(), 0)... };
			static_cast<void>(unused); // Avoid warning for unused variable
		}
	}
};
