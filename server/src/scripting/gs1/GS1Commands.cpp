#include <algorithm>
#include <any>
#include <array>
#include <cmath>
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
#include <level/LevelItem.h>
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
#include <utilities/Extents.h>
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
static void fn_warpto(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);

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
		{ hash("setgif"), &fn_setimg },
		{ hash("setgifpart"), &fn_setimgpart },
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
		{ hash("warpto"), &fn_warpto },
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
		log::printLine(log::script, "Unknown command in NPC '{}': {}", visitor->who, commandName);
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

	// Record if we are expecting a flag for the next argument.
	visitor->expectingFlag = (std::ranges::find(flagProcessingCommands, commandName) != std::ranges::end(flagProcessingCommands));

	// Collect the arguments from the node.
	std::vector<GS1ScriptValue*> arguments;
	std::vector<std::any> results;
	for (size_t i = 0; i < node->children.size(); ++i)
	{
		auto ret = node->children[i]->accept(visitor);
		if (ret.has_value())
		{
			results.emplace_back(std::move(ret));
			auto* container = std::any_cast<GS1ScriptValue>(&results.back());
			if (container == nullptr)
				throw std::runtime_error("BuiltInCommand argument is not a valid GS1ScriptValue");

			arguments.push_back(std::move(container));

			// Unset the expecting flag.
			visitor->expectingFlag = false;
		}
	}

	// Unset the expecting flag.
	visitor->expectingFlag = false;

	try
	{
		// Execute the command.
		it->second(visitor, commandName, arguments);
	}
	catch (...)
	{
		if (popContext)
			visitor->popSource();
		throw;
	}

	// If we pushed a context, we need to pop it after the command execution.
	if (popContext)
		visitor->popSource();
}

///////////////////////////////////////////////////////////////////////////////

static std::optional<PixelPosition> getPositionForArrow(const ScriptObjectSource& source, uint8_t dir)
{
	auto server = BabyDI::Get<Server>();
	if (source.second == ScriptObjectSourceType::NPC)
	{
		if (auto npc = server->getNPC(source.first); npc != nullptr)
		{
			PixelPosition sourcePosition = { npc->character.pixelX, npc->character.pixelY };
			if (npc->isCharacter())
			{
				int16_t dX = (dir == 1 ? -24 : (dir == 3 ? 24 : 0));
				int16_t dY = (dir == 0 ? -24 : (dir == 2 ? 24 : 0));
				sourcePosition.translate(16 + dX, 24 + dY);
			}
			return sourcePosition;
		}
	}

	return std::nullopt;
}

///////////////////////////////////////////////////////////////////////////////

// addguildmember guild,account,nick;
void fn_addguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("addguildmember is not implemented yet.");
}

// addstring list,text;
// Adds a string to a string list.
void fn_addstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: addstring list,text");

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
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: addweapon weaponname");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto weaponname = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->addWeapon(weaponname);
	}
}

// attachplayertoobj objecttype,id;
// Attaches player to object (objecttype 0 = npcs, nothing else supported).
void fn_attachplayertoobj(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: attachplayertoobj objecttype,id");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto objecttype = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto id = DoubleAsIntegralFloor<NPCID>(visitor->getGameValueAs<double>(*arguments[1]));

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
	if (arguments.size() < 2)
		throw std::invalid_argument("invalid arguments: callnpc index,eventname,params");

	NPCID sourceNPC = 0;
	if (visitor->getOriginalSource().second == ScriptObjectSourceType::NPC)
		sourceNPC = visitor->getOriginalSource().first;

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto index = DoubleAsIntegralFloor<size_t>(visitor->getGameValueAs<double>(*arguments[0]));

		std::vector<std::string> eventAndParams;
		eventAndParams.emplace_back(visitor->getGameValueAs<std::string>(*arguments[1]));
		if (arguments.size() > 2)
		{
			auto params = string::fromCSV(visitor->getGameValueAs<std::string>(*arguments[2]));
			eventAndParams.insert(eventAndParams.end(), std::ranges::begin(params), std::ranges::end(params));
		}

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
		auto carryObjectTypeId = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::CARRYSPRITE>(SetBy::SERVER, carryObjectTypeId);
	}
	*/
}

// copyflags fromprefix,toprefix;
void fn_copyflags(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("copyflags is not implemented yet.");
}

