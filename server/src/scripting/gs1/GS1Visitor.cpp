#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <exception>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <ANTLRInputStream.h>
#include <CommonTokenStream.h>
#include <GS1Lexer.h>
#include <GS1Parser.h>
#include <tree/ErrorNode.h>
#include <tree/ParseTree.h>
#include <tree/ParseTreeType.h>
#include <tree/TerminalNode.h>

#include <BabyDI.h>
#include <Server.h>
#include <level/Level.h>
#include <level/LevelBaddy.h>
#include <level/LevelItem.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <scripting/Script.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/GS1Commands.h>
#include <scripting/gs1/GS1ErrorListener.h>
#include <scripting/gs1/GS1Functions.h>
#include <scripting/gs1/GS1MessageCodes.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>
#include <utilities/manager/GuildManager.h>
#include <utilities/manager/ITranslationManager.h>
#include <utilities/std/generator.h>

#ifdef DEBUG
	#define RECOVERABLE_PARSE_ERROR(MESSAGE, RETVAL) throw std::runtime_error(std::format("GS1 Parse Error: {}", MESSAGE))
#else
	#define RECOVERABLE_PARSE_ERROR(MESSAGE, RETVAL) \
		do                                           \
		{                                            \
			reportError(MESSAGE, context, false);    \
			return RETVAL;                           \
		}                                            \
		while (false)
#endif

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Constructor.

GS1Visitor::GS1Visitor()
	: server(BabyDI::Get<Server>()), translationManager(BabyDI::Get<ITranslationManager>()), guildManager(BabyDI::Get<GuildManager>())
{
	const auto npc = server->getNPCServer();
	npcServer = npc.get();

	const auto gs1 = std::dynamic_pointer_cast<ScriptEngineGS1>(npc->scripting.getScriptEngine(ScriptEngineGS1::EngineName));
	scriptEngine = gs1.get();
}

///////////////////////////////////////////////////////////////////////////////
// File static functions.

static std::any makeGS1ScriptValue(StoresInGameValue auto value)
{
	return std::make_any<GS1ScriptValue>(GameValue{std::move(value)});
}

template<typename T>
	requires std::same_as<T, GS1ScriptValue> || std::same_as<T, GameVariable*> || std::same_as<T, GameVariable> || std::same_as<T, GameValue> || std::same_as<T, ScriptObject>
static std::any makeGS1ScriptValue(T&& value)
{
	return std::make_any<GS1ScriptValue>(std::forward<T>(value));
}

static std::optional<size_t> getSymbolType(antlr4::tree::ParseTree* tree)
{
	if (tree == nullptr) return std::nullopt;

	// We might be looking for the direct child.
	if (tree->children.size() == 1)
		tree = tree->children[0];

	// Find the symbol type if this is a TerminalNode.
	if (const auto* node = dynamic_cast<antlr4::tree::TerminalNode*>(tree); node != nullptr)
		return node->getSymbol()->getType();

	return std::nullopt;
}

static std::generator<Script*> getJoinedClassesFromSource(const ScriptObject source)
{
	auto* server = BabyDI::Get<Server>();
	switch (source.second)
	{
		case ScriptObjectType::NPC:
			if (const auto npc = server->getNPC(source.first); npc != nullptr)
			{
				for (const ScriptClassPtr& scriptClass : npc->getJoinedClasses())
					co_yield &scriptClass->getScript();
			}
			break;

		case ScriptObjectType::WEAPON:
		{
			auto& weaponList = server->getWeaponList();
			if (const auto it = weaponList.find(source.first); it != weaponList.end())
			{
				for (const ScriptClassPtr& scriptClass : it->second->getJoinedClasses())
					co_yield &scriptClass->getScript();
			}
		}

		default:;
	}
}

static GameVariableStore* getGameVariableStoreFromSource(const ScriptObject& source)
{
	static GameVariableStore invalidStore;

	auto* server = BabyDI::Get<Server>();
	switch (source.second)
	{
		case ScriptObjectType::PLAYER:
			if (const auto player = server->getNPCServer()->getPlayer(source.first); player != nullptr)
				return &player->account.variables;
			break;
		case ScriptObjectType::NPC:
			if (const auto npc = server->getNPC(source.first); npc != nullptr)
				return &npc->scripting.variables;
			break;
		case ScriptObjectType::WEAPON:
		{
			auto& weaponList = server->getWeaponList();
			if (const auto it = weaponList.find(source.first); it != weaponList.end())
				return &it->second->scripting.variables;
			log::printLine(log::script, "Could not find weapon source.");
			return &invalidStore;
		}
		case ScriptObjectType::LEVEL:
		{
			auto& levelList = server->getLevelList();
			if (const auto it = levelList.find(source.first); it != levelList.end())
				return &it->second->scripting.variables;
			log::printLine(log::script, "Could not find level source.");
			return &invalidStore;
		}
		case ScriptObjectType::SERVER:
			return &server->Scripting.variables;
		default:;
	}
	return nullptr;
}

static GS1ScriptValue getGS1ScriptValueFromAny(std::any& value)
{
	if (auto* gs1ScriptValue = std::any_cast<GS1ScriptValue>(&value); gs1ScriptValue != nullptr)
		return *gs1ScriptValue;
	return {};
}

///////////////////////////////////////////////////////////////////////////////
// Static member functions.

double GS1Visitor::getColorValueFromString(const std::string_view colorString)
{
	auto it = std::ranges::find(colorNames, colorString);
	if (it == colorNames.end())
		it = colorNames.begin();

	return static_cast<double>(std::distance(colorNames.begin(), it));
}

GameVariable* GS1Visitor::getGameVariable(std::any& value)
{
	if (const auto gs1ScriptValue = std::any_cast<GS1ScriptValue>(&value); gs1ScriptValue != nullptr)
		return getGameVariable(*gs1ScriptValue);
	if (const auto gameVariable = std::any_cast<GameVariable*>(&value); gameVariable != nullptr)
		return *gameVariable;
	if (const auto gameVariable = std::any_cast<GameVariable>(&value); gameVariable != nullptr)
		return gameVariable;
	return nullptr;
}

GameVariable* GS1Visitor::getGameVariable(GS1ScriptValue& value)
{
	if (const auto gameVariable = std::get_if<GameVariable*>(&value); gameVariable != nullptr)
		return *gameVariable;
	if (const auto gameVariable = std::get_if<GameVariable>(&value); gameVariable != nullptr)
		return gameVariable;
	return nullptr;
}

const GameVariable* GS1Visitor::getGameVariable(const std::any& value)
{
	if (const auto gs1ScriptValue = std::any_cast<GS1ScriptValue>(&value); gs1ScriptValue != nullptr)
		return getGameVariable(*gs1ScriptValue);
	if (const auto gameVariable = std::any_cast<GameVariable*>(&value); gameVariable != nullptr)
		return *gameVariable;
	if (const auto gameVariable = std::any_cast<GameVariable>(&value); gameVariable != nullptr)
		return gameVariable;
	return nullptr;
}

const GameVariable* GS1Visitor::getGameVariable(const GS1ScriptValue& value)
{
	if (const auto gameVariable = std::get_if<GameVariable*>(&value); gameVariable != nullptr)
		return *gameVariable;
	if (const auto gameVariable = std::get_if<GameVariable>(&value); gameVariable != nullptr)
		return gameVariable;
	return nullptr;
}

std::optional<ScriptObject> GS1Visitor::getScriptObject(std::any& value)
{
	if (const auto scriptObject = std::any_cast<ScriptObject>(&value); scriptObject != nullptr)
		return *scriptObject;
	return getScriptValueAsCopy<ScriptObject>(value);
}

