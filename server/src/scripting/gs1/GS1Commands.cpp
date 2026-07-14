#include <algorithm>
#include <any>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tree/ParseTree.h>
#include <tree/ParseTreeType.h>

#include <CString.h>
#include <IEnums.h>

#include <BabyDI.h>
#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
#include <level/LevelArrow.h>
#include <level/LevelBaddy.h>
#include <level/LevelItem.h>
#include <npcserver/NPCServer.h>
#include <object/Character.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>
#include <utilities/manager/GuildManager.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

using BuiltInCommandHandleFunc = void (*)(GS1Visitor*, std::string_view, const std::vector<GS1ScriptValue*>&);
using BuiltInCommandHandleMap = std::unordered_map<size_t, BuiltInCommandHandleFunc>;

#if DEBUG
static void fn_debugger(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
#endif

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
static void fn_changeimgcolors(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_changeimgmode(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_changeimgpart(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_changeimgvis(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_changeimgzoom(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
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
static void fn_hideimg(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_hideimgs(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
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
static void fn_seteffect(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
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
static void fn_showpoly(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showpoly2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showstats(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showtext(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_showtext2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
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
static void fn_toweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_triggeraction(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_unfreezeplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_unset(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_updateboard(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_updateboard2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_updateterrain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_warpto(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
// GR extensions
static void fn_enabledamagereactions(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);
static void fn_disabledamagereactions(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments);

static BuiltInCommandHandleMap GenerateMap()
{
	string::string_hash hash{};
	BuiltInCommandHandleMap map =
	{
#if DEBUG
		{hash("gr-debugger"), &fn_debugger},
#endif
		{hash("addguildmember"), &fn_addguildmember},
		{hash("addstring"), &fn_addstring},
		{hash("addweapon"), &fn_addweapon},
		{hash("attachplayertoobj"), &fn_attachplayertoobj},
		{hash("blockagain"), &fn_blockagain},
		{hash("callnpc"), &fn_callnpc},
		{hash("canbecarried"), &fn_canbecarried},
		{hash("canbepulled"), &fn_canbepulled},
		{hash("canbepushed"), &fn_canbepushed},
		{hash("cannotbecarried"), &fn_cannotbecarried},
		{hash("cannotbepulled"), &fn_cannotbepulled},
		{hash("cannotbepushed"), &fn_cannotbepushed},
		{hash("cannotwarp"), &fn_cannotwarp},
		{hash("canwarp"), &fn_canwarp},
		{hash("canwarp2"), &fn_canwarp2},
		{hash("carryobject"), &fn_carryobject},
		{hash("changeimgcolors"), &fn_changeimgcolors},
		{hash("changeimgmode"), &fn_changeimgmode},
		{hash("changeimgpart"), &fn_changeimgpart},
		{hash("changeimgvis"), &fn_changeimgvis},
		{hash("changeimgzoom"), &fn_changeimgzoom},
		{hash("copylevel"), &fn_copylevel},
		{hash("copystrings"), &fn_copystrings},
		{hash("deletelevel"), &fn_deletelevel},
		{hash("deletestring"), &fn_deletestring},
		{hash("destroy"), &fn_destroy},
		{hash("detachplayer"), &fn_detachplayer},
		{hash("disableweapons"), &fn_disableweapons},
		{hash("dontblock"), &fn_dontblock},
		{hash("drawoverplayer"), &fn_drawoverplayer},
		{hash("drawovertrees"), &fn_drawovertrees},
		{hash("drawunderplayer"), &fn_drawunderplayer},
		{hash("enableweapons"), &fn_enableweapons},
		{hash("explodebomb"), &fn_explodebomb},
		{hash("freezeplayer2"), &fn_freezeplayer2},
		{hash("hide"), &fn_hide},
		{hash("hideimg"), &fn_hideimg},
		{hash("hideimgs"), &fn_hideimgs},
		{hash("hitcompu"), &fn_hitcompu},
		{hash("hitnpc"), &fn_hitnpc},
		{hash("hitobjects"), &fn_hitobjects},
		{hash("hitplayer"), &fn_hitplayer},
		{hash("hurt"), &fn_hurt},
		{hash("insertstring"), &fn_insertstring},
		{hash("join"), &fn_join},
		{hash("lay"), &fn_lay},
		{hash("lay2"), &fn_lay2},
		{hash("message"), &fn_message},
		{hash("move"), &fn_move},
		{hash("noplayeronwall"), &fn_noplayeronwall},
		{hash("putbomb"), &fn_putbomb},
		{hash("putcomp"), &fn_putcomp},
		{hash("putexplosion"), &fn_putexplosion},
		{hash("putexplosion2"), &fn_putexplosion2},
		{hash("puthorse"), &fn_puthorse},
		{hash("putnewcomp"), &fn_putnewcomp},
		{hash("putnpc"), &fn_putnpc},
		{hash("putnpc2"), &fn_putnpc2},
		{hash("removearrow"), &fn_removearrow},
		{hash("removebomb"), &fn_removebomb},
		{hash("removecompus"), &fn_removecompus},
		{hash("removeexplo"), &fn_removeexplo},
		{hash("removeguild"), &fn_removeguild},
		{hash("removeguildmember"), &fn_removeguildmember},
		{hash("removehorse"), &fn_removehorse},
		{hash("removeitem"), &fn_removeitem},
		{hash("removestring"), &fn_removestring},
		{hash("removeweapon"), &fn_removeweapon},
		{hash("replacestring"), &fn_replacestring},
		{hash("saveinfo"), &fn_saveinfo},
		{hash("savelog"), &fn_savelog},
		{hash("savelog2"), &fn_savelog2},
		{hash("say"), &fn_say},
		{hash("say2"), &fn_say2},
		{hash("sendpm"), &fn_sendpm},
		{hash("sendrpgmessage"), &fn_sendrpgmessage},
		{hash("sendtonc"), &fn_sendtonc},
		{hash("sendtorc"), &fn_sendtorc},
		{hash("serverwarp"), &fn_serverwarp},
		{hash("set"), &fn_set},
		{hash("setani"), &fn_setani},
		{hash("setarray"), &fn_setarray},
		{hash("setbeltcolor"), &fn_setbeltcolor},
		{hash("setbody"), &fn_setbody},
		{hash("setcharani"), &fn_setcharani},
		{hash("setchargender"), &fn_setchargender},
		{hash("setcharprop"), &fn_setcharprop},
		{hash("setcoatcolor"), &fn_setcoatcolor},
		{hash("seteffect"), &fn_seteffect},
		{hash("setgender"), &fn_setgender},
		{hash("setgif"), &fn_setimg},
		{hash("setgifpart"), &fn_setimgpart},
		{hash("sethead"), &fn_sethead},
		{hash("setimg"), &fn_setimg},
		{hash("setimgpart"), &fn_setimgpart},
		{hash("setlevel"), &fn_setlevel},
		{hash("setlevel2"), &fn_setlevel2},
		{hash("setmap"), &fn_setmap},
		{hash("setminimap"), &fn_setminimap},
		{hash("setplayerdir"), &fn_setplayerdir},
		{hash("setplayerprop"), &fn_setplayerprop},
		{hash("setpm"), &fn_setpm},
		{hash("setshape"), &fn_setshape},
		{hash("setshield"), &fn_setshield},
		{hash("setshoecolor"), &fn_setshoecolor},
		{hash("setshootparams"), &fn_setshootparams},
		{hash("setskincolor"), &fn_setskincolor},
		{hash("setsleevecolor"), &fn_setsleevecolor},
		{hash("setstring"), &fn_setstring},
		{hash("setsword"), &fn_setsword},
		{hash("setz"), &fn_setz},
		{hash("shoot"), &fn_shoot},
		{hash("shootarrow"), &fn_shootarrow},
		{hash("shootball"), &fn_shootball},
		{hash("shootfireball"), &fn_shootfireball},
		{hash("shootfireblast"), &fn_shootfireblast},
		{hash("shootnuke"), &fn_shootnuke},
		{hash("show"), &fn_show},
		{hash("showani"), &fn_showani},
		{hash("showani2"), &fn_showani2},
		{hash("showcharacter"), &fn_showcharacter},
		{hash("showimg"), &fn_showimg},
		{hash("showimg2"), &fn_showimg2},
		{hash("showpoly"), &fn_showpoly},
		{hash("showpoly2"), &fn_showpoly2},
		{hash("showstats"), &fn_showstats},
		{hash("showtext"), &fn_showtext},
		{hash("showtext2"), &fn_showtext2},
		{hash("sleep"), &fn_sleep},
		{hash("spyfire"), &fn_spyfire},
		{hash("take"), &fn_take},
		{hash("take2"), &fn_take2},
		{hash("takehorse"), &fn_takehorse},
		{hash("takeplayercarry"), &fn_takeplayercarry},
		{hash("takeplayerhorse"), &fn_takeplayerhorse},
		{hash("throwcarry"), &fn_throwcarry},
		{hash("timershow"), &fn_timershow},
		{hash("tokenize"), &fn_tokenize},
		{hash("tokenize2"), &fn_tokenize2},
		{hash("toweapons"), &fn_toweapons},
		{hash("triggeraction"), &fn_triggeraction},
		{hash("unfreezeplayer"), &fn_unfreezeplayer},
		{hash("unset"), &fn_unset},
		{hash("updateboard"), &fn_updateboard},
		{hash("updateboard2"), &fn_updateboard2},
		{hash("updateterrain"), &fn_updateterrain},
		{hash("warpto"), &fn_warpto},
		// GR extensions
		{hash("enabledamagereactions"), &fn_enabledamagereactions},
		{hash("disabledamagereactions"), &fn_disabledamagereactions},
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

constexpr std::array<std::string_view, 2> translatableCommands =
{
	"say2"sv,
	"sendpm"sv,
};

///////////////////////////////////////////////////////////////////////////////

static std::any translateStringForPlayer(antlr4::tree::ParseTree* node, GS1Visitor* visitor, PlayerPtr player)
{
	if (node == nullptr)
		return std::any{};
	if (visitor == nullptr || player == nullptr || node->getTreeType() != antlr4::tree::ParseTreeType::RULE)
		return node->accept(visitor);

	return GS1ScriptValue{visitor->translateSourceText(node, player->account.language)};
}

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
		log::printLine(log::script, "Unknown command in NPC [{}] '{}': {}", visitor->getOriginalSource().first, visitor->who, commandName);
		return;
	}

	// Find the nearest player.  We use this for a couple calculations.
	std::optional<ScriptObject> player = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER);

	// Special case for 'setplayerprop' and 'setcharprop', which need to push a unique context onto the stack.
	// We need to bring the relevant context to the front so the message code links to the correct player or NPC, since it can touch both.
	bool popContext = false;
	if (commandName == "setcharprop")
	{
		auto npc = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC);
		if (!npc.has_value())
			npc = visitor->getOriginalSource();

		visitor->pushSource(npc.value());
		popContext = true;
	}
	else if (commandName == "setplayerprop")
	{
		if (!player.has_value())
		{
			if (visitor->getEvent().initiator.second != ScriptObjectType::PLAYER)
				return;
			player = visitor->getEvent().initiator;
		}

		visitor->pushSource(player.value());
		popContext = true;
	}

	// Record if we are expecting a flag for the next argument.
	visitor->expectingFlag = (std::ranges::find(flagProcessingCommands, commandName) != std::ranges::end(flagProcessingCommands));
	bool isTranslatable = std::ranges::contains(translatableCommands, commandName);

	std::vector<GS1ScriptValue*> arguments;
	std::vector<std::any> keepAlive;

	// Helper to package a value and keep it alive for the duration of the command execution.
	auto makeValue = [&](std::any&& anyValue)
	{
		if (!anyValue.has_value())
			return;

		keepAlive.emplace_back(std::move(anyValue));
		auto* container = std::any_cast<GS1ScriptValue>(&keepAlive.back());
		if (container == nullptr)
			throw std::runtime_error("BuiltInCommand argument is not a valid GS1ScriptValue");

		arguments.push_back(std::move(container));
	};

	// Save the player pointer so we don't keep searching for it.
	PlayerPtr playerPtr = nullptr;
	auto server = BabyDI::Get<Server>();
	if (isTranslatable && player.has_value())
		playerPtr = server->getPlayer(player.value().first);

	// Collect the arguments from the node.
	for (size_t i = 0; i < node->children.size(); ++i)
	{
		// If the command is translatable, run it through the translation process before packaging the value.
		if (isTranslatable && visitor->expectingFlag == false && player.has_value())
		{
			if (auto stringContext = visitor->walkToContext(node->children[i]); stringContext != nullptr)
			{
				makeValue(translateStringForPlayer(stringContext, visitor, playerPtr));
				continue;
			}
		}

		makeValue(node->children[i]->accept(visitor));
	}

	// Unset the expecting flag.
	visitor->expectingFlag = false;

	try
	{
		// Execute the command.
		it->second(visitor, commandName, arguments);
	}
	catch (const sleep_exception&)
	{
		throw;
	}
	catch (const return_exception&)
	{
		throw;
	}
	catch (const std::logic_error& ex)
	{
		auto server = BabyDI::Get<Server>();
		log::printLine(log::npc, "[WARNING] NPC [{}] '{}', error: {}", visitor->getOriginalSource().first, visitor->who, ex.what());
		server->sendToNC(std::format("Script problem: NPC [{}] '{}', issue: {}", visitor->getOriginalSource().first, visitor->who, ex.what()));
	}
	catch (const std::exception& ex)
	{
		auto server = BabyDI::Get<Server>();
		log::printLine(log::npc, "[ERROR] NPC [{}] '{}', error: {}", visitor->getOriginalSource().first, visitor->who, ex.what());
		server->sendToNC(std::format("Script error: NPC [{}] '{}', error: {}", visitor->getOriginalSource().first, visitor->who, ex.what()));
		throw;
	}

	// If we pushed a context, we need to pop it after the command execution.
	if (popContext)
		visitor->popSource();
}

///////////////////////////////////////////////////////////////////////////////

static std::optional<PixelPosition> getPositionForArrow(const ScriptObject& source, uint8_t dir)
{
	auto server = BabyDI::Get<Server>();
	if (source.second == ScriptObjectType::NPC)
	{
		if (auto npc = server->getNPC(source.first); npc != nullptr)
		{
			PixelPosition sourcePosition = npc->character.getGlobalPosition();
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

#if DEBUG
void fn_debugger(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	auto server = BabyDI::Get<Server>();
	const auto& sourceNPC = visitor->getOriginalSource();
	auto sourcePlayer = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER);
	NPCPtr npc = nullptr;
	PlayerPtr player = nullptr;
	if (sourceNPC.second == ScriptObjectType::NPC)
		npc = server->getNPC(sourceNPC.first);
	if (sourcePlayer.has_value())
		player = server->getNPCServer()->getPlayer(sourcePlayer.value().first);

	//player->setPropWith<PlayerProp::ID>(SetBy::SERVER, 0_ui16);
}
#endif

// addguildmember guild,account,nick;
void fn_addguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: addguildmember guild,account,nick");

	auto guild = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{});
	auto account = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(std::string{});
	auto nick = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[2]).value_or(std::string{});

	if (auto guildManager = BabyDI::Get<GuildManager>(); guildManager)
		guildManager->addPlayerToGuild(guild, account, nick);
}

// addstring list,text;
// Adds a string to a string list.
void fn_addstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: addstring list,text");

	if (auto listVar = GS1Visitor::getGameVariable(*arguments[0]); listVar != nullptr)
	{
		auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(std::string{});
		string::wrapQuotesMutate(text);

		auto list = listVar->get<std::string>();
		if (list.has_value())
		{
			if (!list.value().get().empty())
				list.value().get() += ",";
			list.value().get() += text;
		}
		else
		{
			listVar->assign<std::string>(std::move(text));
		}
	}
}

// addweapon weaponname;
// Adds a weapon from a database to your inventory.
void fn_addweapon(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: addweapon weaponname");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto weaponname = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{});
		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto objecttype = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto id = DoubleAsIntegralFloor<NPCID>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));

		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::ATTACHNPC>(SetBy::SERVER, id, objecttype);
	}
}

// blockagain;
// Enables collision.
void fn_blockagain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
	if (visitor->getOriginalSource().second == ScriptObjectType::NPC)
		sourceNPC = visitor->getOriginalSource().first;

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

		std::vector<std::string> eventAndParams;
		eventAndParams.emplace_back(GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(std::string{}));

		// Don't allow the created event to be called.
		// Disabled in client version 2.10.
		if (string::equalsi(eventAndParams[0], "created"sv))
			return;

		if (arguments.size() > 2)
		{
			auto params = string::fromCSV(GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[2]).value_or(std::string{}));
			eventAndParams.insert(eventAndParams.end(), std::ranges::begin(params), std::ranges::end(params));
		}

		auto server = BabyDI::Get<Server>();
		if (index < level->getNPCs().size())
		{
			auto& mapNPCs = level->getNPCs();
			auto iter = mapNPCs.begin();
			std::ranges::advance(iter, index, mapNPCs.end());
			if (iter != mapNPCs.end())
			{
				if (auto npc = server->getNPC(*iter); npc != nullptr)
					npc->scripting.events.addEvent(ScriptEventType::CUSTOM, source::FromNPC(sourceNPC), eventAndParams);
			}
		}
	}
}

// canbecarried;
// Flags as carryable.
void fn_canbecarried(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->warpRestrictions = NPCWarpRestrictions::NOTALLOWED;
	}
}

// canwarp;
// Flags as being able to change levels by touching any links.
void fn_canwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->warpRestrictions = NPCWarpRestrictions::ALLOWED;
	}
}

// canwarp2;
// Flags as being able to change levels by using level-edge links.
void fn_canwarp2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->warpRestrictions = NPCWarpRestrictions::ONLYOVERWORLD;
	}
}

// carryobject carryobjecttype;
// Sets the carry object type of the NPC.
void fn_carryobject(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	// TODO: There is no NPC prop for the carry image type.  We may have to investigate official.

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		[[maybe_unused]] auto carryObjectTypeId = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr && npc->isCharacter())
			npc->setPropWith<NPCProp::GANI>(SetBy::SERVER, "carrystill"s);
	}
}