// copylevel oldfile,newfile;
void fn_copylevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("copylevel is not implemented yet.");
}

// copystrings fromprefix,toprefix;
void fn_copystrings(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("copystrings is not implemented yet.");
}

// deletelevel filename;
void fn_deletelevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("deletelevel is not implemented yet.");
}

// deletestring list,index;
// Deletes a string from a string list at the specified index.
void fn_deletestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: deletestring list,index");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto list = string::splitHard(listVar->get<std::string>().value_or(std::string{}), ","sv);
		auto index = DoubleAsIntegralFloor<size_t>(std::max(0.0, visitor->getGameValueAs<double>(*arguments[1])));

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
// Destroys and NPC.
void fn_destroy(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->getOriginalSource().second; source == ScriptObjectSourceType::NPC)
	{
		auto* server = BabyDI::Get<Server>();
		server->getNPCServer()->deleteNPC(visitor->getOriginalSource().first);
	}
}

// detachplayer;
// Detaches the player.
void fn_detachplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
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
// Configures the NPC to draw over the player.
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
// Configure the NPC to draw on the same layer as the player.
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
// Configure the NPC to draw under the player.
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
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->enableWeapons();
	}
}

// explodebomb index;
// Explodes the bomb at the specified index.
void fn_explodebomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: explodebomb index");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto index = DoubleAsIntegralFloor<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
		if (auto bomb = level->getBomb(index); bomb != nullptr)
		{
			auto power = bomb->power;
			auto position = bomb->position;
			level->removeBomb(inform_client, index);

			if (power != 2)
				level->addExplosion(inform_client, position, source::FromServer(), 1, power);
			// TODO: superbomb
		}
	}
}

// freezeplayer2;
// Freezes the player, preventing movement and actions.
void fn_freezeplayer2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->freezePlayer();
	}
}

// hide;
// Hides the NPC.
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
	throw unimplemented_error("hitcompu is not implemented yet.");
}

// hitnpc index,halfhearts,fromx,fromy;
void fn_hitnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	// Could probably emulate with move().
	throw unimplemented_error("hitnpc is not implemented yet.");
}

// hitobjects power,x,y;
// Hit objects at a location.
void fn_hitobjects(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: hitobjects power,x,y");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto power = DoubleAsIntegralFloor<int8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto x = static_cast<float>(visitor->getGameValueAs<double>(*arguments[1]));
		auto y = static_cast<float>(visitor->getGameValueAs<double>(*arguments[2]));

		auto* server = BabyDI::Get<Server>();
		server->hitObjectsAtPoint({ x, y }, power * 2, level);
	}
}

// hitplayer index,halfhearts,fromx,fromy;
// Hits a player in the level.
void fn_hitplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: hitplayer index,halfhearts,fromx,fromy");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto index = DoubleAsIntegralFloor<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto halfhearts = DoubleAsIntegralFloor<int8_t>(visitor->getGameValueAs<double>(*arguments[1]));
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
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: hurt halfhearts");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto halfhearts = DoubleAsIntegralFloor<int8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto npcId = visitor->getOriginalSource().first;
		if (visitor->getOriginalSource().second != ScriptObjectSourceType::NPC)
			npcId = 0;

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			server->hitPlayer(player->getId(), halfhearts, player->getX() + 1.5, player->getY() + 2, server->getNPC(npcId));
	}
}

// insertstring list,index,text;
// Inserts a string into a string array at the given position.
void fn_insertstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: insertstring list,index,text");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto list = string::splitHard(listVar->get<std::string>().value_or(std::string{}), ","sv);
		auto index = DoubleAsIntegralFloor<size_t>(std::max(0.0, visitor->getGameValueAs<double>(*arguments[1])));
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
		throw std::invalid_argument("invalid arguments: join class");

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
// Lays the specified item at the feet of the NPC.
void fn_lay(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: lay itemname");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto itemname = std::clamp(DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0])), 0_ui8, 24_ui8);

		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			PixelPosition layPosition{ npc->character.pixelX, npc->character.pixelY };

			// Characters lay (+0.5, +3) no matter which direction they are looking.
			if (npc->isCharacter())
				layPosition.translate(static_cast<int16_t>(8), static_cast<int16_t>(16 * 3));

			if (auto level = npc->level.lock(); level != nullptr)
				level->addItem(inform_client, layPosition, static_cast<LevelItemType>(itemname));
		}
	}
}

