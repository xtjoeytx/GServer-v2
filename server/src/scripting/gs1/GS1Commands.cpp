#include <algorithm>
#include <any>
#include <array>
#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tree/ParseTree.h>

#include <CString.h>
#include <IEnums.h>

#include <BabyDI.h>
#include <Server.h>
#include <level/LevelBaddy.h>
#include <npcserver/NPCServer.h>
#include <object/Character.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/PropsContainer.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

using BuiltInCommandHandleFunc = void(*)(GS1Visitor*, std::string_view, const std::vector<GS1ScriptValue*>&);
using BuiltInCommandHandleMap = std::unordered_map<size_t, BuiltInCommandHandleFunc>;

static void fn_addguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_addstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_addweapon(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_attachplayertoobj(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_blockagain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_callnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
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
static void fn_copyflags(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_copylevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_copystrings(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_deletelevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_deletestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_destroy(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_detachplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_disableweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_dontblock(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_drawoverplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_drawovertrees(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_drawunderplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
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
static void fn_say(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
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
		{ hash("callnpc"), &fn_callnpc },
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
		{ hash("copyflags"), &fn_copyflags },
		{ hash("copylevel"), &fn_copylevel },
		{ hash("copystrings"), &fn_copystrings },
		{ hash("deletelevel"), &fn_deletelevel },
		{ hash("deletestring"), &fn_deletestring },
		{ hash("destroy"), &fn_destroy },
		{ hash("detachplayer"), &fn_detachplayer },
		{ hash("disableweapons"), &fn_disableweapons },
		{ hash("dontblock"), &fn_dontblock },
		{ hash("drawoverplayer"), &fn_drawoverplayer },
		{ hash("drawovertrees"), &fn_drawovertrees },
		{ hash("drawunderplayer"), &fn_drawunderplayer },
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
		{ hash("say"), &fn_say },
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

constexpr std::array<std::string_view, 8> flagProcessingCommands =
{
	"addstring"sv,
	"deletestring"sv,
	"insertstring"sv,
	"removestring"sv,
	"replacestring"sv,
	"set"sv,
	"setstring"sv,
	"unset"sv,
};

///////////////////////////////////////////////////////////////////////////////

void processBuiltInCommand(GS1Visitor* visitor, antlr4::tree::ParseTree* node, std::string_view commandName)
{
	static BuiltInCommandHandleMap map = GenerateMap();

	if (visitor == nullptr)
		throw std::runtime_error("processBuiltInCommand received an empty visitor");
	if (commandName.empty())
		throw std::runtime_error("processBuiltInCommand received an empty command name");

	// Find the command in the map.
	size_t hash = string::string_hash{}(commandName);
	auto it = map.find(hash);
	if (it == map.end())
	{
		log::printLine(log::script, "processBuiltInCommand received an unknown command: {}", commandName);
		return;
	}

	// Special case for 'setplayerprop' and 'setcharprop', which need to push a unique context onto the stack.
	// We need to bring the relevant context to the front so the message code links to the correct player or NPC, since it can touch both.
	bool popContext = false;
	if (commandName == "setcharprop")
	{
		auto npc = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC);
		if (!npc.has_value())
			npc = visitor->getOriginalSource();

		visitor->pushSource(npc.value());
		popContext = true;
	}
	else if (commandName == "setplayerprop")
	{
		auto player = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER);
		if (!player.has_value())
		{
			if (visitor->getEvent().initiator.second != ScriptObjectSourceType::PLAYER)
				return;
			player = visitor->getEvent().initiator;
		}

		visitor->pushSource(player.value());
		popContext = true;
	}

	// Record if we are expecting a flag.
	visitor->expectingFlag = (std::ranges::find(flagProcessingCommands, commandName) != std::ranges::end(flagProcessingCommands));

	// Collect the arguments from the node.
	std::vector<GS1ScriptValue*> arguments;
	auto children = visitor->visitChildrenAndCollect(node);
	for (auto& result : children)
	{
		auto* container = std::any_cast<GS1ScriptValue>(&result);
		if (container == nullptr)
			throw std::runtime_error("BuiltInCommand argument is not a valid GS1ScriptValue");

		// Add to the arguments.
		arguments.push_back(container);
	}

	// Unset the expecting flag.
	visitor->expectingFlag = false;

	// Execute the command.
	it->second(visitor, commandName, arguments);

	// If we pushed a context, we need to pop it after the command execution.
	if (popContext)
		visitor->popSource();
}

///////////////////////////////////////////////////////////////////////////////

// addguildmember guild,account,nick;
void fn_addguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("addguildmember is not implemented yet.");
}

// addstring list,text;
// Adds a string to a string list.
void fn_addstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("addstring requires exactly two arguments: list and text.");

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
		auto weaponname = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->addWeapon(weaponname);
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
			player->setPropWith<PlayerProp::ATTACHNPC>(SetBy::SERVER, id, objecttype);
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
		{
			uint8_t blockFlags = npc->blockFlags & ~PROPID(NPCBlockFlags::NOBLOCK);
			npc->setPropWith<NPCProp::BLOCKFLAGS>(SetBy::SERVER, blockFlags);
		}
	}
}

// callnpc index,eventname,params;
// Sends an event to an NPC.
void fn_callnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("callnpc requires at least two arguments: index and eventname.");

	NPCID sourceNPC = 0;
	if (visitor->getOriginalSource().second == ScriptObjectSourceType::NPC)
		sourceNPC = visitor->getOriginalSource().first;

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));

		std::vector<std::string> eventAndParams;
		eventAndParams.emplace_back(visitor->getGameValueAs<std::string>(*arguments[1]));
		if (arguments.size() > 2)
			eventAndParams.append_range(string::fromCSV(visitor->getGameValueAs<std::string>(*arguments[2])));

		auto* server = BabyDI::Get<Server>();
		auto& levelNPCs = level->getNPCs();
		if (index < levelNPCs.size())
		{
			if (auto npc = server->getNPC(levelNPCs[index]); npc != nullptr)
				npc->scripting.events.addEvent(ScriptEventType::CUSTOM, source::FromNPC(sourceNPC), eventAndParams);
		}
	}
}

