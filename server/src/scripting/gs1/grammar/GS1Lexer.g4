lexer grammar GS1Lexer;

@lexer::header
{
// --------------------------------------------------------
#include <array>
#include <cctype>
#include <cstdint>
#include <deque>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
using namespace std::literals;
// --------------------------------------------------------
}

@lexer::context
{
// --------------------------------------------------------
static constexpr std::array reservedConstants =
{
    "pi"sv,
    "allstats"sv,
    "allfeatures"sv,
};

static constexpr bool isReservedConstant(const std::string_view name)
{
    for (const auto& constant : reservedConstants)
    {
        if (name.compare(constant) == 0)
            return true;
    }
    return false;
}

static constexpr std::string_view trimRight(const std::string_view view)
{
	for (size_t i = view.size(); i > 0; --i)
	{
		if (const auto ch = view[i - 1]; !std::isspace(static_cast<unsigned char>(ch)))
			return view.substr(0, i);
	}
	return {};
}
// --------------------------------------------------------
}

@lexer::members
{
// --------------------------------------------------------
/*
    Mode parameter argument guide:
    - V  variable (number/array/string)
    - R  expression (variable + math)
    - P  parameters (multiple expressions)
    - S  string
    - M  raw string (string that doesn't process message codes)
    - U  translatable string (raw string that accepts the #U2 message code)
    - K  variable length comma-separated string list
    - X  message code
    - B  baddy name
    - L  carry item name
    - C  color name
    - D  direction name or number
    - I  item name
    - Z  code (putnpc2 special case)
    - (  left parenthesis
    - )  right parenthesis
    - <  left parenthesis that tests if a comma is found before the ) and, if not, skips the next mode (playersays special case)

    Unused official types:
    - F  filename
    - Q  filename for putnpc, for some reason
*/

using PrototypeList = std::vector<std::pair<std::string_view, std::string_view>>;

// --------------------------------------------------------
// ---[ COMMANDS ]-----------------------------------------

PrototypeList registeredCommands =
{
    {"addguildmember"sv,        "SSS"sv},
    {"addstring"sv,             "VS"sv},        // SS
    {"addtiledef "sv,           "SSR"sv},       // SSR
    {"addtiledef2"sv,           "SSRR"sv},      // SSRR
    {"addweapon"sv,             "S"sv},
    {"attachplayertoobj"sv,     "RR"sv},        // RR
    {"blockagain"sv,            ""sv},          //
    {"blockagainlocal"sv,       ""sv},          //
    {"callnpc"sv,               "RS"sv},        // RS
    {"callweapon"sv,            "RSS"sv},       // RS
    {"canbecarried"sv,          ""sv},          //
    {"canbepulled"sv,           ""sv},          //
    {"canbepushed"sv,           ""sv},          //
    {"cannotbecarried"sv,       ""sv},          //
    {"cannotbepulled"sv,        ""sv},          //
    {"cannotbepushed"sv,        ""sv},          //
    {"cannotwarp"sv,            ""sv},          //
    {"canwarp"sv,               ""sv},          //
    {"canwarp2"sv,              ""sv},          //
    {"carryobject"sv,           "L"sv},         // L
    {"changeimgcolors"sv,       "RRRRR"sv},     // RRRRR
    {"changeimgmode"sv,         "RR"sv},        // RR
    {"changeimgpart"sv,         "RRRRR"sv},     // RRRRR
    {"changeimgvis"sv,          "RR"sv},        // RR
    {"changeimgzoom"sv,         "RR"sv},        // RR
    {"copylevel"sv,             "SS"sv},
    {"copystrings"sv,           "SS"sv},
    {"deletelevel"sv,           "S"sv},
    {"deletestring"sv,          "VR"sv},        // SR
    {"destroy"sv,               ""sv},          //
    {"detachplayer"sv,          ""sv},          //
    {"disabledefmovement"sv,    ""sv},          //
    {"disablemap"sv,            ""sv},          //
    {"disablepause"sv,          ""sv},          //
    {"disableselectweapons"sv,  ""sv},          //
    {"disableweapons"sv,        ""sv},          //
    {"dontblock"sv,             ""sv},          //
    {"dontblocklocal"sv,        ""sv},          //
    {"drawaslight"sv,           ""sv},          //
    {"drawoverplayer"sv,        ""sv},          //
    {"drawovertrees"sv,         ""sv},
    {"drawunderplayer"sv,       ""sv},          //
    {"enabledefmovement"sv,     ""sv},          //
    {"enablefeatures"sv,        "R"sv},         // R
    {"enablemap"sv,             ""sv},          //
    {"enablepause"sv,           ""sv},          //
    {"enableselectweapons"sv,   ""sv},          //
    {"enableweapons"sv,         ""sv},          //
    {"explodebomb"sv,           "R"sv},         // R
    {"followplayer"sv,          ""sv},          //
    {"freezeplayer"sv,          "R"sv},         // R
    {"freezeplayer2"sv,         ""sv},          //
    {"hide"sv,                  ""sv},          //
    {"hideimg"sv,               "R"sv},         // R
    {"hideimgs"sv,              "RR"sv},        // RR
    {"hidelocal"sv,             ""sv},          //
    {"hideplayer"sv,            "R"sv},         // R
    {"hidesword"sv,             "R"sv},         // R
    {"hitcompu"sv,              "RRRR"sv},      // RRRR
    {"hitnpc"sv,                "RRRR"sv},      // RRRR
    {"hitobjects"sv,            "RRR"sv},       // RRR
    {"hitplayer"sv,             "RRRR"sv},      // RRRR
    {"hurt "sv,                 "R"sv},         // R
    {"insertstring"sv,          "VRS"sv},       // SRS
    {"join"sv,                  "S"sv},         // S
    {"lay "sv,                  "I"sv},         // I
    {"lay2"sv,                  "IRR"sv},       // IRR
    {"loadmap"sv,               "S"sv},         // S
    {"message"sv,               "S"sv},         // S
    {"move "sv,                 "RRRR"sv},      // RRRR
    {"noplayerkilling"sv,       ""sv},          //
    {"noplayeronwall"sv,        ""sv},          //
    {"openurl "sv,              "M"sv},         // M
    {"openurl2 "sv,             "MRR"sv},       // MRR
    {"play "sv,                 "S"sv},         // F
    {"play2 "sv,                "SRRR"sv},      // FRRR
    {"playlooped"sv,            "S"sv},         // F
    {"putbomb"sv,               "RRR"sv},       // RRR
    {"putcomp"sv,               "BRR"sv},       // BRR
    {"putexplosion "sv,         "RRR"sv},       // RRR
    {"putexplosion2"sv,         "RRRR"sv},      // RRRR
    {"puthorse"sv,              "MRR"sv},       // MRR
    {"putleaps"sv,              "RRR"sv},       // RRR
    {"putnewcomp"sv,            "BRRMR"sv},     // BRRMR
    {"putnpc"sv,                "SSRR"sv},      // QQRR
    {"putnpc2"sv,               "RRZ"sv},       // RRS
    {"putobject"sv,             "SRR"sv},       // ORR
    {"reducebombs"sv,           "R"sv},
    {"reducedarts"sv,           "R"sv},
    {"reducerupees"sv,          "R"sv},
    {"reflectarrow"sv,          "R"sv},         // R
    {"removearrow"sv,           "R"sv},         // R
    {"removebomb"sv,            "R"sv},         // R
    {"removecompus"sv,          ""sv},          //
    {"removeexplo"sv,           "R"sv},         // R
    {"removeguild"sv,           "S"sv},
    {"removeguildmember"sv,     "SSS"sv},
    {"removehorse"sv,           "R"sv},         // R
    {"removeitem"sv,            "R"sv},         // R
    {"removestring"sv,          "VS"sv},        // SS
    {"removetiledefs"sv,        "S"sv},         // S
    {"removeweapon"sv,          "S"sv},
    {"replaceani"sv,            "MM"sv},        // MM
    {"replacestring"sv,         "VRS"sv},       // SRS
    {"resetfocus"sv,            ""sv},          //
    {"saveinfo"sv,              "SS"sv},
    {"savelog"sv,               "S"sv},         // S
    {"savelog2"sv,              "MS"sv},        // MS
    {"say "sv,                  "R"sv},         // R
    {"say2"sv,                  "S"sv},         // S
    {"sendpm"sv,                "S"sv},         // S
    {"sendrpgmessage"sv,        "S"sv},         // S
    {"sendtonc"sv,              "S"sv},         // S
    {"sendtorc"sv,              "S"sv},         // S
    {"serverwarp"sv,            "S"sv},         // S
    {"set "sv,                  "V"sv},         // M
    {"setani"sv,                "S"sv},         // SS
    {"setarray"sv,              "VR"sv},        // RR
    {"setbackpal"sv,            "S"sv},         // F
    {"setbacktile"sv,           "R"sv},         // R
    {"setbacktile2"sv,          "RRRRR"sv},     // RRRRR
    {"setbeltcolor"sv,          "C"sv},         // C
    {"setbody"sv,               "S"sv},         // F
    {"setbow"sv,                "S"sv},         // S
    {"setcharani"sv,            "S"sv},         // SS
    {"setchargender"sv,         "S"sv},         // S
    {"setcharprop"sv,           "XS"sv},        // MS
    {"setcoatcolor"sv,          "C"sv},         // C
    {"setcoloreffect"sv,        "RRRR"sv},      // RRRR
    {"setcursor "sv,            "R"sv},         // R
    {"setcursor2"sv,            "S"sv},         // F
    {"seteffect "sv,            "RRP"sv},       // RRRR
    {"seteffectmode"sv,         "R"sv},         // R
    {"setfocus"sv,              "RR"sv},        // RR
    {"setgender"sv,             "S"sv},         // S
    {"setgif "sv,               "S"sv},         // F
    {"setgifpart"sv,            "SRRRR"sv},     // FRRRR
    {"sethead"sv,               "S"sv},         // F
    {"setimg "sv,               "S"sv},         // F
    {"setimgpart"sv,            "SRRRR"sv},     // FRRRR
    {"setletters"sv,            "S"sv},         // F
    {"setlevel "sv,             "S"sv},         // M
    {"setlevel2"sv,             "SRR"sv},       // MRR
    {"setmap"sv,                "SSRR"sv},      // FFRR
    {"setminimap"sv,            "SSRR"sv},      // FFRR
    {"setmusicvolume"sv,        "RR"sv},        // RR
    {"setplayerdir"sv,          "D"sv},         // D
    {"setplayerprop"sv,         "XS"sv},        // MS
    {"setplayerx"sv,            "R"sv},
    {"setplayery"sv,            "R"sv},
    {"setpm"sv,                 "S"sv},         // S
    {"setshape "sv,             "RRR"sv},       // RRR
    {"setshape2"sv,             "RRR"sv},       // RRR
    {"setshield"sv,             "SR"sv},        // MR
    {"setshoecolor"sv,          "C"sv},         // C
    {"setshootparams "sv,       "K"sv},         // S
    {"setskincolor"sv,          "C"sv},         // C
    {"setsleevecolor"sv,        "C"sv},         // C
    {"setspritesimage"sv,       "S"sv},
    {"setstatusimage"sv,        "S"sv},
    {"setstring"sv,             "VS"sv},        // SS
    {"setsword"sv,              "SR"sv},        // MR
    {"seturllevel"sv,           "M"sv},         // M
    {"setx"sv,                  "R"sv},
    {"sety"sv,                  "R"sv},
    {"setz "sv,                 "RRRRRRRR"sv},  // RRRRRRRR
    {"setzoomeffect"sv,         "R"sv},         // R
    {"shoot "sv,                "RRRRRRS"sv},   // RRRRRRSS
    {"shootarrow"sv,            "D"sv},         // D
    {"shootball"sv,             ""sv},          // [GR] D
    {"shootfireball"sv,         "D"sv},         // D
    {"shootfireblast"sv,        "D"sv},         // D
    {"shootnuke"sv,             "D"sv},         // D
    {"show"sv,                  ""sv},          //
    {"showani"sv,               "RRRDS"sv},     // RRRRS
    {"showani2"sv,              "RRRRDS"sv},    // RRRRRS
    {"showcharacter"sv,         ""sv},          //
    {"showfile"sv,              "S"sv},         // F
    {"showimg"sv,               "RSRR"sv},      // RMRR
    {"showimg2"sv,              "RSRRR"sv},     // RMRRR
    {"showlocal"sv,             ""sv},          //
    {"showpoly"sv,              "RR"sv},        // RR
    {"showpoly2"sv,             "RR"sv},        // RR
    {"showstats"sv,             "R"sv},         // R
    {"showtext"sv,              "RRRSSS"sv},    // RRRSSS
    {"showtext2"sv,             "RRRRSSS"sv},   // RRRRSSS
    {"sleep"sv,                 "R"sv},         // R
    {"spyfire"sv,               "RR"sv},        // RR
    {"stopmidi"sv,              ""sv},          //
    {"stopsound"sv,             "S"sv},         // F
    {"take "sv,                 "I"sv},         // I
    {"take2"sv,                 "R"sv},         // R
    {"takehorse"sv,             "R"sv},         // R
    {"takeplayercarry"sv,       ""sv},          //
    {"takeplayerhorse"sv,       ""sv},          //
    {"throwcarry"sv,            ""sv},          //
    {"timereverywhere"sv,       ""sv},          //
    {"timershow"sv,             ""sv},          //
    {"toinventory"sv,           "S"sv},         // S
    {"tokenize "sv,             "S"sv},         // S
    {"tokenize2"sv,             "SS"sv},        // SS
    {"toweapons"sv,             "M"sv},         // M
    {"triggeraction"sv,         "RRSK"sv},      // RRSS
    {"unfreezeplayer"sv,        ""sv},          //
    {"unset "sv,                "V"sv},         // M
    {"updateboard "sv,          "RRRR"sv},      // RRRR  (clientside has no space)
    {"updateboard2 "sv,         "RRRR"sv},
    {"updateterrain"sv,         ""sv},          //
    {"warpto"sv,                "SRR"sv},
    {"wraptext "sv,             "RSS"sv},       // RSS
    {"wraptext2 "sv,            "RRSS"sv},      // RRSS
};

// --------------------------------------------------------
// ---[ FUNCTIONS ]----------------------------------------

PrototypeList registeredFunctions =
{
    {"abs"sv,                   "(P)"sv},
    {"aindexof"sv,              "(P)"sv},
    {"arctan"sv,                "(P)"sv},
    {"arraylen"sv,              "(P)"sv},
    {"ascii"sv,                 "(P)"sv},
    {"base64decode"sv,          "(S)"sv},
    {"base64encode"sv,          "(S)"sv},
    {"cos"sv,                   "(P)"sv},
    {"exp"sv,                   "(P)"sv},
    {"findnearestplayer"sv,     "(P)"sv},
    {"findnearestplayers"sv,    "(P)"sv},
    {"getangle"sv,              "(P)"sv},
    {"getareanpcs"sv,           "(P)"sv},
    {"getdir"sv,                "(P)"sv},
    {"getflagkeys"sv,           "(S)"sv},
    {"getnearestplayer"sv,      "(P)"sv},
    {"getnearestplayers"sv,     "(P)"sv},
    {"getnpc"sv,                "(S)"sv},
    {"getplayer"sv,             "(S)"sv},
    {"getz"sv,                  "(P)"sv},
    {"hasright"sv,              "(SS)"sv},
    {"hasweapon"sv,             "(S)"sv},
    {"imgheight"sv,             "(S)"sv},
    {"imgwidth"sv,              "(S)"sv},
    {"indexof"sv,               "(SS)"sv},
    {"int"sv,                   "(P)"sv},
    {"keycode"sv,               "(S)"sv},
    {"keydown"sv,               "(P)"sv},
    {"keydown2"sv,              "(P)"sv},
    {"lindexof"sv,              "(SV)"sv},
    {"log"sv,                   "(P)"sv},
    {"max"sv,                   "(P)"sv},
    {"min"sv,                   "(P)"sv},
    {"onmapx"sv,                "(S)"sv},
    {"onmapy"sv,                "(S)"sv},
    {"onwall"sv,                "(P)"sv},
    {"onwall2"sv,               "(P)"sv},
    {"onwater"sv,               "(P)"sv},
    {"onwater2"sv,              "(P)"sv},
    {"passwordmatches"sv,       "(SS)"sv},
    {"playersays"sv,            "<RS)"sv},
    {"playersays2"sv,           "<RS)"sv},
    {"random"sv,                "(P)"sv},
    {"sarraylen"sv,             "(V)"sv},
    {"screenx"sv,               "(P)"sv},
    {"screeny"sv,               "(P)"sv},
    {"sin"sv,                   "(P)"sv},
    {"startswith"sv,            "(SS)"sv},
    {"strcontains"sv,           "(SS)"sv},
    {"strequals"sv,             "(SS)"sv},
    {"strlen"sv,                "(S)"sv},
    {"strtofloat"sv,            "(S)"sv},
    {"testbomb"sv,              "(P)"sv},
    {"testcompu"sv,             "(P)"sv},
    {"testexplo"sv,             "(P)"sv},
    {"testhorse"sv,             "(P)"sv},
    {"testitem"sv,              "(P)"sv},
    {"testnpc"sv,               "(P)"sv},
    {"testplayer"sv,            "(P)"sv},
    {"testsign"sv,              "(P)"sv},
    {"textheight"sv,            "(RSSS)"sv},
    {"textwidth"sv,             "(RSSS)"sv},
    {"tiletype"sv,              "(P)"sv},
    {"vecx"sv,                  "(P)"sv},
    {"vecy"sv,                  "(P)"sv},
    {"worldx"sv,                "(P)"sv},
    {"worldy"sv,                "(P)"sv},
};

// --------------------------------------------------------
// ---[ LEXER MANIPULATION ]-------------------------------

constexpr bool addNewCommand(std::string_view name, std::string_view prototype)
{
    auto existing = std::ranges::find(registeredCommands, name, [](const auto& pair) { return pair.first; });
    if (existing == std::ranges::end(registeredCommands))
    {
        registeredCommands.emplace_back(name, prototype);
        return true;
    }
    existing->second = prototype;
    return false;
}

constexpr bool addNewFunction(std::string_view name, std::string_view prototype)
{
    auto existing = std::ranges::find(registeredFunctions, name, [](const auto& pair) { return pair.first; });
    if (existing == std::ranges::end(registeredFunctions))
    {
        registeredFunctions.emplace_back(name, prototype);
        return true;
    }
    existing->second = prototype;
    return false;
}

constexpr bool isRegisteredCommand(std::string_view name)
{
    for (const auto& builtIn : registeredCommands | std::views::keys)
    {
        if (trimRight(builtIn) == name)
            return true;
    }
    return false;
}

constexpr bool isRegisteredFunction(std::string_view name)
{
    for (const auto& builtIn : registeredFunctions | std::views::keys)
    {
        if (builtIn == name)
            return true;
    }
    return false;
}

constexpr std::string_view getPrototype(PrototypeList& list, std::string_view commandName)
{
    for (const auto& [builtIn, prototype] : list)
    {
        if (trimRight(builtIn) == commandName)
            return prototype;
    }
    return {};
}

// --------------------------------------------------------
// ---[ LEXER LOGIC ]--------------------------------------

virtual std::unique_ptr<antlr4::Token> nextToken() override
{
    // If we have a before token queued, return it.
    if (!m_pendingTokensBefore.empty())
    {
        auto token = std::move(m_pendingTokensBefore.front());
        m_pendingTokensBefore.pop_front();
        return token;
    }

    // Check for any after tokens now.
    if (!m_pendingTokensAfter.empty())
    {
        auto token = std::move(m_pendingTokensAfter.front());
        m_pendingTokensAfter.pop_front();
        return token;
    }

    // Get the next token.
    auto next = antlr4::Lexer::nextToken();
#if DEBUG
    auto text = next->getText();
#endif

    // If we have before tokens queued, save this one for later and return the queued token.
    if (!m_pendingTokensBefore.empty())
    {
        auto token = std::move(m_pendingTokensBefore.front());
        m_pendingTokensBefore.pop_front();
        m_pendingTokensBefore.push_front(std::move(next));
        return token;
    }

    // Return our token.
    return next;
}

void emitIdentifierBefore(size_t type, std::string_view name)
{
    m_pendingTokensBefore.emplace_back(_factory->create(type, ""));
}

void emitIdentifierAfter(size_t type, std::string_view name)
{
    m_pendingTokensAfter.emplace_back(_factory->create(type, ""));
}

int breakpoint()
{
    return _input->index();
}

enum POPMODE
{
    POPMODE_COMMAND,
    POPMODE_FUNCTION,
    POPMODE_ARRAYINDEX
};

struct CommandState
{
    std::string_view arguments;
    POPMODE popMode;
    bool commaPop = true;
    size_t braceCount = 0;  // {}
    size_t parenCount = 0;  // ()
};

bool canFuncPop() const
{
    return !m_commandStates.empty() && m_commandStates.back().popMode == POPMODE_FUNCTION;
}

bool canCmdPop() const
{
    return m_commandStates.empty() || m_commandStates.back().popMode == POPMODE_COMMAND;
}

bool canArrayPop() const
{
    return !m_commandStates.empty() && m_commandStates.back().popMode == POPMODE_ARRAYINDEX;
}

bool canCommaPop() const
{
    return m_commandStates.empty() || m_commandStates.back().commaPop;
}

bool shouldFuncPop() const
{
    return canFuncPop() && m_commandStates.back().parenCount == 0;
}

bool shouldCmdPop() const
{
    return canCmdPop() && m_commandStates.back().braceCount == 0;
}

void incParen()
{
    if (!m_commandStates.empty())
        ++m_commandStates.back().parenCount;
}

void decParen()
{
    if (!m_commandStates.empty() && m_commandStates.back().parenCount > 0)
        --m_commandStates.back().parenCount;
}

void incBrace()
{
    if (!m_commandStates.empty())
        ++m_commandStates.back().braceCount;
}

void decBrace()
{
    if (!m_commandStates.empty() && m_commandStates.back().braceCount > 0)
        --m_commandStates.back().braceCount;
}

bool isNextArgLeftParen() const
{
    return !m_commandStates.empty() && !m_commandStates.back().arguments.empty() && m_commandStates.back().arguments.front() == '(';
}

bool isNotDefaultMode() const
{
    return !m_commandStates.empty();
}

bool containsValidCommand(std::string_view name)
{
    for (const auto& builtIn : registeredCommands | std::views::keys)
    {
        if (name.starts_with(builtIn))
            return true;
    }
    return false;
}

void pushCommand(std::string_view arguments)
{
    if (arguments.empty()) return;
    m_commandStates.emplace_back(CommandState{arguments, POPMODE_COMMAND, true});
    pushMode(IN_PARAM_1);	// Just a dummy state that gets immediately cleared.
    popNextMode();
}

void pushArrayAccess()
{
    m_commandStates.emplace_back(CommandState{"P", POPMODE_ARRAYINDEX, false});
    pushMode(IN_PARAM_1);	// Just a dummy state that gets immediately cleared.
    popNextMode();
}

void checkIfNextModeOptional()
{
    popNextMode();

    // Look ahead for comma and right parenthesis.
    bool skipNext = false;
    try
    {
        size_t symbol = EOF;
        size_t index = 1;
        while ((symbol = _input->LA(index++)) != EOF)
        {
            // If we reached the end, we didn't have a comma, so skip the next mode.
            if (symbol == ')')
            {
                skipNext = true;
                break;
            }

            // We found a comma, so we can stop looking.
            if (symbol == ',')
                break;
        }
    }
    catch (...)
    {
        // Who cares.
    }

    // If we are skipping the next mode, do that now.
    if (skipNext)
        popNextMode();
}

void popNextMode(bool terminateEarly = false)
{
    if (m_commandStates.empty()) return;
    auto& currentState = m_commandStates.back();

    if (terminateEarly) currentState.arguments = {};
    if (currentState.arguments.empty())
    {
        popMode();
        m_commandStates.pop_back();
    }
    else
    {
        auto mode = currentState.arguments.front();
        currentState.arguments.remove_prefix(1);

        // Last string?  Commas are included!
        if ((mode == 'S' || mode == 'M' || mode == 'U') && (currentState.arguments.empty() || currentState.arguments.front() == ')'))
            currentState.commaPop = false;

        switch (mode)
        {
            case 'V': setMode(IN_PARAM_V); emitIdentifierAfter(GS1Lexer::IDENTIFIER, getText()); break;
            case 'R': setMode(IN_PARAM_R); break;
            case 'P': setMode(IN_PARAM_R); currentState.commaPop = false; break;
            case 'S': setMode(IN_PARAM_S); emitIdentifierAfter(GS1Lexer::STRING, getText()); break;
            case 'M': setMode(IN_PARAM_M); emitIdentifierAfter(GS1Lexer::STRING, getText()); break;
            case 'U': setMode(IN_PARAM_U); emitIdentifierAfter(GS1Lexer::STRING, getText()); break;
            case 'K': setMode(IN_PARAM_K); emitIdentifierAfter(GS1Lexer::STRING, getText()); break;
            case 'X': setMode(IN_PARAM_X); emitIdentifierAfter(GS1Lexer::RAWMESSAGECODE, getText()); break;
            case 'B': setMode(IN_PARAM_B); break;
            case 'I': setMode(IN_PARAM_I); break;
            case 'C': setMode(IN_PARAM_C); break;
            case 'L': setMode(IN_PARAM_L); break;
            case 'D': setMode(IN_PARAM_D); break;
            case 'Z': setMode(IN_PARAM_Z); break;
            case '(': setMode(IN_PARAM_1); break;
            case ')': setMode(IN_PARAM_2); break;
            case '<': setMode(IN_PARAM_3); break;
            default: setMode(DEFAULT_MODE); break;
        }
    }
}

std::deque<CommandState> m_commandStates;
std::deque<std::unique_ptr<antlr4::Token>> m_pendingTokensBefore{};
std::deque<std::unique_ptr<antlr4::Token>> m_pendingTokensAfter{};
// --------------------------------------------------------
}


