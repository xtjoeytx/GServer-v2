#ifndef SCRIPTSYSTEM_H
#define SCRIPTSYSTEM_H

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <variant>
#include <any>

#include <scripting/ScriptTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std::literals::string_view_literals;

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class IScriptEngine;
class ScriptClass;

struct ScriptExecutionContext
{
	IScriptEngine* engine = nullptr;
	std::shared_ptr<std::any> script;
	std::unordered_map<std::string, std::weak_ptr<ScriptClass>> joinedClasses;
};

using CompiledScriptResult = std::variant<ScriptExecutionContext, std::string>;
using CompiledScriptResultPtr = std::shared_ptr<ScriptExecutionContext>;


class ScriptSystem
{
public:
	void registerScriptEngine(std::string_view name, std::shared_ptr<IScriptEngine> engine);

public:
	// Gets the compiled client script.
	// Forces the script to be compiled with the GS2 engine, as the client only understands GS2 bytecode.
	CompiledScriptResultPtr getCompiledClientScript(ScriptType type, std::string_view name, std::string_view source);

	// Gets the compiled server script.
	CompiledScriptResultPtr getCompiledServerScript(ScriptType type, std::string_view name, std::string_view source);

	void runQueuedEvents();

public:
	std::string defaultScriptEngine = "GS2";

private:
	CompiledScriptResultPtr getCompiledScript(IScriptEngine* engine, ScriptType type, std::string_view name, std::string_view source);

private:
	std::unordered_map<std::string, std::shared_ptr<IScriptEngine>, string::string_hash, std::equal_to<>> m_script_engines;
	std::unordered_map<size_t, CompiledScriptResultPtr> m_script_cache;
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SCRIPTSYSTEM_H
