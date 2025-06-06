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
constexpr std::array<std::string_view, 199> builtInCommands = {
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
    "copyflagss",
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
	if (!m_pendingTokens.empty())
		m_pickQueuedTokens = true;

	return next;
}

void emitIdentifier(size_t type, std::string_view name)
{
	m_pendingTokens.emplace_back(_factory->create({ this, _input }, type, _text, antlr4::Lexer::HIDDEN, tokenStartCharIndex + name.length(), tokenStartCharIndex + name.length(), tokenStartLine, tokenStartCharPositionInLine + name.length()));
}

bool m_pickQueuedTokens = false;
std::deque<std::unique_ptr<antlr4::Token>> m_pendingTokens{};
// --------------------------------------------------------
}

tokens { COMMAND, FUNCTION, MESSAGECODE, STRING, BADDY, COLOR }

/*
	Mode parameter argument guide:
	- E  expression
	- V  variable (number/array/string)
	- S  string
	- P  parameters (any number of normal parameter types)
	- O  optional parameters
	- M  message code
	- B  baddy name
	- 1  left parenthesis (for starting function arguments that begin with a string)
	- ITEM    item name
	- COLOR   color name
	- GENDER  gender name
	- CARRY   carry item name
	- DIRECTION   direction name or number
	- STRINGLIST  variable length comma-separated string list
*/