std::optional<ScriptObject> GS1Visitor::getScriptObject(GS1ScriptValue& value)
{
	if (const auto scriptObject = std::get_if<ScriptObject>(&value); scriptObject != nullptr)
		return *scriptObject;
	return getScriptValueAsCopy<ScriptObject>(value);
}

std::optional<ScriptObject> GS1Visitor::getScriptObject(const GameVariable& value)
{
	return value.getCopy<ScriptObject>();
}

bool GS1Visitor::isGameValue(const GS1ScriptValue& value)
{
	return std::holds_alternative<GameValue>(value) || std::holds_alternative<GameVariable*>(value) || std::holds_alternative<GameVariable>(value);
}

bool GS1Visitor::isScriptObject(const GS1ScriptValue& value)
{
	return std::holds_alternative<ScriptObject>(value);
}

///////////////////////////////////////////////////////////////////////////////
// Public member functions.

std::optional<ScriptObject> GS1Visitor::findNearestScriptObjectSourceFromStack(const ScriptObjectType type) const
{
	for (const auto& source : sourceStack())
	{
		if (source.second == type)
			return source;
	}
	return std::nullopt;
}

GameVariableStore* GS1Visitor::findGameVariableStoreFromStack(const ScriptObjectType type, int skip) const
{
	std::optional<ScriptObject> foundSource;

	for (const auto& source : sourceStack())
	{
		if (source.second == type)
		{
			if (skip <= 0)
				return getGameVariableStoreFromSource(source);

			foundSource = source;
			--skip;
		}
	}

	if (foundSource.has_value())
		return getGameVariableStoreFromSource(foundSource.value());

	return nullptr;
}

GameVariableStore* GS1Visitor::getGameVariableStoreForStorageType(size_t type) const
{
	GameVariableStore* store = nullptr;
	int skip = 0;
	if (inList(type, ENUM(StorageType::THISO), ENUM(StorageType::CLIENTO), ENUM(StorageType::CLIENTRO)))
		skip = 1;

	switch (type)
	{
		case ENUM(StorageType::THIS):
		case ENUM(StorageType::THISO):
			store = findGameVariableStoreFromStack(ScriptObjectType::NPC, skip);
			if (store == nullptr)
				store = findGameVariableStoreFromStack(ScriptObjectType::WEAPON, skip);
			break;
		case ENUM(StorageType::CLIENT):
		case ENUM(StorageType::CLIENTR):
		case ENUM(StorageType::CLIENTO):
		case ENUM(StorageType::CLIENTRO):
			store = findGameVariableStoreFromStack(ScriptObjectType::PLAYER, skip);
			break;
		case ENUM(StorageType::SERVER):
		case ENUM(StorageType::SERVERR):
			store = m_serverStore;
			break;
		case ENUM(StorageType::LEVEL):
		{
			auto pair = getPlayerOrNPCFromSource(m_originalSource);
			if (!pair.has_value())
				return nullptr;

			// clang-format off
			const auto picker = visit_functions{
				[this](const PlayerPtr& player) -> LevelPtr
				{
					return server->getLoadedLevel(player->account.level, player);
				},
				[](const NPCPtr& npc) -> LevelPtr
				{
					return npc->getLevel();
				}
			};
			// clang-format on

			const auto level = std::visit(picker, pair.value());
			return &level->scripting.variables;
		}
		case ENUM(StorageType::LOCAL):
		case ENUM(StorageType::TEMP):
			return builtInStore;
		default:;
	}

	return store;
}

// --

std::shared_ptr<Level> GS1Visitor::findCurrentLevel() const
{
	auto testSource = [this](const ScriptObject& source) -> std::shared_ptr<Level>
	{
		if (source.second == ScriptObjectType::NPC)
		{
			if (const auto npc = server->getNPC(source.first); npc != nullptr)
				return npc->getLevel();
		}
		else if (source.second == ScriptObjectType::PLAYER)
		{
			if (const auto player = server->getNPCServer()->getPlayer(source.first); player != nullptr)
				return server->getLoadedLevel(player->account.level, player);
		}
		else if (source.second == ScriptObjectType::LEVEL)
		{
			auto& levelList = server->getLevelList();
			if (const auto level = levelList.find(source.first); level != levelList.end())
				return level->second;
		}
		return nullptr;
	};

	for (const auto& source : sourceStack())
	{
		if (auto level = testSource(source); level != nullptr)
			return level;
	}
	return nullptr;
}

std::tuple<std::shared_ptr<Level>, std::shared_ptr<SubLevel>, std::shared_ptr<StaticLevelData>> GS1Visitor::findCurrentLevelData() const
{
	auto testSource = [this](const ScriptObject& source) -> std::tuple<std::shared_ptr<Level>, std::shared_ptr<SubLevel>, std::shared_ptr<StaticLevelData>>
	{
		if (source.second == ScriptObjectType::NPC)
		{
			if (const auto npc = server->getNPC(source.first); npc != nullptr)
			{
				if (auto level = npc->getLevel(); level != nullptr)
				{
					auto [subLevel, levelData] = level->getSubLevelAndStaticDataAtPosition(npc->character.getMapPosition());
					return std::make_tuple(level, subLevel, levelData);
				}
			}
		}
		else if (source.second == ScriptObjectType::PLAYER)
		{
			if (const auto player = server->getNPCServer()->getPlayer(source.first); player != nullptr)
			{
				if (auto level = server->getLoadedLevel(player->account.level, player); level != nullptr)
				{
					auto [subLevel, levelData] = level->getSubLevelAndStaticDataAtPosition(player->getMapPosition());
					return std::make_tuple(level, subLevel, levelData);
				}
			}
		}
		else if (source.second == ScriptObjectType::LEVEL)
		{
			auto& levelList = server->getLevelList();
			if (const auto level = levelList.find(source.first); level != levelList.end())
				return std::make_tuple(level->second, nullptr, nullptr);
		}
		return std::make_tuple(nullptr, nullptr, nullptr);
	};

	for (const auto& source : sourceStack())
	{
		if (auto level = testSource(source); std::get<0>(level) != nullptr)
			return level;
	}
	return std::make_tuple(nullptr, nullptr, nullptr);
}

//--

GameVariable* GS1Visitor::getGameVariableFromSource(const ScriptObject& source, const std::string_view identifier) const
{
	switch (source.second)
	{
		case ScriptObjectType::NPC:
			if (const auto npc = server->getNPC(source.first); npc != nullptr)
				return getScriptParameter(*npc, identifier);
			break;
		case ScriptObjectType::PLAYER:
			if (const auto player = server->getNPCServer()->getPlayer(source.first); player != nullptr)
				return getScriptParameter(*player, identifier);
			break;
		default:;
	}

	const auto level = findCurrentLevel();
	if (level == nullptr)
		return nullptr;

	switch (source.second)
	{
		case ScriptObjectType::BADDY:
			if (const auto baddy = level->getBaddyById(source.first); baddy.has_value())
				return getScriptParameter(*baddy.value(), identifier);
			break;
		case ScriptObjectType::BOMB:
			if (const auto bomb = level->getBomb(source.first); bomb.has_value())
				return getScriptParameter(*bomb.value(), identifier);
			break;
		case ScriptObjectType::ARROW:
			if (const auto arrow = level->getArrow(source.first); arrow.has_value())
				return getScriptParameter(*arrow.value(), identifier);
			break;
		case ScriptObjectType::ITEM:
			if (const auto item = level->getItem(source.first); item.has_value())
				return getScriptParameter(*item.value(), identifier);
			break;
		case ScriptObjectType::EXPLOSION:
			if (const auto explo = level->getExplosion(source.first); explo.has_value())
				return getScriptParameter(*explo.value(), identifier);
			break;
		case ScriptObjectType::HORSE:
			if (const auto horse = level->getHorse(source.first); horse.has_value())
				return getScriptParameter(*horse.value(), identifier);
			break;
		case ScriptObjectType::SIGN:
			if (const auto sign = level->getSign(source.first); sign.has_value())
				return getScriptParameter(*sign.value(), identifier);
			break;
		default:;
	}

	return nullptr;
}

