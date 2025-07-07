lexer grammar GS1Lexer;

@lexer::header
{
// --------------------------------------------------------
#include <array>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
// --------------------------------------------------------
}

@lexer::context
{
// --------------------------------------------------------
constexpr std::array<std::string_view, 200> builtInCommands = {
    "addguildmember",
    "addstring",
    "addtiledef",
    "addtiledef2",
    "addweapon",
    "attachplayertoobj",
    "blockagain",
    "blockagainlocal",
    "callnpc",
    "callweapon",
    "canbecarried",
    "canbepulled",
    "canbepushed",
    "cannotbecarried",
    "cannotbepulled",
    "cannotbepushed",
    "cannotwarp",
    "canwarp",
    "canwarp2",
    "carryobject",
    "changeimgcolors",
    "changeimgmode",
    "changeimgpart",
    "changeimgvis",
    "changeimgzoom",
    "copyflags",
    "copylevel",
    "copystrings",
    "deletelevel",
    "deletestring",
    "destroy",
    "detachplayer",
    "disabledefmovement",
    "disablemap",
    "disablepause",
    "disableselectweapons",
    "disableweapons",
    "dontblock",
    "dontblocklocal",
    "drawaslight",
    "drawoverplayer",
	"drawovertrees",
    "drawunderplayer",
    "enabledefmovement",
    "enablefeatures",
    "enablemap",
    "enablepause",
    "enableselectweapons",
    "enableweapons",
    "explodebomb",
    "followplayer",
    "freezeplayer",
    "freezeplayer2",
    "hide",
    "hideimg",
    "hideimgs",
    "hidelocal",
    "hideplayer",
    "hidesword",
    "hitcompu",
    "hitnpc",
    "hitobjects",
    "hitplayer",
    "hurt ",
    "insertstring",
    "join ",
    "lay ",
    "lay2",
    "loadmap",
    "message",
    "move",
    "noplayerkilling",
    "noplayeronwall",
    "openurl",
    "openurl2",
    "play ",
    "play2",
    "playlooped",
    "putbomb",
    "putcomp",
    "putexplosion ",
    "putexplosion2",
    "puthorse",
    "putleaps",
    "putnewcomp",
    "putnpc",
    "putnpc2",
    "putobject",
    "reflectarrow",
    "removearrow",
    "removebomb",
    "removecompus",
    "removeexplo",
    "removeguild",
    "removeguildmember",
    "removehorse",
    "removeitem",
    "removestring",
    "removetiledefs",
    "removeweapon",
    "replaceani",
    "replacestring",
    "resetfocus",
    "saveinfo",
    "savelog ",
    "savelog2",
    "say ",
    "say2",
    "sendpm",
    "sendrpgmessage",
    "sendtonc",
    "sendtorc",
    "serverwarp",
    "set ",
    "setani",
    "setarray",
    "setbackpal",
    "setbeltcolor",
    "setbody",
    "setcharani",
    "setchargender",
    "setcharprop",
    "setcoatcolor",
    "setcoloreffect",
    "setcursor ",
    "setcursor2",
    "seteffect",
    "seteffectmode ",
    "setfocus",
    "setgender",
    "setgif ",
    "sethead",
    "setimg",
    "setimgpart",
    "setletters",
    "setlevel ",
    "setlevel2",
    "setmap",
    "setminimap",
    "setmusicvolume",
    "setplayerdir",
    "setplayerprop",
    "setpm",
    "setshape",
    "setshape2",
    "setshield",
    "setshoecolor",
    "setshootparams",
    "setskincolor",
    "setsleevecolor",
    "setspritesimage",
    "setstatusimage",
    "setstring",
    "setsword",
    "seturllevel",
    "setz",
    "setzoomeffect",
    "shoot",
    "shootarrow",
    "shootball",
    "shootfireball",
    "shootfireblast",
    "shootnuke",
    "show",
    "showani",
    "showani2",
    "showcharacter",
    "showfile",
    "showimg",
    "showimg2",
    "showlocal",
    "showpoly",
    "showpoly2",
    "showstats",
    "showtext",
    "showtext2",
    "sleep",
    "spyfire",
    "stopmidi",
    "stopsound",
    "take ",
    "take2",
    "takehorse",
    "takeplayercarry",
    "takeplayerhorse",
    "throwcarry",
    "timereverywhere",
    "timershow",
    "toinventory",
    "tokenize",
    "tokenize2",
    "toweapons",
    "triggeraction",
    "unfreezeplayer",
    "unset ",
    "updateboard ",
    "updateboard2 ",
    "updateterrain",
    "wraptext",
    "wraptext2",
};

constexpr bool isBuiltInCommand(std::string_view name)
{
	for (const auto& builtIn : builtInCommands)
	{
		if (name.starts_with(builtIn))
			return true;
	}
	return false;
}
// --------------------------------------------------------
}

@lexer::members
{
// --------------------------------------------------------
// Allow injected tokens.
virtual std::unique_ptr<antlr4::Token> nextToken() override
{
	if (m_pickQueuedTokens && !m_pendingTokens.empty())
	{
		auto token = std::move(m_pendingTokens.front());
		m_pendingTokens.pop_front();
		return token;
	}

	if (m_pickQueuedTokens)
		m_pickQueuedTokens = false;

	auto next = antlr4::Lexer::nextToken();
#if DEBUG
	auto text = next->getText();
#endif
	if (!m_pendingTokens.empty())
		m_pickQueuedTokens = true;

	return next;
}

void emitIdentifier(size_t type, std::string_view name)
{
	m_pendingTokens.emplace_back(_factory->create(type, ""));
}

int breakpoint()
{
	return _input->index();
}

enum POPMODE
{
	POPMODE_COMMAND,
	POPMODE_FUNCTION
};

struct CommandState
{
	std::string_view arguments;
	POPMODE popMode;
	bool commaPop = true;
};

bool canFuncPop() const
{
	return !m_commandStates.empty() && m_commandStates.back().popMode == POPMODE_FUNCTION;
}

bool canCmdPop() const
{
	return m_commandStates.empty() || m_commandStates.back().popMode == POPMODE_COMMAND;
}

bool canCommaPop() const
{
	return m_commandStates.empty() || m_commandStates.back().commaPop;
}

bool isNextArgLeftParen() const
{
	return !m_commandStates.empty() && !m_commandStates.back().arguments.empty() && m_commandStates.back().arguments.front() == '(';
}

bool isNotDefaultMode() const
{
	return !m_commandStates.empty();
}

void pushCommand(std::string_view arguments)
{
	m_commandStates.emplace_back(CommandState{ arguments, POPMODE_COMMAND, true });
	pushMode(IN_PARAM_1);	// Just a dummy state that gets immediately cleared.
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
		if (mode == 'S' && (currentState.arguments.empty() || currentState.arguments.front() == '('))
			currentState.commaPop = false;

		switch (mode)
		{
			case 'V': setMode(IN_PARAM_V); emitIdentifier(GS1Lexer::IDENTIFIER, getText()); break;
			case 'E': setMode(IN_PARAM_E); break;
			case 'P': setMode(IN_PARAM_E); currentState.commaPop = false; break;
			case 'S': setMode(IN_PARAM_S); emitIdentifier(GS1Lexer::STRING, getText()); break;
			case 'L': setMode(IN_PARAM_L); emitIdentifier(GS1Lexer::STRING, getText()); break;
			case 'M': setMode(IN_PARAM_M); break;
			case 'B': setMode(IN_PARAM_B); break;
			case 'I': setMode(IN_PARAM_I); break;
			case 'C': setMode(IN_PARAM_C); break;
			case 'G': setMode(IN_PARAM_G); break;
			case 'U': setMode(IN_PARAM_U); break;
			case 'D': setMode(IN_PARAM_D); break;
			case 'Z': setMode(IN_PARAM_Z); break;
			case 'X': setMode(IN_PARAM_X); break;
			case '(': setMode(IN_PARAM_1); break;
			case ')': setMode(IN_PARAM_2); break;
			default: setMode(DEFAULT_MODE); break;
		}
	}
}