CMD_SETSTRING            : 'setstring'            { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(COMMAND), pushMode(IN_VS);
CMD_ADDSTRING            : 'addstring'            { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(COMMAND), pushMode(IN_VS);
CMD_INSERTSTRING         : 'insertstring'         { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(COMMAND), pushMode(IN_VES);
CMD_REPLACESTRING        : 'replacestring'        { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(COMMAND), pushMode(IN_VES);
CMD_REMOVESTRING         : 'removestring'         { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(COMMAND), pushMode(IN_VS);
CMD_DELETESTRING         : 'deletestring'         { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(COMMAND), pushMode(IN_V);
CMD_SET                  : 'set '                 -> type(COMMAND);
CMD_UNSET                : 'unset '               -> type(COMMAND);
CMD_SLEEP                : 'sleep'                -> type(COMMAND);
CMD_SETARRAY             : 'setarray'             -> type(COMMAND);
CMD_TIMEREVERYWHERE      : 'timereverywhere'      -> type(COMMAND);
CMD_SETGIF               : 'setgif '              -> type(COMMAND), pushMode(IN_S);
CMD_SETIMG               : 'setimg'               -> type(COMMAND), pushMode(IN_S);
CMD_SETIMGPART           : 'setimgpart'           -> type(COMMAND), pushMode(IN_SP);
CMD_HIDE                 : 'hide'                 -> type(COMMAND);
CMD_SHOW                 : 'show'                 -> type(COMMAND);
CMD_DONTBLOCK            : 'dontblock'            -> type(COMMAND);
CMD_BLOCKAGAIN           : 'blockagain'           -> type(COMMAND);
CMD_DRAWOVERPLAYER       : 'drawoverplayer'       -> type(COMMAND);
CMD_DRAWUNDERPLAYER      : 'drawunderplayer'      -> type(COMMAND);
CMD_DRAWASLIGHT          : 'drawaslight'          -> type(COMMAND);
CMD_SETEFFECTMODE        : 'seteffectmode '       -> type(COMMAND);
CMD_CANBECARRIED         : 'canbecarried'         -> type(COMMAND);
CMD_CANNOTBECARRIED      : 'cannotbecarried'      -> type(COMMAND);
CMD_CANBEPUSHED          : 'canbepushed'          -> type(COMMAND);
CMD_CANNOTBEPUSHED       : 'cannotbepushed'       -> type(COMMAND);
CMD_CANBEPULLED          : 'canbepulled'          -> type(COMMAND);
CMD_CANNOTBEPULLED       : 'cannotbepulled'       -> type(COMMAND);
CMD_MOVE                 : 'move'                 -> type(COMMAND);
CMD_SAY                  : 'say '                 -> type(COMMAND);
CMD_SAY2                 : 'say2'                 -> type(COMMAND), pushMode(IN_S);
CMD_LAY                  : 'lay '                 -> type(COMMAND), pushMode(IN_ITEM);
CMD_LAY2                 : 'lay2'                 -> type(COMMAND), pushMode(IN_ITEM);
CMD_TAKE                 : 'take '                -> type(COMMAND), pushMode(IN_ITEM);
CMD_TAKE2                : 'take2'                -> type(COMMAND);
CMD_MESSAGE              : 'message'              -> type(COMMAND), pushMode(IN_S);
CMD_TIMERSHOW            : 'timershow'            -> type(COMMAND);
CMD_SHOWCHARACTER        : 'showcharacter'        -> type(COMMAND);
CMD_SETCHARPROP          : 'setcharprop'          -> type(COMMAND), pushMode(IN_MS);
CMD_SETCHARANI           : 'setcharani'           -> type(COMMAND), pushMode(IN_S);
CMD_SETCHARGENDER        : 'setchargender'        -> type(COMMAND), pushMode(IN_GENDER);
CMD_TRIGGERACTION        : 'triggeraction'        -> type(COMMAND), pushMode(IN_EESS);
CMD_PUTNPC               : 'putnpc'               -> type(COMMAND), pushMode(IN_SSP);
CMD_PUTNPC2              : 'putnpc2'              -> type(COMMAND), pushMode(IN_SSP);
CMD_CALLNPC              : 'callnpc'              -> type(COMMAND), pushMode(IN_ES);
CMD_CALLWEAPON           : 'callweapon'           -> type(COMMAND), pushMode(IN_ES);
CMD_DESTROY              : 'destroy'              -> type(COMMAND);
CMD_CARRYOBJECT          : 'carryobject'          -> type(COMMAND), pushMode(IN_CARRY);
CMD_THROWCARRY           : 'throwcarry'           -> type(COMMAND);
CMD_FOLLOWPLAYER         : 'followplayer'         -> type(COMMAND);
CMD_TOINVENTORY          : 'toinventory'          -> type(COMMAND);
CMD_TOWEAPONS            : 'toweapons'            -> type(COMMAND), pushMode(IN_S);
CMD_SETCOLOREFFECT       : 'setcoloreffect'       -> type(COMMAND);
CMD_SETZOOMEFFECT        : 'setzoomeffect'        -> type(COMMAND);
CMD_SHOWIMG              : 'showimg'              -> type(COMMAND), pushMode(IN_ESP);
CMD_SHOWIMG2             : 'showimg2'             -> type(COMMAND), pushMode(IN_ESP);
CMD_SHOWANI              : 'showani'              -> type(COMMAND), pushMode(IN_ESP);
CMD_SHOWANI2             : 'showani2'             -> type(COMMAND), pushMode(IN_ESP);
CMD_SHOWPOLY             : 'showpoly'             -> type(COMMAND);
CMD_SHOWPOLY2            : 'showpoly2'            -> type(COMMAND);
CMD_SHOWTEXT             : 'showtext'             -> type(COMMAND), pushMode(IN_EEESSS);
CMD_SHOWTEXT2            : 'showtext2'            -> type(COMMAND), pushMode(IN_EEEESSS);
CMD_HIDEIMG              : 'hideimg'              -> type(COMMAND);
CMD_HIDEIMGS             : 'hideimgs'             -> type(COMMAND);
CMD_CHANGEIMGPART        : 'changeimgpart'        -> type(COMMAND);
CMD_CHANGEIMGVIS         : 'changeimgvis'         -> type(COMMAND);
CMD_CHANGEIMGCOLORS      : 'changeimgcolors'      -> type(COMMAND);
CMD_CHANGEIMGZOOM        : 'changeimgzoom'        -> type(COMMAND);
CMD_CHANGEIMGMODE        : 'changeimgmode'        -> type(COMMAND);
CMD_SHOOTARROW           : 'shootarrow'           -> type(COMMAND), pushMode(IN_DIRECTION);
CMD_SHOOTFIREBALL        : 'shootfireball'        -> type(COMMAND), pushMode(IN_DIRECTION);
CMD_SHOOTFIREBLAST       : 'shootfireblast'       -> type(COMMAND), pushMode(IN_DIRECTION);
CMD_SHOOTNUKE            : 'shootnuke'            -> type(COMMAND), pushMode(IN_DIRECTION);
CMD_SHOOTBALL            : 'shootball'            -> type(COMMAND);
CMD_SPYFIRE              : 'spyfire'              -> type(COMMAND);
CMD_HITPLAYER            : 'hitplayer'            -> type(COMMAND);
CMD_HITNPC               : 'hitnpc'               -> type(COMMAND);
CMD_HITOBJECTS           : 'hitobjects'           -> type(COMMAND);
CMD_HIDELOCAL            : 'hidelocal'            -> type(COMMAND);
CMD_SHOWLOCAL            : 'showlocal'            -> type(COMMAND);
CMD_DONTBLOCKLOCAL       : 'dontblocklocal'       -> type(COMMAND);
CMD_BLOCKAGAINLOCAL      : 'blockagainlocal'      -> type(COMMAND);
CMD_TAKEHORSE            : 'takehorse'            -> type(COMMAND);
CMD_TOKENIZE             : 'tokenize'             -> type(COMMAND), pushMode(IN_S);
CMD_TOKENIZE2            : 'tokenize2'            -> type(COMMAND), pushMode(IN_SS);
CMD_SETSHAPE             : 'setshape'             -> type(COMMAND);
CMD_SETSHAPE2            : 'setshape2'            -> type(COMMAND);
CMD_WRAPTEXT             : 'wraptext'             -> type(COMMAND), pushMode(IN_ESS);
CMD_WRAPTEXT2            : 'wraptext2'            -> type(COMMAND), pushMode(IN_EESS);
CMD_SETSHOOTPARAMS       : 'setshootparams'       -> type(COMMAND), pushMode(IN_STRINGLIST);
CMD_SHOOT                : 'shoot'                -> type(COMMAND), pushMode(IN_EEEEESS);
CMD_SETLEVEL             : 'setlevel '            -> type(COMMAND), pushMode(IN_S);
CMD_SETLEVEL2            : 'setlevel2'            -> type(COMMAND), pushMode(IN_SP);
CMD_SETURLLEVEL          : 'seturllevel'          -> type(COMMAND), pushMode(IN_S);
CMD_SETBODY              : 'setbody'              -> type(COMMAND), pushMode(IN_S);
CMD_SETHEAD              : 'sethead'              -> type(COMMAND), pushMode(IN_S);
CMD_SETSWORD             : 'setsword'             -> type(COMMAND), pushMode(IN_SP);
CMD_SETSHIELD            : 'setshield'            -> type(COMMAND), pushMode(IN_SP);
CMD_SETANI               : 'setani'               -> type(COMMAND), pushMode(IN_SS);
CMD_SETPLAYERDIR         : 'setplayerdir'         -> type(COMMAND), pushMode(IN_DIRECTION);
CMD_SETGENDER            : 'setgender'            -> type(COMMAND), pushMode(IN_GENDER);
CMD_SETSKINCOLOR         : 'setskincolor'         -> type(COMMAND), pushMode(IN_COLOR);
CMD_SETCOATCOLOR         : 'setcoatcolor'         -> type(COMMAND), pushMode(IN_COLOR);
CMD_SETSLEEVECOLOR       : 'setsleevecolor'       -> type(COMMAND), pushMode(IN_COLOR);
CMD_SETSHOECOLOR         : 'setshoecolor'         -> type(COMMAND), pushMode(IN_COLOR);
CMD_SETBELTCOLOR         : 'setbeltcolor'         -> type(COMMAND), pushMode(IN_COLOR);
CMD_SETPLAYERPROP        : 'setplayerprop'        -> type(COMMAND), pushMode(IN_MS);
CMD_TAKEPLAYERCARRY      : 'takeplayercarry'      -> type(COMMAND);
CMD_TAKEPLAYERHORSE      : 'takeplayerhorse'      -> type(COMMAND);
CMD_DISABLEWEAPONS       : 'disableweapons'       -> type(COMMAND);
CMD_ENABLEWEAPONS        : 'enableweapons'        -> type(COMMAND);
CMD_FREEZEPLAYER         : 'freezeplayer '        -> type(COMMAND);
CMD_FREEZEPLAYER2        : 'freezeplayer2'        -> type(COMMAND);
CMD_UNFREEZEPLAYER       : 'unfreezeplayer'       -> type(COMMAND);
CMD_HIDEPLAYER           : 'hideplayer'           -> type(COMMAND);
CMD_HIDESWORD            : 'hidesword'            -> type(COMMAND);
CMD_HURT                 : 'hurt '                -> type(COMMAND);
CMD_DISABLEDEFMOVEMENT   : 'disabledefmovement'   -> type(COMMAND);
CMD_ENABLEDEFMOVEMENT    : 'enabledefmovement'    -> type(COMMAND);
CMD_DISABLESELECTWEAPONS : 'disableselectweapons' -> type(COMMAND);
CMD_ENABLESELECTWEAPONS  : 'enableselectweapons'  -> type(COMMAND);
CMD_DISABLEPAUSE         : 'disablepause'         -> type(COMMAND);
CMD_ENABLEPAUSE          : 'enablepause'          -> type(COMMAND);
CMD_DISABLEMAP           : 'disablemap'           -> type(COMMAND);
CMD_ENABLEMAP            : 'enablemap'            -> type(COMMAND);
CMD_ENABLEFEATURES       : 'enablefeatures'       -> type(COMMAND);
CMD_REPLACEANI           : 'replaceani'           -> type(COMMAND), pushMode(IN_SS);
CMD_ATTACHPLAYERTOOBJ    : 'attachplayertoobj'    -> type(COMMAND);
CMD_DETACHPLAYER         : 'detachplayer'         -> type(COMMAND);
CMD_UPDATEBOARD          : 'updateboard'          -> type(COMMAND);
CMD_UPDATEBOARD2         : 'updateboard2'         -> type(COMMAND);
CMD_PUTOBJECT            : 'putobject'            -> type(COMMAND), pushMode(IN_SP);
CMD_PUTBOMB              : 'putbomb'              -> type(COMMAND);
CMD_PUTEXPLOSION         : 'putexplosion'         -> type(COMMAND);
CMD_PUTEXPLOSION2        : 'putexplosion2'        -> type(COMMAND);
CMD_PUTLEAPS             : 'putleaps'             -> type(COMMAND);
CMD_PUTHORSE             : 'puthorse'             -> type(COMMAND), pushMode(IN_SP);
CMD_SETBACKPAL           : 'setbackpal'           -> type(COMMAND), pushMode(IN_S);
CMD_SETLETTERS           : 'setletters'           -> type(COMMAND), pushMode(IN_S);
CMD_SETMAP               : 'setmap'               -> type(COMMAND), pushMode(IN_SSP);
CMD_SETMINIMAP           : 'setminimap'           -> type(COMMAND), pushMode(IN_SSP);
CMD_SETEFFECT            : 'seteffect'            -> type(COMMAND);
CMD_SETFOCUS             : 'setfocus'             -> type(COMMAND);
CMD_RESETFOCUS           : 'resetfocus'           -> type(COMMAND);
CMD_NOPLAYERKILLING      : 'noplayerkilling'      -> type(COMMAND);
CMD_NOPLAYERONWALL       : 'noplayeronwall'       -> type(COMMAND);
CMD_REMOVEBOMB           : 'removebomb'           -> type(COMMAND);
CMD_REMOVEARROW          : 'removearrow'          -> type(COMMAND);
CMD_REMOVEITEM           : 'removeitem'           -> type(COMMAND);
CMD_REMOVEEXPLO          : 'removeexplo'          -> type(COMMAND);
CMD_REMOVEHORSE          : 'removehorse'          -> type(COMMAND);
CMD_EXPLODEBOMB          : 'explodebomb'          -> type(COMMAND);
CMD_REFLECTARROW         : 'reflectarrow'         -> type(COMMAND);
CMD_ADDTILEDEF           : 'addtiledef'           -> type(COMMAND), pushMode(IN_SSP);
CMD_ADDTILEDEF2          : 'addtiledef2'          -> type(COMMAND), pushMode(IN_SSP);
CMD_REMOVETILEDEFS       : 'removetiledefs'       -> type(COMMAND), pushMode(IN_S);
CMD_LOADMAP              : 'loadmap'              -> type(COMMAND), pushMode(IN_S);
CMD_UPDATETERRAIN        : 'updateterrain'        -> type(COMMAND);
CMD_SHOWSTATS            : 'showstats'            -> type(COMMAND);
CMD_PUTCOMP              : 'putcomp'              -> type(COMMAND), pushMode(IN_BP);
CMD_PUTNEWCOMP           : 'putnewcomp'           -> type(COMMAND), pushMode(IN_BEESP);
CMD_HITCOMPU             : 'hitcompu'             -> type(COMMAND);
CMD_REMOVECOMPUS         : 'removecompus'         -> type(COMMAND);
CMD_PLAY                 : 'play '                -> type(COMMAND), pushMode(IN_S);
CMD_PLAY2                : 'play2'                -> type(COMMAND), pushMode(IN_SP);
CMD_PLAYLOOPED           : 'playlooped'           -> type(COMMAND), pushMode(IN_S);
CMD_STOPSOUND            : 'stopsound'            -> type(COMMAND), pushMode(IN_S);
CMD_STOPMIDI             : 'stopmidi'             -> type(COMMAND);
CMD_SETMUSICVOLUME       : 'setmusicvolume'       -> type(COMMAND);
CMD_OPENURL              : 'openurl'              -> type(COMMAND), pushMode(IN_S);
CMD_OPENURL2             : 'openurl2'             -> type(COMMAND), pushMode(IN_SP);
CMD_SHOWFILE             : 'showfile'             -> type(COMMAND), pushMode(IN_S);
CMD_JOIN                 : 'join '                -> type(COMMAND), pushMode(IN_S);
CMD_SETCURSOR            : 'setcursor '           -> type(COMMAND);
CMD_CANWARP              : 'canwarp'              -> type(COMMAND);
CMD_CANWARP2             : 'canwarp2'             -> type(COMMAND);
CMD_CANNOTWARP           : 'cannotwarp'           -> type(COMMAND);
CMD_ADDWEAPON            : 'addweapon'            -> type(COMMAND), pushMode(IN_S);
CMD_REMOVEWEAPON         : 'removeweapon'         -> type(COMMAND), pushMode(IN_S);
CMD_SETSPRITESIMAGE      : 'setspritesimage'      -> type(COMMAND), pushMode(IN_S);
CMD_SETSTATUSIMAGE       : 'setstatusimage'       -> type(COMMAND), pushMode(IN_S);
CMD_ADDGUILDMEMBER       : 'addguildmember'       -> type(COMMAND), pushMode(IN_SSS);
CMD_REMOVEGUILDMEMBER    : 'removeguildmember'    -> type(COMMAND), pushMode(IN_SSS);
CMD_REMOVEGUILD          : 'removeguild'          -> type(COMMAND), pushMode(IN_S);
CMD_COPYSTRINGS          : 'copystrings'          -> type(COMMAND), pushMode(IN_SS);
CMD_COPYFLAGSS           : 'copyflagss'           -> type(COMMAND), pushMode(IN_SS);
CMD_SENDTORC             : 'sendtorc'             -> type(COMMAND), pushMode(IN_S);
CMD_SENDTONC             : 'sendtonc'             -> type(COMMAND), pushMode(IN_S);
CMD_SENDPM               : 'sendpm'               -> type(COMMAND), pushMode(IN_S);
CMD_SETPM                : 'setpm'                -> type(COMMAND), pushMode(IN_S);
CMD_SENDRPGMESSAGE       : 'sendrpgmessage'       -> type(COMMAND), pushMode(IN_S);
CMD_SERVERWARP           : 'serverwarp'           -> type(COMMAND), pushMode(IN_S);
CMD_SETZ                 : 'setz'				  -> type(COMMAND);
CMD_SETCURSOR2           : 'setcursor2'           -> type(COMMAND), pushMode(IN_S);
CMD_COPYLEVEL            : 'copylevel'            -> type(COMMAND), pushMode(IN_SS);
CMD_DELETELEVEL          : 'deletelevel'          -> type(COMMAND), pushMode(IN_S);
CMD_SAVEINFO             : 'saveinfo'             -> type(COMMAND), pushMode(IN_SS);
CMD_SAVELOG              : 'savelog '             -> type(COMMAND), pushMode(IN_S);
CMD_SAVELOG2             : 'savelog2'             -> type(COMMAND), pushMode(IN_SS);

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
	) -> type(FUNCTION)
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
	) -> type(FUNCTION), pushMode(IN_F_1S)
	;

FUNC_GROUP_3
	: (
		'startswith'
		| 'strcontains'
		| 'strequals'
		| 'indexof'
	) -> type(FUNCTION), pushMode(IN_F_1SS)
	;

FUNC_GROUP_4 : ('textwidth' | 'textheight') -> type(FUNCTION), pushMode(IN_F_1ESSS);
FUNC_GROUP_5 : 'lindexof' -> type(FUNCTION), pushMode(IN_F_1SV);
FUNC_GROUP_6 : 'sarraylen' { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(FUNCTION), pushMode(IN_F_1V);

MC_NOINDEX		: '#' ([angcmWw1235678LFfpbND] | 'C' [01234] | 'P1' DIGITS? | 'P2' DIGITS? | 'P3' '0'? | 'P' [456789]) -> type(MESSAGECODE);
MC_SIMPLE		: '#' ([angcmWw1235678ptKkGNQ] | 'C' [01234] | 'P1' DIGITS? | 'P2' DIGITS? | 'P3' '0'? | 'P' [456789]) WS* '(' -> type(MESSAGECODE), pushMode(IN_F_P);
MC_COMPUTED_S	: '#' [s] WS* '(' { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
MC_COMPUTED_V	: '#' [v] WS* '(' { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
MC_I			: '#I' WS* '('    { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
MC_T			: '#T' WS* '(' -> type(MESSAGECODE), pushMode(IN_F_S);
MC_e			: '#e' WS* '(' -> type(MESSAGECODE), pushMode(IN_F_EES);
MC_i			: '#i' WS* '(' -> type(MESSAGECODE), pushMode(IN_F_SO);
MC_R			: '#R' WS* '(' -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);

// TODO: Some string lists are CSV.

STORAGE_THIS    : 'this.';
STORAGE_THISO   : 'thiso.';
STORAGE_CLIENT  : 'client.';
STORAGE_CLIENTR : 'clientr.';
STORAGE_SERVER  : 'server.';
STORAGE_SERVERR : 'serverr.';
STORAGE_LEVEL   : 'level.';
STORAGE_LOCAL   : 'local.';
STORAGE_TEMP    : 'temp.';

// Keep above KW_TRUE/KW_FALSE.
LITERAL
	: REAL
	| KW_TRUE
	| KW_FALSE
	;

KW_WITH			: 'with';
KW_FUNCTION		: 'function';
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

IDENTIFIER
	: [a-zA-Z_][a-zA-Z0-9_]* {!isBuiltInCommand(getText())}?
	;

LINECOMMENT
	: '//' ~ [\r\n]* -> skip
	;

BLOCKCOMMENT
	: '/*' .*? '*/' -> skip
	;

REAL
	: DIGITS+ ('.' DIGITS+)?
	| '0x' HEXDIGITS+
	;

END
	: ';'
	;

WS
	: WHITESPACE+ -> skip
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

fragment COLOR
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

mode IN_ITEM;

ITEM_WS              : WHITESPACE+ -> skip;
ITEM_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
ITEM_POP_END         : END -> type(END), popMode;
ITEM_POP_COMMA       : TOKEN_COMMA -> type(TOKEN_COMMA), popMode;
ITEM
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

// --------------------------------------------------------
mode IN_CARRY;

CARRY_WS              : WHITESPACE+ -> skip;
CARRY_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
CARRY_POP_END         : END -> type(END), popMode;
CARRY
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

// --------------------------------------------------------
mode IN_DIRECTION;

DIRECTION_WS              : WHITESPACE+ -> skip;
DIRECTION_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
DIRECTION_POP_END         : END -> type(END), popMode;
DIRECTION
	: 'up'
	| 'left'
	| 'down'
	| 'right'
	| [0123]
	;

// --------------------------------------------------------
mode IN_GENDER;

GENDER_WS              : WHITESPACE+ -> skip;
GENDER_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
GENDER_POP_END         : END -> type(END), popMode;
GENDER
	: 'male' | 'female' ;

// --------------------------------------------------------
mode IN_COLOR;

COLOR_WS              : WHITESPACE+ -> skip;
COLOR_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
COLOR_POP_END         : END -> type(END), popMode;
COLOR_                : COLOR ->type(COLOR) ;

// --------------------------------------------------------
mode IN_MS;

MS_WS    : WHITESPACE+ -> skip;
MS_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_S);
MS_MC_NOINDEX    : MC_NOINDEX -> type(MESSAGECODE);
MS_MC_SIMPLE     : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
MS_MC_COMPUTED_S : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
MS_MC_COMPUTED_V : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
MS_MC_I          : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
MS_MC_T          : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
MS_MC_e          : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
MS_MC_i          : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
MS_MC_R          : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
MS_FUNC_GROUP_1  : FUNC_GROUP_1 -> type(FUNCTION);
MS_FUNC_GROUP_2  : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
MS_FUNC_GROUP_3  : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
MS_FUNC_GROUP_4  : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
MS_FUNC_GROUP_5  : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
MS_FUNC_GROUP_6  : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
MS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
MS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
MS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
MS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
MS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
MS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
MS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
MS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
MS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
MS_LITERAL       : LITERAL -> type(LITERAL);
MS_IDENTIFIER    : IDENTIFIER -> type(IDENTIFIER);
MS_OP_ADD        : OP_ADD -> type(OP_ADD);
MS_OP_SUB        : OP_SUB -> type(OP_SUB);
MS_OP_MUL        : OP_MUL -> type(OP_MUL);
MS_OP_DIV        : OP_DIV -> type(OP_DIV);
MS_OP_MOD        : OP_MOD -> type(OP_MOD);
MS_OP_POW        : OP_POW -> type(OP_POW);
MS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
MS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
MS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
MS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);

// --------------------------------------------------------
mode IN_EEEESSS;

EEEESSS_WS           : WHITESPACE+ -> skip;
EEEESSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_EEESSS);
EEEESSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
EEEESSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
EEEESSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
EEEESSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
EEEESSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
EEEESSS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
EEEESSS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
EEEESSS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
EEEESSS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
EEEESSS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
EEEESSS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
EEEESSS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
EEEESSS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
EEEESSS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
EEEESSS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
EEEESSS_LITERAL         : LITERAL -> type(LITERAL);
EEEESSS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
EEEESSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
EEEESSS_OP_ADD            : OP_ADD -> type(OP_ADD);
EEEESSS_OP_SUB            : OP_SUB -> type(OP_SUB);
EEEESSS_OP_MUL            : OP_MUL -> type(OP_MUL);
EEEESSS_OP_DIV            : OP_DIV -> type(OP_DIV);
EEEESSS_OP_MOD            : OP_MOD -> type(OP_MOD);
EEEESSS_OP_POW            : OP_POW -> type(OP_POW);
EEEESSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
EEEESSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
EEEESSS_OP_LESS           : OP_LESS -> type(OP_LESS);
EEEESSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
EEEESSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
EEEESSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
EEEESSS_OP_IN             : OP_IN -> type(OP_IN);
EEEESSS_OP_INC            : OP_INC -> type(OP_INC);
EEEESSS_OP_DEC            : OP_DEC -> type(OP_DEC);
EEEESSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
EEEESSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
EEEESSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
EEEESSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
EEEESSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
EEEESSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
EEEESSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
EEEESSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
EEEESSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
EEEESSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_EEESSS;

EEESSS_WS           : WHITESPACE+ -> skip;
EEESSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_EESSS);
EEESSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
EEESSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
EEESSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
EEESSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
EEESSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
EEESSS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
EEESSS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
EEESSS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
EEESSS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
EEESSS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
EEESSS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
EEESSS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
EEESSS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
EEESSS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
EEESSS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
EEESSS_LITERAL         : LITERAL -> type(LITERAL);
EEESSS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
EEESSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
EEESSS_OP_ADD            : OP_ADD -> type(OP_ADD);
EEESSS_OP_SUB            : OP_SUB -> type(OP_SUB);
EEESSS_OP_MUL            : OP_MUL -> type(OP_MUL);
EEESSS_OP_DIV            : OP_DIV -> type(OP_DIV);
EEESSS_OP_MOD            : OP_MOD -> type(OP_MOD);
EEESSS_OP_POW            : OP_POW -> type(OP_POW);
EEESSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
EEESSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
EEESSS_OP_LESS           : OP_LESS -> type(OP_LESS);
EEESSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
EEESSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
EEESSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
EEESSS_OP_IN             : OP_IN -> type(OP_IN);
EEESSS_OP_INC            : OP_INC -> type(OP_INC);
EEESSS_OP_DEC            : OP_DEC -> type(OP_DEC);
EEESSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
EEESSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
EEESSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
EEESSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
EEESSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
EEESSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
EEESSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
EEESSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
EEESSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
EEESSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_EESSS;

EESSS_WS           : WHITESPACE+ -> skip;
EESSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_ESSS);
EESSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
EESSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
EESSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
EESSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
EESSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
EESSS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
EESSS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
EESSS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
EESSS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
EESSS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
EESSS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
EESSS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
EESSS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
EESSS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
EESSS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
EESSS_LITERAL         : LITERAL -> type(LITERAL);
EESSS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
EESSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
EESSS_OP_ADD            : OP_ADD -> type(OP_ADD);
EESSS_OP_SUB            : OP_SUB -> type(OP_SUB);
EESSS_OP_MUL            : OP_MUL -> type(OP_MUL);
EESSS_OP_DIV            : OP_DIV -> type(OP_DIV);
EESSS_OP_MOD            : OP_MOD -> type(OP_MOD);
EESSS_OP_POW            : OP_POW -> type(OP_POW);
EESSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
EESSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
EESSS_OP_LESS           : OP_LESS -> type(OP_LESS);
EESSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
EESSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
EESSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
EESSS_OP_IN             : OP_IN -> type(OP_IN);
EESSS_OP_INC            : OP_INC -> type(OP_INC);
EESSS_OP_DEC            : OP_DEC -> type(OP_DEC);
EESSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
EESSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
EESSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
EESSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
EESSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
EESSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
EESSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
EESSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
EESSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
EESSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_ESSS;

ESSS_WS           : WHITESPACE+ -> skip;
ESSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SSS);
ESSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
ESSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
ESSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
ESSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
ESSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
ESSS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
ESSS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
ESSS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
ESSS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
ESSS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
ESSS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
ESSS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
ESSS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
ESSS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
ESSS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
ESSS_LITERAL         : LITERAL -> type(LITERAL);
ESSS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
ESSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
ESSS_OP_ADD            : OP_ADD -> type(OP_ADD);
ESSS_OP_SUB            : OP_SUB -> type(OP_SUB);
ESSS_OP_MUL            : OP_MUL -> type(OP_MUL);
ESSS_OP_DIV            : OP_DIV -> type(OP_DIV);
ESSS_OP_MOD            : OP_MOD -> type(OP_MOD);
ESSS_OP_POW            : OP_POW -> type(OP_POW);
ESSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
ESSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
ESSS_OP_LESS           : OP_LESS -> type(OP_LESS);
ESSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
ESSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
ESSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
ESSS_OP_IN             : OP_IN -> type(OP_IN);
ESSS_OP_INC            : OP_INC -> type(OP_INC);
ESSS_OP_DEC            : OP_DEC -> type(OP_DEC);
ESSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
ESSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
ESSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
ESSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
ESSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
ESSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
ESSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
ESSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
ESSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
ESSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_EEEEESS;

EEEEESS_WS           : WHITESPACE+ -> skip;
EEEEESS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_EEEESS);
EEEEESS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
EEEEESS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
EEEEESS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
EEEEESS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
EEEEESS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
EEEEESS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
EEEEESS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
EEEEESS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
EEEEESS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
EEEEESS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
EEEEESS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
EEEEESS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
EEEEESS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
EEEEESS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
EEEEESS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
EEEEESS_LITERAL         : LITERAL -> type(LITERAL);
EEEEESS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
EEEEESS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
EEEEESS_OP_ADD            : OP_ADD -> type(OP_ADD);
EEEEESS_OP_SUB            : OP_SUB -> type(OP_SUB);
EEEEESS_OP_MUL            : OP_MUL -> type(OP_MUL);
EEEEESS_OP_DIV            : OP_DIV -> type(OP_DIV);
EEEEESS_OP_MOD            : OP_MOD -> type(OP_MOD);
EEEEESS_OP_POW            : OP_POW -> type(OP_POW);
EEEEESS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
EEEEESS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
EEEEESS_OP_LESS           : OP_LESS -> type(OP_LESS);
EEEEESS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
EEEEESS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
EEEEESS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
EEEEESS_OP_IN             : OP_IN -> type(OP_IN);
EEEEESS_OP_INC            : OP_INC -> type(OP_INC);
EEEEESS_OP_DEC            : OP_DEC -> type(OP_DEC);
EEEEESS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
EEEEESS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
EEEEESS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
EEEEESS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
EEEEESS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
EEEEESS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
EEEEESS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
EEEEESS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
EEEEESS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
EEEEESS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_EEEESS;

