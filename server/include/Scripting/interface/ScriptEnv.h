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

	virtual int getType() const = 0;

	virtual void initialize() = 0;

	virtual void cleanup(bool shutDown = false) = 0;

	virtual IScriptFunction* compile(const std::string& name, const std::string& source) = 0;

	virtual void callFunctionInScope(std::function<void()> function) = 0;

	virtual void terminateExecution() = 0;

	const ScriptRunError& getScriptError() const
	{
		return m_lastScriptError;
	}

protected:
	ScriptRunError m_lastScriptError;
};

#endif
