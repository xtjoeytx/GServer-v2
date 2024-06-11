#ifndef SCRIPTACTION_H
#define SCRIPTACTION_H

#include "ScriptArguments.h"
#include "ScriptFunction.h"
#include <cassert>
#include <string>

class ScriptAction
{
public:
	ScriptAction() : m_function(nullptr), m_args(nullptr)
	{
	}

	explicit ScriptAction(IScriptFunction* function, IScriptArguments* args, const std::string& action = "")
		: m_function(function), m_args(args), m_action(action)
	{
		m_function->increaseReference();
	}

	ScriptAction(const ScriptAction& o) = delete;

	ScriptAction& operator=(const ScriptAction& o) = delete;

	ScriptAction(ScriptAction&& o) noexcept
	{
		m_action = std::move(o.m_action);
		m_args = o.m_args;
		m_function = o.m_function;

		o.m_args = nullptr;
		o.m_function = nullptr;
	}

	ScriptAction& operator=(ScriptAction&& o) noexcept
	{
		m_action = std::move(o.m_action);
		m_args = o.m_args;
		m_function = o.m_function;

		o.m_args = nullptr;
		o.m_function = nullptr;
		return *this;
	}

	~ScriptAction()
	{
		if (m_args)
		{
			delete m_args;
		}

		if (m_function)
		{
			m_function->decreaseReference();
			if (!m_function->isReferenced())
			{
				delete m_function;
			}
		}
	}

	bool invoke() const
	{
		assert(m_args);

		return m_args->invoke(m_function, true);
	}

	const std::string& getAction() const
	{
		return m_action;
	}

	IScriptArguments* getArguments() const
	{
		return m_args;
	}

	IScriptFunction* getFunction() const
	{
		return m_function;
	}

protected:
	std::string m_action;
	IScriptArguments* m_args;
	IScriptFunction* m_function;
};

#endif