size_t m_bracketCount = 0;
std::deque<CommandState> m_commandStates;

bool m_pickQueuedTokens = false;
std::deque<std::unique_ptr<antlr4::Token>> m_pendingTokens{};
// --------------------------------------------------------
}

tokens { COMMAND, FUNCTION, MESSAGECODE, STRING, BADDY, ITEM, COLOR, GENDER, CARRY, DIRECTION }

/*
	Mode parameter argument guide:
	- V  variable (number/array/string)
	- E  expression (variable + math)
	- P  parameters (multiple expressions)
	- S  string
	- L  variable length comma-separated string list
	- M  message code
	- B  baddy name
	- I  item name
	- C  color name
	- G  gender name
	- U  carry item name
	- D  direction name or number
	- Z  code (putnpc2 special case)
	- X  storage special case
	- (  left parenthesis
	- )  right parenthesis
*/

CMD_SETSTRING            : 'setstring'            { pushCommand("VS"); } -> type(COMMAND);
CMD_ADDSTRING            : 'addstring'            { pushCommand("VS"); } -> type(COMMAND);
CMD_INSERTSTRING         : 'insertstring'         { pushCommand("VES"); } -> type(COMMAND);
CMD_REPLACESTRING        : 'replacestring'        { pushCommand("VES"); } -> type(COMMAND);
CMD_REMOVESTRING         : 'removestring'         { pushCommand("VS"); } -> type(COMMAND);
CMD_DELETESTRING         : 'deletestring'         { pushCommand("VE"); } -> type(COMMAND);
CMD_SET                  : 'set '                 { pushCommand("V"); } -> type(COMMAND);
CMD_UNSET                : 'unset '               { pushCommand("V"); } -> type(COMMAND);
CMD_SLEEP                : 'sleep'                { pushCommand("E"); } -> type(COMMAND);
CMD_SETARRAY             : 'setarray'             { pushCommand("VE"); } -> type(COMMAND);
CMD_TIMEREVERYWHERE      : 'timereverywhere'      -> type(COMMAND);
CMD_SETGIF               : 'setgif '              { pushCommand("S"); } -> type(COMMAND);
CMD_SETIMG               : 'setimg'               { pushCommand("S"); } -> type(COMMAND);
CMD_SETIMGPART           : 'setimgpart'           { pushCommand("SEEEE"); } -> type(COMMAND);
CMD_HIDE                 : 'hide'                 -> type(COMMAND);
CMD_SHOW                 : 'show'                 -> type(COMMAND);
CMD_DONTBLOCK            : 'dontblock'            -> type(COMMAND);
CMD_BLOCKAGAIN           : 'blockagain'           -> type(COMMAND);
CMD_DRAWOVERPLAYER       : 'drawoverplayer'       -> type(COMMAND);
CMD_DRAWOVERTREES        : 'drawovertrees'        -> type(COMMAND);
CMD_DRAWUNDERPLAYER      : 'drawunderplayer'      -> type(COMMAND);
CMD_DRAWASLIGHT          : 'drawaslight'          -> type(COMMAND);
CMD_SETEFFECTMODE        : 'seteffectmode '       { pushCommand("EEEE"); } -> type(COMMAND);
CMD_CANBECARRIED         : 'canbecarried'         -> type(COMMAND);
CMD_CANNOTBECARRIED      : 'cannotbecarried'      -> type(COMMAND);
CMD_CANBEPUSHED          : 'canbepushed'          -> type(COMMAND);
CMD_CANNOTBEPUSHED       : 'cannotbepushed'       -> type(COMMAND);
CMD_CANBEPULLED          : 'canbepulled'          -> type(COMMAND);
CMD_CANNOTBEPULLED       : 'cannotbepulled'       -> type(COMMAND);
CMD_MOVE                 : 'move'                 { pushCommand("EEEE"); } -> type(COMMAND);
CMD_SAY                  : 'say '                 { pushCommand("E"); } -> type(COMMAND);
CMD_SAY2                 : 'say2'                 { pushCommand("S"); } -> type(COMMAND);
CMD_LAY                  : 'lay '                 { pushCommand("I"); } -> type(COMMAND);
CMD_LAY2                 : 'lay2'                 { pushCommand("IEE"); } -> type(COMMAND);
CMD_TAKE                 : 'take '                { pushCommand("I"); } -> type(COMMAND);
CMD_TAKE2                : 'take2'                { pushCommand("E"); } -> type(COMMAND);
CMD_MESSAGE              : 'message'              { pushCommand("S"); } -> type(COMMAND);
CMD_TIMERSHOW            : 'timershow'            -> type(COMMAND);
CMD_SHOWCHARACTER        : 'showcharacter'        -> type(COMMAND);
CMD_SETCHARPROP          : 'setcharprop'          { pushCommand("MS"); } -> type(COMMAND);
CMD_SETCHARANI           : 'setcharani'           { pushCommand("S"); } -> type(COMMAND);
CMD_SETCHARGENDER        : 'setchargender'        { pushCommand("G"); } -> type(COMMAND);
CMD_TRIGGERACTION        : 'triggeraction'        { pushCommand("EEL"); } -> type(COMMAND);
CMD_PUTNPC               : 'putnpc'               { pushCommand("SSEE"); } -> type(COMMAND);
CMD_PUTNPC2              : 'putnpc2'              { pushCommand("EEZ"); } -> type(COMMAND);
CMD_CALLNPC              : 'callnpc'              { pushCommand("ES"); } -> type(COMMAND);
CMD_CALLWEAPON           : 'callweapon'           { pushCommand("ESS"); } -> type(COMMAND);
CMD_DESTROY              : 'destroy'              -> type(COMMAND);
CMD_CARRYOBJECT          : 'carryobject'          { pushCommand("U"); } -> type(COMMAND);
CMD_THROWCARRY           : 'throwcarry'           -> type(COMMAND);
CMD_FOLLOWPLAYER         : 'followplayer'         -> type(COMMAND);
CMD_TOINVENTORY          : 'toinventory'          { pushCommand("S"); } -> type(COMMAND);
CMD_TOWEAPONS            : 'toweapons'            { pushCommand("S"); } -> type(COMMAND);
CMD_SETCOLOREFFECT       : 'setcoloreffect'       { pushCommand("EEEE"); } -> type(COMMAND);
CMD_SETZOOMEFFECT        : 'setzoomeffect'        { pushCommand("E"); } -> type(COMMAND);
CMD_SHOWIMG              : 'showimg'              { pushCommand("ESEE"); } -> type(COMMAND);
CMD_SHOWIMG2             : 'showimg2'             { pushCommand("ESEEE"); } -> type(COMMAND);
CMD_SHOWANI              : 'showani'              { pushCommand("EEEDSS"); } -> type(COMMAND);
CMD_SHOWANI2             : 'showani2'             { pushCommand("EEEEDSS"); } -> type(COMMAND);
CMD_SHOWPOLY             : 'showpoly'             { pushCommand("EE"); } -> type(COMMAND);
CMD_SHOWPOLY2            : 'showpoly2'            { pushCommand("EE"); } -> type(COMMAND);
CMD_SHOWTEXT             : 'showtext'             { pushCommand("EEESSS"); } -> type(COMMAND);
CMD_SHOWTEXT2            : 'showtext2'            { pushCommand("EEEESSS"); } -> type(COMMAND);
CMD_HIDEIMG              : 'hideimg'              { pushCommand("E"); } -> type(COMMAND);
CMD_HIDEIMGS             : 'hideimgs'             { pushCommand("EE"); } -> type(COMMAND);
CMD_CHANGEIMGPART        : 'changeimgpart'        { pushCommand("EEEEE"); } -> type(COMMAND);
CMD_CHANGEIMGVIS         : 'changeimgvis'         { pushCommand("EE"); } -> type(COMMAND);
CMD_CHANGEIMGCOLORS      : 'changeimgcolors'      { pushCommand("EEEEE"); } -> type(COMMAND);
CMD_CHANGEIMGZOOM        : 'changeimgzoom'        { pushCommand("EE"); } -> type(COMMAND);
CMD_CHANGEIMGMODE        : 'changeimgmode'        { pushCommand("EE"); } -> type(COMMAND);
CMD_SHOOTARROW           : 'shootarrow'           { pushCommand("D"); } -> type(COMMAND);
CMD_SHOOTFIREBALL        : 'shootfireball'        { pushCommand("D"); } -> type(COMMAND);
CMD_SHOOTFIREBLAST       : 'shootfireblast'       { pushCommand("D"); } -> type(COMMAND);
CMD_SHOOTNUKE            : 'shootnuke'            { pushCommand("D"); } -> type(COMMAND);
CMD_SHOOTBALL            : 'shootball'            -> type(COMMAND);
CMD_SPYFIRE              : 'spyfire'              { pushCommand("EE"); } -> type(COMMAND);
CMD_HITPLAYER            : 'hitplayer'            { pushCommand("EEEE"); } -> type(COMMAND);
CMD_HITNPC               : 'hitnpc'               { pushCommand("EEEE"); } -> type(COMMAND);
CMD_HITOBJECTS           : 'hitobjects'           { pushCommand("EEE"); } -> type(COMMAND);
CMD_HIDELOCAL            : 'hidelocal'            -> type(COMMAND);
CMD_SHOWLOCAL            : 'showlocal'            -> type(COMMAND);
CMD_DONTBLOCKLOCAL       : 'dontblocklocal'       -> type(COMMAND);
CMD_BLOCKAGAINLOCAL      : 'blockagainlocal'      -> type(COMMAND);
CMD_TAKEHORSE            : 'takehorse'            -> type(COMMAND);
CMD_TOKENIZE             : 'tokenize'             { pushCommand("S"); } -> type(COMMAND);
CMD_TOKENIZE2            : 'tokenize2'            { pushCommand("SS"); } -> type(COMMAND);
CMD_SETSHAPE             : 'setshape'             { pushCommand("EEE"); } -> type(COMMAND);
CMD_SETSHAPE2            : 'setshape2'            { pushCommand("EEE"); } -> type(COMMAND);
CMD_WRAPTEXT             : 'wraptext'             { pushCommand("ESS"); } -> type(COMMAND);
CMD_WRAPTEXT2            : 'wraptext2'            { pushCommand("EESS"); } -> type(COMMAND);
CMD_SETSHOOTPARAMS       : 'setshootparams'       { pushCommand("L"); } -> type(COMMAND);
CMD_SHOOT                : 'shoot'                { pushCommand("EEEEESS"); } -> type(COMMAND);
CMD_SETLEVEL             : 'setlevel '            { pushCommand("S"); } -> type(COMMAND);
CMD_SETLEVEL2            : 'setlevel2'            { pushCommand("SEE"); } -> type(COMMAND);
CMD_SETURLLEVEL          : 'seturllevel'          { pushCommand("S"); } -> type(COMMAND);
CMD_SETBODY              : 'setbody'              { pushCommand("S"); } -> type(COMMAND);
CMD_SETHEAD              : 'sethead'              { pushCommand("S"); } -> type(COMMAND);
CMD_SETSWORD             : 'setsword'             { pushCommand("SE"); } -> type(COMMAND);
CMD_SETSHIELD            : 'setshield'            { pushCommand("SE"); } -> type(COMMAND);
CMD_SETANI               : 'setani'               { pushCommand("SS"); } -> type(COMMAND);
CMD_SETPLAYERDIR         : 'setplayerdir'         { pushCommand("D"); } -> type(COMMAND);
CMD_SETGENDER            : 'setgender'            { pushCommand("G"); } -> type(COMMAND);
CMD_SETSKINCOLOR         : 'setskincolor'         { pushCommand("C"); } -> type(COMMAND);
CMD_SETCOATCOLOR         : 'setcoatcolor'         { pushCommand("C"); } -> type(COMMAND);
CMD_SETSLEEVECOLOR       : 'setsleevecolor'       { pushCommand("C"); } -> type(COMMAND);
CMD_SETSHOECOLOR         : 'setshoecolor'         { pushCommand("C"); } -> type(COMMAND);
CMD_SETBELTCOLOR         : 'setbeltcolor'         { pushCommand("C"); } -> type(COMMAND);
CMD_SETPLAYERPROP        : 'setplayerprop'        { pushCommand("MS"); } -> type(COMMAND);
CMD_TAKEPLAYERCARRY      : 'takeplayercarry'      -> type(COMMAND);
CMD_TAKEPLAYERHORSE      : 'takeplayerhorse'      -> type(COMMAND);
CMD_DISABLEWEAPONS       : 'disableweapons'       -> type(COMMAND);
CMD_ENABLEWEAPONS        : 'enableweapons'        -> type(COMMAND);
CMD_FREEZEPLAYER         : 'freezeplayer '        { pushCommand("E"); } -> type(COMMAND);
CMD_FREEZEPLAYER2        : 'freezeplayer2'        -> type(COMMAND);
CMD_UNFREEZEPLAYER       : 'unfreezeplayer'       -> type(COMMAND);
CMD_HIDEPLAYER           : 'hideplayer'           { pushCommand("E"); } -> type(COMMAND);
CMD_HIDESWORD            : 'hidesword'            { pushCommand("E"); } -> type(COMMAND);
CMD_HURT                 : 'hurt '                { pushCommand("E"); } -> type(COMMAND);
CMD_DISABLEDEFMOVEMENT   : 'disabledefmovement'   -> type(COMMAND);
CMD_ENABLEDEFMOVEMENT    : 'enabledefmovement'    -> type(COMMAND);
CMD_DISABLESELECTWEAPONS : 'disableselectweapons' -> type(COMMAND);
CMD_ENABLESELECTWEAPONS  : 'enableselectweapons'  -> type(COMMAND);
CMD_DISABLEPAUSE         : 'disablepause'         -> type(COMMAND);
CMD_ENABLEPAUSE          : 'enablepause'          -> type(COMMAND);
CMD_DISABLEMAP           : 'disablemap'           -> type(COMMAND);
CMD_ENABLEMAP            : 'enablemap'            -> type(COMMAND);
CMD_ENABLEFEATURES       : 'enablefeatures'       { pushCommand("E"); } -> type(COMMAND);
CMD_REPLACEANI           : 'replaceani'           { pushCommand("SS"); } -> type(COMMAND);
CMD_ATTACHPLAYERTOOBJ    : 'attachplayertoobj'    { pushCommand("EE"); } -> type(COMMAND);
CMD_DETACHPLAYER         : 'detachplayer'         -> type(COMMAND);
CMD_UPDATEBOARD          : 'updateboard'          { pushCommand("EEEE"); } -> type(COMMAND);
CMD_UPDATEBOARD2         : 'updateboard2'         { pushCommand("EEEE"); } -> type(COMMAND);
CMD_PUTOBJECT            : 'putobject'            { pushCommand("SEE"); } -> type(COMMAND);
CMD_PUTBOMB              : 'putbomb'              { pushCommand("EEE"); } -> type(COMMAND);
CMD_PUTEXPLOSION         : 'putexplosion'         { pushCommand("EEE"); } -> type(COMMAND);
CMD_PUTEXPLOSION2        : 'putexplosion2'        { pushCommand("EEEE"); } -> type(COMMAND);
CMD_PUTLEAPS             : 'putleaps'             { pushCommand("EEE"); } -> type(COMMAND);
CMD_PUTHORSE             : 'puthorse'             { pushCommand("SEE"); } -> type(COMMAND);
CMD_SETBACKPAL           : 'setbackpal'           { pushCommand("S"); } -> type(COMMAND);
CMD_SETLETTERS           : 'setletters'           { pushCommand("S"); } -> type(COMMAND);
CMD_SETMAP               : 'setmap'               { pushCommand("SSEE"); } -> type(COMMAND);
CMD_SETMINIMAP           : 'setminimap'           { pushCommand("SSEE"); } -> type(COMMAND);
CMD_SETEFFECT            : 'seteffect'            { pushCommand("EEEE"); } -> type(COMMAND);
CMD_SETFOCUS             : 'setfocus'             { pushCommand("EE"); } -> type(COMMAND);
CMD_RESETFOCUS           : 'resetfocus'           -> type(COMMAND);
CMD_NOPLAYERKILLING      : 'noplayerkilling'      -> type(COMMAND);
CMD_NOPLAYERONWALL       : 'noplayeronwall'       -> type(COMMAND);
CMD_REMOVEBOMB           : 'removebomb'           { pushCommand("E"); } -> type(COMMAND);
CMD_REMOVEARROW          : 'removearrow'          { pushCommand("E"); } -> type(COMMAND);
CMD_REMOVEITEM           : 'removeitem'           { pushCommand("E"); } -> type(COMMAND);
CMD_REMOVEEXPLO          : 'removeexplo'          { pushCommand("E"); } -> type(COMMAND);
CMD_REMOVEHORSE          : 'removehorse'          { pushCommand("E"); } -> type(COMMAND);
CMD_EXPLODEBOMB          : 'explodebomb'          { pushCommand("E"); } -> type(COMMAND);
CMD_REFLECTARROW         : 'reflectarrow'         { pushCommand("E"); } -> type(COMMAND);
CMD_ADDTILEDEF           : 'addtiledef'           { pushCommand("SSE"); } -> type(COMMAND);
CMD_ADDTILEDEF2          : 'addtiledef2'          { pushCommand("SSEE"); } -> type(COMMAND);
CMD_REMOVETILEDEFS       : 'removetiledefs'       { pushCommand("S"); } -> type(COMMAND);
CMD_LOADMAP              : 'loadmap'              { pushCommand("S"); } -> type(COMMAND);
CMD_UPDATETERRAIN        : 'updateterrain'        -> type(COMMAND);
CMD_SHOWSTATS            : 'showstats'            { pushCommand("E"); } -> type(COMMAND);
CMD_PUTCOMP              : 'putcomp'              { pushCommand("BEE"); } -> type(COMMAND);
CMD_PUTNEWCOMP           : 'putnewcomp'           { pushCommand("BEESE"); } -> type(COMMAND);
CMD_HITCOMPU             : 'hitcompu'             { pushCommand("EEEE"); } -> type(COMMAND);
CMD_REMOVECOMPUS         : 'removecompus'         -> type(COMMAND);
CMD_PLAY                 : 'play '                { pushCommand("S"); } -> type(COMMAND);
CMD_PLAY2                : 'play2'                { pushCommand("SEEE"); } -> type(COMMAND);
CMD_PLAYLOOPED           : 'playlooped'           { pushCommand("S"); } -> type(COMMAND);
CMD_STOPSOUND            : 'stopsound'            { pushCommand("S"); } -> type(COMMAND);
CMD_STOPMIDI             : 'stopmidi'             -> type(COMMAND);
CMD_SETMUSICVOLUME       : 'setmusicvolume'       { pushCommand("EE"); } -> type(COMMAND);
CMD_OPENURL              : 'openurl'              { pushCommand("S"); } -> type(COMMAND);
CMD_OPENURL2             : 'openurl2'             { pushCommand("SEE"); } -> type(COMMAND);
CMD_SHOWFILE             : 'showfile'             { pushCommand("S"); } -> type(COMMAND);
CMD_JOIN                 : 'join '                { pushCommand("S"); } -> type(COMMAND);
CMD_SETCURSOR            : 'setcursor '           { pushCommand("E"); } -> type(COMMAND);
CMD_SETCURSOR2           : 'setcursor2'           { pushCommand("S"); } -> type(COMMAND);
CMD_CANWARP              : 'canwarp'              -> type(COMMAND);
CMD_CANWARP2             : 'canwarp2'             -> type(COMMAND);
CMD_CANNOTWARP           : 'cannotwarp'           -> type(COMMAND);
CMD_ADDWEAPON            : 'addweapon'            { pushCommand("S"); } -> type(COMMAND);
CMD_REMOVEWEAPON         : 'removeweapon'         { pushCommand("S"); } -> type(COMMAND);
CMD_SETSPRITESIMAGE      : 'setspritesimage'      { pushCommand("S"); } -> type(COMMAND);
CMD_SETSTATUSIMAGE       : 'setstatusimage'       { pushCommand("S"); } -> type(COMMAND);
CMD_ADDGUILDMEMBER       : 'addguildmember'       { pushCommand("SSS"); } -> type(COMMAND);
CMD_REMOVEGUILDMEMBER    : 'removeguildmember'    { pushCommand("SSS"); } -> type(COMMAND);
CMD_REMOVEGUILD          : 'removeguild'          { pushCommand("S"); } -> type(COMMAND);
CMD_COPYSTRINGS          : 'copystrings'          { pushCommand("SS"); } -> type(COMMAND);
CMD_COPYFLAGS            : 'copyflags'            { pushCommand("SS"); } -> type(COMMAND);
CMD_SENDTORC             : 'sendtorc'             { pushCommand("S"); } -> type(COMMAND);
CMD_SENDTONC             : 'sendtonc'             { pushCommand("S"); } -> type(COMMAND);
CMD_SENDPM               : 'sendpm'               { pushCommand("S"); } -> type(COMMAND);
CMD_SETPM                : 'setpm'                { pushCommand("S"); } -> type(COMMAND);
CMD_SENDRPGMESSAGE       : 'sendrpgmessage'       { pushCommand("S"); } -> type(COMMAND);
CMD_SERVERWARP           : 'serverwarp'           { pushCommand("S"); } -> type(COMMAND);
CMD_SETZ                 : 'setz'                 { pushCommand("EEEEEEEE"); } -> type(COMMAND);
CMD_COPYLEVEL            : 'copylevel'            { pushCommand("SS"); } -> type(COMMAND);
CMD_DELETELEVEL          : 'deletelevel'          { pushCommand("S"); } -> type(COMMAND);
CMD_SAVEINFO             : 'saveinfo'             { pushCommand("SS"); } -> type(COMMAND);
CMD_SAVELOG              : 'savelog '             { pushCommand("S"); } -> type(COMMAND);
CMD_SAVELOG2             : 'savelog2'             { pushCommand("SS"); } -> type(COMMAND);

