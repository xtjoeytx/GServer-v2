#include <random>
#include <numbers>
#include <numeric>

#include <common.h>

#include <Server.h>
#include <object/Character.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptContainers.h>
#include <scripting/gs1/GS1Functions.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/StringUtils.h>

using namespace preagonal::grammar::gs1;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

using BuiltInCommandHandleFunc = void(*)(GS1Visitor*, std::string_view, const std::vector<GS1ScriptValue*>&);
using BuiltInCommandHandleMap = std::unordered_map<size_t, BuiltInCommandHandleFunc>;

static void fn_addguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_addstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_addweapon(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_attachplayertoobj(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_blockagain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_canbecarried(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_canbepulled(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_canbepushed(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_cannotbecarried(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_cannotbepulled(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_cannotbepushed(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_cannotwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_canwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_canwarp2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_carryobject(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_copyflagss(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_copylevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_copystrings(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_deletelevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_deletestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_destroy(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_detachplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_disabledefmovement(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_disableweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_dontblock(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_drawaslight(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_drawoverplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_drawunderplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_enabledefmovement(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_enableweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_explodebomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_freezeplayer2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_hide(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_hitcompu(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_hitnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_hitobjects(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_hitplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_hurt(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_insertstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_join(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_lay(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_lay2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_message(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_move(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_noplayeronwall(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_putbomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_putcomp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_putexplosion(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_putexplosion2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_puthorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_putnewcomp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_putnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_putnpc2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removearrow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removebomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removecompus(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removeexplo(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removeguild(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removeguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removeitem(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_removeweapon(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_replacestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_saveinfo(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_savelog(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_savelog2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_say2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_sendpm(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_sendrpgmessage(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_sendtonc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_sendtorc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_serverwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_set(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setarray(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setbeltcolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setbody(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setcharani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setchargender(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setcharprop(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setcoatcolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setgender(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setgif(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_sethead(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setimg(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setimgpart(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setlevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setlevel2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setmap(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setminimap(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setplayerdir(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setplayerprop(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setpm(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setshape(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setshape2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setshield(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setshoecolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setshootparams(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setskincolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setsleevecolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setsword(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_setz(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_shoot(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_shootarrow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_shootball(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_shootfireball(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_shootfireblast(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_shootnuke(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_show(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showani2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showcharacter(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showimg(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showimg2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showstats(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_sleep(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_spyfire(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_take(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_take2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_takehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_takeplayercarry(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_takeplayerhorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_throwcarry(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_timershow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_tokenize(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_tokenize2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_triggeraction(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_unfreezeplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_unset(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_updateboard(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_updateboard2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_updateterrain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);

static BuiltInCommandHandleMap GenerateMap()
{
	string::string_hash hash{};
	BuiltInCommandHandleMap map =
	{
		{ hash("addguildmember"), &fn_addguildmember },
		{ hash("addstring"), &fn_addstring },
		{ hash("addweapon"), &fn_addweapon },
		{ hash("attachplayertoobj"), &fn_attachplayertoobj },
		{ hash("blockagain"), &fn_blockagain },
		{ hash("canbecarried"), &fn_canbecarried },
		{ hash("canbepulled"), &fn_canbepulled },
		{ hash("canbepushed"), &fn_canbepushed },
		{ hash("cannotbecarried"), &fn_cannotbecarried },
		{ hash("cannotbepulled"), &fn_cannotbepulled },
		{ hash("cannotbepushed"), &fn_cannotbepushed },
		{ hash("cannotwarp"), &fn_cannotwarp },
		{ hash("canwarp"), &fn_canwarp },
		{ hash("canwarp2"), &fn_canwarp2 },
		{ hash("carryobject"), &fn_carryobject },
		{ hash("copyflagss"), &fn_copyflagss },
		{ hash("copylevel"), &fn_copylevel },
		{ hash("copystrings"), &fn_copystrings },
		{ hash("deletelevel"), &fn_deletelevel },
		{ hash("deletestring"), &fn_deletestring },
		{ hash("destroy"), &fn_destroy },
		{ hash("detachplayer"), &fn_detachplayer },
		{ hash("disabledefmovement"), &fn_disabledefmovement },
		{ hash("disableweapons"), &fn_disableweapons },
		{ hash("dontblock"), &fn_dontblock },
		{ hash("drawaslight"), &fn_drawaslight },
		{ hash("drawoverplayer"), &fn_drawoverplayer },
		{ hash("drawunderplayer"), &fn_drawunderplayer },
		{ hash("enabledefmovement"), &fn_enabledefmovement },
		{ hash("enableweapons"), &fn_enableweapons },
		{ hash("explodebomb"), &fn_explodebomb },
		{ hash("freezeplayer2"), &fn_freezeplayer2 },
		{ hash("hide"), &fn_hide },
		{ hash("hitcompu"), &fn_hitcompu },
		{ hash("hitnpc"), &fn_hitnpc },
		{ hash("hitobjects"), &fn_hitobjects },
		{ hash("hitplayer"), &fn_hitplayer },
		{ hash("hurt"), &fn_hurt },
		{ hash("insertstring"), &fn_insertstring },
		{ hash("join"), &fn_join },
		{ hash("lay"), &fn_lay },
		{ hash("lay2"), &fn_lay2 },
		{ hash("message"), &fn_message },
		{ hash("move"), &fn_move },
		{ hash("noplayeronwall"), &fn_noplayeronwall },
		{ hash("putbomb"), &fn_putbomb },
		{ hash("putcomp"), &fn_putcomp },
		{ hash("putexplosion"), &fn_putexplosion },
		{ hash("putexplosion2"), &fn_putexplosion2 },
		{ hash("puthorse"), &fn_puthorse },
		{ hash("putnewcomp"), &fn_putnewcomp },
		{ hash("putnpc"), &fn_putnpc },
		{ hash("putnpc2"), &fn_putnpc2 },
		{ hash("removearrow"), &fn_removearrow },
		{ hash("removebomb"), &fn_removebomb },
		{ hash("removecompus"), &fn_removecompus },
		{ hash("removeexplo"), &fn_removeexplo },
		{ hash("removeguild"), &fn_removeguild },
		{ hash("removeguildmember"), &fn_removeguildmember },
		{ hash("removehorse"), &fn_removehorse },
		{ hash("removeitem"), &fn_removeitem },
		{ hash("removestring"), &fn_removestring },
		{ hash("removeweapon"), &fn_removeweapon },
		{ hash("replacestring"), &fn_replacestring },
		{ hash("saveinfo"), &fn_saveinfo },
		{ hash("savelog"), &fn_savelog },
		{ hash("savelog2"), &fn_savelog2 },
		{ hash("say2"), &fn_say2 },
		{ hash("sendpm"), &fn_sendpm },
		{ hash("sendrpgmessage"), &fn_sendrpgmessage },
		{ hash("sendtonc"), &fn_sendtonc },
		{ hash("sendtorc"), &fn_sendtorc },
		{ hash("serverwarp"), &fn_serverwarp },
		{ hash("set"), &fn_set },
		{ hash("setani"), &fn_setani },
		{ hash("setarray"), &fn_setarray },
		{ hash("setbeltcolor"), &fn_setbeltcolor },
		{ hash("setbody"), &fn_setbody },
		{ hash("setcharani"), &fn_setcharani },
		{ hash("setchargender"), &fn_setchargender },
		{ hash("setcharprop"), &fn_setcharprop },
		{ hash("setcoatcolor"), &fn_setcoatcolor },
		{ hash("setgender"), &fn_setgender },
		{ hash("setgif"), &fn_setgif },
		{ hash("sethead"), &fn_sethead },
		{ hash("setimg"), &fn_setimg },
		{ hash("setimgpart"), &fn_setimgpart },
		{ hash("setlevel"), &fn_setlevel },
		{ hash("setlevel2"), &fn_setlevel2 },
		{ hash("setmap"), &fn_setmap },
		{ hash("setminimap"), &fn_setminimap },
		{ hash("setplayerdir"), &fn_setplayerdir },
		{ hash("setplayerprop"), &fn_setplayerprop },
		{ hash("setpm"), &fn_setpm },
		{ hash("setshape"), &fn_setshape },
		{ hash("setshape2"), &fn_setshape2 },
		{ hash("setshield"), &fn_setshield },
		{ hash("setshoecolor"), &fn_setshoecolor },
		{ hash("setshootparams"), &fn_setshootparams },
		{ hash("setskincolor"), &fn_setskincolor },
		{ hash("setsleevecolor"), &fn_setsleevecolor },
		{ hash("setstring"), &fn_setstring },
		{ hash("setsword"), &fn_setsword },
		{ hash("setz"), &fn_setz },
		{ hash("shoot"), &fn_shoot },
		{ hash("shootarrow"), &fn_shootarrow },
		{ hash("shootball"), &fn_shootball },
		{ hash("shootfireball"), &fn_shootfireball },
		{ hash("shootfireblast"), &fn_shootfireblast },
		{ hash("shootnuke"), &fn_shootnuke },
		{ hash("show"), &fn_show },
		{ hash("showani"), &fn_showani },
		{ hash("showani2"), &fn_showani2 },
		{ hash("showcharacter"), &fn_showcharacter },
		{ hash("showimg"), &fn_showimg },
		{ hash("showimg2"), &fn_showimg2 },
		{ hash("showstats"), &fn_showstats },
		{ hash("sleep"), &fn_sleep },
		{ hash("spyfire"), &fn_spyfire },
		{ hash("take"), &fn_take },
		{ hash("take2"), &fn_take2 },
		{ hash("takehorse"), &fn_takehorse },
		{ hash("takeplayercarry"), &fn_takeplayercarry },
		{ hash("takeplayerhorse"), &fn_takeplayerhorse },
		{ hash("throwcarry"), &fn_throwcarry },
		{ hash("timershow"), &fn_timershow },
		{ hash("tokenize"), &fn_tokenize },
		{ hash("tokenize2"), &fn_tokenize2 },
		{ hash("triggeraction"), &fn_triggeraction },
		{ hash("unfreezeplayer"), &fn_unfreezeplayer },
		{ hash("unset"), &fn_unset },
		{ hash("updateboard"), &fn_updateboard },
		{ hash("updateboard2"), &fn_updateboard2 },
		{ hash("updateterrain"), &fn_updateterrain },
	};
	return map;
}

///////////////////////////////////////////////////////////////////////////////

void processBuiltInCommand(preagonal::grammar::gs1::GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	static BuiltInCommandHandleMap map = GenerateMap();

	if (visitor == nullptr)
		throw std::exception("processBuiltInCommand received an empty visitor");
	if (commandName.empty())
		throw std::exception("processBuiltInCommand received an empty command name");

	size_t hash = string::string_hash{}(commandName);
	auto it = map.find(hash);
	if (it != map.end())
		return it->second(visitor, commandName, arguments);

	// TODO(Nalin): Remove this eventually.
	throw std::exception("processBuiltInCommand received an unknown command");
}

///////////////////////////////////////////////////////////////////////////////

// addguildmember guild,account,nick;
void fn_addguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("addguildmember is not implemented yet.");
}

// addstring list,text;
// Adds a string to a string list.
void fn_addstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::exception("addstring requires exactly two arguments: list and text.");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto text = visitor->getGameValueAs<std::string>(*arguments[1]);
		auto& list = listVar->get<std::string>();
		if (list.has_value() && !list.value().empty())
			listVar->assign(list.value() + "," + text);
		else
			listVar->assign(text);
	}
}

// addweapon weaponname;
// Adds a weapon from a database to your inventory.
void fn_addweapon(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->addWeapon(first);
	}
}

// attachplayertoobj objecttype,id;
// Attaches player to object (objecttype 0 = npcs, nothing else supported).
void fn_attachplayertoobj(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto objecttype = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto id = static_cast<NPCID>(visitor->getGameValueAs<double>(*arguments[1]));

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->sendPropsFromResults(player->setPropWith<PlayerProp::ATTACHNPC>(SetBy::SERVER, id, objecttype));
	}
}

// blockagain;
// Enables collision.
void fn_blockagain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setProp(NPCProp::BLOCKFLAGS, static_cast<uint8_t>(NPCBlockFlags::BLOCK));
	}
}

// canbecarried;
// Flags as carryable.
void fn_canbecarried(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("canbecarried is not implemented yet.");
}

// canbepulled;
// Flags as pullable.
void fn_canbepulled(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("canbepulled is not implemented yet.");
}

// canbepushed;
// Flags as pushable.
void fn_canbepushed(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("canbepushed is not implemented yet.");
}

// cannotbecarried;
// Flags as not carryable.
void fn_cannotbecarried(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("cannotbecarried is not implemented yet.");
}

// cannotbepulled;
// Flags as not pullable.
void fn_cannotbepulled(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("cannotbepulled is not implemented yet.");
}

// cannotbepushed;
// Flags as not pushable.
void fn_cannotbepushed(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("cannotbepushed is not implemented yet.");
}

// cannotwarp;
// Flags as not warpable.
void fn_cannotwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->warpRestrictions = NPCWarpRestrictions::NOTALLOWED;
	}
}

// canwarp;
// Flags as being able to change levels by touching links.
void fn_canwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->warpRestrictions = NPCWarpRestrictions::ALLOWED;
	}
}

// canwarp2;
// Flags as being able to change levels by moving across levels on a gmap.
void fn_canwarp2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->warpRestrictions = NPCWarpRestrictions::ONLYOVERWORLD;
	}
}

// carryobject carryobjecttype;
void fn_carryobject(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("carryobject is not implemented yet.");
}

// copyflagss fromprefix,toprefix;
void fn_copyflagss(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("copyflagss is not implemented yet.");
}

// copylevel oldfile,newfile;
void fn_copylevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("copylevel is not implemented yet.");
}

// copystrings fromprefix,toprefix;
void fn_copystrings(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("copystrings is not implemented yet.");
}

// deletelevel filename;
void fn_deletelevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("deletelevel is not implemented yet.");
}

// deletestring list,index;
void fn_deletestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("deletestring is not implemented yet.");
}

// destroy;
void fn_destroy(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("destroy is not implemented yet.");
}

// detachplayer;
void fn_detachplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->sendPropsFromResults(player->setPropWith<PlayerProp::ATTACHNPC>(SetBy::SERVER, static_cast<NPCID>(0), 0_ui8));
	}
}

// disabledefmovement;
void fn_disabledefmovement(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
		}
	}
	*/

	throw std::exception("disabledefmovement is not implemented yet.");
}

// disableweapons;
// Disables the player's weapons.
void fn_disableweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->disableWeapons();
	}
}

// dontblock;
// Disables collision.
void fn_dontblock(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setProp(NPCProp::BLOCKFLAGS, static_cast<uint8_t>(NPCBlockFlags::NOBLOCK));
	}
}

// drawaslight;
// Draws the object as a light source.
void fn_drawaslight(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("drawaslight is not implemented yet.");
}

// drawoverplayer;
void fn_drawoverplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setProp(NPCProp::VISFLAGS, static_cast<uint8_t>(npc->visFlags & (PROPID(NPCVisFlags::DRAWOVERPLAYER) | PROPID(NPCVisFlags::VISIBLE))));
	}
}

// drawunderplayer;
void fn_drawunderplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setProp(NPCProp::VISFLAGS, static_cast<uint8_t>(npc->visFlags & (PROPID(NPCVisFlags::DRAWUNDERPLAYER) | PROPID(NPCVisFlags::VISIBLE))));
	}
}

// enabledefmovement;
void fn_enabledefmovement(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
		}
	}
	*/

	throw std::exception("enabledefmovement is not implemented yet.");
}

// enableweapons;
// Enables the player's weapons.
void fn_enableweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->enableWeapons();
	}
}

// explodebomb index;
void fn_explodebomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("explodebomb is not implemented yet.");
}

// freezeplayer2;
// Freezes the player, preventing movement and actions.
void fn_freezeplayer2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->freezePlayer();
	}
}

// hide;
void fn_hide(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setProp(NPCProp::VISFLAGS, static_cast<uint8_t>(npc->visFlags & ~PROPID(NPCVisFlags::VISIBLE)));
	}
}

// hitcompu index,power,fromx,fromy;
void fn_hitcompu(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("hitcompu is not implemented yet.");
}

// hitnpc index,halfhearts,fromx,fromy;
void fn_hitnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("hitnpc is not implemented yet.");
}

// hitobjects power,x,y;
void fn_hitobjects(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("hitobjects is not implemented yet.");
}

// hitplayer index,halfhearts,fromx,fromy;
void fn_hitplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
		}
	}
	*/

	throw std::exception("hitplayer is not implemented yet.");
}

// hurt halfhearts;
void fn_hurt(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
		}
	}
	*/

	throw std::exception("hurt is not implemented yet.");
}

// insertstring list,index,text;
void fn_insertstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::exception("insertstring requires exactly three arguments: list, index, and text.");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto list = string::splitHard(listVar->get<std::string>().value_or({}), ","sv);
		auto index = static_cast<size_t>(std::max(0.0, visitor->getGameValueAs<double>(*arguments[1])));
		auto text = visitor->getGameValueAs<std::string>(*arguments[2]);

		// Insert blank strings to fill the space.
		if (index > list.size())
		{
			std::vector<std::string> emptyStrings(index - list.size(), "");
			list.append_range(emptyStrings);
		}

		// Insert the text at the specified index.
		if (index == list.size())
		{
			list.push_back(text);
		}
		else if (index < list.size())
		{
			list.insert(list.begin() + index, text);
		}

		// Write it back.
		listVar->assign(string::join(list, ","));
	}
}

// join class;
void fn_join(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("join is not implemented yet.");
}

// lay itemname;
void fn_lay(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("lay is not implemented yet.");
}

// lay2 itemname,x,y;
void fn_lay2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("lay2 is not implemented yet.");
}

// message text;
void fn_message(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("message requires exactly one argument: text.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto text = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setProp(NPCProp::MESSAGE, text);
	}
}

// move dx,dy,time,options;
void fn_move(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("move is not implemented yet.");
}

// noplayeronwall;
void fn_noplayeronwall(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("noplayeronwall is not implemented yet.");
}

// putbomb power,x,y;
void fn_putbomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("putbomb is not implemented yet.");
}

// putcomp baddyname,x,y;
// Adds a new baddy to the level with the specified parameters.
void fn_putcomp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::exception("putcomp requires exactly three arguments: baddyname, x, y.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		uint8_t baddyname = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);

		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->level.lock(); level != nullptr)
				level->putNewBaddy((float)x, (float)y, static_cast<BaddyType>(baddyname));
		}
	}
}

// putexplosion radius,x,y;
void fn_putexplosion(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("putexplosion is not implemented yet.");
}

// putexplosion2 power,radius,x,y;
void fn_putexplosion2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("putexplosion2 is not implemented yet.");
}

// puthorse imagefile,x,y;
void fn_puthorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("puthorse is not implemented yet.");
}

// putnewcomp baddyname,x,y,imagefile,power;
// Adds a new baddy to the level with the specified parameters.
void fn_putnewcomp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::exception("putnewcomp requires exactly five arguments: baddyname, x, y, imagefile, and power.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		uint8_t baddyname = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);
		auto imagefile = visitor->getGameValueAs<std::string>(*arguments[3]);
		auto power = visitor->getGameValueAs<double>(*arguments[4]);

		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->level.lock(); level != nullptr)
				level->putNewBaddy((float)x, (float)y, static_cast<BaddyType>(baddyname), static_cast<uint8_t>(power), imagefile);
		}
	}
}

// putnpc imagefile,scriptfile,x,y;
void fn_putnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("putnpc is not implemented yet.");
}

// putnpc2 x,y,{ script };
void fn_putnpc2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("putnpc2 is not implemented yet.");
}

// removearrow index;
void fn_removearrow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("removearrow is not implemented yet.");
}

// removebomb index;
void fn_removebomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("removebomb is not implemented yet.");
}

// removecompus;
// Removes all baddies from the level.
void fn_removecompus(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->level.lock(); level != nullptr)
				level->removeAllBaddies();
		}
	}
}

// removeexplo index;
void fn_removeexplo(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("removeexplo is not implemented yet.");
}

// removeguild guild;
void fn_removeguild(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("removeguild is not implemented yet.");
}

// removeguildmember guild,account,nick;
void fn_removeguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("removeguildmember is not implemented yet.");
}

// removehorse index;
void fn_removehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("removehorse is not implemented yet.");
}

// removeitem index;
void fn_removeitem(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("removeitem is not implemented yet.");
}

// removestring list,text;
void fn_removestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::exception("removestring requires exactly two arguments: list and text.");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto list = listVar->get<std::string>().value_or({});
		auto text = visitor->getGameValueAs<std::string>(*arguments[1]);

		size_t textLength = text.size();
		size_t pos = 0;
		while ((pos = list.find(text, pos)) != std::string::npos)
		{
			if (pos + textLength + 1 < list.size() && list[pos + textLength] == ',')
			{
				// If the text is followed by a comma, remove it as well.
				list.erase(pos, textLength + 1);
			}
			else
			{
				list.erase(pos, textLength);
			}
		}

		listVar->assign(list);
	}
}

// removeweapon weaponname;
void fn_removeweapon(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->deleteWeapon(first);
	}
}

// replacestring list,index,text;
void fn_replacestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::exception("replacestring requires exactly three arguments: list, index, and text.");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto list = listVar->get<std::string>().value_or({});
		auto index = static_cast<size_t>(std::max(0.0, visitor->getGameValueAs<double>(*arguments[1])));
		auto text = visitor->getGameValueAs<std::string>(*arguments[2]);

		auto start = std::ranges::begin(list);
		if (index != 0)
		{
			size_t loc = 0;
			start = std::ranges::find_if(list, [&loc, &index](const char& c) { return (c == ',' && ++loc == index); });
		}
		if (start == std::ranges::end(list))
			return;

		auto end = std::ranges::find(start, std::ranges::end(list), ',');
		list.replace(start, end, text);
		listVar->assign(list);
	}
}

// saveinfo text,text;
void fn_saveinfo(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("saveinfo is not implemented yet.");
}

// savelog text;
void fn_savelog(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("savelog is not implemented yet.");
}

// savelog2 filename,text;
void fn_savelog2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("savelog2 is not implemented yet.");
}

// say2 message;
void fn_say2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->sendSignMessage(first);
	}
}

// sendpm message;
void fn_sendpm(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
		}
	}
	*/

	throw std::exception("sendpm is not implemented yet.");
}

