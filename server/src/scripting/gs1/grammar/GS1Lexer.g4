lexer grammar GS1Lexer;

@lexer::header
{
// --------------------------------------------------------
#include <array>
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
    "putexplosion",
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

tokens { COMMAND, FUNCTION, MESSAGECODE, STRING, BADDY, COLOR }

/*
	Mode parameter argument guide:
	- V  variable
	- C  computed identifier
	- S  string
	- P  parameters (any number of normal parameter types)
	- O  optional parameters
	- M  message code
	- B  baddy name
	- 1  left parenthesis (for starting function arguments)
	- ITEM    item name
	- COLOR   color name
	- GENDER  gender name
	- CARRY   carry item name
	- DIRECTION   direction name or number
	- STRINGLIST  variable length comma-separated string list
*/

CMD_SET                  : 'set '                 -> type(COMMAND);
CMD_UNSET                : 'unset '               -> type(COMMAND);
CMD_SLEEP                : 'sleep'                -> type(COMMAND);
CMD_SETARRAY             : 'setarray'             -> type(COMMAND);
CMD_SETSTRING            : 'setstring'            -> type(COMMAND), pushMode(IN_CS);
CMD_TIMEREVERYWHERE      : 'timereverywhere'      -> type(COMMAND);
CMD_ADDSTRING            : 'addstring'            -> type(COMMAND), pushMode(IN_CS);
CMD_INSERTSTRING         : 'insertstring'         -> type(COMMAND), pushMode(IN_CVS);
CMD_REPLACESTRING        : 'replacestring'        -> type(COMMAND), pushMode(IN_CVS);
CMD_REMOVESTRING         : 'removestring'         -> type(COMMAND), pushMode(IN_CS);
CMD_DELETESTRING         : 'deletestring'         -> type(COMMAND);
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
CMD_TRIGGERACTION        : 'triggeraction'        -> type(COMMAND), pushMode(IN_VVSS);
CMD_PUTNPC               : 'putnpc'               -> type(COMMAND), pushMode(IN_SSP);
CMD_PUTNPC2              : 'putnpc2'              -> type(COMMAND), pushMode(IN_SSP);
CMD_CALLNPC              : 'callnpc'              -> type(COMMAND), pushMode(IN_VS);
CMD_CALLWEAPON           : 'callweapon'           -> type(COMMAND), pushMode(IN_VS);
CMD_DESTROY              : 'destroy'              -> type(COMMAND);
CMD_CARRYOBJECT          : 'carryobject'          -> type(COMMAND), pushMode(IN_CARRY);
CMD_THROWCARRY           : 'throwcarry'           -> type(COMMAND);
CMD_FOLLOWPLAYER         : 'followplayer'         -> type(COMMAND);
CMD_TOINVENTORY          : 'toinventory'          -> type(COMMAND);
CMD_TOWEAPONS            : 'toweapons'            -> type(COMMAND), pushMode(IN_S);
CMD_SETCOLOREFFECT       : 'setcoloreffect'       -> type(COMMAND);
CMD_SETZOOMEFFECT        : 'setzoomeffect'        -> type(COMMAND);
CMD_SHOWIMG              : 'showimg'              -> type(COMMAND), pushMode(IN_VSP);
CMD_SHOWIMG2             : 'showimg2'             -> type(COMMAND), pushMode(IN_VSP);
CMD_SHOWANI              : 'showani'              -> type(COMMAND), pushMode(IN_VSP);
CMD_SHOWANI2             : 'showani2'             -> type(COMMAND), pushMode(IN_VSP);
CMD_SHOWPOLY             : 'showpoly'             -> type(COMMAND);
CMD_SHOWPOLY2            : 'showpoly2'            -> type(COMMAND);
CMD_SHOWTEXT             : 'showtext'             -> type(COMMAND), pushMode(IN_VVVSSS);
CMD_SHOWTEXT2            : 'showtext2'            -> type(COMMAND), pushMode(IN_VVVVSSS);
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
CMD_WRAPTEXT             : 'wraptext'             -> type(COMMAND), pushMode(IN_VSS);
CMD_WRAPTEXT2            : 'wraptext2'            -> type(COMMAND), pushMode(IN_VVSS);
CMD_SETSHOOTPARAMS       : 'setshootparams'       -> type(COMMAND), pushMode(IN_STRINGLIST);
CMD_SHOOT                : 'shoot'                -> type(COMMAND), pushMode(IN_VVVVVSS);
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
CMD_ENABLEFEATURES       : 'enablefeatures'       -> type(COMMAND), pushMode(IN_ENABLEFEATURES);
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
CMD_PUTNEWCOMP           : 'putnewcomp'           -> type(COMMAND), pushMode(IN_BVVSP);
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
		| 'sarraylen'
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

FUNC_GROUP_4 : ('textwidth' | 'textheight') -> type(FUNCTION), pushMode(IN_F_1VSSS);
FUNC_GROUP_5 : 'lindexof' -> type(FUNCTION), pushMode(IN_F_1SP);

MC_NOINDEX	: '#' ([ngcmWw1235678LFfpbD] | 'C' [01234] | 'P1' DIGITS? | 'P2' DIGITS? | 'P3' '0'? | 'P' [456789]) -> type(MESSAGECODE);
MC_SIMPLE	: '#' ([angcmWw1235678ptKkGNQ] | 'C' [01234] | 'P1' DIGITS? | 'P2' DIGITS? | 'P3' '0'? | 'P' [456789]) WS* '(' -> type(MESSAGECODE), pushMode(IN_F_P);
MC_COMPUTED : '#' [sv] WS* '(' -> type(MESSAGECODE), pushMode(IN_F_C);
MC_T        : '#T' WS* '(' -> type(MESSAGECODE), pushMode(IN_F_S);
MC_e		: '#e' WS* '(' -> type(MESSAGECODE), pushMode(IN_F_VVS);
MC_I		: '#I' WS* '(' -> type(MESSAGECODE), pushMode(IN_F_CP);
MC_i		: '#i' WS* '(' -> type(MESSAGECODE), pushMode(IN_F_SO);
MC_R		: '#R' WS* '(' -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);