// canbecarried;
// Flags as carryable.
void fn_canbecarried(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			uint8_t blockFlags = npc->blockFlags | PROPID(NPCBlockFlags::CANBECARRIED);
			npc->setPropWith<NPCProp::BLOCKFLAGS>(SetBy::SERVER, blockFlags);
		}
	}
}

// canbepulled;
// Flags as pullable.
void fn_canbepulled(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			uint8_t blockFlags = npc->blockFlags | PROPID(NPCBlockFlags::CANBEPULLED);
			npc->setPropWith<NPCProp::BLOCKFLAGS>(SetBy::SERVER, blockFlags);
		}
	}
}

// canbepushed;
// Flags as pushable.
void fn_canbepushed(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			uint8_t blockFlags = npc->blockFlags | PROPID(NPCBlockFlags::CANBEPUSHED);
			npc->setPropWith<NPCProp::BLOCKFLAGS>(SetBy::SERVER, blockFlags);
		}
	}
}

// cannotbecarried;
// Flags as not carryable.
void fn_cannotbecarried(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			uint8_t blockFlags = npc->blockFlags & ~PROPID(NPCBlockFlags::CANBECARRIED);
			npc->setPropWith<NPCProp::BLOCKFLAGS>(SetBy::SERVER, blockFlags);
		}
	}
}

// cannotbepulled;
// Flags as not pullable.
void fn_cannotbepulled(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			uint8_t blockFlags = npc->blockFlags & ~PROPID(NPCBlockFlags::CANBEPULLED);
			npc->setPropWith<NPCProp::BLOCKFLAGS>(SetBy::SERVER, blockFlags);
		}
	}
}

// cannotbepushed;
// Flags as not pushable.
void fn_cannotbepushed(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			uint8_t blockFlags = npc->blockFlags & ~PROPID(NPCBlockFlags::CANBEPUSHED);
			npc->setPropWith<NPCProp::BLOCKFLAGS>(SetBy::SERVER, blockFlags);
		}
	}
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
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto carryObjectTypeId = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::CARRYSPRITE>(SetBy::SERVER, carryObjectTypeId);
	}
	*/
}

// copyflags fromprefix,toprefix;
void fn_copyflags(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("copyflags is not implemented yet.");
}

// copylevel oldfile,newfile;
void fn_copylevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("copylevel is not implemented yet.");
}

