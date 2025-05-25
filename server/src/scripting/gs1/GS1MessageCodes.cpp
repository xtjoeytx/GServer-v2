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

using PlayerOrNPC = std::optional<std::variant<PlayerPtr, NPCPtr>>;

static PlayerPtr getPlayerFromSource(const ScriptEventSource& source, std::optional<size_t> index = std::nullopt)
{
	if (source.second != ScriptEventSourceType::PLAYER)
		return nullptr;

	auto* server = BabyDI::Get<Server>();
	if (auto player = server->getPlayer(source.first); player != nullptr)
	{
		if (index.has_value())
		{
			if (auto level = server->getLevel(player->account.level); level != nullptr && index.value() < level->getPlayers().size())
				player = server->getPlayer(level->getPlayers().at(index.value()));
		}
		return player;
	}

	return nullptr;
}

static PlayerClientPtr getPlayerClientFromSource(const ScriptEventSource& source, std::optional<size_t> index = std::nullopt)
{
	auto player = getPlayerFromSource(source, index);
	if (auto client = std::dynamic_pointer_cast<PlayerClient>(player); client != nullptr)
		return client;
	return nullptr;
}

static NPCPtr getNPCFromSource(const ScriptEventSource& source, std::optional<size_t> index = std::nullopt)
{
	if (source.second != ScriptEventSourceType::NPC)
		return nullptr;
	auto* server = BabyDI::Get<Server>();
	if (auto npc = server->getNPC(source.first); npc != nullptr)
	{
		if (index.has_value())
		{
			if (auto level = npc->level.lock(); level != nullptr && index.value() < level->getNPCs().size())
				npc = server->getNPC(level->getNPCs().at(index.value()));
		}
		return npc;
	}
	return nullptr;
}

static PlayerOrNPC getPlayerOrNPCFromSource(const ScriptEventSource& source, std::optional<size_t> index = std::nullopt)
{
	if (source.second == ScriptEventSourceType::SERVER)
		return std::nullopt;

	auto* server = BabyDI::Get<Server>();
	if (source.second == ScriptEventSourceType::PLAYER)
		return getPlayerFromSource(source, index);
	else if (source.second == ScriptEventSourceType::NPC)
		return getNPCFromSource(source, index);

	return std::nullopt;
}

static Character* getCharacterFromSource(const ScriptEventSource& source, std::optional<size_t> index = std::nullopt)
{
	if (source.second == ScriptEventSourceType::SERVER)
		return nullptr;

	auto* server = BabyDI::Get<Server>();
	if (source.second == ScriptEventSourceType::PLAYER)
	{
		if (auto player = getPlayerFromSource(source, index); player != nullptr)
			return &player->account.character;
	}
	else if (source.second == ScriptEventSourceType::NPC)
	{
		if (auto npc = getNPCFromSource(source, index); npc != nullptr)
			return &npc->character;
	}

	return nullptr;
}

static ScriptVariable handleCharacterBasedMessageCode(GS1Visitor* visitor, const std::vector<ScriptVariableContainer*>& arguments, std::function<ScriptVariable(Character&, std::vector<ScriptVariableContainer*> const&)> picker)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0));

	Character* character = getCharacterFromSource(visitor->getCurrentSource(), index);
	if (character == nullptr)
		return std::string{};

	return picker(*character, arguments);
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

// #1 | #1(index)
// Sword image filename of the player.
ScriptVariable mc_1(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.swordImage;
	});
}

// #2 | #2(index)
// Shield image filename of the player.
ScriptVariable mc_2(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.shieldImage;
	});
}

// #3 | #3(index)
// Head image filename of the player.
ScriptVariable mc_3(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.headImage;
	});
}

// #4 is unused.

// #5 | #5(index)
// Horse image filename of the player.
ScriptVariable mc_5(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.horseImage;
	});
}

// #6 | #6(index)
// NPC image filename of the carried NPC.
ScriptVariable mc_6(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0));

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

// #7 | #7(index)
// Bow image filename of the player.
ScriptVariable mc_7(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.bowImage;
	});
}

