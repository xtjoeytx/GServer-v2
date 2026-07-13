#include <algorithm>
#include <any>
#include <array>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <tree/ParseTree.h>
#include <tree/ParseTreeType.h>
#include <tomcrypt.h>

#include <BabyDI.h>
#include <Server.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/GS1MessageCodes.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>
#include <utilities/manager/GuildManager.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

static GS1ScriptValue makeGS1ScriptValue(StoresInGameValue auto value)
{
	return GS1ScriptValue{GameValue{std::move(value)}};
}

template<typename T>
	requires std::same_as<T, GS1ScriptValue> || std::same_as<T, GameVariable*> || std::same_as<T, GameValue> || std::same_as<T, ScriptObject>
static GS1ScriptValue makeGS1ScriptValue(T&& value)
{
	return GS1ScriptValue{std::move(value)};
}

///////////////////////////////////////////////////////////////////////////////

using MessageCodeHandleFunc = GS1ScriptValue (*)(GS1Visitor*, std::string_view, const std::vector<GS1ScriptValue*>&);
using MessageCodeHandleMap = std::unordered_map<size_t, MessageCodeHandleFunc>;

static GS1ScriptValue mc_CharacterProperty(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_6(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_a(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_b(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_D(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_E(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
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
static GS1ScriptValue mc_N(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_p(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_Q(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_R(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_S(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_s(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_t(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_T(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_U(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_v(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_W(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);
static GS1ScriptValue mc_w(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);

static MessageCodeHandleMap GenerateMap()
{
	string::string_hash hash{};
	MessageCodeHandleMap map =
	{
		{hash("1"), &mc_CharacterProperty},
		{hash("2"), &mc_CharacterProperty},
		{hash("3"), &mc_CharacterProperty},
		{hash("5"), &mc_CharacterProperty},
		{hash("6"), &mc_6},
		{hash("7"), &mc_CharacterProperty},
		{hash("8"), &mc_CharacterProperty},
		{hash("a"), &mc_a},
		{hash("b"), &mc_b},
		{hash("c"), &mc_CharacterProperty},
		{hash("D"), &mc_D},
		{hash("E"), &mc_E},
		{hash("e"), &mc_e},
		{hash("F"), &mc_F},
		{hash("f"), &mc_f},
		{hash("g"), &mc_g},
		{hash("G"), &mc_G},
		{hash("I"), &mc_I},
		{hash("i"), &mc_i},
		{hash("K"), &mc_K},
		{hash("k"), &mc_k},
		{hash("L"), &mc_L},
		{hash("m"), &mc_CharacterProperty},
		{hash("n"), &mc_CharacterProperty},
		{hash("N"), &mc_N},
		{hash("p"), &mc_p},
		{hash("Q"), &mc_Q},
		{hash("R"), &mc_R},
		{hash("S"), &mc_S},
		{hash("s"), &mc_s},
		{hash("t"), &mc_t},
		{hash("T"), &mc_T},
		{hash("U"), &mc_U},
		{hash("v"), &mc_v},
		{hash("W"), &mc_W},
		{hash("w"), &mc_w},
	};
	return map;
}

/// @brief Message codes that switch to flag processing mode, which results in identifiers defaulting to client storage.
/// TODO: This might not be required anymore.
constexpr std::array<std::string_view, 8> flagProcessingMessageCodes =
{
	"I"sv,
	"s"sv,
};

constexpr std::array<std::string_view, 1> translatableMessageCodes =
{
	"U"sv,
};

///////////////////////////////////////////////////////////////////////////////

static std::any translateStringForPlayer(antlr4::tree::ParseTree* node, GS1Visitor* visitor, PlayerPtr player)
{
	if (node == nullptr)
		return std::any{};
	if (visitor == nullptr || player == nullptr || node->getTreeType() != antlr4::tree::ParseTreeType::RULE)
		return node->accept(visitor);

	return visitor->translateSourceText(node, player->account.language);
}

///////////////////////////////////////////////////////////////////////////////

GS1ScriptValue processMessageCode(GS1Visitor* visitor, antlr4::tree::ParseTree* node, std::string_view messageCode)
{
	static MessageCodeHandleMap map = GenerateMap();

	if (visitor == nullptr)
		throw std::runtime_error("processMessageCode received an empty visitor");
	if (messageCode.empty())
		throw std::runtime_error("processMessageCode received an empty message code");

	bool isTranslatable = std::ranges::contains(translatableMessageCodes, messageCode);
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
			throw std::runtime_error("Message code argument is not a valid GS1ScriptValue");

		arguments.push_back(std::move(container));
	};

	{
		// Record if we are expecting a flag.
		SetAndRestore<bool> expectingFlagGuard(visitor->expectingFlag, (std::ranges::find(flagProcessingMessageCodes, messageCode) != std::ranges::end(flagProcessingMessageCodes)));

		auto player = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER);

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
	}

	// #C0 - #C4
	// #P1 - #P30
	if (messageCode.starts_with("C") || messageCode.starts_with("P"))
	{
		return mc_CharacterProperty(visitor, messageCode, arguments);
	}
	// Any other message code in the map.
	else
	{
		size_t hash = string::string_hash{}(messageCode);
		auto it = map.find(hash);
		if (it != map.end())
			return it->second(visitor, messageCode, arguments);
	}

	// Not a known message code, so just return the string.
	return GameValue{node->getText()};
}

///////////////////////////////////////////////////////////////////////////////

// #1 | #1(index)  [Read / Write] : Sword image filename of the player.
// #2 | #2(index)  [Read / Write] : Shield image filename of the player.
// #3 | #3(index)  [Read / Write] : Head image filename of the player.
// #4 is unused.
// #5 | #5(index)  [Read / Write] : Horse image filename of the player.
// #7 | #7(index)  [Read / Write] : Bow image filename of the player.
// #8 | #8(index)  [Read / Write] : Body image filename of the player.
// #c | #c(index)  [Read / Write] : Current chat text of the player.
// #m | #m(index)  [Read / Write] : The animation of the player.
// #n | #n(index)  [Read / Write] : The nickname of the player.
// #C0 - #C4 | #C0(index) - #C4(index)  [Read / Write]
// #C0 - skin color
// #C1 - coat color
// #C2 - sleeves color
// #C3 - shoes color
// #C4 - belt color
// New World additional body colors:
// #C5 - #C7 | #C5(index) - #C7(index)  [Read / Write]
// #P1 - #P30 | #P1(index) - #P30(index)  [Read / Write] : Gani attributes.
GS1ScriptValue mc_CharacterProperty(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<int32_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = DoubleAsIntegralFloor<int32_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

	ScriptObject currentSource;

	// An index of -1 means we are looking at the source NPC.
	if (index.value_or(0) == -1)
	{
		auto activeNPC = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::NPC);
		if (activeNPC.has_value())
			currentSource = activeNPC.value();
	}
	// An index of 0 or greater means we are looking at the player.
	else if (index.has_value() && index.value() >= 0)
	{
		auto activePlayer = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER);
		if (activePlayer.has_value())
			currentSource = activePlayer.value();
	}
	// No index means we try to get the character from the current source, biasing to the initiator.
	else
	{
		currentSource = visitor->getCurrentSource(true);
		if (currentSource.second != ScriptObjectType::PLAYER && currentSource.second != ScriptObjectType::NPC)
			currentSource = visitor->getOriginalSource();
	}

	auto playerOrNPC = getPlayerOrNPCFromSource(currentSource, index);
	if (!playerOrNPC.has_value())
		return GameValue{std::string{}};

	// clang-format off
	const auto picker = visit_functions
	{
		[&](PlayerPtr& player) -> string_map<GameVariable>*
		{
			if (player == nullptr)
				return nullptr;

			player->constructScriptParameters();
			return &player->scriptParameters;
		},
		[&](NPCPtr& npc) -> string_map<GameVariable>*
		{
			if (npc == nullptr)
				return nullptr;

			npc->constructScriptParameters();
			return &npc->scriptParameters;
		}
	};
	// clang-format on

	string_map<GameVariable>* scriptParameters = std::visit(picker, playerOrNPC.value());
	if (scriptParameters == nullptr)
		return GameValue{std::string{}};

	// Found the message code.
	if (auto it = scriptParameters->find(std::format("#{}", messageCode)); it != scriptParameters->end())
		return &it->second;

	return GameValue{std::string{}};
}

// #6 | #6(index)  [Read]
// NPC image filename of the carried NPC.
GS1ScriptValue mc_6(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

	auto result = getPlayerOrNPCFromSource(visitor->getCurrentSource(), index);
	if (!result.has_value())
		return makeGS1ScriptValue(std::string{});

	// clang-format off
	const auto picker = visit_functions{
		[](PlayerPtr& player) -> std::string
		{
			if (auto client = std::dynamic_pointer_cast<PlayerClient>(player); client != nullptr && client->getCarryNPC() != 0)
			{
				auto* server = BabyDI::Get<Server>();
				if (auto npc = server->getNPC(client->getCarryNPC()); npc != nullptr)
					return string::toLower(npc->image);
			}
			return std::string{};
		},
		[](NPCPtr& npc) -> std::string
		{
			return std::string{};
		},
	};
	// clang-format on

	return makeGS1ScriptValue(std::visit(picker, result.value()));
}

// #a | #a(index)  [Read]
// Account name of the player.
GS1ScriptValue mc_a(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source->first); player != nullptr)
			return makeGS1ScriptValue(player->account.name);
	}

	return makeGS1ScriptValue(std::string{});
}

// #b
// Line break for say2.
GS1ScriptValue mc_b(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	// Pass this along with processing it.
	// The client deals with it.
	return makeGS1ScriptValue("#b"s);
}

// #D | #D(filename)
// Current file being downloaded | The download position of the specified file.
GS1ScriptValue mc_D(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: #D | #D(filename)");
}

// #E | #E(password)
// #E: The current emoticon character being displayed by the player.
// #E(password): Password to hash.
GS1ScriptValue mc_E(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::logic_error("clientside only: #E, specify a password: #E(password)");

	auto param0 = GS1Visitor::getScriptValueAs<std::string>(*arguments[0]);
	if (!param0.has_value())
		return makeGS1ScriptValue(""s);

	auto& password = param0.value().get();

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

	return makeGS1ScriptValue(std::string{output.data(), outputLength});
}

// #e(start_index, length, string)
// Extracts a substring from the given string.
GS1ScriptValue mc_e(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 3)
		throw std::invalid_argument("invalid arguments: #e(start_index, length, string)");

	auto startIndex = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
	auto length = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
	auto str = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[2]).value_or(std::string{});
	return makeGS1ScriptValue(str.substr(startIndex, length));
}

// #F  [Read]
// The level filename of the current player. (#L will return the NPC level filename)
GS1ScriptValue mc_F(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	auto result = getPlayerOrNPCFromSource(visitor->getCurrentSource(true));
	if (!result.has_value())
		return makeGS1ScriptValue(std::string{});

	// clang-format off
	const auto picker = visit_functions{
		[](PlayerPtr& player) -> std::string
		{
			if (player != nullptr)
			{
				if (auto client = std::dynamic_pointer_cast<PlayerClient>(player); client != nullptr)
					return client->getLevelName();
				return player->account.level;
			}
			return std::string{};
		},
		[](NPCPtr& npc) -> std::string
		{
			if (npc != nullptr)
			{
				if (auto level = npc->getLevel(); level != nullptr)
					return std::string{level->levelName};
				return npc->level;
			}
			return std::string{};
		},
	};
	// clang-format on

	return makeGS1ScriptValue(std::visit(picker, result.value()));
}

// #f | #f(index)  [Read]
// Image filename of the NPC.
GS1ScriptValue mc_f(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

	auto npc = getNPCFromSource(visitor->getCurrentSource(), index);
	if (npc != nullptr)
		return makeGS1ScriptValue(string::toLower(npc->image));

	return makeGS1ScriptValue(std::string{});
}

// #g | #g(index)  [Read]
// Guild name of the player.
GS1ScriptValue mc_g(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

	if (auto client = getPlayerClientFromSource(visitor->getCurrentSource(), index); client != nullptr)
		return makeGS1ScriptValue(client->getGuild().toString());

	return makeGS1ScriptValue(std::string{});
}

// #G | #G(index)  [Read]
// Upgrade status of the player (the player's account level).
// trial, classic, vip, gold
GS1ScriptValue mc_G(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

	if (auto client = getPlayerClientFromSource(visitor->getCurrentSource(), index); client != nullptr)
	{
		if (client->isGuest())
			return makeGS1ScriptValue("trial"s);

		return makeGS1ScriptValue("classic"s);
	}

	return makeGS1ScriptValue(std::string{});
}

// #I(string_list, index)
// Returns the string at the given index from the string list.
GS1ScriptValue mc_I(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: #I(string_list, index)");

	auto csvStringList = string::fromCSV(GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{}));
	auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[1]).value_or(0.0));
	if (index < csvStringList.size())
		return makeGS1ScriptValue(csvStringList[index]);

	return makeGS1ScriptValue(std::string{});
}

// #i(image) | #i(image, x, y, width, height)
// Displays an image or part of an image when used in a sign.
GS1ScriptValue mc_i(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: #i(image) | #i(image, x, y, width, height)");
}

// #K(ascii)
// The character represented by the given ASCII code.
GS1ScriptValue mc_K(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: #K(ascii)");

	uint8_t ascii = std::min(static_cast<size_t>(255), DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)));
	return makeGS1ScriptValue(std::string{static_cast<char>(ascii)});
}

// #k(key_index)
// The description of the specified key (in client language/key assignments).
GS1ScriptValue mc_k(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: #k(key_index)");
}

// #L  [Read]
// The current level filename of the NPC (use #F for the player).
GS1ScriptValue mc_L(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	auto npc = getNPCFromSource(visitor->getOriginalSource());
	if (npc != nullptr)
		return makeGS1ScriptValue(npc->getLevelName());

	return makeGS1ScriptValue(std::string{});
}

// #N | #N(index)  [Read]
// The database NPC name.
GS1ScriptValue mc_N(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	std::optional<size_t> index = std::nullopt;
	if (arguments.size() == 1)
		index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));

	if (auto npc = getNPCFromSource(visitor->getCurrentSource(), index); npc != nullptr)
		return makeGS1ScriptValue(npc->name);

	return makeGS1ScriptValue(std::string{});
}

