#ifndef GS2SCRIPTMANAGER_H
#define GS2SCRIPTMANAGER_H

#include <mutex>
#include <queue>

#include "CompilerThreadJob.h"
#include "GS2Context.h"
#include "interface/ScriptUtils.h"
#include "utils/ContextThreadPool.h"

class GS2ScriptManager
{
	using BytecodeCache = std::unordered_map<std::string, CompilerResponse>;

	// used for threadpool job queue
	using CompilerThreadPool     = CustomThreadPool<CallbackThreadJob>;
	using internal_callback_type = std::function<void(CompilerResponse&)>;
	using queue_item_type        = std::pair<internal_callback_type, CompilerResponse>;

public:
	using user_callback_type = std::function<void(const CompilerResponse&)>;

	GS2ScriptManager();
	~GS2ScriptManager() {}

	void compileScript(const std::string& script, user_callback_type finishedCb);
	void runQueue();

private:
	// Async Compile
	void queueCompileJob(const std::string& script, user_callback_type& finishedCb);

	// Sync Compile
	GS2Context _context;
	void syncCompileJob(const std::string& script, user_callback_type& finishedCb);

	//
	BytecodeCache _bytecodeCache;
	CompilerThreadPool _compilerThreadPool;

	std::queue<queue_item_type> _cbQueue;
	std::mutex _cbQueueLock;
};

#endif
