#ifndef SCRIPTENGINEGS1_H
#define SCRIPTENGINEGS1_H

#include <cstdint>
#include <optional>
#include <variant>
#include <string_view>
#include <string>
#include <memory>

#include <object/Character.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <scripting/IScriptEngine.h>
#include <scripting/ScriptContainers.h>

// Stupid Windows.h defines
#undef ERROR
#undef TRANSPARENT
#include <antlr4-runtime.h>

#include <GS1Lexer.h>
#include <GS1Parser.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

using PlayerOrNPC = std::optional<std::variant<PlayerPtr, NPCPtr>>;

PlayerPtr getPlayerFromSource(const ScriptObjectSource& source, std::optional<size_t> index = std::nullopt);
PlayerClientPtr getPlayerClientFromSource(const ScriptObjectSource& source, std::optional<size_t> index = std::nullopt);
NPCPtr getNPCFromSource(const ScriptObjectSource& source, std::optional<size_t> index = std::nullopt);
PlayerOrNPC getPlayerOrNPCFromSource(const ScriptObjectSource& source, std::optional<size_t> index = std::nullopt);
Character* getCharacterFromSource(const ScriptObjectSource& source, std::optional<size_t> index = std::nullopt);

//----------------------------

/// @brief A GS1 variable pair of a GameVariable and an index (for an array access).
using GS1GameVariable = std::pair<GameVariableVariant, std::optional<size_t>>;

/// @brief A GS1 script value used in the GS1 visitor pattern.
using GS1ScriptValue = std::variant<GS1GameVariable, GameValue, ScriptObjectSource>;

/// @brief A GS1 object source with an optional GameVariableStore.
using GS1ObjectSourceWithStore = std::pair<ScriptObjectSource, GameVariableStore*>;

//----------------------------

/// @brief Wraps the GS1 script components needed for parsing and execution.
struct GS1ScriptWrapper
{
	GS1ScriptWrapper(std::string_view script);

	std::shared_ptr<antlr4::ANTLRInputStream> input;
	std::shared_ptr<preagonal::grammar::gs1::GS1Lexer> lexer;
	std::shared_ptr<antlr4::CommonTokenStream> tokens;
	std::shared_ptr<preagonal::grammar::gs1::GS1Parser> parser;
	preagonal::grammar::gs1::GS1Parser::ProgramContext* program = nullptr;
};

//----------------------------

class ScriptEngineGS1 : public IScriptEngine
{
public:
	ScriptEngineGS1();
	virtual ~ScriptEngineGS1() override {}

public:
	virtual ScriptEngineMode getExecutionMode() override { return ScriptEngineMode::DIRECT; }
	virtual ScriptExecutionType getExecutionType() override { return ScriptExecutionType::INTERPRETED; }

public:
	virtual CompiledScriptResult compileScript(ScriptType type, std::string_view name, const std::string& script) override;
	virtual bool reset() override { return false; }

public:
	virtual bool execute(const ScriptEvent& event, ScriptObjectSource source, CompiledScriptResultPtr context) override;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1

#endif // SCRIPTENGINEGS1_H
