#ifndef SCRIPTENGINEGS1_H
#define SCRIPTENGINEGS1_H

#include <scripting/IScriptEngine.h>

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

enum class CursorNumbers : uint8_t
{
	DEFAULT = 0,
	HIDDEN = 1,
	NORMAL = 2,
	CROSS = 3,
	TEXT = 4,
	HIDDEN_2 = 5,
	RESIZE_LL_UR = 6,
	RESIZE_UD = 7,
	RESIZE_UL_LR = 8,
	RESIZE_LR = 9,
	UP_ARROW = 10,
	HOURGLASS = 11,
	FILE = 12,
	NOT_ALLOWED = 13,
	BREAK_ADJUST_LR = 14,
	BREAK_ADJUST_UD = 15,
	MULTIPLE_FILES = 16,
	SQL_HOURGLASS = 17,
	NOT_ALLOWED_2 = 18,
	MOUSE_HOURGLASS = 19,
	MOUSE_QUESTION = 20,
	POINTING_HAND = 21,
	FOUR_DIR_ARROW = 22,

	COUNT
};

enum class GameFeatureFlags : uint32_t
{
	PAUSE = 0x0002,
	WEAPONSELECT = 0x0004,
	SPARRATING = 0x0008,
	DROPITEM = 0x0010,
	SWITCHWEAPON = 0x0020,
	CHAT = 0x0040,
	MESSAGETEXT = 0x0080,
	OTHERPLAYERHEARTS = 0x0100,
	NICKNAMES = 0x0200,
	TOALL_PM_BUBBLES = 0x0400,
	VIEWPROFILE = 0x0800,
	EMOTICONS = 0x1000,
	LEVELSNAPSHOTS = 0x2000,
	LEVELZOOMING = 0x4000,
	LOGFRAME = 0x8000,
	ALLFEATURES = 0xFFFF
};

enum class GraalColors : uint8_t
{
	WHITE = 0,
	YELLOW,
	ORANGE,
	PINK,
	RED,
	DARKRED,
	LIGHTGREEN,
	GREEN,
	DARKGREEN,
	LIGHTBLUE,
	BLUE,
	DARKBLUE,
	BROWN,
	CYNOBER,
	PURPLE,
	DARKPURPLE,
	LIGHTGRAY,
	GRAY,
	BLACK,
	TRANSPARENT,

	COUNT
};
constexpr size_t GRAAL_COLOR_MAX = static_cast<size_t>(GraalColors::COUNT);

//----------------------------

std::string getGraalColorName(GraalColors color);

//----------------------------

struct GS1ScriptWrapper
{
	GS1ScriptWrapper(std::string_view script);

	std::shared_ptr<antlr4::ANTLRInputStream> input;
	std::shared_ptr<preagonal::grammar::gs1::GS1Lexer> lexer;
	std::shared_ptr<antlr4::CommonTokenStream> tokens;
	std::shared_ptr<preagonal::grammar::gs1::GS1Parser> parser;
	preagonal::grammar::gs1::GS1Parser::ProgramContext* program = nullptr;
};

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
	virtual bool execute(const ScriptEvent& event, ScriptEventSource source, CompiledScriptResultPtr context) override;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1

#endif // SCRIPTENGINEGS1_H