// changeimgcolors index,red,green,blue,alpha;
// Sets the RGBA colors of the showimg.
void fn_changeimgcolors(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::invalid_argument("invalid arguments: changeimgcolors index,red,green,blue,alpha");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto red = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);
			auto green = GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0);
			auto blue = GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0);
			auto alpha = GS1Visitor::getScriptValueAsCopy<double>(*arguments[4]).value_or(0.0);

			server->getNPCServer()->changeShowImgColors(npc, index, red, green, blue, alpha);
		}
	}
}

// changeimgmode index,mode;
// Sets the drawing mode of the showimg.
void fn_changeimgmode(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: changeimgmode index,mode");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto mode = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));

			server->getNPCServer()->changeShowImgMode(npc, index, mode);
		}
	}
}

// changeimgpart index,x,y,width,height;
// Sets the image part of the showimg.
void fn_changeimgpart(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::invalid_argument("invalid arguments: changeimgpart index,x,y,width,height");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto x = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
			auto y = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
			auto width = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0));
			auto height = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[4]).value_or(0.0));

			server->getNPCServer()->changeShowImgPart(npc, index, ImagePartRectangle{{x, y}, {width, height}});
		}
	}
}

// changeimgvis index,drawingheight;
// Sets the layer of the showimg.
void fn_changeimgvis(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: changeimgvis index,drawingheight");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto drawingheight = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));

			server->getNPCServer()->changeShowImgLayer(npc, index, drawingheight);
		}
	}
}

