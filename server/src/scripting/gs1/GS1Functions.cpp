#include <algorithm>
#include <any>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <numbers>
#include <optional>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <tomcrypt.h>
//#include <tomcrypt_misc.h>
#include <tree/ParseTree.h>

#include <BabyDI.h>
#include <GS1Parser.h>
#include <IEnums.h>

#include <Server.h>
#include <level/LevelBaddy.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/GS1Functions.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/FilePermissions.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

using BuiltInFunctionHandleFunc = GS1ScriptValue (*)(GS1Visitor*, const std::vector<GS1ScriptValue*>&);
using BuiltInFunctionHandleMap = std::unordered_map<size_t, BuiltInFunctionHandleFunc>;

static GS1ScriptValue fn_abs(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_aindexof(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_arctan(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_arraylen(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_ascii(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_base64decode(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_base64encode(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_cos(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_exp(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_findnearestplayer(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_findnearestplayers(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getangle(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getareanpcs(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getdir(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getflagkeys(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getnearestplayer(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getnearestplayers(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getnpc(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getplayer(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getplayersingroup(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getz(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_hasright(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_hasweapon(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_imgheight(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_imgwidth(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_indexof(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_int(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_keycode(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_keydown(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_keydown2(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_lindexof(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_log(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_max(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_min(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onmapx(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onmapy(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onwall(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onwall2(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onwater(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onwater2(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_passwordmatches(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_playersays(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_playersays2(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_random(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_sarraylen(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_screenx(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_screeny(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_sin(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_startswith(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_strcontains(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_strequals(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_strlen(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_strtofloat(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testbomb(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testcompu(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testexplo(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testhorse(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testitem(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testnpc(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testplayer(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testsign(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_textheight(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_textwidth(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_tiletype(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_vecx(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_vecy(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_worldx(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_worldy(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments);

static BuiltInFunctionHandleMap GenerateMap()
{
	constexpr string::string_hash hash{};
	BuiltInFunctionHandleMap map =
	{
		{hash("abs"), &fn_abs},
		{hash("aindexof"), &fn_aindexof},
		{hash("arctan"), &fn_arctan},
		{hash("arraylen"), &fn_arraylen},
		{hash("ascii"), &fn_ascii},
		{hash("base64decode"), &fn_base64decode},
		{hash("base64encode"), &fn_base64encode},
		{hash("cos"), &fn_cos},
		{hash("exp"), &fn_exp},
		{hash("findnearestplayer"), &fn_findnearestplayer},
		{hash("findnearestplayers"), &fn_findnearestplayers},
		{hash("getangle"), &fn_getangle},
		{hash("getareanpcs"), &fn_getareanpcs},
		{hash("getdir"), &fn_getdir},
		{hash("getflagkeys"), &fn_getflagkeys},
		{hash("getnearestplayer"), &fn_getnearestplayer},
		{hash("getnearestplayers"), &fn_getnearestplayers},
		{hash("getnpc"), &fn_getnpc},
		{hash("getplayer"), &fn_getplayer},
		{hash("getplayersingroup"), &fn_getplayersingroup},
		{hash("getz"), &fn_getz},
		{hash("hasright"), &fn_hasright},
		{hash("hasweapon"), &fn_hasweapon},
		{hash("imgheight"), &fn_imgheight},
		{hash("imgwidth"), &fn_imgwidth},
		{hash("indexof"), &fn_indexof},
		{hash("int"), &fn_int},
		{hash("keycode"), &fn_keycode},
		{hash("keydown"), &fn_keydown},
		{hash("keydown2"), &fn_keydown2},
		{hash("lindexof"), &fn_lindexof},
		{hash("log"), &fn_log},
		{hash("max"), &fn_max},
		{hash("min"), &fn_min},
		{hash("onmapx"), &fn_onmapx},
		{hash("onmapy"), &fn_onmapy},
		{hash("onwall"), &fn_onwall},
		{hash("onwall2"), &fn_onwall2},
		{hash("onwater"), &fn_onwater},
		{hash("onwater2"), &fn_onwater2},
		{hash("passwordmatches"), &fn_passwordmatches},
		{hash("playersays"), &fn_playersays},
		{hash("playersays2"), &fn_playersays2},
		{hash("random"), &fn_random},
		{hash("sarraylen"), &fn_sarraylen},
		{hash("screenx"), &fn_screenx},
		{hash("screeny"), &fn_screeny},
		{hash("sin"), &fn_sin},
		{hash("startswith"), &fn_startswith},
		{hash("strcontains"), &fn_strcontains},
		{hash("strequals"), &fn_strequals},
		{hash("strlen"), &fn_strlen},
		{hash("strtofloat"), &fn_strtofloat},
		{hash("testbomb"), &fn_testbomb},
		{hash("testcompu"), &fn_testcompu},
		{hash("testexplo"), &fn_testexplo},
		{hash("testhorse"), &fn_testhorse},
		{hash("testitem"), &fn_testitem},
		{hash("testnpc"), &fn_testnpc},
		{hash("testplayer"), &fn_testplayer},
		{hash("testsign"), &fn_testsign},
		{hash("textheight"), &fn_textheight},
		{hash("textwidth"), &fn_textwidth},
		{hash("tiletype"), &fn_tiletype},
		{hash("vecx"), &fn_vecx},
		{hash("vecy"), &fn_vecy},
		{hash("worldx"), &fn_worldx},
		{hash("worldy"), &fn_worldy},
	};
	return map;
}

constexpr std::array<std::string_view, 2> flagProcessingFunctions =
{
	"lindexof"sv,
	"sarraylen"sv,
};

///////////////////////////////////////////////////////////////////////////////

GS1ScriptValue processBuiltInFunction(GS1Visitor* visitor, const antlr4::tree::ParseTree* node, const std::string_view functionName)
{
	static BuiltInFunctionHandleMap map = GenerateMap();

	if (visitor == nullptr)
		throw std::runtime_error("processBuiltInFunction received an empty visitor");
	if (functionName.empty())
		throw std::runtime_error("processBuiltInFunction received an empty function name");

	// Find the command in the map.
	const size_t hash = string::string_hash{}(functionName);
	const auto it = map.find(hash);
	if (it == map.end())
	{
		log::printLine(log::script, "Unknown function in NPC '{}': {}", visitor->who, functionName);
		return {};
	}

	// Record if we are expecting a flag.
	const bool oldExpectingFlag = visitor->expectingFlag;
	visitor->expectingFlag = (std::ranges::find(flagProcessingFunctions, functionName) != std::ranges::end(flagProcessingFunctions));

	// Collect the arguments from the node.
	std::vector<GS1ScriptValue*> arguments;
	std::vector<std::any> results;
	for (const auto child : node->children)
	{
		if (auto ret = child->accept(visitor); ret.has_value())
		{
			results.emplace_back(std::move(ret));
			auto* container = std::any_cast<GS1ScriptValue>(&results.back());
			if (container == nullptr)
				throw std::runtime_error("BuiltInFunction argument is not a valid GS1ScriptValue");

			arguments.push_back(container);

			// Reset the expectingFlag toggle back to normal.
			visitor->expectingFlag = oldExpectingFlag;
		}
	}

	// Reset the expectingFlag toggle back to normal.
	visitor->expectingFlag = oldExpectingFlag;

	// Execute the command.
	return it->second(visitor, arguments);
}

///////////////////////////////////////////////////////////////////////////////

// abs(value)
// Absolute value of a number.
GS1ScriptValue fn_abs(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: abs(value)");

	const auto value = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	return GameValue{std::abs(value)};
}

// aindexof(value, array)
// Returns the index of the first occurrence of value in the array, or -1 if not found.
GS1ScriptValue fn_aindexof(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: aindexof(value, array)");

	const auto value = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	auto array = GS1Visitor::getScriptValueAsCopy<std::vector<double>>(*arguments[1]).value_or(std::vector<double>{});

	const auto result = std::ranges::find(array, value);
	if (result == std::ranges::end(array))
		return GameValue{-1.0};

	const auto distance = std::ranges::distance(std::ranges::begin(array), result);
	return GameValue{static_cast<double>(distance)};
}

// arctan(value)
// Returns the arctangent of the value in radians.
GS1ScriptValue fn_arctan(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: arctan(value)");

	const auto value = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	return GameValue{std::atan(value)};
}

// arraylen(array)
// Returns the length of the array.
GS1ScriptValue fn_arraylen(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: arraylen(array)");

	const auto array = GS1Visitor::getScriptValueAsCopy<std::vector<double>>(*arguments[0]).value_or(std::vector<double>{});
	return GameValue{static_cast<double>(array.size())};
}

// ascii(string)
// Returns the ASCII value of the first character in the string.
GS1ScriptValue fn_ascii(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: ascii(string)");

	const auto str = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	if (str.empty())
		return GameValue{0.0};

	return GameValue{static_cast<double>(static_cast<uint8_t>(str[0]))};
}

// base64decode(string)
// Decodes a Base64 encoded string.
GS1ScriptValue fn_base64decode(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: base64decode(string)");

	const auto input = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	const auto output = std::make_unique<unsigned char[]>(input.length());
	unsigned long outputLength = input.length();
	base64_decode(input.c_str(), input.length(), output.get(), &outputLength);

	return GameValue{std::string{reinterpret_cast<const char*>(output.get()), outputLength}};
}

// base64encode(string)
// Encodes a string to Base64 format.
GS1ScriptValue fn_base64encode(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: base64encode(string)");

	const auto input = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);

	// Calculate the length of the resulting base64 string.
	unsigned long outputLength = 4 * ((input.length() + 2) / 3) + 1;

	// Encode.
	const auto output = std::make_unique<char[]>(outputLength);
	base64_encode(reinterpret_cast<const unsigned char*>(input.c_str()), static_cast<unsigned long>(input.length()), output.get(), &outputLength);

	return GameValue{std::string{output.get(), outputLength}};
}

// cos(value)
// Returns the cosine of the value in radians.
GS1ScriptValue fn_cos(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: cos(value)");

	const auto value = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	return GameValue{std::cos(value)};
}

// exp(value)
// Computes e raised to the power of the value.
GS1ScriptValue fn_exp(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: exp(value)");

	const auto value = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	return GameValue{std::exp(value)};
}

// findnearestplayer(x, y)
// Finds the nearest player to the specified position and returns a player source.
GS1ScriptValue fn_findnearestplayer(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: findnearestplayer(x, y)");

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		const auto position = toPixelPosition({x, y});

		// Find the nearest player.
		std::tuple<PlayerID, double> nearestPlayer{0, std::numeric_limits<double>::max()};
		const auto* server = BabyDI::Get<Server>();
		for (const auto& id : level->findInRangePlayers(position))
		{
			if (auto player = server->getNPCServer()->getPlayer(id); player != nullptr)
			{
				TilePosition playerPos = toTilePosition(player->account.character.getGlobalPosition());
				auto distance = std::hypot(playerPos.x() - x, playerPos.y() - y);
				if (distance < std::get<1>(nearestPlayer))
					nearestPlayer = {id, distance};
			}
		}

		// Return the closest player.
		if (std::get<0>(nearestPlayer) != 0)
			return ScriptObject{std::get<0>(nearestPlayer), ScriptObjectType::PLAYER};
	}

	return GameValue{0.0};
}

// findnearestplayers(x, y)
// Finds all players in the level, orders them by distance from the specified position, and returns a list of player sources.
// Probably not supported in GS1.
GS1ScriptValue fn_findnearestplayers(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("not implemented: findnearestplayers");
}

// getangle(dx, dy)
// Returns the angle in radians from (0,0) to the position specified by dx and dy.
GS1ScriptValue fn_getangle(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	/*
	( 0,-1) up:    1.570796 (pi/2)
	(-1, 0) left:  3.141593 (pi)
	( 0, 1) down:  4.712389 (3pi/2)
	( 1, 0) right: 0.000000 (0)
	*/
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: getangle(dx, dy)");

	const auto dx = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	auto dy = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);

	// No angle if no direction is specified.
	if (DoubleIsZero(dx) && DoubleIsZero(dy))
		return GameValue{0.0};

	// Flip the Y coordinate to match the game's coordinate system.
	dy = -dy;

	// Get the angle.
	auto angle = std::atan2(dy, dx);

	// If the angle is negative, we need to adjust it to be in the range [0, 2π).
	if (angle < 0.0)
		angle += std::numbers::pi * 2;

	return GameValue{angle};
}

// getareanpcs(x, y, width, height)
// Returns the indices of all NPCS in the area specified.
GS1ScriptValue fn_getareanpcs(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: getareanpcs(x, y, width, height)");

	std::vector<double> result;
	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 16);
		auto y = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);
		auto width = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
		auto height = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0) * 16);

		auto npcs = level->findIntersectingNPCs({{x, y}, {width, height}}, true);
		for (const auto id : npcs)
			result.emplace_back(static_cast<double>(id));
	}
	return GameValue{result};
}

// getdir(dx, dy)
// Returns the direction to look in the relative position specified by dx and dy.
GS1ScriptValue fn_getdir(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (const auto* character = getCharacterFromSource(visitor->getOriginalSource()); character != nullptr)
	{
		if (arguments.size() != 2)
			throw std::invalid_argument("invalid arguments: getdir(dx, dy)");

		const auto dx = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
		const auto dy = -1 * GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);

		// Get the angle we are looking.
		double angle = 0.0;
		if (!DoubleIsZero(dx) || DoubleIsZero(dy))
		{
			// Get the angle.
			angle = std::atan2(dy, dx);

			// If the angle is negative, we need to adjust it to be in the range [0, 2π).
			if (angle < 0.0)
				angle += std::numbers::pi * 2;
		}

		constexpr auto angleNE = std::numbers::pi * (1.0 / 4.0);
		constexpr auto angleNW = std::numbers::pi * (3.0 / 4.0);
		constexpr auto angleSW = std::numbers::pi + angleNE;
		constexpr auto angleSE = std::numbers::pi + angleNW;

		// Convert the angle to a direction.
		// Diagonals are biased towards up (0) and down (2).
		if (angle < angleNE)
			return GameValue{3.0};
		if (angle <= angleNW)
			return GameValue{0.0};
		if (angle < angleSW)
			return GameValue{1.0};
		if (angle <= angleSE)
			return GameValue{2.0};
		return GameValue{3.0};
	}

	// Default to looking down.
	return GameValue{2.0};
}

// getflagkeys(prefix)
// Searches for all flags in the format of prefix### and returns an array of all the ###.
// E.g., bankaccount_0, bankaccount_1, etc. with the prefix "bankaccount_" would return an array of {0, 1, ...}.
GS1ScriptValue fn_getflagkeys(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: getflagkeys(prefix)");

	auto prefix = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);

	std::vector<double> results;
	const auto storageType = GS1Visitor::getStorageTypeFromIdentifier(prefix).value_or(ENUM(StorageType::CLIENT));
	GS1Visitor::fixStorageNameOnIdentifier(prefix);

	const auto variableStore = visitor->getGameVariableStoreForStorageType(storageType);
	if (variableStore == nullptr)
		return GameValue{results};

	for (const auto& key : variableStore->store | std::views::keys)
	{
		if (key.starts_with(prefix))
		{
			const auto index = string::toNumber(std::string_view{key.c_str() + prefix.length(), key.length() - prefix.length()});
			results.push_back(index);
		}
	}

	return GameValue{results};
}

// getnearestplayer(x, y)
// Finds the nearest player to the specified position and returns the player index.
GS1ScriptValue fn_getnearestplayer(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: getnearestplayer(x, y)");

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		const auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		const auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		const auto position = toPixelPosition({x, y});

		// Find the nearest player.
		std::tuple<PlayerID, double> nearestPlayer{0, std::numeric_limits<double>::max()};
		const auto* server = BabyDI::Get<Server>();
		for (const auto& id : level->findInRangePlayers(position))
		{
			if (auto player = server->getNPCServer()->getPlayer(id); player != nullptr)
			{
				TilePosition playerPos = toTilePosition(player->account.character.getGlobalPosition());
				auto distance = std::hypot(playerPos.x() - x, playerPos.y() - y);
				if (distance < std::get<1>(nearestPlayer))
					nearestPlayer = {id, distance};
			}
		}

		// Return the closest player.
		if (std::get<0>(nearestPlayer) != 0)
			return GameValue{static_cast<double>(std::get<0>(nearestPlayer))};
	}

	return GameValue{0.0};
}

// getnearestplayers(x, y, condition)
// Returns an array of all the level players sorted by how close they are to the specified position, matching the condition expression.
GS1ScriptValue fn_getnearestplayers(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() < 2)
		throw std::invalid_argument("invalid arguments: getnearestplayers(x, y, flag)");

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		const auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		const auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		const auto position = toPixelPosition({x, y});

		std::map<double, size_t> playersByDistance;
		const auto* server = BabyDI::Get<Server>();
		auto& players = level->getPlayers();
		for (const auto& id : level->findInRangePlayers(position))
		{
			// Execute the condition.
			bool skip = false;
			if (arguments.size() > 2)
			{
				visitor->pushSource(source::FromPlayer(id));
				skip = DoubleIsZero(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0));
				visitor->popSource();
			}
			if (skip) continue;

			if (auto iter = std::ranges::find(players, id); iter != std::ranges::end(players))
			{
				if (auto player = server->getNPCServer()->getPlayer(id); player != nullptr)
				{
					auto pidx = std::ranges::distance(std::ranges::begin(players), iter);
					TilePosition playerPos = toTilePosition(player->account.character.getGlobalPosition());
					auto distance = std::hypot(playerPos.x() - x, playerPos.y() - y);
					playersByDistance.emplace(distance, pidx);
				}
			}
		}

		// Construct the player ID array.
		std::vector<double> playerIds;
		for (const auto& id : playersByDistance | std::views::values)
			playerIds.push_back(static_cast<double>(id));

		// Return it.
		return GameValue{playerIds};
	}

	return GameValue{std::vector<double>()};
}

// getnpc(name)
// Returns an NPC object that links to the NPC, or a false value if not found.
GS1ScriptValue fn_getnpc(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: getnpc(name)");

	const auto npcName = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);

	auto* server = BabyDI::Get<Server>();
	auto& npcList = server->getNPCList();
	for (auto& [id, npc] : npcList)
	{
		if (npc->name == npcName)
			return ScriptObject{id, ScriptObjectType::NPC};
	}

	return GameValue{0.0};
}

// getplayer(account)
// Returns a Player object that links to the player with the specified account name, or a false value if not found.
GS1ScriptValue fn_getplayer(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: getplayer(account)");

	const auto playerName = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);

	const auto* server = BabyDI::Get<Server>();
	if (const auto player = server->getNPCServer()->getPlayer(playerName, PLTYPE_ANYCLIENT); player != nullptr)
	{
		return ScriptObject{player->getId(), ScriptObjectType::PLAYER};
	}

	return GameValue{0.0};
}

// [GR] getplayersingroup(group)
// Returns an array of all players in the specified group, for use with allplayers[].
GS1ScriptValue fn_getplayersingroup(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: getplayersingroup(group)");

	std::vector<double> results{};

	const auto allPlayersVar = visitor->server->Scripting.variables.get("allplayers").lock();
	if (allPlayersVar == nullptr)
		return GameValue{results};

	const auto allPlayersArr = allPlayersVar->get<std::vector<ScriptObject>>();
	if (!allPlayersArr.has_value())
		return GameValue{results};

	const auto group = string::toLower(GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s));
	const auto& allPlayers = allPlayersArr.value().get();
	for (size_t i = 0; i < allPlayers.size(); ++i)
	{
		if (const auto player = visitor->server->getNPCServer()->getPlayer<PlayerClient>(allPlayers[i].first); player != nullptr)
		{
			if (player->getGroup() == group)
				results.push_back(static_cast<double>(i));
		}
	}

	return GameValue{results};
}

// getz(x, y)
// Returns the Z coordinate at the specified X and Y position in the world.
GS1ScriptValue fn_getz(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: getz(x, y)");

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
		return GameValue{level->getHeightAt(toPixelPosition({x, y}))};
	}

	return GameValue{0.0};
}

// hasright(rw,path)
// Checks if the player has the specified file browser rights for the path.  (rw = read/write, r = read only, w = write only)
GS1ScriptValue fn_hasright(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: hasright(rw, path)");

	const auto rights = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	if (rights.empty())
		return GameValue{false};

	const auto path = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(""s);
	if (auto player = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); player.has_value())
	{
		const auto* server = BabyDI::Get<Server>();
		if (const auto playerObject = server->getNPCServer()->getPlayer(player.value().first); playerObject != nullptr)
		{
			bool result = false;
			if (rights.contains('r'))
				result |= playerObject->account.folderRights.hasPermission(path, FilePermissions::Type::Read);
			if (!result && rights.contains('w'))
				result |= playerObject->account.folderRights.hasPermission(path, FilePermissions::Type::Write);

			return GameValue{result};
		}
	}

	return GameValue{false};
}

// hasweapon(name)
// Checks if the player has the specified weapon.
GS1ScriptValue fn_hasweapon(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: hasweapon(name)");

	const auto weaponName = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	if (const auto player = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); player.has_value())
	{
		const auto* server = BabyDI::Get<Server>();
		if (const auto playerObject = server->getNPCServer()->getPlayer(player.value().first); playerObject != nullptr)
			return GameValue{playerObject->account.hasWeapon(weaponName)};
	}

	return GameValue{false};
}

// imgheight(image)
// Returns the height of the specified image.
GS1ScriptValue fn_imgheight(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: imgheight(image)");
}

// imgwidth(image)
// Returns the width of the specified image.
GS1ScriptValue fn_imgwidth(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: imgwidth(image)");
}

// indexof(substring, string)
// Returns the index of the first occurrence of substring in the string, or -1 if not found.
GS1ScriptValue fn_indexof(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: indexof(substring, string)");

	const auto substring = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	auto str = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(""s);

	return GameValue{str.find(substring) != std::string::npos ? static_cast<double>(str.find(substring)) : -1.0};
}

// int(value)
// Converts the value to an integer.
GS1ScriptValue fn_int(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: int(value)");

	const auto value = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	return GameValue{static_cast<double>(static_cast<int64_t>(value))};
	/*
	if (value < 0.0)
		return static_cast<double>(static_cast<int64_t>(value - 0.5));
	else
		return static_cast<double>(static_cast<int64_t>(value + 0.5));
	*/
}

// keycode(key)
// Returns the Windows Virtual Key Code for the specified key.
GS1ScriptValue fn_keycode(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	// Use characters that can be typed on a US ANSI keyboard.
	using P = std::pair<char, uint8_t>;
	constexpr std::array<P, 60> vkcs =
	{
		std::make_pair('\t', 0x09_ui8),
		std::make_pair(' ', 0x20_ui8),
		std::make_pair('0', 0x30_ui8),
		std::make_pair('1', 0x31_ui8),
		std::make_pair('2', 0x32_ui8),
		std::make_pair('3', 0x33_ui8),
		std::make_pair('4', 0x34_ui8),
		std::make_pair('5', 0x35_ui8),
		std::make_pair('6', 0x36_ui8),
		std::make_pair('7', 0x37_ui8),
		std::make_pair('8', 0x38_ui8),
		std::make_pair('9', 0x39_ui8),
		std::make_pair('A', 0x41_ui8),
		std::make_pair('B', 0x42_ui8),
		std::make_pair('C', 0x43_ui8),
		std::make_pair('D', 0x44_ui8),
		std::make_pair('E', 0x45_ui8),
		std::make_pair('F', 0x46_ui8),
		std::make_pair('G', 0x47_ui8),
		std::make_pair('H', 0x48_ui8),
		std::make_pair('I', 0x49_ui8),
		std::make_pair('J', 0x4A_ui8),
		std::make_pair('K', 0x4B_ui8),
		std::make_pair('L', 0x4C_ui8),
		std::make_pair('M', 0x4D_ui8),
		std::make_pair('N', 0x4E_ui8),
		std::make_pair('O', 0x4F_ui8),
		std::make_pair('P', 0x50_ui8),
		std::make_pair('Q', 0x51_ui8),
		std::make_pair('R', 0x52_ui8),
		std::make_pair('S', 0x53_ui8),
		std::make_pair('T', 0x54_ui8),
		std::make_pair('U', 0x55_ui8),
		std::make_pair('V', 0x56_ui8),
		std::make_pair('W', 0x57_ui8),
		std::make_pair('X', 0x58_ui8),
		std::make_pair('Y', 0x59_ui8),
		std::make_pair('Z', 0x5A_ui8),
		//
		std::make_pair(';', 0xBA_ui8),
		std::make_pair(':', 0xBA_ui8),
		//
		std::make_pair('=', 0xBB_ui8),
		std::make_pair('+', 0xBB_ui8),
		//
		std::make_pair(',', 0xBC_ui8),
		std::make_pair('<', 0xBC_ui8),
		//
		std::make_pair('-', 0xBD_ui8),
		std::make_pair('_', 0xBD_ui8),
		//
		std::make_pair('.', 0xBE_ui8),
		std::make_pair('>', 0xBE_ui8),
		//
		std::make_pair('/', 0xBF_ui8),
		std::make_pair('?', 0xBF_ui8),
		//
		std::make_pair('`', 0xC0_ui8),
		std::make_pair('~', 0xC0_ui8),
		//
		std::make_pair('[', 0xDB_ui8),
		std::make_pair('{', 0xDB_ui8),
		//
		std::make_pair('\\', 0xDC_ui8),
		std::make_pair('|', 0xDC_ui8),
		//
		std::make_pair(']', 0xDD_ui8),
		std::make_pair('}', 0xDD_ui8),
		//
		std::make_pair('\'', 0xDE_ui8),
		std::make_pair('"', 0xDE_ui8),
	};

	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: keycode(key)");

	const auto key = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	if (key.empty())
		return GameValue{0.0};

	// Simple uppercase.
	char keycode = key.front();
	if (keycode >= 'a' && keycode <= 'z')
		keycode -= 32;

	// clang-format off
	const auto result = std::ranges::find_if(vkcs, [&keycode](const P& pair) { return pair.first == keycode; });
	if (result == std::ranges::end(vkcs))
		return GameValue{0.0};
	// clang-format on

	return GameValue{static_cast<double>(result->second)};
}

// keydown(key)
// Checks if the specified key is currently pressed down.  (0..10: up, left, down, right, S, A, D, M, tab, Q, P)
GS1ScriptValue fn_keydown(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: keydown(key)");
}

// keydown2(keycode, ignorecase)
// Checks if the specified key is currently pressed down, with an optional case-insensitive check for key codes.
// (ignorecase must be false to check for shift, ctrl, alt)
GS1ScriptValue fn_keydown2(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: keydown2(keycode, ignorecase)");
}

// lindexof(string, list)
// Returns the index of the first occurrence of string in the string list, or -1 if not found.
GS1ScriptValue fn_lindexof(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: lindexof(string, list)");

	const auto str = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	const auto list = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(""s);
	const auto listItems = string::splitToVectorView(list, ","sv);
	for (size_t i = 0; i < listItems.size(); ++i)
	{
		if (string::trim(listItems[i]) == string::trim(str))
			return GameValue{static_cast<double>(i)};
	}

	return GameValue{-1.0};
}

// log(base, value)
// Returns the logarithm of the value with the given base.
GS1ScriptValue fn_log(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: log(base, value)");

	const auto base = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	const auto value = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);
	if (value <= 0.0)
		return GameValue{0.0};

	return GameValue{std::log(value) / std::log(base)};
}

// max(value1, value2)
// Returns the maximum of the two values.
GS1ScriptValue fn_max(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: max(value1, value2)");

	const auto value1 = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	const auto value2 = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);

	if (value1 > value2)
		return GameValue{value1};
	else
		return GameValue{value2};
}

// min(value1, value2)
// Returns the minimum of the two values.
GS1ScriptValue fn_min(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: min(value1, value2)");

	const auto value1 = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	const auto value2 = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);

	if (value1 < value2)
		return GameValue{value1};
	else
		return GameValue{value2};
}

// onmapx(level)
// The level's X position on the map.
GS1ScriptValue fn_onmapx(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: onmapx(level)");

	if (const auto curLevel = visitor->findCurrentLevel(); curLevel != nullptr)
	{
		const auto level = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
		if (const auto map = curLevel->getMap(); map != nullptr)
			return GameValue{static_cast<double>(map->getLevelPosition(level).value_or(MapPosition{0, 0}).x())};
	}

	return GameValue{-1.0};
}

// onmapy(level)
// The level's Y position on the map.
GS1ScriptValue fn_onmapy(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: onmapy(level)");

	if (const auto curLevel = visitor->findCurrentLevel(); curLevel != nullptr)
	{
		const auto level = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
		if (const auto map = curLevel->getMap(); map != nullptr)
			return GameValue{static_cast<double>(map->getLevelPosition(level).value_or(MapPosition{0, 0}).y())};
	}

	return GameValue{-1.0};
}

// onwall(x, y)
// Checks if the specified X and Y coordinates are on a wall tile.
// Also checks if the coordinates are on a player or NPC, and returns true if so.
// isNoPkZone levels have no player collision, so it will skip players if true.
GS1ScriptValue fn_onwall(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: onwall(x, y)");

	auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
	auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		if (level->isOnWall(toPixelPosition({x, y})))
			return GameValue{true};

		if (const auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
		{
			const auto server = BabyDI::Get<Server>();
			if (const auto npc = server->getNPC(source.value().first); npc != nullptr)
			{
				const auto subLevel = level->getSubLevelAtPosition(npc->character.getMapPosition());
				if (subLevel == nullptr)
					return GameValue{true};
				if (!npc->noPlayerOnWall && !subLevel->isNoPkZone && level->isOnPlayer(toPixelPosition({x, y})))
					return GameValue{true};
				if (level->isOnNPC(toPixelPosition({x, y})))
					return GameValue{true};
			}
		}
	}

	return GameValue{false};
}

// onwall2(x, y, width, height)
// Checks if the specified rectangle defined by X, Y, width, and height is on a wall tile.
// Also checks if the rectangle is on a player or NPC, and returns true if so.
// isNoPkZone levels have no player collision, so it will skip players if true.
GS1ScriptValue fn_onwall2(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: onwall2(x, y, width, height)");

	auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
	auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
	auto width = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
	auto height = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0) * 16);

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		if (level->isOnWall2(PixelRectangleArea{toPixelPosition({x, y}), {width, height}}))
			return GameValue{true};

		if (const auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC); source.has_value())
		{
			const auto server = BabyDI::Get<Server>();
			if (const auto npc = server->getNPC(source.value().first); npc != nullptr)
			{
				const auto subLevel = level->getSubLevelAtPosition(npc->character.getMapPosition());
				if (subLevel == nullptr)
					return GameValue{true};

				const PixelRectangleArea searchRect{toPixelPosition({x, y}), {width, height}};
				if (!npc->noPlayerOnWall && !subLevel->isNoPkZone && level->isOnPlayer(searchRect))
					return GameValue{true};
				if (level->isOnNPC(searchRect))
					return GameValue{true};
			}
		}
	}

	return GameValue{false};
}

// onwater(x, y)
// Checks if the specified X and Y coordinates are on a water tile.
GS1ScriptValue fn_onwater(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: onwater(x, y)");

	auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
	auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		if (level->isOnWater(toPixelPosition({x, y})))
			return GameValue{true};
	}

	return GameValue{false};
}

// onwater2(x, y, width, height)
// Checks if the specified X and Y coordinates are on a water tile.
GS1ScriptValue fn_onwater2(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("invalid arguments: onwater2(x, y, width, height)");

	auto x = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
	auto y = static_cast<float>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
	auto width = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[2]).value_or(0.0) * 16);
	auto height = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[3]).value_or(0.0) * 16);

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		if (level->isOnWater2(PixelRectangleArea{toPixelPosition({x, y}), {width, height}}))
			return GameValue{true};
	}

	return GameValue{false};
}

// passwordmatches(encrypted, password)
// Checks if the provided encrypted password matches the specified password.
GS1ScriptValue fn_passwordmatches(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: passwordmatches(encrypted, password)");

	const auto param0 = GS1Visitor::getScriptValueAs<std::string>(*arguments[0]);
	const auto param1 = GS1Visitor::getScriptValueAs<std::string>(*arguments[1]);
	if (!param0.has_value() || !param1.has_value())
		return GameValue{false};

	const auto& encrypted = param0.value().get();
	const auto& password = param1.value().get();

	std::array<uint8_t, 32> hash{};
	hash_state sha256state{};
	sha256_init(&sha256state);
	sha256_process(&sha256state, reinterpret_cast<const unsigned char*>(password.data()), password.size());
	sha256_done(&sha256state, hash.data());

	// Calculate the length of the resulting base64 string.
	constexpr unsigned long SHA256BASE64 = 4 * ((hash.size() + 2) / 3) + 1;
	unsigned long outputLength = SHA256BASE64;
	std::array<char, SHA256BASE64> output{};
	base64_encode(reinterpret_cast<const unsigned char*>(hash.data()), static_cast<unsigned long>(hash.size()), output.data(), &outputLength);

	return GameValue{(encrypted == std::string{output.data(), outputLength})};
}

// playersays(text)
// playersays(index,text)
// Checks if the player says the specified text.
// Equivalent to "playerchats && strequals(#c,text)"
GS1ScriptValue fn_playersays(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index;
	std::string text;
	if (arguments.size() == 2)
	{
		const auto specifiedIndex = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(""s);
		if (specifiedIndex >= 0)
			index = static_cast<size_t>(specifiedIndex);
	}
	else
	{
		text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	}

	if (const auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		if (const auto player = getPlayerFromSource(*source, index); player != nullptr)
		{
			if (string::equalsi(player->account.character.chatMessage, text))
				return GameValue{true};
		}
	}

	return GameValue{false};
}

// playersays2(text)
// playersays2(index,text)
// Checks if the player's chat contains the specified text.
// Equivalent to "playerchats && strcontains(#c,text)"
GS1ScriptValue fn_playersays2(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index;
	std::string text;
	if (arguments.size() == 2)
	{
		const auto specifiedIndex = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
		text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(""s);
		if (specifiedIndex >= 0)
			index = static_cast<size_t>(specifiedIndex);
	}
	else
	{
		text = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	}

	if (const auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		if (const auto player = getPlayerFromSource(*source, index); player != nullptr)
		{
			if (string::findi(player->account.character.chatMessage, text) != std::string::npos)
				return GameValue{true};
		}
	}

	return GameValue{false};
}

// random(min, max)
// Returns a random number between min and max.  a <= value < b
GS1ScriptValue fn_random(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	using namespace std::chrono;
	static std::minstd_rand rng(static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()));

	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: random(min, max)");

	const auto value1 = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	const auto value2 = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);

	std::uniform_real_distribution dist(std::min(value1, value2), std::max(value1, value2));
	return GameValue{static_cast<double>(dist(rng))};
}

// sarraylen(list)
// Returns the length of the string list.
GS1ScriptValue fn_sarraylen(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: sarraylen(list)");

	auto list = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	return GameValue{static_cast<double>(std::ranges::count(list, ',') + 1)};
}

// screenx(x, y)
// Converts level coordinates (x, y) to the screen's X coordinate.
GS1ScriptValue fn_screenx(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: screenx(x, y)");
}

// screeny(x, y)
// Converts level coordinates (x, y) to the screen's Y coordinate.
GS1ScriptValue fn_screeny(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: screeny(x, y)");
}

// sin(value)
// Returns the sine of the value in radians.
GS1ScriptValue fn_sin(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: sin(value)");

	const auto value = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);

	if (value < 0 || value > std::numbers::pi)
		return GameValue{0.0};

	return GameValue{std::sin(value)};
}

// startswith(prefix, string)
// Checks if the string starts with the given prefix.
GS1ScriptValue fn_startswith(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: startswith(prefix, string)");

	const auto prefix = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	const auto str = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(""s);

	return GameValue{string::findi(str, prefix) == 0};
}

// strcontains(string, substring)
// Checks if the string contains the given substring.
GS1ScriptValue fn_strcontains(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: strcontains(string, substring)");

	const auto str = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	const auto substring = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(""s);

	return GameValue{string::findi(str, substring) != std::string::npos};
}

// strequals(string1, string2)
// Checks if the two strings are equal.
GS1ScriptValue fn_strequals(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: strequals(string1, string2)");

	const auto str1 = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	const auto str2 = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(""s);

	return GameValue{string::equalsi(str1, str2)};
}

// strlen(string)
// Returns the length of the string.
GS1ScriptValue fn_strlen(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: strlen(string)");

	const auto str = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);

	return GameValue{static_cast<double>(str.length())};
}

// strtofloat(string)
// Converts a string to a float.
GS1ScriptValue fn_strtofloat(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: strtofloat(string)");

	const auto str = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(""s);
	if (str.empty())
		return GameValue{0.0};

	return GameValue{string::toDouble(str)};
}

// testbomb(x, y)
// The index of the bomb at level position (x, y), or -1 if there is no bomb at that position.
GS1ScriptValue fn_testbomb(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: testbomb(x, y)");

	const auto x = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 16);
	const auto y = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto& bombs = level->getBombs();
		for (size_t i = 0; i < bombs.size(); ++i)
		{
			auto& bomb = bombs[i];
			if (inRange(bomb.position.x(), x, x + 2) && inRange(bomb.position.y(), y, y + 2))
				return GameValue{static_cast<double>(i)};
		}
	}

	return GameValue{-1.0};
}

// testcompu(x, y)
// The index of the baddie at level position (x, y), or -1 if there is no baddie at that position.
GS1ScriptValue fn_testcompu(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: testcompu(x, y)");

	const auto x = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 16);
	const auto y = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		size_t index = 0;
		for (const auto& baddy : level->getBaddies())
		{
			if (inRange(baddy.position.x(), x, x + 2) && inRange(baddy.position.y(), y, y + 3) && baddy.mode != BaddyMode::DEAD)
				return GameValue{static_cast<double>(index)};
			++index;
		}
	}

	return GameValue{-1.0};
}

// testexplo(x, y)
// The index of the explosion at level position (x, y), or -1 if there is no explosion at that position.
GS1ScriptValue fn_testexplo(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: testexplo(x, y)");

	const auto x = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 16);
	const auto y = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto& explos = level->getExplosions();
		for (size_t i = 0; i < explos.size(); ++i)
		{
			auto& explo = explos[i];
			if (inRange(explo.position.x(), x, x + 2) && inRange(explo.position.y(), y, y + 2))
				return GameValue{static_cast<double>(i)};
		}
	}

	return GameValue{-1.0};
}

// testhorse(x, y)
// The index of the horse at level position (x, y), or -1 if there is no horse at that position.
GS1ScriptValue fn_testhorse(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: testhorse(x, y)");

	const auto x = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 16);
	const auto y = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto& horses = level->getHorses();
		for (size_t i = 0; i < horses.size(); ++i)
		{
			auto& horse = horses[i];
			if (inRange(horse.position.x(), x, x + 2) && inRange(horse.position.y(), y, y + 3))
				return GameValue{static_cast<double>(i)};
		}
	}

	return GameValue{-1.0};
}

// testitem(x, y)
// The index of the item at level position (x, y), or -1 if there is no item at that position.
GS1ScriptValue fn_testitem(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: testitem(x, y)");

	const auto x = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 16);
	const auto y = DoubleAsIntegralFloor<int16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto& items = level->getItems();
		for (size_t i = 0; i < items.size(); ++i)
		{
			auto& item = items[i];
			if (inRange(item.position.x(), x, x + 2) && inRange(item.position.y(), y, y + 2))
				return GameValue{static_cast<double>(i)};
		}
	}

	return GameValue{-1.0};
}

// testnpc(x, y)
// The index of the NPC at level position (x, y), or -1 if there is no NPC at that position.
GS1ScriptValue fn_testnpc(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: testnpc(x, y)");

	const auto x = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 16);
	const auto y = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);
	const auto position = PixelPosition{x, y};

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		const auto* server = BabyDI::Get<Server>();

		bool found = false;
		size_t index = 0;
		for (const auto& npcId : level->findInRangeNPCs(position))
		{
			if (auto npc = server->getNPC(npcId); npc != nullptr)
			{
				if (positionInRectangle(position, npc->getCollisionBoundingBox()))
				{
					found = true;
					break;
				}
			}
			++index;
		}

		if (found)
			return GameValue{static_cast<double>(index)};
	}

	return GameValue{-1.0};
}

// testplayer(x, y)
// The index of the player at level position (x, y), or -2 if there is no player at that position.
GS1ScriptValue fn_testplayer(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: testplayer(x, y)");

	const auto x = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0) * 16);
	const auto y = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0) * 16);
	const auto position = PixelPosition{x, y};
	const auto* server = BabyDI::Get<Server>();

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		bool found = false;
		size_t index = 0;
		for (const auto& playerId : level->findInRangePlayers(position))
		{
			if (auto player = server->getPlayer(playerId); player != nullptr)
			{
				if (positionInRectangle(position, player->getCollisionBoundingBox()))
				{
					found = true;
					break;
				}
			}
			++index;
		}

		if (found)
			return GameValue{static_cast<double>(index)};
	}

	return GameValue{-2.0};
}

// testsign(x, y)
// The index of the sign at level position (x, y), or -1 if there is no sign at that position.
GS1ScriptValue fn_testsign(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: testsign(x, y)");

	const auto x = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
	const auto y = DoubleAsIntegralFloor<uint16_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		size_t index = 0;
		for (const auto& [_, position] : level->getSignPositions())
		{
			if (position.x() == x && position.y() == y)
				return GameValue{static_cast<double>(index)};
			++index;
		}
	}
	return GameValue{-1.0};
}

// textheight(zoom, font, style)
// Returns the height of the text in pixels, given the zoom level, font name, and style.
GS1ScriptValue fn_textheight(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: textheight(zoom, font, style)");
}

// textwidth(zoom, font, style, text)
// Returns the width of the text in pixels, given the zoom level, font name, style, and text.
GS1ScriptValue fn_textwidth(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: textwidth(zoom, font, style, text)");
}

// tiletype(x, y)
// Returns the tile type at level position (x, y).
GS1ScriptValue fn_tiletype(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: tiletype(x, y)");

	if (const auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		const auto x = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
		const auto y = GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0);

		const auto tilePosition = toTilePosition(Position<double>{x, y});
		auto mapPosition = toMapPosition(tilePosition);

		if (!level->isGmap())
			mapPosition = {0, 0};

		if (const auto tiles = level->getTiles(mapPosition); tiles.has_value())
		{
			const auto index = static_cast<size_t>(std::max(x, 0.0) + (std::max(y, 0.0) * 64));
			if (index < 4096)
			{
				const auto server = BabyDI::Get<Server>();
				const auto tile = tiles.value()->at(index);
				return GameValue{static_cast<double>(ENUM(server->getNPCServer()->getTileType(tile, level)))};
			}
		}
	}

	// Not found?  Default to blocking.
	return GameValue{22.0};
}

// vecx(dir)
// Returns the X component of the vector for the specified direction (0,-1,0,1) for (up, left, down, right).
GS1ScriptValue fn_vecx(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: vecx(dir)");

	static double vecValues[] = {0.0, -1.0, 0.0, 1.0};
	const auto dir = DoubleAsIntegralFloor<int8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)) % 4;
	return GameValue{vecValues[dir]};
}

// vecy(dir)
// Returns the Y component of the vector for the specified direction (-1,0,1,0) for (up, left, down, right).
GS1ScriptValue fn_vecy(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: vecy(dir)");

	static double vecValues[] = {-1.0, 0.0, 1.0, 0.0};
	const auto dir = DoubleAsIntegralFloor<int8_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)) % 4;
	return GameValue{vecValues[dir]};
}

// worldx(x, y)
// Converts screen (x, y) to level X.
GS1ScriptValue fn_worldx(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: worldx(x, y)");
}

// worldy(x, y)
// Converts screen (x, y) to level Y.
GS1ScriptValue fn_worldy(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: worldy(x, y)");
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