// #p(index)  [Read]
// The action parameter of the specified index.
GS1ScriptValue mc_p(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: #p(index)");

	// The first event argument is the name of the triggeraction, so add +1 to get to the params.
	auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0)) + 1;
	if (index < visitor->getEvent().args.size())
	{
		if (auto* arg = std::any_cast<std::string>(&visitor->getEvent().args.at(index)); arg != nullptr)
			return makeGS1ScriptValue(*arg);
	}
	return makeGS1ScriptValue(std::string{});
}

// #Q(guild_name, account_name)  [Read]
// The nickname for a player in a guild.
GS1ScriptValue mc_Q(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 2)
		throw std::invalid_argument("invalid arguments: #Q(guild_name, account_name)");

	auto guildName = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{});
	auto accountName = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[1]).value_or(std::string{});

	auto guildManager = BabyDI::Get<GuildManager>();
	auto names = guildManager->getPlayerNicknamesForGuild(guildName, accountName);
	if (names.has_value())
	{
		auto& firstIter = names.value().first;
		auto& secondIter = names.value().second;
		auto nickNameRange = std::ranges::subrange(firstIter, secondIter) | std::views::values;
		return makeGS1ScriptValue(string::join(nickNameRange));
	}

	return makeGS1ScriptValue(std::string{});
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

	return makeGS1ScriptValue(GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[index]).value_or(std::string{}));
}