// changeimgzoom index,zoomfactor;
// Sets the zoom of the showimg.
void fn_changeimgzoom(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: changeimgzoom index,zoomfactor");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto zoomfactor = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));

			server->getNPCServer()->changeShowImgZoom(npc, index, zoomfactor);
		}
	}
}

// copylevel oldfile,newfile;
// Makes a copy of a level under a new file name.
void fn_copylevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: copylevel oldfile,newfile");

	auto oldfile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{});
	auto newfile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(std::string{});

	auto server = BabyDI::Get<Server>();
	auto& fs = server->getFileSystem();
	if (auto foundInfo = fs.infoi(fs::FileCategory::LEVEL, oldfile); foundInfo != nullptr)
		std::filesystem::copy_file(foundInfo->file, foundInfo->file.parent_path() / newfile, std::filesystem::copy_options::overwrite_existing);
}

// copystrings fromprefix,toprefix;
// Copies strings that start with fromprefix and replaces the prefix with toprefix.
void fn_copystrings(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: copystrings fromprefix,toprefix");

	auto fromPrefix = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{});
	auto toPrefix = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(std::string{});

	size_t fromStorageType = GS1Visitor::getStorageTypeFromIdentifier(fromPrefix).value_or(ENUM(StorageType::CLIENT));
	size_t toStorageType = GS1Visitor::getStorageTypeFromIdentifier(toPrefix).value_or(ENUM(StorageType::CLIENT));
	GS1Visitor::stripStorageNameFromIdentifier(fromPrefix);
	GS1Visitor::stripStorageNameFromIdentifier(toPrefix);

	auto fromStore = visitor->getGameVariableStoreForStorageType(fromStorageType);
	auto toStore = visitor->getGameVariableStoreForStorageType(toStorageType);
	if (fromStore == nullptr || toStore == nullptr)
		return;

	for (auto& [key, value] : fromStore->store)
	{
		if (key.starts_with(fromPrefix))
		{
			auto toKey = std::format("{}{}", toPrefix, key.substr(fromPrefix.size()));
			toStore->add(toKey, GameValue{value->value});
		}
	}
}

// deletelevel filename;
void fn_deletelevel(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: deletelevel filename");

	auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{});

	auto server = BabyDI::Get<Server>();
	if (auto level = server->getLoadedLevelNoHint(filename); level != nullptr)
	{
		for (auto playerId : level->getPlayers())
			server->warpPlayerToSafePlace(playerId);

		auto path = server->getFileSystem().find(fs::FileCategory::LEVEL, level->levelName);
		std::filesystem::remove(path);
	}
}

// deletestring list,index;
// Deletes a string from a string list at the specified index.
void fn_deletestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: deletestring list,index");

	if (auto listVar = GS1Visitor::getGameVariable(*arguments[0]); listVar != nullptr)
	{
		auto value = listVar->get<std::string>();
		if (!value.has_value() || value.value().get().empty())
			return;

		auto list = string::fromCSV(value.value().get());
		auto index = DoubleAsIntegralFloor<size_t>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0)));

		// Check for out of bounds.
		if (index >= list.size())
			return;

		// Move the iterator to the index we want to delete.
		auto it = list.begin();
		std::advance(it, index);

		// Delete the string.
		list.erase(it);

		// Write it back.
		value.value().get() = string::toCSV(list);
	}
}

