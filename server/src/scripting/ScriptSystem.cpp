#include <cassert>
#include <memory>
#include <string_view>
#include <string>
#include <utility>
#include <variant>

#include <scripting/IScriptEngine.h>
#include <scripting/ScriptSystem.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

void ScriptSystem::registerScriptEngine(std::string_view name, std::shared_ptr<IScriptEngine> engine)
{
	// Check if the engine is already registered.
	if (m_script_engines.find(name) != m_script_engines.end())
		log::printLine(log::server, "Script engine '{}' is already registered. Overwriting.", name);

	// Register the script engine.
	m_script_engines.insert_or_assign(std::string{ name }, engine);
}

CompiledScriptResultPtr ScriptSystem::getCompiledClientScript(std::string_view who, std::string_view source)
{
	// Check for empty source.
	auto trimmed = string::trim(source);
	if (trimmed.empty())
		return {};

	// We are using GS2.
	if (auto it = m_script_engines.find("GS2"); it != m_script_engines.end())
		return getCompiledScript(it->second.get(), who, trimmed);

	// Throw at this point.  We should always have a GS2 engine.
	assert(false);
	return {};
}

CompiledScriptResultPtr ScriptSystem::getCompiledServerScript(std::string_view who, std::string_view source)
{
	// Check for empty source.
	auto trimmed = string::trim(source);
	if (trimmed.empty())
		return {};

	// Determine the scripting engine to use.
	// TODO: What do we do about GS1 mixed with GS2?
	std::string script_engine = defaultScriptEngine;
	if (trimmed.starts_with("//#"))
	{
		// Read the line and get the script engine we are going to use.
		auto engine = string::extractLine(trimmed).substr(3);
		if (!engine.empty())
			script_engine = engine;
	}

	// Find the script engine.
	if (auto it = m_script_engines.find(script_engine); it != m_script_engines.end())
		return getCompiledScript(it->second.get(), who, trimmed);

	return {};
}

///////////////////////////////////////////////////////////////////////////////

CompiledScriptResultPtr ScriptSystem::getCompiledScript(IScriptEngine* engine, std::string_view who, std::string_view source)
{
	// Check for a cached script.
	size_t script_hash = string::string_hash{}(source);
	if (auto it = m_script_cache.find(script_hash); it != m_script_cache.end())
		return it->second;

	// Compile the script.
	auto result = engine->compileScript(who, source);
	if (std::holds_alternative<ScriptExecutionContext>(result))
	{
		auto& context = std::get<ScriptExecutionContext>(result);
		auto contextPtr = std::make_shared<ScriptExecutionContext>(std::move(context));
		m_script_cache.insert_or_assign(script_hash, contextPtr);
		return contextPtr;
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