// TODO: Some string lists are CSV.

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
OP_ASSIGN2      : ':=';
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
OP_LESS			: '<';
OP_GREAT		: '>';
OP_LESS_EQ		: '<=';
OP_GREAT_EQ		: '>=';
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

IDENTIFIER
	: [a-zA-Z_][a-zA-Z0-9_]* ('.' IDENTIFIER)? {!isBuiltInCommand(getText())}?
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
MS_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
MS_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
MS_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
MS_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
MS_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
MS_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
MS_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
MS_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
MS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
MS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
MS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
MS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
MS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
MS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
MS_LITERAL      : LITERAL -> type(LITERAL);
MS_OP_ADD            : OP_ADD -> type(OP_ADD);
MS_OP_SUB            : OP_SUB -> type(OP_SUB);
MS_OP_MUL            : OP_MUL -> type(OP_MUL);
MS_OP_DIV            : OP_DIV -> type(OP_DIV);
MS_OP_MOD            : OP_MOD -> type(OP_MOD);
MS_OP_POW            : OP_POW -> type(OP_POW);
MS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
MS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
MS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
MS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);

// --------------------------------------------------------
mode IN_ENABLEFEATURES;

ENABLEFEATURES_WS              : WHITESPACE+ -> skip;
ENABLEFEATURES_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
ENABLEFEATURES_POP_END         : END -> type(END), popMode;
ENABLEFEATURES_FUNC_GROUP_1    : FUNC_GROUP_1 -> type(FUNCTION);
ENABLEFEATURES_FUNC_GROUP_2    : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
ENABLEFEATURES_FUNC_GROUP_3    : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
ENABLEFEATURES_FUNC_GROUP_4    : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
ENABLEFEATURES_FUNC_GROUP_5    : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
ENABLEFEATURES_LITERAL         : LITERAL -> type(LITERAL);
ALLFEATURES                    : 'allfeatures';
ENABLEFEATURES_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
ENABLEFEATURES_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
ENABLEFEATURES_OP_ADD            : OP_ADD -> type(OP_ADD);
ENABLEFEATURES_OP_SUB            : OP_SUB -> type(OP_SUB);
ENABLEFEATURES_OP_MUL            : OP_MUL -> type(OP_MUL);
ENABLEFEATURES_OP_DIV            : OP_DIV -> type(OP_DIV);
ENABLEFEATURES_OP_MOD            : OP_MOD -> type(OP_MOD);
ENABLEFEATURES_OP_POW            : OP_POW -> type(OP_POW);
ENABLEFEATURES_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
ENABLEFEATURES_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
ENABLEFEATURES_OP_LESS           : OP_LESS -> type(OP_LESS);
ENABLEFEATURES_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
ENABLEFEATURES_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
ENABLEFEATURES_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
ENABLEFEATURES_OP_IN             : OP_IN -> type(OP_IN);
ENABLEFEATURES_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
ENABLEFEATURES_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
ENABLEFEATURES_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
ENABLEFEATURES_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
ENABLEFEATURES_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
ENABLEFEATURES_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
ENABLEFEATURES_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
ENABLEFEATURES_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
ENABLEFEATURES_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VVVVSSS;

VVVVSSS_WS           : WHITESPACE+ -> skip;
VVVVSSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VVVSSS);
VVVVSSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VVVVSSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VVVVSSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VVVVSSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VVVVSSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VVVVSSS_LITERAL      : LITERAL -> type(LITERAL);
VVVVSSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VVVVSSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VVVVSSS_OP_ADD            : OP_ADD -> type(OP_ADD);
VVVVSSS_OP_SUB            : OP_SUB -> type(OP_SUB);
VVVVSSS_OP_MUL            : OP_MUL -> type(OP_MUL);
VVVVSSS_OP_DIV            : OP_DIV -> type(OP_DIV);
VVVVSSS_OP_MOD            : OP_MOD -> type(OP_MOD);
VVVVSSS_OP_POW            : OP_POW -> type(OP_POW);
VVVVSSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VVVVSSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VVVVSSS_OP_LESS           : OP_LESS -> type(OP_LESS);
VVVVSSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VVVVSSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VVVVSSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VVVVSSS_OP_IN             : OP_IN -> type(OP_IN);
VVVVSSS_OP_INC            : OP_INC -> type(OP_INC);
VVVVSSS_OP_DEC            : OP_DEC -> type(OP_DEC);
VVVVSSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VVVVSSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VVVVSSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VVVVSSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VVVVSSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VVVVSSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VVVVSSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VVVVSSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VVVVSSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VVVVSSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VVVSSS;