// lay2 itemname,x,y;
// Lays the specified item at the given x and y location.
void fn_lay2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: lay2 itemname,x,y");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto itemname = std::clamp(DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0])), 0_ui8, 24_ui8);
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);
		level->addItem(inform_client, toPixelPosition((float)x, (float)y), static_cast<LevelItemType>(itemname));
	}
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
		throw std::invalid_argument("invalid arguments: move dx,dy,time,options");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto dx = static_cast<float>(visitor->getGameValueAs<double>(*arguments[0]));
		auto dy = static_cast<float>(visitor->getGameValueAs<double>(*arguments[1]));
		auto time = static_cast<float>(visitor->getGameValueAs<double>(*arguments[2]));
		auto options = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[3]));

		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			server->moveNPC(npc, dx, dy, time, options);
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
// Creates a bomb at the specified location with the given power.
void fn_putbomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: putbomb power,x,y");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto power = std::clamp(DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0])), 1_ui8, 3_ui8);
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);
		level->addBomb(inform_client, toPixelPosition((float)x, (float)y), power);
	}
}

// putcomp baddyname,x,y;
// Adds a new baddy to the level with the specified parameters.
void fn_putcomp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: putcomp baddyname,x,y");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		uint8_t baddyname = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);
		level->putNewBaddy(toPixelPosition((float)x, (float)y), static_cast<BaddyType>(baddyname));
	}
}

// putexplosion radius,x,y;
// Creates an explosion at the specified location with the given radius.
void fn_putexplosion(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: putexplosion radius,x,y");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto radius = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);
		level->addExplosion(inform_client, toPixelPosition((float)x, (float)y), visitor->getCurrentSource(), radius, 1);
	}
}

// putexplosion2 power,radius,x,y;
// Creates an explosion at the specified location with the given power and radius.
void fn_putexplosion2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: putexplosion2 power,radius,x,y");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto power = std::clamp(DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0])), 1_ui8, 3_ui8);
		auto radius = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto x = visitor->getGameValueAs<double>(*arguments[2]);
		auto y = visitor->getGameValueAs<double>(*arguments[3]);
		level->addExplosion(inform_client, toPixelPosition((float)x, (float)y), visitor->getCurrentSource(), radius, power);
	}
}

// puthorse imagefile,x,y;
// Creates a new horse at the specified location with the given image file.
void fn_puthorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: puthorse imagefile,x,y");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto imagefile = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);
		level->addHorse(inform_client, imagefile, toPixelPosition((float)x, (float)y), 2, 0);
	}
}

// putnewcomp baddyname,x,y,imagefile,power;
// Adds a new baddy to the level with the specified parameters.
void fn_putnewcomp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::invalid_argument("invalid arguments: putnewcomp baddyname,x,y,imagefile,power");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		uint8_t baddyname = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);
		auto imagefile = visitor->getGameValueAs<std::string>(*arguments[3]);
		auto power = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[4]));
		level->putNewBaddy(toPixelPosition((float)x, (float)y), static_cast<BaddyType>(baddyname), power, imagefile);
	}
}

// putnpc imagefile,scriptfile,x,y;
// Creates a new level NPC with the specified parameters.
void fn_putnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: putnpc imagefile,scriptfile,x,y");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto imagefile = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto scriptfile = visitor->getGameValueAs<std::string>(*arguments[1]);
		auto x = visitor->getGameValueAs<double>(*arguments[2]);
		auto y = visitor->getGameValueAs<double>(*arguments[3]);

		auto* server = BabyDI::Get<Server>();
		if (auto* fs = server->getFileSystem(FS_FILE); fs != nullptr)
		{
			auto filepath = fs->findi(scriptfile);
			if (!filepath.isEmpty())
			{
				auto script = fs->load(filepath);
				server->addNPC(imagefile, script.toStringView(), x, y, level, NPCStorageType::LEVEL, true);
			}
		}
	}
}

