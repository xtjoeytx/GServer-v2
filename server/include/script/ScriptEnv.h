#pragma once

#ifndef SCRIPTENV_H
#define SCRIPTENV_H

#include <functional>
#include "ScriptUtils.h"

class IScriptFunction;

class IScriptEnv
{
	public:
		IScriptEnv() {}
		virtual ~IScriptEnv() {}
	
		virtual int GetType() = 0;
	
		virtual void Initialize() = 0;
		virtual void Cleanup() = 0;
		virtual IScriptFunction * Compile(const std::string& name, const std::string& source) = 0;
		
		virtual void CallFunctionInScope(std::function<void()> function) = 0;

		inline const ScriptRunError& getScriptError() const {
			return _lastScriptError;
		}
	
	protected:
		ScriptRunError _lastScriptError;
};

#endif