EEEESS_WS           : WHITESPACE+ -> skip;
EEEESS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_EEESS);
EEEESS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
EEEESS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
EEEESS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
EEEESS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
EEEESS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
EEEESS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
EEEESS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
EEEESS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
EEEESS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
EEEESS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
EEEESS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
EEEESS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
EEEESS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
EEEESS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
EEEESS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
EEEESS_LITERAL         : LITERAL -> type(LITERAL);
EEEESS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
EEEESS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
EEEESS_OP_ADD            : OP_ADD -> type(OP_ADD);
EEEESS_OP_SUB            : OP_SUB -> type(OP_SUB);
EEEESS_OP_MUL            : OP_MUL -> type(OP_MUL);
EEEESS_OP_DIV            : OP_DIV -> type(OP_DIV);
EEEESS_OP_MOD            : OP_MOD -> type(OP_MOD);
EEEESS_OP_POW            : OP_POW -> type(OP_POW);
EEEESS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
EEEESS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
EEEESS_OP_LESS           : OP_LESS -> type(OP_LESS);
EEEESS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
EEEESS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
EEEESS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
EEEESS_OP_IN             : OP_IN -> type(OP_IN);
EEEESS_OP_INC            : OP_INC -> type(OP_INC);
EEEESS_OP_DEC            : OP_DEC -> type(OP_DEC);
EEEESS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
EEEESS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
EEEESS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
EEEESS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
EEEESS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
EEEESS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
EEEESS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
EEEESS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
EEEESS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
EEEESS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_EEESS;