// copystrings fromprefix,toprefix;
void fn_copystrings(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("copystrings is not implemented yet.");
}

// deletelevel filename;
void fn_deletelevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("deletelevel is not implemented yet.");
}

// deletestring list,index;
void fn_deletestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("deletestring requires exactly two arguments: list and index.");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto list = string::splitHard(listVar->get<std::string>().value_or({}), ","sv);
		auto index = static_cast<size_t>(std::max(0.0, visitor->getGameValueAs<double>(*arguments[1])));

		// Check for out of bounds.
		if (index >= list.size())
			return;

		// Move the iterator to the index we want to delete.
		auto it = list.begin();
		std::advance(it, index);

		// Delete the string.
		list.erase(it);

		// Write it back.
		listVar->assign(string::join(list, ","));
	}
}

// destroy;
void fn_destroy(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->getOriginalSource().second; source == ScriptObjectSourceType::NPC)
	{
		auto* server = BabyDI::Get<Server>();
		server->getNPCServer()->deleteNPC(visitor->getOriginalSource().first);
	}
}

// detachplayer;
void fn_detachplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::ATTACHNPC>(SetBy::SERVER, static_cast<NPCID>(0), 0_ui8);
	}
}

// disableweapons;
// Disables the player's weapons.
void fn_disableweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
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
		{
			uint8_t blockFlags = npc->blockFlags | PROPID(NPCBlockFlags::NOBLOCK);
			npc->setPropWith<NPCProp::BLOCKFLAGS>(SetBy::SERVER, blockFlags);
		}
	}
}

// drawoverplayer;
void fn_drawoverplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, static_cast<uint8_t>(npc->visFlags & (PROPID(NPCVisFlags::DRAWOVERPLAYER) | PROPID(NPCVisFlags::VISIBLE))));
	}
}

// drawovertrees;
void fn_drawovertrees(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto visFlags = npc->visFlags;
			visFlags &= ~(PROPID(NPCVisFlags::DRAWOVERPLAYER) | PROPID(NPCVisFlags::DRAWUNDERPLAYER));
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, static_cast<uint8_t>(visFlags | PROPID(NPCVisFlags::VISIBLE)));
		}
	}
}

// drawunderplayer;
void fn_drawunderplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, static_cast<uint8_t>(npc->visFlags & (PROPID(NPCVisFlags::DRAWUNDERPLAYER) | PROPID(NPCVisFlags::VISIBLE))));
	}
}

// enableweapons;
// Enables the player's weapons.
void fn_enableweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->enableWeapons();
	}
}

// explodebomb index;
void fn_explodebomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("explodebomb is not implemented yet.");
}

// freezeplayer2;
// Freezes the player, preventing movement and actions.
void fn_freezeplayer2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
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
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, static_cast<uint8_t>(npc->visFlags & ~PROPID(NPCVisFlags::VISIBLE)));
	}
}

// hitcompu index,power,fromx,fromy;
void fn_hitcompu(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("hitcompu is not implemented yet.");
}

// hitnpc index,halfhearts,fromx,fromy;
void fn_hitnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	// Could probably emulate with move().
	throw std::runtime_error("hitnpc is not implemented yet.");
}

// hitobjects power,x,y;
// Hit objects at a location.
void fn_hitobjects(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto power = static_cast<int8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto x = static_cast<float>(visitor->getGameValueAs<double>(*arguments[1]));
		auto y = static_cast<float>(visitor->getGameValueAs<double>(*arguments[2]));

		auto* server = BabyDI::Get<Server>();
		server->hitObjectsAtPoint({ x, y }, power, level);
	}
}

// hitplayer index,halfhearts,fromx,fromy;
// Hits a player in the level.
void fn_hitplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto halfhearts = static_cast<int8_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto fromx = static_cast<float>(visitor->getGameValueAs<double>(*arguments[2]));
		auto fromy = static_cast<float>(visitor->getGameValueAs<double>(*arguments[3]));

		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->level.lock(); level != nullptr)
			{
				auto playerIdIter = level->getPlayers().begin();
				std::advance(playerIdIter, index);
				if (playerIdIter != level->getPlayers().end())
					server->hitPlayer(*playerIdIter, halfhearts, fromx, fromy, npc);
			}
		}
	}
}