FUNC_GROUP_1
	: (
		'onwall'
		| 'onwall2'
		| 'onwater'
		| 'groundsheight'
		| 'waterheight'
		| 'keydown'
		| 'keydown2'
		| 'arraylen'
		| 'abs'
		| 'arctan'
		| 'cos'
		| 'sin'
		| 'getangle'
		| 'int'
		| 'random'
		| 'min'
		| 'max'
		| 'log'
		| 'testbomb'
		| 'testcompu'
		| 'testexplo'
		| 'testhorse'
		| 'testitem'
		| 'testnpc'
		| 'testplayer'
		| 'testsign'
		| 'tiletype'
		| 'ascii'
		| 'getz'
		| 'screenx'
		| 'screeny'
		| 'worldx'
		| 'worldy'
		| 'vecx'
		| 'vecy'
		| 'findnearestplayer'
		| 'findnearestplayers'
		| 'getnearestplayer'
		| 'getnearestplayers'
		| 'aindexof'
		| 'getdir'
		| 'getareanpcs'
	) { pushCommand("(P)"); } -> type(FUNCTION)
	;

FUNC_GROUP_2
	: (
		'onmapx'
		| 'onmapy'
		| 'strlen'
		| 'strtofloat'
		| 'imgwidth'
		| 'imgheight'
		| 'keycode'
		| 'base64encode'
		| 'base64decode'
		| 'playersays'
		| 'playersays2'
		| 'hasweapon'
		| '_'
		| 'N_'
		| 'getplayer'
		| 'getnpc'
	) { pushCommand("(S)"); } -> type(FUNCTION)
	;