EEESS_WS           : WHITESPACE+ -> skip;
EEESS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_EESS);
EEESS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
EEESS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
EEESS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
EEESS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
EEESS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
EEESS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
EEESS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
EEESS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
EEESS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
EEESS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
EEESS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
EEESS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
EEESS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
EEESS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
EEESS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
EEESS_LITERAL         : LITERAL -> type(LITERAL);
EEESS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
EEESS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
EEESS_OP_ADD            : OP_ADD -> type(OP_ADD);
EEESS_OP_SUB            : OP_SUB -> type(OP_SUB);
EEESS_OP_MUL            : OP_MUL -> type(OP_MUL);
EEESS_OP_DIV            : OP_DIV -> type(OP_DIV);
EEESS_OP_MOD            : OP_MOD -> type(OP_MOD);
EEESS_OP_POW            : OP_POW -> type(OP_POW);
EEESS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
EEESS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
EEESS_OP_LESS           : OP_LESS -> type(OP_LESS);
EEESS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
EEESS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
EEESS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
EEESS_OP_IN             : OP_IN -> type(OP_IN);
EEESS_OP_INC            : OP_INC -> type(OP_INC);
EEESS_OP_DEC            : OP_DEC -> type(OP_DEC);
EEESS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
EEESS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
EEESS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
EEESS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
EEESS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
EEESS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
EEESS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
EEESS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
EEESS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
EEESS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_EESS;