tokens { MESSAGECODE, RAWMESSAGECODE, STRING, BADDY, ITEM, COLOR, GENDER, CARRY, DIRECTION }

COMMAND
    : [a-zA-Z0-9]+[ ]?  { isRegisteredCommand(getText()) }?   { pushCommand(getPrototype(registeredCommands, getText())); }
    ;

FUNCTION
    : [a-zA-Z0-9]+      { isRegisteredFunction(getText()) && _input->LA(1) == '(' }?    { pushCommand(getPrototype(registeredFunctions, getText())); }
    ;

MC_ESCAPE		: '##'          { setText("#"); }         -> type(IDENTIFIER);
MC_NOINDEX		: '#' ([angcmWw1235678NDLFfpbES] | 'C' [01234567] | 'P1' DIGITS? | 'P2' DIGITS? | 'P3' '0'? | 'P' [456789]) { _input->LA(1) != '(' }? -> type(MESSAGECODE);
MC_SIMPLE		: '#' ([angcmWw1235678NDptKkG]   | 'C' [01234567] | 'P1' DIGITS? | 'P2' DIGITS? | 'P3' '0'? | 'P' [456789]) { pushCommand("(P)"); }   -> type(MESSAGECODE);
MC_COMPUTED_S	: '#s'          { pushCommand("(V)"); }   -> type(MESSAGECODE);
MC_COMPUTED_V	: '#v'          { pushCommand("(R)"); }   -> type(MESSAGECODE);
MC_I			: '#I'          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
MC_T			: '#T'          { pushCommand("(S)"); }   -> type(MESSAGECODE);
MC_E			: '#E'          { pushCommand("(S)"); }   -> type(MESSAGECODE);
MC_U			: '#U'          { pushCommand("(U)"); }   -> type(MESSAGECODE);
MC_U2			: '#U2'         { pushCommand("(S)"); }   -> type(MESSAGECODE);
MC_e			: '#e'          { pushCommand("(RRS)"); } -> type(MESSAGECODE);
MC_i			: '#i'          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
MC_R			: '#R'          { pushCommand("(K)"); }   -> type(MESSAGECODE);
MC_Q            : '#Q'          { pushCommand("(SS)"); }  -> type(MESSAGECODE);

