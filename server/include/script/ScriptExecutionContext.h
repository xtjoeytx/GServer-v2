#pragma once

#ifndef SCRIPTEXECUTION_H
#define SCRIPTEXECUTION_H

#include <vector>
#include "ScriptAction.h"
#include "ScriptUtils.h"

class ScriptAction;

class ScriptExecutionContext
{
public:
	ScriptExecutionContext() { }
	~ScriptExecutionContext() { resetExecution(); }

	bool hasActions() const;
	void addAction(ScriptAction *action);

	double getExecutionTime();
	unsigned int getExecutionCalls() const;

	void resetExecution();
	void runExecution();

private:
	std::vector<ScriptAction *> _actions;
	std::vector<ScriptTimeSample> _scriptTimeSamples;
};

inline bool ScriptExecutionContext::hasActions() const
{
	return !_actions.empty();
}

inline void ScriptExecutionContext::addAction(ScriptAction *action)
{
	_actions.push_back(action);
}

inline unsigned int ScriptExecutionContext::getExecutionCalls() const
{
	return (unsigned int)_scriptTimeSamples.size();
}

inline double ScriptExecutionContext::getExecutionTime()
{
	double exectime = 0.0;

#ifndef NOSCRIPTPROFILING
	auto time_now = std::chrono::high_resolution_clock::now();

	for (auto it = _scriptTimeSamples.begin(); it != _scriptTimeSamples.end();)
	{
		auto sample_diff = std::chrono::duration_cast<std::chrono::seconds>((*it).expiration - time_now);
		if (sample_diff.count() <= 0)
		{
			it = _scriptTimeSamples.erase(it);
			continue;
		}

		exectime += (*it).sample;
		++it;
	}
#endif

	return exectime;
}

inline void ScriptExecutionContext::resetExecution()
{
	for (auto it = _actions.begin(); it != _actions.end(); ++it)
		delete *it;
	_actions.clear();

	//_scriptTimeSamples.clear();
}

inline void ScriptExecutionContext::runExecution()
{
#ifndef NOSCRIPTPROFILING
	auto currentTimer = std::chrono::high_resolution_clock::now();
#endif

	// iterate over queued actions
	for (auto it = _actions.begin(); it != _actions.end();)
	{
		ScriptAction *action = *it;
		V8ENV_D("Running action: %s\n", action->getAction().c_str());
		action->Invoke();
		it = _actions.erase(it);
		delete action;
	}

#ifndef NOSCRIPTPROFILING
	auto endTimer = std::chrono::high_resolution_clock::now();
	auto time_diff = std::chrono::duration<double>(endTimer - currentTimer);
	_scriptTimeSamples.push_back({ endTimer + std::chrono::seconds(60), time_diff.count() });
#endif
}

#endif