// #S
// The player's selected sword (Newworld).
GS1ScriptValue mc_S(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	throw std::logic_error("clientside only: #S");
}

// #s(identifier)
// The string value of a variable.
GS1ScriptValue mc_s(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: #s(identifier)");

	return makeGS1ScriptValue(GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{}));
}

// #t(index)  [Read]
// The token at the specified index as created via tokenize.
GS1ScriptValue mc_t(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: #t(index)");

	auto index = DoubleAsIntegralFloor<size_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
	if (index >= visitor->tokenizeTokens.size())
		return makeGS1ScriptValue(std::string{});

	// Explicitly place it in another string as the return will trigger move semantics.
	return makeGS1ScriptValue(visitor->tokenizeTokens[index]);
}

// #T(string)
// Trims the string.
GS1ScriptValue mc_T(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: #T(string)");

	auto str = GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{});
	string::trim(str);
	return makeGS1ScriptValue(str);
}

// #U(string)
// Replaces the string with a translated version of it.
GS1ScriptValue mc_U(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: #U(string)");

	// Translation has already happened.
	return makeGS1ScriptValue(GS1Visitor::getScriptValueAsCopy<std::string>(*arguments[0]).value_or(std::string{}));
}

// #v(identifier)
// The value of an number variable as a string.
GS1ScriptValue mc_v(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() != 1)
		throw std::invalid_argument("invalid arguments: #v(identifier)");

	auto number = GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0);
	return makeGS1ScriptValue(std::format("{}", number));
}