// destroy;
// Destroys an NPC.
void fn_destroy(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->getOriginalSource(); source.second == ScriptObjectType::NPC)
	{
		auto server = BabyDI::Get<Server>();
		if (server->getSettings().get<bool>("protectdbnpcs").value_or(true))
		{
			if (auto npc = server->getNPC(source.first); npc != nullptr && npc->storageType == NPCStorageType::DATABASE && npc->scriptType != NPCTYPE_LOCAL && npc->scriptType != NPCTYPE_ITEM)
			{
				log::printLine(log::npc, "NPC '{}' attempted to destroy itself, but DB NPCs are protected.", npc->name);
				return;
			}
		}

		server->getNPCServer()->deleteNPC(source.first);

		// NPC execution stops after destroy is called.
		throw return_exception{};
	}
}

// detachplayer;
// Detaches the player.
void fn_detachplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::ATTACHNPC>(SetBy::SERVER, static_cast<NPCID>(0), 0_ui8);
	}
}

// disableweapons;
// Disables the player's weapons.
void fn_disableweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->disableWeapons();
	}
}

// dontblock;
// Disables collision.
void fn_dontblock(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			uint8_t newVisFlags = npc->visFlags & ~(PROPID(NPCVisFlags::DRAWUNDERPLAYER) | PROPID(NPCVisFlags::DRAWOVERPLAYER));
			newVisFlags |= (PROPID(NPCVisFlags::DRAWOVERPLAYER) | PROPID(NPCVisFlags::VISIBLE));
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, newVisFlags);
		}
	}
}

// drawovertrees;
// Configure the NPC to draw on the same layer as the player.
void fn_drawovertrees(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			uint8_t newVisFlags = npc->visFlags & ~(PROPID(NPCVisFlags::DRAWUNDERPLAYER) | PROPID(NPCVisFlags::DRAWOVERPLAYER));
			newVisFlags |= (PROPID(NPCVisFlags::DRAWUNDERPLAYER) | PROPID(NPCVisFlags::VISIBLE));
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, newVisFlags);
		}
	}
}

// enableweapons;
// Enables the player's weapons.
void fn_enableweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
		auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		if (auto bomb = level->getBomb(index); bomb.has_value())
		{
			auto& power = bomb.value()->power;
			auto& position = bomb.value()->position;
			level->removeBomb(inform_client, index);

			if (power != 2)
				level->addExplosion(inform_client, position, source::FromServer(), 2, power);
			else
			{
				// Superbomb is 5 explosions.
				// The center explosion is a size of 4.  The others are a size of 2.
				level->addExplosion(inform_client, position, source::FromServer(), 4, power);
				level->addExplosion(inform_client, translatePosition(position, -32, -32), source::FromServer(), 2, power);
				level->addExplosion(inform_client, translatePosition(position, 32, -32), source::FromServer(), 2, power);
				level->addExplosion(inform_client, translatePosition(position, -32, 32), source::FromServer(), 2, power);
				level->addExplosion(inform_client, translatePosition(position, 32, 32), source::FromServer(), 2, power);
			}
		}
	}
}

// freezeplayer2;
// Freezes the player, preventing movement and actions.
void fn_freezeplayer2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->freezePlayer();
	}
}

// hide;
// Hides the NPC.
void fn_hide(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, static_cast<uint8_t>(npc->visFlags & ~PROPID(NPCVisFlags::VISIBLE)));
	}
}

// hideimg index;
// Removes the image at the specified index.
void fn_hideimg(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: hideimg index");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			server->getNPCServer()->hideImages(npc, index);
		}
	}
}

// hideimgs indexstart,indexend;
// Removes the images in the specified range.
void fn_hideimgs(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: hideimgs indexstart,indexend");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto indexstart = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto indexend = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
			server->getNPCServer()->hideImages(npc, indexstart, indexend);
		}
	}
}

// hitcompu index,power,fromx,fromy;
void fn_hitcompu(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("hitcompu is not implemented yet.");
}

// hitnpc index,halfhearts,fromx,fromy;
// Hits the specified NPC.
void fn_hitnpc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: hitnpc index,halfhearts,fromx,fromy");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto halfhearts = DoubleAsIntegralFloor<int8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto fromx = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		auto fromy = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0));

		if (index < level->getNPCs().size())
		{
			auto server = BabyDI::Get<Server>();
			auto& mapNPCs = level->getNPCs();
			auto iter = mapNPCs.begin();
			std::ranges::advance(iter, index, mapNPCs.end());
			if (iter != mapNPCs.end())
			{
				if (auto npc = server->getNPC(*iter); npc != nullptr)
				{
					// Get the DX/DY.
					auto tilePosition = npc->getTilePosition();
					auto dx = tilePosition.x() - fromx;
					auto dy = tilePosition.y() - fromy;
					float length = std::sqrt(dx * dx + dy * dy);
					dx /= length;
					dy /= length;

					// Set the NPC's props.
					npc->setPropWith<NPCProp::HURTDXDY>(SetBy::SERVER, dx, dy);
					npc->setPropWith<NPCProp::POWER>(SetBy::SERVER, static_cast<uint8_t>(std::max(0, npc->character.hitpointsInHalves - halfhearts)));
					if (npc->isCharacter())
						npc->setPropWith<NPCProp::GANI>(SetBy::SERVER, "hurt"sv);

					// Queue up events.
					npc->scripting.events.addEvent(ScriptEventType::WASHIT, visitor->getCurrentSource());
				}
			}
		}
	}
}

// hitobjects power,x,y;
// Hit objects at a location.
void fn_hitobjects(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: hitobjects power,x,y");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->getLevel(); level != nullptr)
			{
				auto power = DoubleAsIntegralFloor<int8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 2);
				auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
				auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
				server->hitObjectsAtPoint({x, y}, power, level, npc);
			}
		}
	}
}

// hitplayer index,halfhearts,fromx,fromy;
// Hits a player in the level.
void fn_hitplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: hitplayer index,halfhearts,fromx,fromy");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto halfhearts = DoubleAsIntegralFloor<int8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto fromx = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		auto fromy = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0));

		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->getLevel(); level != nullptr)
			{
				auto& mapPlayers = level->getPlayers();
				if (index < mapPlayers.size())
					server->hitPlayer(mapPlayers[index], halfhearts, fromx, fromy, npc);
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto halfhearts = DoubleAsIntegralFloor<int8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto npcId = visitor->getOriginalSource().first;
		if (visitor->getOriginalSource().second != ScriptObjectType::NPC)
			npcId = 0;

		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
		{
			auto tilePos = toTilePosition(player->account.character.getLocalPosition());
			server->hitPlayer(player->getId(), halfhearts, tilePos.x() + 1.5, tilePos.y() + 2, server->getNPC(npcId));
		}
	}
}

// insertstring list,index,text;
// Inserts a string into a string array at the given position.
void fn_insertstring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: insertstring list,index,text");

	if (auto listVar = GS1Visitor::getGameVariable(*arguments[0]); listVar != nullptr)
	{
		auto value = listVar->get<std::string>();
		if (!value.has_value())
		{
			listVar->assign<std::string>("");
			value = listVar->get<std::string>();
		}

		auto list = string::fromCSV(value.value().get());
		auto index = DoubleAsIntegralFloor<size_t>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0)));
		auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[2]).value_or("");

		// Cap index to the size of the list.
		if (index > list.size())
			index = list.size();

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
		value.value().get() = string::toCSV(list);
	}
}

// join class;
// Joins a class.
void fn_join(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: join class");

	auto class_ = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{});
	auto server = BabyDI::Get<Server>();
	visitor->scriptContext->joinedClasses.insert({class_, server->getNPCServer()->getClass(class_)});

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->joinClass(class_);
	}
	else if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::WEAPON); source.has_value())
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto itemname = std::clamp(DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)), 0_ui8, 24_ui8);

		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			PixelPosition layPosition = npc->character.getGlobalPosition();

			// Characters lay (+0.5, +3) no matter which direction they are looking.
			if (npc->isCharacter())
				layPosition.translate(static_cast<int16_t>(8), static_cast<int16_t>(16 * 3));

			if (auto level = npc->getLevel(); level != nullptr)
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
		auto itemname = std::clamp(DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)), 0_ui8, 24_ui8);
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		level->addItem(inform_client, toPixelPosition({x, y}), static_cast<LevelItemType>(itemname));
	}
}