// hurt halfhearts;
// Hurts a player.
void fn_hurt(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto halfhearts = static_cast<int8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto npcId = visitor->getOriginalSource().first;
		if (visitor->getOriginalSource().second != ScriptObjectSourceType::NPC)
			npcId = 0;

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			server->hitPlayer(player->getId(), halfhearts, player->getX() + 1.5, player->getY() + 2, server->getNPC(npcId));
	}
}

// insertstring list,index,text;
// Inserts a string into a string array at the given position.
void fn_insertstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("insertstring requires exactly three arguments: list, index, and text.");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto list = string::splitHard(listVar->get<std::string>().value_or({}), ","sv);
		auto index = static_cast<size_t>(std::max(0.0, visitor->getGameValueAs<double>(*arguments[1])));
		auto text = visitor->getGameValueAs<std::string>(*arguments[2]);

		// Insert blank strings to fill the space.
		if (index > list.size())
		{
			std::vector<std::string> emptyStrings(index - list.size(), "");
			for (auto& str : emptyStrings)
				list.emplace_back(std::move(str));
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
// Joins a class.
void fn_join(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("join requires exactly one argument: class.");

	auto class_ = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto* server = BabyDI::Get<Server>();

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->joinClass(class_);
	}
	else if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::WEAPON); source.has_value())
	{
		auto& weaponList = server->getWeaponList();
		if (auto weapon = weaponList.find(source.value().first); weapon != weaponList.end())
			weapon->second->joinClass(class_);
	}
}

// lay itemname;
void fn_lay(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("lay is not implemented yet.");
}

// lay2 itemname,x,y;
void fn_lay2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("lay2 is not implemented yet.");
}

// message text;
// Sets the NPC message.
void fn_message(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		std::string text{};
		if (arguments.size() != 0)
			text = visitor->getGameValueAs<std::string>(*arguments[0]);

		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::MESSAGE>(SetBy::SERVER, text);
	}
}

// move dx,dy,time,options;
// Moves an NPC smoothly on the client.
void fn_move(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("move requires exactly four arguments: dx, dy, time, options.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto dx = static_cast<float>(visitor->getGameValueAs<double>(*arguments[0]));
		auto dy = static_cast<float>(visitor->getGameValueAs<double>(*arguments[1]));
		auto time = static_cast<float>(visitor->getGameValueAs<double>(*arguments[2]));
		auto options = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[3]));

		auto pixelDX = static_cast<int16_t>(dx * 16);
		auto pixelDY = static_cast<int16_t>(dy * 16);

		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			server->moveNPC(npc, dx, dy, time, options);
			npc->setPropWith<NPCProp::X2>(SetBy::SERVER, static_cast<int16_t>(npc->character.pixelX + pixelDX));
			npc->setPropWith<NPCProp::Y2>(SetBy::SERVER, static_cast<int16_t>(npc->character.pixelY + pixelDY));
		}
	}
}

// noplayeronwall;
// Disables onwall checks from detecting players.
void fn_noplayeronwall(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->noPlayerOnWall = true;
	}
}

// putbomb power,x,y;
void fn_putbomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("putbomb is not implemented yet.");
}

// putcomp baddyname,x,y;
// Adds a new baddy to the level with the specified parameters.
void fn_putcomp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("putcomp requires exactly three arguments: baddyname, x, y.");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		uint8_t baddyname = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);
		level->putNewBaddy((float)x, (float)y, static_cast<BaddyType>(baddyname));
	}
}

// putexplosion radius,x,y;
void fn_putexplosion(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("putexplosion is not implemented yet.");
}

// putexplosion2 power,radius,x,y;
void fn_putexplosion2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("putexplosion2 is not implemented yet.");
}

// puthorse imagefile,x,y;
void fn_puthorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("puthorse is not implemented yet.");
}

// putnewcomp baddyname,x,y,imagefile,power;
// Adds a new baddy to the level with the specified parameters.
void fn_putnewcomp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::invalid_argument("putnewcomp requires exactly five arguments: baddyname, x, y, imagefile, and power.");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		uint8_t baddyname = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);
		auto imagefile = visitor->getGameValueAs<std::string>(*arguments[3]);
		auto power = visitor->getGameValueAs<double>(*arguments[4]);
		level->putNewBaddy((float)x, (float)y, static_cast<BaddyType>(baddyname), static_cast<uint8_t>(power), imagefile);
	}
}

