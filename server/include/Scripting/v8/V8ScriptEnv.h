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

	int getType() const override { return 1; }

	void initialize() override;

	void cleanup(bool shutDown = false) override;

	IScriptFunction* compile(const std::string& name, const std::string& source) override;

	void callFunctionInScope(std::function<void()> function) override;

	void terminateExecution() override;

	// Parse errors from a TryCatch into lastScriptError
	bool parseErrors(v8::TryCatch* tryCatch);

	// --
	v8::Isolate* isolate() const;

	v8::Local<v8::Context> context() const;

	v8::Local<v8::Object> global() const;

	v8::Local<v8::ObjectTemplate> globalTemplate() const;

	v8::Local<v8::FunctionTemplate> getConstructor(const std::string& key) const;

	void setGlobal(v8::Local<v8::Object> global);

	void setGlobalTemplate(v8::Local<v8::ObjectTemplate> global_tpl);

	bool setConstructor(const std::string& key, v8::Local<v8::FunctionTemplate> func_tpl);

	// --
	template<class T>
	std::unique_ptr<IScriptObject<T>> wrap(const std::string& constructor_name, T* obj);

	template<class T>
	T* unwrap(v8::Local<v8::Value> value) const;

private:
	static int m_count;
	static std::unique_ptr<v8::Platform> m_platform;

	bool m_initialized;
	v8::Isolate::CreateParams m_createParams;
	v8::Isolate* m_isolate;
	v8::Persistent<v8::Context> m_context;
	v8::Persistent<v8::Object> m_global;
	v8::Persistent<v8::ObjectTemplate> m_globalTpl;
	std::unordered_map<std::string, v8::Global<v8::FunctionTemplate>> m_constructorMap;
};

inline v8::Isolate* V8ScriptEnv::isolate() const
{
	return m_isolate;
}

inline v8::Local<v8::Context> V8ScriptEnv::context() const
{
	return persistentToLocal(isolate(), m_context);
}

inline v8::Local<v8::Object> V8ScriptEnv::global() const
{
	return persistentToLocal(isolate(), m_global);
}

inline void V8ScriptEnv::setGlobal(v8::Local<v8::Object> global)
{
	m_global.Reset(isolate(), global);
}

inline v8::Local<v8::ObjectTemplate> V8ScriptEnv::globalTemplate() const
{
	return persistentToLocal(isolate(), m_globalTpl);
}

inline void V8ScriptEnv::setGlobalTemplate(v8::Local<v8::ObjectTemplate> global_tpl)
{
	m_globalTpl.Reset(isolate(), global_tpl);
}

inline v8::Local<v8::FunctionTemplate> V8ScriptEnv::getConstructor(const std::string& key) const
{
	auto it = m_constructorMap.find(key);
	if (it == m_constructorMap.end())
		return v8::Local<v8::FunctionTemplate>();

	return globalPersistentToLocal(isolate(), (*it).second);
}

template<class T>
inline std::unique_ptr<IScriptObject<T>> V8ScriptEnv::wrap(const std::string& constructor_name, T* obj)
{
	// Fetch the v8 isolate and context
	v8::Isolate* pisolate = isolate();
	v8::Local<v8::Context> pcontext = context();
	assert(!pcontext.IsEmpty());

	// Create a stack-allocated scope for v8 calls, and enter context
	v8::Locker locker(pisolate);
	v8::Isolate::Scope isolate_scope(pisolate);
	v8::HandleScope handle_scope(pisolate);
	v8::Context::Scope context_scope(pcontext);

	// Create an instance for the wrapped object
	v8::Local<v8::FunctionTemplate> ctor_tpl = getConstructor(constructor_name);
	v8::Local<v8::ObjectTemplate> obj_tpl = ctor_tpl->InstanceTemplate();
	v8::Local<v8::Object> new_instance = obj_tpl->NewInstance(pcontext).ToLocalChecked();
	new_instance->SetAlignedPointerInInternalField(0, obj);

	return std::make_unique<V8ScriptObject<T>>(obj, pisolate, new_instance);
}

template<class T>
inline T* V8ScriptEnv::unwrap(v8::Local<v8::Value> value) const
{
	T* obj = 0;
	if (value->IsObject())
	{
		v8::MaybeLocal<v8::Object> handle = value->ToObject(context());
		if (!handle.IsEmpty())
			obj = static_cast<T*>(handle.ToLocalChecked()->GetAlignedPointerFromInternalField(0));
	}

	return obj;
}

#endif