// putnpc2 x,y,{ script };
// Creates a new database NPC at the location and with the specified script.
void fn_putnpc2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: putnpc2 x,y,{ script }");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = visitor->getGameValueAs<double>(*arguments[0]);
		auto y = visitor->getGameValueAs<double>(*arguments[1]);
		auto script = visitor->getGameValueAs<std::string>(*arguments[2]);
		string::trimMutate(script);

		auto* server = BabyDI::Get<Server>();
		server->getNPCServer()->addNPC({}, script, level, { (float)x, (float)y });
	}
}

// removearrow index;
void fn_removearrow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("removearrow is clientside only.");
}

// removebomb index;
// Removes a bomb from the level.
void fn_removebomb(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: removebomb index");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto index = DoubleAsIntegralFloor<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
		level->removeBomb(inform_client, index);
	}
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
	throw std::logic_error("removeexplo is clientside only.");
}

// removeguild guild;
void fn_removeguild(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("removeguild is not implemented yet.");
}

// removeguildmember guild,account,nick;
void fn_removeguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("removeguildmember is not implemented yet.");
}

// removehorse index;
// Removes the horse from the level at the specified index.
void fn_removehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: removehorse index");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto index = DoubleAsIntegralFloor<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
		level->removeHorse(inform_client, index);
	}
}

// removeitem index;
// Removes the item from the level at the specified index.
void fn_removeitem(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: removeitem index");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto index = DoubleAsIntegralFloor<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
		level->removeItem(inform_client, index);
	}
}

// removestring list,text;
// Removes all occurrences of the specified text from the string list.
void fn_removestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: removestring list,text");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto list = listVar->get<std::string>().value_or(std::string{});
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
// Removes the specified weapon from the player.
void fn_removeweapon(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: removeweapon weaponname");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto weaponname = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->deleteWeapon(weaponname);
	}
}

// replacestring list,index,text;
// Replaces the string at the specified index in the list with the given text.
void fn_replacestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: replacestring list,index,text");

	if (auto* listVar = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); listVar != nullptr)
	{
		auto list = listVar->get<std::string>().value_or(std::string{});
		auto index = DoubleAsIntegralFloor<size_t>(std::max(0.0, visitor->getGameValueAs<double>(*arguments[1])));
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
	throw unimplemented_error("saveinfo is not implemented yet.");
}

// savelog text;
void fn_savelog(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("savelog is not implemented yet.");
}

// savelog2 filename,text;
void fn_savelog2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("savelog2 is not implemented yet.");
}

// say signindex;
// Displays the text of a sign at the specified index to the player.
void fn_say(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: say signindex");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto signIndex = DoubleAsIntegralFloor<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
		if (signIndex < level->getSigns().size())
		{
			auto& sign = level->getSigns()[signIndex];
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
			{
				auto* server = BabyDI::Get<Server>();
				if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
					player->sendSignMessage(sign.unformattedText);
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
		std::string message;
		if (arguments.size() != 0)
		{
			message = visitor->getGameValueAs<std::string>(*arguments[0]);
			string::eraseCharsMutate(message, "\r\n"sv);
		}

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->sendSignMessage(message);
	}
}

// sendpm message;
// Sends a private message to the player.
void fn_sendpm(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: sendpm message");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto message = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->sendPrivateMessage(NPCServerPlayerID, string::replaceMutate(message, "#b", "\n"));
	}
}

// sendrpgmessage message;
// Sends a message to the F2 message window of the player.
void fn_sendrpgmessage(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		std::string message;
		if (arguments.size() != 0)
			message = visitor->getGameValueAs<std::string>(*arguments[0]);

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->sendRPGMessage(message);
	}
}

// sendtonc message;
// Sends a message to the NC (NPC Control).
void fn_sendtonc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	std::string message;
	if (arguments.size() != 0)
		message = visitor->getGameValueAs<std::string>(*arguments[0]);

	auto* server = BabyDI::Get<Server>();
	server->sendToNC(message);
}

// sendtorc message;
// Sends a message to the RC (Remote Control).
void fn_sendtorc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	std::string message;
	if (arguments.size() != 0)
		message = visitor->getGameValueAs<std::string>(*arguments[0]);

	auto* server = BabyDI::Get<Server>();
	server->sendToRC(message);
}

// serverwarp servername;
// Warps a player to a different server.
void fn_serverwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: serverwarp servername");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto servername = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			server->getServerList().sendPacket(CString() >> (char)SVO_SERVERINFO >> (short)player->getId() << servername);
	}
}