EESS_WS           : WHITESPACE+ -> skip;
EESS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_ESS);
EESS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
EESS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
EESS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
EESS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
EESS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
EESS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
EESS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
EESS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
EESS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
EESS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
EESS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
EESS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
EESS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
EESS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
EESS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
EESS_LITERAL         : LITERAL -> type(LITERAL);
EESS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
EESS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
EESS_OP_ADD            : OP_ADD -> type(OP_ADD);
EESS_OP_SUB            : OP_SUB -> type(OP_SUB);
EESS_OP_MUL            : OP_MUL -> type(OP_MUL);
EESS_OP_DIV            : OP_DIV -> type(OP_DIV);
EESS_OP_MOD            : OP_MOD -> type(OP_MOD);
EESS_OP_POW            : OP_POW -> type(OP_POW);
EESS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
EESS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
EESS_OP_LESS           : OP_LESS -> type(OP_LESS);
EESS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
EESS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
EESS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
EESS_OP_IN             : OP_IN -> type(OP_IN);
EESS_OP_INC            : OP_INC -> type(OP_INC);
EESS_OP_DEC            : OP_DEC -> type(OP_DEC);
EESS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
EESS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
EESS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
EESS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
EESS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
EESS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
EESS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
EESS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
EESS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
EESS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_ESS;

ESS_WS           : WHITESPACE+ -> skip;
ESS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SS);
ESS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
ESS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
ESS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
ESS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
ESS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
ESS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
ESS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
ESS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
ESS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
ESS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
ESS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
ESS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
ESS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
ESS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
ESS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
ESS_LITERAL         : LITERAL -> type(LITERAL);
ESS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
ESS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
ESS_OP_ADD            : OP_ADD -> type(OP_ADD);
ESS_OP_SUB            : OP_SUB -> type(OP_SUB);
ESS_OP_MUL            : OP_MUL -> type(OP_MUL);
ESS_OP_DIV            : OP_DIV -> type(OP_DIV);
ESS_OP_MOD            : OP_MOD -> type(OP_MOD);
ESS_OP_POW            : OP_POW -> type(OP_POW);
ESS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
ESS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
ESS_OP_LESS           : OP_LESS -> type(OP_LESS);
ESS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
ESS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
ESS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
ESS_OP_IN             : OP_IN -> type(OP_IN);
ESS_OP_INC            : OP_INC -> type(OP_INC);
ESS_OP_DEC            : OP_DEC -> type(OP_DEC);
ESS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
ESS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
ESS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
ESS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
ESS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
ESS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
ESS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
ESS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
ESS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
ESS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_EES;

