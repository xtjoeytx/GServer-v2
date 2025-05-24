#include <random>

#include <common.h>

#include <scripting/gs1/GS1MessageCodes.h>
#include <scripting/gs1/GS1Visitor.h>
#include <utilities/StringUtils.h>

using namespace preagonal::grammar::gs1;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

using MessageCodeHandleFunc = ScriptVariable(*)(GS1Visitor*, std::string_view, const std::vector<ScriptVariableContainer*>&);
using MessageCodeHandleMap = std::unordered_map<size_t, MessageCodeHandleFunc>;

static ScriptVariable mc_1(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_3(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_5(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_6(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_7(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_8(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_a(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_b(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_c(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_D(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_e(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_F(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_f(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_g(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_G(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_I(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_i(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_K(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_k(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_L(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_m(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_n(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_N(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_p(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_Q(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_R(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_s(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_t(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_T(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_v(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_W(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_w(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);

static ScriptVariable mc_C(GS1Visitor* visitor, uint8_t index, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);
static ScriptVariable mc_P(GS1Visitor* visitor, uint8_t index, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);

static MessageCodeHandleMap GenerateMap()
{
	string::string_hash hash{};
	MessageCodeHandleMap map =
	{
		{ hash("1"), &mc_1 },
		{ hash("2"), &mc_2 },
		{ hash("3"), &mc_3 },
		{ hash("5"), &mc_5 },
		{ hash("6"), &mc_6 },
		{ hash("7"), &mc_7 },
		{ hash("8"), &mc_8 },
		{ hash("a"), &mc_a },
		{ hash("b"), &mc_b },
		{ hash("c"), &mc_c },
		{ hash("D"), &mc_D },
		{ hash("e"), &mc_e },
		{ hash("F"), &mc_F },
		{ hash("f"), &mc_f },
		{ hash("g"), &mc_g },
		{ hash("G"), &mc_G },
		{ hash("I"), &mc_I },
		{ hash("i"), &mc_i },
		{ hash("K"), &mc_K },
		{ hash("k"), &mc_k },
		{ hash("L"), &mc_L },
		{ hash("m"), &mc_m },
		{ hash("n"), &mc_n },
		{ hash("N"), &mc_N },
		{ hash("p"), &mc_p },
		{ hash("Q"), &mc_Q },
		{ hash("R"), &mc_R },
		{ hash("s"), &mc_s },
		{ hash("t"), &mc_t },
		{ hash("T"), &mc_T },
		{ hash("v"), &mc_v },
		{ hash("W"), &mc_W },
		{ hash("w"), &mc_w },
	};
	return map;
}

///////////////////////////////////////////////////////////////////////////////

ScriptVariable processMessageCode(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	static MessageCodeHandleMap map = GenerateMap();

	if (visitor == nullptr)
		throw std::exception("processMessageCode received an empty visitor");
	if (messageCode.empty())
		throw std::exception("processMessageCode received an empty message code");

	// #C0 - #C4
	if (messageCode.starts_with("C"))
	{
		if (messageCode.size() == 2)
		{
			uint8_t index = static_cast<uint8_t>(messageCode[1] - '0');
			if (index >= 0 && index <= 4)
				return mc_C(visitor, index, messageCode, arguments);
		}
	}
	// #P1 - #P30
	else if (messageCode.starts_with("P"))
	{
		std::string_view indexStr = messageCode.substr(1);
		auto index = std::stoi(std::string(indexStr));
		if (index >= 1 && index <= 30)
			return mc_P(visitor, static_cast<uint8_t>(index), messageCode, arguments);
	}
	// Any other message code in the map.
	else
	{
		size_t hash = string::string_hash{}(messageCode);
		auto it = map.find(hash);
		if (it != map.end())
			return it->second(visitor, messageCode, arguments);
	}

	throw std::exception("processMessageCode received an unknown message code");
}

///////////////////////////////////////////////////////////////////////////////

// #1 | #1(index)
// Sword image filename of the player.
ScriptVariable mc_1(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #1 is not implemented yet");
	return ScriptVariable(std::string{});
}

// #2 | #2(index)
// Shield image filename of the player.
ScriptVariable mc_2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #2 is not implemented yet");
	return ScriptVariable(std::string{});
}

// #3 | #3(index)
// Head image filename of the player.
ScriptVariable mc_3(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #3 is not implemented yet");
	return ScriptVariable(std::string{});
}

// #4 is unused.

// #5 | #5(index)
// Horse image filename of the player.
ScriptVariable mc_5(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #5 is not implemented yet");
	return ScriptVariable(std::string{});
}

// #6 | #6(index)
// NPC image filename of the carried NPC.
ScriptVariable mc_6(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #6 is not implemented yet");
	return ScriptVariable(std::string{});
}

// #7 | #7(index)
// Bow image filename of the player.
ScriptVariable mc_7(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #7 is not implemented yet");
	return ScriptVariable(std::string{});
}

// #8 | #8(index)
// Body image filename of the player.
ScriptVariable mc_8(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #8 is not implemented yet");
	return ScriptVariable(std::string{});
}

// #a | #a(index)
// Account name of the player.
ScriptVariable mc_a(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #a is not implemented yet");
	return ScriptVariable(std::string{});
}

// #b
// Line break for say2.
ScriptVariable mc_b(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	// Pass this along with processing it.
	// The client deals with it.
	return ScriptVariable("#b");
}

// #c | #c(index)
// Current chat text of the player.
ScriptVariable mc_c(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #c is not implemented yet");
	return ScriptVariable(std::string{});
}

// #D | #D(filename)
// Current file being downloaded | The download position of the specified file.
ScriptVariable mc_D(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #D is registered as a clientside message code");
}

// #e(start_index, length, string)
// Extracts a substring from the given string.
ScriptVariable mc_e(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	if (arguments.size() != 3)
		throw std::exception("Message Code #e requires exactly 3 arguments");

	auto startIndex = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0));
	auto length = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[1], 0.0));
	auto str = visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[2], std::string{});
	return ScriptVariable(str.substr(startIndex, length));
}

// #F
// The level filename of the current player. (#L will return the NPC level filename)
ScriptVariable mc_F(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #F is not implemented yet");
	return ScriptVariable(std::string{});
}

// #f
// Image filename of the NPC.
ScriptVariable mc_f(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #f is not implemented yet");
	return ScriptVariable(std::string{});
}

// #g | #g(index)
// Guild name of the player.
ScriptVariable mc_g(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #g is not implemented yet");
	return ScriptVariable(std::string{});
}

// #G
// Update stats of the player.  (???)
ScriptVariable mc_G(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #G is not implemented yet");
	return ScriptVariable(std::string{});
}

// #I(string_list, index)
// Returns the string at the given index from the string list.
ScriptVariable mc_I(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	if (arguments.size() != 2)
		throw std::exception("Message Code #I requires exactly 2 arguments");

	auto csvStringList = string::fromCSV(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], std::string{}));
	auto index = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[1], 0.0));
	if (index < csvStringList.size())
		return ScriptVariable(csvStringList[index]);

	return ScriptVariable(std::string{});
}

// #i(image) | #i(image, x, y, width, height)
// Displays an image or part of an image when used in a sign.
ScriptVariable mc_i(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #i is registered as a clientside message code");
}

// #K(key_index)
// The name of the specified key.
ScriptVariable mc_K(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #K is registered as a clientside message code");
}

// #k(key_index)
// The description of the specified key (in client language/key assignments).
ScriptVariable mc_k(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #k is registered as a clientside message code");
}

// #L
// The current level filename of the NPC (use #F for the player).
ScriptVariable mc_L(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #L is not implemented yet");
	return ScriptVariable(std::string{});
}

// #m | #m(index)
// The animation of the player.
ScriptVariable mc_m(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #m is not implemented yet");
	return ScriptVariable(std::string{});
}

// #n | #n(index)
// The nickname of the player.
ScriptVariable mc_n(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #n is not implemented yet");
	return ScriptVariable(std::string{});
}

// #N | #N(index)
// The database NPC name.
ScriptVariable mc_N(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #N is not implemented yet");
	return ScriptVariable(std::string{});
}

// #p(index)
// The action parameter of the specified index.
ScriptVariable mc_p(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #p is not implemented yet");
	return ScriptVariable(std::string{});
}

// #Q(guild_name, account_name)
// The nickname for a player in a guild.
ScriptVariable mc_Q(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #Q is not implemented yet");
	return ScriptVariable(std::string{});
}

// #R(string_list)
// Randomly selects a string from the given string list.
ScriptVariable mc_R(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	using namespace std::chrono;
	auto seed = static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
	std::minstd_rand rng(seed);
	std::uniform_int_distribution<size_t> dist(0, arguments.size() - 1);
	size_t index = dist(rng);

	return ScriptVariable(arguments[index]->get());
}

// #s(identifier)
// The string value of a variable.
ScriptVariable mc_s(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #s requires exactly 1 argument");

	return visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], std::string{});
}

// #t(index)
// The token at the specified index as created via tokenize.
ScriptVariable mc_t(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #t is not implemented yet");
	return ScriptVariable(std::string{});
}

// #T(string)
// Trims the tring.
ScriptVariable mc_T(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #T requires exactly 1 argument");

	auto str = visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], std::string{});
	string::trim(str);
	return ScriptVariable(str);
}

// #v(identifier)
// The value of an number variable as a string.
ScriptVariable mc_v(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #s requires exactly 1 argument");

	auto number = visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0);
	return ScriptVariable(std::format("{}", number));
}

// #W | #W(index)
// Image filename of a player's weapon.
ScriptVariable mc_W(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	// TODO(Nalin): #W might not be possible serverside, but #W(index) should be possible.
	throw std::exception("Message Code #W is registered as a clientside message code");
}

// #w | #w(index)
// The name of the player's weapon.
ScriptVariable mc_w(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	// TODO(Nalin): #w might not be possible serverside, but #w(index) should be possible.
	throw std::exception("Message Code #w is registered as a clientside message code");
}

//----------------------------

// #C0 - #C4 | #C0(index) - #C4(index)
// #C0 - skin color
// #C1 - coat color
// #C2 - sleeves color
// #C3 - shoes color
// #C4 - belt color
ScriptVariable mc_C(GS1Visitor* visitor, uint8_t index, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #C0-#C4 is not implemented yet");
	return ScriptVariable(std::string{});
}

// #P1 - #P30 | #P1(index) - #P30(index)
// Gani attributes.
ScriptVariable mc_P(GS1Visitor* visitor, uint8_t index, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #P1-#P30 is not implemented yet");
	return ScriptVariable(std::string{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