// set flag;
// Sets a flag on the player.
void fn_set(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: set flag");

	if (auto* flag = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); flag != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		if (flag->identifier.starts_with("client.") || flag->identifier.starts_with("clientr."))
		{
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
			{
				if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
// setani gani,attribs;
// Sets the animation for the player.
void fn_setani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setani gani,attribs");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto gani = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::GANI>(SetBy::SERVER, gani);
	}
}

// setarray var,size;
// Creates an array of the given size.
void fn_setarray(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: setarray var,size");

	if (auto* var = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); var != nullptr)
	{
		auto size = DoubleAsIntegralFloor<size_t>(std::max(0.0, visitor->getGameValueAs<double>(*arguments[1])));
		std::vector<double> arrayValues;
		arrayValues.assign(size, 0.0);

		var->assign(GameValue{ std::move(arrayValues) });
	}
}

// setbeltcolor color;
// Sets the player's belt color.
void fn_setbeltcolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setbeltcolor color");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setbody filename");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::BODYIMG>(SetBy::SERVER, filename);
	}
}

// setcharani gani;
// setcharani gani,attribs;
// Sets the NPC character's animation.
void fn_setcharani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setcharani gani,attribs");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto gani = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::GANI>(SetBy::SERVER, gani);
	}
}

// setchargender gender;
// Sets the NPC character's gender (controls which voice is used).
void fn_setchargender(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setchargender gender");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto gender = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
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
	if (arguments.size() == 0)
		throw std::invalid_argument("invalid arguments: setcharprop messagecode,text");

	if (auto* messagecode = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); messagecode != nullptr)
	{
		std::string text;
		if (arguments.size() == 2)
			text = visitor->getGameValueAs<std::string>(*arguments[1]);

		messagecode->assign(text);
	}
}

// setcoatcolor color;
// Sets the player's coat color.
void fn_setcoatcolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setcoatcolor color");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setgender gender");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto gender = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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

// sethead filename;
// Sets the player's head image.
void fn_sethead(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: sethead filename");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
		throw std::invalid_argument("invalid arguments: setimg filename");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::IMAGE>(SetBy::SERVER, filename);
	}
}

// setimgpart filename,x,y,width,height;
// Sets a part of the image for the NPC, allowing for more detailed control over the displayed image.
void fn_setimgpart(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::invalid_argument("invalid arguments: setimgpart filename,x,y,width,height");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto x = DoubleAsIntegralFloor<uint16_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto y = DoubleAsIntegralFloor<uint16_t>(visitor->getGameValueAs<double>(*arguments[2]));
		auto width = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[3]));
		auto height = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[4]));

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
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setlevel filename");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->warp(filename, { player->account.character.pixelX, player->account.character.pixelY });
	}
}

// setlevel2 filename,x,y;
// Warps the player to a new level specified by the filename and coordinates (x, y).
void fn_setlevel2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: setlevel2 filename,x,y");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->warp(filename, { static_cast<int16_t>(x * 16), static_cast<int16_t>(y * 16) });
	}
}

// setmap imgfile,levelsfile,x,y;
// Sets the big map for the player with the specified image file, levels file, and coordinates (x, y).
void fn_setmap(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: setmap imgfile,levelsfile,x,y");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto imgfile = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto levelsfile = visitor->getGameValueAs<std::string>(*arguments[1]);
		auto x = visitor->getGameValueAs<double>(*arguments[2]);
		auto y = visitor->getGameValueAs<double>(*arguments[3]);

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->sendPacket(CString() >> (char)PLO_BIGMAP << imgfile << "," << levelsfile << "," << CString(x) << "," << CString(y));
	}
}

// setminimap imgfile,levelsfile,x,y;
// Sets the minimap for the player with the specified image file, levels file, and coordinates (x, y).
void fn_setminimap(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: setminimap imgfile,levelsfile,x,y");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto imgfile = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto levelsfile = visitor->getGameValueAs<std::string>(*arguments[1]);
		auto x = visitor->getGameValueAs<double>(*arguments[2]);
		auto y = visitor->getGameValueAs<double>(*arguments[3]);

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->sendPacket(CString() >> (char)PLO_MINIMAP << imgfile << "," << levelsfile << "," << CString(x) << "," << CString(y));
	}
}

