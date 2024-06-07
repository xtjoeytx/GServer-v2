#ifndef V8SCRIPTWRAPPED_H
#define V8SCRIPTWRAPPED_H

#include "ScriptBindings.h"
#include "V8ScriptUtils.h"
#include <unordered_map>
#include <v8.h>

template<class T>
class V8ScriptObject : public IScriptObject<T>
{
public:
	V8ScriptObject(T* object, v8::Isolate* isolate, v8::Local<v8::Object> handle)
		: IScriptObject<T>(object), m_isolate(isolate)
	{
		m_handle.Reset(isolate, handle);
	}

	~V8ScriptObject()
	{
		// clear handle for children
		for (auto it = m_children.begin(); it != m_children.end(); ++it)
		{
			v8::Local<v8::Object> child = GlobalPersistentToLocal(m_isolate, it->second);

			child->SetAlignedPointerInInternalField(0, nullptr);
			it->second.Reset();
		}

		// clear handle
		v8::Local<v8::Object> obj = Handle(m_isolate);
		obj->SetAlignedPointerInInternalField(0, nullptr);
		m_handle.Reset();
	}

	void addChild(const std::string& prop, v8::Local<v8::Object> handle)
	{
		removeChild(prop);

		v8::Global<v8::Object> persist_child;
		persist_child.Reset(m_isolate, handle);
		m_children[prop] = std::move(persist_child);
	}

	void removeChild(const std::string& prop)
	{
		auto it = m_children.find(prop);
		if (it != m_children.end())
		{
			v8::Local<v8::Object> child = GlobalPersistentToLocal(m_isolate, it->second);
			child->SetAlignedPointerInInternalField(0, nullptr);
			it->second.Reset();

			m_children.erase(it);
		}
	}

	v8::Local<v8::Object> Handle(v8::Isolate* isolate) const
	{
		return PersistentToLocal(isolate, m_handle);
	}

	v8::Persistent<v8::Object>& Persistent()
	{
		return m_handle;
	}

	// TODO(joey): This is not implemented just yet. Protect / Unprotect objects from being garbage collected.
	// May not be used because as of now there is no objects you can create in script so..
	//inline void Protect() {
	//	m_object.ClearWeak();
	//}

	//inline void Unprotect() {
	//	m_object.SetWeak(this, _V8WeakObjectCallback, v8::WeakCallbackType::kParameter);
	//	m_object.MarkIndependent();
	//}

	//inline static void _V8WeakObjectCallback(const v8::WeakCallbackInfo<BaseObject>& data) {
	//}

private:
	v8::Isolate* m_isolate;
	v8::Persistent<v8::Object> m_handle;
	std::unordered_map<std::string, v8::Global<v8::Object>> m_children;
};

#endif