VVVSSS_WS           : WHITESPACE+ -> skip;
VVVSSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VVSSS);
VVVSSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VVVSSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VVVSSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VVVSSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VVVSSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VVVSSS_LITERAL      : LITERAL -> type(LITERAL);
VVVSSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VVVSSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VVVSSS_OP_ADD            : OP_ADD -> type(OP_ADD);
VVVSSS_OP_SUB            : OP_SUB -> type(OP_SUB);
VVVSSS_OP_MUL            : OP_MUL -> type(OP_MUL);
VVVSSS_OP_DIV            : OP_DIV -> type(OP_DIV);
VVVSSS_OP_MOD            : OP_MOD -> type(OP_MOD);
VVVSSS_OP_POW            : OP_POW -> type(OP_POW);
VVVSSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VVVSSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VVVSSS_OP_LESS           : OP_LESS -> type(OP_LESS);
VVVSSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VVVSSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VVVSSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VVVSSS_OP_IN             : OP_IN -> type(OP_IN);
VVVSSS_OP_INC            : OP_INC -> type(OP_INC);
VVVSSS_OP_DEC            : OP_DEC -> type(OP_DEC);
VVVSSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VVVSSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VVVSSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VVVSSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VVVSSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VVVSSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VVVSSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VVVSSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VVVSSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VVVSSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VVSSS;

VVSSS_WS           : WHITESPACE+ -> skip;
VVSSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VSSS);
VVSSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VVSSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VVSSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VVSSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VVSSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VVSSS_LITERAL      : LITERAL -> type(LITERAL);
VVSSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VVSSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VVSSS_OP_ADD            : OP_ADD -> type(OP_ADD);
VVSSS_OP_SUB            : OP_SUB -> type(OP_SUB);
VVSSS_OP_MUL            : OP_MUL -> type(OP_MUL);
VVSSS_OP_DIV            : OP_DIV -> type(OP_DIV);
VVSSS_OP_MOD            : OP_MOD -> type(OP_MOD);
VVSSS_OP_POW            : OP_POW -> type(OP_POW);
VVSSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VVSSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VVSSS_OP_LESS           : OP_LESS -> type(OP_LESS);
VVSSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VVSSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VVSSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VVSSS_OP_IN             : OP_IN -> type(OP_IN);
VVSSS_OP_INC            : OP_INC -> type(OP_INC);
VVSSS_OP_DEC            : OP_DEC -> type(OP_DEC);
VVSSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VVSSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VVSSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VVSSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VVSSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VVSSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VVSSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VVSSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VVSSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VVSSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VSSS;

VSSS_WS           : WHITESPACE+ -> skip;
VSSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SSS);
VSSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VSSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VSSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VSSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VSSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VSSS_LITERAL      : LITERAL -> type(LITERAL);
VSSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VSSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VSSS_OP_ADD            : OP_ADD -> type(OP_ADD);
VSSS_OP_SUB            : OP_SUB -> type(OP_SUB);
VSSS_OP_MUL            : OP_MUL -> type(OP_MUL);
VSSS_OP_DIV            : OP_DIV -> type(OP_DIV);
VSSS_OP_MOD            : OP_MOD -> type(OP_MOD);
VSSS_OP_POW            : OP_POW -> type(OP_POW);
VSSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VSSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VSSS_OP_LESS           : OP_LESS -> type(OP_LESS);
VSSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VSSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VSSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VSSS_OP_IN             : OP_IN -> type(OP_IN);
VSSS_OP_INC            : OP_INC -> type(OP_INC);
VSSS_OP_DEC            : OP_DEC -> type(OP_DEC);
VSSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VSSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VSSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VSSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VSSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VSSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VSSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VSSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VSSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VSSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VVVVVSS;

VVVVVSS_WS           : WHITESPACE+ -> skip;
VVVVVSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VVVVSS);
VVVVVSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VVVVVSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VVVVVSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VVVVVSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VVVVVSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VVVVVSS_LITERAL      : LITERAL -> type(LITERAL);
VVVVVSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VVVVVSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VVVVVSS_OP_ADD            : OP_ADD -> type(OP_ADD);
VVVVVSS_OP_SUB            : OP_SUB -> type(OP_SUB);
VVVVVSS_OP_MUL            : OP_MUL -> type(OP_MUL);
VVVVVSS_OP_DIV            : OP_DIV -> type(OP_DIV);
VVVVVSS_OP_MOD            : OP_MOD -> type(OP_MOD);
VVVVVSS_OP_POW            : OP_POW -> type(OP_POW);
VVVVVSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VVVVVSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VVVVVSS_OP_LESS           : OP_LESS -> type(OP_LESS);
VVVVVSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VVVVVSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VVVVVSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VVVVVSS_OP_IN             : OP_IN -> type(OP_IN);
VVVVVSS_OP_INC            : OP_INC -> type(OP_INC);
VVVVVSS_OP_DEC            : OP_DEC -> type(OP_DEC);
VVVVVSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VVVVVSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VVVVVSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VVVVVSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VVVVVSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VVVVVSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VVVVVSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VVVVVSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VVVVVSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VVVVVSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VVVVSS;