// setplayerdir dir;
// Sets the direction of the player sprite.
void fn_setplayerdir(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setplayerdir dir");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto dir = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: setplayerprop messagecode,text");

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
	throw unimplemented_error("setpm is not implemented yet.");
}

// setshape type,width,height;
// type 1 = rectangle
void fn_setshape(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: setshape type,width,height");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto type = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
		if (type != 1)
			return;

		auto width = DoubleAsIntegralFloor<uint16_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto height = DoubleAsIntegralFloor<uint16_t>(visitor->getGameValueAs<double>(*arguments[2]));
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->shape = { width, height };
	}
}

// setshield image,power;
// Sets the player's shield image.
void fn_setshield(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: setshield image,power");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto image = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto power = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::SHIELDPOWER>(SetBy::SERVER, image, power);
	}
}

// setshoecolor color;
// Sets the player's shoe color.
void fn_setshoecolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setshoecolor color");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
		throw std::invalid_argument("invalid arguments: setshootparams params");

	auto params = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto* server = BabyDI::Get<Server>();
	server->setShootParams(string::fromCSV(params));
}

// setskincolor color;
// Sets the player's skin color.
void fn_setskincolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setskincolor color");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
		throw std::invalid_argument("invalid arguments: setshoecolor color");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto color = visitor->getGameValueAs<double>(*arguments[0]);
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
	if (arguments.size() == 0)
		throw std::invalid_argument("invalid arguments: setstring var,text");

	// Assign the string.
	if (auto* var = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); var != nullptr)
	{
		std::string text;
		if (arguments.size() == 2)
			text = visitor->getGameValueAs<std::string>(*arguments[1]);

		// Special handling for prefixed variables.
		// Maybe think of a way to do this automatically on the assign rather than doing this.
		auto* server = BabyDI::Get<Server>();
		if (var->identifier.starts_with("client.") || var->identifier.starts_with("clientr."))
		{
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
			{
				if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: setsword image,power");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto image = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto power = DoubleAsIntegralFloor<int8_t>(visitor->getGameValueAs<double>(*arguments[1]));
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::SWORDPOWER>(SetBy::SERVER, image, power);
	}
}

// setz x,y,width,height,a,b,c,d;
void fn_setz(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("setz is not implemented yet.");
}

// shoot x,y,z,angle,zangle,power,gani,ganiparams;
// Creates a shoot style projectile.
void fn_shoot(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() < 7)
		throw std::invalid_argument("invalid arguments: shoot x,y,z,angle,zangle,power,gani,ganiparams");

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
// Shoots an arrow in the specified direction.
void fn_shootarrow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: shootarrow dir");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto dir = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));

		auto server = BabyDI::Get<Server>();
		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = { (dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16) };

		auto sourcePosition = getPositionForArrow(source, dir);
		if (!sourcePosition.has_value())
			return;

		level->addArrow(inform_client, sourcePosition.value(), speed, dir, arrowTypeNormal, source);
	}
}

// shootball;
// (gr) shootball dir;
void fn_shootball(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	// TODO(GS1): Conformance modes.

	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: shootball dir");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto dir = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));

		auto server = BabyDI::Get<Server>();
		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = { (dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16) };

		auto sourcePosition = getPositionForArrow(source, dir);
		if (!sourcePosition.has_value())
			return;

		level->addArrow(inform_client, sourcePosition.value(), speed, dir, arrowTypeBall, source);
	}
}

// shootfireball dir;
// Shoots a fireball in the specified direction.
void fn_shootfireball(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: shootfireball dir");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto dir = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));

		auto server = BabyDI::Get<Server>();
		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = { (dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16) };

		auto sourcePosition = getPositionForArrow(source, dir);
		if (!sourcePosition.has_value())
			return;

		level->addArrow(inform_client, sourcePosition.value(), speed, dir, arrowTypeFireball, source);
	}
}

// shootfireblast dir;
// Shoots a fireblast in the specified direction.
void fn_shootfireblast(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: shootfireblast dir");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto dir = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));

		auto server = BabyDI::Get<Server>();
		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = { (dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16) };

		auto sourcePosition = getPositionForArrow(source, dir);
		if (!sourcePosition.has_value())
			return;

		level->addArrow(inform_client, sourcePosition.value(), speed, dir, arrowTypeFireblast, source);
	}
}