GameVariable* GS1Visitor::getGameVariableFromStorage(const std::string_view identifier, const std::optional<size_t> type) const
{
	// If we have a specific storage type, try to get the store for it.
	if (type.has_value())
	{
		if (const auto store = getGameVariableStoreForStorageType(type.value()); store != nullptr)
			return store->getOrAdd(identifier).lock().get();
	}

	// First, try to get a built-in variable.
	if (builtInStore != nullptr && builtInStore->contains(identifier))
		return builtInStore->get(identifier).lock().get();

	// Second, check the server's global variable store.
	if (server->Scripting.variables.contains(identifier))
		return server->Scripting.variables.get(identifier).lock().get();

	auto checkStore = [&](const ScriptObject& source) -> std::optional<GameVariable*>
	{
		auto* store = getGameVariableStoreFromSource(source);
		const bool storeHasIdentifier = store != nullptr && store->contains(identifier);

		// First, if we have a storage type, get directly from the variable store.
		if (type.has_value() && storeHasIdentifier)
			return store->get(identifier).lock().get();

		// Second, if we have no storage type, check for a property.
		if (auto property = getGameVariableFromSource(source, identifier); property != nullptr)
			return property;

		// Lastly, check the variable store.
		if (storeHasIdentifier)
			return store->get(identifier).lock().get();

		return std::nullopt;
	};

	// Next, look in the current source's store.
	if (const auto result = checkStore(getCurrentSource()); result.has_value())
		return result.value();

	// Now look in the original source's store.
	if (const auto result = checkStore(getOriginalSource()); result.has_value())
		return result.value();

	// Lastly, look at the initiator's store.
	if (m_event->initiator != getOriginalSource())
	{
		if (const auto result = checkStore(m_event->initiator); result.has_value())
			return result.value();
	}

	// If we still don't have a store, use the built-in store.
	return builtInStore->getOrAdd(identifier).lock().get();
}

//--

GameValue GS1Visitor::translateSourceText(antlr4::tree::ParseTree* node, std::string_view language)
{
	// TODO: We should cache this somewhere.

	if (node == nullptr)
		return GameValue{std::string{}};
	if (m_parser == nullptr)
		return GameValue{node->getText()};

	// Get the compound string context.
	const auto compoundStringContext = walkToContext<GS1Parser::CompoundStringContext>(node);
	if (compoundStringContext == nullptr)
		return GameValue{node->getText()};

	// Get the raw text of the compound string.
	std::string raw;
	if (auto* tokenStream = m_parser->getTokenStream(); tokenStream != nullptr)
		raw = tokenStream->getText(compoundStringContext->getSourceInterval());

	// If the text is empty or consists solely of whitespace, just evaluate the original string without translating.
	if (string::empty_or_whitespace(raw))
		return GameValue{node->getText()};

	return translateSourceText(raw, language);
}

GameValue GS1Visitor::translateSourceText(const std::string_view sourceText, const std::string_view language)
{
	// TODO: We should cache this somewhere.

	// Get the translation manager.
	// If we don't have one, just evaluate the original string.
	if (translationManager == nullptr)
		return GameValue{std::string{sourceText}};

	// Translate the raw string.
	const auto translated = translationManager->getText(language, string::trim(sourceText));

	// Reparse the translated string and get the result.
	return processStringExpression(translated);
}

GameValue GS1Visitor::processStringExpression(const std::string_view expression)
{
	// If we are already reparsing string content, do not recurse.
	if (m_reparsingStringExpression)
		return GameValue{std::string{expression}};

	// Reparse the string expression and get the result.
	SetAndRestore sar{m_reparsingStringExpression, true};
	const auto result = reparseExpression(expression, "S", [](GS1Parser& parser)
	{
		return parser.compound_string();
	});

	return GameValue{getScriptValueAsCopy<std::string>(result).value_or(std::string{})};
}

GameValue GS1Visitor::processMathExpression(std::string_view expression)
{
	// If we are already reparsing math content, do not recurse.
	//if (m_reparsingMathExpression)
	//	return 0.0;

	// Reparse the math expression and get the result.
	//SetAndRestore sar{m_reparsingMathExpression, true};
	const auto result = reparseExpression(std::format("({})", expression), "E", [](GS1Parser& parser)
	{
		return parser.primaryExpression();
	});

	return GameValue{getScriptValueAsCopy<double>(result).value_or(0.0)};
}

//--

std::vector<std::any> GS1Visitor::visitChildrenAndCollect(const antlr4::tree::ParseTree* node)
{
	if (node == nullptr) return {};
	std::vector<std::any> results;
	for (const auto child : node->children)
	{
		if (auto ret = child->accept(this); ret.has_value())
			results.emplace_back(std::move(ret));
	}
	return results;
}

///////////////////////////////////////////////////////////////////////////////
// Member functions.

std::any GS1Visitor::safeVisit(antlr4::tree::ParseTree* node)
{
	if (node == nullptr)
		return {};
	return visit(node);
}

std::any GS1Visitor::reparseExpression(std::string_view expression, const std::string_view lexerMode, const std::function<antlr4::tree::ParseTree*(GS1Parser&)>& node)
{
	GS1ErrorListener errorListenerLexer("lexing", who);
	GS1ErrorListener errorListenerParser("parsing", who);

	// Create an input stream for the expression.
	antlr4::ANTLRInputStream inputStream{expression};
	GS1Lexer lexer(&inputStream);
	lexer.removeErrorListeners();
	lexer.addErrorListener(&errorListenerLexer);

	// Put the lexer into the chosen mode.
	// E = expression, S = string.
	lexer.pushCommand(lexerMode);

	// Fill up our token stream with the lexer.
	antlr4::CommonTokenStream tokens(&lexer);
	tokens.fill();

	// Construct a parser to handle our tokens.
	GS1Parser parser(&tokens);
	parser.removeErrorListeners();
	parser.addErrorListener(&errorListenerParser);

	// Get our AST on the fragment.
	auto* tree = node(parser);

	// Sanity check the errors.
	if (parser.getNumberOfSyntaxErrors() != 0)
		throw std::runtime_error(std::format("failed to reparse expression: {}", expression));

	// Return the result of processing the tree.
	return tree->accept(this);
}

void GS1Visitor::setCurrentPlayerVariables(const std::optional<ScriptObject>& source) const
{
	if (!source.has_value() || source.value().second != ScriptObjectType::PLAYER)
	{
		builtInStore->clearTemporary("player");
		return;
	}

	const auto player = server->getNPCServer()->getPlayer(source.value().first);
	if (player == nullptr)
		return;

	// All the player property shortcuts.
	player->constructScriptParameters();
	auto playerSource = source::FromPlayer(player->getId());
	for (const auto& [name, variable] : player->scriptParameters)
	{
		GameVariable var{.name = std::format("player{}", name), .lifetime = variables::Lifetime::TEMPORARY};
		var.source = playerSource;
		var.setters = variable.setters;
		var.getters = variable.getters;
		builtInStore->add(std::move(var));
	}
}

///////////////////////////////////////////////////////////////////////////////

