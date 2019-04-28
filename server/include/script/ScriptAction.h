#pragma once

#include <string>
#include "ScriptArguments.h"

class IScriptFunction;

class ScriptAction
{
public:
	explicit ScriptAction(IScriptFunction *function, IScriptArguments *args, const std::string& action = std::string())
		: _function(function), _args(args), _action(action) {
	}

	ScriptAction(const ScriptAction& other)
		: _function(other.getFunction()), _args(other.getArguments()), _action(other.getAction()) {
		printf("Copy ctor\n");
	}

	ScriptAction(ScriptAction&& other) noexcept
		: _function(std::move(other.getFunction())), _args(std::move(other.getArguments())), _action(std::move(other.getAction())) {
		printf("Move ctor\n");
	}

	~ScriptAction() {
		if (_args) {
			delete _args;
		}
	}

	inline void Invoke() const {
		_args->Invoke(_function);
	}

	inline const std::string& getAction() const {
		return _action;
	}

	inline IScriptArguments * getArguments() const {
		return _args;
	}

	inline IScriptFunction * getFunction() const {
		return _function;
	}

protected:
	std::string _action;
	IScriptArguments *_args;
	IScriptFunction *_function;
};
