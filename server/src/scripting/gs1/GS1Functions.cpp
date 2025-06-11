#include <array>
#include <algorithm>
#include <random>
#include <numbers>

#include <common.h>

#include <Server.h>
#include <object/Character.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptContainers.h>
#include <scripting/gs1/GS1Functions.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
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
		log::printLine(log::script, "processBuiltInFunction received an unknown function: {}", functionName);
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
	throw std::runtime_error("Built-in function _ not implemented");
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
	throw std::runtime_error("Built-in function base64decode not implemented");
}

// base64encode(string)
// Encodes a string to Base64 format.
GS1ScriptValue fn_base64encode(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function base64encode not implemented");
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

GS1ScriptValue fn_findnearestplayer(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function findnearestplayer not implemented");
}

GS1ScriptValue fn_findnearestplayers(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function findnearestplayers not implemented");
}

// getangle(dx, dy)
// Returns the angle in radians from the current position to the position specified by dx and dy.
GS1ScriptValue fn_getangle(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function getangle not implemented");
}

GS1ScriptValue fn_getareanpcs(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function getareanpcs not implemented");
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

GS1ScriptValue fn_getnearestplayer(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function getnearestplayer not implemented");
}

GS1ScriptValue fn_getnearestplayers(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function getnearestplayers not implemented");
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
	if (auto player = server->getPlayer(playerName, PLTYPE_ANYPLAYER); player != nullptr)
	{
		return ScriptObjectSource{ player->getId(), ScriptObjectSourceType::PLAYER };
	}

	return 0.0;
}

// getz(x, y)
// Returns the Z coordinate at the specified X and Y position in the world.
GS1ScriptValue fn_getz(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function getz not implemented");
}

GS1ScriptValue fn_groundsheight(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function groundsheight not implemented");
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
		if (auto playerObject = server->getPlayer(player.value().first); playerObject != nullptr)
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
	throw std::runtime_error("Built-in function onmapx not implemented");
}

// onmapy(level)
// The level's Y position on the map.
GS1ScriptValue fn_onmapy(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function onmapy not implemented");
}

// onwall(x, y)
// Checks if the specified X and Y coordinates are on a wall tile.
GS1ScriptValue fn_onwall(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function onwall not implemented");
}

// onwall2(x, y, width, height)
// Checks if the specified rectangle defined by X, Y, width, and height is on a wall tile.
GS1ScriptValue fn_onwall2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function onwall2 not implemented");
}

// onwater(x, y)
// Checks if the specified X and Y coordinates are on a water tile.
GS1ScriptValue fn_onwater(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function onwater not implemented");
}

GS1ScriptValue fn_playersays(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function playersays not implemented");
}

GS1ScriptValue fn_playersays2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function playersays2 not implemented");
}

// screenx(x, y)
// Converts level coordinates (x, y) to the screen's X coordinate.
GS1ScriptValue fn_screenx(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function screenx is a clientside function");
}

// screeny(x, y)
// Converts level coordinates (x, y) to the screen's Y coordinate.
GS1ScriptValue fn_screeny(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function screeny is a clientside function");
}

// testbomb(x, y)
// The index of the bomb at level position (x, y), or -1 if there is no bomb at that position.
GS1ScriptValue fn_testbomb(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function testbomb not implemented");
}

// testcompu(x, y)
// The index of the baddie at level position (x, y), or -1 if there is no baddie at that position.
GS1ScriptValue fn_testcompu(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function testcompu not implemented");
}

// testexplo(x, y)
// The index of the explosion at level position (x, y), or -1 if there is no explosion at that position.
GS1ScriptValue fn_testexplo(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function testexplo not implemented");
}

// testhorse(x, y)
// The index of the horse at level position (x, y), or -1 if there is no horse at that position.
GS1ScriptValue fn_testhorse(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function testhorse not implemented");
}

// testitem(x, y)
// The index of the item at level position (x, y), or -1 if there is no item at that position.
GS1ScriptValue fn_testitem(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function testitem not implemented");
}

// testnpc(x, y)
// The index of the NPC at level position (x, y), or -1 if there is no NPC at that position.
GS1ScriptValue fn_testnpc(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function testnpc not implemented");
}

// testplayer(x, y)
// The index of the player at level position (x, y), or -2 if there is no player at that position.
// -1 is reserved for the current npc if showcharacter is enabled.
GS1ScriptValue fn_testplayer(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function testplayer not implemented");
}

// testsign(x, y)
// The index of the sign at level position (x, y), or -1 if there is no sign at that position.
GS1ScriptValue fn_testsign(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function testsign not implemented");
}

// textheight(zoom, font, style)
// Returns the height of the text in pixels, given the zoom level, font name, and style.
GS1ScriptValue fn_textheight(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function textheight is a clientside function");
}

// textwidth(zoom, font, style, text)
// Returns the width of the text in pixels, given the zoom level, font name, style, and text.
GS1ScriptValue fn_textwidth(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function textwidth is a clientside function");
}

// tiletype(x, y)
// Returns the "new order" tile type used for setshape2 on level position (x, y).
GS1ScriptValue fn_tiletype(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function tiletype not implemented");
}

// vecx(dir)
// Returns the X component of the vector for the specified direction (0,-1,0,1) for (up, left, down, right).
GS1ScriptValue fn_vecx(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function vecx not implemented");
}

// vecy(dir)
// Returns the Y component of the vector for the specified direction (-1,0,1,0) for (up, left, down, right).
GS1ScriptValue fn_vecy(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function vecy not implemented");
}

// might just be a flag like gravity
GS1ScriptValue fn_waterheight(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function waterheight not implemented");
}

// worldx(x, y)
// Converts screen (x, y) to level X.
GS1ScriptValue fn_worldx(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function worldx is a clientside function");
}

// worldy(x, y)
// Converts screen (x, y) to level Y.
GS1ScriptValue fn_worldy(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::runtime_error("Built-in function worldy is a clientside function");
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