// shootnuke dir;
// Shoots a nuke in the specified direction.
void fn_shootnuke(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: shootnuke dir");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto dir = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));

		auto server = BabyDI::Get<Server>();
		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = { (dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16) };

		auto sourcePosition = getPositionForArrow(source, dir);
		if (!sourcePosition.has_value())
			return;

		level->addArrow(inform_client, sourcePosition.value(), speed, dir, arrowTypeNukeshot, source);
	}
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
	throw unimplemented_error("showani is not implemented yet.");
}

// showani2 index,x,y,z,direction,gani,params;
void fn_showani2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("showani2 is not implemented yet.");
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
	throw unimplemented_error("showimg is not implemented yet.");
}

// showimg2 index,filename,x,y,z;
void fn_showimg2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("showimg2 is not implemented yet.");
}

// showstats bitflag;
void fn_showstats(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("showstats is not implemented yet.");
}

// sleep duration;
void fn_sleep(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("sleep is not implemented yet.");
}

// spyfire length,power;
// Sends a spyfire explosion from the player.
void fn_spyfire(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: spyfire length,power");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto length = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0])) & 0b11111;
		auto power = DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[1])) & 0b111;
		uint8_t length_power = (length << 3) | power;

		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
		{
			server->sendPacketToLevelArea(CString() >> (char)PLO_FIRESPY >> (short)source.value().first >> (char)(length_power), player);
			if (auto level = player->getLevel(); level != nullptr)
				level->addSpyFire({ player->account.character.pixelX, player->account.character.pixelY }, source.value(), player->account.character.direction, length, power);
		}
	}
}

// take itemname;
// Takes an item on the level in a 10-tile radius from the NPC.
void fn_take(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: take itemname");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->level.lock(); level != nullptr)
			{
				auto itemname = std::clamp(DoubleAsIntegralFloor<uint8_t>(visitor->getGameValueAs<double>(*arguments[0])), 0_ui8, 24_ui8);

				// Get our search position.
				PixelPosition searchPosition = { npc->character.pixelX, npc->character.pixelY };
				if (npc->isCharacter())
					searchPosition.translate(static_cast<int16_t>(8), static_cast<int16_t>(16 * 3));

				// Find all the items within 10 tiles of the NPC.
				std::vector<uint8_t> itemIndices;
				auto& levelItems = level->getItems();
				for (size_t i = levelItems.size(); i > 0; --i)
				{
					auto& item = levelItems[i - 1];
					if (PROPID(item.item) != itemname)
						continue;

					auto distance = static_cast<int32_t>(std::hypot(item.position.x() - searchPosition.x(), item.position.y() - searchPosition.y()));
					if (distance <= (10 * 16))
					{
						itemIndices.push_back(static_cast<uint8_t>(i - 1));
						if (LevelItem::isRupeeType(item.item))
							npc->setPropWith<NPCProp::RUPEES>(SetBy::SERVER, npc->getProp<NPCProp::RUPEES>().value + LevelItem::GetRupeeCount(item.item));
						else if (item.item == LevelItemType::HEART)
							npc->setPropWith<NPCProp::POWER>(SetBy::SERVER, static_cast<GBYTE1>(npc->getProp<NPCProp::POWER>().value + 2));
						else if (item.item == LevelItemType::DARTS)
							npc->setPropWith<NPCProp::ARROWS>(SetBy::SERVER, static_cast<GBYTE1>(npc->getProp<NPCProp::ARROWS>().value + 5));
						else if (item.item == LevelItemType::BOMBS)
							npc->setPropWith<NPCProp::BOMBS>(SetBy::SERVER, static_cast<GBYTE1>(npc->getProp<NPCProp::BOMBS>().value + 5));
						else if (item.item == LevelItemType::GLOVE1)
							npc->setPropWith<NPCProp::GLOVEPOWER>(SetBy::SERVER, std::max(npc->getProp<NPCProp::GLOVEPOWER>().value, 1_ui8));
						else if (item.item == LevelItemType::GLOVE2)
							npc->setPropWith<NPCProp::GLOVEPOWER>(SetBy::SERVER, std::max(npc->getProp<NPCProp::GLOVEPOWER>().value, 2_ui8));
					}
				}

				// Remove all taken items.
				for (auto& index : itemIndices)
					level->removeItem(inform_client, index);
			}
		}
	}
}

