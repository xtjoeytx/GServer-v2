#pragma once

#include <v8.h>
#include "ScriptWrapped.h"

template<class T>
class V8ScriptWrapped : public IScriptWrapped<T>
{
public:
	V8ScriptWrapped(T *object, v8::Isolate *isolate, v8::Local<v8::Object> handle)
		: IScriptWrapped<T>(object) {
		_handle.Reset(isolate, handle);
	}
	
	~V8ScriptWrapped() {
		_handle.Reset();
	}

	inline v8::Local<v8::Object> Handle(v8::Isolate *isolate) const {
		return PersistentToLocal(isolate, _handle);
	}
	
	inline v8::Persistent<v8::Object>& Persistent() {
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
	v8::Persistent<v8::Object> _handle;
};
