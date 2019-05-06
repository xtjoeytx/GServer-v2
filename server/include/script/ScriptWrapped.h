#pragma once

#include <assert.h>

// default implementation
template <typename T>
struct TypeName
{
	static const char * Get()
	{
		return typeid(T).name();
	}
};

template<class T>
class IScriptWrapped
{
public:
	IScriptWrapped(T *object)
		: _object(object), _referenceCount(0) {
	}
	
	virtual ~IScriptWrapped() {
		// TODO(joey): This assert is triggered when updating levels quickly. The reason for this
		// is because npcs may have actions queued up, and referenceCount doesn't decrease on destructor
		// only when the action is invoked and the arguments are parsed. Will look into this, but shouldn't
		// have any side effects anyway.
		assert(_referenceCount == 0);
	}
	
	inline T * Object() const {
		return _object;
	}
	
	inline bool isReferenced() const {
		return _referenceCount > 0;
	}

	inline int getReferenceCount() const {
		return _referenceCount;
	}

	inline void increaseReference() {
		_referenceCount++;
		printf("Increase reference: %d (%s)\n", _referenceCount, TypeName<T>::Get());
	}

	inline void decreaseReference() {
		_referenceCount--;
		printf("Decrease reference: %d (%s)\n", _referenceCount, TypeName<T>::Get());
	}

protected:
	T *_object;
	int _referenceCount;
};