// Keep above KW_TRUE/KW_FALSE.
LITERAL
    : REAL
    | KW_TRUE
    | KW_FALSE
    ;

KW_WITH			: 'with';
KW_FUNCTION		: 'function' { pushCommand("V()"); };
KW_IF			: 'if';
KW_ELSE			: 'else';
KW_FOR			: 'for';
KW_WHILE		: 'while';
KW_RETURN		: 'return';
KW_BREAK		: 'break';
KW_CONTINUE		: 'continue';
KW_TRUE			: 'true';
KW_FALSE		: 'false';

OP_ASSIGN		: '=';
OP_ASSIGN2		: ':='	-> type(OP_ASSIGN);
OP_ADD			: '+';
OP_SUB			: '-';
OP_MUL			: '*';
OP_DIV			: '/';
OP_MOD			: '%';
OP_POW			: '^';
OP_ASSIGN_ADD	: '+=';
OP_ASSIGN_SUB	: '-=';
OP_ASSIGN_MUL	: '*=';
OP_ASSIGN_DIV	: '/=';
OP_ASSIGN_MOD	: '%=';
OP_ASSIGN_POW	: '^=';
OP_EQUAL		: '==';
OP_NOTEQ		: '!=';
OP_NOTEQ2		: '<>'	-> type(OP_NOTEQ);
OP_LESS			: '<';
OP_GREAT		: '>';
OP_LESS_EQ		: '<=';
OP_LESS_EQ2		: '=<'	-> type(OP_LESS_EQ);
OP_GREAT_EQ		: '>=';
OP_GREAT_EQ2	: '=>'	-> type(OP_GREAT_EQ);
OP_IN			: ' in ';
OP_INC			: '++';
OP_DEC			: '--';
OP_LOGICALAND	: '&&';
OP_LOGICALOR	: '||';
OP_LOGICALNOT	: '!';