EES_WS           : WHITESPACE+ -> skip;
EES_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_ES);
EES_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
EES_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
EES_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
EES_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
EES_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
EES_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
EES_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
EES_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
EES_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
EES_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
EES_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
EES_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
EES_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
EES_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
EES_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
EES_LITERAL         : LITERAL -> type(LITERAL);
EES_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
EES_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
EES_OP_ADD            : OP_ADD -> type(OP_ADD);
EES_OP_SUB            : OP_SUB -> type(OP_SUB);
EES_OP_MUL            : OP_MUL -> type(OP_MUL);
EES_OP_DIV            : OP_DIV -> type(OP_DIV);
EES_OP_MOD            : OP_MOD -> type(OP_MOD);
EES_OP_POW            : OP_POW -> type(OP_POW);
EES_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
EES_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
EES_OP_LESS           : OP_LESS -> type(OP_LESS);
EES_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
EES_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
EES_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
EES_OP_IN             : OP_IN -> type(OP_IN);
EES_OP_INC            : OP_INC -> type(OP_INC);
EES_OP_DEC            : OP_DEC -> type(OP_DEC);
EES_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
EES_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
EES_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
EES_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
EES_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
EES_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
EES_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
EES_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
EES_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
EES_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_ES;

ES_WS           : WHITESPACE+ -> skip;
ES_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_S);
ES_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
ES_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
ES_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
ES_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
ES_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
ES_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
ES_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
ES_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
ES_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
ES_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
ES_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
ES_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
ES_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
ES_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
ES_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
ES_LITERAL         : LITERAL -> type(LITERAL);
ES_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
ES_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
ES_OP_ADD            : OP_ADD -> type(OP_ADD);
ES_OP_SUB            : OP_SUB -> type(OP_SUB);
ES_OP_MUL            : OP_MUL -> type(OP_MUL);
ES_OP_DIV            : OP_DIV -> type(OP_DIV);
ES_OP_MOD            : OP_MOD -> type(OP_MOD);
ES_OP_POW            : OP_POW -> type(OP_POW);
ES_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
ES_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
ES_OP_LESS           : OP_LESS -> type(OP_LESS);
ES_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
ES_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
ES_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
ES_OP_IN             : OP_IN -> type(OP_IN);
ES_OP_INC            : OP_INC -> type(OP_INC);
ES_OP_DEC            : OP_DEC -> type(OP_DEC);
ES_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
ES_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
ES_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
ES_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
ES_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
ES_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
ES_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
ES_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
ES_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
ES_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_BP;

BP_WS    : WHITESPACE+ -> skip;
BP_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), popMode;
BP_BADDY : BADDY -> type(BADDY);

// --------------------------------------------------------
mode IN_BEESP;

BEESP_WS    : WHITESPACE+ -> skip;
BEESP_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_EESP);
BEESP_BADDY : BADDY -> type(BADDY);

// --------------------------------------------------------
mode IN_EESP;

EESP_WS           : WHITESPACE+ -> skip;
EESP_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_ESP);
EESP_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
EESP_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
EESP_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
EESP_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
EESP_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
EESP_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
EESP_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
EESP_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
EESP_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
EESP_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
EESP_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
EESP_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
EESP_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
EESP_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
EESP_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
EESP_LITERAL         : LITERAL -> type(LITERAL);
EESP_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
EESP_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
EESP_OP_ADD            : OP_ADD -> type(OP_ADD);
EESP_OP_SUB            : OP_SUB -> type(OP_SUB);
EESP_OP_MUL            : OP_MUL -> type(OP_MUL);
EESP_OP_DIV            : OP_DIV -> type(OP_DIV);
EESP_OP_MOD            : OP_MOD -> type(OP_MOD);
EESP_OP_POW            : OP_POW -> type(OP_POW);
EESP_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
EESP_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
EESP_OP_LESS           : OP_LESS -> type(OP_LESS);
EESP_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
EESP_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
EESP_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
EESP_OP_IN             : OP_IN -> type(OP_IN);
EESP_OP_INC            : OP_INC -> type(OP_INC);
EESP_OP_DEC            : OP_DEC -> type(OP_DEC);
EESP_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
EESP_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
EESP_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
EESP_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
EESP_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
EESP_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
EESP_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
EESP_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
EESP_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
EESP_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_ESP;

ESP_WS           : WHITESPACE+ -> skip;
ESP_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SP);
ESP_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
ESP_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
ESP_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
ESP_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
ESP_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
ESP_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
ESP_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
ESP_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
ESP_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
ESP_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
ESP_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
ESP_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
ESP_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
ESP_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
ESP_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
ESP_LITERAL         : LITERAL -> type(LITERAL);
ESP_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
ESP_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
ESP_OP_ADD            : OP_ADD -> type(OP_ADD);
ESP_OP_SUB            : OP_SUB -> type(OP_SUB);
ESP_OP_MUL            : OP_MUL -> type(OP_MUL);
ESP_OP_DIV            : OP_DIV -> type(OP_DIV);
ESP_OP_MOD            : OP_MOD -> type(OP_MOD);
ESP_OP_POW            : OP_POW -> type(OP_POW);
ESP_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
ESP_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
ESP_OP_LESS           : OP_LESS -> type(OP_LESS);
ESP_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
ESP_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
ESP_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
ESP_OP_IN             : OP_IN -> type(OP_IN);
ESP_OP_INC            : OP_INC -> type(OP_INC);
ESP_OP_DEC            : OP_DEC -> type(OP_DEC);
ESP_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
ESP_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
ESP_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
ESP_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
ESP_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
ESP_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
ESP_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
ESP_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
ESP_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
ESP_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_SSP;

SSP_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SP);
SSP_MC_NOINDEX     : MC_NOINDEX -> type(MESSAGECODE);
SSP_MC_SIMPLE      : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
SSP_MC_COMPUTED_S  : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
SSP_MC_COMPUTED_V  : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
SSP_MC_I           : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
SSP_MC_T           : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
SSP_MC_e           : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
SSP_MC_i           : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
SSP_MC_R           : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
SSP_STRING_ESCAPE  : '##' -> type(STRING);
SSP_STRING_LITERAL : ~[#,]+ -> type(STRING);

// --------------------------------------------------------
mode IN_SP;

SP_POP_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), popMode;
SP_MC_NOINDEX     : MC_NOINDEX -> type(MESSAGECODE);
SP_MC_SIMPLE      : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
SP_MC_COMPUTED_S  : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
SP_MC_COMPUTED_V  : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
SP_MC_I           : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
SP_MC_T           : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
SP_MC_e           : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
SP_MC_i           : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
SP_MC_R           : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
SP_STRING_ESCAPE  : '##' -> type(STRING);
SP_STRING_LITERAL : ~[#,]+ -> type(STRING);

// --------------------------------------------------------
mode IN_VES;

VES_WS              : WHITESPACE+ -> skip;
VES_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_ES);
VES_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
VES_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
VES_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
VES_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
VES_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
VES_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
VES_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
VES_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
VES_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
VES_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
VES_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
VES_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
VES_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
VES_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
VES_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
VES_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
VES_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
VES_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
VES_LITERAL         : LITERAL -> type(LITERAL);
VES_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);

// --------------------------------------------------------
mode IN_VS;