VVVVSS_WS           : WHITESPACE+ -> skip;
VVVVSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VVVSS);
VVVVSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VVVVSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VVVVSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VVVVSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VVVVSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VVVVSS_LITERAL      : LITERAL -> type(LITERAL);
VVVVSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VVVVSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VVVVSS_OP_ADD            : OP_ADD -> type(OP_ADD);
VVVVSS_OP_SUB            : OP_SUB -> type(OP_SUB);
VVVVSS_OP_MUL            : OP_MUL -> type(OP_MUL);
VVVVSS_OP_DIV            : OP_DIV -> type(OP_DIV);
VVVVSS_OP_MOD            : OP_MOD -> type(OP_MOD);
VVVVSS_OP_POW            : OP_POW -> type(OP_POW);
VVVVSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VVVVSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VVVVSS_OP_LESS           : OP_LESS -> type(OP_LESS);
VVVVSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VVVVSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VVVVSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VVVVSS_OP_IN             : OP_IN -> type(OP_IN);
VVVVSS_OP_INC            : OP_INC -> type(OP_INC);
VVVVSS_OP_DEC            : OP_DEC -> type(OP_DEC);
VVVVSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VVVVSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VVVVSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VVVVSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VVVVSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VVVVSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VVVVSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VVVVSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VVVVSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VVVVSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VVVSS;

VVVSS_WS           : WHITESPACE+ -> skip;
VVVSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VVSS);
VVVSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VVVSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VVVSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VVVSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VVVSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VVVSS_LITERAL      : LITERAL -> type(LITERAL);
VVVSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VVVSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VVVSS_OP_ADD            : OP_ADD -> type(OP_ADD);
VVVSS_OP_SUB            : OP_SUB -> type(OP_SUB);
VVVSS_OP_MUL            : OP_MUL -> type(OP_MUL);
VVVSS_OP_DIV            : OP_DIV -> type(OP_DIV);
VVVSS_OP_MOD            : OP_MOD -> type(OP_MOD);
VVVSS_OP_POW            : OP_POW -> type(OP_POW);
VVVSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VVVSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VVVSS_OP_LESS           : OP_LESS -> type(OP_LESS);
VVVSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VVVSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VVVSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VVVSS_OP_IN             : OP_IN -> type(OP_IN);
VVVSS_OP_INC            : OP_INC -> type(OP_INC);
VVVSS_OP_DEC            : OP_DEC -> type(OP_DEC);
VVVSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VVVSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VVVSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VVVSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VVVSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VVVSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VVVSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VVVSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VVVSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VVVSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VVSS;

VVSS_WS           : WHITESPACE+ -> skip;
VVSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VSS);
VVSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VVSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VVSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VVSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VVSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VVSS_LITERAL      : LITERAL -> type(LITERAL);
VVSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VVSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VVSS_OP_ADD            : OP_ADD -> type(OP_ADD);
VVSS_OP_SUB            : OP_SUB -> type(OP_SUB);
VVSS_OP_MUL            : OP_MUL -> type(OP_MUL);
VVSS_OP_DIV            : OP_DIV -> type(OP_DIV);
VVSS_OP_MOD            : OP_MOD -> type(OP_MOD);
VVSS_OP_POW            : OP_POW -> type(OP_POW);
VVSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VVSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VVSS_OP_LESS           : OP_LESS -> type(OP_LESS);
VVSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VVSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VVSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VVSS_OP_IN             : OP_IN -> type(OP_IN);
VVSS_OP_INC            : OP_INC -> type(OP_INC);
VVSS_OP_DEC            : OP_DEC -> type(OP_DEC);
VVSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VVSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VVSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VVSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VVSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VVSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VVSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VVSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VVSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VVSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VSS;

VSS_WS           : WHITESPACE+ -> skip;
VSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SS);
VSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VSS_LITERAL      : LITERAL -> type(LITERAL);
VSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VSS_OP_ADD            : OP_ADD -> type(OP_ADD);
VSS_OP_SUB            : OP_SUB -> type(OP_SUB);
VSS_OP_MUL            : OP_MUL -> type(OP_MUL);
VSS_OP_DIV            : OP_DIV -> type(OP_DIV);
VSS_OP_MOD            : OP_MOD -> type(OP_MOD);
VSS_OP_POW            : OP_POW -> type(OP_POW);
VSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VSS_OP_LESS           : OP_LESS -> type(OP_LESS);
VSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VSS_OP_IN             : OP_IN -> type(OP_IN);
VSS_OP_INC            : OP_INC -> type(OP_INC);
VSS_OP_DEC            : OP_DEC -> type(OP_DEC);
VSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VVS;

