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
