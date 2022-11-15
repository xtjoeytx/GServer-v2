#pragma once

#ifndef SCRIPTEXECUTION_H
#define SCRIPTEXECUTION_H

#include <algorithm>
#include <chrono>
#include <vector>
#include "ScriptAction.h"
#include "ScriptUtils.h"
#include "CScriptEngine.h"

class ScriptExecutionContext
{
public:
	ScriptExecutionContext(CScriptEngine *scriptEngine)
		: _scriptEngine(scriptEngine) { }

	~ScriptExecutionContext() { resetExecution(); }

	bool hasActions() const;
	std::pair<unsigned int, double> getExecutionData();

	void addAction(ScriptAction& action);
	void addAction(ScriptAction&& action);
	void addExecutionSample(const ScriptTimeSample& sample);
	void resetExecution();
	bool runExecution();

private:
	CScriptEngine *_scriptEngine;
	std::vector<ScriptAction> _actions;
	std::vector<ScriptTimeSample> _scriptTimeSamples;
};

inline bool ScriptExecutionContext::hasActions() const
{
	return !_actions.empty();
}

inline void ScriptExecutionContext::addExecutionSample(const ScriptTimeSample& sample)
{
#ifndef NOSCRIPTPROFILING
	_scriptTimeSamples.push_back(sample);

	// Remove any script samples over a minute old
	//if (_scriptTimeSamples.size() > 1024)
	{
		auto curSampleTime = sample.sample_time;
		while (!_scriptTimeSamples.empty())
		{
			auto oldSample = _scriptTimeSamples.begin();
			auto sample_diff = std::chrono::duration_cast<std::chrono::minutes>(curSampleTime - oldSample->sample_time);
			if (sample_diff.count() < 1)
				break;

			_scriptTimeSamples.erase(oldSample);
		}
	}
#endif
}

inline std::pair<unsigned int, double> ScriptExecutionContext::getExecutionData()
{
	double exectime = 0.0;
	unsigned int calls = 0;

#ifndef NOSCRIPTPROFILING
	auto time_now = std::chrono::high_resolution_clock::now();

	for (auto it = _scriptTimeSamples.begin(); it != _scriptTimeSamples.end();)
	{
		auto sample_diff = std::chrono::duration_cast<std::chrono::minutes>(time_now - (*it).sample_time);
		if (sample_diff.count() >= 1)
		{
			it = _scriptTimeSamples.erase(it);
			continue;
		}

		exectime += (*it).sample;
		calls++;
		++it;
	}
#endif

	return { calls, exectime };
}

inline void ScriptExecutionContext::addAction(ScriptAction& action)
{
	if (action.getFunction()) {
		_actions.push_back(std::move(action));
	}
}

inline void ScriptExecutionContext::addAction(ScriptAction&& action)
{
	if (action.getFunction()) {
		_actions.push_back(std::move(action));
	}
}

inline void ScriptExecutionContext::resetExecution()
{
	_actions.clear();

#ifndef NOSCRIPTPROFILING
	//_scriptTimeSamples.clear();
#endif
}

inline bool ScriptExecutionContext::runExecution()
{
	// Take ownership of the queued actions, and clear them incase any scripts add actions.
	std::vector<ScriptAction> iterateActions = std::move(_actions);
	_actions.clear();

	// Send start timer to engine
	auto currentTimer = std::chrono::high_resolution_clock::now();
	_scriptEngine->StartScriptExecution(currentTimer);

	// iterate over queued actions
	SCRIPTENV_D("Running %zd actions:\n", iterateActions.size());
	for (auto & action : iterateActions)
	{
		SCRIPTENV_D("Running action: %s\n", action.getAction().c_str());
		auto res = action.Invoke();
		if (!res) {
			_scriptEngine->reportScriptException(_scriptEngine->getScriptError());
		}
	}

	if (!_scriptEngine->StopScriptExecution())
	{
		// TODO(joey): Report to server? What should we do, hm.
		printf("Oh no we were killed!!\n");
	}

#ifndef NOSCRIPTPROFILING
	auto endTimer = std::chrono::high_resolution_clock::now();
	auto time_diff = std::chrono::duration<double>(endTimer - currentTimer);
	addExecutionSample({ time_diff.count(), endTimer });
#endif

	return hasActions();
}

#endif
