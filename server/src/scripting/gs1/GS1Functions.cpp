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
#include <numbers>
#include <random>
#include <stdexcept>
#include <string_view>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <tree/ParseTree.h>

#include <BabyDI.h>
#include <IEnums.h>

#include <Server.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <scripting/gs1/GS1Functions.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

using BuiltInFunctionHandleFunc = GS1ScriptValue(*)(GS1Visitor*, std::string_view, const std::vector<GS1ScriptValue*>&);
using BuiltInFunctionHandleMap = std::unordered_map<size_t, BuiltInFunctionHandleFunc>;

static GS1ScriptValue fn__(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_N_(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_abs(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_arctan(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_cos(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_int(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_log(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_max(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_min(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_random(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_sin(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_strtofloat(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_ascii(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_base64decode(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_base64encode(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_startswith(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_strcontains(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_strequals(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_strlen(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_aindexof(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_arraylen(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_indexof(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_lindexof(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_sarraylen(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_findnearestplayer(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_findnearestplayers(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getangle(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getareanpcs(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getdir(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getnearestplayer(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getnearestplayers(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getnpc(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getplayer(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_getz(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_groundsheight(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_hasweapon(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_imgheight(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_imgwidth(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_keycode(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_keydown(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_keydown2(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onmapx(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onmapy(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onwall(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onwall2(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_onwater(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_playersays(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_playersays2(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_screenx(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_screeny(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testbomb(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testcompu(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testexplo(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testhorse(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testitem(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testnpc(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testplayer(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_testsign(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_textheight(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_textwidth(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_tiletype(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_vecx(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_vecy(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_waterheight(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_worldx(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue fn_worldy(GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);

static BuiltInFunctionHandleMap GenerateMap()
{
	string::string_hash hash{};
	BuiltInFunctionHandleMap map =
	{
		{ hash("_"), &fn__ },
		{ hash("N_"), &fn_N_ },
		{ hash("abs"), &fn_abs },
		{ hash("arctan"), &fn_arctan },
		{ hash("cos"), &fn_cos },
		{ hash("int"), &fn_int },
		{ hash("log"), &fn_log },
		{ hash("max"), &fn_max },
		{ hash("min"), &fn_min },
		{ hash("random"), &fn_random },
		{ hash("sin"), &fn_sin },
		{ hash("strtofloat"), &fn_strtofloat },
		{ hash("ascii"), &fn_ascii },
		{ hash("base64decode"), &fn_base64decode },
		{ hash("base64encode"), &fn_base64encode },
		{ hash("startswith"), &fn_startswith },
		{ hash("strcontains"), &fn_strcontains },
		{ hash("strequals"), &fn_strequals },
		{ hash("strlen"), &fn_strlen },
		{ hash("aindexof"), &fn_aindexof },
		{ hash("arraylen"), &fn_arraylen },
		{ hash("indexof"), &fn_indexof },
		{ hash("lindexof"), &fn_lindexof },
		{ hash("sarraylen"), &fn_sarraylen },
		{ hash("findnearestplayer"), &fn_findnearestplayer },
		{ hash("findnearestplayers"), &fn_findnearestplayers },
		{ hash("getangle"), &fn_getangle },
		{ hash("getareanpcs"), &fn_getareanpcs },
		{ hash("getdir"), &fn_getdir },
		{ hash("getnearestplayer"), &fn_getnearestplayer },
		{ hash("getnearestplayers"), &fn_getnearestplayers },
		{ hash("getnpc"), &fn_getnpc },
		{ hash("getplayer"), &fn_getplayer },
		{ hash("getz"), &fn_getz },
		{ hash("groundsheight"), &fn_groundsheight },
		{ hash("hasweapon"), &fn_hasweapon },
		{ hash("imgheight"), &fn_imgheight },
		{ hash("imgwidth"), &fn_imgwidth },
		{ hash("keycode"), &fn_keycode },
		{ hash("keydown"), &fn_keydown },
		{ hash("keydown2"), &fn_keydown2 },
		{ hash("onmapx"), &fn_onmapx },
		{ hash("onmapy"), &fn_onmapy },
		{ hash("onwall"), &fn_onwall },
		{ hash("onwall2"), &fn_onwall2 },
		{ hash("onwater"), &fn_onwater },
		{ hash("playersays"), &fn_playersays },
		{ hash("playersays2"), &fn_playersays2 },
		{ hash("screenx"), &fn_screenx },
		{ hash("screeny"), &fn_screeny },
		{ hash("testbomb"), &fn_testbomb },
		{ hash("testcompu"), &fn_testcompu },
		{ hash("testexplo"), &fn_testexplo },
		{ hash("testhorse"), &fn_testhorse },
		{ hash("testitem"), &fn_testitem },
		{ hash("testnpc"), &fn_testnpc },
		{ hash("testplayer"), &fn_testplayer },
		{ hash("testsign"), &fn_testsign },
		{ hash("textheight"), &fn_textheight },
		{ hash("textwidth"), &fn_textwidth },
		{ hash("tiletype"), &fn_tiletype },
		{ hash("vecx"), &fn_vecx },
		{ hash("vecy"), &fn_vecy },
		{ hash("waterheight"), &fn_waterheight },
		{ hash("worldx"), &fn_worldx },
		{ hash("worldy"), &fn_worldy },
	};
	return map;
}

constexpr std::array<std::string_view, 2> flagProcessingFunctions =
{
	"lindexof"sv,
	"sarraylen"sv,
};

///////////////////////////////////////////////////////////////////////////////

GS1ScriptValue processBuiltInFunction(GS1Visitor* visitor, antlr4::tree::ParseTree* node, std::string_view functionName)
{
	static BuiltInFunctionHandleMap map = GenerateMap();

	if (visitor == nullptr)
		throw std::runtime_error("processBuiltInFunction received an empty visitor");
	if (functionName.empty())
		throw std::runtime_error("processBuiltInFunction received an empty function name");

	// Find the command in the map.
	size_t hash = string::string_hash{}(functionName);
	auto it = map.find(hash);
	if (it == map.end())
	{
		log::printLine(log::script, "Unknown function in NPC '{}': {}", visitor->who, functionName);
		return {};
	}

	// Record if we are expecting a flag.
	bool oldExpectingFlag = visitor->expectingFlag;
	visitor->expectingFlag = (std::ranges::find(flagProcessingFunctions, functionName) != std::ranges::end(flagProcessingFunctions));

	// Collect the arguments from the node.
	std::vector<GS1ScriptValue*> arguments;
	auto children = visitor->visitChildrenAndCollect(node);
	for (auto& result : children)
	{
		auto* container = std::any_cast<GS1ScriptValue>(&result);
		if (container == nullptr)
			throw std::runtime_error("BuiltInFunction argument is not a valid GS1ScriptValue");

		// Add to the arguments.
		arguments.push_back(container);
	}

	// Reset the expectingFlag toggle back to normal.
	visitor->expectingFlag = oldExpectingFlag;

	// Execute the command.
	return it->second(visitor, functionName, arguments);
}

///////////////////////////////////////////////////////////////////////////////

// _(string)
// Translates the string according to the client's language settings.
GS1ScriptValue fn__(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function _ not implemented");
}

// N_(string)
// Does not translate the string, but adds a stub for it in the server's translation files.
GS1ScriptValue fn_N_(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function N_ requires exactly one argument");

	auto str = visitor->getGameValueAs<std::string>(*arguments[0]);

	// TODO: Implement this.

	return str;
}

//----------------------------

// abs(value)
// Absolute value of a number.
GS1ScriptValue fn_abs(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function abs requires exactly one argument");

	auto value = visitor->getGameValueAs<double>(*arguments[0]);
	return std::abs(value);
}

// arctan(value)
// Returns the arctangent of the value in radians.
GS1ScriptValue fn_arctan(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function arctan requires exactly one argument");

	auto value = visitor->getGameValueAs<double>(*arguments[0]);
	return std::atan(value);
}

// cos(value)
// Returns the cosine of the value in radians.
GS1ScriptValue fn_cos(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function cos requires exactly one argument");

	auto value = visitor->getGameValueAs<double>(*arguments[0]);
	return std::cos(value);
}

// int(value)
// Converts the value to an integer.
GS1ScriptValue fn_int(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function int requires exactly one argument");

	auto value = visitor->getGameValueAs<double>(*arguments[0]);
	return static_cast<double>(static_cast<int64_t>(value));
	/*
	if (value < 0.0)
		return static_cast<double>(static_cast<int64_t>(value - 0.5));
	else
		return static_cast<double>(static_cast<int64_t>(value + 0.5));
	*/
}

// log(value)
// Returns the natural logarithm of the value.
GS1ScriptValue fn_log(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function log requires exactly one argument");

	auto value = visitor->getGameValueAs<double>(*arguments[0]);
	if (value <= 0.0)
		return 0.0;

	return std::log(value);
}

// max(value1, value2)
// Returns the maximum of the two values.
GS1ScriptValue fn_max(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function max requires exactly two arguments");

	auto value1 = visitor->getGameValueAs<double>(*arguments[0]);
	auto value2 = visitor->getGameValueAs<double>(*arguments[1]);

	if (value1 > value2)
		return value1;
	else
		return value2;
}

// min(value1, value2)
// Returns the minimum of the two values.
GS1ScriptValue fn_min(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function max requires exactly two arguments");

	auto value1 = visitor->getGameValueAs<double>(*arguments[0]);
	auto value2 = visitor->getGameValueAs<double>(*arguments[1]);

	if (value1 < value2)
		return value1;
	else
		return value2;
}

// random(min, max)
// Returns a random number between min and max.  a <= value < b
GS1ScriptValue fn_random(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function max requires exactly two arguments");

	auto value1 = static_cast<int64_t>(visitor->getGameValueAs<double>(*arguments[0]));
	auto value2 = static_cast<int64_t>(visitor->getGameValueAs<double>(*arguments[1]));

	using namespace std::chrono;
	auto seed = static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
	std::minstd_rand rng(seed);
	std::uniform_int_distribution<int32_t> dist(value1, value2 - 1);

	return static_cast<double>(dist(rng));
}

// sin(value)
// Returns the sine of the value in radians.
GS1ScriptValue fn_sin(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function sin requires exactly one argument");

	auto value = visitor->getGameValueAs<double>(*arguments[0]);

	if (value < 0 || value > std::numbers::pi)
		return 0.0;

	return std::sin(value);
}

// strtofloat(string)
// Converts a string to a float.
GS1ScriptValue fn_strtofloat(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function strtofloat requires exactly one argument");

	auto str = visitor->getGameValueAs<std::string>(*arguments[0]);
	if (str.empty())
		return 0.0;

	return string::toDouble(str);
}

//----------------------------

// ascii(string)
// Returns the ASCII value of the first character in the string.
GS1ScriptValue fn_ascii(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function ascii requires exactly one argument");

	auto str = visitor->getGameValueAs<std::string>(*arguments[0]);
	if (str.empty())
		return 0.0;

	return static_cast<double>(static_cast<uint8_t>(str[0]));
}

// base64decode(string)
// Decodes a Base64 encoded string.
GS1ScriptValue fn_base64decode(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function base64decode not implemented");
}

// base64encode(string)
// Encodes a string to Base64 format.
GS1ScriptValue fn_base64encode(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function base64encode not implemented");
}

// startswith(prefix, string)
// Checks if the string starts with the given prefix.
GS1ScriptValue fn_startswith(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function startswith requires exactly two arguments");

	auto prefix = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto str = visitor->getGameValueAs<std::string>(*arguments[1]);

	return str.starts_with(prefix) ? 1.0 : 0.0;
}

// strcontains(string, substring)
// Checks if the string contains the given substring.
GS1ScriptValue fn_strcontains(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function strcontains requires exactly two arguments");

	auto str = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto substring = visitor->getGameValueAs<std::string>(*arguments[1]);

	return str.find(substring) != std::string::npos ? 1.0 : 0.0;
}

// strequals(string1, string2)
// Checks if the two strings are equal.
GS1ScriptValue fn_strequals(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function strequals requires exactly two arguments");

	auto str1 = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto str2 = visitor->getGameValueAs<std::string>(*arguments[1]);

	return str1 == str2 ? 1.0 : 0.0;
}

// strlen(string)
// Returns the length of the string.
GS1ScriptValue fn_strlen(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function strlen requires exactly one argument");

	auto str = visitor->getGameValueAs<std::string>(*arguments[0]);

	return static_cast<double>(str.length());
}

//----------------------------

// aindexof(value, array)
// Returns the index of the first occurrence of value in the array, or -1 if not found.
GS1ScriptValue fn_aindexof(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function aindexof requires exactly two arguments");

	auto value = visitor->getGameValueAs<double>(*arguments[0]);
	auto array = visitor->getGameValueAs<std::vector<double>>(*arguments[1]);

	auto result = std::ranges::find(array, value);
	if (result == std::ranges::end(array))
		return -1.0;

	auto distance = std::ranges::distance(std::ranges::begin(array), result);
	return static_cast<double>(distance);
}

// arraylen(array)
// Returns the length of the array.
GS1ScriptValue fn_arraylen(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function arraylen requires exactly one argument");

	auto array = visitor->getGameValueAs<std::vector<double>>(*arguments[0]);

	return static_cast<double>(array.size());
}

// indexof(substring, string)
// Returns the index of the first occurrence of substring in the string, or -1 if not found.
GS1ScriptValue fn_indexof(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function indexof requires exactly two arguments");

	auto substring = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto str = visitor->getGameValueAs<std::string>(*arguments[1]);

	return str.find(substring) != std::string::npos ? static_cast<double>(str.find(substring)) : -1.0;
}

// lindexof(string, list)
// Returns the index of the first occurrence of string in the string list, or -1 if not found.
GS1ScriptValue fn_lindexof(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function lindexof requires exactly two arguments");

	auto str = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto list = visitor->getGameValueAs<std::string>(*arguments[1]);
	auto listItems = string::splitHard(list, ","sv);
	for (auto i = 0; i < listItems.size(); ++i)
	{
		if (string::trim(listItems[i]) == string::trim(str))
			return static_cast<double>(i);
	}

	return -1.0;
}

// sarraylen(list)
// Returns the length of the string list.
GS1ScriptValue fn_sarraylen(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function sarraylen requires exactly one argument");

	auto list = visitor->getGameValueAs<std::string>(*arguments[0]);
	return static_cast<double>(std::ranges::count(list, ',') + 1);
}

//----------------------------

// findnearestplayer(x, y)
// Finds the nearest player to the specified position and returns a player source.
GS1ScriptValue fn_findnearestplayer(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function findnearestplayer requires exactly two arguments");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = visitor->getGameValueAs<double>(*arguments[0]);
		auto y = visitor->getGameValueAs<double>(*arguments[1]);

		// Find the nearest player.
		std::tuple<PlayerID, double> nearestPlayer{ 0, std::numeric_limits<double>::max() };
		auto* server = BabyDI::Get<Server>();
		for (const auto& id : level->getPlayers())
		{
			if (auto player = server->getNPCServer()->getPlayer(id); player != nullptr)
			{
				auto distance = std::hypot(player->getX() - x, player->getY() - y);
				if (distance < std::get<1>(nearestPlayer))
					nearestPlayer = { id, distance };
			}
		}

		// Return the closest player.
		if (std::get<0>(nearestPlayer) != 0)
			return ScriptObjectSource{ std::get<0>(nearestPlayer), ScriptObjectSourceType::PLAYER };
	}

	return 0.0;
}

// findnearestplayers(x, y)
// Finds all players in the level, orders them by distance from the specified position, and returns a list of player sources.
// Probably not supported in GS1.
GS1ScriptValue fn_findnearestplayers(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function findnearestplayers not implemented");
}

// getangle(dx, dy)
// Returns the angle in radians from the current position to the position specified by dx and dy.
GS1ScriptValue fn_getangle(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function getangle not implemented");
}

// getareanpcs(x, y, width, height)
// Returns the indices of all NPCS in the area specified.
GS1ScriptValue fn_getareanpcs(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 4)
		throw std::invalid_argument("Built-in function getareanpcs requires exactly four arguments");

	std::vector<double> result;
	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = static_cast<int16_t>(visitor->getGameValueAs<double>(*arguments[0]) * 16);
		auto y = static_cast<int16_t>(visitor->getGameValueAs<double>(*arguments[1]) * 16);
		auto width = static_cast<uint16_t>(visitor->getGameValueAs<double>(*arguments[2]) * 16);
		auto height = static_cast<uint16_t>(visitor->getGameValueAs<double>(*arguments[3]) * 16);

		auto npcs = level->findIntersectingNPCs({ { x, y }, { width, height } }, true);
		for (auto id : npcs)
			result.emplace_back(static_cast<double>(id));
	}
	return result;
}

// getdir(dx, dy)
// Returns the direction to look in the relative position specified by dx and dy.
GS1ScriptValue fn_getdir(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (auto* character = getCharacterFromSource(visitor->getOriginalSource()); character != nullptr)
	{
		if (arguments.size() != 2)
			throw std::invalid_argument("Built-in function getdir requires exactly two arguments");

		auto dx = visitor->getGameValueAs<double>(*arguments[0]);
		auto dy = visitor->getGameValueAs<double>(*arguments[1]);
		auto ix = static_cast<int>(std::min(-1.0, std::max(1.0, std::round(dx))));
		auto iy = static_cast<int>(std::min(-1.0, std::max(1.0, std::round(dy))));

		// Up
		if (ix == 0 && iy == -1)
			return 0.0;
		// Left
		if (ix == -1 && iy == 0)
			return 1.0;
		// Down
		if (ix == 0 && iy == 1)
			return 2.0;
		// Right
		if (ix == 1 && iy == 0)
			return 3.0;
	}

	// Default to looking down.
	return 2.0;
}

// getnearestplayer(x, y)
// Finds the nearest player to the specified position and returns the player index.
GS1ScriptValue fn_getnearestplayer(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function getnearestplayer requires exactly two arguments");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = visitor->getGameValueAs<double>(*arguments[0]);
		auto y = visitor->getGameValueAs<double>(*arguments[1]);

		// Find the nearest player.
		std::tuple<PlayerID, double> nearestPlayer{ 0, std::numeric_limits<double>::max() };
		auto* server = BabyDI::Get<Server>();
		for (const auto& id : level->getPlayers())
		{
			if (auto player = server->getNPCServer()->getPlayer(id); player != nullptr)
			{
				auto distance = std::hypot(player->getX() - x, player->getY() - y);
				if (distance < std::get<1>(nearestPlayer))
					nearestPlayer = { id, distance };
			}
		}

		// Return the closest player.
		if (std::get<0>(nearestPlayer) != 0)
			return static_cast<double>(std::get<0>(nearestPlayer));
	}

	return 0.0;
}

// getnearestplayers(x, y, flag)
// Returns an array of all the level players sorted by how close they are to the specified position, containing the optional flag.
GS1ScriptValue fn_getnearestplayers(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() < 2)
		throw std::invalid_argument("Built-in function getnearestplayers requires two or three arguments");

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto x = visitor->getGameValueAs<double>(*arguments[0]);
		auto y = visitor->getGameValueAs<double>(*arguments[1]);

		std::string flag;
		if (arguments.size() > 2)
			flag = visitor->getGameValueAs<std::string>(*arguments[2]);

		std::map<double, PlayerID> playersByDistance;
		auto* server = BabyDI::Get<Server>();
		for (const auto& id : level->getPlayers())
		{
			if (auto player = server->getNPCServer()->getPlayer(id); player != nullptr)
			{
				if (!flag.empty() && !player->account.variables.contains(flag))
					continue;

				auto distance = std::hypot(player->getX() - x, player->getY() - y);
				playersByDistance.emplace(distance, id);
			}
		}

		// Construct the player ID array.
		std::vector<double> playerIds;
		for (const auto& [distance, id] : playersByDistance)
			playerIds.push_back(static_cast<double>(id));

		// Return it.
		return playerIds;
	}

	return std::vector<double>();
}

// getnpc(name)
// Returns an NPC object that links to the NPC, or a false value if not found.
GS1ScriptValue fn_getnpc(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function getnpc requires exactly one argument");

	auto npcName = visitor->getGameValueAs<std::string>(*arguments[0]);

	auto* server = BabyDI::Get<Server>();
	auto& npcList = server->getNPCList();
	for (auto& [id, npc] : npcList)
	{
		if (npc->name == npcName)
			return ScriptObjectSource{ id, ScriptObjectSourceType::NPC };
	}

	return 0.0;
}

// getplayer(account)
// Returns a Player object that links to the player with the specified account name, or a false value if not found.
GS1ScriptValue fn_getplayer(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function getplayer requires exactly one argument");

	auto playerName = visitor->getGameValueAs<std::string>(*arguments[0]);

	auto* server = BabyDI::Get<Server>();
	if (auto player = server->getNPCServer()->getPlayer(playerName, PLTYPE_ANYPLAYER); player != nullptr)
	{
		return ScriptObjectSource{ player->getId(), ScriptObjectSourceType::PLAYER };
	}

	return 0.0;
}

// getz(x, y)
// Returns the Z coordinate at the specified X and Y position in the world.
GS1ScriptValue fn_getz(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function getz not implemented");
}

GS1ScriptValue fn_groundsheight(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function groundsheight not implemented");
}

// hasweapon(name)
// Checks if the player has the specified weapon.
GS1ScriptValue fn_hasweapon(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function hasweapon requires exactly one argument");

	auto weaponName = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto player = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::PLAYER);
	if (player.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto playerObject = server->getNPCServer()->getPlayer(player.value().first); playerObject != nullptr)
			return playerObject->account.hasWeapon(weaponName) ? 1.0 : 0.0;
	}

	return 0.0;
}

// imgheight(image)
// Returns the height of the specified image.
GS1ScriptValue fn_imgheight(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function imgheight is a clientside function");
}

// imgwidth(image)
// Returns the width of the specified image.
GS1ScriptValue fn_imgwidth(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function imgwidth is a clientside function");
}

// keycode(key)
// Returns the key code for the specified key.
GS1ScriptValue fn_keycode(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function keycode is a clientside function");
}

// keydown(key)
// Checks if the specified key is currently pressed down.  (0..10: up, left, down, right, S, A, D, M, tab, Q, P)
GS1ScriptValue fn_keydown(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function keydown is a clientside function");
}

// keydown2(keycode, ignorecase)
// Checks if the specified key is currently pressed down, with an optional case-insensitive check for key codes.
// (ignorecase must be false to check for shift, ctrl, alt)
GS1ScriptValue fn_keydown2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function keydown2 is a clientside function");
}

// onmapx(level)
// The level's X position on the map.
GS1ScriptValue fn_onmapx(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function onmapx requires exactly one argument");

	auto level = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto* server = BabyDI::Get<Server>();
	if (auto levelPtr = server->getLevel(level); levelPtr != nullptr)
		return static_cast<double>(levelPtr->getMapX());

	return 0.0;
}

// onmapy(level)
// The level's Y position on the map.
GS1ScriptValue fn_onmapy(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function onmapy requires exactly one argument");

	auto level = visitor->getGameValueAs<std::string>(*arguments[0]);
	auto* server = BabyDI::Get<Server>();
	if (auto levelPtr = server->getLevel(level); levelPtr != nullptr)
		return static_cast<double>(levelPtr->getMapY());

	return 0.0;
}

// onwall(x, y)
// Checks if the specified X and Y coordinates are on a wall tile.
GS1ScriptValue fn_onwall(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function onwall requires exactly two arguments");

	auto x = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
	auto y = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[1]));

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		if (!level->isOnWall({ x, y }))
			return 0.0;

		if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
		{
			auto server = BabyDI::Get<Server>();
			if (auto npc = server->getNPC(source.value().first); npc != nullptr && !npc->noPlayerOnWall)
				return level->isOnPlayer({ x, y }) ? 1.0 : 0.0;
		}
		return 1.0;
	}
	return 0.0;
}

// onwall2(x, y, width, height)
// Checks if the specified rectangle defined by X, Y, width, and height is on a wall tile.
GS1ScriptValue fn_onwall2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function onwall2 requires exactly four arguments");

	auto x = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
	auto y = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[1]));
	auto width = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[2]));
	auto height = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[3]));

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		if (!level->isOnWall2({ { x, y }, { width, height } }))
			return 0.0;

		if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectSourceType::NPC); source.has_value())
		{
			auto server = BabyDI::Get<Server>();
			if (auto npc = server->getNPC(source.value().first); npc != nullptr && !npc->noPlayerOnWall)
				return level->isOnPlayer({ { x, y }, { width, height } }) ? 1.0 : 0.0;
		}
		return 1.0;
	}
	return 0.0;
}

// onwater(x, y)
// Checks if the specified X and Y coordinates are on a water tile.
GS1ScriptValue fn_onwater(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function onwater requires exactly two arguments");

	auto x = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
	auto y = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[1]));

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		if (level->isOnWater({ x, y }))
			return 1.0;
	}
	return 0.0;
}

// playersays ???
GS1ScriptValue fn_playersays(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function playersays not implemented");
}

// playersays2 ???
GS1ScriptValue fn_playersays2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function playersays2 not implemented");
}

// screenx(x, y)
// Converts level coordinates (x, y) to the screen's X coordinate.
GS1ScriptValue fn_screenx(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("Built-in function screenx is a clientside function");
}

// screeny(x, y)
// Converts level coordinates (x, y) to the screen's Y coordinate.
GS1ScriptValue fn_screeny(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("Built-in function screeny is a clientside function");
}

// testbomb(x, y)
// The index of the bomb at level position (x, y), or -1 if there is no bomb at that position.
GS1ScriptValue fn_testbomb(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function testbomb not implemented");
}

// testcompu(x, y)
// The index of the baddie at level position (x, y), or -1 if there is no baddie at that position.
GS1ScriptValue fn_testcompu(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function testcompu requires exactly two arguments");

	/*
	auto x = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[0]));
	auto y = static_cast<uint8_t>(visitor->getGameValueAs<double>(*arguments[1]));

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto* server = BabyDI::Get<Server>();
		// TODO: Need a way to iterate through the level baddies.
	}
	return 0.0;
	*/

	throw unimplemented_error("Built-in function testcompu not implemented");
}

// testexplo(x, y)
// The index of the explosion at level position (x, y), or -1 if there is no explosion at that position.
GS1ScriptValue fn_testexplo(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function testexplo not implemented");
}

// testhorse(x, y)
// The index of the horse at level position (x, y), or -1 if there is no horse at that position.
GS1ScriptValue fn_testhorse(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function testhorse not implemented");
}

// testitem(x, y)
// The index of the item at level position (x, y), or -1 if there is no item at that position.
GS1ScriptValue fn_testitem(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function testitem not implemented");
}

// testnpc(x, y)
// The index of the NPC at level position (x, y), or -1 if there is no NPC at that position.
GS1ScriptValue fn_testnpc(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function testnpc requires exactly two arguments");

	auto x = static_cast<int16_t>(visitor->getGameValueAs<double>(*arguments[0]) * 16);
	auto y = static_cast<int16_t>(visitor->getGameValueAs<double>(*arguments[1]) * 16);

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto& npcs = level->getNPCs();
		auto* server = BabyDI::Get<Server>();
		for (size_t i = 0; i < npcs.size(); ++i)
		{
			if (auto npc = server->getNPC(npcs[i]); npc != nullptr)
			{
				if (positionInRectangle(Position<int16_t>{ x, y }, npc->getBoundingBox()))
					return static_cast<double>(i);
			}
		}
	}
	return -1.0;
}

// testplayer(x, y)
// The index of the player at level position (x, y), or -2 if there is no player at that position.
// -1 is reserved for the current npc if showcharacter is enabled.
GS1ScriptValue fn_testplayer(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function testplayer requires exactly two arguments");

	auto x = static_cast<int16_t>(visitor->getGameValueAs<double>(*arguments[0]) * 16);
	auto y = static_cast<int16_t>(visitor->getGameValueAs<double>(*arguments[1]) * 16);

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto& players = level->getPlayers();
		auto* server = BabyDI::Get<Server>();
		for (size_t i = 0; i < players.size(); ++i)
		{
			if (auto player = server->getNPCServer()->getPlayer(players[i]); player != nullptr)
			{
				auto bbox = player->getBoundingBox();
				if (positionInRectangle(Position<int16_t>{ x, y }, bbox))
					return static_cast<double>(i);
			}
		}
	}
	return -2.0;
}

// testsign(x, y)
// The index of the sign at level position (x, y), or -1 if there is no sign at that position.
GS1ScriptValue fn_testsign(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("Built-in function testsign requires exactly two arguments");

	auto x = static_cast<int16_t>(visitor->getGameValueAs<double>(*arguments[0]));
	auto y = static_cast<int16_t>(visitor->getGameValueAs<double>(*arguments[1]));

	if (auto level = visitor->findCurrentLevel(); level != nullptr)
	{
		auto& signs = level->getSigns();
		auto* server = BabyDI::Get<Server>();
		for (size_t i = 0; i < signs.size(); ++i)
		{
			auto& sign = signs[i];
			if (sign->getX() == x && sign->getY() == y)
				return static_cast<double>(i);
		}
	}
	return -1.0;
}

// textheight(zoom, font, style)
// Returns the height of the text in pixels, given the zoom level, font name, and style.
GS1ScriptValue fn_textheight(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function textheight is a clientside function");
}

// textwidth(zoom, font, style, text)
// Returns the width of the text in pixels, given the zoom level, font name, style, and text.
GS1ScriptValue fn_textwidth(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function textwidth is a clientside function");
}

// tiletype(x, y)
// Returns the "new order" tile type used for setshape2 on level position (x, y).
GS1ScriptValue fn_tiletype(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function tiletype not implemented");
}

// vecx(dir)
// Returns the X component of the vector for the specified direction (0,-1,0,1) for (up, left, down, right).
GS1ScriptValue fn_vecx(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function vecx requires exactly one argument");

	static double vecValues[] = { 0.0, -1.0, 0.0, 1.0 };
	auto dir = static_cast<int8_t>(visitor->getGameValueAs<double>(*arguments[0])) % 4;
	return vecValues[dir];
}

// vecy(dir)
// Returns the Y component of the vector for the specified direction (-1,0,1,0) for (up, left, down, right).
GS1ScriptValue fn_vecy(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("Built-in function vecy requires exactly one argument");

	static double vecValues[] = { -1.0, 0.0, 1.0, 0.0 };
	auto dir = static_cast<int8_t>(visitor->getGameValueAs<double>(*arguments[0])) % 4;
	return vecValues[dir];
}

// might just be a flag like gravity
GS1ScriptValue fn_waterheight(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw unimplemented_error("Built-in function waterheight not implemented");
}

// worldx(x, y)
// Converts screen (x, y) to level X.
GS1ScriptValue fn_worldx(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("Built-in function worldx is a clientside function");
}

// worldy(x, y)
// Converts screen (x, y) to level Y.
GS1ScriptValue fn_worldy(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("Built-in function worldy is a clientside function");
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
