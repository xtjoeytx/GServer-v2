#ifndef SCRIPTENGINEGS1_H
#define SCRIPTENGINEGS1_H

#include <array>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

#include <ANTLRInputStream.h>
#include <CommonTokenStream.h>
#include <GS1Lexer.h>
#include <GS1Parser.h>

#include <object/Character.h>
#include <object/NPC.h>
#include <player/PlayerClient.h>
#include <scripting/gs1/GS1ErrorListener.h>
#include <scripting/IScriptEngine.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>

// Forward declare.
namespace preagonal::gs1::grammar
{
class GS1Visitor;
}

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

inline constexpr std::array<std::string_view, 10> baddyNames =
{
	"graysoldier"sv, "bluesoldier"sv, "redsoldier"sv, "shootingsoldier"sv, "swampsoldier"sv,
	"frog"sv, "octopus"sv, "goldenwarrior"sv, "lizardon"sv, "dragon"sv
};

inline constexpr std::array<std::string_view, 20> colorNames =
{
	"white"sv, "yellow"sv, "orange"sv, "pink"sv, "red"sv,
	"darkred"sv, "lightgreen"sv, "green"sv, "darkgreen"sv, "lightblue"sv,
	"blue"sv, "darkblue"sv, "brown"sv, "cynober"sv, "purple"sv,
	"darkpurple"sv, "lightgray"sv, "gray"sv, "black"sv, "transparent"sv
};

inline constexpr std::array<std::string_view, 4> directionNames =
{
	"up"sv, "left"sv, "down"sv, "right"sv
};

inline constexpr std::array<std::string_view, 2> genderNames =
{
	"male"sv, "female"sv
};

inline constexpr std::array<std::string_view, 11> carryNames =
{
	"bush"sv, "sign"sv, "vase"sv, "stone"sv, "blackstone"sv,
	"bomb"sv, "hotbomb"sv, "superbomb"sv, "joltbomb"sv, "hotjoltbomb"sv,
	"none"sv
};

inline constexpr std::array<std::string_view, 25> itemNames =
{
	"greenrupee"sv, "bluerupee"sv, "redrupee"sv, "bombs"sv, "darts"sv,
	"heart"sv, "glove1"sv, "bow"sv, "bomb"sv, "shield"sv,
	"sword"sv, "fullheart"sv, "superbomb"sv, "battleaxe"sv, "goldensword"sv,
	"mirrorshield"sv, "glove2"sv, "lizardshield"sv, "lizardsword"sv, "goldrupee"sv,
	"fireball"sv, "fireblast"sv, "nukeshot"sv, "joltbomb"sv, "spinattack"sv
};

inline static const std::unordered_map<ScriptEventType, std::string_view> eventFlagMap =
{
	{ ScriptEventType::CREATED, "created" },
	{ ScriptEventType::INITIALIZED, "initialized" },
	{ ScriptEventType::PLAYERLOGIN, "playerlogin" },
	{ ScriptEventType::PLAYERLOGOUT, "playerlogout" },
	{ ScriptEventType::PLAYERENTERS, "playerenters" },
	{ ScriptEventType::PLAYERLEAVES, "playerleaves" },
	{ ScriptEventType::PLAYERTOUCHSME, "playertouchsme" },
	{ ScriptEventType::PLAYERTOUCHSOTHER, "playertouchsother" },
	{ ScriptEventType::PLAYERLAYSITEM, "playerlaysitem" },
	{ ScriptEventType::PLAYERCHATS, "playerchats" },
	{ ScriptEventType::PLAYERDIES, "playerdies" },
	{ ScriptEventType::PLAYERENDREADING, "playerendreading" },
	{ ScriptEventType::WEAPONFIRED, "weaponfired" },
	{ ScriptEventType::FIREDONHORSE, "firedonhorse" },
	{ ScriptEventType::COMPUSDIED, "compusdied" },
	{ ScriptEventType::NPCWARPED, "npcwarped" },
	{ ScriptEventType::EXPLODED, "exploded" },
	{ ScriptEventType::WASHIT, "washit" },
	{ ScriptEventType::WASSHOT, "wasshot" },
	{ ScriptEventType::WASPELT, "waspelt" },
	{ ScriptEventType::TIMEOUT, "timeout" },
	//
	{ ScriptEventType::SERVERLISTCONNECT, "serverlistconnect" }
};

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
	GS1ScriptWrapper(std::string_view who, std::string_view script);

	std::shared_ptr<grammar::GS1ErrorListener> errorListenerLexer;
	std::shared_ptr<grammar::GS1ErrorListener> errorListenerParser;
	std::shared_ptr<antlr4::ANTLRInputStream> input;
	std::shared_ptr<grammar::GS1Lexer> lexer;
	std::shared_ptr<antlr4::CommonTokenStream> tokens;
	std::shared_ptr<grammar::GS1Parser> parser;
	std::shared_ptr<grammar::GS1Visitor> visitor;
	grammar::GS1Parser::ProgramContext* program = nullptr;
	GameVariableStore variables;
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
	virtual CompiledScriptResult compileScript(std::string_view who, std::string_view script) override;
	virtual bool reset() override { return false; }

public:
	virtual bool execute(ScriptEvent& event, ScriptObjectSource source, CompiledScriptResultPtr context) override;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1

#endif // SCRIPTENGINEGS1_H