VVS_WS           : WHITESPACE+ -> skip;
VVS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VS);
VVS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VVS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VVS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VVS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VVS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VVS_LITERAL      : LITERAL -> type(LITERAL);
VVS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VVS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VVS_OP_ADD            : OP_ADD -> type(OP_ADD);
VVS_OP_SUB            : OP_SUB -> type(OP_SUB);
VVS_OP_MUL            : OP_MUL -> type(OP_MUL);
VVS_OP_DIV            : OP_DIV -> type(OP_DIV);
VVS_OP_MOD            : OP_MOD -> type(OP_MOD);
VVS_OP_POW            : OP_POW -> type(OP_POW);
VVS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VVS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VVS_OP_LESS           : OP_LESS -> type(OP_LESS);
VVS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VVS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VVS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VVS_OP_IN             : OP_IN -> type(OP_IN);
VVS_OP_INC            : OP_INC -> type(OP_INC);
VVS_OP_DEC            : OP_DEC -> type(OP_DEC);
VVS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VVS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VVS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VVS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VVS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VVS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VVS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VVS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VVS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VVS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VS;

VS_WS           : WHITESPACE+ -> skip;
VS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_S);
VS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VS_LITERAL      : LITERAL -> type(LITERAL);
VS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VS_OP_ADD            : OP_ADD -> type(OP_ADD);
VS_OP_SUB            : OP_SUB -> type(OP_SUB);
VS_OP_MUL            : OP_MUL -> type(OP_MUL);
VS_OP_DIV            : OP_DIV -> type(OP_DIV);
VS_OP_MOD            : OP_MOD -> type(OP_MOD);
VS_OP_POW            : OP_POW -> type(OP_POW);
VS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VS_OP_LESS           : OP_LESS -> type(OP_LESS);
VS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VS_OP_IN             : OP_IN -> type(OP_IN);
VS_OP_INC            : OP_INC -> type(OP_INC);
VS_OP_DEC            : OP_DEC -> type(OP_DEC);
VS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_BP;

BP_WS    : WHITESPACE+ -> skip;
BP_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), popMode;
BP_BADDY : BADDY -> type(BADDY);

// --------------------------------------------------------
mode IN_BVVSP;

BIISP_WS    : WHITESPACE+ -> skip;
BIISP_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VVSP);
BIISP_BADDY : BADDY -> type(BADDY);

// --------------------------------------------------------
mode IN_VVSP;

VVSP_WS           : WHITESPACE+ -> skip;
VVSP_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VSP);
VVSP_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VVSP_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VVSP_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VVSP_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VVSP_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VVSP_LITERAL      : LITERAL -> type(LITERAL);
VVSP_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VVSP_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VVSP_OP_ADD            : OP_ADD -> type(OP_ADD);
VVSP_OP_SUB            : OP_SUB -> type(OP_SUB);
VVSP_OP_MUL            : OP_MUL -> type(OP_MUL);
VVSP_OP_DIV            : OP_DIV -> type(OP_DIV);
VVSP_OP_MOD            : OP_MOD -> type(OP_MOD);
VVSP_OP_POW            : OP_POW -> type(OP_POW);
VVSP_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VVSP_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VVSP_OP_LESS           : OP_LESS -> type(OP_LESS);
VVSP_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VVSP_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VVSP_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VVSP_OP_IN             : OP_IN -> type(OP_IN);
VVSP_OP_INC            : OP_INC -> type(OP_INC);
VVSP_OP_DEC            : OP_DEC -> type(OP_DEC);
VVSP_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VVSP_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VVSP_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VVSP_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VVSP_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VVSP_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VVSP_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VVSP_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VVSP_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VVSP_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_VSP;

VSP_WS           : WHITESPACE+ -> skip;
VSP_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SP);
VSP_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
VSP_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
VSP_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
VSP_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
VSP_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
VSP_LITERAL      : LITERAL -> type(LITERAL);
VSP_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
VSP_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
VSP_OP_ADD            : OP_ADD -> type(OP_ADD);
VSP_OP_SUB            : OP_SUB -> type(OP_SUB);
VSP_OP_MUL            : OP_MUL -> type(OP_MUL);
VSP_OP_DIV            : OP_DIV -> type(OP_DIV);
VSP_OP_MOD            : OP_MOD -> type(OP_MOD);
VSP_OP_POW            : OP_POW -> type(OP_POW);
VSP_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
VSP_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
VSP_OP_LESS           : OP_LESS -> type(OP_LESS);
VSP_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
VSP_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
VSP_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
VSP_OP_IN             : OP_IN -> type(OP_IN);
VSP_OP_INC            : OP_INC -> type(OP_INC);
VSP_OP_DEC            : OP_DEC -> type(OP_DEC);
VSP_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
VSP_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
VSP_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
VSP_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
VSP_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
VSP_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
VSP_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
VSP_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
VSP_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
VSP_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_SSP;

