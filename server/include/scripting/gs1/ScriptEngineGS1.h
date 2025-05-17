#ifndef SCRIPTENGINEGS1_H
#define SCRIPTENGINEGS1_H

#include "scripting/IScriptEngine.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

struct ScriptEventFlagNames
{
	static constexpr std::string_view CREATED = "created"sv;
	static constexpr std::string_view INITIALIZED = "initialized"sv;
	static constexpr std::string_view PLAYERLOGIN = "playerlogin"sv;
	static constexpr std::string_view PLAYERLOGOUT = "playerlogout"sv;
	static constexpr std::string_view PLAYERENTERS = "playerenters"sv;
	static constexpr std::string_view PLAYERLEAVES = "playerleaves"sv;
	static constexpr std::string_view PLAYERTOUCHSME = "playertouchsme"sv;
	static constexpr std::string_view PLAYERTOUCHSOTHER = "playertouchsother"sv;
	static constexpr std::string_view PLAYERLAYSITEM = "playerlaysitem"sv;
	static constexpr std::string_view PLAYERCHATS = "playerchats"sv;
	static constexpr std::string_view PLAYERDIES = "playerdies"sv;
	static constexpr std::string_view PLAYERENDREADING = "playerendreading"sv;
	static constexpr std::string_view WEAPONFIRED = "weaponfired"sv;
	static constexpr std::string_view FIREDONHORSE = "firedonhorse"sv;
	static constexpr std::string_view COMPUSDIED = "compusdied"sv;
	static constexpr std::string_view WARPED = "warped"sv;
	static constexpr std::string_view NPCWARPED = "npcwarped"sv;
	static constexpr std::string_view EXPLODED = "exploded"sv;
	static constexpr std::string_view WASHIT = "washit"sv;
	static constexpr std::string_view WASSHOT = "wasshot"sv;
	static constexpr std::string_view WASPELT = "waspelt"sv;
	static constexpr std::string_view TIMEOUT = "timeout"sv;
	//
	static constexpr std::string_view SERVERLISTCONNECT = "serverlistconnect"sv;
	static constexpr std::string_view PLAYERTOUCHESME = "playertouchesme"sv;
	static constexpr std::string_view PLAYERTOUCHESOTHER = "playertouchesother"sv;
};

class ScriptEngineGS1 : public IScriptEngine
{
public:
	virtual ~ScriptEngineGS1() override {}

public:
	virtual ScriptEngineMode getExecutionMode() override { return ScriptEngineMode::DIRECT; }
	virtual ScriptExecutionType getExecutionType() override { return ScriptExecutionType::INTERPRETED; }

public:
	virtual CompiledScriptResult compileScript(ScriptType type, std::string_view name, const std::string& script) override { return {}; }
	virtual bool execute(const ScriptEvent& event, ScriptVariableStore* object_variables, ScriptVariableStore* level_variables) override { return false; }
	virtual bool reset() override { return false; }
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SCRIPTENGINEGS1_H
