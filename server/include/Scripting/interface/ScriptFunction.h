#pragma once

#ifndef SCRIPTFUNCTION_H
#define SCRIPTFUNCTION_H

class IScriptArguments;

class IScriptFunction
{
public:
	IScriptFunction() : _referenceCount(0) { }
	
	virtual ~IScriptFunction() = 0;

	inline bool isReferenced() const {
		return _referenceCount > 0;
	}

	inline int getReferenceCount() const {
		return _referenceCount;
	}

	inline void increaseReference() {
		_referenceCount++;
	}

	inline void decreaseReference() {
		_referenceCount--;
	}

private:
	int _referenceCount;
};

inline IScriptFunction::~IScriptFunction() = default;

#endif
