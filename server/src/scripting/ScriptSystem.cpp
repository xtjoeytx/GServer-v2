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
#include <scripting/gs2/ScriptEngineGS2.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

static std::variant<ScriptCompilationFailureType, CompiledScriptResultPtr> getCompiledScript(IScriptEngine* engine, hash_map<CompiledScriptResultPtr>& cache, ScriptCompilationTarget target, const std::string_view who, const std::string_view source)
{
	// Check cache.
	const size_t script_hash = string::string_hash{}(source);
	if (const auto it = cache.find(script_hash); it != cache.end())
		return it->second;

	// Compile the script.
	auto result = engine->compileScript(target, who, source);
	if (std::holds_alternative<ScriptExecutionContext>(result))
	{
		auto& context = std::get<ScriptExecutionContext>(result);
		auto contextPtr = std::make_shared<ScriptExecutionContext>(std::move(context));
		cache.insert_or_assign(script_hash, contextPtr);
		return contextPtr;
	}
	if (std::holds_alternative<std::pair<ScriptCompilationFailureType, std::string>>(result))
	{
		auto& failure = std::get<std::pair<ScriptCompilationFailureType, std::string>>(result);
		return failure.first;
	}

	return nullptr;
}

static auto getError(std::variant<ScriptCompilationFailureType, CompiledScriptResultPtr>& result) -> std::optional<ScriptCompilationFailureType>
{
	const auto error = std::get_if<ScriptCompilationFailureType>(&result);
	if (error == nullptr) return std::nullopt;
	return *error;
}

static auto getSuccess(std::variant<ScriptCompilationFailureType, CompiledScriptResultPtr>& result) -> std::optional<CompiledScriptResultPtr>
{
	const auto error = std::get_if<CompiledScriptResultPtr>(&result);
	if (error == nullptr) return std::nullopt;
	return *error;
}

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
	auto trimmed = string::trim(source);
	if (trimmed.empty())
		return nullptr;

	// Determine the scripting engine to use.
	std::string script_engine = defaultScriptEngineClient;
	if (trimmed.starts_with("//#"))
	{
		// Read the line and get the script engine we are going to use.
		if (const auto engine = string::extractLine(trimmed).substr(3); !engine.empty())
			script_engine = string::toLower(engine);
	}

	auto compileScript = [this](const std::string_view scriptEngine, const std::string_view target, const std::string_view script) -> std::variant<ScriptCompilationFailureType, CompiledScriptResultPtr>
	{
		if (const auto engine = m_script_engines.find(scriptEngine); engine != m_script_engines.end())
			return getCompiledScript(engine->second.get(), m_script_cache, ScriptCompilationTarget::CLIENT, target, script);
		return ScriptCompilationFailureType::ENGINE_NOT_FOUND;
	};

	// Try to get the compiled script.
	auto result = compileScript(script_engine, who, trimmed);
	if (!getError(result).has_value())
		return getSuccess(result).value();

	// If we got rejected, return nullptr.
	// If we forced a non-default script and it failed, return nullptr.
	if (getError(result).value() == ScriptCompilationFailureType::REJECTED || script_engine != defaultScriptEngineClient)
		return nullptr;

	// Try the default script engine.
	result = compileScript(defaultScriptEngineClient, who, trimmed);
	if (auto success = getSuccess(result); success.has_value())
		return success.value();

	// If we got rejected, return nullptr.
	// If we had tried the gs2 engine and failed, return nullptr.
	if (getError(result).value() == ScriptCompilationFailureType::REJECTED || script_engine == gs2::ScriptEngineGS2::EngineName)
		return nullptr;

	// We try the gs2 engine as a final fallback.
	result = compileScript(gs2::ScriptEngineGS2::EngineName, who, trimmed);
	if (auto success = getSuccess(result); success.has_value())
		return success.value();

	// Still failed.
	return nullptr;
}

CompiledScriptResultPtr ScriptSystem::getCompiledServerScript(const std::string_view who, const std::string_view source)
{
	// Check for empty source.
	auto trimmed = string::trim(source);
	if (trimmed.empty())
		return nullptr;

	// Determine the scripting engine to use.
	std::string script_engine = defaultScriptEngineServer;
	if (trimmed.starts_with("//#"))
	{
		// Read the line and get the script engine we are going to use.
		// Only a name of a registered engine selects one: the old worlds start their server part with a marker comment such as "//#SERVERSIDE"
		// (512 NPCs in Valikorlia), which is no engine; with it taken for one the lookup below failed and the NPC got no server script at all.
		if (const auto engine = string::extractLine(trimmed).substr(3); !engine.empty() && m_script_engines.contains(engine))
			script_engine = engine;
	}

	auto compileScript = [this](const std::string_view scriptEngine, const std::string_view target, const std::string_view script) -> std::variant<ScriptCompilationFailureType, CompiledScriptResultPtr>
	{
		if (const auto engine = m_script_engines.find(scriptEngine); engine != m_script_engines.end())
			return getCompiledScript(engine->second.get(), m_script_cache, ScriptCompilationTarget::SERVER, target, script);
		return ScriptCompilationFailureType::ENGINE_NOT_FOUND;
	};

	// Try to get the compiled script.
	auto result = compileScript(script_engine, who, trimmed);
	if (!getError(result).has_value())
		return getSuccess(result).value();

	// If we got rejected, return nullptr.
	// If we forced a non-default script and it failed, return nullptr.
	if (getError(result).value() == ScriptCompilationFailureType::REJECTED || script_engine != defaultScriptEngineServer)
		return nullptr;

	// Try the default script engine.
	result = compileScript(defaultScriptEngineServer, who, trimmed);
	if (auto success = getSuccess(result); success.has_value())
		return success.value();

	// Failure, return nullptr;
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
} // end namespace preagonal