void GS1Visitor::execute(const ScriptEvent& event, const ScriptObject& source, GS1Parser& parser, ScriptExecutionContext& context, antlr4::tree::ParseTree* startNode)
{
	scriptContext = &context;

	m_parser = &parser;
	m_event = &event;
	m_originalSource = source;

	m_serverStore = getGameVariableStoreFromSource(source::FromServer());

	// Check for a sleep resume.
	// Sleeping scripts use the timeout event to resume themselves.
	if (event.type == ScriptEventType::TIMEOUT && !m_sleepCallStack.empty())
	{
		m_callStack = std::move(m_sleepCallStack);
		m_sleepCallStack.clear();
		startNode = m_callStack.back().first;

		m_currentSource = std::move(m_sleepCurrentSource);
		m_sleepCurrentSource.clear();
	}

	// Execute!
	try
	{
		size_t loops = 0;
		do
		{
			visit(startNode);

			if (!m_callStack.empty())
				startNode = m_callStack.back().first;

			assert(loops++ < 100);
		}
		while (!m_callStack.empty());
	}
	catch (const break_exception&)
	{
	}
	catch (const continue_exception&)
	{
	}
	catch (const return_exception&)
	{
	}
	catch (const sleep_exception&)
	{
		// Save the call stack.
		// We do it here because the sleep exception may get caught in multiple blocks in the visitor.
		m_sleepCallStack = std::move(m_callStack);
		m_callStack.clear();

		m_sleepCurrentSource = std::move(m_currentSource);
		m_currentSource.clear();
	}
	catch (...)
	{
		// Clear the call stack on any other exception.
		m_callStack.clear();
		throw;
	}

	m_callStack.clear();
}

///////////////////////////////////////////////////////////////////////////////

void GS1Visitor::reportError(std::string_view message, antlr4::tree::ParseTree* node, const bool abort)
{
	std::vector<std::pair<uint8_t, std::string>> logbatch;

	logbatch.emplace_back(0_ui8, std::format("* GS1 runtime script error for '{}':", who));
	if (abort) logbatch.emplace_back(0_ui8, "* Aborting script execution due to fatal error. *");

	logbatch.emplace_back(1_ui8, std::format("Error: {}", message));
	if (node != nullptr) logbatch.emplace_back(1_ui8, std::format("Code: '{}'", node->getText()));

	// Log the batch of messages.
	log::batch(log::script, logbatch);

	// Send the log messages to the server.
	std::ranges::for_each(logbatch, [this](const auto& kvp)
	{
		server->sendToNC(kvp.second);
	});

	if (abort) throw std::runtime_error("Terminating GS1 script.");
}

////////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitProgram(GS1Parser::ProgramContext* ctx)
{
	for (const auto node : ctx->children)
		node->accept(this);

	return {};
}

std::any GS1Visitor::visitBlock(GS1Parser::BlockContext* ctx)
{
	if (ctx->children.empty())
		return {};

	antlr4::tree::ParseTree* currentNode = ctx->children[0];
	size_t currentIndex = 0;

	auto moveNext = [&]()
	{
		// Move to the next node.
		if (++currentIndex < ctx->children.size())
		{
			currentNode = ctx->children[currentIndex];
			std::get<1>(m_callStack.back()) = currentIndex;
		}
		else
			currentNode = nullptr;
	};

	// If we already have a call stack, resume from where we left off.
	if (!m_callStack.empty() && ctx == m_callStack.back().first)
	{
		std::tie(currentNode, currentIndex) = m_callStack.back();
		currentNode = ctx->children[currentIndex];
		moveNext();
	}
	else m_callStack.emplace_back(ctx, currentIndex);

	// Move through the children.
	while (currentNode != nullptr)
	{
		// Visit the current node.
		if (antlr4::tree::ErrorNode::is(*currentNode))
			visitErrorNode(dynamic_cast<antlr4::tree::ErrorNode*>(currentNode));
		else if (antlr4::tree::TerminalNode::is(*currentNode))
			visitTerminal(dynamic_cast<antlr4::tree::TerminalNode*>(currentNode));
		else
		{
			try
			{
				currentNode->accept(this);
			}
			catch (const sleep_exception&)
			{
				// Don't pop off the call stack so we can resume from this spot.
				throw;
			}
			catch (...)
			{
				m_callStack.pop_back();
				throw;
			}
		}

		// Move to the next node.
		moveNext();
	}

	m_callStack.pop_back();

	return {};
}

////////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitStatementIf(GS1Parser::StatementIfContext* context)
{
	if (getScriptValueAsCopy<bool>(visit(context->expression())).value_or(false))
		return visit(context->block(0));
	else
		return safeVisit(context->block(1));
}

std::any GS1Visitor::visitStatementFor(GS1Parser::StatementForContext* context)
{
	bool enterLoopAfterSleep = false;

	// Sleep resume.
	// The block statement should be next in the stack, which means it will resume where it left off.
	if (!m_callStack.empty() && m_callStack.back().first == context)
	{
		m_callStack.pop_back();
		enterLoopAfterSleep = true;
	}
	else
	{
		// Assignment.
		safeVisit(context->assignmentStatement(0));
	}

	// for (assignmentStatement; expression; (assignmentStatement | expression)) block

	// Condition.
	size_t loopCount = 0;
	while ((loopCount++ < maximumLoopCount && getScriptValueAsCopy<bool>(safeVisit(context->expression(0))).value_or(false)) || enterLoopAfterSleep)
	{
		enterLoopAfterSleep = false;

		// Block.
		try
		{
			visit(context->block());
		}
		catch (const break_exception&)
		{
			break;
		}
		catch (const continue_exception&)
		{
			// Increment happens below.
			// continue;
		}
		catch (const sleep_exception&)
		{
			m_callStack.emplace_back(context, 0);
			throw;
		}

		// Increment.
		safeVisit(context->expression(1));
		safeVisit(context->assignmentStatement(1));
	}

	return {};
}

std::any GS1Visitor::visitStatementWhile(GS1Parser::StatementWhileContext* context)
{
	bool enterLoopAfterSleep = false;

	// Sleep resume.
	// The block statement should be next in the stack, which means it will resume where it left off.
	if (!m_callStack.empty() && m_callStack.back().first == context)
	{
		m_callStack.pop_back();
		enterLoopAfterSleep = true;
	}

	// Condition.
	size_t loopCount = 0;
	while ((loopCount++ < maximumLoopCount && getScriptValueAsCopy<bool>(visit(context->expression())).value_or(false)) || enterLoopAfterSleep)
	{
		enterLoopAfterSleep = false;

		// Block.
		try
		{
			visit(context->block());
		}
		catch (const break_exception&)
		{
			break;
		}
		catch (const continue_exception&)
		{
			continue;
		}
		catch (const sleep_exception&)
		{
			m_callStack.emplace_back(context, 0);
			throw;
		}
	}

	return {};
}

std::any GS1Visitor::visitStatementWith(GS1Parser::StatementWithContext* context)
{
	auto expression = visit(context->expression());
	auto value = getGS1ScriptValueFromAny(expression);
	auto scriptObject = getScriptObject(value);

	// No object?  Don't execute the block.
	if (!scriptObject.has_value())
		return {};

	// Push the source object onto the source stack.
	m_currentSource.emplace_back(*scriptObject);
	setCurrentPlayerVariables(*scriptObject);

	// Execute the block with the new source.
	auto result = visit(context->block());

	// Pop the source off the source stack.
	m_currentSource.pop_back();
	setCurrentPlayerVariables(findNearestScriptObjectSourceFromStack(ScriptObjectType::PLAYER));

	return result;
}

std::any GS1Visitor::visitStatementFunctionDefinition(GS1Parser::StatementFunctionDefinitionContext* context)
{
	// Don't execute user functions while walking through the tree.
	return {};
}