// putnpc imagefile,scriptfile,x,y;
void fn_putnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("putnpc is not implemented yet.");
}

// putnpc2 x,y,{ script };
void fn_putnpc2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("putnpc2 is not implemented yet.");
}

// removearrow index;
void fn_removearrow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("removearrow is not implemented yet.");
}

// removebomb index;
void fn_removebomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("removebomb is not implemented yet.");
}

// removecompus;
// Removes all baddies from the level.
void fn_removecompus(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto level = visitor->findCurrentLevel(); level != nullptr)
		level->removeAllBaddies();
}

// removeexplo index;
void fn_removeexplo(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("removeexplo is not implemented yet.");
}

// removeguild guild;
void fn_removeguild(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("removeguild is not implemented yet.");
}

// removeguildmember guild,account,nick;
void fn_removeguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("removeguildmember is not implemented yet.");
}

// removehorse index;
void fn_removehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("removehorse is not implemented yet.");
}

// removeitem index;
void fn_removeitem(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("removeitem is not implemented yet.");
}

// removestring list,text;
void fn_removestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("removestring requires exactly two arguments: list and text.");

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
		auto weaponname = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->deleteWeapon(weaponname);
	}
}

// replacestring list,index,text;
void fn_replacestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("replacestring requires exactly three arguments: list, index, and text.");

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
	throw std::runtime_error("saveinfo is not implemented yet.");
}

// savelog text;
void fn_savelog(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("savelog is not implemented yet.");
}

// savelog2 filename,text;
void fn_savelog2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("savelog2 is not implemented yet.");
}

// say signindex;
// Displays the text of a sign at the specified index to the player.
void fn_say(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("say requires exactly one argument: signindex.");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto signIndex = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
		if (signIndex < level->getSigns().size())
		{
			auto& sign = level->getSigns()[signIndex];
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
			{
				auto* server = BabyDI::Get<Server>();
				if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
					player->sendSignMessage(sign->getUText().toString());
			}
		}
	}
}

// say2 message;
// Displays a custom sign message to the player.
void fn_say2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto message = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->sendSignMessage(message);
	}
}

// sendpm message;
// Sends a private message to the player.
void fn_sendpm(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto message = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->sendPrivateMessage(NPCServerPlayerID, string::replaceMutate(message, "#b", "\n"));
	}
}

// sendrpgmessage message;
// Sends a message to the F2 message window of the player.
void fn_sendrpgmessage(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto message = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->sendRPGMessage(message);
	}
}

// sendtonc message;
// Sends a message to the NC (NPC Control).
void fn_sendtonc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	auto message = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto* server = BabyDI::Get<Server>();
	server->sendToNC(message);
}

// sendtorc message;
// Sends a message to the RC (Remote Control).
void fn_sendtorc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	auto message = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto* server = BabyDI::Get<Server>();
	server->sendToRC(message);
}

// serverwarp servername;
// Warps a player to a different server.
void fn_serverwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto servername = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			server->getServerList().sendPacket(CString() >> (char)SVO_SERVERINFO >> (short)player->getId() << servername);
	}
}

// set flag;
// Sets a flag on the player.
void fn_set(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("set requires exactly one argument: flag.");

	if (auto* flag = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); flag != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		if (flag->identifier.starts_with("client.") || flag->identifier.starts_with("clientr."))
		{
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
			{
				if (auto player = server->getPlayer(source.value().first); player != nullptr)
					player->setFlag(flag->identifier, std::nullopt, true);
			}
		}
		else if (flag->identifier.starts_with("server.") || flag->identifier.starts_with("serverr."))
		{
			server->setFlag(flag->identifier, std::nullopt);
		}
		else
		{
			flag->assign<bool>(true);
		}
	}
}

// setani gani;
// setani gani,params;
// Sets the animation for the player.
void fn_setani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto gani = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			std::vector<SetResults> results;
			results.push_back(player->setPropWith<PlayerProp::GANI>(SetBy::SERVER, gani));

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
					results.push_back(player->setProp(propId, SetBy::SERVER, prop));
				}
			}

			//player->sendPropsFromResults(results);
		}
	}
}

