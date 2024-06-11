#ifndef SCRIPTFUNCTION_H
#define SCRIPTFUNCTION_H

class IScriptArguments;

class IScriptFunction
{
public:
	IScriptFunction() : m_referenceCount(0) {}

	virtual ~IScriptFunction() = 0;

	inline bool isReferenced() const
	{
		return m_referenceCount > 0;
	}

	inline int getReferenceCount() const
	{
		return m_referenceCount;
	}

	inline void increaseReference()
	{
		m_referenceCount++;
	}

	inline void decreaseReference()
	{
		m_referenceCount--;
	}

private:
	int m_referenceCount;
};

inline IScriptFunction::~IScriptFunction() = default;

#endif