// message text;
// Sets the NPC message.
void fn_message(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		std::string text{};
		if (arguments.size() != 0)
			text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{});

		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto dx = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto dy = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto time = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		auto options = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0));

		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->addMoveToQueue(toLocalPixelPosition(dx, dy), time, options);
	}
}

// noplayeronwall;
// Disables onwall checks from detecting players.
void fn_noplayeronwall(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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
		auto power = std::clamp(DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)), 1_ui8, 3_ui8);
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		level->addBomb(inform_client, toPixelPosition({x, y}), power);
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
		uint8_t baddyname = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		level->putNewBaddy(toLocalPixelPosition(x, y), static_cast<BaddyType>(baddyname));
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
		auto radius = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		level->addExplosion(inform_client, toPixelPosition({x, y}), visitor->getCurrentSource(), radius, 1);
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
		auto power = std::clamp(DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)), 1_ui8, 3_ui8);
		auto radius = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0));
		level->addExplosion(inform_client, toPixelPosition({x, y}), visitor->getCurrentSource(), radius, power);
	}
}

// puthorse imagefile,x,y;
// Creates a new horse at the specified location with the given image file.
void fn_puthorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: puthorse imagefile,x,y");

	auto server = BabyDI::Get<Server>();
	if (server->getSettings().get<bool>("puthorseenabled").value_or(true) == false)
	{
		log::printLine(log::npc, "puthorse command is disabled on this server.");
		return;
	}

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto imagefile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		level->addHorse(inform_client, imagefile, toPixelPosition({x, y}), 2, 0);
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
		uint8_t baddyname = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		auto imagefile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[3]).value_or("");
		auto power = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[4]).value_or(0.0));
		level->putNewBaddy(toLocalPixelPosition(x, y), static_cast<BaddyType>(baddyname), power, imagefile);
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
		auto imagefile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto scriptfile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0));

		auto server = BabyDI::Get<Server>();
		auto& fs = server->getFileSystem();
		if (auto file = fs.openi(fs::FileCategory::FILE, scriptfile); file != nullptr)
		{
			auto script = file->readAsString();
			server->addNPC(imagefile, script, x, y, level, NPCStorageType::LEVEL, true);
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
		auto x = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
		auto y = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);
		auto script = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[2]).value_or("");
		string::trimMutate(script);

		auto server = BabyDI::Get<Server>();
		server->getNPCServer()->addNPC({}, script, level, {(float)x, (float)y});
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
		auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
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
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: removeguild guild");

	auto guild = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");

	if (auto guildManager = BabyDI::Get<GuildManager>(); guildManager)
		guildManager->deleteGuild(guild);
}

// removeguildmember guild,account,nick;
void fn_removeguildmember(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() < 2)
		throw std::invalid_argument("invalid arguments: removeguildmember guild,account,nick");

	auto guild = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
	auto account = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");
	std::string nick = (arguments.size() > 2) ? GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[2]).value_or("") : std::string{};

	if (auto guildManager = BabyDI::Get<GuildManager>(); guildManager)
	{
		if (nick.empty())
			guildManager->removePlayerEntirelyFromGuild(guild, account);
		else guildManager->removePlayerFromGuild(guild, account, nick);
	}
}

// removehorse index;
// Removes the horse from the level at the specified index.
void fn_removehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: removehorse index");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
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
		auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		level->removeItem(inform_client, index);
	}
}

// removestring list,text;
// Removes all occurrences of the specified text from the string list.
void fn_removestring(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: removestring list,text");

	if (auto listVar = GS1Visitor::getGameVariable(*arguments[0]); listVar != nullptr)
	{
		auto value = listVar->get<std::string>();
		if (!value.has_value() || value.value().get().empty())
			return;

		auto list = string::fromCSV(value.value().get());
		auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");
		std::erase(list, text);

		value.value().get() = string::toCSV(list);
	}
}

// removeweapon weaponname;
// Removes the specified weapon from the player.
void fn_removeweapon(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: removeweapon weaponname");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto weaponname = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto server = BabyDI::Get<Server>();
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

	if (auto listVar = GS1Visitor::getGameVariable(*arguments[0]); listVar != nullptr)
	{
		auto value = listVar->get<std::string>();
		if (!value.has_value())
		{
			listVar->assign<std::string>("");
			value = listVar->get<std::string>();
		}

		auto list = string::fromCSV(value.value().get());
		auto index = DoubleAsIntegralFloor<size_t>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0)));
		auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[2]).value_or("");

		if (index >= list.size())
			list.push_back(text);
		else
			list.at(index) = text;

		value.value().get() = string::toCSV(list);
	}
}

// saveinfo key,value;
// Saves information to the database.
// Was only used for the Graal2001 server, and only for a limited time.
void fn_saveinfo(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("saveinfo is not implemented.");
}

// savelog text;
// Writes text to npclog.txt.
void fn_savelog(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: savelog text");

	auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
	log::printLine(log::npc, text);
}

// savelog2 filename,text;
// Writes text to a specified log file.
void fn_savelog2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: savelog2 filename,text");

	auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
	auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");

	auto server = BabyDI::Get<Server>();
	server->logToFile(filename, text);
}

// say signindex;
// Displays the text of a sign at the specified index to the player.
void fn_say(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: say signindex");

	if (auto levelTuple = visitor->findCurrentLevelData(); std::get<0>(levelTuple) != nullptr)
	{
		//auto& level = std::get<0>(levelTuple);
		//auto& subLevel = std::get<1>(levelTuple);
		auto& levelData = std::get<2>(levelTuple);
		if (levelData != nullptr)
		{
			auto signIndex = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER);
			if (source.has_value() && signIndex < levelData->signs.size())
			{
				auto& sign = levelData->signs[signIndex];
				auto server = BabyDI::Get<Server>();
				if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
					player->sendSignMessage(sign.text);
			}
		}
	}
}

// say2 message;
// Displays a custom sign message to the player.
void fn_say2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		std::string message;
		if (arguments.size() != 0)
		{
			message = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
			string::eraseCharsMutate(message, "\r\n"sv);
		}

		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto message = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->sendPrivateMessage(NPCServerPlayerID, message);
	}
}

// sendrpgmessage message;
// Sends a message to the F2 message window of the player.
void fn_sendrpgmessage(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		std::string message;
		if (arguments.size() != 0)
			message = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");

		auto server = BabyDI::Get<Server>();
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
		message = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");

	auto server = BabyDI::Get<Server>();
	server->sendToNC(message);
}

// sendtorc message;
// Sends a message to the RC (Remote Control).
void fn_sendtorc(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	std::string message;
	if (arguments.size() != 0)
		message = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");

	auto server = BabyDI::Get<Server>();
	server->sendToRC(message);
}