// #W | #W(index)  [Read]
// Image filename of a player's weapon.
GS1ScriptValue mc_W(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() == 0)
		throw std::logic_error("clientside only: #W, specify a weapon index: #W(index)");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source->first); player != nullptr)
		{
			auto& weaponList = player->account.weapons;
			if (weaponList.empty())
				return makeGS1ScriptValue(std::string{});

			int64_t index = DoubleAsIntegralFloor<int64_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			if (index >= 0 && index < (int64_t)weaponList.size())
			{
				if (auto weapon = server->getWeapon(weaponList[(size_t)index]); weapon != nullptr)
					return makeGS1ScriptValue(weapon->image);
			}
		}
	}
	return makeGS1ScriptValue(std::string{});
}

// #w | #w(index)  [Read]
// The name of the player's weapon.
GS1ScriptValue mc_w(GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments)
{
	if (arguments.size() == 0)
		throw std::logic_error("clientside only: #w, specify a weapon index: #w(index)");

	if (auto source = visitor->findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER); source.has_value())
	{
		auto* server = BabyDI::Get<Server>();
		if (auto player = server->getNPCServer()->getPlayer(source->first); player != nullptr)
		{
			auto& weaponList = player->account.weapons;
			if (weaponList.empty())
				return makeGS1ScriptValue(std::string{});

			int64_t index = DoubleAsIntegralFloor<int64_t>(GS1Visitor::getScriptValueAsCopy<double>(*arguments[0]).value_or(0.0));
			if (index >= 0 && index < (int64_t)weaponList.size())
			{
				// Explicitly place it in another string as the return will trigger move semantics.
				return makeGS1ScriptValue(weaponList[(size_t)index]);
			}
		}
	}
	return makeGS1ScriptValue(std::string{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