SSP_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SP);
SSP_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
SSP_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
SSP_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
SSP_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
SSP_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
SSP_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
SSP_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
SSP_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
SSP_STRING_ESCAPE  : '##' -> type(STRING);
SSP_STRING_LITERAL : ~[#,]+ -> type(STRING);

// --------------------------------------------------------
mode IN_SP;

SP_POP_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), popMode;
SP_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
SP_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
SP_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
SP_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
SP_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
SP_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
SP_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
SP_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
SP_STRING_ESCAPE  : '##' -> type(STRING);
SP_STRING_LITERAL : ~[#,]+ -> type(STRING);

// --------------------------------------------------------
mode IN_CVS;

CVS_WS          : WHITESPACE+ -> skip;
CVS_COMMA       : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_VS);
CVS_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
CVS_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
CVS_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
CVS_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
CVS_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
CVS_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
CVS_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
CVS_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
CVS_LITERAL     : LITERAL -> type(LITERAL);
CVS_IDENTIFIER  : IDENTIFIER -> type(IDENTIFIER);
CVS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
CVS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);

// --------------------------------------------------------
mode IN_CS;

CS_WS          : WHITESPACE+ -> skip;
CS_COMMA       : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_S);
CS_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
CS_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
CS_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
CS_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
CS_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
CS_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
CS_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
CS_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
CS_LITERAL     : LITERAL -> type(LITERAL);
CS_IDENTIFIER  : IDENTIFIER -> type(IDENTIFIER);
CS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
CS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);

// --------------------------------------------------------
mode IN_IS;

IS_WS         : WHITESPACE+ -> skip;
IS_COMMA      : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_S);
IS_IDENTIFIER : IDENTIFIER -> type(IDENTIFIER);

// --------------------------------------------------------
mode IN_SSS;

SSS_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_SS);
SSS_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
SSS_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
SSS_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
SSS_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
SSS_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
SSS_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
SSS_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
SSS_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
SSS_STRING_ESCAPE  : '##' -> type(STRING);
SSS_STRING_LITERAL : ~[#,]+ -> type(STRING);

// --------------------------------------------------------
mode IN_SS;

SS_COMMA : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_S);
SS_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
SS_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
SS_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
SS_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
SS_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
SS_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
SS_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
SS_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
SS_STRING_ESCAPE  : '##' -> type(STRING);
SS_STRING_LITERAL : ~[#,]+ -> type(STRING);

// --------------------------------------------------------
mode IN_S;

S_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
S_POP_END         : END -> type(END), popMode;
S_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
S_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
S_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
S_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
S_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
S_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
S_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
S_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
S_STRING_ESCAPE  : '##' -> type(STRING);
S_STRING_LITERAL : ~[#};]+ -> type(STRING);

// --------------------------------------------------------
mode IN_STRINGLIST;

STRINGLIST_POP_BRACE_RIGHT : TOKEN_BRACE_RIGHT -> type(TOKEN_BRACE_RIGHT), popMode;
STRINGLIST_POP_END         : END -> type(END), popMode;
STRINGLIST_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA);
STRINGLIST_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
STRINGLIST_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
STRINGLIST_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
STRINGLIST_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
STRINGLIST_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
STRINGLIST_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
STRINGLIST_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
STRINGLIST_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
STRINGLIST_STRING_ESCAPE  : '##' -> type(STRING);
STRINGLIST_STRING_LITERAL : ~[#};,]+ -> type(STRING);

///////////////////////////////////////////////////////////
// FUNCTION PARSING
///////////////////////////////////////////////////////////

// --------------------------------------------------------
mode IN_F_1VSSS;

F_1VSSS_WS               : WHITESPACE+ -> skip;
F_1VSSS_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), mode(IN_F_VSSS);

// --------------------------------------------------------
mode IN_F_1SP;

F_1SP_WS               : WHITESPACE+ -> skip;
F_1SP_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), mode(IN_F_SO);

// --------------------------------------------------------
mode IN_F_1SS;

F_1SS_WS               : WHITESPACE+ -> skip;
F_1SS_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), mode(IN_F_SS);

// --------------------------------------------------------
mode IN_F_1S;

F_1S_WS               : WHITESPACE+ -> skip;
F_1S_TOKEN_PAREN_LEFT : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT), mode(IN_F_S);

// --------------------------------------------------------
mode IN_F_VVS;

F_VVS_WS           : WHITESPACE+ -> skip;
F_VVS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_VS);
F_VVS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
F_VVS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
F_VVS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
F_VVS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
F_VVS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
F_VVS_LITERAL      : LITERAL -> type(LITERAL);
F_VVS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
F_VVS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
F_VVS_OP_ADD            : OP_ADD -> type(OP_ADD);
F_VVS_OP_SUB            : OP_SUB -> type(OP_SUB);
F_VVS_OP_MUL            : OP_MUL -> type(OP_MUL);
F_VVS_OP_DIV            : OP_DIV -> type(OP_DIV);
F_VVS_OP_MOD            : OP_MOD -> type(OP_MOD);
F_VVS_OP_POW            : OP_POW -> type(OP_POW);
F_VVS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
F_VVS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
F_VVS_OP_LESS           : OP_LESS -> type(OP_LESS);
F_VVS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
F_VVS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
F_VVS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
F_VVS_OP_IN             : OP_IN -> type(OP_IN);
F_VVS_OP_INC            : OP_INC -> type(OP_INC);
F_VVS_OP_DEC            : OP_DEC -> type(OP_DEC);
F_VVS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
F_VVS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
F_VVS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
F_VVS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_VVS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
F_VVS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
F_VVS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
F_VVS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
F_VVS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
F_VVS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_F_VS;

