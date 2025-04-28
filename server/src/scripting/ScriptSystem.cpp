#include "scripting/ScriptSystem.h"

#include <cassert>

#include "scripting/IScriptEngine.h"
#include "utilities/StringUtils.h"
#include "utilities/Log.h"

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

bool ScriptSystem::queueEvent(std::shared_ptr<IScriptedObject>& script_object, std::string_view script_event)
{
	return false;
}

ScriptCompilationResultPtr ScriptSystem::getCompiledClientScript(ScriptType type, std::string_view name, std::string_view source)
{
	// Check for empty source.
	auto trimmed = string::trim(source);
	if (trimmed.empty())
		return {};

	// We are using GS2.
	if (auto it = m_script_engines.find("GS2"); it != m_script_engines.end())
		return getCompiledScript(it->second.get(), type, name, trimmed);

	// Throw at this point.  We should always have a GS2 engine.
	assert(false);
	return {};
}

ScriptCompilationResultPtr ScriptSystem::getCompiledServerScript(ScriptType type, std::string_view name, std::string_view source)
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
		return getCompiledScript(it->second.get(), type, name, trimmed);

	return {};
}

void ScriptSystem::runQueuedEvents()
{
}

///////////////////////////////////////////////////////////////////////////////

ScriptCompilationResultPtr ScriptSystem::getCompiledScript(IScriptEngine* engine, ScriptType type, std::string_view name, std::string_view source)
{
	// Check for a cached script.
	size_t script_hash = string::string_hash{}(source);
	if (auto it = m_script_cache.find(script_hash); it != m_script_cache.end())
		return it->second;

	// Compile the script.
	auto result = engine->compileScript(type, name, std::string{ source });
	if (result->success)
		m_script_cache.insert_or_assign(script_hash, result);

	return result;
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