std::any GS1Visitor::visitStatementUserFunctionCall(GS1Parser::StatementUserFunctionCallContext* context)
{
	if (m_parser == nullptr)
		throw std::runtime_error("GS1Visitor is missing the link to the parser");

	auto identifier = context->compound_identifier()->getText();
	if (const auto function = m_parser->userFunctions.find(identifier); function != m_parser->userFunctions.end())
	{
		try
		{
			visit(function->second);
		}
		catch (const return_exception&)
		{
		}
		return {};
	}

	// Try to call the function in our joined classes.
	ScriptEvent eventCopy = *m_event;
	for (const auto script : getJoinedClassesFromSource(m_originalSource))
	{
		if (script->runUserDefinedFunction(identifier, eventCopy, m_originalSource))
			return {};
	}

	RECOVERABLE_PARSE_ERROR(std::format("Could not find user function '{}'.", identifier), {});
	return {};
}

std::any GS1Visitor::visitStatementBuiltInCommand(GS1Parser::StatementBuiltInCommandContext* context)
{
	// Get the command.
	auto command = context->COMMAND()->getText();
	string::trimRightMutate(command);

	try
	{
		// Process the built-in command.
		processBuiltInCommand(this, context, command);
	}
	catch (const sleep_exception&)
	{
		throw;
	}
	catch (const return_exception&)
	{
		throw;
	}
	catch (const unimplemented_error& e)
	{
		reportError(e.what(), context, false);
	}
	catch (const std::exception& e)
	{
		reportError(e.what(), context);
	}

	return {};
}

std::any GS1Visitor::visitStatementAssignment(GS1Parser::StatementAssignmentContext* context)
{
	// We need this to fix problems with timeout being both a flag and an NPC property.
	SetAndRestore sar{expectingTimeoutAsVariable, true};

	auto results = visitChildrenAndCollect(context);
	if (results.size() != 2 || context->children.size() != 3)
		throw std::runtime_error("AssignmentOperation is not a binary expression");

	const auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("AssignmentOperation has no operation");

	// Do the vector assignment operation separately as everything else runs on doubles.
	if (op.value() == GS1Parser::OP_ASSIGN && scriptValueContains<std::vector<double>>(results[1]))
	{
		const auto left = getGameVariable(results[0]);
		const auto right = getScriptValueAs<std::vector<double>>(results[1]);
		if (!left || !right)
			throw std::runtime_error("AssignmentOperation is not a vector expression");

		left->assign(right.value().get());

		// Update NPC last update time if the variable is from an NPC source.
		if (left->source.has_value() && left->source.value().second == ScriptObjectType::NPC)
		{
			if (const auto npc = server->getNPC(left->source.value().first); npc != nullptr)
				npc->lastUpdateTime = server->getFrameStartTime();
		}
		return {};
	}

	const auto left = getGameVariable(results[0]);
	auto right = getScriptValueAsCopy<double>(results[1]).value_or(0.0);

	// Special case for assigning a value to "timeout", which erases any existing sleep call stack.
	// Source: npcprogramming.doc, section 5.4.
	if (op.value() == GS1Parser::OP_ASSIGN && left->name == "timeout")
	{
		m_sleepCallStack.clear();
		m_sleepCurrentSource.clear();
	}

	const double leftD = left->getCopy<double>().value_or(0.0);

	// Perform the operation.
	switch (op.value())
	{
		case GS1Parser::OP_ASSIGN:
			left->assign(right, left->index);
			break;
		case GS1Parser::OP_ASSIGN_ADD:
			left->assign(leftD + right, left->index);
			break;
		case GS1Parser::OP_ASSIGN_SUB:
			left->assign(leftD - right, left->index);
			break;
		case GS1Parser::OP_ASSIGN_MUL:
			left->assign(leftD * right, left->index);
			break;
		case GS1Parser::OP_ASSIGN_DIV:
			left->assign(leftD / right, left->index);
			break;
		case GS1Parser::OP_ASSIGN_MOD:
			left->assign(static_cast<double>(static_cast<int64_t>(leftD) % static_cast<int64_t>(right)), left->index);
			break;
		case GS1Parser::OP_ASSIGN_POW:
			left->assign(std::pow(leftD, right), left->index);
			break;
		default:;
	}

	// Update NPC last update time if the variable is from an NPC source.
	if (left->source.has_value() && left->source.value().second == ScriptObjectType::NPC)
	{
		if (const auto npc = server->getNPC(left->source.value().first); npc != nullptr)
			npc->lastUpdateTime = server->getFrameStartTime();
	}

	// Assignment operations are statements and can't be used inside expressions.
	return {};
}

////////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitExpressionIn(GS1Parser::ExpressionInContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	SetAndRestore sar{expectingTimeoutAsVariable, true};

	std::vector<double> values;
	for (const auto& be : context->exponentiationExpression())
		values.emplace_back(getScriptValueAsCopy<double>(visit(be)).value_or(0.0));

	std::any right_any;
	if (context->primaryExpression() != nullptr)
		right_any = visit(context->primaryExpression());
	else right_any = visit(context->range_literal());

	const auto right_range = std::any_cast<std::pair<std::any, std::any>>(&right_any);
	const auto right_vector = getScriptValueAs<std::vector<double>>(right_any);

	size_t range_op_left = GS1Parser::TOKEN_PIPE;
	size_t range_op_right = GS1Parser::TOKEN_PIPE;
	if (right_range != nullptr)
	{
		range_op_left = getSymbolType(context->range_literal()->children[0]).value_or(GS1Parser::TOKEN_PIPE);
		range_op_right = getSymbolType(context->range_literal()->children[4]).value_or(GS1Parser::TOKEN_PIPE);
	}
	// Check for an early exit.
	else if (!right_vector.has_value())
		return makeGS1ScriptValue(false);

	bool range_met = true;
	for (const auto& check : values)
	{
		if (right_range != nullptr)
		{
			const double first = getScriptValueAsCopy<double>(right_range->first).value_or(0.0);
			const double second = getScriptValueAsCopy<double>(right_range->second).value_or(0.0);
			bool test_left = false, test_right = false;
			if (first < second)
			{
				test_left = (range_op_left == GS1Parser::TOKEN_PIPE) ? (first <= check) : (first < check);
				test_right = (range_op_right == GS1Parser::TOKEN_PIPE) ? (check <= second) : (check < second);
			}
			else
			{
				test_left = (range_op_left == GS1Parser::TOKEN_PIPE) ? (first >= check) : (first > check);
				test_right = (range_op_right == GS1Parser::TOKEN_PIPE) ? (check >= second) : (check > second);
			}
			const bool in_range = test_left && test_right;
			range_met = range_met && in_range;
		}
		else
		{
			range_met = range_met && (std::ranges::contains(right_vector.value().get(), check));
		}

		// Early out if we already know the result.
		if (!range_met)
			break;
	}

	return makeGS1ScriptValue(range_met);
}

std::any GS1Visitor::visitExpressionTernary(GS1Parser::ExpressionTernaryContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	std::any result = visit(context->logicalOrExpression());
	for (size_t i = 1; i < context->children.size(); i += 4)
	{
		if (getScriptValueAsCopy<bool>(result).value_or(false))
			result = visit(context->children[i + 1]);
		else result = visit(context->children[i + 3]);
	}
	return result;
}

std::any GS1Visitor::visitExpressionLogicOr(GS1Parser::ExpressionLogicOrContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	const auto left = getScriptValueAsCopy<bool>(visit(context->logicalAndExpression(0))).value_or(false);
	if (left) return makeGS1ScriptValue(true);

	for (size_t i = 2; i < context->children.size(); i += 2)
	{
		const auto right = getScriptValueAsCopy<bool>(visit(context->children[i])).value_or(false);
		if (right) return makeGS1ScriptValue(true);
	}

	return makeGS1ScriptValue(false);
}

std::any GS1Visitor::visitExpressionLogicAnd(GS1Parser::ExpressionLogicAndContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	const auto left = getScriptValueAsCopy<bool>(visit(context->equalityExpression(0))).value_or(false);
	if (!left) return makeGS1ScriptValue(false);

	for (size_t i = 2; i < context->children.size(); i += 2)
	{
		const auto right = getScriptValueAsCopy<bool>(visit(context->children[i])).value_or(false);
		if (!right) return makeGS1ScriptValue(false);
	}

	return makeGS1ScriptValue(true);
}