// serverwarp servername;
// Warps a player to a different server.
void fn_serverwarp(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: serverwarp servername");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto servername = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto server = BabyDI::Get<Server>();
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

	if (auto flag = GS1Visitor::getGameVariable(*arguments[0]); flag != nullptr)
	{
		auto server = BabyDI::Get<Server>();
		if (flag->name.starts_with("client.") || flag->name.starts_with("clientr."))
		{
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
			{
				if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
					player->setFlag(flag->name, std::nullopt, SetBy::SERVER);
			}
		}
		else if (flag->name.starts_with("server.") || flag->name.starts_with("serverr."))
		{
			server->setFlag(flag->name, std::nullopt);
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto gani = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto server = BabyDI::Get<Server>();
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

	if (auto var = GS1Visitor::getGameVariable(*arguments[0]); var != nullptr)
	{
		auto size = DoubleAsIntegralFloor<size_t>(std::clamp(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0), 0.0, static_cast<double>(maximumArraySize)));
		std::vector<double> arrayValues;
		arrayValues.assign(size, 0.0);

		// Copy over existing values, if any.
		if (var->has<std::vector<double>>())
		{
			auto& existing = var->get<std::vector<double>>().value().get();
			for (size_t i = 0; i < std::min(size, existing.size()); ++i)
				arrayValues[i] = existing[i];
		}

		var->assign<std::vector<double>>(std::move(arrayValues));
	}
}

// setbacktile tileindex;
// [newworld] For tiles that have transparency, it controls which tile gets rendered behind it for the whole level.

// setbacktile2 tileindex,x,y,width,height;
// [newworld] Same as setbacktile, but allows specifying a rectangular area for the tile to be set behind.

// setbeltcolor color;
// Sets the player's belt color.
void fn_setbeltcolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setbeltcolor color");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto color = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[ENUM(ColorSlots::BELT)] = static_cast<uint8_t>(color);
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto gani = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto gender = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto server = BabyDI::Get<Server>();
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
// Sets an NPC character's property.
void fn_setcharprop(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() == 0)
		throw std::invalid_argument("invalid arguments: setcharprop messagecode,text");

	if (auto messagecode = GS1Visitor::getGameVariable(*arguments[0]); messagecode != nullptr)
	{
		std::string text;
		if (arguments.size() == 2)
			text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(std::string{});

		if (messagecode->name.starts_with("#C"))
		{
			double color = GS1Visitor::getColorValueFromString(text);
			messagecode->assign<double>(std::move(color));
		}
		else
		{
			messagecode->assign<std::string>(std::move(text));
		}
	}
}

// setcoatcolor color;
// Sets the player's coat color.
void fn_setcoatcolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setcoatcolor color");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto color = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[ENUM(ColorSlots::COAT)] = static_cast<uint8_t>(color);
			player->setPropWith<PlayerProp::COLORS>(SetBy::SERVER, colors);
		}
	}
}

// (1.20 to 1.31r1)	seteffect r,g,b;
// (2.0+)			seteffect r,g,b,a;
void fn_seteffect(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	// The original seteffect command tinted the entire game screen.
	// The newer seteffect command adjusts how an NPC is drawn, tinting it and making it translucent.
	throw std::logic_error("seteffect is clientside only.");
}

// setgender gender;
// Set's the player's gender (controls which voice is used).
void fn_setgender(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setgender gender");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto gender = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto x = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto y = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		auto width = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0));
		auto height = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[4]).value_or(0.0));

		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->warp(filename, player->account.character.getLocalPosition());
	}
}

// setlevel2 filename,x,y;
// Warps the player to a new level specified by the filename and coordinates (x, y).
void fn_setlevel2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: setlevel2 filename,x,y");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto x = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);
		auto y = GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0);

		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer<PlayerClient>(source.value().first); player != nullptr)
			player->warp(filename, {static_cast<int16_t>(x * 16), static_cast<int16_t>(y * 16)});
	}
}

// setmap imgfile,levelsfile,x,y;
// Sets the big map for the player with the specified image file, levels file, and coordinates (x, y).
void fn_setmap(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: setmap imgfile,levelsfile,x,y");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto imgfile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto levelsfile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");
		auto x = GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0);
		auto y = GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0);

		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto imgfile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto levelsfile = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");
		auto x = GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0);
		auto y = GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0);

		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto dir = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto server = BabyDI::Get<Server>();
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
	if (arguments.size() == 0)
		throw std::invalid_argument("invalid arguments: setplayerprop messagecode,text");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		if (auto messagecode = GS1Visitor::getGameVariable(*arguments[0]); messagecode != nullptr)
		{
			std::string text;
			if (arguments.size() == 2)
				text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");

			if (messagecode->name.starts_with("#C"))
			{
				double color = GS1Visitor::getColorValueFromString(text);
				messagecode->assign<double>(std::move(color));
			}
			else
			{
				messagecode->assign<std::string>(std::move(text));
			}
		}
	}
}

// setpm message;
void fn_setpm(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	std::string message;

	if (arguments.size() != 0)
		message = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");

	auto server = BabyDI::Get<Server>();
	if (auto npcServerPlayer = server->getNPCServer()->getPlayerNPCServer(); npcServerPlayer != nullptr)
	{
		auto lines = string::split(message, "#b"sv);
		auto finalMessage = string::toCSV(lines, true);
		npcServerPlayer->privateMessage = finalMessage;
	}
}

// setshape type,width,height;
// type 1 = rectangle
void fn_setshape(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: setshape type,width,height");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto type = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		if (type != 1)
			return;

		auto width = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto height = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->shape = {width, height};
	}
}

// setshield image,power;
// Sets the player's shield image.
void fn_setshield(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: setshield image,power");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto image = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto power = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto server = BabyDI::Get<Server>();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto color = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[ENUM(ColorSlots::SHOES)] = static_cast<uint8_t>(color);
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

	auto params = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
	auto server = BabyDI::Get<Server>();
	server->setShootParams(string::fromCSV(params));
}

// setskincolor color;
// Sets the player's skin color.
void fn_setskincolor(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: setskincolor color");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto color = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[ENUM(ColorSlots::SKIN)] = static_cast<uint8_t>(color);
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto color = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
		{
			auto colors = player->getProp<PlayerProp::COLORS>();
			colors.values[ENUM(ColorSlots::SLEEVES)] = static_cast<uint8_t>(color);
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
	if (auto var = GS1Visitor::getGameVariable(*arguments[0]); var != nullptr)
	{
		std::string text;
		if (arguments.size() == 2)
			text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");

		// Special handling for prefixed variables.
		// Maybe think of a way to do this automatically on the assign rather than doing this.
		auto server = BabyDI::Get<Server>();
		if (var->name.starts_with("client.") || var->name.starts_with("clientr."))
		{
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
			{
				if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
				{
					if (text.empty())
						player->deleteFlag(var->name, SetBy::SERVER);
					else player->setFlag(var->name, text, SetBy::SERVER);
				}
			}
		}
		else if (var->name.starts_with("server.") || var->name.starts_with("serverr."))
		{
			if (text.empty())
				server->deleteFlag(var->name);
			else server->setFlag(std::format("{}={}", var->name, text));
		}
		else
		{
			var->assign<std::string>(std::move(text));
		}
	}
}

// setsword image,power;
// Sets the players sword image and power.
void fn_setsword(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: setsword image,power");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto image = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto power = DoubleAsIntegralFloor<int8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::SWORDPOWER>(SetBy::SERVER, image, power);
	}
}

// setz x,y,width,height,a,b,c,d;
void fn_setz(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("setz is clientside only.");
}

// shoot x,y,z,angle,zangle,power,gani,ganiattribs;
// Creates a shoot style projectile.
void fn_shoot(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() < 7)
		throw std::invalid_argument("invalid arguments: shoot x,y,z,angle,zangle,power,gani,ganiattribs");

	auto level = visitor->findCurrentLevel();
	if (level == nullptr)
		return;

	auto pi = std::numbers::pi;

	auto x = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 16);
	auto y = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);
	auto z = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
	auto angle = static_cast<float>(std::clamp(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0), 0.0, 2 * pi));
	auto zangle = static_cast<float>(std::clamp(GS1Visitor::getScriptValueAsCopy<double>(*arguments[4]).value_or(0.0), -(pi / 2), (pi / 2)));
	auto power = static_cast<uint8_t>(std::clamp(GS1Visitor::getScriptValueAsCopy<double>(*arguments[5]).value_or(0.0), 0.0, 5.0) * 44);
	auto gani = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[6]).value_or("");

	auto server = BabyDI::Get<Server>();
	auto gravity = static_cast<float>(server->Scripting.variables.getValue<double>("gravity").value_or(2.0));
	level->addShoot(inform_client, {x, y, z}, angle, zangle, power, gravity, gani, visitor->getOriginalSource());
}

