#ifndef SCRIPTENV_H
#define SCRIPTENV_H

#include "ScriptUtils.h"
#include <functional>

class IScriptFunction;

class IScriptEnv
{
public:
	IScriptEnv() {}
	virtual ~IScriptEnv() {}

	virtual int GetType() const = 0;

	virtual void Initialize()                                                            = 0;
	virtual void Cleanup(bool shutDown = false)                                          = 0;
	virtual IScriptFunction* Compile(const std::string& name, const std::string& source) = 0;
	virtual void CallFunctionInScope(std::function<void()> function)                     = 0;
	virtual void TerminateExecution()                                                    = 0;

	const ScriptRunError& getScriptError() const
	{
		return _lastScriptError;
	}

protected:
	ScriptRunError _lastScriptError;
};

#endif