TOKEN_BRACKET_LEFT  : '[' { pushArrayAccess(); };
TOKEN_BRACKET_RIGHT : ']';
TOKEN_BRACE_LEFT	: '{';
TOKEN_BRACE_RIGHT	: '}' { emitIdentifierBefore(GS1Lexer::END, getText()); };
TOKEN_PAREN_LEFT	: '(';
TOKEN_PAREN_RIGHT	: ')';
TOKEN_COMMA			: ',';
TOKEN_PIPE			: '|';
TOKEN_QUESTION		: '?';
TOKEN_COLON			: ':';
TOKEN_PERIOD		: '.';

RESERVEDCONSTANTS
    : 'pi'
    | 'allstats'
    | 'allfeatures'
    ;

LINECOMMENT
    : '//' ~ [\r\n]* -> channel(HIDDEN)
    ;

BLOCKCOMMENT
    : '/*' .*? '*/' -> channel(HIDDEN)
    ;

REAL
    : DIGITS+ ('.' DIGITS+)?
    | '.' DIGITS+
    | '0x' HEXDIGITS+
    ;

IDENTIFIER
    : [a-zA-Z0-9_]+ { isNotDefaultMode() || !(containsValidCommand(getText()) || isRegisteredFunction(getText())) }?
    ;

END
    : ';'
    ;

