#pragma once

#include <v8.h>
#include <unordered_map>
#include "ScriptWrapped.h"

template<class T>
class V8ScriptWrapped : public IScriptWrapped<T>
{
public:
	V8ScriptWrapped(T *object, v8::Isolate *isolate, v8::Local<v8::Object> handle)
		: IScriptWrapped<T>(object), _isolate(isolate)
	{
		_handle.Reset(isolate, handle);
	}
	
	~V8ScriptWrapped()
	{
		// clear handle for children
		for (auto it = _children.begin(); it != _children.end(); ++it)
		{
			v8::Local<v8::Object> child = GlobalPersistentToLocal(_isolate, it->second);

			child->SetAlignedPointerInInternalField(0, nullptr);
			it->second.Reset();
		}

		// clear handle
		v8::Local<v8::Object> obj = Handle(_isolate);
		obj->SetAlignedPointerInInternalField(0, nullptr);
		_handle.Reset();
	}

	void addChild(const std::string& prop, v8::Local<v8::Object> handle)
	{
		removeChild(prop);

		v8::Global<v8::Object> persist_child;
		persist_child.Reset(_isolate, handle);
		_children[prop] = std::move(persist_child);
	}

	void removeChild(const std::string& prop)
	{
		auto it = _children.find(prop);
		if (it != _children.end())
		{
			v8::Local<v8::Object> child = GlobalPersistentToLocal(_isolate, it->second);
			child->SetAlignedPointerInInternalField(0, nullptr);
			it->second.Reset();

			_children.erase(it);
		}
	}

	v8::Local<v8::Object> Handle(v8::Isolate *isolate) const {
		return PersistentToLocal(isolate, _handle);
	}
	
	v8::Persistent<v8::Object>& Persistent() {
		return _handle;
	}

	// TODO(joey): This is not implemented just yet. Protect / Unprotect objects from being garbage collected.
	// May not be used because as of now there is no objects you can create in script so..
	//inline void Protect() {
	//	_object.ClearWeak();
	//}

	//inline void Unprotect() {
	//	_object.SetWeak(this, _V8WeakObjectCallback, v8::WeakCallbackType::kParameter);
	//	_object.MarkIndependent();
	//}

	//inline static void _V8WeakObjectCallback(const v8::WeakCallbackInfo<BaseObject>& data) {
	//}

private:
	v8::Isolate *_isolate;
	v8::Persistent<v8::Object> _handle;
	std::unordered_map<std::string, v8::Global<v8::Object>> _children;
};
