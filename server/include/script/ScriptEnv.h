#pragma once

#include "ScriptRunError.h"

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
		
		inline const ScriptRunError& getScriptError() const {
			return _lastScriptError;
		}
	
	protected:
		ScriptRunError _lastScriptError;
};