// shootarrow dir;
// Shoots an arrow in the specified direction.
void fn_shootarrow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: shootarrow dir");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto dir = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = {(dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16)};

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
		auto dir = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = {(dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16)};

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
		auto dir = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = {(dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16)};

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
		auto dir = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = {(dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16)};

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
		auto dir = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

		const auto& source = visitor->getOriginalSource();
		PixelPosition speed = {(dir == 0 || dir == 2) ? 0 : (dir == 1 ? -16 : 16), (dir == 1 || dir == 3) ? 0 : (dir == 0 ? -16 : 16)};

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
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, static_cast<uint8_t>(npc->visFlags | (uint8_t)NPCVisFlags::VISIBLE));
	}
}

// showani index,x,y,direction,gani,params;
// Shows a gani at the specified position.
void fn_showani(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::invalid_argument("invalid arguments: showani index,x,y,direction,gani,params");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto x = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);
			auto y = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
			auto direction = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0));
			auto gani = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[4]).value_or("");

			server->getNPCServer()->showGani(npc, index, {x, y}, gani, direction);
		}
	}
}

// showani2 index,x,y,z,direction,gani,params;
// Shows a gani at the specified position.
void fn_showani2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 6)
		throw std::invalid_argument("invalid arguments: showani2 index,x,y,z,direction,gani,params");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto x = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);
			auto y = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
			auto z = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0) * 16);
			auto direction = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[4]).value_or(0.0));
			auto gani = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[5]).value_or("");

			server->getNPCServer()->showGani(npc, index, {x, y, z}, gani, direction);
		}
	}
}

// showcharacter;
// Turns the NPC into a character.
void fn_showcharacter(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			npc->setPropWith<NPCProp::IMAGE>(SetBy::SERVER, "#c#"s);
			npc->shape = {0, 0};
		}
	}
}

// showimg index,filename,x,y;
// Displays an image on the level at the specified coordinates.
void fn_showimg(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: showimg index,filename,x,y");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");
			auto x = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
			auto y = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0) * 16);

			server->getNPCServer()->showImage(npc, index, {x, y}, filename);
		}
	}
}

// showimg2 index,filename,x,y,z;
// Displays an image on the level at the specified coordinates.
void fn_showimg2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 5)
		throw std::invalid_argument("invalid arguments: showimg2 index,filename,x,y,z");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");
			auto x = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
			auto y = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0) * 16);
			auto z = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[4]).value_or(0.0) * 16);

			server->getNPCServer()->showImage(npc, index, {x, y, z}, filename);
		}
	}
}

// showpoly index,{ x1,y1,...,xn,yn };
// Displays a polygon at the specified coordinates.
void fn_showpoly(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: showpoly index,{ x1,y1,...,xn,yn }");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto polygons = GS1Visitor::getScriptValueAsCopy<std::vector<double>>(*arguments[1]).value_or(std::vector<double>{});

			if (polygons.size() == 0 || polygons.size() % 2 != 0)
				throw std::invalid_argument("invalid arguments: showpoly index,{ x1,y1,...,xn,yn }");

			server->getNPCServer()->showPoly(npc, index, polygons);
		}
	}
}

// showpoly2 index,{ x1,y1,z1,...,xn,yn,zn };
// Displays a polygon at the specified coordinates.
void fn_showpoly2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: showpoly2 index,{ x1,y1,z1,...,xn,yn,zn }");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto polygons = GS1Visitor::getScriptValueAsCopy<std::vector<double>>(*arguments[1]).value_or(std::vector<double>{});

			if (polygons.size() == 0 || polygons.size() % 3 != 0)
				throw std::invalid_argument("invalid arguments: showpoly2 index,{ x1,y1,z1,...,xn,yn,zn }");

			server->getNPCServer()->showPoly(npc, index, polygons);
		}
	}
}

// showstats bitflag;
void fn_showstats(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("showstats is not implemented yet.");
}

// showtext index,x,y,font,style,text;
// Displays text at the specified coordinates.
void fn_showtext(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 6)
		throw std::invalid_argument("invalid arguments: showtext index,x,y,font,style,text");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto x = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);
			auto y = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
			auto font = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[3]).value_or("");
			auto style = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[4]).value_or("");
			auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[5]).value_or("");

			server->getNPCServer()->showText(npc, index, {x, y}, text, font, style);
		}
	}
}

// showtext2 index,x,y,z,font,style,text;
// Displays text at the specified coordinates.
void fn_showtext2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 7)
		throw std::invalid_argument("invalid arguments: showtext2 index,x,y,z,font,style,text");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			auto index = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			auto x = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);
			auto y = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
			auto z = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0) * 16);
			auto font = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[4]).value_or("");
			auto style = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[5]).value_or("");
			auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[6]).value_or("");

			server->getNPCServer()->showText(npc, index, {x, y, z}, text, font, style);
		}
	}
}

// sleep duration;
// Pauses script execution for the specified duration in seconds.
void fn_sleep(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: sleep duration");

	if (auto source = visitor->getOriginalSource(); source.second == ScriptObjectType::NPC)
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.first); npc != nullptr)
		{
			auto duration = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
			npc->timeout = std::chrono::duration_cast<std::chrono::milliseconds>(duration_seconds_double(duration));
			throw sleep_exception{};
		}
	}
}

// spyfire length,power;
// Sends a spyfire explosion from the player.
void fn_spyfire(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: spyfire length,power");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto length = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)) & 0b11111;
		auto power = DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0)) & 0b111;
		uint8_t length_power = (length << 3) | power;

		auto server = BabyDI::Get<Server>();
		if (auto player = server->getPlayer<PlayerClient>(source.value().first); player != nullptr)
		{
			if (auto level = player->getLevel(); level != nullptr)
			{
				server->sendPacketToNearby(CString() >> (char)PLO_FIRESPY >> (short)source.value().first >> (char)(length_power), player->account.character.getGlobalPosition(), level);
				level->addSpyFire(player->account.character.getGlobalPosition(), source.value(), player->account.character.direction, length, power);
			}
		}
	}
}

// take itemname;
// Takes an item on the level in a 10-tile radius from the NPC.
void fn_take(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: take itemname");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->getLevel(); level != nullptr)
			{
				auto itemname = std::clamp(DoubleAsIntegralFloor<uint8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)), 0_ui8, 24_ui8);

				// Get our search position.
				PixelPosition searchPosition = npc->character.getGlobalPosition();
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

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->getLevel(); level != nullptr)
			{
				auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
				if (auto item = level->getItem(index); item.has_value())
				{
					if (LevelItem::isRupeeType(item.value()->item))
						npc->setPropWith<NPCProp::RUPEES>(SetBy::SERVER, npc->getProp<NPCProp::RUPEES>().value + LevelItem::GetRupeeCount(item.value()->item));
					else if (item.value()->item == LevelItemType::HEART)
						npc->setPropWith<NPCProp::POWER>(SetBy::SERVER, static_cast<GBYTE1>(npc->getProp<NPCProp::POWER>().value + 2));
					else if (item.value()->item == LevelItemType::DARTS)
						npc->setPropWith<NPCProp::ARROWS>(SetBy::SERVER, static_cast<GBYTE1>(npc->getProp<NPCProp::ARROWS>().value + 5));
					else if (item.value()->item == LevelItemType::BOMBS)
						npc->setPropWith<NPCProp::BOMBS>(SetBy::SERVER, static_cast<GBYTE1>(npc->getProp<NPCProp::BOMBS>().value + 5));
					else if (item.value()->item == LevelItemType::GLOVE1)
						npc->setPropWith<NPCProp::GLOVEPOWER>(SetBy::SERVER, std::max(npc->getProp<NPCProp::GLOVEPOWER>().value, 1_ui8));
					else if (item.value()->item == LevelItemType::GLOVE2)
						npc->setPropWith<NPCProp::GLOVEPOWER>(SetBy::SERVER, std::max(npc->getProp<NPCProp::GLOVEPOWER>().value, 2_ui8));

					level->removeItem(inform_client, index);
				}
			}
		}
	}
}