FUNC_GROUP_3
	: (
		'startswith'
		| 'strcontains'
		| 'strequals'
		| 'indexof'
	) { pushCommand("(SS)"); } -> type(FUNCTION)
	;

FUNC_GROUP_4 : ('textwidth' | 'textheight') { pushCommand("(ESSS)"); } -> type(FUNCTION);
FUNC_GROUP_5 : 'lindexof'  { pushCommand("(SV)"); } -> type(FUNCTION);
FUNC_GROUP_6 : 'sarraylen' { pushCommand("(V)"); }  -> type(FUNCTION);

MC_NOINDEX		: '#' ([angcmWw1235678LFfpbND] | 'C' [01234] | 'P1' DIGITS? | 'P2' DIGITS? | 'P3' '0'? | 'P' [456789]) { _input->LA(1) != '(' }? -> type(MESSAGECODE);
MC_SIMPLE		: '#' ([angcmWw1235678ptKkGNQ] | 'C' [01234] | 'P1' DIGITS? | 'P2' DIGITS? | 'P3' '0'? | 'P' [456789]) { pushCommand("(P)"); } -> type(MESSAGECODE);
MC_COMPUTED_S	: '#s' { pushCommand("(V)"); }   -> type(MESSAGECODE);
MC_COMPUTED_V	: '#v' { pushCommand("(E)"); }   -> type(MESSAGECODE);
MC_I			: '#I' { pushCommand("(VP)"); }  -> type(MESSAGECODE);
MC_T			: '#T' { pushCommand("(S)"); }   -> type(MESSAGECODE);
MC_e			: '#e' { pushCommand("(EES)"); } -> type(MESSAGECODE);
MC_i			: '#i' { pushCommand("(SP)"); }  -> type(MESSAGECODE);
MC_R			: '#R' { pushCommand("(L)"); }   -> type(MESSAGECODE);