// sendrpgmessage message;
// Sends a message to the F2 message window of the player.
void fn_sendrpgmessage(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->sendRPGMessage(first);
	}
}

// sendtonc message;
void fn_sendtonc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("sendtonc is not implemented yet.");
}

// sendtorc message;
void fn_sendtorc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("sendtorc is not implemented yet.");
}

// serverwarp servername;
void fn_serverwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("serverwarp is not implemented yet.");
}

// set flag;
void fn_set(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
		}
	}
	*/

	throw std::exception("set is not implemented yet.");
}

// setani gani;
// setani gani,params;
// Sets the animation for the player.
void fn_setani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			std::vector<SetResults> results;
			results.push_back(player->setPropWith<PlayerProp::GANI>(SetBy::SERVER, first));

			// If there are additional parameters, set them as gani attributes.
			if (arguments.size() > 1)
			{
				for (size_t i = 1; i < arguments.size(); ++i)
				{
					if (arguments[i] == nullptr)
						continue;
					auto param = visitor->getGameValueAs<std::string>(*arguments[i]);
					auto propId = static_cast<PlayerProp>(GaniAttributePropList.at(i - 1));
					auto prop = std::make_shared<PropertyString>(param);
					results.push_back(player->setProp(propId, prop, SetBy::SERVER));
				}
			}

			player->sendPropsFromResults(results);
		}
	}
}