// take2 index;
// Takes an item at the specified index on the level.
void fn_take2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: take2 index");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->level.lock(); level != nullptr)
			{
				auto index = DoubleAsIntegralFloor<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
				if (auto item = level->getItem(index); item != nullptr)
				{
					if (LevelItem::isRupeeType(item->item))
						npc->setPropWith<NPCProp::RUPEES>(SetBy::SERVER, npc->getProp<NPCProp::RUPEES>().value + LevelItem::GetRupeeCount(item->item));
					else if (item->item == LevelItemType::HEART)
						npc->setPropWith<NPCProp::POWER>(SetBy::SERVER, static_cast<GBYTE1>(npc->getProp<NPCProp::POWER>().value + 2));
					else if (item->item == LevelItemType::DARTS)
						npc->setPropWith<NPCProp::ARROWS>(SetBy::SERVER, static_cast<GBYTE1>(npc->getProp<NPCProp::ARROWS>().value + 5));
					else if (item->item == LevelItemType::BOMBS)
						npc->setPropWith<NPCProp::BOMBS>(SetBy::SERVER, static_cast<GBYTE1>(npc->getProp<NPCProp::BOMBS>().value + 5));
					else if (item->item == LevelItemType::GLOVE1)
						npc->setPropWith<NPCProp::GLOVEPOWER>(SetBy::SERVER, std::max(npc->getProp<NPCProp::GLOVEPOWER>().value, 1_ui8));
					else if (item->item == LevelItemType::GLOVE2)
						npc->setPropWith<NPCProp::GLOVEPOWER>(SetBy::SERVER, std::max(npc->getProp<NPCProp::GLOVEPOWER>().value, 2_ui8));

					level->removeItem(inform_client, index);
				}
			}
		}
	}
}

// takehorse index;
void fn_takehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("takehorse is not implemented yet.");
}

// takeplayercarry;
// Takes the carried object from the player.
void fn_takeplayercarry(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::CARRYNPC>(SetBy::SERVER, static_cast<uint32_t>(0));
	}
	*/
}

// timershow;
void fn_timershow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("timershow is not implemented yet.");
}

// tokenize text;
// Tokenizes a string into tokens using spaces as a delimiter.
void fn_tokenize(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: tokenize text");

	auto text = visitor->getGameValueAs<std::string>(*arguments[0]);
	visitor->tokenizeTokens = string::splitHard(text, " "sv);
}

// tokenize2 delims,text;
// Tokenizes a string into tokens using the specified delimiters.
void fn_tokenize2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: tokenize2 delims,text");

	auto delims = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto text = visitor->getGameValueAs<std::string>(*arguments[1]);
	visitor->tokenizeTokens = string::splitHard(text, delims);
}

// triggeraction x,y,action,params;
// Sends out a trigger action.
void fn_triggeraction(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() < 4)
		throw std::invalid_argument("invalid arguments: triggeraction x,y,action,params");

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
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->unfreezePlayer();
	}
}

// unset flag;
// Unsets a player's flag.
void fn_unset(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: unset flag");

	if (auto* flag = visitor->getGameVariableFromGS1ScriptValue(*arguments[0]); flag != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		if (flag->identifier.starts_with("client.") || flag->identifier.starts_with("clientr."))
		{
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER); source.has_value())
			{
				if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
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
	throw unimplemented_error("updateboard is not implemented yet.");
}

// updateboard2 x,y,width,height;
void fn_updateboard2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("updateboard2 is not implemented yet.");
}

// updateterrain;
void fn_updateterrain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("updateterrain is not implemented yet.");
}

// warpto levelname,x,y;
// Warps an NPC to a new level.
void fn_warpto(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: warpto levelname,x,y");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
	{
		auto filename = visitor->getGameValueAs<std::string>(*arguments[0]);
		auto x = visitor->getGameValueAs<double>(*arguments[1]);
		auto y = visitor->getGameValueAs<double>(*arguments[2]);

		auto* server = BabyDI::Get<Server>();
		if (auto level = server->getLevel(filename); level != nullptr)
		{
			if (auto npc = server->getNPC(source.value().first); npc != nullptr)
				npc->warp(level, { static_cast<int16_t>(x * 16), static_cast<int16_t>(y * 16) });
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
