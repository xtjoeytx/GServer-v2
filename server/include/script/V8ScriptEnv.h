#pragma once

#include <vector>
#include <v8.h>
#include "ScriptEnv.h"
#include "ScriptUtils.h"
#include "V8Macros.h"
#include "V8ScriptWrapped.h"

#define CONSTRUCTOR_COUNT 10

class IScriptFunction;

class V8ScriptEnv : public IScriptEnv
{
public:
	V8ScriptEnv();
	virtual ~V8ScriptEnv();
	
	int GetType() override {
		return 1;
	}
	
	void Initialize() override;
	void Cleanup() override;
	IScriptFunction * Compile(const std::string& name, const std::string& source) override;
	
	bool ParseErrors(v8::TryCatch *tryCatch);

	// --
	template<class T>
	inline IScriptWrapped<T> * Wrap(size_t idx, T *obj);
	
	template<class T>
	inline T * Unwrap(v8::Local<v8::Value> value);
	
	// --
	inline v8::Isolate * Isolate() const {
		return _isolate;
	}
	
	inline v8::Local<v8::Context> Context() const {
		return PersistentToLocal(Isolate(), _context);
	}
	
	inline v8::Local<v8::Object> Global() const {
		return PersistentToLocal(Isolate(), _global);
	}
	
	inline void SetGlobal(v8::Local<v8::Object> global) {
		_global.Reset(Isolate(), global);
	}
	
	inline v8::Local<v8::ObjectTemplate> GlobalTemplate() const {
		return PersistentToLocal(Isolate(), _global_tpl);
	}

	inline void SetGlobalTemplate(v8::Local<v8::ObjectTemplate> global_tpl) {
		_global_tpl.Reset(Isolate(), global_tpl);
	}
	
	inline v8::Local<v8::FunctionTemplate> GetConstructor(size_t idx) const {
		return PersistentToLocal(Isolate(), _ctors[idx]);
	}
	
	inline void SetConstructor(size_t idx, v8::Local<v8::FunctionTemplate> func_tpl) {
		_ctors[idx].Reset(Isolate(), func_tpl);
	}

private:
	static int s_count;
	static std::unique_ptr<v8::Platform> s_platform;
	
	bool _initialized;
	v8::Isolate::CreateParams create_params;
	v8::Isolate * _isolate;
	v8::Persistent<v8::Context> _context;
	v8::Persistent<v8::Object> _global;
	v8::Persistent<v8::ObjectTemplate> _global_tpl;
	v8::Persistent<v8::FunctionTemplate> _ctors[CONSTRUCTOR_COUNT];
};

template<class T>
IScriptWrapped<T> * V8ScriptEnv::Wrap(size_t idx, T *obj)
{
	// Fetch the v8 isolate and context
	v8::Isolate *isolate = Isolate();
	v8::Local<v8::Context> context = Context();
	assert(!context.IsEmpty());

	// Create a stack-allocated scope for v8 calls, and enter context
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);
	v8::Context::Scope context_scope(context);

	// Create an instance for the wrapped object
	v8::Local<v8::FunctionTemplate> ctor_tpl = GetConstructor(idx);
	v8::Local<v8::ObjectTemplate> obj_tpl = ctor_tpl->InstanceTemplate();
	v8::Local<v8::Object> new_instance = obj_tpl->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, obj);
	
	// TODO(joey): Use unique_ptr for this
	V8ScriptWrapped<T> *wrapped_object = new V8ScriptWrapped<T>(obj, isolate, new_instance);
	return wrapped_object;
}

template<class T>
T * V8ScriptEnv::Unwrap(v8::Local<v8::Value> value)
{
	T *obj = 0;
	if (value->IsObject())
	{
		v8::MaybeLocal<v8::Object> handle = value->ToObject(Context());
		if (!handle.IsEmpty())
			obj = static_cast<T *>(handle.ToLocalChecked()->GetAlignedPointerFromInternalField(0));
	}
	
	return obj;
}