// setarray var,size;
void fn_setarray(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::exception("setarray requires exactly two arguments: var and size.");
	
	if (auto* var = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); var != nullptr)
	{
		auto size = static_cast<size_t>(std::max(0.0, visitor->getGameValueAs<double>(*arguments[1])));
		std::vector<double> arrayValues;
		arrayValues.assign(size, 0.0);

		var->assign(GameValue{ std::move(arrayValues) });
	}
}

// setbeltcolor color;
// Sets the player's belt color.
void fn_setbeltcolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[4] = static_cast<uint8_t>(color);
			player->sendPropsFromResults(player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors));
		}
	}
}

// setbody filename;
// Sets the body image for the player.
void fn_setbody(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->sendPropsFromResults(player->setPropWith<PlayerProp::BODYIMG>(SetBy::SERVER, filename));
	}
}

// setcharani gani;
// setcharani gani,params;
void fn_setcharani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() == 0)
		throw std::exception("setcharani requires at least one argument: gani.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto gani = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			npc->setProp(NPCProp::GANI, gani);
			if (arguments.size() > 1)
			{
				auto params = string::fromCSV(visitor->getGameValueAs<std::string>(*arguments[1]));
				for (auto i = 0; i < params.size() && i < 30; ++i)
				{
					auto propId = npcGaniAttrPackets.at(i);
					npc->setProp((NPCProp)propId, params[i]);
				}
			}
		}
	}
}

