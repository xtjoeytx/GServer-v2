#ifndef V8SCRIPTENV_H
#define V8SCRIPTENV_H

#include "ScriptBindings.h"
#include "V8ScriptObject.h"
#include "V8ScriptUtils.h"
#include <memory>
#include <v8.h>
#include <vector>

class IScriptFunction;

class V8ScriptEnv : public IScriptEnv
{
public:
	V8ScriptEnv();
	virtual ~V8ScriptEnv();

	int GetType() const override { return 1; }

	void Initialize() override;
	void Cleanup(bool shutDown = false) override;

	IScriptFunction* Compile(const std::string& name, const std::string& source) override;
	void CallFunctionInScope(std::function<void()> function) override;
	void TerminateExecution() override;

	// Parse errors from a TryCatch into lastScriptError
	bool ParseErrors(v8::TryCatch* tryCatch);

	// --
	v8::Isolate* Isolate() const;
	v8::Local<v8::Context> Context() const;
	v8::Local<v8::Object> Global() const;
	v8::Local<v8::ObjectTemplate> GlobalTemplate() const;
	v8::Local<v8::FunctionTemplate> GetConstructor(const std::string& key) const;

	void SetGlobal(v8::Local<v8::Object> global);
	void SetGlobalTemplate(v8::Local<v8::ObjectTemplate> global_tpl);
	bool SetConstructor(const std::string& key, v8::Local<v8::FunctionTemplate> func_tpl);

	// --
	template<class T>
	std::unique_ptr<IScriptObject<T>> Wrap(const std::string& constructor_name, T* obj);

	template<class T>
	T* Unwrap(v8::Local<v8::Value> value) const;

private:
	static int s_count;
	static std::unique_ptr<v8::Platform> s_platform;

	bool _initialized;
	v8::Isolate::CreateParams create_params;
	v8::Isolate* _isolate;
	v8::Persistent<v8::Context> _context;
	v8::Persistent<v8::Object> _global;
	v8::Persistent<v8::ObjectTemplate> _global_tpl;
	std::unordered_map<std::string, v8::Global<v8::FunctionTemplate>> _constructorMap;
};

inline v8::Isolate* V8ScriptEnv::Isolate() const
{
	return _isolate;
}

inline v8::Local<v8::Context> V8ScriptEnv::Context() const
{
	return PersistentToLocal(Isolate(), _context);
}

inline v8::Local<v8::Object> V8ScriptEnv::Global() const
{
	return PersistentToLocal(Isolate(), _global);
}

inline void V8ScriptEnv::SetGlobal(v8::Local<v8::Object> global)
{
	_global.Reset(Isolate(), global);
}

inline v8::Local<v8::ObjectTemplate> V8ScriptEnv::GlobalTemplate() const
{
	return PersistentToLocal(Isolate(), _global_tpl);
}

inline void V8ScriptEnv::SetGlobalTemplate(v8::Local<v8::ObjectTemplate> global_tpl)
{
	_global_tpl.Reset(Isolate(), global_tpl);
}

inline v8::Local<v8::FunctionTemplate> V8ScriptEnv::GetConstructor(const std::string& key) const
{
	auto it = _constructorMap.find(key);
	if (it == _constructorMap.end())
		return v8::Local<v8::FunctionTemplate>();

	return GlobalPersistentToLocal(Isolate(), (*it).second);
}

template<class T>
inline std::unique_ptr<IScriptObject<T>> V8ScriptEnv::Wrap(const std::string& constructor_name, T* obj)
{
	// Fetch the v8 isolate and context
	v8::Isolate* isolate = Isolate();
	v8::Local<v8::Context> context = Context();
	assert(!context.IsEmpty());

	// Create a stack-allocated scope for v8 calls, and enter context
	v8::Locker locker(isolate);
	v8::Isolate::Scope isolate_scope(isolate);
	v8::HandleScope handle_scope(isolate);
	v8::Context::Scope context_scope(context);

	// Create an instance for the wrapped object
	v8::Local<v8::FunctionTemplate> ctor_tpl = GetConstructor(constructor_name);
	v8::Local<v8::ObjectTemplate> obj_tpl = ctor_tpl->InstanceTemplate();
	v8::Local<v8::Object> new_instance = obj_tpl->NewInstance(context).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, obj);

	return std::make_unique<V8ScriptObject<T>>(obj, isolate, new_instance);
}

template<class T>
inline T* V8ScriptEnv::Unwrap(v8::Local<v8::Value> value) const
{
	T* obj = 0;
	if (value->IsObject())
	{
		v8::MaybeLocal<v8::Object> handle = value->ToObject(Context());
		if (!handle.IsEmpty())
			obj = static_cast<T*>(handle.ToLocalChecked()->GetAlignedPointerFromInternalField(0));
	}

	return obj;
}

#endif
