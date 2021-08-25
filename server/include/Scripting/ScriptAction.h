#pragma once

#ifndef SCRIPTACTION_H
#define SCRIPTACTION_H

#include <cassert>
#include <string>
#include "ScriptArguments.h"

class IScriptFunction;

class ScriptAction
{
public:
	ScriptAction() :
		_function(nullptr), _args(nullptr)
	{

	}

	explicit ScriptAction(IScriptFunction *function, IScriptArguments *args, const std::string& action = "")
		: _function(function), _args(args), _action(action)
	{
		_function->increaseReference();
	}

	ScriptAction(const ScriptAction& o) = delete;
	ScriptAction& operator=(const ScriptAction& o) = delete;

	ScriptAction(ScriptAction&& o) noexcept
	{
		_action = std::move(o._action);
		_args = o._args;
		_function = o._function;

		o._args = nullptr;
		o._function = nullptr;
	}

	ScriptAction& operator=(ScriptAction&& o) noexcept
	{
		_action = std::move(o._action);
		_args = o._args;
		_function = o._function;

		o._args = nullptr;
		o._function = nullptr;
		return *this;
	}

	~ScriptAction()
	{
		if (_args)
		{
			delete _args;
		}

		if (_function)
		{
			_function->decreaseReference();
			if (!_function->isReferenced())
			{
				delete _function;
			}
		}
	}

	void Invoke() const
	{
		assert(_args);

		_args->Invoke(_function);
	}

	const std::string& getAction() const
	{
		return _action;
	}

	IScriptArguments * getArguments() const
	{
		return _args;
	}

	IScriptFunction * getFunction() const
	{
		return _function;
	}

protected:
	std::string _action;
	IScriptArguments *_args;
	IScriptFunction *_function;
};

#endif
