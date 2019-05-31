#pragma once

#ifndef SCRIPTEXECUTION_H
#define SCRIPTEXECUTION_H

#include <chrono>
#include <vector>
#include "ScriptAction.h"
#include "ScriptUtils.h"
#include "CScriptEngine.h"

class ScriptAction;

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <v8.h>

extern std::atomic<bool> shutdownProgram;

static v8::Isolate *isolate = nullptr;

class ScriptExecutionContext
{
public:
	ScriptExecutionContext(CScriptEngine *scriptEngine)
		: _scriptEngine(scriptEngine)
	{
		if (!isolate) {

			V8ScriptEnv *env = static_cast<V8ScriptEnv *>(scriptEngine->getScriptEnv());
			isolate = env->Isolate();
		}
	}

	~ScriptExecutionContext() { resetExecution(); }

	std::pair<unsigned int, double> getExecutionData();
	void addExecutionSample(const ScriptTimeSample& sample);

	bool hasActions() const;
	void addAction(ScriptAction *action);
	void resetExecution();
	void runExecution();

private:
	CScriptEngine *_scriptEngine;
	std::vector<ScriptAction *> _actions;
	std::vector<ScriptTimeSample> _scriptTimeSamples;
};

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

inline void ScriptExecutionContext::addExecutionSample(const ScriptTimeSample& sample)
{
#ifndef NOSCRIPTPROFILING
	_scriptTimeSamples.push_back(sample);
#endif
}

inline bool ScriptExecutionContext::hasActions() const
{
	return !_actions.empty();
}

inline void ScriptExecutionContext::addAction(ScriptAction *action)
{
	_actions.push_back(action);
}

inline void ScriptExecutionContext::resetExecution()
{
	for (auto it = _actions.begin(); it != _actions.end(); ++it)
		delete *it;
	_actions.clear();

#ifndef NOSCRIPTPROFILING
	//_scriptTimeSamples.clear();
#endif
}

inline void ScriptExecutionContext::runExecution()
{
	static std::atomic<bool> running(false);
	static std::chrono::high_resolution_clock::time_point start_time;
	static std::mutex time_mutex;

	static std::thread thread_object([](v8::Isolate *isolate)
	{
		while (!shutdownProgram.load())
		{
			if (running.load())
			{
				auto time_now = std::chrono::high_resolution_clock::now();
				std::chrono::milliseconds time_diff;
				{
					std::lock_guard<std::mutex> guard(time_mutex);
					time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(time_now - start_time);
				}

				if (time_diff.count() >= 500) {
					isolate->TerminateExecution();
					running.store(false);
					printf("Killed execution for running too long!\n");
				}
			}
			
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}, isolate);

//#ifndef NOSCRIPTPROFILING
	// TODO(joey): needed to kill scripts
	auto currentTimer = std::chrono::high_resolution_clock::now();
//#endif

	{
		std::lock_guard<std::mutex> guard(time_mutex);
		start_time = currentTimer;
	}

	running.store(true);

	// iterate over queued actions
	for (auto it = _actions.begin(); it != _actions.end();)
	{
		ScriptAction *action = *it;
		SCRIPTENV_D("Running action: %s\n", action->getAction().c_str());
		action->Invoke();
		it = _actions.erase(it);
		delete action;
	}

	if (!running.load()) {
		printf("Oh no we were killed!!\n");
	}
	else running.store(false);

#ifndef NOSCRIPTPROFILING
	auto endTimer = std::chrono::high_resolution_clock::now();
	auto time_diff = std::chrono::duration<double>(endTimer - currentTimer);
	addExecutionSample({ time_diff.count(), endTimer });
#endif
}

#endif
