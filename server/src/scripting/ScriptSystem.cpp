#include <cassert>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <BabyDI.h>
#include <Server.h>
#include <scripting/IScriptEngine.h>
#include <scripting/ScriptSystem.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

void ScriptSystem::registerScriptEngine(const std::string_view name, std::shared_ptr<IScriptEngine> engine)
{
	// Check if the engine is already registered.
	if (m_script_engines.contains(name))
		log::printLine(log::server, "Script engine '{}' is already registered. Overwriting.", name);

	// Register the script engine.
	m_script_engines.insert_or_assign(std::string{name}, engine);

	// Load the configuration file.
	if (const auto server = BabyDI::Get<Server>(); server != nullptr)
	{
		if (const auto config = server->getFileSystemServer().infoi(fs::FileCategory::CONFIG, std::format("scriptengine-{}.txt", name)); config != nullptr)
			engine->loadConfiguration(config->file);
	}
}

CompiledScriptResultPtr ScriptSystem::getCompiledClientScript(const std::string_view who, const std::string_view source)
{
	// Check for empty source.
	const auto trimmed = string::trim(source);
	if (trimmed.empty())
		return nullptr;

	// We are using GS2.
	if (const auto it = m_script_engines.find("gs2"); it != m_script_engines.end())
		return getCompiledScript(it->second.get(), who, trimmed);

	// Throw at this point.  We should always have a GS2 engine.
	assert(false);
	return nullptr;
}

CompiledScriptResultPtr ScriptSystem::getCompiledServerScript(const std::string_view who, const std::string_view source)
{
	// Check for empty source.
	auto trimmed = string::trim(source);
	if (trimmed.empty())
		return nullptr;

	// Determine the scripting engine to use.
	// TODO: What do we do about GS1 mixed with GS2?
	std::string script_engine = defaultScriptEngine;
	if (trimmed.starts_with("//#"))
	{
		// Read the line and get the script engine we are going to use.
		if (const auto engine = string::extractLine(trimmed).substr(3); !engine.empty())
			script_engine = engine;
	}

	// Find the script engine.
	if (const auto it = m_script_engines.find(script_engine); it != m_script_engines.end())
		return getCompiledScript(it->second.get(), who, trimmed);

	return nullptr;
}

std::shared_ptr<IScriptEngine> ScriptSystem::getScriptEngine(const std::string_view name) const
{
	const auto engine = m_script_engines.find(name);
	if (engine == m_script_engines.end())
		return nullptr;

	return engine->second;
}

std::vector<std::tuple<precise_clock::duration, size_t, std::string_view>> ScriptSystem::getExecutionProfiles(const size_t count) const
{
	const auto now = precise_clock::now();

	// Collect all of the results and sort from longest to shortest.
	std::vector<std::tuple<precise_clock::duration, size_t, std::string_view>> result;
	for (const auto& script : m_script_cache | std::views::values | removeNulls)
	{
		// Remove any execution profiles that are older than 1 minute.
		util::truncateContainerWhen(script->executionProfiles, [cutoff = now - std::chrono::minutes(1)](const auto& pair)
		{
			return pair.first < cutoff;
		});

		// Add up the total duration of the remaining execution profiles.
		auto duration = std::ranges::fold_left(script->executionProfiles | std::views::values, precise_clock::duration(0), std::plus<precise_clock::duration>{});

		result.emplace_back(duration, script->executionProfiles.size(), script->identifier);
	}
	std::sort(result.rbegin(), result.rend());

	// Remove any entries that are empty or beyond the count.
	size_t idx = 0;
	util::truncateContainerWhen(result, [&idx, count](const auto& triplet)
	{
		++idx;
		return idx >= count || std::get<0>(triplet).count() == 0;
	});

	return result;
}

///////////////////////////////////////////////////////////////////////////////

CompiledScriptResultPtr ScriptSystem::getCompiledScript(IScriptEngine* engine, const std::string_view who, const std::string_view source)
{
	// Check for a cached script.
	const size_t script_hash = string::string_hash{}(source);
	if (const auto it = m_script_cache.find(script_hash); it != m_script_cache.end())
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