// setarray var,size;
// Creates an array of the given size.
void fn_setarray(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("setarray requires exactly two arguments: var and size.");
	
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
			player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors);
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
			player->setPropWith<PlayerProp::BODYIMG>(SetBy::SERVER, filename);
	}
}

// setcharani gani;
// setcharani gani,params;
// Sets the NPC character's animation.
void fn_setcharani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() == 0)
		throw std::invalid_argument("setcharani requires at least one argument: gani.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto gani = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			npc->setPropWith<NPCProp::GANI>(SetBy::SERVER, gani);
			if (arguments.size() > 1)
			{
				auto params = string::fromCSV(visitor->getGameValueAs<std::string>(*arguments[1]));
				for (auto i = 0; i < params.size() && i < 30; ++i)
				{
					auto propId = static_cast<NPCProp>(NPCGaniAttrPackets.at(i));
					auto prop = npc->getProp(propId);
					prop->apply(params[i]);
					npc->setProp(propId, SetBy::SERVER, prop);
				}
			}
		}
	}
}

// setchargender gender;
// Sets the NPC character's gender (controls which voice is used).
void fn_setchargender(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("fn_setchargender requires at least one argument: gender.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto gender = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto visFlags = npc->visFlags;
			if (gender == 0)
				visFlags |= PROPID(NPCVisFlags::MALE);
			else visFlags &= ~PROPID(NPCVisFlags::MALE);

			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, visFlags);
		}
	}
}

// setcharprop messagecode,text;
// Sets an NPC' character property.
void fn_setcharprop(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("setcharprop requires exactly two arguments: messagecode and text.");

	if (auto* messagecode = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); messagecode != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		auto text = visitor->getGameValueAs<std::string>(*arguments[1]);
		messagecode->assign(text);
	}
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
			player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors);
		}
	}
}

// setgender gender;
// Set's the player's gender (controls which voice is used).
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

			player->setPropWith<PlayerProp::STATUS>(SetBy::SERVER, status);
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
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			// This needs to go to everybody (for the player list), so we have to send it immediately.
			auto results = player->setPropWith<PlayerProp::HEADGIF>(SetBy::SERVER, filename);
			results.resultFlags = results.sendToAll;
			player->sendPropsFromResults(results);
		}
	}
}

// setimg filename;
// Sets the image of the NPC to a new one.
void fn_setimg(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("setimg requires exactly one argument: filename.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::IMAGE>(SetBy::SERVER, filename);
	}
}

// setimgpart filename,x,y,width,height;
void fn_setimgpart(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::invalid_argument("setimgpart requires exactly five arguments: filename, x, y, width, height.");

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
			npc->setPropWith<NPCProp::IMAGE>(SetBy::SERVER, filename);
			npc->setPropWith<NPCProp::IMAGEPART>(SetBy::SERVER, x, y, width, height);
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
	throw std::runtime_error("setmap is not implemented yet.");
}

// setminimap imgfile,levelsfile,x,y;
void fn_setminimap(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("setminimap is not implemented yet.");
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

			player->setPropWith<PlayerProp::SPRITE>(SetBy::SERVER, newDir);
		}
	}
}

// setplayerprop messagecode,text;
// Sets a property for the player.
void fn_setplayerprop(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		if (auto* messagecode = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); messagecode != nullptr)
		{
			auto text = visitor->getGameValueAs<std::string>(*arguments[1]);
			messagecode->assign<std::string>(text);
		}
	}
}

// setpm message;
void fn_setpm(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("setpm is not implemented yet.");
}

// setshape type,width,height;
// type 1 = rectangle
void fn_setshape(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("setshape requires exactly three arguments: type, width, height.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto type = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		if (type != 1)
			return;

		auto width = static_cast<uint16_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto height = static_cast<uint16_t>(visitor->getGameValueAs<double>(*arguments[2]));
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->shape = { width, height };
	}
}

// setshield image,power;
void fn_setshield(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("setshield requires exactly two arguments: image and power.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto image = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto power = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::SHIELDPOWER>(SetBy::SERVER, image, power);
	}
}

// setshoecolor color;
// Sets the player's shoe color.
void fn_setshoecolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("setshoecolor requires exactly one argument: color.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[3] = static_cast<uint8_t>(color);
			player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors);
		}
	}
}

