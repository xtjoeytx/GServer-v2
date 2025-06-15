#ifndef GS2SCRIPTMANAGER_H
#define GS2SCRIPTMANAGER_H

#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <GS2Context.h>
#include <exceptions/GS2CompilerError.h>
#include <utils/ContextThreadPool.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

// Custom thread pool callback that returns a future to a CompilerResponse, while also allowing a callback.
class CompiledWithCallbackThreadJob
{
public:
	struct job_result
	{
		CompilerResponse response;
	};

	struct thread_context
	{
		GS2Context gs2context;
	};

	using future_type = std::future<job_result>;
	using promise_type = std::promise<job_result>;
	using callback_type = std::function<void(thread_context&, promise_type&)>;

public:
	CompiledWithCallbackThreadJob(callback_type callback)
		: m_fn(std::move(callback))
	{
	}

	void run(thread_context& th_context, promise_type& promise)
	{
		m_fn(th_context, promise);
	}

	static void init(thread_context& th_context) {}

private:
	callback_type m_fn;
};

// The original compiler response cannot be shoved into a future.
struct BetterCompilerResponse
{
	bool success;
	std::vector<GS2CompilerError> errors;
	std::vector<uint8_t> bytecode;
	std::set<std::string> joinedClasses;
};

// GS2 script management.
class GS2ScriptManager
{
	// used for threadpool job queue
	using CompilerThreadPool = CustomThreadPool<CompiledWithCallbackThreadJob>;
	using internal_callback_type = std::function<void(CompilerResponse&)>;
	using queue_item_type = std::pair<internal_callback_type, CompilerResponse>;

public:
	using user_callback_type = std::function<void(const CompilerResponse&)>;

	GS2ScriptManager();
	~GS2ScriptManager() {}

	std::future<CompilerResponse> compileScript(const std::string& script, user_callback_type finishedCb = {});
	void runQueue();

private:
	std::future<CompilerResponse> queueCompileJob(const std::string& script, user_callback_type& finishedCb);

	GS2Context _context;
	CompilerThreadPool m_compilerThreadPool;

	std::queue<queue_item_type> m_cbQueue;
	std::mutex m_cbQueueLock;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // GS2SCRIPTMANAGER_H
