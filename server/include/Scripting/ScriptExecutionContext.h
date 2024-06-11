#ifndef SCRIPTEXECUTION_H
#define SCRIPTEXECUTION_H

#include "ScriptAction.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"
#include <algorithm>
#include <chrono>
#include <vector>

class ScriptExecutionContext
{
public:
	ScriptExecutionContext(ScriptEngine* scriptEngine)
		: m_scriptEngine(scriptEngine) {}

	~ScriptExecutionContext() { resetExecution(); }

	bool hasActions() const;

	std::pair<unsigned int, double> getExecutionData();

	void addAction(ScriptAction& action);

	void addAction(ScriptAction&& action);

	void addExecutionSample(const ScriptTimeSample& sample);

	void resetExecution();

	bool runExecution();

private:
	ScriptEngine* m_scriptEngine;
	std::vector<ScriptAction> m_actions;
	std::vector<ScriptTimeSample> m_scriptTimeSamples;
};

inline bool ScriptExecutionContext::hasActions() const
{
	return !m_actions.empty();
}

inline void ScriptExecutionContext::addExecutionSample(const ScriptTimeSample& sample)
{
#ifndef NOSCRIPTPROFILING
	m_scriptTimeSamples.push_back(sample);

	// Remove any script samples over a minute old
	//if (m_scriptTimeSamples.size() > 1024)
	{
		auto curSampleTime = sample.sample_time;
		while (!m_scriptTimeSamples.empty())
		{
			auto oldSample = m_scriptTimeSamples.begin();
			auto sample_diff = std::chrono::duration_cast<std::chrono::minutes>(curSampleTime - oldSample->sample_time);
			if (sample_diff.count() < 1)
				break;

			m_scriptTimeSamples.erase(oldSample);
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

	for (auto it = m_scriptTimeSamples.begin(); it != m_scriptTimeSamples.end();)
	{
		auto sample_diff = std::chrono::duration_cast<std::chrono::minutes>(time_now - (*it).sample_time);
		if (sample_diff.count() >= 1)
		{
			it = m_scriptTimeSamples.erase(it);
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
	if (action.getFunction())
	{
		m_actions.push_back(std::move(action));
	}
}

inline void ScriptExecutionContext::addAction(ScriptAction&& action)
{
	if (action.getFunction())
	{
		m_actions.push_back(std::move(action));
	}
}

inline void ScriptExecutionContext::resetExecution()
{
	m_actions.clear();

#ifndef NOSCRIPTPROFILING
		//m_scriptTimeSamples.clear();
#endif
}

inline bool ScriptExecutionContext::runExecution()
{
	// Take ownership of the queued actions, and clear them incase any scripts add actions.
	std::vector<ScriptAction> iterateActions = std::move(m_actions);
	m_actions.clear();

	// Send start timer to engine
	auto currentTimer = std::chrono::high_resolution_clock::now();
	m_scriptEngine->startScriptExecution(currentTimer);

	// iterate over queued actions
	SCRIPTENV_D("Running %zd actions:\n", iterateActions.size());
	for (auto& action: iterateActions)
	{
		SCRIPTENV_D("Running action: %s\n", action.getAction().c_str());
		auto res = action.invoke();
		if (!res)
		{
			m_scriptEngine->reportScriptException(m_scriptEngine->getScriptError());
		}
	}

	if (!m_scriptEngine->stopScriptExecution())
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