// setchargender gender;
void fn_setchargender(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	// Did gender get an NPC prop?
	throw std::exception("setchargender is not implemented yet.");
}

// setcharprop messagecode,text;
void fn_setcharprop(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::exception("setcharprop requires exactly two arguments: messagecode and text.");

	if (auto* messagecodeVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); messagecodeVar != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		auto text = visitor->getGameValueAs<std::string>(*arguments[1]);
		messagecodeVar->assign(text);
	}

	throw std::exception("setcharprop is not implemented yet.");
}

// setcoatcolor color;
// Sets the player's coat color.
void fn_setcoatcolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[1] = static_cast<uint8_t>(color);
			player->sendPropsFromResults(player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors));
		}
	}
}

// setgender gender;
void fn_setgender(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto gender = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			auto status = player->account.status;
			if (gender == 0)
				status |= PLSTATUS_MALE;
			else
				status &= ~PLSTATUS_MALE;

			player->sendPropsFromResults(player->setPropWith<PlayerProp::STATUS>(SetBy::SERVER, status));
		}
	}
}

// setgif image;
void fn_setgif(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	fn_setimg(visitor, commandName, arguments);
}

// sethead filename;
void fn_sethead(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->sendPropsFromResults(player->setPropWith<PlayerProp::HEADGIF>(SetBy::SERVER, first));
	}
}