std::any GS1Visitor::visitExpressionEquality(GS1Parser::ExpressionEqualityContext* context)
{
	if (context->children.size() < 3)
		return visitChildren(context);

	const auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("ExpressionEquality does not have an operator");

	SetAndRestore sar{expectingTimeoutAsVariable, true};

	auto first = visit(context->children[0]);
	auto second = visit(context->children[2]);

	// Vector comparison checks.
	if (scriptValueContains<std::vector<double>>(first) && scriptValueContains<std::vector<double>>(second))
	{
		const auto left_vector = getScriptValueAs<std::vector<double>>(first);
		const auto right_vector = getScriptValueAs<std::vector<double>>(second);
		if (!left_vector.has_value() || !right_vector.has_value())
			throw std::runtime_error("ExpressionEquality has no left-hand side or right-hand side vector value");

		switch (op.value())
		{
			case GS1Parser::OP_EQUAL:
			case GS1Parser::OP_ASSIGN:
				return makeGS1ScriptValue(left_vector.value().get() == right_vector.value().get());
			case GS1Parser::OP_NOTEQ:
				return makeGS1ScriptValue(left_vector.value().get() != right_vector.value().get());
			default:;
		}
	}

	// Scalar comparison checks.
	const auto left = getScriptValueAsCopy<double>(first).value_or(0.0);
	const auto right = getScriptValueAsCopy<double>(second).value_or(0.0);

	// Do the comparison.
	switch (op.value())
	{
		case GS1Parser::OP_EQUAL:
		case GS1Parser::OP_ASSIGN:
			return makeGS1ScriptValue(DoublesAreSame(left, right));
		case GS1Parser::OP_NOTEQ:
			return makeGS1ScriptValue(!DoublesAreSame(left, right));
		default:;
	}

	throw std::runtime_error("ExpressionEquality has an unknown operator");
}

std::any GS1Visitor::visitExpressionRelational(GS1Parser::ExpressionRelationalContext* context)
{
	if (context->children.size() < 3)
		return visitChildren(context);

	SetAndRestore sar{expectingTimeoutAsVariable, true};

	const auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("ExpressionRelational does not have an operator");

	const auto left = getScriptValueAsCopy<double>(visit(context->children[0])).value_or(0.0);
	const auto right = getScriptValueAsCopy<double>(visit(context->children[2])).value_or(0.0);

	// Do the comparison.
	switch (op.value())
	{
		case GS1Parser::OP_LESS:
			return makeGS1ScriptValue(left < right);
		case GS1Parser::OP_GREAT:
			return makeGS1ScriptValue(left > right);
		case GS1Parser::OP_LESS_EQ:
			return makeGS1ScriptValue(left <= right);
		case GS1Parser::OP_GREAT_EQ:
			return makeGS1ScriptValue(left >= right);
		default:;
	}

	throw std::runtime_error("ExpressionRelational has an unknown operator");
}

std::any GS1Visitor::visitExpressionAdditive(GS1Parser::ExpressionAdditiveContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	SetAndRestore sar{expectingTimeoutAsVariable, true};

	double result = getScriptValueAsCopy<double>(visit(context->children[0])).value_or(0.0);
	std::string literal;
	for (size_t i = 1; i < context->children.size(); i += 2)
	{
		auto op = getSymbolType(context->children[i]);
		if (!op.has_value())
			continue;

		// Check if the right side is a literal for a small optimization.
		double right = 0.0;
		const auto child = context->children[i + 1];
		if (child->getTreeType() == antlr4::tree::ParseTreeType::TERMINAL)
		{
			literal = child->getText();
			if (literal == "true") right = 1.0;
			else if (literal == "false") right = 0.0;
			else right = string::toDouble(literal);
		}
		else
		{
			right = getScriptValueAsCopy<double>(visit(child)).value_or(0.0);
		}

		if (op.value() == GS1Parser::OP_ADD)
			result += right;
		else if (op.value() == GS1Parser::OP_SUB)
			result -= right;
	}

	return makeGS1ScriptValue(result);
}

std::any GS1Visitor::visitExpressionMultiplicative(GS1Parser::ExpressionMultiplicativeContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	SetAndRestore sar{expectingTimeoutAsVariable, true};

	double result = getScriptValueAsCopy<double>(visit(context->children[0])).value_or(0.0);
	std::string literal;
	for (size_t i = 1; i < context->children.size(); i += 2)
	{
		auto op = getSymbolType(context->children[i]);
		if (!op.has_value())
			continue;

		// Check if the right side is a literal for a small optimization.
		double right = 0.0;
		const auto child = context->children[i + 1];
		if (child->getTreeType() == antlr4::tree::ParseTreeType::TERMINAL)
		{
			literal = child->getText();
			if (literal == "true") right = 1.0;
			else if (literal == "false") right = 0.0;
			else right = string::toDouble(literal);
		}
		else
		{
			right = getScriptValueAsCopy<double>(visit(child)).value_or(0.0);
		}

		if (op.value() == GS1Parser::OP_MUL)
			result *= right;
		else if (op.value() == GS1Parser::OP_DIV)
			result /= right;
		else if (op.value() == GS1Parser::OP_MOD)
			result = static_cast<double>(static_cast<int64_t>(result) % static_cast<int64_t>(right));
	}

	return makeGS1ScriptValue(result);
}

std::any GS1Visitor::visitExpressionExponentiation(GS1Parser::ExpressionExponentiationContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	SetAndRestore sar{expectingTimeoutAsVariable, true};

	double result = getScriptValueAsCopy<double>(visit(context->children[0])).value_or(0.0);
	for (size_t i = 1; i < context->children.size(); i += 2)
	{
		if (auto op = getSymbolType(context->children[i]); !op.has_value())
			continue;

		const auto right = getScriptValueAsCopy<double>(visit(context->children[i + 1])).value_or(0.0);
		result = std::pow(result, right);
	}

	return makeGS1ScriptValue(result);
}

std::any GS1Visitor::visitExpressionUnary(GS1Parser::ExpressionUnaryContext* context)
{
	const auto op = getSymbolType(context->children[0]);
	if (!op.has_value())
		throw std::runtime_error("ExpressionUnary does not have an operator");

	if (op.value() == GS1Parser::OP_LOGICALNOT)
		return makeGS1ScriptValue(DoubleIsZero(getScriptValueAsCopy<double>(visit(context->unaryExpression())).value_or(0.0)));

	if (op.value() == GS1Parser::OP_SUB)
	{
		SetAndRestore sar{expectingTimeoutAsVariable, true};
		return makeGS1ScriptValue(-1 * getScriptValueAsCopy<double>(visit(context->unaryExpression())).value_or(0.0));
	}

	return visit(context->unaryExpression());
}

std::any GS1Visitor::visitExpressionPostfix(GS1Parser::ExpressionPostfixContext* context)
{
	const auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("ExpressionPostfix has no operation");

	SetAndRestore sar{expectingTimeoutAsVariable, true};

	auto anyval = visit(context->children[0]);
	const auto variable = getGameVariable(anyval);
	if (!variable)
		throw std::runtime_error("ExpressionPostfix has no valid variable");

	if (const auto left = variable->get<double>(); left.has_value())
	{
		auto& value = left.value().get();

		// Perform the operation.
		switch (op.value())
		{
			case GS1Parser::OP_INC:
				value += 1.0;
				break;
			case GS1Parser::OP_DEC:
				value -= 1.0;
				break;
			default:
				throw std::runtime_error("ExpressionPostfix does not have an operator");
		}

		// Update NPC last update time if the variable is from an NPC source.
		if (variable->source.has_value() && variable->source.value().second == ScriptObjectType::NPC)
		{
			if (const auto npc = server->getNPC(variable->source.value().first); npc != nullptr)
				npc->lastUpdateTime = server->getFrameStartTime();
		}
	}

	// GS1 assignment operations are statements and can't be used inside expressions.
	// So don't return anything.
	return {};
}

