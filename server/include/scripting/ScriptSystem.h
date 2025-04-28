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

#include "scripting/IScriptedObject.h"
#include "utilities/StringUtils.h"

///////////////////////////////////////////////////////////////////////////////

using namespace std::literals::string_view_literals;

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class IScriptEngine;

enum class ScriptType
{
	CLASS,
	WEAPON,

	COUNT
};
constexpr size_t SCRIPT_TYPE_COUNT = static_cast<size_t>(ScriptType::COUNT);

struct ScriptEvents
{
	static constexpr std::string_view CREATED = "created"sv;
	static constexpr std::string_view INITIALIZED = "initialized"sv;
	static constexpr std::string_view PLAYERENTERS = "playerenters"sv;
	static constexpr std::string_view PLAYERLEAVES = "playerleaves"sv;
	static constexpr std::string_view PLAYERTOUCHSME = "playertouchsme"sv;
	static constexpr std::string_view PLAYERTOUCHESME = "playertouchesme"sv;
	static constexpr std::string_view PLAYERLOGIN = "playerlogin"sv;
	static constexpr std::string_view PLAYERLOGOUT = "playerlogout"sv;
	static constexpr std::string_view PLAYERCHATS = "playerchats"sv;
	static constexpr std::string_view PLAYERDIES = "playerdies"sv;
	static constexpr std::string_view TIMEOUT = "timeout"sv;
	static constexpr std::string_view WARPED = "warped"sv;
	static constexpr std::string_view NPCWARPED = "npcwarped"sv;
	static constexpr std::string_view EXPLODED = "exploded"sv;
	static constexpr std::string_view SERVERLISTCONNECT = "serverlistconnect"sv;
};

struct ScriptEventState
{
	bool created = false;
	bool initialized = false;
	bool playerenters = false;
	bool playerleaves = false;
	bool playertouchesme = false;
	bool playerlogin = false;
	bool playerlogout = false;
	bool playerchats = false;
	bool playerdies = false;
	bool timeout = false;
	bool warped = false;
	bool npcwarped = false;
	bool exploded = false;
	bool serverlistconnect = false;
	std::vector<std::string> custom;
};

struct ScriptCompilationResult
{
	bool success = false;
	std::string errorMessage;
	std::shared_ptr<std::vector<uint8_t>> bytecode;
	std::set<std::string> joinedClasses;
};
using ScriptCompilationResultPtr = std::shared_ptr<ScriptCompilationResult>;

class ScriptSystem
{
public:
	void registerScriptEngine(std::string_view name, std::shared_ptr<IScriptEngine> engine);

public:
	bool queueEvent(std::shared_ptr<IScriptedObject>& script_object, std::string_view script_event);

	template<typename... Args>
	constexpr bool queueEvent(std::shared_ptr<IScriptedObject>& script_object, std::string_view script_event, Args...)
	{
		assert(false);
		return false;
	}

	// Gets the compiled client script.
	// Forces the script to be compiled with the GS2 engine, as the client only understands GS2 bytecode.
	ScriptCompilationResultPtr getCompiledClientScript(ScriptType type, std::string_view name, std::string_view source);

	// Gets the compiled server script.
	ScriptCompilationResultPtr getCompiledServerScript(ScriptType type, std::string_view name, std::string_view source);

	void runQueuedEvents();

public:
	std::string defaultScriptEngine = "GS2";

private:
	ScriptCompilationResultPtr getCompiledScript(IScriptEngine* engine, ScriptType type, std::string_view name, std::string_view source);

private:
	std::unordered_map<std::string, std::shared_ptr<IScriptEngine>, string::string_hash, std::equal_to<>> m_script_engines;
	std::unordered_map<size_t, ScriptCompilationResultPtr> m_script_cache;
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SCRIPTSYSTEM_H
