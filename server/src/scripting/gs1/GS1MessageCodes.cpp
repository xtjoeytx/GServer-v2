#include <random>

#include <common.h>

#include <Server.h>
#include <object/Character.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <scripting/gs1/GS1MessageCodes.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/StringUtils.h>

using namespace preagonal::grammar::gs1;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

using MessageCodeHandleFunc = GS1ScriptValue(*)(GS1Visitor*, std::string_view, const std::vector<GS1ScriptValue*>&);
using MessageCodeHandleMap = std::unordered_map<size_t, MessageCodeHandleFunc>;

static GS1ScriptValue mc_1(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_3(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_5(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_6(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_7(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_8(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_a(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_b(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_c(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_D(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_e(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_F(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_f(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_g(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_G(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_I(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_i(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_K(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_k(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_L(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_m(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_n(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_N(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_p(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_Q(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_R(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_s(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_t(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_T(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_v(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_W(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_w(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);

static GS1ScriptValue mc_C(GS1Visitor* visitor, uint8_t index, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_P(GS1Visitor* visitor, uint8_t index, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);

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

static GS1ScriptValue handleCharacterBasedMessageCode(GS1Visitor* visitor, const std::vector<GS1ScriptValue*>& arguments, std::function<GS1ScriptValue(Character&, std::vector<GS1ScriptValue*> const&)> picker)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));

	Character* character = getCharacterFromSource(visitor->getCurrentSource(), index);
	if (character == nullptr)
		return std::string{};

	return picker(*character, arguments);
}

///////////////////////////////////////////////////////////////////////////////

GS1ScriptValue processMessageCode(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
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
		auto index = string::toNumber<uint8_t>(std::string(indexStr));
		if (index >= 1 && index <= 30)
			return mc_P(visitor, index, messageCode, arguments);
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

// #1 | #1(index)  [Read / Write]
// Sword image filename of the player.
GS1ScriptValue mc_1(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.swordImage;
	});
}

// #2 | #2(index)  [Read / Write]
// Shield image filename of the player.
GS1ScriptValue mc_2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.shieldImage;
	});
}

// #3 | #3(index)  [Read / Write]
// Head image filename of the player.
GS1ScriptValue mc_3(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.headImage;
	});
}

// #4 is unused.

// #5 | #5(index)  [Read / Write]
// Horse image filename of the player.
GS1ScriptValue mc_5(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.horseImage;
	});
}

// #6 | #6(index)  [Read]
// NPC image filename of the carried NPC.
GS1ScriptValue mc_6(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));

	auto result = getPlayerOrNPCFromSource(visitor->getCurrentSource(), index);
	if (!result.has_value())
		return std::string{};

	const auto picker = visit_functions
	{
		[](PlayerPtr& player) -> std::string
		{
			if (auto client = std::dynamic_pointer_cast<PlayerClient>(player); client != nullptr && client->getCarryNpcId() != 0)
			{
				auto* server = BabyDI::Get<Server>();
				if (auto npc = server->getNPC(client->getCarryNpcId()); npc != nullptr)
					return npc->image;
			}
			return std::string{};
		},
		[](NPCPtr& npc) -> std::string { return std::string{}; },
	};

	return std::visit(picker, result.value());
}

// #7 | #7(index)  [Read / Write]
// Bow image filename of the player.
GS1ScriptValue mc_7(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.bowImage;
	});
}

// #8 | #8(index)  [Read / Write]
// Body image filename of the player.
GS1ScriptValue mc_8(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.bodyImage;
	});
}

// #a | #a(index)  [Read]
// Account name of the player.
GS1ScriptValue mc_a(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));

	if (auto player = getPlayerFromSource(visitor->getCurrentSource(), index); player != nullptr)
		return player->account.name;

	return std::string{};
}

// #b
// Line break for say2.
GS1ScriptValue mc_b(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	// Pass this along with processing it.
	// The client deals with it.
	return "#b"s;
}

// #c | #c(index)  [Read / Write]
// Current chat text of the player.
GS1ScriptValue mc_c(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.chatMessage;
	});
}

// #D | #D(filename)
// Current file being downloaded | The download position of the specified file.
GS1ScriptValue mc_D(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("Message Code #D is registered as a clientside message code");
}

// #e(start_index, length, string)
// Extracts a substring from the given string.
GS1ScriptValue mc_e(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::exception("Message Code #e requires exactly 3 arguments");

	auto startIndex = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
	auto length = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[1]));
	auto str = visitor->getGameValueAs<std::string>(*arguments[2]);
	return str.substr(startIndex, length);
}

// #F  [Read]
// The level filename of the current player. (#L will return the NPC level filename)
GS1ScriptValue mc_F(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	auto result = getPlayerOrNPCFromSource(visitor->getCurrentSource());
	if (!result.has_value())
		return std::string{};

	const auto picker = visit_functions
	{
		[](PlayerPtr& player) -> std::string { return player->account.level; },
		[](NPCPtr& npc) -> std::string
		{
			if (auto level = npc->level.lock(); level != nullptr)
				return level->getLevelName().toString();
			return std::string{};
		},
	};

	return std::visit(picker, result.value());
}

// #f  [Read]
// Image filename of the NPC.
GS1ScriptValue mc_f(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	auto npc = getNPCFromSource(visitor->getCurrentSource());
	if (npc != nullptr)
		return npc->image;

	return std::string{};
}

// #g | #g(index)  [Read]
// Guild name of the player.
GS1ScriptValue mc_g(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));

	if (auto client = getPlayerClientFromSource(visitor->getCurrentSource(), index); client != nullptr)
		return client->getGuild().toString();

	return std::string{};
}

// #G | #G(index)  [Read]
// Upgrade status of the player.  (???)
// player.upgradestatus #G(index)
GS1ScriptValue mc_G(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("Message Code #G is not implemented yet");
	return std::string{};
}

// #I(string_list, index)
// Returns the string at the given index from the string list.
GS1ScriptValue mc_I(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::exception("Message Code #I requires exactly 2 arguments");

	auto csvStringList = string::fromCSV(visitor->getGameValueAs<std::string>(*arguments[0]));
	auto index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[1]));
	if (index < csvStringList.size())
		return csvStringList[index];

	return std::string{};
}

// #i(image) | #i(image, x, y, width, height)
// Displays an image or part of an image when used in a sign.
GS1ScriptValue mc_i(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("Message Code #i is registered as a clientside message code");
}

// #K(key_index)
// The name of the specified key.
GS1ScriptValue mc_K(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("Message Code #K is registered as a clientside message code");
}

// #k(key_index)
// The description of the specified key (in client language/key assignments).
GS1ScriptValue mc_k(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("Message Code #k is registered as a clientside message code");
}

// #L  [Read]
// The current level filename of the NPC (use #F for the player).
GS1ScriptValue mc_L(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	auto npc = getNPCFromSource(visitor->getOriginalSource());
	if (npc != nullptr)
	{
		if (auto level = npc->level.lock(); level != nullptr)
			return level->getLevelName().toString();
	}

	return std::string{};
}

// #m | #m(index)  [Read / Write]
// The animation of the player.
GS1ScriptValue mc_m(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.gani;
	});
}

// #n | #n(index)  [Read / Write]
// The nickname of the player.
GS1ScriptValue mc_n(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.nickName;
	});
}

// #N | #N(index)  [Read]
// The database NPC name.
GS1ScriptValue mc_N(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));

	if (auto npc = getNPCFromSource(visitor->getCurrentSource(), index); npc != nullptr)
		return npc->name;

	return std::string{};
}