// setimg filename;
// Sets the image of the NPC to a new one.
void fn_setimg(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("setimg requires exactly one argument: filename.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setProp(NPCProp::IMAGE, filename);
	}
}

// setimgpart filename,x,y,width,height;
void fn_setimgpart(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::exception("setimgpart requires exactly five arguments: filename, x, y, width, height.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto x = static_cast<uint16_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto y = static_cast<uint16_t>(visitor->getGameValueAs<double>(*arguments[2]));
		auto width = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[3]));
		auto height = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[4]));

		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			npc->setProp(NPCProp::IMAGE, filename);
			npc->setPropsFromPacket(CString() >> (char)NPCProp::IMAGEPART >> (short)x >> (short)y >> (char)width >> (char)height, CLVER_2_17, true);
		}
	}
}

// setlevel filename;
// Warps the player to a new level specified by the filename.
void fn_setlevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->warp(filename, player->getX(), player->getY());
	}
}

// setlevel2 filename,x,y;
// Warps the player to a new level specified by the filename and coordinates (x, y).
void fn_setlevel2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->warp(filename, static_cast<float>(x), static_cast<float>(y));
	}
}

// setmap imgfile,levelsfile,x,y;
void fn_setmap(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("setmap is not implemented yet.");
}

// setminimap imgfile,levelsfile,x,y;
void fn_setminimap(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("setminimap is not implemented yet.");
}