// setshootparams params;
// Sets the shoot parameters that calls to the shoot command will use.
void fn_setshootparams(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		return;

	auto params = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto* server = BabyDI::Get<Server>();
	server->setShootParams(string::fromCSV(params));
}

// setskincolor color;
// Sets the player's skin color.
void fn_setskincolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("setskincolor requires exactly one argument: color.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[0] = static_cast<uint8_t>(color);
			player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors);
		}
	}
}

// setsleevecolor color;
// Sets the player's sleeve color.
void fn_setsleevecolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("setsleevecolor requires exactly one argument: color.");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[2] = static_cast<uint8_t>(color);
			player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors);
		}
	}
}

// setstring var,text;
// Sets a string variable with the given text.
void fn_setstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("setstring requires exactly two arguments: var and text.");

	// Assign the string.
	if (auto* var = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); var != nullptr)
	{
		auto text = visitor->getGameValueAs<std::string>(*arguments[1]);

		// Special handling for prefixed variables.
		// Maybe think of a way to do this automatically on the assign rather than doing this.
		auto* server = BabyDI::Get<Server>();
		if (var->identifier.starts_with("client.") || var->identifier.starts_with("clientr."))
		{
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
			{
				if (auto player = server->getPlayer(source.value().first); player != nullptr)
				{
					if (text.empty())
						player->deleteFlag(var->identifier, true);
					else player->setFlag(var->identifier, text, true);
				}
			}
		}
		else if (var->identifier.starts_with("server.") || var->identifier.starts_with("serverr."))
		{
			if (text.empty())
				server->deleteFlag(var->identifier, true);
			else server->setFlag(std::format("{}={}", var->identifier, text), true);
		}
		else
		{
			var->assign<std::string>(text);
		}
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
			player->setPropWith<PlayerProp::SWORDPOWER>(SetBy::SERVER, image, power);
	}
}

// setz x,y,width,height,a,b,c,d;
void fn_setz(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("setz is not implemented yet.");
}

// shoot x,y,z,angle,zangle,power,gani,ganiparams;
void fn_shoot(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() < 7)
		return;

	auto level = visitor->findCurrentLevel();
	if (level == nullptr)
		return;

	auto x = visitor->getGameValueAs<double>(*arguments[0]);
	auto y = visitor->getGameValueAs<double>(*arguments[1]);
	auto z = visitor->getGameValueAs<double>(*arguments[2]);
	auto angle = static_cast<float>(visitor->getGameValueAs<double>(*arguments[3]));
	auto zangle = static_cast<float>(visitor->getGameValueAs<double>(*arguments[4]));
	auto power = static_cast<float>(visitor->getGameValueAs<double>(*arguments[5]));
	auto gani = visitor->getGameValueAs<std::string>(*arguments[6]);

	std::string ganiparams;
	if (arguments.size() > 7)
		ganiparams = visitor->getGameValueAs<std::string>(*arguments[7]);

	auto* server = BabyDI::Get<Server>();
	server->sendShootToOneLevel(level, x, y, z, angle, zangle, power, gani, ganiparams);
}

// shootarrow dir;
void fn_shootarrow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("shootarrow is not implemented yet.");
}

// shootball;
void fn_shootball(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("shootball is not implemented yet.");
}

// shootfireball dir;
void fn_shootfireball(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("shootfireball is not implemented yet.");
}

// shootfireblast dir;
void fn_shootfireblast(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("shootfireblast is not implemented yet.");
}

// shootnuke dir;
void fn_shootnuke(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("shootnuke is not implemented yet.");
}

// show;
// Makes the NPC visible.
void fn_show(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, static_cast<uint8_t>(npc->visFlags | (uint8_t)NPCVisFlags::VISIBLE));
	}
}

// showani index,x,y,direction,gani,params;
void fn_showani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("showani is not implemented yet.");
}

// showani2 index,x,y,z,direction,gani,params;
void fn_showani2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("showani2 is not implemented yet.");
}

// showcharacter;
// Turns the NPC into a character.
void fn_showcharacter(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::IMAGE>(SetBy::SERVER, "#c#"s);
	}
}

// showimg index,filename,x,y;
void fn_showimg(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("showimg is not implemented yet.");
}

