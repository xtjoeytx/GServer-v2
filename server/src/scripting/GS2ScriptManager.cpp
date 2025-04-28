#include "scripting/GS2ScriptManager.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

const uint32_t THREADPOOL_WORKERS = 0;

GS2ScriptManager::GS2ScriptManager()
	: m_compilerThreadPool(THREADPOOL_WORKERS)
{
}

std::future<CompilerResponse> GS2ScriptManager::compileScript(const std::string& script, user_callback_type finishedCb)
{
	return queueCompileJob(script, finishedCb);

	// Disabling any async functionality for now, npcs should be compiled during level-loading
	// and level should not be sent until all the npcs are finished compiling. Can't really
	// enforce this since Level is loaded synchronously, but if it does turn into a problem
	// we can migrate to using the threadpool for script compilations and delay sending levels
	// until we finish loading the level. We could also switch to some eager-level-loading method,
	// preloading any levels that are links from other levels or listed in a loaded map etc..
}

std::future<CompilerResponse> GS2ScriptManager::queueCompileJob(const std::string& script, user_callback_type& finishedCb)
{
	std::promise<CompilerResponse> promise;

	if constexpr (THREADPOOL_WORKERS == 0)
	{
		std::promise<CompilerResponse> promise;
		auto response = _context.compile(script);

		if (finishedCb)
			finishedCb(response);

		promise.set_value(std::move(response));
		return promise.get_future();
	}

	// Worker job
	auto threadFunction = [&promise, script, finishedCb, this](CompiledWithCallbackThreadJob::thread_context& context, CompiledWithCallbackThreadJob::promise_type& badPromise)
	{
		// Compile code
		auto result = context.gs2context.compile(script);

		// Call the user-defined callback after we insert the bytecode into the cache
		auto completedFunc = [this, &promise, &script, &finishedCb](CompilerResponse& response)
		{
			finishedCb(response);
			promise.set_value(std::move(response));
		};

		// Create a tuple with the callback, and arguments
		auto fnData = std::make_pair(std::move(completedFunc), std::move(result));

		std::scoped_lock lock(m_cbQueueLock);
		m_cbQueue.push(std::move(fnData));
	};

	// Don't use the future returned by the compiler because it is bad and horrible.
	m_compilerThreadPool.queue(CompiledWithCallbackThreadJob{ std::move(threadFunction) });
	return promise.get_future();
}

void GS2ScriptManager::runQueue()
{
	std::queue<queue_item_type> tmpQueue;

	{
		std::scoped_lock lock(m_cbQueueLock);
		if (!m_cbQueue.empty())
			tmpQueue.swap(m_cbQueue);
	}

	while (!tmpQueue.empty())
	{
		auto& queueItem = tmpQueue.front();

		internal_callback_type& func = queueItem.first;
		CompilerResponse& response = queueItem.second;
		func(response);

		tmpQueue.pop();
	}
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