// takehorse index;
// Mounts the horse at the specified index on the level.
void fn_takehorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: takehorse index");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = npc->getLevel(); level != nullptr)
			{
				auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
				if (auto horse = level->getHorse(index); horse.has_value())
				{
					npc->setPropWith<NPCProp::HORSEIMAGE>(SetBy::SERVER, horse.value()->image);
					level->removeHorse(inform_client, index);
				}
			}
		}
	}
}

// takeplayercarry;
// Takes the carried object from the player.
void fn_takeplayercarry(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
		{
			if (auto level = player->getLevel(); level != nullptr)
				level->addThrownItem(player->getTilePosition().translate(0.5f, 1.0f), player->account.character.direction, ENUM<CarryObjectSprite>(player->getCarrySprite()), source::FromPlayer(player->getId()));

			player->sendPacket(CString() >> (char)PLO_THROWCARRIED >> (short)player->getId());
			player->setPropWith<PlayerProp::CARRYSPRITE>(SetBy::SERVER, 0xFF_ui8);
		}
	}
}

// takeplayerhorse;
// Takes the horse from the player.
void fn_takeplayerhorse(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
			player->setPropWith<PlayerProp::HORSEGIF>(SetBy::SERVER, std::string{});
	}
}

// throwcarry;
// Throws the carried object.
void fn_throwcarry(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr && npc->isCharacter() && npc->character.gani.starts_with("carry"))
			npc->setPropWith<NPCProp::GANI>(SetBy::SERVER, "idle"sv);
	}
}

// timershow;
// Shows the NPC's clientside timeout counter.
void fn_timershow(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, static_cast<uint8_t>(npc->visFlags | PROPID(NPCVisFlags::TIMERSHOW)));
	}
}

// tokenize text;
// Tokenizes a string into tokens using spaces as a delimiter.
void fn_tokenize(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: tokenize text");

	auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
	visitor->tokenizeTokens = string::splitToVector(text, " "sv);
	visitor->builtInStore->add(GameVariable{.name = "tokenscount", .value = static_cast<double>(visitor->tokenizeTokens.size()), .lifetime = variables::Lifetime::TEMPORARY});
}

// tokenize2 delims,text;
// Tokenizes a string into tokens using the specified delimiters.
void fn_tokenize2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: tokenize2 delims,text");

	auto delims = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
	auto text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or("");
	visitor->tokenizeTokens = string::splitToVector(text, delims);
	visitor->builtInStore->add(GameVariable{.name = "tokenscount", .value = static_cast<double>(visitor->tokenizeTokens.size()), .lifetime = variables::Lifetime::TEMPORARY});
}

// toweapons name;
// Adds the NPC as a weapon for the player.
void fn_toweapons(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: toweapons name");

	// Get our source NPC.
	auto server = BabyDI::Get<Server>();
	const auto& source = visitor->getOriginalSource();
	if (source.second != ScriptObjectType::NPC || server == nullptr)
		return;

	// Get the active player.
	PlayerPtr player = nullptr;
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
		player = server->getNPCServer()->getPlayer(source.value().first);
	if (player == nullptr)
		return;

	if (auto npc = server->getNPC(source.first); npc != nullptr)
	{
		// Get or create the weapon, and make sure the script is current.
		auto name = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto weapon = server->getWeapon(name);
		if (weapon == nullptr)
		{
			weapon = std::make_shared<Weapon>(name, npc->image, std::string{npc->getScript().getOriginalSource()});
			weapon->saveWeapon();
			server->NC_AddWeapon(weapon);
		}
		// Script differs, update the weapon.
		else if (weapon->getScript().getOriginalSource() != npc->getScript().getOriginalSource())
		{
			weapon->updateWeapon(npc->image, std::string{npc->getScript().getOriginalSource()}).saveWeapon();
			server->updateWeaponForPlayers(weapon);
		}

		// Give the weapon to the player.
		player->addWeapon(weapon);
	}
}

// triggeraction x,y,action,params;
// Sends out a trigger action.
void fn_triggeraction(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() < 4)
		throw std::invalid_argument("invalid arguments: triggeraction x,y,action,params");

	auto x = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	auto y = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);
	auto action = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[2]).value_or("");
	auto params = string::toCSV(arguments | std::views::drop(3) | std::views::transform([&visitor](GS1ScriptValue* value)
	{
		return GS1Visitor::getScriptValueAsCopy<std::string>(*value).value_or("");
	}));

	auto server = BabyDI::Get<Server>();
	if (action == "clientside")
	{
		if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
			server->sendTriggerAction(source.value().first, 0, {0, 0}, action, params);
	}
	else
	{
		const auto& currentSource = visitor->getCurrentSource();
		LevelPtr targetLevel = visitor->findCurrentLevel();
		uint32_t npcId = 0;
		if (currentSource.second == ScriptObjectType::NPC)
			npcId = currentSource.first;
		if (targetLevel != nullptr)
			server->sendTriggerAction(targetLevel, npcId, {static_cast<int16_t>(x * 16), static_cast<int16_t>(y * 16)}, action, params);
	}
}

// unfreezeplayer;
// Unfreezes a player.
void fn_unfreezeplayer(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
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

	if (auto flag = GS1Visitor::getGameVariable(*arguments[0]); flag != nullptr)
	{
		auto server = BabyDI::Get<Server>();
		if (flag->name.starts_with("client.") || flag->name.starts_with("clientr."))
		{
			if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
			{
				if (auto player = server->getNPCServer()->getPlayer(source.value().first); player != nullptr)
					player->deleteFlag(flag->name, SetBy::SERVER);
			}
		}
		else if (flag->name.starts_with("server.") || flag->name.starts_with("serverr."))
		{
			server->deleteFlag(flag->name);
		}
		else
		{
			flag->assign<bool>(false);
		}
	}
}

// updateboard x,y,width,height;
// Updates a portion of the map board, making changes visible to other players.
void fn_updateboard(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: updateboard x,y,width,height");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = static_cast<float>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)));
		auto y = static_cast<float>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0)));
		auto width = static_cast<float>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0)));
		auto height = static_cast<float>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0)));
		level->updateBoard({{x, y}, {width, height}});
	}
}

// updateboard2 x,y,width,height;
// Updates a portion of the map board, saves the changes to the map file, and makes the changes visible to other players.
void fn_updateboard2(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: updateboard2 x,y,width,height");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = static_cast<float>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)));
		auto y = static_cast<float>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0)));
		auto width = static_cast<float>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0)));
		auto height = static_cast<float>(std::max(0.0, GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0)));
		level->updateBoard2({{x, y}, {width, height}});
	}
}

// updateterrain;
void fn_updateterrain(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("updateterrain is clientside only.");
}

// warpto levelname,x,y;
// Warps an NPC to a new level.
void fn_warpto(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: warpto levelname,x,y");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto filename = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or("");
		auto x = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);
		auto y = GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0);

		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
		{
			if (auto level = server->getLoadedLevel(filename, npc->getLevel()); level != nullptr)
				npc->warp(level, {static_cast<int16_t>(x * 16), static_cast<int16_t>(y * 16)});
		}
	}
}

//----------------------------

// enabledamagereactions;
// Enables damage reactions for the NPC, allowing it to react to damage with animations and invincibility frames.
void fn_enabledamagereactions(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->allowServerDamageReactions = true;
	}
}

// disabledamagereactions;
// Disables damage reactions for the NPC, going back to the default behavior of not automatically reacting to damage.
void fn_disabledamagereactions(GS1Visitor* visitor, std::string_view commandName, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto npc = server->getNPC(source.value().first); npc != nullptr)
			npc->allowServerDamageReactions = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
