#ifndef SCRIPTENGINEGS1_H
#define SCRIPTENGINEGS1_H

#include <array>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

#include <ANTLRInputStream.h>
#include <CommonTokenStream.h>
#include <GS1Parser.h>

#include <object/Character.h>
#include <object/NPC.h>
#include <object/Player.h>
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

inline constexpr std::array<CarryObjectSprite, 11> carrySprites =
{
	CarryObjectSprite::BUSH, CarryObjectSprite::SIGN, CarryObjectSprite::VASE, CarryObjectSprite::STONE, CarryObjectSprite::BLACKSTONE,
	CarryObjectSprite::BOMB, CarryObjectSprite::HOTBOMB, CarryObjectSprite::SUPERBOMB, CarryObjectSprite::JOLTBOMB, CarryObjectSprite::HOTJOLTBOMB,
	CarryObjectSprite::NONE
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
	{ ScriptEventType::PLAYERHURT, "playerhurt" },
	{ ScriptEventType::PLAYERDIES, "playerdies" },
	{ ScriptEventType::COMPUSDIED, "compusdied" },
	{ ScriptEventType::NPCWARPED, "npcwarped" },
	{ ScriptEventType::EXPLODED, "exploded" },
	{ ScriptEventType::WASHIT, "washit" },
	{ ScriptEventType::WASSHOT, "wasshot" },
	{ ScriptEventType::WASPELT, "waspelt" },
	{ ScriptEventType::WASTHROWN, "wasthrown" },
	{ ScriptEventType::TIMEOUT, "timeout" },
	{ ScriptEventType::PRIVATEMESSAGE, "pm" },
	{ ScriptEventType::MOVEMENTFINISHED, "movementfinished" },
	//
	{ ScriptEventType::SERVERLISTCONNECT, "serverlistconnect" }
};

///////////////////////////////////////////////////////////////////////////////

struct unimplemented_error : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

struct sleep_exception : public std::exception {};
struct break_exception : public std::exception {};
struct continue_exception : public std::exception {};
struct return_exception : public std::exception {};

///////////////////////////////////////////////////////////////////////////////

using PlayerOrNPC = std::optional<std::variant<PlayerPtr, NPCPtr>>;

PlayerPtr getPlayerFromSource(const ScriptObject& source, std::optional<int64_t> index = std::nullopt);
PlayerClientPtr getPlayerClientFromSource(const ScriptObject& source, std::optional<int64_t> index = std::nullopt);
NPCPtr getNPCFromSource(const ScriptObject& source, std::optional<int64_t> index = std::nullopt);
PlayerOrNPC getPlayerOrNPCFromSource(const ScriptObject& source, std::optional<int64_t> index = std::nullopt);
Character* getCharacterFromSource(const ScriptObject& source, std::optional<int64_t> index = std::nullopt);

//----------------------------

/// @brief A GS1 variable pair of a GameValue and an index (for an array access).
using GS1GameVariable = std::pair<GameValue, std::optional<int64_t>>;

/// @brief A GS1 script value used in the GS1 visitor pattern.
using GS1ScriptValue = std::variant<GS1GameVariable, GameValue, ScriptObject>;

/// @brief A GS1 object source with an optional GameVariableStore.
using GS1ObjectSourceWithStore = std::pair<ScriptObject, GameVariableStore*>;

//----------------------------

/// @brief Wraps the GS1 script components needed for parsing and execution.
struct GS1ScriptWrapper
{
	GS1ScriptWrapper(std::string_view who, std::string_view script);

	std::shared_ptr<grammar::GS1ErrorListener> errorListenerLexer;
	std::shared_ptr<grammar::GS1ErrorListener> errorListenerParser;
	std::shared_ptr<antlr4::ANTLRInputStream> input;
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
	virtual bool execute(ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context) override;
	virtual bool executeFunction(std::string_view function, ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context) override;

protected:
	bool prepare(GS1ScriptWrapper& wrapper, ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context, NPCPtr& npc, LevelPtr& level);
	void cleanup(GS1ScriptWrapper& wrapper);
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1

#endif // SCRIPTENGINEGS1_H