VS_WS              : WHITESPACE+ -> skip;
VS_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_S);
VS_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
VS_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
VS_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
VS_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
VS_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
VS_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
VS_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
VS_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
VS_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
VS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
VS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
VS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
VS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
VS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
VS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
VS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
VS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
VS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
VS_LITERAL         : LITERAL -> type(LITERAL);
VS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);

// --------------------------------------------------------
mode IN_V;

V_WS          : WHITESPACE+ -> skip;
V_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
V_POP_END         : END -> type(END), popMode;
V_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
V_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
V_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
V_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
V_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
V_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
V_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
V_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
V_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
V_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
V_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
V_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
V_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
V_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
V_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
V_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
V_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
V_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
V_LITERAL         : LITERAL -> type(LITERAL);
V_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);

// --------------------------------------------------------
mode IN_IS;

IS_WS         : WHITESPACE+ -> skip;
IS_COMMA      : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_S);
IS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
IS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
IS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
IS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
IS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
IS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
IS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
IS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
IS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
IS_LITERAL         : LITERAL -> type(LITERAL);
IS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);

// --------------------------------------------------------
mode IN_SSS;

SSS_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SS);
SSS_MC_NOINDEX     : MC_NOINDEX -> type(MESSAGECODE);
SSS_MC_SIMPLE      : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
SSS_MC_COMPUTED_S  : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
SSS_MC_COMPUTED_V  : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
SSS_MC_I           : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
SSS_MC_T           : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
SSS_MC_e           : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
SSS_MC_i           : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
SSS_MC_R           : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
SSS_STRING_ESCAPE  : '##' -> type(STRING);
SSS_STRING_LITERAL : ~[#,]+ -> type(STRING);

// --------------------------------------------------------
mode IN_SS;

SS_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_S);
SS_MC_NOINDEX     : MC_NOINDEX -> type(MESSAGECODE);
SS_MC_SIMPLE      : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
SS_MC_COMPUTED_S  : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
SS_MC_COMPUTED_V  : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
SS_MC_I           : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
SS_MC_T           : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
SS_MC_e           : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
SS_MC_i           : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
SS_MC_R           : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
SS_STRING_ESCAPE  : '##' -> type(STRING);
SS_STRING_LITERAL : ~[#,]+ -> type(STRING);

// --------------------------------------------------------
mode IN_S;

S_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
S_POP_END         : END -> type(END), popMode;
S_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
S_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
S_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
S_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
S_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
S_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
S_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
S_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
S_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
S_STRING_ESCAPE   : '##' -> type(STRING);
S_STRING_LITERAL  : ~[#};]+ -> type(STRING);

// --------------------------------------------------------
mode IN_STRINGLIST;

STRINGLIST_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
STRINGLIST_POP_END         : END -> type(END), popMode;
STRINGLIST_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA);
STRINGLIST_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
STRINGLIST_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
STRINGLIST_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
STRINGLIST_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
STRINGLIST_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
STRINGLIST_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
STRINGLIST_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
STRINGLIST_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
STRINGLIST_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
STRINGLIST_STRING_ESCAPE   : '##' -> type(STRING);
STRINGLIST_STRING_LITERAL  : ~[#};,]+ -> type(STRING);

///////////////////////////////////////////////////////////
// FUNCTION PARSING
///////////////////////////////////////////////////////////

// --------------------------------------------------------
mode IN_F_1ESSS;

F_1ESSS_WS               : WHITESPACE+ -> skip;
F_1ESSS_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), mode(IN_F_ESSS);

// --------------------------------------------------------
mode IN_F_1SV;

F_1SV_WS               : WHITESPACE+ -> skip;
F_1SV_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), mode(IN_F_SV);

// --------------------------------------------------------
mode IN_F_1SS;

F_1SS_WS               : WHITESPACE+ -> skip;
F_1SS_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), mode(IN_F_SS);

// --------------------------------------------------------
mode IN_F_1S;

F_1S_WS               : WHITESPACE+ -> skip;
F_1S_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), mode(IN_F_S);

// --------------------------------------------------------
mode IN_F_1V;

F_1V_WS               : WHITESPACE+ -> skip;
F_1V_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), mode(IN_F_V);

// --------------------------------------------------------
mode IN_F_EES;

F_EES_WS           : WHITESPACE+ -> skip;
F_EES_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_ES);
F_EES_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
F_EES_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
F_EES_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
F_EES_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
F_EES_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
F_EES_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
F_EES_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
F_EES_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
F_EES_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
F_EES_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
F_EES_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
F_EES_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
F_EES_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
F_EES_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
F_EES_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
F_EES_LITERAL         : LITERAL -> type(LITERAL);
F_EES_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
F_EES_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
F_EES_OP_ADD            : OP_ADD -> type(OP_ADD);
F_EES_OP_SUB            : OP_SUB -> type(OP_SUB);
F_EES_OP_MUL            : OP_MUL -> type(OP_MUL);
F_EES_OP_DIV            : OP_DIV -> type(OP_DIV);
F_EES_OP_MOD            : OP_MOD -> type(OP_MOD);
F_EES_OP_POW            : OP_POW -> type(OP_POW);
F_EES_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
F_EES_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
F_EES_OP_LESS           : OP_LESS -> type(OP_LESS);
F_EES_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
F_EES_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
F_EES_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
F_EES_OP_IN             : OP_IN -> type(OP_IN);
F_EES_OP_INC            : OP_INC -> type(OP_INC);
F_EES_OP_DEC            : OP_DEC -> type(OP_DEC);
F_EES_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
F_EES_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
F_EES_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
F_EES_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_EES_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
F_EES_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
F_EES_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
F_EES_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
F_EES_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
F_EES_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_F_ES;

F_ES_WS           : WHITESPACE+ -> skip;
F_ES_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_S);
F_ES_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
F_ES_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
F_ES_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
F_ES_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
F_ES_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
F_ES_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
F_ES_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
F_ES_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
F_ES_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
F_ES_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
F_ES_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
F_ES_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
F_ES_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
F_ES_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
F_ES_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
F_ES_LITERAL         : LITERAL -> type(LITERAL);
F_ES_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
F_ES_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
F_ES_OP_ADD            : OP_ADD -> type(OP_ADD);
F_ES_OP_SUB            : OP_SUB -> type(OP_SUB);
F_ES_OP_MUL            : OP_MUL -> type(OP_MUL);
F_ES_OP_DIV            : OP_DIV -> type(OP_DIV);
F_ES_OP_MOD            : OP_MOD -> type(OP_MOD);
F_ES_OP_POW            : OP_POW -> type(OP_POW);
F_ES_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
F_ES_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
F_ES_OP_LESS           : OP_LESS -> type(OP_LESS);
F_ES_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
F_ES_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
F_ES_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
F_ES_OP_IN             : OP_IN -> type(OP_IN);
F_ES_OP_INC            : OP_INC -> type(OP_INC);
F_ES_OP_DEC            : OP_DEC -> type(OP_DEC);
F_ES_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
F_ES_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
F_ES_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
F_ES_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_ES_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
F_ES_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
F_ES_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
F_ES_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
F_ES_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
F_ES_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_F_ESSS;