// TODO: Some string lists are CSV.

STORAGE_THIS    : 'this.'     { pushCommand("X"); };
STORAGE_THISO   : 'thiso.'    { pushCommand("X"); };
STORAGE_CLIENT  : 'client.'   { pushCommand("X"); };
STORAGE_CLIENTR : 'clientr.'  { pushCommand("X"); };
STORAGE_CLIENTO : 'cliento.'  { pushCommand("X"); };
STORAGE_CLIENTRO: 'clientro.' { pushCommand("X"); };
STORAGE_SERVER  : 'server.'   { pushCommand("X"); };
STORAGE_SERVERR : 'serverr.'  { pushCommand("X"); };
STORAGE_LEVEL   : 'level.'    { pushCommand("X"); };
STORAGE_LOCAL   : 'local.'    { pushCommand("X"); };
STORAGE_TEMP    : 'temp.'     { pushCommand("X"); };

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

TOKEN_BRACKET_LEFT  : '[';
TOKEN_BRACKET_RIGHT : ']';
TOKEN_BRACE_LEFT	: '{';
TOKEN_BRACE_RIGHT	: '}';
TOKEN_PAREN_LEFT	: '(';
TOKEN_PAREN_RIGHT	: ')';
TOKEN_COMMA			: ',';
TOKEN_PIPE			: '|';
TOKEN_QUESTION		: '?';
TOKEN_COLON			: ':';
TOKEN_PERIOD		: '.';