F_VS_WS           : WHITESPACE+ -> skip;
F_VS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_S);
F_VS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
F_VS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
F_VS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
F_VS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
F_VS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
F_VS_LITERAL      : LITERAL -> type(LITERAL);
F_VS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
F_VS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
F_VS_OP_ADD            : OP_ADD -> type(OP_ADD);
F_VS_OP_SUB            : OP_SUB -> type(OP_SUB);
F_VS_OP_MUL            : OP_MUL -> type(OP_MUL);
F_VS_OP_DIV            : OP_DIV -> type(OP_DIV);
F_VS_OP_MOD            : OP_MOD -> type(OP_MOD);
F_VS_OP_POW            : OP_POW -> type(OP_POW);
F_VS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
F_VS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
F_VS_OP_LESS           : OP_LESS -> type(OP_LESS);
F_VS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
F_VS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
F_VS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
F_VS_OP_IN             : OP_IN -> type(OP_IN);
F_VS_OP_INC            : OP_INC -> type(OP_INC);
F_VS_OP_DEC            : OP_DEC -> type(OP_DEC);
F_VS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
F_VS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
F_VS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
F_VS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_VS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
F_VS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
F_VS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
F_VS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
F_VS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
F_VS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_F_VSSS;

F_VSSS_WS           : WHITESPACE+ -> skip;
F_VSSS_COMMA        : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_SSS);
F_VSSS_FUNC_GROUP_1 : FUNC_GROUP_1 -> type(FUNCTION);
F_VSSS_FUNC_GROUP_2 : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
F_VSSS_FUNC_GROUP_3 : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
F_VSSS_FUNC_GROUP_4 : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
F_VSSS_FUNC_GROUP_5 : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
F_VSSS_LITERAL      : LITERAL -> type(LITERAL);
F_VSSS_IDENTIFIER   : IDENTIFIER -> type(IDENTIFIER);
F_VSSS_OP_ASSIGN         : OP_ASSIGN -> type(OP_ASSIGN);
F_VSSS_OP_ADD            : OP_ADD -> type(OP_ADD);
F_VSSS_OP_SUB            : OP_SUB -> type(OP_SUB);
F_VSSS_OP_MUL            : OP_MUL -> type(OP_MUL);
F_VSSS_OP_DIV            : OP_DIV -> type(OP_DIV);
F_VSSS_OP_MOD            : OP_MOD -> type(OP_MOD);
F_VSSS_OP_POW            : OP_POW -> type(OP_POW);
F_VSSS_OP_EQUAL          : OP_EQUAL -> type(OP_EQUAL);
F_VSSS_OP_NOTEQ          : OP_NOTEQ -> type(OP_NOTEQ);
F_VSSS_OP_LESS           : OP_LESS -> type(OP_LESS);
F_VSSS_OP_GREAT          : OP_GREAT -> type(OP_GREAT);
F_VSSS_OP_LESS_EQ        : OP_LESS_EQ -> type(OP_LESS_EQ);
F_VSSS_OP_GREAT_EQ       : OP_GREAT_EQ -> type(OP_GREAT_EQ);
F_VSSS_OP_IN             : OP_IN -> type(OP_IN);
F_VSSS_OP_INC            : OP_INC -> type(OP_INC);
F_VSSS_OP_DEC            : OP_DEC -> type(OP_DEC);
F_VSSS_OP_LOGICALAND     : OP_LOGICALAND -> type(OP_LOGICALAND);
F_VSSS_OP_LOGICALOR      : OP_LOGICALOR -> type(OP_LOGICALOR);
F_VSSS_OP_LOGICALNOT     : OP_LOGICALNOT -> type(OP_LOGICALNOT);
F_VSSS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_VSSS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);
F_VSSS_TOKEN_PAREN_LEFT    : TOKEN_PAREN_LEFT -> type(TOKEN_PAREN_LEFT);
F_VSSS_TOKEN_PAREN_RIGHT   : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT);
F_VSSS_TOKEN_PIPE          : TOKEN_PIPE -> type(TOKEN_PIPE);
F_VSSS_TOKEN_QUESTION      : TOKEN_QUESTION -> type(TOKEN_QUESTION);
F_VSSS_TOKEN_COLON         : TOKEN_COLON -> type(TOKEN_COLON);

// --------------------------------------------------------
mode IN_F_IS;

F_IS_WS         : WHITESPACE+ -> skip;
F_IS_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_IS_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_S);
F_IS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);