////////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitBuiltInFunctionCall(GS1Parser::BuiltInFunctionCallContext* context)
{
	// Get the command.
	auto command = context->FUNCTION()->getText();
	string::trimRightMutate(command);

	try
	{
		// Process the built-in function call.
		return processBuiltInFunction(this, context, command);
	}
	catch (const std::exception& e)
	{
		reportError(e.what(), context);
	}
	return {};
}

std::any GS1Visitor::visitIdentifierAccess(GS1Parser::IdentifierAccessContext* context)
{
	auto first = visit(context->identifier_value(0));
	if (context->children.size() == 1)
	{
		// No accessors, just return the first identifier value.
		return first;
	}

	// The first identifier value should be a ScriptObject.
	auto value = getGS1ScriptValueFromAny(first);
	auto objectSource = getScriptObject(value);
	if (!objectSource.has_value())
		RECOVERABLE_PARSE_ERROR(std::format("Identifier did not contain a script object: {}.", context->children[0]->getText()), 0.0);

	size_t pos = 1;
	const size_t identifierCount = context->identifier_value().size();

	// Iterate through the identifier values, adjusting our current source object as we go.
	do
	{
		// Temporarily push the current source onto the stack and get the next identifier value.
		// We don't need to keep it on the stack so pop it after we're done.
		m_currentSource.push_back(objectSource.value());
		{
			first = visit(context->identifier_value(pos++));
		}
		m_currentSource.pop_back();

		// Check if the result is a ScriptObject.
		value = getGS1ScriptValueFromAny(first);
		objectSource = getScriptObject(value);

		// If not, we might be done.
		if (!objectSource.has_value())
		{
			if (pos > identifierCount)
				throw std::runtime_error("IdentifierAccess has no valid identifier value.");
			return makeGS1ScriptValue(std::move(value));
		}
	}
	while (pos < identifierCount);

	// If we made it here somehow, just return an empty GS1ScriptValue.
	return makeGS1ScriptValue(0.0);
}

std::any GS1Visitor::visitIdentifierValue(GS1Parser::IdentifierValueContext* context)
{
	auto identifier_any = visit(context->compound_identifier());
	const auto identifier = std::any_cast<std::string>(&identifier_any);
	if (identifier == nullptr)
		throw std::runtime_error("IdentifierValue has no valid compound_identifier");

	const auto expressions = context->expression();
	std::optional<int64_t> index = std::nullopt;

	// Identify the storage type based on the identifier name.
	auto storage = getStorageTypeFromIdentifier(*identifier);

	// Test for tiles[x,y].
	// Since tiles[x,y] is a unique case, we encode the index with the X/Y.
	if (*identifier == "tiles" && expressions.size() == 2)
	{
		const auto param1 = visit(expressions[0]);
		const auto param2 = visit(expressions[1]);
		const auto x = static_cast<int32_t>(std::max(0.0, getScriptValueAsCopy<double>(param1).value_or(0.0)));
		const auto y = static_cast<int32_t>(std::max(0.0, getScriptValueAsCopy<double>(param2).value_or(0.0)));
		index = (static_cast<int64_t>(x) << 32) | (static_cast<int64_t>(y) & 0xFFFFFFFF);
	}
	else if (expressions.size() == 1)
	{
		// Get the array index.
		const auto expression_any = visit(expressions[0]);
		index = static_cast<int64_t>(getScriptValueAsCopy<double>(expression_any).value_or(0.0));
	}

	// If we have an identifier, and the flag store has a matching value, return that.
	if (!identifier->empty() && flagStore.contains(*identifier))
	{
		// Timeout is annoying, so make sure we are not doing something that needs the NPC timeout.
		if (*identifier != "timeout" || !expectingTimeoutAsVariable)
		{
			if (const auto flag = flagStore.get(*identifier).lock(); flag != nullptr)
				return makeGS1ScriptValue(flag->getCopy<bool>().value_or(false));
		}
	}

	// Strip the storage type from the identifier, if needed.
	fixStorageNameOnIdentifier(*identifier);

	// If we have no storage value, and we are expecting a flag, force client storage.
	if (!storage.has_value() && expectingFlag)
		storage = ENUM(StorageType::CLIENT);

	// Conformance.
	// If we are expecting a flag, force certain variables to script storage.
	if (!expectingFlag && (scriptEngine->config.strictMode.getValue() || scriptEngine->config.alwaysScopeVariables.getValue() == false))
	{
		constexpr std::array alwaysScriptStorage = {
			// Client storage types.
			ENUM(StorageType::CLIENT),
			ENUM(StorageType::CLIENTO),
			ENUM(StorageType::CLIENTR),
			ENUM(StorageType::CLIENTRO),
		};
		if (storage.has_value() && inList(storage.value(), alwaysScriptStorage))
			storage = ENUM(StorageType::LOCAL);
	}

	// Get the game variable store for the identifier.
	// If there is no storage type, it pulls from the built-in variable store (saved on the script context).
	if (auto variable = getGameVariableFromStorage(*identifier, storage); variable != nullptr)
	{
		// If it is temp storage, make sure the variable is marked as temporary so it isn't saved.
		if (storage.value_or(ENUM(StorageType::THIS)) == ENUM(StorageType::TEMP))
			variable->lifetime = variables::Lifetime::TEMPORARY;

		// If we have an index, it is an array access, so return the value at that index.
		// We need a direct reference to the value in the array.
		// Since variables are stored in maps, references are never invalidated when the container size changes, so this is safe.
		if (index.has_value())
		{
			if (index.value() < 0)
				return makeGS1ScriptValue(GameVariable{.value{0.0}});

			size_t fixedIndex = static_cast<size_t>(std::max(0_i64, index.value()));
			if (const auto val = variable->get<double>(index); val.has_value() && !variable->index.has_value())
			{
				// Construct a new GameVariable that wraps around the reference.
				return makeGS1ScriptValue(helpers::wrapReferenceIntoGameVariable(val.value()));
			}
			else
			{
				// Make a copy and set the index.
				GameVariable wrap{*variable};
				wrap.index = fixedIndex;
				return makeGS1ScriptValue(std::move(wrap));
			}
		}

		// NOLINTNEXTLINE(*-move-const-arg)
		return makeGS1ScriptValue(std::move(variable));
	}

	// Return a default value if the identifier is not found.
	return makeGS1ScriptValue(GameVariable{.value{0.0}});
}

std::any GS1Visitor::visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context)
{
	std::string compoundIdentifier;

	// Temporarily turn off the flag expectation while we build the final identifier.
	// This allows things like server.player_#v(playerid) to read from the correct storage area.
	SetAndRestore sar{expectingFlag, false};

	bool hasReservedConstant = false;
	auto reserved = context->RESERVEDCONSTANTS();

	for (auto& tree : context->children)
	{
		// Check for reserved constants.
		// Variables like 'pi' are not allowed, but 'this.pi' would be.
		// So flag that we had a reserved constant so we can check if it is allowed after the full compound identifier is formed.
		if (std::ranges::contains(reserved, tree))
		{
			hasReservedConstant = true;
			compoundIdentifier.append(tree->getText());
		}
		// Just normal text.
		else if (tree->getTreeType() == antlr4::tree::ParseTreeType::TERMINAL)
			compoundIdentifier.append(tree->getText());
		// Something like a message code.
		else
		{
			auto piece = tree->accept(this);
			compoundIdentifier.append(getScriptValueAsCopy<std::string>(piece).value_or(""s));
		}
	}

	string::trimMutate(compoundIdentifier);

	if (hasReservedConstant)
	{
		if (isReservedConstant(compoundIdentifier))
			throw script_error(std::format("'{}' is a reserved keyword and cannot be used as an identifier.", compoundIdentifier));
	}

	return std::make_any<std::string>(std::move(compoundIdentifier));
}