ALLSTATS
	: 'allstats'
	;

ALLFEATURES
	: 'allfeatures'
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

// TODO: Identifiers can be numbers, so a redesign will be required.
IDENTIFIER
	: [a-zA-Z0-9_]+ { isNotDefaultMode() || !isBuiltInCommand(getText()) }?
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

fragment GENDERS
	: 'male'
	| 'female'
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

mode IN_PARAM_V;

PARAM_V_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_V_POP_PAREN_LEFT  : TOKEN_PAREN_LEFT  { isNextArgLeftParen() }? { popNextMode(); popNextMode(); } -> type(TOKEN_PAREN_LEFT);
PARAM_V_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }?         { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_V_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?          { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_V_POP_END         : END               { canCmdPop() }?          { popNextMode(true); } -> type(END);
PARAM_V_POP_COMMA       : TOKEN_COMMA                                 { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_V_FUNC_GROUP_1    : FUNC_GROUP_1 { pushCommand("(P)"); }    -> type(FUNCTION);
PARAM_V_FUNC_GROUP_2    : FUNC_GROUP_2 { pushCommand("(S)"); }    -> type(FUNCTION);
PARAM_V_FUNC_GROUP_3    : FUNC_GROUP_3 { pushCommand("(SS)"); }   -> type(FUNCTION);
PARAM_V_FUNC_GROUP_4    : FUNC_GROUP_4 { pushCommand("(ESSS)"); } -> type(FUNCTION);
PARAM_V_FUNC_GROUP_5    : FUNC_GROUP_5 { pushCommand("(SV)"); }   -> type(FUNCTION);
PARAM_V_FUNC_GROUP_6    : FUNC_GROUP_6 { pushCommand("(V)"); }    -> type(FUNCTION);
PARAM_V_MC_NOINDEX      : MC_NOINDEX                              -> type(MESSAGECODE);
PARAM_V_MC_SIMPLE       : MC_SIMPLE     { pushCommand("(P)"); }   -> type(MESSAGECODE);
PARAM_V_MC_COMPUTED_S   : MC_COMPUTED_S { pushCommand("(V)"); }   -> type(MESSAGECODE);
PARAM_V_MC_COMPUTED_V   : MC_COMPUTED_V { pushCommand("(E)"); }   -> type(MESSAGECODE);
PARAM_V_MC_I            : MC_I          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
PARAM_V_MC_T            : MC_T          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_V_MC_e            : MC_e          { pushCommand("(EES)"); } -> type(MESSAGECODE);
PARAM_V_MC_i            : MC_i          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
PARAM_V_MC_R            : MC_R          { pushCommand("(L)"); }   -> type(MESSAGECODE);
PARAM_V_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
PARAM_V_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
PARAM_V_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
PARAM_V_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
PARAM_V_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
PARAM_V_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
PARAM_V_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
PARAM_V_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
PARAM_V_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
PARAM_V_LITERAL         : LITERAL -> type(LITERAL);
PARAM_V_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
PARAM_V_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
PARAM_V_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
PARAM_V_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
PARAM_V_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
PARAM_V_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);
PARAM_V_TOKEN_PERIOD        : TOKEN_PERIOD -> type(TOKEN_PERIOD);

// --------------------------------------------------------
mode IN_PARAM_E;

PARAM_E_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_E_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() && m_bracketCount == 0 }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_E_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?                         { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_E_POP_END         : END               { canCmdPop() }?                         { popNextMode(true); } -> type(END);
PARAM_E_POP_COMMA       : TOKEN_COMMA       { canCommaPop() }?                       { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_E_COMMA           : TOKEN_COMMA       { !canCommaPop() }?   -> type(TOKEN_COMMA);
PARAM_E_FUNC_GROUP_1    : FUNC_GROUP_1 { pushCommand("(P)"); }    -> type(FUNCTION);
PARAM_E_FUNC_GROUP_2    : FUNC_GROUP_2 { pushCommand("(S)"); }    -> type(FUNCTION);
PARAM_E_FUNC_GROUP_3    : FUNC_GROUP_3 { pushCommand("(SS)"); }   -> type(FUNCTION);
PARAM_E_FUNC_GROUP_4    : FUNC_GROUP_4 { pushCommand("(ESSS)"); } -> type(FUNCTION);
PARAM_E_FUNC_GROUP_5    : FUNC_GROUP_5 { pushCommand("(SV)"); }   -> type(FUNCTION);
PARAM_E_FUNC_GROUP_6    : FUNC_GROUP_6 { pushCommand("(V)"); }    -> type(FUNCTION);
PARAM_E_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
PARAM_E_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
PARAM_E_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
PARAM_E_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
PARAM_E_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
PARAM_E_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
PARAM_E_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
PARAM_E_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
PARAM_E_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
PARAM_E_LITERAL         : LITERAL -> type(LITERAL);
PARAM_E_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
PARAM_E_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
PARAM_E_OP_ADD            : OP_ADD -> type(OP_ADD);
PARAM_E_OP_SUB            : OP_SUB -> type(OP_SUB);
PARAM_E_OP_MUL            : OP_MUL -> type(OP_MUL);
PARAM_E_OP_DIV            : OP_DIV -> type(OP_DIV);
PARAM_E_OP_MOD            : OP_MOD -> type(OP_MOD);
PARAM_E_OP_POW            : OP_POW -> type(OP_POW);
PARAM_E_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
PARAM_E_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
PARAM_E_OP_LESS           : OP_LESS -> type(OP_LESS);
PARAM_E_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
PARAM_E_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
PARAM_E_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
PARAM_E_OP_IN             : OP_IN -> type(OP_IN);
PARAM_E_OP_INC            : OP_INC -> type(OP_INC);
PARAM_E_OP_DEC            : OP_DEC -> type(OP_DEC);
PARAM_E_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
PARAM_E_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
PARAM_E_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
PARAM_E_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
PARAM_E_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
PARAM_E_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT  { ++m_bracketCount; } -> type(TOKEN_PAREN_LEFT);
PARAM_E_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT { --m_bracketCount; } -> type(TOKEN_PAREN_RIGHT);
PARAM_E_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
PARAM_E_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
PARAM_E_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);
PARAM_E_TOKEN_PERIOD        : TOKEN_PERIOD -> type(TOKEN_PERIOD);

// --------------------------------------------------------
mode IN_PARAM_S;

PARAM_S_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }?  { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_S_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?   { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_S_POP_END         : END               { canCmdPop() }?   { popNextMode(true); } -> type(END);
PARAM_S_POP_COMMA       : TOKEN_COMMA       { canCommaPop() }? { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_S_MC_NOINDEX      : MC_NOINDEX                              -> type(MESSAGECODE);
PARAM_S_MC_SIMPLE       : MC_SIMPLE     { pushCommand("(P)"); }   -> type(MESSAGECODE);
PARAM_S_MC_COMPUTED_S   : MC_COMPUTED_S { pushCommand("(V)"); }   -> type(MESSAGECODE);
PARAM_S_MC_COMPUTED_V   : MC_COMPUTED_V { pushCommand("(E)"); }   -> type(MESSAGECODE);
PARAM_S_MC_I            : MC_I          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
PARAM_S_MC_T            : MC_T          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_S_MC_e            : MC_e          { pushCommand("(EES)"); } -> type(MESSAGECODE);
PARAM_S_MC_i            : MC_i          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
PARAM_S_MC_R            : MC_R          { pushCommand("(L)"); }   -> type(MESSAGECODE);
PARAM_S_STRING_ESCAPE   : '##'                                            -> type(STRING);
PARAM_S_STRING_LITERAL1 : ~[#),]+     { canFuncPop() && canCommaPop() }?  -> type(STRING);
PARAM_S_STRING_LITERAL2 : ~[#};,]+    { canCmdPop()  && canCommaPop() }?  -> type(STRING);
PARAM_S_STRING_LITERAL_END1 : ~[#)]+  { canFuncPop() && !canCommaPop() }? -> type(STRING);
PARAM_S_STRING_LITERAL_END2 : ~[#};]+ { canCmdPop()  && !canCommaPop() }? -> type(STRING);

// --------------------------------------------------------
mode IN_PARAM_L;

PARAM_L_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_L_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_L_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_L_COMMA           : TOKEN_COMMA   { emitIdentifier(GS1Lexer::STRING, getText()); } -> type(TOKEN_COMMA);
PARAM_L_MC_NOINDEX      : MC_NOINDEX                              -> type(MESSAGECODE);
PARAM_L_MC_SIMPLE       : MC_SIMPLE     { pushCommand("(P)"); }   -> type(MESSAGECODE);
PARAM_L_MC_COMPUTED_S   : MC_COMPUTED_S { pushCommand("(V)"); }   -> type(MESSAGECODE);
PARAM_L_MC_COMPUTED_V   : MC_COMPUTED_V { pushCommand("(E)"); }   -> type(MESSAGECODE);
PARAM_L_MC_I            : MC_I          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
PARAM_L_MC_T            : MC_T          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_L_MC_e            : MC_e          { pushCommand("(EES)"); } -> type(MESSAGECODE);
PARAM_L_MC_i            : MC_i          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
PARAM_L_MC_R            : MC_R          { pushCommand("(L)"); }   -> type(MESSAGECODE);
PARAM_L_STRING_ESCAPE   : '##' -> type(STRING);
PARAM_L_STRING_LITERAL1  : ~[#),]+  { canFuncPop() }? -> type(STRING);
PARAM_L_STRING_LITERAL2  : ~[#};,]+ { canCmdPop() }?  -> type(STRING);

// --------------------------------------------------------
mode IN_PARAM_M;

PARAM_M_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_M_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_M_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_M_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_M_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_M_MC_NOINDEX      : MC_NOINDEX                              -> type(MESSAGECODE);
PARAM_M_MC_SIMPLE       : MC_SIMPLE     { pushCommand("(P)"); }   -> type(MESSAGECODE);
PARAM_M_MC_COMPUTED_S   : MC_COMPUTED_S { pushCommand("(V)"); }   -> type(MESSAGECODE);
PARAM_M_MC_COMPUTED_V   : MC_COMPUTED_V { pushCommand("(E)"); }   -> type(MESSAGECODE);
PARAM_M_MC_I            : MC_I          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
PARAM_M_MC_T            : MC_T          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_M_MC_e            : MC_e          { pushCommand("(EES)"); } -> type(MESSAGECODE);
PARAM_M_MC_i            : MC_i          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
PARAM_M_MC_R            : MC_R          { pushCommand("(L)"); }   -> type(MESSAGECODE);
PARAM_M_FUNC_GROUP_1    : FUNC_GROUP_1 { pushCommand("(P)"); }    -> type(FUNCTION);
PARAM_M_FUNC_GROUP_2    : FUNC_GROUP_2 { pushCommand("(S)"); }    -> type(FUNCTION);
PARAM_M_FUNC_GROUP_3    : FUNC_GROUP_3 { pushCommand("(SS)"); }   -> type(FUNCTION);
PARAM_M_FUNC_GROUP_4    : FUNC_GROUP_4 { pushCommand("(ESSS)"); } -> type(FUNCTION);
PARAM_M_FUNC_GROUP_5    : FUNC_GROUP_5 { pushCommand("(SV)"); }   -> type(FUNCTION);
PARAM_M_FUNC_GROUP_6    : FUNC_GROUP_6 { pushCommand("(V)"); }    -> type(FUNCTION);
PARAM_M_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
PARAM_M_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
PARAM_M_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
PARAM_M_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
PARAM_M_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
PARAM_M_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
PARAM_M_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
PARAM_M_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
PARAM_M_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
PARAM_M_LITERAL       : LITERAL -> type(LITERAL);
PARAM_M_IDENTIFIER    : IDENTIFIER -> type(IDENTIFIER);
PARAM_M_OP_ADD        : OP_ADD -> type(OP_ADD);
PARAM_M_OP_SUB        : OP_SUB -> type(OP_SUB);
PARAM_M_OP_MUL        : OP_MUL -> type(OP_MUL);
PARAM_M_OP_DIV        : OP_DIV -> type(OP_DIV);
PARAM_M_OP_MOD        : OP_MOD -> type(OP_MOD);
PARAM_M_OP_POW        : OP_POW -> type(OP_POW);
PARAM_M_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
PARAM_M_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
PARAM_M_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
PARAM_M_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);

// --------------------------------------------------------
mode IN_PARAM_B;

PARAM_B_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_B_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_B_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_B_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_B_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_B_BADDY           : BADDY -> type(BADDY);

// --------------------------------------------------------
mode IN_PARAM_I;

PARAM_I_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_I_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_I_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_I_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_I_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_I_ITEM            : ITEMNAMES -> type(ITEM);

// --------------------------------------------------------
mode IN_PARAM_C;

PARAM_C_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_C_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_C_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_C_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_C_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_C_COLOR           : COLORS -> type(COLOR);

// --------------------------------------------------------
mode IN_PARAM_G;

PARAM_G_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_G_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_G_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_G_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_G_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_G_GENDER          : GENDERS -> type(GENDER);

// --------------------------------------------------------
mode IN_PARAM_U;

PARAM_U_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_U_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_U_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_U_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_U_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_U_CARRY           : CARRYNAMES -> type(CARRY);

// --------------------------------------------------------
mode IN_PARAM_D;

PARAM_D_WS              : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_D_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT { canFuncPop() }? { popNextMode(true); } -> type(TOKEN_PAREN_RIGHT);
PARAM_D_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { canCmdPop() }?  { popNextMode(true); } -> type(TOKEN_BRACE_RIGHT);
PARAM_D_POP_END         : END               { canCmdPop() }?  { popNextMode(true); } -> type(END);
PARAM_D_POP_COMMA       : TOKEN_COMMA                         { popNextMode(); }     -> type(TOKEN_COMMA);
PARAM_D_DIR             : DIR -> type(DIRECTION);
PARAM_D_FUNC_GROUP_1    : FUNC_GROUP_1 { pushCommand("(P)"); }    -> type(FUNCTION);
PARAM_D_FUNC_GROUP_2    : FUNC_GROUP_2 { pushCommand("(S)"); }    -> type(FUNCTION);
PARAM_D_FUNC_GROUP_3    : FUNC_GROUP_3 { pushCommand("(SS)"); }   -> type(FUNCTION);
PARAM_D_FUNC_GROUP_4    : FUNC_GROUP_4 { pushCommand("(ESSS)"); } -> type(FUNCTION);
PARAM_D_FUNC_GROUP_5    : FUNC_GROUP_5 { pushCommand("(SV)"); }   -> type(FUNCTION);
PARAM_D_FUNC_GROUP_6    : FUNC_GROUP_6 { pushCommand("(V)"); }    -> type(FUNCTION);
PARAM_D_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
PARAM_D_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
PARAM_D_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
PARAM_D_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
PARAM_D_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
PARAM_D_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
PARAM_D_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
PARAM_D_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
PARAM_D_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
PARAM_D_LITERAL         : LITERAL -> type(LITERAL);
PARAM_D_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
PARAM_D_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
PARAM_D_OP_ADD            : OP_ADD -> type(OP_ADD);
PARAM_D_OP_SUB            : OP_SUB -> type(OP_SUB);
PARAM_D_OP_MUL            : OP_MUL -> type(OP_MUL);
PARAM_D_OP_DIV            : OP_DIV -> type(OP_DIV);
PARAM_D_OP_MOD            : OP_MOD -> type(OP_MOD);
PARAM_D_OP_POW            : OP_POW -> type(OP_POW);
PARAM_D_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
PARAM_D_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
PARAM_D_OP_LESS           : OP_LESS -> type(OP_LESS);
PARAM_D_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
PARAM_D_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
PARAM_D_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
PARAM_D_OP_IN             : OP_IN -> type(OP_IN);
PARAM_D_OP_INC            : OP_INC -> type(OP_INC);
PARAM_D_OP_DEC            : OP_DEC -> type(OP_DEC);
PARAM_D_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
PARAM_D_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
PARAM_D_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
PARAM_D_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
PARAM_D_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
PARAM_D_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
PARAM_D_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
PARAM_D_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
PARAM_D_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
PARAM_D_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);
PARAM_D_TOKEN_PERIOD        : TOKEN_PERIOD -> type(TOKEN_PERIOD);

// --------------------------------------------------------
mode IN_PARAM_Z;

PARAM_Z_START           : TOKEN_BRACE_LEFT  { m_bracketCount == 0 }? { ++m_bracketCount; } -> channel(HIDDEN);
PARAM_Z_POP_END         : END               { m_bracketCount == 0 }? { popNextMode(); }    -> type(END);
PARAM_Z_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT { m_bracketCount == 1 }? { --m_bracketCount; popNextMode(); } -> channel(HIDDEN);
PARAM_Z_BRACE_LEFT      : TOKEN_BRACE_LEFT                           { ++m_bracketCount; } -> type(STRING);
PARAM_Z_BRACE_RIGHT     : TOKEN_BRACE_RIGHT                          { --m_bracketCount; } -> type(STRING);
PARAM_Z_END             : END               { m_bracketCount != 0 }? -> type(STRING);
PARAM_Z_STRING          : ~[{};]+ -> type(STRING);

// --------------------------------------------------------
mode IN_PARAM_X;

PARAM_X_IDENTIFIER    : IDENTIFIER    { popNextMode(); }        -> type(IDENTIFIER);
PARAM_X_MC_NOINDEX    : MC_NOINDEX                              -> type(MESSAGECODE);
PARAM_X_MC_SIMPLE     : MC_SIMPLE     { pushCommand("(P)"); }   -> type(MESSAGECODE);
PARAM_X_MC_COMPUTED_S : MC_COMPUTED_S { pushCommand("(V)"); }   -> type(MESSAGECODE);
PARAM_X_MC_COMPUTED_V : MC_COMPUTED_V { pushCommand("(E)"); }   -> type(MESSAGECODE);
PARAM_X_MC_I          : MC_I          { pushCommand("(VP)"); }  -> type(MESSAGECODE);
PARAM_X_MC_T          : MC_T          { pushCommand("(S)"); }   -> type(MESSAGECODE);
PARAM_X_MC_e          : MC_e          { pushCommand("(EES)"); } -> type(MESSAGECODE);
PARAM_X_MC_i          : MC_i          { pushCommand("(SP)"); }  -> type(MESSAGECODE);
PARAM_X_MC_R          : MC_R          { pushCommand("(L)"); }   -> type(MESSAGECODE);

// --------------------------------------------------------
mode IN_PARAM_1;

PARAM_1_WS               : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_1_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT { m_commandStates.back().popMode = POPMODE_FUNCTION; popNextMode(); } -> type(TOKEN_PAREN_LEFT);

// --------------------------------------------------------
mode IN_PARAM_2;

PARAM_2_WS               : WHITESPACE+ -> type(WS), channel(HIDDEN);
PARAM_2_TOKEN_PAREN_LEFT : TOKEN_PAREN_RIGHT { popNextMode(); } -> type(TOKEN_PAREN_RIGHT);