WS
    : WHITESPACE+ -> channel(HIDDEN)
    ;

fragment BADDY
    : 'graysoldier'
    | 'bluesoldier'
    | 'redsoldier'
    | 'shootingsoldier'
    | 'swampsoldier'
    | 'frog'
    | 'octopus'
    | 'spider'
    | 'goldenwarrior'
    | 'lizardon'
    | 'dragon'
    ;

fragment COLORS
    : 'white'
    | 'yellow'
    | 'orange'
    | 'pink'
    | 'red'
    | 'darkred'
    | 'lightgreen'
    | 'green'
    | 'darkgreen'
    | 'lightblue'
    | 'blue'
    | 'darkblue'
    | 'brown'
    | 'cynober'
    | 'purple'
    | 'darkpurple'
    | 'lightgray'
    | 'gray'
    | 'black'
    | 'transparent'
    ;

fragment DIR
    : 'up'
    | 'left'
    | 'down'
    | 'right'
    ;

fragment CARRYNAMES
    : 'bush'
    | 'sign'
    | 'vase'
    | 'stone'
    | 'blackstone'
    | 'bomb'
    | 'hotbomb'
    | 'superbomb'
    | 'joltbomb'
    | 'hotjoltbomb'
    | 'none'
    ;

fragment ITEMNAMES
    : 'greenrupee'
    | 'bluerupee'
    | 'redrupee'
    | 'bombs'
    | 'darts'
    | 'heart'
    | 'glove1'
    | 'bow'
    | 'bomb'
    | 'shield'
    | 'sword'
    | 'fullheart'
    | 'superbomb'
    | 'battleaxe'
    | 'goldensword'
    | 'mirrorshield'
    | 'glove2'
    | 'lizardshield'
    | 'lizardsword'
    | 'goldrupee'
    | 'fireball'
    | 'fireblast'
    | 'nukeshot'
    | 'joltbomb'
    | 'spinattack'
    ;

fragment LETTERS
    : [a-zA-Z]
    ;

fragment DIGITS
    : [0-9]
    ;

fragment HEXDIGITS
    : [0-9a-fA-F]
    ;

fragment WHITESPACE
    : [ \r\n\t]
    ;

///////////////////////////////////////////////////////////
// COMMAND PARSING
///////////////////////////////////////////////////////////

// --------------------------------------------------------
// ---[ VARIABLE ]-----------------------------------------

mode IN_PARAM_V;

