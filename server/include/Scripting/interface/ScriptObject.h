#ifndef SCRIPTOBJECT_H
#define SCRIPTOBJECT_H

#include <cassert>

template<class T>
class IScriptObject
{
public:
	IScriptObject(T* object)
		: _object(object), _referenceCount(0)
	{
	}

	virtual ~IScriptObject()
	{
		// This assert is triggered when updating levels quickly. The reason for this
		// is because npcs may have actions queued up, and referenceCount doesn't decrease on destructor
		// only when the action is invoked and the arguments are parsed. Will look into this, but shouldn't
		// have any side effects anyway.
		// joey (5/24/19) - believe this is fixed, but leaving the note and enabling the assert
		assert(_referenceCount == 0);
	}

	T* Object() const
	{
		return _object;
	}

	bool isReferenced() const
	{
		return _referenceCount > 0;
	}

	int getReferenceCount() const
	{
		return _referenceCount;
	}

	void increaseReference()
	{
		_referenceCount++;
	}

	void decreaseReference()
	{
		_referenceCount--;
	}

protected:
	T* _object;
	int _referenceCount;
};

#endif