F_ESSS_WS           : WHITESPACE+ -> skip;
F_ESSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_SSS);
F_ESSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
F_ESSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
F_ESSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
F_ESSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
F_ESSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
F_ESSS_FUNC_GROUP_6 : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
F_ESSS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
F_ESSS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
F_ESSS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
F_ESSS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
F_ESSS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
F_ESSS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
F_ESSS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
F_ESSS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
F_ESSS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
F_ESSS_LITERAL         : LITERAL -> type(LITERAL);
F_ESSS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
F_ESSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
F_ESSS_OP_ADD            : OP_ADD -> type(OP_ADD);
F_ESSS_OP_SUB            : OP_SUB -> type(OP_SUB);
F_ESSS_OP_MUL            : OP_MUL -> type(OP_MUL);
F_ESSS_OP_DIV            : OP_DIV -> type(OP_DIV);
F_ESSS_OP_MOD            : OP_MOD -> type(OP_MOD);
F_ESSS_OP_POW            : OP_POW -> type(OP_POW);
F_ESSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
F_ESSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
F_ESSS_OP_LESS           : OP_LESS -> type(OP_LESS);
F_ESSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
F_ESSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
F_ESSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
F_ESSS_OP_IN             : OP_IN -> type(OP_IN);
F_ESSS_OP_INC            : OP_INC -> type(OP_INC);
F_ESSS_OP_DEC            : OP_DEC -> type(OP_DEC);
F_ESSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
F_ESSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
F_ESSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
F_ESSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_ESSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
F_ESSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
F_ESSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
F_ESSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
F_ESSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
F_ESSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_F_IS;

F_IS_WS         : WHITESPACE+ -> skip;
F_IS_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_IS_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_S);
F_IS_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
F_IS_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
F_IS_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
F_IS_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
F_IS_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
F_IS_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
F_IS_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
F_IS_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
F_IS_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
F_IS_LITERAL         : LITERAL -> type(LITERAL);
F_IS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);

// --------------------------------------------------------
mode IN_F_VP;

F_VP_WS         : WHITESPACE+ -> skip;
F_VP_POP_PAREN_RIGHT  : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_VP_COMMA            : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_P);
F_VP_MC_NOINDEX       : MC_NOINDEX -> type(MESSAGECODE);
F_VP_MC_SIMPLE        : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_VP_MC_COMPUTED_S    : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_VP_MC_COMPUTED_V    : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_VP_MC_I             : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
F_VP_MC_T             : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_VP_MC_e             : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
F_VP_MC_i             : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_VP_MC_R             : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_VP_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
F_VP_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
F_VP_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
F_VP_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
F_VP_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
F_VP_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
F_VP_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
F_VP_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
F_VP_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
F_VP_LITERAL         : LITERAL -> type(LITERAL);
F_VP_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);

// --------------------------------------------------------
mode IN_F_V;

F_V_WS              : WHITESPACE+ -> skip;
F_V_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_V_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
F_V_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_V_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_V_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_V_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
F_V_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_V_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
F_V_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_V_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_V_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
F_V_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
F_V_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
F_V_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
F_V_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
F_V_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
F_V_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
F_V_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
F_V_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
F_V_LITERAL         : LITERAL -> type(LITERAL);
F_V_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
F_V_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_V_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);

// --------------------------------------------------------
mode IN_F_SO;

F_SO_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_SO_POP_COMMA       : TOKEN_COMMA -> type(TOKEN_COMMA), popMode;
F_SO_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
F_SO_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_SO_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_SO_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_SO_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
F_SO_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_SO_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
F_SO_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_SO_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_SO_STRING_ESCAPE   : '##' -> type(STRING);
F_SO_STRING_LITERAL  :  ~[#),]+ -> type(STRING);

// --------------------------------------------------------
mode IN_F_SV;

F_SV_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_SV_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_V);
F_SV_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
F_SV_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_SV_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_SV_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_SV_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
F_SV_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_SV_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
F_SV_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_SV_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_SV_STRING_ESCAPE   : '##' -> type(STRING);
F_SV_STRING_LITERAL  : ~[#),]+ -> type(STRING);

// --------------------------------------------------------
mode IN_F_SSS;

F_SSS_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_SSS_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_SS);
F_SSS_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
F_SSS_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_SSS_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_SSS_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_SSS_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_IS);
F_SSS_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_SSS_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
F_SSS_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_SSS_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_SSS_STRING_ESCAPE   : '##' -> type(STRING);
F_SSS_STRING_LITERAL  : ~[#),]+ -> type(STRING);

// --------------------------------------------------------
mode IN_F_SS;

F_SS_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_SS_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_S);
F_SS_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
F_SS_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_SS_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_SS_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_SS_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
F_SS_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_SS_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
F_SS_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_SS_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_SS_STRING_ESCAPE   : '##' -> type(STRING);
F_SS_STRING_LITERAL  : ~[#),]+ -> type(STRING);

// --------------------------------------------------------
mode IN_F_S;

F_S_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_S_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
F_S_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_S_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_S_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_S_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
F_S_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_S_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
F_S_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_S_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_S_STRING_ESCAPE   : '##' -> type(STRING);
F_S_STRING_LITERAL  : ~[#)]+ -> type(STRING);

// --------------------------------------------------------
mode IN_F_P;

F_P_WS           : WHITESPACE+ -> skip;
F_P_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_P_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA);
F_P_FUNC_GROUP_1    : FUNC_GROUP_1 -> type(FUNCTION);
F_P_FUNC_GROUP_2    : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
F_P_FUNC_GROUP_3    : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
F_P_FUNC_GROUP_4    : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1ESSS);
F_P_FUNC_GROUP_5    : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SV);
F_P_FUNC_GROUP_6    : FUNC_GROUP_6 -> type(FUNCTION), pushMode(IN_F_1V);
F_P_STORAGE_THIS    : STORAGE_THIS -> type(STORAGE_THIS);
F_P_STORAGE_THISO   : STORAGE_THISO -> type(STORAGE_THISO);
F_P_STORAGE_CLIENT  : STORAGE_CLIENT -> type(STORAGE_CLIENT);
F_P_STORAGE_CLIENTR : STORAGE_CLIENTR -> type(STORAGE_CLIENTR);
F_P_STORAGE_SERVER  : STORAGE_SERVER -> type(STORAGE_SERVER);
F_P_STORAGE_SERVERR : STORAGE_SERVERR -> type(STORAGE_SERVERR);
F_P_STORAGE_LEVEL   : STORAGE_LEVEL -> type(STORAGE_LEVEL);
F_P_STORAGE_LOCAL   : STORAGE_LOCAL -> type(STORAGE_LOCAL);
F_P_STORAGE_TEMP    : STORAGE_TEMP -> type(STORAGE_TEMP);
F_P_LITERAL         : LITERAL -> type(LITERAL);
F_P_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
F_P_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
F_P_OP_ADD            : OP_ADD -> type(OP_ADD);
F_P_OP_SUB            : OP_SUB -> type(OP_SUB);
F_P_OP_MUL            : OP_MUL -> type(OP_MUL);
F_P_OP_DIV            : OP_DIV -> type(OP_DIV);
F_P_OP_MOD            : OP_MOD -> type(OP_MOD);
F_P_OP_POW            : OP_POW -> type(OP_POW);
F_P_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
F_P_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
F_P_OP_LESS           : OP_LESS -> type(OP_LESS);
F_P_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
F_P_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
F_P_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
F_P_OP_IN             : OP_IN -> type(OP_IN);
F_P_OP_INC            : OP_INC -> type(OP_INC);
F_P_OP_DEC            : OP_DEC -> type(OP_DEC);
F_P_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
F_P_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
F_P_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
F_P_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_P_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
F_P_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), pushMode(IN_F_P);
F_P_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
F_P_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
F_P_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_F_STRINGLIST;

F_STRINGLIST_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_STRINGLIST_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA);
F_STRINGLIST_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
F_STRINGLIST_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_STRINGLIST_MC_COMPUTED_S   : MC_COMPUTED_S { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_STRINGLIST_MC_COMPUTED_V   : MC_COMPUTED_V { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_V);
F_STRINGLIST_MC_I            : MC_I          { emitIdentifier(GS1Lexer::IDENTIFIER, getText()); } -> type(MESSAGECODE), pushMode(IN_F_VP);
F_STRINGLIST_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_STRINGLIST_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_EES);
F_STRINGLIST_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_STRINGLIST_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_STRINGLIST_STRING_ESCAPE   : '##' -> type(STRING);
F_STRINGLIST_STRING_LITERAL  : ~[#),]+ -> type(STRING);