// #8 | #8(index)
// Body image filename of the player.
ScriptVariable mc_8(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.bodyImage;
	});
}

// #a | #a(index)
// Account name of the player.
ScriptVariable mc_a(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0));

	if (auto player = getPlayerFromSource(visitor->getCurrentSource(), index); player != nullptr)
		return player->account.name;

	return std::string{};
}

// #b
// Line break for say2.
ScriptVariable mc_b(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	// Pass this along with processing it.
	// The client deals with it.
	return "#b"s;
}

// #c | #c(index)
// Current chat text of the player.
ScriptVariable mc_c(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.chatMessage;
	});
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
	return str.substr(startIndex, length);
}

// #F
// The level filename of the current player. (#L will return the NPC level filename)
ScriptVariable mc_F(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
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

// #f
// Image filename of the NPC.
ScriptVariable mc_f(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	auto npc = getNPCFromSource(visitor->getCurrentSource());
	if (npc != nullptr)
		return npc->image;

	return std::string{};
}

// #g | #g(index)
// Guild name of the player.
ScriptVariable mc_g(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0));

	if (auto client = getPlayerClientFromSource(visitor->getCurrentSource(), index); client != nullptr)
		return client->getGuild().toString();

	return std::string{};
}

// #G | #G(index)
// Upgrade status of the player.  (???)
// player.upgradestatus #G(index)
ScriptVariable mc_G(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #G is not implemented yet");
	return std::string{};
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
		return csvStringList[index];

	return std::string{};
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
	auto npc = getNPCFromSource(visitor->getOriginalSource());
	if (npc != nullptr)
	{
		if (auto level = npc->level.lock(); level != nullptr)
			return level->getLevelName().toString();
	}

	return std::string{};
}

// #m | #m(index)
// The animation of the player.
ScriptVariable mc_m(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.gani;
	});
}

// #n | #n(index)
// The nickname of the player.
ScriptVariable mc_n(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.nickName;
	});
}

// #N | #N(index)
// The database NPC name.
ScriptVariable mc_N(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0));

	if (auto npc = getNPCFromSource(visitor->getCurrentSource(), index); npc != nullptr)
		return npc->name;

	return std::string{};
}

// #p(index)
// The action parameter of the specified index.
ScriptVariable mc_p(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #p requires exactly 1 argument");

	auto index = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0));
	if (index < visitor->getEvent().args.size())
	{
		if (auto* arg = std::any_cast<std::string>(&visitor->getEvent().args.at(index)); arg != nullptr)
			return *arg;
	}
	return std::string{};
}

// #Q(guild_name, account_name)
// The nickname for a player in a guild.
ScriptVariable mc_Q(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	throw std::exception("Message Code #Q is not implemented yet");
	return std::string{};
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

	return arguments[index]->get();
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
	if (arguments.size() != 1)
		throw std::exception("Message Code #t requires exactly 1 argument");

	auto index = static_cast<size_t>(visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0));
	if (index >= visitor->tokens.size())
		return std::string{};

	return visitor->tokens[index];
}

// #T(string)
// Trims the tring.
ScriptVariable mc_T(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #T requires exactly 1 argument");

	auto str = visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], std::string{});
	string::trim(str);
	return str;
}

// #v(identifier)
// The value of an number variable as a string.
ScriptVariable mc_v(GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	if (arguments.size() != 1)
		throw std::exception("Message Code #s requires exactly 1 argument");

	auto number = visitor->gs1TryGetScriptVariableValueFromContainer(*arguments[0], 0.0);
	return std::format("{}", number);
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
	return handleCharacterBasedMessageCode(visitor, arguments, [&index](Character& character, const auto& arguments) -> ScriptVariable
	{
		return getGraalColorName(static_cast<GraalColors>(character.colors[index]));
	});
}

// #P1 - #P30 | #P1(index) - #P30(index)
// Gani attributes.
ScriptVariable mc_P(GS1Visitor* visitor, uint8_t index, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments)
{
	return handleCharacterBasedMessageCode(visitor, arguments, [&index](Character& character, const auto& arguments) -> ScriptVariable
	{
		return character.ganiAttributes[index];
	});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