// setplayerdir dir;
// Sets the direction of the player sprite.
void fn_setplayerdir(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto dir = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			// Set the new player direction relative to their current sprite.
			uint8_t sprite = player->account.character.sprite;
			uint8_t currentDir = sprite % 4;
			uint8_t newDir = currentDir + (dir - currentDir);

			player->sendPropsFromResults(player->setPropWith<PlayerProp::SPRITE>(SetBy::SERVER, newDir));
		}
	}
}

// setplayerprop messagecode,text;
// Sets a property for the player.
void fn_setplayerprop(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		if (auto* var = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); var != nullptr)
		{
			auto text = visitor->getGameValueAs<std::string>(*arguments[1]);
			var->assign<std::string>(text);
		}
	}
}

// setpm message;
void fn_setpm(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("setpm is not implemented yet.");
}

// setshape type,width,height;
// type 1 = rectangle
void fn_setshape(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("setshape is not implemented yet.");
}

// setshape2 width,height,{tiletypes...};
void fn_setshape2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("setshape2 is not implemented yet.");
}

// setshield image,power;
void fn_setshield(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto image = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto power = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->sendPropsFromResults(player->setPropWith<PlayerProp::SHIELDPOWER>(SetBy::SERVER, image, power));
	}
}

// setshoecolor color;
// Sets the player's shoe color.
void fn_setshoecolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[3] = static_cast<uint8_t>(color);
			player->sendPropsFromResults(player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors));
		}
	}
}