PARAM_V_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_V_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?          { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_V_POP_PAREN_LEFT  : TOKEN_PAREN_LEFT  { isNextArgLeftParen() }? { popNextMode(); popNextMode(); } -> type(TOKEN_PAREN_LEFT);
PARAM_V_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }?         { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_V_POP_END         : END               { canCmdPop() }?          { popNextMode(true); } -> type(END);
PARAM_V_POP_COMMA       : TOKEN_COMMA                                 { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_V_FUNCTION        : FUNCTION      { pushCommand(getPrototype(registeredFunctions, getText())); } -> type(FUNCTION);
PARAM_V_MC_ESCAPE       : MC_ESCAPE     { setText("#"); }         -> type(IDENTIFIER);
PARAM_V_MC_NOINDEX      : MC_NOINDEX                              -> type(MESSAGECODE);
PARAM_V_MC_SIMPLE       : MC_SIMPLE     { pushCommand("(P)"); }   -> type(MESSAGECODE);
PARAM_V_MC_COMPUTED_S   : MC_COMPUTED_S { pushCommand("(V)"); }   -> type(MESSAGECODE);
PARAM_V_MC_COMPUTED_V   : MC_COMPUTED_V { pushCommand("(R)"); }   -> type(MESSAGECODE);
PARAM_V_MC_I            : MC_I          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
PARAM_V_MC_T            : MC_T          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_V_MC_E            : MC_E          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_V_MC_U            : MC_U          { pushCommand("(U)"); }   -> type(MESSAGECODE);
PARAM_V_MC_U2           : MC_U2         { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_V_MC_e            : MC_e          { pushCommand("(RRS)"); } -> type(MESSAGECODE);
PARAM_V_MC_i            : MC_i          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
PARAM_V_MC_R            : MC_R          { pushCommand("(K)"); }   -> type(MESSAGECODE);
PARAM_V_MC_Q            : MC_Q          { pushCommand("(SS)"); }  -> type(MESSAGECODE);
PARAM_V_LITERAL         : LITERAL -> type(LITERAL);
PARAM_V_RESERVEDCONSTANTS : RESERVEDCONSTANTS -> type(RESERVEDCONSTANTS);
PARAM_V_IDENTIFIER        : IDENTIFIER -> type(IDENTIFIER);
PARAM_V_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT { pushArrayAccess(); } -> type(TOKEN_BRACKET_LEFT);
PARAM_V_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
PARAM_V_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
PARAM_V_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);
PARAM_V_TOKEN_PERIOD        : TOKEN_PERIOD -> type(TOKEN_PERIOD);

// --------------------------------------------------------
// ---[ EXPRESSION (REAL) ]--------------------------------

mode IN_PARAM_R;

PARAM_R_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_R_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { shouldCmdPop()  }?  { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_R_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { shouldFuncPop() }?  { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_R_POP_END         : END               { canCmdPop() }?      { popNextMode(true); } -> type(END);
PARAM_R_POP_COMMA       : TOKEN_COMMA       { canCommaPop() }?    { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_R_COMMA           : TOKEN_COMMA       { !canCommaPop() }?   -> type(TOKEN_COMMA);
PARAM_R_FUNCTION        : FUNCTION      { pushCommand(getPrototype(registeredFunctions, getText())); } -> type(FUNCTION);
PARAM_R_LITERAL         : LITERAL       -> type(LITERAL);
PARAM_R_IDENTIFIER      : IDENTIFIER    -> type(IDENTIFIER);
PARAM_R_OP_ASSIGN       : OP_ASSIGN     -> type(OP_ASSIGN);
PARAM_R_OP_ADD          : OP_ADD        -> type(OP_ADD);
PARAM_R_OP_SUB          : OP_SUB        -> type(OP_SUB);
PARAM_R_OP_MUL          : OP_MUL        -> type(OP_MUL);
PARAM_R_OP_DIV          : OP_DIV        -> type(OP_DIV);
PARAM_R_OP_MOD          : OP_MOD        -> type(OP_MOD);
PARAM_R_OP_POW          : OP_POW        -> type(OP_POW);
PARAM_R_OP_EQUAL        : OP_EQUAL      -> type(OP_EQUAL);
PARAM_R_OP_NOTEQ        : OP_NOTEQ      -> type(OP_NOTEQ);
PARAM_R_OP_LESS         : OP_LESS       -> type(OP_LESS);
PARAM_R_OP_GREAT        : OP_GREAT      -> type(OP_GREAT);
PARAM_R_OP_LESS_EQ      : OP_LESS_EQ    -> type(OP_LESS_EQ);
PARAM_R_OP_GREAT_EQ     : OP_GREAT_EQ   -> type(OP_GREAT_EQ);
PARAM_R_OP_IN           : OP_IN         -> type(OP_IN);
PARAM_R_OP_INC          : OP_INC        -> type(OP_INC);
PARAM_R_OP_DEC          : OP_DEC        -> type(OP_DEC);
PARAM_R_OP_LOGICALAND   : OP_LOGICALAND -> type(OP_LOGICALAND);
PARAM_R_OP_LOGICALOR    : OP_LOGICALOR  -> type(OP_LOGICALOR);
PARAM_R_OP_LOGICALNOT   : OP_LOGICALNOT -> type(OP_LOGICALNOT);
PARAM_R_TOKEN_BRACKET_LEFT      : TOKEN_BRACKET_LEFT                     { pushArrayAccess(); } -> type(TOKEN_BRACKET_LEFT);
PARAM_R_POP_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT { canArrayPop() }? { popNextMode(); }     -> type(TOKEN_BRACKET_RIGHT);
PARAM_R_TOKEN_BRACKET_RIGHT     : TOKEN_BRACKET_RIGHT { !canArrayPop() }?                       -> type(TOKEN_BRACKET_RIGHT);
PARAM_R_TOKEN_BRACE_LEFT    : TOKEN_BRACE_LEFT  { incBrace(); } -> type(TOKEN_BRACE_LEFT);
PARAM_R_TOKEN_BRACE_RIGHT   : TOKEN_BRACE_RIGHT { decBrace(); } -> type(TOKEN_BRACE_RIGHT);
PARAM_R_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT  { incParen(); } -> type(TOKEN_PAREN_LEFT);
PARAM_R_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT { decParen(); } -> type(TOKEN_PAREN_RIGHT);
PARAM_R_TOKEN_PIPE          : TOKEN_PIPE        -> type(TOKEN_PIPE);
PARAM_R_TOKEN_QUESTION      : TOKEN_QUESTION    -> type(TOKEN_QUESTION);
PARAM_R_TOKEN_COLON         : TOKEN_COLON       -> type(TOKEN_COLON);
PARAM_R_TOKEN_PERIOD        : TOKEN_PERIOD      -> type(TOKEN_PERIOD);

// --------------------------------------------------------
// ---[ STRING ]-------------------------------------------

mode IN_PARAM_S;

PARAM_S_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?   { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_S_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }?  { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_S_POP_END         : END               { canCmdPop() }?   { popNextMode(true); } -> type(END);
PARAM_S_POP_COMMA       : TOKEN_COMMA       { canCommaPop() }? { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_S_MC_ESCAPE       : MC_ESCAPE                            { setText("#"); }      -> type(STRING);
PARAM_S_MC_NOINDEX      : MC_NOINDEX                              -> type(MESSAGECODE);
PARAM_S_MC_SIMPLE       : MC_SIMPLE     { pushCommand("(P)"); }   -> type(MESSAGECODE);
PARAM_S_MC_COMPUTED_S   : MC_COMPUTED_S { pushCommand("(V)"); }   -> type(MESSAGECODE);
PARAM_S_MC_COMPUTED_V   : MC_COMPUTED_V { pushCommand("(R)"); }   -> type(MESSAGECODE);
PARAM_S_MC_I            : MC_I          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
PARAM_S_MC_T            : MC_T          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_S_MC_E            : MC_E          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_S_MC_U            : MC_U          { pushCommand("(U)"); }   -> type(MESSAGECODE);
PARAM_S_MC_U2           : MC_U2         { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_S_MC_e            : MC_e          { pushCommand("(RRS)"); } -> type(MESSAGECODE);
PARAM_S_MC_i            : MC_i          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
PARAM_S_MC_R            : MC_R          { pushCommand("(K)"); }   -> type(MESSAGECODE);
PARAM_S_MC_Q            : MC_Q          { pushCommand("(SS)"); }  -> type(MESSAGECODE);
PARAM_S_STRING_ESCAPE   : '##'                                            -> type(STRING);
PARAM_S_STRING_LITERAL1 : ~[#),]+     { canFuncPop() && canCommaPop()  }? -> type(STRING);
PARAM_S_STRING_LITERAL2 : ~[#};,]+    { canCmdPop()  && canCommaPop()  }? -> type(STRING);
PARAM_S_STRING_LITERAL_END1 : ~[#)]+  { canFuncPop() && !canCommaPop() }? -> type(STRING);
PARAM_S_STRING_LITERAL_END2 : ~[#};]+ { canCmdPop()  && !canCommaPop() }? -> type(STRING);

// --------------------------------------------------------
// ---[ RAW STRING ]---------------------------------------

mode IN_PARAM_M;

PARAM_M_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?   { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_M_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }?  { popNextMode(true); }   -> type(TOKEN_PAREN_RIGHT);
PARAM_M_POP_END         : END               { canCmdPop() }?   { popNextMode(true); }   -> type(END);
PARAM_M_POP_COMMA       : TOKEN_COMMA       { canCommaPop() }? { popNextMode(); }       -> type(TOKEN_COMMA);
PARAM_M_STRING_LITERAL1 : ~[),]+     { canFuncPop() && canCommaPop()  }?    -> type(STRING);
PARAM_M_STRING_LITERAL2 : ~[};,]+    { canCmdPop()  && canCommaPop()  }?    -> type(STRING);
PARAM_M_STRING_LITERAL_END1 : ~[)]+  { canFuncPop() && !canCommaPop() }?    -> type(STRING);
PARAM_M_STRING_LITERAL_END2 : ~[};]+ { canCmdPop()  && !canCommaPop() }?    -> type(STRING);

// --------------------------------------------------------
// ---[ TRANSLATABLE STRING ]------------------------------

mode IN_PARAM_U;

PARAM_U_POP_BRACE_RIGHT   : TOKEN_BRACE_RIGHT { shouldCmdPop() }?       { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_U_POP_PAREN_RIGHT   : TOKEN_PAREN_RIGHT { shouldFuncPop() }?      { popNextMode(true); }  -> type(TOKEN_PAREN_RIGHT);
PARAM_U_POP_END           : END               { canCmdPop() }?          { popNextMode(true); }  -> type(END);
PARAM_U_POP_COMMA         : TOKEN_COMMA       { canCommaPop() }?        { popNextMode(); }      -> type(TOKEN_COMMA);
PARAM_U_TOKEN_PAREN_LEFT  : TOKEN_PAREN_LEFT  { canFuncPop() }?                     { incParen(); } -> type(STRING);
PARAM_U_TOKEN_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() && !shouldFuncPop() }? { decParen(); } -> type(STRING);
PARAM_U_TOKEN_BRACE_LEFT  : TOKEN_BRACE_LEFT  { canCmdPop() }?                      { incParen(); } -> type(STRING);
PARAM_U_TOKEN_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() && !shouldCmdPop() }?   { decParen(); } -> type(STRING);
PARAM_U_TOKEN_COMMA       : TOKEN_COMMA       { !canCommaPop() }?           -> type(STRING);
PARAM_U_STRING_LITERAL1   : ~[(),]+   { canFuncPop() && canCommaPop()  }?   -> type(STRING);
PARAM_U_STRING_LITERAL2   : ~[{};,]+  { canCmdPop()  && canCommaPop()  }?   -> type(STRING);
PARAM_U_STRING_LITERAL_END1 : ~[()]+  { canFuncPop() && !canCommaPop() }?   -> type(STRING);
PARAM_U_STRING_LITERAL_END2 : ~[{};]+ { canCmdPop()  && !canCommaPop() }?   -> type(STRING);

// --------------------------------------------------------
// ---[ VARIABLE LENGTH STRING LIST ]----------------------

mode IN_PARAM_K;

PARAM_K_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_K_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_K_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_K_COMMA           : TOKEN_COMMA   { emitIdentifierAfter(GS1Lexer::STRING, getText()); } -> type(TOKEN_COMMA);
PARAM_K_MC_ESCAPE       : MC_ESCAPE                           { setText("#"); }      -> type(STRING);
PARAM_K_MC_NOINDEX      : MC_NOINDEX                              -> type(MESSAGECODE);
PARAM_K_MC_SIMPLE       : MC_SIMPLE     { pushCommand("(P)"); }   -> type(MESSAGECODE);
PARAM_K_MC_COMPUTED_S   : MC_COMPUTED_S { pushCommand("(V)"); }   -> type(MESSAGECODE);
PARAM_K_MC_COMPUTED_V   : MC_COMPUTED_V { pushCommand("(R)"); }   -> type(MESSAGECODE);
PARAM_K_MC_I            : MC_I          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
PARAM_K_MC_T            : MC_T          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_K_MC_E            : MC_E          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_K_MC_U            : MC_U          { pushCommand("(U)"); }   -> type(MESSAGECODE);
PARAM_K_MC_U2           : MC_U2         { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_K_MC_e            : MC_e          { pushCommand("(RRS)"); } -> type(MESSAGECODE);
PARAM_K_MC_i            : MC_i          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
PARAM_K_MC_R            : MC_R          { pushCommand("(K)"); }   -> type(MESSAGECODE);
PARAM_K_MC_Q            : MC_Q          { pushCommand("(SS)"); }  -> type(MESSAGECODE);
PARAM_K_STRING_ESCAPE   : '##' -> type(STRING);
PARAM_K_STRING_LITERAL1  : ~[#),]+  { canFuncPop() }? -> type(STRING);
PARAM_K_STRING_LITERAL2  : ~[#};,]+ { canCmdPop() }?  -> type(STRING);

// --------------------------------------------------------
// ---[ MESSAGE CODE ]-------------------------------------

mode IN_PARAM_X;

PARAM_X_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_X_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_X_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_X_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_X_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_X_MC_ESCAPE       : MC_ESCAPE                           { setText("#"); }      -> type(IDENTIFIER);
PARAM_X_MC_NOINDEX      : MC_NOINDEX                              -> type(MESSAGECODE);
PARAM_X_MC_SIMPLE       : MC_SIMPLE     { pushCommand("(P)"); }   -> type(MESSAGECODE);
PARAM_X_MC_COMPUTED_S   : MC_COMPUTED_S { pushCommand("(V)"); }   -> type(MESSAGECODE);
PARAM_X_MC_COMPUTED_V   : MC_COMPUTED_V { pushCommand("(R)"); }   -> type(MESSAGECODE);
PARAM_X_MC_I            : MC_I          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
PARAM_X_MC_T            : MC_T          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_X_MC_E            : MC_E          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_X_MC_U            : MC_U          { pushCommand("(U)"); }   -> type(MESSAGECODE);
PARAM_X_MC_U2           : MC_U2         { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_X_MC_e            : MC_e          { pushCommand("(RRS)"); } -> type(MESSAGECODE);
PARAM_X_MC_i            : MC_i          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
PARAM_X_MC_R            : MC_R          { pushCommand("(K)"); }   -> type(MESSAGECODE);
PARAM_X_MC_Q            : MC_Q          { pushCommand("(SS)"); }  -> type(MESSAGECODE);
PARAM_X_FUNCTION        : FUNCTION      { pushCommand(getPrototype(registeredFunctions, getText())); } -> type(FUNCTION);
PARAM_X_LITERAL         : LITERAL       -> type(LITERAL);
PARAM_X_IDENTIFIER      : IDENTIFIER    -> type(IDENTIFIER);
PARAM_X_OP_ADD          : OP_ADD        -> type(OP_ADD);
PARAM_X_OP_SUB          : OP_SUB        -> type(OP_SUB);
PARAM_X_OP_MUL          : OP_MUL        -> type(OP_MUL);
PARAM_X_OP_DIV          : OP_DIV        -> type(OP_DIV);
PARAM_X_OP_MOD          : OP_MOD        -> type(OP_MOD);
PARAM_X_OP_POW          : OP_POW        -> type(OP_POW);
PARAM_X_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT { pushArrayAccess(); } -> type(TOKEN_BRACKET_LEFT);
PARAM_X_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT  -> type(TOKEN_PAREN_LEFT);
PARAM_X_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);

// --------------------------------------------------------
// ---[ BADDY NAME ]---------------------------------------

mode IN_PARAM_B;

PARAM_B_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_B_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_B_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_B_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_B_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_B_BADDY           : BADDY -> type(BADDY);

// --------------------------------------------------------
// ---[ ITEM NAME ]----------------------------------------

mode IN_PARAM_I;

PARAM_I_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_I_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_I_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_I_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_I_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_I_ITEM            : ITEMNAMES -> type(ITEM);

// --------------------------------------------------------
// ---[ COLOR ]--------------------------------------------

mode IN_PARAM_C;

PARAM_C_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_C_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_C_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_C_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_C_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_C_COLOR           : COLORS -> type(COLOR);

// --------------------------------------------------------
// ---[ CARRY ITEM ]---------------------------------------

mode IN_PARAM_L;

PARAM_L_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_L_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_L_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_L_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_L_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_L_CARRY           : CARRYNAMES -> type(CARRY);

// --------------------------------------------------------
// ---[ DIRECTION ]----------------------------------------

mode IN_PARAM_D;

PARAM_D_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_D_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); emitIdentifierBefore(GS1Lexer::END, getText()); } -> type(TOKEN_BRACE_RIGHT);
PARAM_D_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_D_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_D_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_D_FUNCTION        : FUNCTION      { pushCommand(getPrototype(registeredFunctions, getText())); } -> type(FUNCTION);
PARAM_D_DIR             : DIR           -> type(DIRECTION);
PARAM_D_LITERAL         : LITERAL       -> type(LITERAL);
PARAM_D_IDENTIFIER      : IDENTIFIER    -> type(IDENTIFIER);
PARAM_D_OP_ASSIGN           : OP_ASSIGN -> type(OP_ASSIGN);
PARAM_D_OP_ADD              : OP_ADD    -> type(OP_ADD);
PARAM_D_OP_SUB              : OP_SUB    -> type(OP_SUB);
PARAM_D_OP_MUL              : OP_MUL    -> type(OP_MUL);
PARAM_D_OP_DIV              : OP_DIV    -> type(OP_DIV);
PARAM_D_OP_MOD              : OP_MOD    -> type(OP_MOD);
PARAM_D_OP_POW              : OP_POW    -> type(OP_POW);
PARAM_D_OP_EQUAL            : OP_EQUAL  -> type(OP_EQUAL);
PARAM_D_OP_NOTEQ            : OP_NOTEQ  -> type(OP_NOTEQ);
PARAM_D_OP_LESS             : OP_LESS   -> type(OP_LESS);
PARAM_D_OP_GREAT            : OP_GREAT  -> type(OP_GREAT);
PARAM_D_OP_LESS_EQ          : OP_LESS_EQ  -> type(OP_LESS_EQ);
PARAM_D_OP_GREAT_EQ         : OP_GREAT_EQ -> type(OP_GREAT_EQ);
PARAM_D_OP_IN               : OP_IN     -> type(OP_IN);
PARAM_D_OP_INC              : OP_INC    -> type(OP_INC);
PARAM_D_OP_DEC              : OP_DEC    -> type(OP_DEC);
PARAM_D_OP_LOGICALAND       : OP_LOGICALAND -> type(OP_LOGICALAND);
PARAM_D_OP_LOGICALOR        : OP_LOGICALOR  -> type(OP_LOGICALOR);
PARAM_D_OP_LOGICALNOT       : OP_LOGICALNOT -> type(OP_LOGICALNOT);
PARAM_D_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT { pushArrayAccess(); } -> type(TOKEN_BRACKET_LEFT);
PARAM_D_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT  -> type(TOKEN_PAREN_LEFT);
PARAM_D_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
PARAM_D_TOKEN_PIPE          : TOKEN_PIPE        -> type(TOKEN_PIPE);
PARAM_D_TOKEN_QUESTION      : TOKEN_QUESTION    -> type(TOKEN_QUESTION);
PARAM_D_TOKEN_COLON         : TOKEN_COLON       -> type(TOKEN_COLON);
PARAM_D_TOKEN_PERIOD        : TOKEN_PERIOD      -> type(TOKEN_PERIOD);

// --------------------------------------------------------
// ---[ EMBEDDED CODE ]------------------------------------

mode IN_PARAM_Z;

PARAM_Z_START           : TOKEN_BRACE_LEFT  { shouldCmdPop()  }? { incBrace(); }    -> channel(HIDDEN);
PARAM_Z_POP_END         : END               { shouldCmdPop()  }? { popNextMode(); } -> type(END);
PARAM_Z_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { !shouldCmdPop() }? { decBrace(); popNextMode(); emitIdentifierBefore(GS1Lexer::END, getText()); } -> channel(HIDDEN);
PARAM_Z_BRACE_LEFT      : TOKEN_BRACE_LEFT                       { incBrace(); }    -> type(STRING);
PARAM_Z_BRACE_RIGHT     : TOKEN_BRACE_RIGHT                      { decBrace(); }    -> type(STRING);
PARAM_Z_END             : END               { !shouldCmdPop() }?                    -> type(STRING);
PARAM_Z_STRING          : ~[{};]+ -> type(STRING);

// --------------------------------------------------------
// ---[ FUNCTION OPEN ]------------------------------------

mode IN_PARAM_1;

PARAM_1_WS                : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_1_TOKEN_PAREN_LEFT  : TOKEN_PAREN_LEFT { m_commandStates.back().popMode = POPMODE_FUNCTION; m_commandStates.back().parenCount = 0; popNextMode(); } -> type(TOKEN_PAREN_LEFT);

// --------------------------------------------------------
// ---[ FUNCTION CLOSE ]-----------------------------------

mode IN_PARAM_2;

PARAM_2_WS                : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_2_TOKEN_PAREN_RIGHT : TOKEN_PAREN_RIGHT { popNextMode(); } -> type(TOKEN_PAREN_RIGHT);

// --------------------------------------------------------
// ---[ FUNCTION OPEN WITH OPTIONAL PARAM ]----------------

mode IN_PARAM_3;

PARAM_3_WS                : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_3_TOKEN_PAREN_LEFT  : TOKEN_PAREN_LEFT { m_commandStates.back().popMode = POPMODE_FUNCTION; m_commandStates.back().parenCount = 0; checkIfNextModeOptional(); } -> type(TOKEN_PAREN_LEFT);