// --------------------------------------------------------
mode IN_F_CP;

F_CS_WS         : WHITESPACE+ -> skip;
F_CS_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_CS_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_P);
F_CS_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
F_CS_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_CS_MC_COMPUTED     : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
F_CS_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_CS_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
F_CS_MC_I            : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
F_CS_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_CS_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_CS_LITERAL         : LITERAL -> type(LITERAL);
F_CS_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
F_CS_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_CS_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);


// --------------------------------------------------------
mode IN_F_C;

F_C_WS              : WHITESPACE+ -> skip;
F_C_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_C_MC_NOINDEX      : MC_NOINDEX -> type(MESSAGECODE);
F_C_MC_SIMPLE       : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_C_MC_COMPUTED     : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
F_C_MC_T            : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_C_MC_e            : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
F_C_MC_I            : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
F_C_MC_i            : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_C_MC_R            : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_C_LITERAL         : LITERAL -> type(LITERAL);
F_C_IDENTIFIER      : IDENTIFIER -> type(IDENTIFIER);
F_C_TOKEN_BRACKET_LEFT  : TOKEN_BRACKET_LEFT -> type(TOKEN_BRACKET_LEFT);
F_C_TOKEN_BRACKET_RIGHT : TOKEN_BRACKET_RIGHT -> type(TOKEN_BRACKET_RIGHT);

// --------------------------------------------------------
mode IN_F_SO;

F_SO_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_SO_POP_COMMA       : TOKEN_COMMA -> type(TOKEN_COMMA), popMode;
F_SO_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
F_SO_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_SO_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
F_SO_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_SO_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
F_SO_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
F_SO_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_SO_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_SO_STRING_ESCAPE  : '##' -> type(STRING);
F_SO_STRING_LITERAL :  ~[#),]+ -> type(STRING);

// --------------------------------------------------------
mode IN_F_SSS;

F_SSS_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_SSS_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_SS);
F_SSS_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
F_SSS_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_SSS_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
F_SSS_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_SSS_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
F_SSS_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_IS);
F_SSS_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_SSS_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_SSS_STRING_ESCAPE  : '##' -> type(STRING);
F_SSS_STRING_LITERAL : ~[#),]+ -> type(STRING);

// --------------------------------------------------------
mode IN_F_SS;

F_SS_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_SS_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA), mode(IN_F_S);
F_SS_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
F_SS_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_SS_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
F_SS_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_SS_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
F_SS_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
F_SS_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_SS_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_SS_STRING_ESCAPE  : '##' -> type(STRING);
F_SS_STRING_LITERAL : ~[#),]+ -> type(STRING);

// --------------------------------------------------------
mode IN_F_S;

F_S_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_S_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
F_S_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_S_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
F_S_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_S_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
F_S_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
F_S_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_S_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_S_STRING_ESCAPE  : '##' -> type(STRING);
F_S_STRING_LITERAL : ~[#)]+ -> type(STRING);

// --------------------------------------------------------
mode IN_F_P;

F_P_WS           : WHITESPACE+ -> skip;
F_P_POP_PAREN_RIGHT : TOKEN_PAREN_RIGHT -> type(TOKEN_PAREN_RIGHT), popMode;
F_P_COMMA           : TOKEN_COMMA -> type(TOKEN_COMMA);
F_P_FUNC_GROUP_1    : FUNC_GROUP_1 -> type(FUNCTION);
F_P_FUNC_GROUP_2    : FUNC_GROUP_2 -> type(FUNCTION), pushMode(IN_F_1S);
F_P_FUNC_GROUP_3    : FUNC_GROUP_3 -> type(FUNCTION), pushMode(IN_F_1SS);
F_P_FUNC_GROUP_4    : FUNC_GROUP_4 -> type(FUNCTION), pushMode(IN_F_1VSSS);
F_P_FUNC_GROUP_5    : FUNC_GROUP_5 -> type(FUNCTION), pushMode(IN_F_1SP);
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
F_STRINGLIST_MC_NOINDEX  : MC_NOINDEX -> type(MESSAGECODE);
F_STRINGLIST_MC_SIMPLE   : MC_SIMPLE -> type(MESSAGECODE), pushMode(IN_F_P);
F_STRINGLIST_MC_COMPUTED : MC_COMPUTED -> type(MESSAGECODE), pushMode(IN_F_C);
F_STRINGLIST_MC_T        : MC_T -> type(MESSAGECODE), pushMode(IN_F_S);
F_STRINGLIST_MC_e        : MC_e -> type(MESSAGECODE), pushMode(IN_F_VVS);
F_STRINGLIST_MC_I        : MC_I -> type(MESSAGECODE), pushMode(IN_F_CP);
F_STRINGLIST_MC_i        : MC_i -> type(MESSAGECODE), pushMode(IN_F_SO);
F_STRINGLIST_MC_R        : MC_R -> type(MESSAGECODE), pushMode(IN_F_STRINGLIST);
F_STRINGLIST_STRING_ESCAPE  : '##' -> type(STRING);
F_STRINGLIST_STRING_LITERAL : ~[#),]+ -> type(STRING);