std::any GS1Visitor::visitCompoundString(GS1Parser::CompoundStringContext* context)
{
	std::string compoundString;

	for (const auto& tree : context->children)
	{
		if (tree->getTreeType() == antlr4::tree::ParseTreeType::TERMINAL)
			compoundString.append(tree->getText());
		else
		{
			auto piece = tree->accept(this);
			if (const auto gs1Val = std::any_cast<GS1ScriptValue>(&piece); gs1Val != nullptr)
			{
				// If this is a GameVariable and the results size is 1, just return the piece.
				if ((std::holds_alternative<GameVariable*>(*gs1Val) || std::holds_alternative<GameVariable>(*gs1Val)) && context->children.size() == 1)
					return piece;
			}
			compoundString.append(getScriptValueAsCopy<std::string>(piece).value_or(""s));
		}
	}

	string::trimMutate(compoundString);
	return makeGS1ScriptValue(GameValue{compoundString});
}

std::any GS1Visitor::visitMessageCode(GS1Parser::MessageCodeContext* context)
{
	auto messageCode = context->MESSAGECODE()->getText();
	if (messageCode.empty())
		RECOVERABLE_PARSE_ERROR(std::format("Message code '{}' is not a valid message code.", messageCode), ""s);

	// Trim out the message code.
	std::string_view messageCodeView{messageCode};
	if (messageCodeView.ends_with('('))
		messageCodeView.remove_suffix(1);
	if (messageCodeView.starts_with('#'))
		messageCodeView.remove_prefix(1);

	try
	{
		// Process the message code.
		return processMessageCode(this, context, messageCodeView);
	}
	catch (const unimplemented_error& e)
	{
		reportError(e.what(), context, false);
		return GS1ScriptValue{GameValue{""s}};
	}
	catch (const std::logic_error& e)
	{
		reportError(e.what(), context, false);
		return GS1ScriptValue{GameValue{""s}};
	}
	catch (const std::exception& e)
	{
		reportError(e.what(), context);
	}
	return {};
}

////////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitFlowReturn(GS1Parser::FlowReturnContext* context)
{
	throw return_exception{};
}

std::any GS1Visitor::visitFlowBreak(GS1Parser::FlowBreakContext* context)
{
	throw break_exception{};
}

std::any GS1Visitor::visitFlowContinue(GS1Parser::FlowContinueContext* context)
{
	throw continue_exception();
}

////////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitLiteral(GS1Parser::LiteralContext* context)
{
	if (context->LITERAL() != nullptr)
	{
		const auto text = context->LITERAL()->getText();
		if (text == "true") return makeGS1ScriptValue(true);
		if (text == "false") return makeGS1ScriptValue(false);
		return makeGS1ScriptValue(std::stod(text));
	}
	else if (const auto reserved = context->RESERVEDCONSTANTS(); reserved != nullptr)
	{
		const auto text = reserved->getText();
		if (string::equalsi(text, "allstats"sv))
			return makeGS1ScriptValue(static_cast<double>(0xFFFF));
		if (string::equalsi(text, "allfeatures"sv))
			return makeGS1ScriptValue(static_cast<double>(0xFFFF));
		if (string::equalsi(text, "pi"sv))
			return makeGS1ScriptValue(std::numbers::pi);
	}
	return makeGS1ScriptValue(0.0);
}

std::any GS1Visitor::visitRangeLiteral(GS1Parser::RangeLiteralContext* context)
{
	auto left = visit(context->expression(0));
	auto right = visit(context->expression(1));
	return std::make_pair(std::move(left), std::move(right));
}

std::any GS1Visitor::visitArrayLiteral(GS1Parser::ArrayLiteralContext* context)
{
	std::vector<double> values;

	size_t valueIndex = 0;
	for (size_t i = 0; i < context->children.size() && i < maximumArraySize; ++i)
	{
		const auto child = context->children[i];
		if (auto symbol = getSymbolType(child); symbol.has_value())
		{
			if (*symbol == GS1Parser::TOKEN_COMMA)
			{
				++valueIndex;
				if (valueIndex > values.size())
					values.push_back(0.0);
			}
		}
		else
		{
			auto result = child->accept(this);
			values.push_back(getScriptValueAsCopy<double>(result).value_or(0.0));
		}
	}

	// This covers the case of {} and {1,}, which should result in {0} and {1,0}, respectively.
	if (valueIndex == values.size())
		values.push_back(0.0);

	// Ensure the array is not larger than the maximum size.
	if (values.size() > maximumArraySize)
		values.resize(maximumArraySize);

	return makeGS1ScriptValue(GameValue{std::move(values)});
}

std::any GS1Visitor::visitItemLiteral(GS1Parser::ItemLiteralContext* context)
{
	const auto text = context->ITEM()->getText();
	auto it = std::ranges::find(ItemNames, text);
	if (it == ItemNames.end())
		it = ItemNames.begin();

	return makeGS1ScriptValue(static_cast<double>(std::distance(ItemNames.begin(), it)));
}

std::any GS1Visitor::visitCarryLiteral(GS1Parser::CarryLiteralContext* context)
{
	const auto text = context->CARRY()->getText();
	auto it = std::ranges::find(carryNames, text);
	if (it == carryNames.end())
		it = carryNames.begin();

	return makeGS1ScriptValue(static_cast<double>(std::distance(carryNames.begin(), it)));
}

std::any GS1Visitor::visitDirectionLiteral(GS1Parser::DirectionLiteralContext* context)
{
	ptrdiff_t index = 0;
	const auto text = context->DIRECTION()->getText();
	if (const auto it = std::ranges::find(directionNames, text); it != directionNames.end())
		index = std::distance(directionNames.begin(), it);
	else
	{
		index = static_cast<ptrdiff_t>(string::toNumber(text));
		index = index % 4;
	}

	return makeGS1ScriptValue(static_cast<double>(index));
}

std::any GS1Visitor::visitGenderLiteral(GS1Parser::GenderLiteralContext* context)
{
	const auto text = context->GENDER()->getText();
	auto it = std::ranges::find(genderNames, text);
	if (it == genderNames.end())
		it = genderNames.begin();

	return makeGS1ScriptValue(static_cast<double>(std::distance(genderNames.begin(), it)));
}

std::any GS1Visitor::visitColorLiteral(GS1Parser::ColorLiteralContext* context)
{
	return makeGS1ScriptValue(getColorValueFromString(context->COLOR()->getText()));
}

std::any GS1Visitor::visitBaddyLiteral(GS1Parser::BaddyLiteralContext* context)
{
	const auto text = context->BADDY()->getText();
	auto it = std::ranges::find(BaddyNames, text);
	if (it == BaddyNames.end())
		it = BaddyNames.begin();

	// Specially handle spider.
	auto id = std::distance(BaddyNames.begin(), it);
	if (id == BADDYTYPE_COUNT)
		id = ENUM(BaddyType::OCTOPUS);

	return makeGS1ScriptValue(static_cast<double>(id));
}

std::any GS1Visitor::visitPrimaryExpression(GS1Parser::PrimaryExpressionContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	if (const auto expression = context->expression(); expression != nullptr)
		return visit(expression);

	if (const auto messageCode = context->messagecode_string(); messageCode != nullptr)
		return visit(messageCode);

	throw std::runtime_error("primaryExpression was unhandled");
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