// setshootparams params;
void fn_setshootparams(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("setshootparams is not implemented yet.");
}

// setskincolor color;
// Sets the player's skin color.
void fn_setskincolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[0] = static_cast<uint8_t>(color);
			player->sendPropsFromResults(player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors));
		}
	}
}

// setsleevecolor color;
// Sets the player's sleeve color.
void fn_setsleevecolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[2] = static_cast<uint8_t>(color);
			player->sendPropsFromResults(player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors));
		}
	}
}

// setstring var,text;
void fn_setstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::exception("setstring requires exactly two arguments: var and text.");

	// Assign the string.
	if (auto* var = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); var != nullptr)
	{
		auto text = visitor->getGameValueAs<std::string>(*arguments[1]);
		var->assign<std::string>(text);
	}
}

// setsword image,power;
// Sets the players sword image and power.
void fn_setsword(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto image = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto power = static_cast<int8_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->sendPropsFromResults(player->setPropWith<PlayerProp::SWORDPOWER>(SetBy::SERVER, image, power));
	}
}

// setz x,y,width,height,a,b,c,d;
void fn_setz(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("setz is not implemented yet.");
}

// shoot x,z,y,angle,zangle,power,gani,ganiparams;
void fn_shoot(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("shoot is not implemented yet.");
}