// #p(index)  [Read / Write]
// The action parameter of the specified index.
GS1ScriptValue mc_p(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #p requires exactly 1 argument");

	auto index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
	if (index < visitor->getEvent().args.size())
	{
		if (auto* arg = std::any_cast<std::string>(&visitor->getEvent().args.at(index)); arg != nullptr)
			return *arg;
	}
	return std::string{};
}

// #Q(guild_name, account_name)  [Read]
// The nickname for a player in a guild.
GS1ScriptValue mc_Q(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::exception("Message Code #Q is not implemented yet");
	return std::string{};
}

// #R(string_list)
// Randomly selects a string from the given string list.
GS1ScriptValue mc_R(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	using namespace std::chrono;
	auto seed = static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
	std::minstd_rand rng(seed);
	std::uniform_int_distribution<size_t> dist(0, arguments.size() - 1);
	size_t index = dist(rng);

	return visitor->getGameValueAs<std::string>(*arguments[index]);
}

// #s(identifier)
// The string value of a variable.
GS1ScriptValue mc_s(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #s requires exactly 1 argument");

	return visitor->getGameValueAs<std::string>(*arguments[0]);
}

// #t(index)  [Read / Write]
// The token at the specified index as created via tokenize.
GS1ScriptValue mc_t(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #t requires exactly 1 argument");

	auto index = static_cast<size_t>(visitor->getGameValueAs<double>(*arguments[0]));
	if (index >= visitor->tokenizeTokens.size())
		return std::string{};

	return visitor->tokenizeTokens[index];
}

// #T(string)
// Trims the tring.
GS1ScriptValue mc_T(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #T requires exactly 1 argument");

	auto str = visitor->getGameValueAs<std::string>(*arguments[0]);
	string::trim(str);
	return str;
}

// #v(identifier)
// The value of an number variable as a string.
GS1ScriptValue mc_v(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #s requires exactly 1 argument");

	auto number = visitor->getGameValueAs<double>(*arguments[0]);
	return std::format("{}", number);
}

// #W | #W(index)  [Read]
// Image filename of a player's weapon.
GS1ScriptValue mc_W(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	// TODO(Nalin): #W might not be possible serverside, but #W(index) should be possible.
	throw std::exception("Message Code #W is registered as a clientside message code");
}

// #w | #w(index)  [Read]
// The name of the player's weapon.
GS1ScriptValue mc_w(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	// TODO(Nalin): #w might not be possible serverside, but #w(index) should be possible.
	throw std::exception("Message Code #w is registered as a clientside message code");
}

//----------------------------

// #C0 - #C4 | #C0(index) - #C4(index)  [Read / Write]
// #C0 - skin color
// #C1 - coat color
// #C2 - sleeves color
// #C3 - shoes color
// #C4 - belt color
GS1ScriptValue mc_C(GS1Visitor* visitor, uint8_t index, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [&index](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return getCharacterColorName(static_cast<CharacterColors>(character.colors[index]));
	});
}

// #P1 - #P30 | #P1(index) - #P30(index)  [Read / Write]
// Gani attributes.
GS1ScriptValue mc_P(GS1Visitor* visitor, uint8_t index, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [&index](Character& character, const auto& arguments) -> GS1ScriptValue
	{
		return character.ganiAttributes[index];
	});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