// showimg2 index,filename,x,y,z;
void fn_showimg2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("showimg2 is not implemented yet.");
}

// showstats bitflag;
void fn_showstats(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("showstats is not implemented yet.");
}

// sleep duration;
void fn_sleep(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("sleep is not implemented yet.");
}

// spyfire length,power;
void fn_spyfire(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("spyfire is not implemented yet.");
}

// take itemname;
// Takes an item on the level in a 10-tile radius from the NPC.
void fn_take(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("take is not implemented yet.");
}

// take2 index;
void fn_take2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("take2 is not implemented yet.");
}

// takehorse index;
void fn_takehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("takehorse is not implemented yet.");
}

// takeplayercarry;
// Takes the carried object from the player.
void fn_takeplayercarry(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::CARRYSPRITE>(SetBy::SERVER, 0xFF);
	}
	*/
}

// takeplayerhorse;
// Takes the horse from the player.
void fn_takeplayerhorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::HORSEGIF>(SetBy::SERVER, std::string{});
	}
}

// throwcarry;
// Throws the carried object.
// Assuming NPC?
void fn_throwcarry(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::CARRYNPC>(SetBy::SERVER, static_cast<uint32_t>(0));
	}
	*/
}

// timershow;
void fn_timershow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("timershow is not implemented yet.");
}

// tokenize text;
// Tokenizes a string into tokens using spaces as a delimiter.
void fn_tokenize(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("tokenize requires exactly one argument: text.");

	auto text = visitor->getGameValueAs<std::string>(*arguments[0]);
	visitor->tokenizeTokens = string::splitHard(text, " "sv);
}

// tokenize2 delims,text;
// Tokenizes a string into tokens using the specified delimiters.
void fn_tokenize2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("tokenize2 requires exactly two arguments: delims and text.");

	auto delims = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto text = visitor->getGameValueAs<std::string>(*arguments[1]);
	visitor->tokenizeTokens = string::splitHard(text, delims);
}

// triggeraction x,y,action,params;
// Sends out a trigger action.
void fn_triggeraction(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() < 3)
		throw std::invalid_argument("triggeraction requires at least three arguments: x, y, action.");

	auto x = visitor->getGameValueAs<double>(*arguments[0]);
	auto y = visitor->getGameValueAs<double>(*arguments[1]);
	auto action = visitor->getGameValueAs<std::string>(*arguments[2]);
	auto params = string::toCSV(arguments | std::views::drop(3) | std::views::transform([&visitor](GS1ScriptValue* value) { return visitor->getGameValueAs<std::string>(*value); }));

	auto* server = BabyDI::Get<Server>();
	if (action == "clientside")
	{
		if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
			server->sendTriggerAction(source.value().first, 0, { 0, 0 }, action, params);
	}
	else
	{
		const auto& currentSource = visitor->getCurrentSource();
		LevelPtr targetLevel = visitor->findCurrentLevel();
		uint32_t npcId = 0;
		if (currentSource.second == ScriptObjectSourceType::NPC)
			npcId = currentSource.first;
		if (targetLevel != nullptr)
			server->sendTriggerAction(targetLevel, npcId, { static_cast<int16_t>(x * 16), static_cast<int16_t>(y * 16) }, action, params);
	}
}

// unfreezeplayer;
// Unfreezes a player.
void fn_unfreezeplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->unfreezePlayer();
	}
}

// unset flag;
// Unsets a player's flag.
void fn_unset(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("unset requires exactly one argument: flag.");

	if (auto* flag = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); flag != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		if (flag->identifier.starts_with("client.") || flag->identifier.starts_with("clientr."))
		{
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
			{
				if (auto player = server->getPlayer(source.value().first); player != nullptr)
					player->deleteFlag(flag->identifier, true);
			}
		}
		else if (flag->identifier.starts_with("server.") || flag->identifier.starts_with("serverr."))
		{
			server->deleteFlag(flag->identifier);
		}
		else
		{
			flag->assign<bool>(false);
		}
	}
}

// updateboard x,y,width,height;
void fn_updateboard(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("updateboard is not implemented yet.");
}

// updateboard2 x,y,width,height;
void fn_updateboard2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("updateboard2 is not implemented yet.");
}

// updateterrain;
void fn_updateterrain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("updateterrain is not implemented yet.");
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
