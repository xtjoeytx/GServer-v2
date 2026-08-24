#ifndef SCRIPTSYSTEM_H
#define SCRIPTSYSTEM_H

#include <any>
#include <memory>
#include <string_view>
#include <string>
#include <unordered_map>
#include <variant>

#include <utilities/CommonTypes.h>

using namespace std::literals::string_view_literals;

///////////////////////////////////////////////////////////////////////////////
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

//----------------------------

class ScriptSystem
{
public:
	void registerScriptEngine(std::string_view name, std::shared_ptr<IScriptEngine> engine);

public:
	// Gets the compiled client script.
	// Forces the script to be compiled with the GS2 engine, as the client only understands GS2 bytecode.
	CompiledScriptResultPtr getCompiledClientScript(std::string_view who, std::string_view source);

	// Gets the compiled server script.
	CompiledScriptResultPtr getCompiledServerScript(std::string_view who, std::string_view source);

	/// @brief Gets a script engine by name.
	/// @param name The name of the script engine to retrieve.
	/// @return The script engine associated with the given name, or nullptr if no such engine is registered.
	std::shared_ptr<IScriptEngine> getScriptEngine(std::string_view name) const;

	/// @brief Gets the default script engine.
	/// @return The default script engine, or nullptr if no default engine is set or registered.
	[[a::inline]] std::shared_ptr<IScriptEngine> getDefaultScriptEngine() const;

public:
	std::string defaultScriptEngine = "gs1";

private:
	CompiledScriptResultPtr getCompiledScript(IScriptEngine* engine, std::string_view who, std::string_view source);

private:
	string_map<std::shared_ptr<IScriptEngine>> m_script_engines;
	hash_map<CompiledScriptResultPtr> m_script_cache;
};

//----------------------------

inline std::shared_ptr<IScriptEngine> ScriptSystem::getDefaultScriptEngine() const
{
	return getScriptEngine(defaultScriptEngine);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTSYSTEM_H