// shootarrow dir;
void fn_shootarrow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("shootarrow is not implemented yet.");
}

// shootball;
void fn_shootball(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("shootball is not implemented yet.");
}

// shootfireball dir;
void fn_shootfireball(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("shootfireball is not implemented yet.");
}

// shootfireblast dir;
void fn_shootfireblast(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("shootfireblast is not implemented yet.");
}

// shootnuke dir;
void fn_shootnuke(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("shootnuke is not implemented yet.");
}

// show;
// Makes the NPC visible.
void fn_show(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setProp(NPCProp::VISFLAGS, static_cast<uint8_t>(npc->visFlags | (uint8_t)NPCVisFlags::VISIBLE));
	}
}

// showani index,x,y,direction,gani,params;
void fn_showani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("showani is not implemented yet.");
}

// showani2 index,x,y,z,direction,gani,params;
void fn_showani2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("showani2 is not implemented yet.");
}

// showcharacter;
// Turns the NPC into a character.
void fn_showcharacter(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setProp(NPCProp::IMAGE, "#c#"s);
	}
}

// showimg index,filename,x,y;
void fn_showimg(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("showimg is not implemented yet.");
}

// showimg2 index,filename,x,y,z;
void fn_showimg2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("showimg2 is not implemented yet.");
}

// showstats bitflag;
void fn_showstats(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("showstats is not implemented yet.");
}

// sleep duration;
void fn_sleep(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("sleep is not implemented yet.");
}

// spyfire length,power;
void fn_spyfire(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("spyfire is not implemented yet.");
}

// take itemname;
void fn_take(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("take is not implemented yet.");
}

// take2 index;
void fn_take2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("take2 is not implemented yet.");
}

// takehorse index;
void fn_takehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("takehorse is not implemented yet.");
}

// takeplayercarry;
void fn_takeplayercarry(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
		}
	}
	*/

	throw std::exception("takeplayercarry is not implemented yet.");
}

// takeplayerhorse;
void fn_takeplayerhorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
		}
	}
	*/

	throw std::exception("takeplayerhorse is not implemented yet.");
}

// throwcarry;
// Throws the carried object.
void fn_throwcarry(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("throwcarry is not implemented yet.");
}

// timershow;
void fn_timershow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("timershow is not implemented yet.");
}

// tokenize text;
// Tokenizes a string into tokens using spaces as a delimiter.
void fn_tokenize(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("tokenize requires exactly one argument: text.");

	auto text = visitor->getGameValueAs<std::string>(*arguments[0]);
	visitor->tokenizeTokens = string::splitHard(text, " "sv);
}

// tokenize2 delims,text;
// Tokenizes a string into tokens using the specified delimiters.
void fn_tokenize2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::exception("tokenize2 requires exactly two arguments: delims and text.");

	auto delims = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto text = visitor->getGameValueAs<std::string>(*arguments[1]);
	visitor->tokenizeTokens = string::splitHard(text, delims);
}

// triggeraction x,y,action,params;
void fn_triggeraction(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (arguments.size() != 4)
		throw std::exception("triggeraction requires four arguments.");

	auto x = visitor->getGameValueAs<double>(*arguments[0]);
	auto y = visitor->getGameValueAs<double>(*arguments[1]);
	auto action = visitor->getGameValueAs<std::string>(*arguments[2]);
	auto params = visitor->getGameValueAs<std::string>(*arguments[3]);

	auto* server = BabyDI::Get<Server>();
	*/

	throw std::exception("triggeraction is not implemented yet.");
}

// unfreezeplayer;
// Unfreezes a player.
void fn_unfreezeplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->unfreezePlayer();
	}
}

// unset flag;
// Unsets a player's flag.
void fn_unset(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto first = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
		}
	}
	*/

	throw std::exception("unset is not implemented yet.");
}

// updateboard x,y,width,height;
void fn_updateboard(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("updateboard is not implemented yet.");
}

// updateboard2 x,y,width,height;
void fn_updateboard2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("updateboard2 is not implemented yet.");
}

// updateterrain;
void fn_updateterrain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("updateterrain is not implemented yet.");
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
