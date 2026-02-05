#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <exception>
#include <format>
#include <generator>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <BabyDI.h>
#include <GS1Parser.h>
#include <tree/ErrorNode.h>
#include <tree/ParseTree.h>
#include <tree/ParseTreeType.h>
#include <tree/TerminalNode.h>

#include <Server.h>
#include <level/Level.h>
#include <level/LevelBaddy.h>
#include <level/LevelItem.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <scripting/gs1/GS1Commands.h>
#include <scripting/gs1/GS1Functions.h>
#include <scripting/gs1/GS1MessageCodes.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/Script.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

#ifdef DEBUG
#define RECOVERABLE_PARSE_ERROR(MESSAGE, RETVAL) throw std::runtime_error(std::format("GS1 Parse Error: {}", MESSAGE))
#else
#define RECOVERABLE_PARSE_ERROR(MESSAGE, RETVAL) do { reportError(MESSAGE, context, false); return RETVAL; } while(false)
#endif

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

constexpr size_t MAX_LOOPS = 10000;

///////////////////////////////////////////////////////////////////////////////
// File static functions.

static std::optional<size_t> getSymbolType(antlr4::tree::ParseTree* tree)
{
	if (tree == nullptr) return std::nullopt;

	// We might be looking for the direct child.
	if (tree->children.size() == 1)
		tree = tree->children[0];

	// Find the symbol type if this is a TerminalNode.
	if (auto* node = dynamic_cast<antlr4::tree::TerminalNode*>(tree); node != nullptr)
		return node->getSymbol()->getType();

	return std::nullopt;
}

static std::generator<Script*> getJoinedClassesFromSource(ScriptObject source)
{
	auto* server = BabyDI::Get<Server>();
	switch (source.second)
	{
		case ScriptObjectType::NPC:
			if (auto npc = server->getNPC(source.first); npc != nullptr)
			{
				for (ScriptClassPtr scriptClass : npc->getJoinedClasses())
					co_yield &scriptClass->getScript();
			}
			break;

		case ScriptObjectType::WEAPON:
		{
			auto& weaponList = server->getWeaponList();
			if (auto it = weaponList.find(source.first); it != weaponList.end())
			{
				for (ScriptClassPtr scriptClass : it->second->getJoinedClasses())
					co_yield &scriptClass->getScript();
			}
		}
	}
}

static GameVariableStore* getGameVariableStoreFromSource(ScriptObject source)
{
	static GameVariableStore invalidStore;

	auto* server = BabyDI::Get<Server>();
	switch (source.second)
	{
		case ScriptObjectType::PLAYER:
			if (auto player = server->getNPCServer()->getPlayer(source.first); player != nullptr)
				return &player->account.variables;
			break;
		case ScriptObjectType::NPC:
			if (auto npc = server->getNPC(source.first); npc != nullptr)
				return &npc->scripting.variables;
			break;
		case ScriptObjectType::WEAPON:
		{
			auto& weaponList = server->getWeaponList();
			if (auto it = weaponList.find(source.first); it != weaponList.end())
				return &it->second->scripting.variables;
			log::printLine(log::script, "Could not find weapon source.");
			return &invalidStore;
		}
		case ScriptObjectType::LEVEL:
		{
			auto& levelList = server->getLevelList();
			if (auto it = levelList.find(source.first); it != levelList.end())
				return &it->second->scripting.variables;
			log::printLine(log::script, "Could not find level source.");
			return &invalidStore;
		}
		case ScriptObjectType::SERVER:
			return &server->Scripting.variables;
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

void GS1Visitor::applyStorageToIdentifier(std::optional<size_t> storage, std::string& identifier) noexcept
{
	if (!storage.has_value())
		return;

	switch (storage.value())
	{
		case GS1Parser::STORAGE_CLIENT:
		case GS1Parser::STORAGE_CLIENTO:
			identifier = std::format("client.{}", identifier);
			break;
		case GS1Parser::STORAGE_CLIENTR:
		case GS1Parser::STORAGE_CLIENTRO:
			identifier = std::format("clientr.{}", identifier);
			break;
		case GS1Parser::STORAGE_SERVER:
			identifier = std::format("server.{}", identifier);
			break;
		case GS1Parser::STORAGE_SERVERR:
			identifier = std::format("serverr.{}", identifier);
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Public member functions.

GameValue* GS1Visitor::getGameValueFromGS1ScriptValue(GS1ScriptValue& value)
{
	if (auto* gs1GameVariable = std::get_if<GS1GameVariable>(&value); gs1GameVariable != nullptr)
		return &gs1GameVariable->first;
	return nullptr;
}

std::optional<GameValue> GS1Visitor::getGameValueFromSource(const ScriptObject& source, std::string_view identifier)
{
	auto* server = BabyDI::Get<Server>();

	switch (source.second)
	{
		case ScriptObjectType::NPC:
			if (auto npc = server->getNPC(source.first); npc != nullptr)
				return getScriptParameter(*npc, identifier);
			break;
		case ScriptObjectType::PLAYER:
			if (auto player = server->getNPCServer()->getPlayer(source.first); player != nullptr)
				return getScriptParameter(*player, identifier);
			break;
		case ScriptObjectType::BADDY:
			if (auto level = findCurrentLevel(); level != nullptr)
			{
				if (auto baddy = level->getBaddyById(source.first); baddy.has_value())
					return getScriptParameter(*baddy.value(), identifier);
			}
			break;
		case ScriptObjectType::BOMB:
			if (auto level = findCurrentLevel(); level != nullptr)
			{
				if (auto bomb = level->getBomb(source.first); bomb.has_value())
					return getScriptParameter(*bomb.value(), identifier);
			}
			break;
		case ScriptObjectType::ARROW:
			if (auto level = findCurrentLevel(); level != nullptr)
			{
				if (auto arrow = level->getArrow(source.first); arrow.has_value())
					return getScriptParameter(*arrow.value(), identifier);
			}
			break;
		case ScriptObjectType::ITEM:
			if (auto level = findCurrentLevel(); level != nullptr)
			{
				if (auto item = level->getItem(source.first); item.has_value())
					return getScriptParameter(*item.value(), identifier);
			}
			break;
		case ScriptObjectType::EXPLOSION:
			if (auto level = findCurrentLevel(); level != nullptr)
			{
				if (auto explo = level->getExplosion(source.first); explo.has_value())
					return getScriptParameter(*explo.value(), identifier);
			}
			break;
		case ScriptObjectType::HORSE:
			if (auto level = findCurrentLevel(); level != nullptr)
			{
				if (auto horse = level->getHorse(source.first); horse.has_value())
					return getScriptParameter(*horse.value(), identifier);
			}
			break;
		case ScriptObjectType::SIGN:
			if (auto level = findCurrentLevel(); level != nullptr)
			{
				if (auto sign = level->getSign(source.first); sign.has_value())
					return getScriptParameter(*sign.value(), identifier);
			}
			break;
	}

	return std::nullopt;
}

GameValue GS1Visitor::getGameValueFromStorage(std::string_view identifier, std::optional<size_t> type)
{
	// If we have a specific storage type, try to get the store for it.
	if (type.has_value())
	{
		if (auto* store = getGameVariableStoreForStorageType(type.value()); store != nullptr)
			return store->getOrStub(identifier);
	}

	// First, try to get a built-in variable.
	if (builtInStore != nullptr && builtInStore->contains(identifier))
		return builtInStore->getOrStub(identifier);

	// Second, look in the current source's store.
	auto* currentStore = getGameVariableStoreFromSource(getCurrentSource());
	{
		// For the current store, we only want to return a variable if it exists.
		// If the variable does not exist, it will be added to the source store.
		if (currentStore != nullptr && currentStore->contains(identifier))
			return currentStore->getOrStub(identifier);
		else if (auto property = getGameValueFromSource(getCurrentSource(), identifier); property.has_value())
			return property.value();
	}

	// Now look in the original source's store.
	{
		if (auto* sourceStore = getGameVariableStoreFromSource(getOriginalSource()); sourceStore != nullptr)
		{
			if (sourceStore->contains(identifier))
				return sourceStore->getOrStub(identifier);
		}
		if (auto property = getGameValueFromSource(getOriginalSource(), identifier); property.has_value())
			return property.value();
	}

	// Now look at the initiator's store.
	if (m_event->initiator != getOriginalSource())
	{
		if (auto* initiatorStore = getGameVariableStoreFromSource(m_event->initiator); initiatorStore != nullptr)
		{
			if (initiatorStore->contains(identifier))
				return initiatorStore->getOrStub(identifier);
		}
		if (auto property = getGameValueFromSource(m_event->initiator, identifier); property.has_value())
			return property.value();
	}

	// If we still don't have a store, use the built-in store.
	return builtInStore->getOrStub(identifier);
}

double GS1Visitor::getColorValueFromString(std::string_view colorString)
{
	auto it = std::ranges::find(colorNames, colorString);
	if (it == colorNames.end())
		it = colorNames.begin();

	return static_cast<double>(std::distance(colorNames.begin(), it));
}

std::vector<std::any> GS1Visitor::visitChildrenAndCollect(antlr4::tree::ParseTree* node)
{
	if (node == nullptr) return {};
	std::vector<std::any> results;
	for (size_t i = 0; i < node->children.size(); ++i)
	{
		auto ret = node->children[i]->accept(this);
		if (ret.has_value())
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

std::optional<ScriptObject> GS1Visitor::findNearestScriptObjectSourceFromStack(ScriptObjectType type) const
{
	for (const auto& source : sourceStack())
	{
		if (source.second == type)
			return source;
	}
	return std::nullopt;
}

std::shared_ptr<Level> GS1Visitor::findCurrentLevel() const
{
	auto* server = BabyDI::Get<Server>();
	auto testSource = [server](const ScriptObject& source) -> std::shared_ptr<Level>
	{
		if (source.second == ScriptObjectType::NPC)
		{
			if (auto npc = server->getNPC(source.first); npc != nullptr)
				return npc->getLevel();
		}
		else if (source.second == ScriptObjectType::PLAYER)
		{
			if (auto player = server->getNPCServer()->getPlayer(source.first); player != nullptr)
				return server->getLoadedLevel(player->account.level, player);
		}
		else if (source.second == ScriptObjectType::LEVEL)
		{
			auto& levelList = server->getLevelList();
			if (auto level = levelList.find(source.first); level != levelList.end())
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
	auto* server = BabyDI::Get<Server>();
	auto testSource = [server](const ScriptObject& source) -> std::tuple<std::shared_ptr<Level>, std::shared_ptr<SubLevel>, std::shared_ptr<StaticLevelData>>
	{
		if (source.second == ScriptObjectType::NPC)
		{
			if (auto npc = server->getNPC(source.first); npc != nullptr)
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
			if (auto player = server->getNPCServer()->getPlayer(source.first); player != nullptr)
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
			if (auto level = levelList.find(source.first); level != levelList.end())
				return std::make_tuple(level->second, nullptr, nullptr);
		}
		return std::make_tuple(nullptr, nullptr, nullptr);
	};

	for (const auto& source : sourceStack())
	{
		if (auto level = testSource(source); std::get<0>(level) != nullptr)
			return level;
	}
	return std::make_tuple(nullptr, nullptr, nullptr);;
}

GameVariableStore* GS1Visitor::findGameVariableStoreFromSourceStack(ScriptObjectType type, int skip) const
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

GameVariableStore* GS1Visitor::getGameVariableStoreForStorageType(size_t type)
{
	GameVariableStore* store = nullptr;
	int skip = 0;
	if (inList(type, GS1Parser::STORAGE_THISO, GS1Parser::STORAGE_CLIENTO, GS1Parser::STORAGE_CLIENTRO))
		skip = 1;

	switch (type)
	{
		case GS1Parser::STORAGE_THIS:
		case GS1Parser::STORAGE_LOCAL:
		case GS1Parser::STORAGE_TEMP:
		case GS1Parser::STORAGE_THISO:
			store = findGameVariableStoreFromSourceStack(ScriptObjectType::NPC, skip);
			if (store == nullptr)
				store = findGameVariableStoreFromSourceStack(ScriptObjectType::WEAPON, skip);
			break;
		case GS1Parser::STORAGE_CLIENT:
		case GS1Parser::STORAGE_CLIENTR:
		case GS1Parser::STORAGE_CLIENTO:
		case GS1Parser::STORAGE_CLIENTRO:
			store = findGameVariableStoreFromSourceStack(ScriptObjectType::PLAYER, skip);
			break;
		case GS1Parser::STORAGE_SERVER:
		case GS1Parser::STORAGE_SERVERR:
			store = m_serverStore;
			break;
		case GS1Parser::STORAGE_LEVEL:
		{
			auto* server = BabyDI::Get<Server>();
			auto pair = getPlayerOrNPCFromSource(m_originalSource);
			if (!pair.has_value())
				return nullptr;

			const auto picker = visit_functions
			{
				[&server](PlayerPtr& player) -> LevelPtr { return server->getLoadedLevel(player->account.level, player); },
				[&server](NPCPtr& npc) -> LevelPtr { return npc->getLevel(); }
			};

			auto level = std::visit(picker, pair.value());
			return &level->scripting.variables;
		}
	}

	return store;
}

GS1GameVariable GS1Visitor::getGameVariableFromAny(std::any& value)
{
	if (auto* gs1ScriptValue = std::any_cast<GS1ScriptValue>(&value); gs1ScriptValue != nullptr)
	{
		if (auto* gs1GameVariable = std::get_if<GS1GameVariable>(gs1ScriptValue); gs1GameVariable != nullptr)
			return *gs1GameVariable;
		return {};
	}

	if (auto* gs1GameVariable = std::any_cast<GS1GameVariable>(&value); gs1GameVariable != nullptr)
		return *gs1GameVariable;

	return {};
}

GameValue GS1Visitor::getReadOnlyGameValueFromGS1ScriptValue(const GS1ScriptValue& value)
{
	if (auto* gs1GameVariable = std::get_if<GS1GameVariable>(&value); gs1GameVariable != nullptr)
	{
		if (gs1GameVariable->second.has_value())
			return gs1GameVariable->first.flatten(gs1GameVariable->second.value());
		return gs1GameVariable->first;
	}
	else if (auto* gameValue = std::get_if<GameValue>(&value); gameValue != nullptr)
	{
		return *gameValue;
	}
	return {};
}

GameValue GS1Visitor::getReadOnlyGameValueFromAny(const std::any& value)
{
	if (auto* gs1ScriptValue = std::any_cast<GS1ScriptValue>(&value); gs1ScriptValue != nullptr)
		return getReadOnlyGameValueFromGS1ScriptValue(*gs1ScriptValue);
	return {};
}

std::optional<ScriptObject> GS1Visitor::getSourceFromGS1ScriptValue(GS1ScriptValue& value)
{
	if (auto* scriptObject = std::get_if<ScriptObject>(&value); scriptObject != nullptr)
		return *scriptObject;
	else if (auto* gs1GameVariable = std::get_if<GS1GameVariable>(&value); gs1GameVariable != nullptr)
	{
		auto* scriptObject = gs1GameVariable->first.get_unsafe<ScriptObject>(gs1GameVariable->second);
		if (scriptObject != nullptr)
			return *scriptObject;
	}
	return std::nullopt;
}

void GS1Visitor::setCurrentPlayerVariables(std::optional<ScriptObject> source)
{
	if (!source.has_value() || source.value().second != ScriptObjectType::PLAYER)
	{
		builtInStore->clearTemporary("player");
		return;
	}

	auto server = BabyDI::Get<Server>();
	auto player = server->getNPCServer()->getPlayer(source.value().first);
	if (player == nullptr)
		return;

	// all the player property shortcuts
	player->constructScriptParameters();
	for (const auto& [name, variable] : player->scriptParameters)
		builtInStore->add(GameValue{ set_temporary, std::format("player{}", name), variable.getGetter(), variable.getSetter() });
}

///////////////////////////////////////////////////////////////////////////////

void GS1Visitor::execute(const ScriptEvent& event, ScriptObject source, GS1Parser& parser, ScriptExecutionContext& context, antlr4::tree::ParseTree* startNode)
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

			assert(loops < 100);
		}
		while (!m_callStack.empty());
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

	m_callStack.clear();
}

///////////////////////////////////////////////////////////////////////////////

void GS1Visitor::reportError(std::string_view message, antlr4::tree::ParseTree* node, bool abort)
{
	std::vector<std::pair<uint8_t, std::string>> logbatch;

	logbatch.emplace_back(0_ui8, std::format("* GS1 runtime script error for '{}':", who));
	if (abort) logbatch.emplace_back(0_ui8, "* Aborting script execution due to fatal error. *");

	logbatch.emplace_back(1_ui8, std::format("Error: {}", message));
	if (node != nullptr) logbatch.emplace_back(1_ui8, std::format("Code: '{}'", node->getText()));

	// Log the batch of messages.
	log::batch(log::script, logbatch);

	// Send the log messages to the server.
	auto server = BabyDI::Get<Server>();
	std::ranges::for_each(logbatch, [&server](const auto& kvp) { server->sendToNC(kvp.second); });

	if (abort) throw std::runtime_error("Terminating GS1 script.");
}

////////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitProgram(GS1Parser::ProgramContext* ctx)
{
	for (auto node : ctx->children)
	{
		try
		{
			node->accept(this);
		}
		// If we get the following exceptions in the block, just continue on to the next statement.
		catch (const break_exception&) {}
		catch (const continue_exception&) {}
		catch (const return_exception&) {}
		// Sleeps stop execution.
		catch (const sleep_exception&)
		{
			throw;
		}
		// Anything else also stops execution.
		catch (...)
		{
			throw;
		}
	}

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
	if ((bool)getReadOnlyGameValueFromAny(visit(context->expression())))
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

	// Condition.
	size_t loopCount = 0;
	while ((loopCount++ < MAX_LOOPS && (bool)getReadOnlyGameValueFromAny(safeVisit(context->expression(0)))) || enterLoopAfterSleep)
	{
		enterLoopAfterSleep = false;

		// Block.
		try
		{
			visit(context->block());
		}
		catch (const break_exception&) { break; }
		catch (const continue_exception&) { continue; }
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
	while ((loopCount++ < MAX_LOOPS && (bool)getReadOnlyGameValueFromAny(visit(context->expression()))) || enterLoopAfterSleep)
	{
		enterLoopAfterSleep = false;

		// Block.
		try
		{
			visit(context->block());
		}
		catch (const break_exception&) { break; }
		catch (const continue_exception&) { continue; }
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
	auto scriptObject = getSourceFromGS1ScriptValue(value);

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
	auto function = m_parser->userFunctions.find(identifier);
	if (function != m_parser->userFunctions.end())
	{
		try
		{
			visit(function->second);
		}
		catch (return_exception&)
		{
		}
		return {};
	}

	// Try to call the function in our joined classes.
	ScriptEvent eventCopy = *m_event;
	for (auto script : getJoinedClassesFromSource(m_originalSource))
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
	SetAndRestore sar{ expectingTimeoutAsVariable, true };

	auto results = visitChildrenAndCollect(context);
	if (results.size() != 2 || context->children.size() != 3)
		throw std::runtime_error("AssignmentOperation is not a binary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("AssignmentOperation has no operation");

	auto left = getGameVariableFromAny(results[0]);
	auto right = getReadOnlyGameValueFromAny(results[1]);

	// Do the assignment operation separately as everything else runs on doubles.
	if (op.value() == GS1Parser::OP_ASSIGN)
	{
		if (left.second.has_value())
			left.first.assign<double>(right, left.second);
		else
		{
			if (auto vec = right.get_unsafe<std::vector<double>>(); vec != nullptr)
				left.first.assign<std::vector<double>>(right);
			else left.first.assign<double>(right);
		}

		// Special case for "timeout" to erase any existing sleep call stack.
		if (left.first.identifier == "timeout")
		{
			m_sleepCallStack.clear();
			m_sleepCurrentSource.clear();
		}

		return {};
	}

	double leftD = left.first.get<double>(left.second).value_or(0.0);
	double rightD = right.get<double>().value_or(0.0);

	// Perform the operation.
	switch (op.value())
	{
		case GS1Parser::OP_ASSIGN_ADD:
			left.first.assign(leftD + rightD, left.second);
			break;
		case GS1Parser::OP_ASSIGN_SUB:
			left.first.assign(leftD - rightD, left.second);
			break;
		case GS1Parser::OP_ASSIGN_MUL:
			left.first.assign(leftD * rightD, left.second);
			break;
		case GS1Parser::OP_ASSIGN_DIV:
			left.first.assign(leftD / rightD, left.second);
			break;
		case GS1Parser::OP_ASSIGN_MOD:
			left.first.assign(static_cast<double>(static_cast<int64_t>(leftD) % static_cast<int64_t>(rightD)), left.second);
			break;
		case GS1Parser::OP_ASSIGN_POW:
			left.first.assign(std::pow(leftD, rightD), left.second);
			break;
	}

	// Assignment operations are statements and can't be used inside expressions.
	return {};
}

////////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitExpressionIn(GS1Parser::ExpressionInContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	SetAndRestore sar{ expectingTimeoutAsVariable, true };

	std::vector<double> values;
	for (auto& be : context->exponentiationExpression())
		values.emplace_back(getReadOnlyGameValueFromAnyAs<double>(visit(be)));

	std::any right_any;
	if (context->primaryExpression() != nullptr)
		right_any = visit(context->primaryExpression());
	else right_any = visit(context->range_literal());

	auto* right_range = std::any_cast<std::pair<std::any, std::any>>(&right_any);
	auto right_value = getReadOnlyGameValueFromAny(right_any);
	auto* right_vector = right_value.get_unsafe<std::vector<double>>();

	size_t range_op_left = GS1Parser::TOKEN_PIPE;
	size_t range_op_right = GS1Parser::TOKEN_PIPE;
	if (right_range != nullptr)
	{
		range_op_left = getSymbolType(context->range_literal()->children[0]).value_or(GS1Parser::TOKEN_PIPE);
		range_op_right = getSymbolType(context->range_literal()->children[4]).value_or(GS1Parser::TOKEN_PIPE);
	}
	// Check for an early exit.
	else if (right_vector == nullptr)
		return std::make_any<GS1ScriptValue>(false);

	bool range_met = true;
	for (const auto& check : values)
	{
		if (right_range != nullptr)
		{
			double first = getReadOnlyGameValueFromAnyAs<double>(right_range->first);
			double second = getReadOnlyGameValueFromAnyAs<double>(right_range->second);
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
			bool in_range = test_left && test_right;
			range_met = range_met && in_range;
		}
		else
		{
			range_met = range_met && (std::ranges::contains(*right_vector, check));
		}

		// Early out if we already know the result.
		if (!range_met)
			break;
	}

	return std::make_any<GS1ScriptValue>(range_met);
}

std::any GS1Visitor::visitExpressionTernary(GS1Parser::ExpressionTernaryContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	std::any result = visit(context->logicalOrExpression());
	for (size_t i = 1; i < context->children.size(); i += 4)
	{
		if ((bool)getReadOnlyGameValueFromAny(result))
			result = std::move(visit(context->children[i + 1]));
		else result = std::move(visit(context->children[i + 3]));
	}
	return result;
}

std::any GS1Visitor::visitExpressionLogicOr(GS1Parser::ExpressionLogicOrContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	auto left = (bool)getReadOnlyGameValueFromAny(visit(context->logicalAndExpression(0)));
	if (left) return std::make_any<GS1ScriptValue>(true);

	for (size_t i = 2; i < context->children.size(); i += 2)
	{
		auto right = (bool)getReadOnlyGameValueFromAny(visit(context->children[i]));
		if (right) return std::make_any<GS1ScriptValue>(true);
	}

	return std::make_any<GS1ScriptValue>(false);
}

std::any GS1Visitor::visitExpressionLogicAnd(GS1Parser::ExpressionLogicAndContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	auto left = (bool)getReadOnlyGameValueFromAny(visit(context->equalityExpression(0)));
	if (!left) return std::make_any<GS1ScriptValue>(false);

	for (size_t i = 2; i < context->children.size(); i += 2)
	{
		auto right = (bool)getReadOnlyGameValueFromAny(visit(context->children[i]));
		if (!right) return std::make_any<GS1ScriptValue>(false);
	}

	return std::make_any<GS1ScriptValue>(true);
}

std::any GS1Visitor::visitExpressionEquality(GS1Parser::ExpressionEqualityContext* context)
{
	if (context->children.size() < 3)
		return visitChildren(context);

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("ExpressionEquality does not have an operator");

	SetAndRestore sar{ expectingTimeoutAsVariable, true };

	auto left = getReadOnlyGameValueFromAny(visit(context->children[0]));
	auto right = getReadOnlyGameValueFromAny(visit(context->children[2]));

	auto* left_vector = left.get_unsafe<std::vector<double>>();
	auto* right_vector = right.get_unsafe<std::vector<double>>();

	// Vector equality checks.
	if (left_vector != nullptr && right_vector != nullptr)
	{
		switch (op.value())
		{
			case GS1Parser::OP_EQUAL:
			case GS1Parser::OP_ASSIGN:
				return std::make_any<GS1ScriptValue>(*left_vector == *right_vector);
			case GS1Parser::OP_NOTEQ:
				return std::make_any<GS1ScriptValue>(*left_vector != *right_vector);
		}
	}

	// Otherwise, we compare the doubles.
	auto left_double = left.get<double>().value_or(0.0);
	auto right_double = right.get<double>().value_or(0.0);

	// Do the comparison.
	switch (op.value())
	{
		case GS1Parser::OP_EQUAL:
		case GS1Parser::OP_ASSIGN:
			return std::make_any<GS1ScriptValue>(DoublesAreSame(left_double, right_double));
		case GS1Parser::OP_NOTEQ:
			return std::make_any<GS1ScriptValue>(!DoublesAreSame(left_double, right_double));
	}

	throw std::runtime_error("ExpressionEquality has an unknown operator");
}

std::any GS1Visitor::visitExpressionRelational(GS1Parser::ExpressionRelationalContext* context)
{
	if (context->children.size() < 3)
		return visitChildren(context);

	SetAndRestore sar{ expectingTimeoutAsVariable, true };

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("ExpressionRelational does not have an operator");

	auto left = getReadOnlyGameValueFromAnyAs<double>(visit(context->children[0]));
	auto right = getReadOnlyGameValueFromAnyAs<double>(visit(context->children[2]));

	// Do the comparison.
	switch (op.value())
	{
		case GS1Parser::OP_LESS:
			return std::make_any<GS1ScriptValue>(left < right);
		case GS1Parser::OP_GREAT:
			return std::make_any<GS1ScriptValue>(left > right);
		case GS1Parser::OP_LESS_EQ:
			return std::make_any<GS1ScriptValue>(left <= right);
		case GS1Parser::OP_GREAT_EQ:
			return std::make_any<GS1ScriptValue>(left >= right);
	}

	throw std::runtime_error("ExpressionRelational has an unknown operator");
}

std::any GS1Visitor::visitExpressionAdditive(GS1Parser::ExpressionAdditiveContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	SetAndRestore sar{ expectingTimeoutAsVariable, true };

	double result = getReadOnlyGameValueFromAnyAs<double>(visit(context->children[0]));
	for (size_t i = 1; i < context->children.size(); i += 2)
	{
		auto op = getSymbolType(context->children[i]);
		if (!op.has_value())
			continue;

		auto right = getReadOnlyGameValueFromAnyAs<double>(visit(context->children[i + 1]));
		if (op.value() == GS1Parser::OP_ADD)
			result += right;
		else if (op.value() == GS1Parser::OP_SUB)
			result -= right;
	}

	return std::make_any<GS1ScriptValue>(result);
}

std::any GS1Visitor::visitExpressionMultiplicative(GS1Parser::ExpressionMultiplicativeContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	SetAndRestore sar{ expectingTimeoutAsVariable, true };

	double result = getReadOnlyGameValueFromAnyAs<double>(visit(context->children[0]));
	for (size_t i = 1; i < context->children.size(); i += 2)
	{
		auto op = getSymbolType(context->children[i]);
		if (!op.has_value())
			continue;

		auto right = getReadOnlyGameValueFromAnyAs<double>(visit(context->children[i + 1]));
		if (op.value() == GS1Parser::OP_MUL)
			result *= right;
		else if (op.value() == GS1Parser::OP_DIV)
			result /= right;
		else if (op.value() == GS1Parser::OP_MOD)
			result = static_cast<double>(static_cast<int64_t>(result) % static_cast<int64_t>(right));
	}

	return std::make_any<GS1ScriptValue>(result);
}

std::any GS1Visitor::visitExpressionExponentiation(GS1Parser::ExpressionExponentiationContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);

	SetAndRestore sar{ expectingTimeoutAsVariable, true };

	double result = getReadOnlyGameValueFromAnyAs<double>(visit(context->children[0]));
	for (size_t i = 1; i < context->children.size(); i += 2)
	{
		auto op = getSymbolType(context->children[i]);
		if (!op.has_value())
			continue;

		auto right = getReadOnlyGameValueFromAnyAs<double>(visit(context->children[i + 1]));
		result = std::pow(result, right);
	}

	return std::make_any<GS1ScriptValue>(result);
}

std::any GS1Visitor::visitExpressionUnary(GS1Parser::ExpressionUnaryContext* context)
{
	auto op = getSymbolType(context->children[0]);
	if (!op.has_value())
		throw std::runtime_error("ExpressionUnary does not have an operator");

	if (op.value() == GS1Parser::OP_LOGICALNOT)
		return std::make_any<GS1ScriptValue>(DoubleIsZero(getReadOnlyGameValueFromAnyAs<double>(visit(context->unaryExpression()))));

	if (op.value() == GS1Parser::OP_SUB)
	{
		SetAndRestore sar{ expectingTimeoutAsVariable, true };
		return std::make_any<GS1ScriptValue>(-getReadOnlyGameValueFromAnyAs<double>(visit(context->unaryExpression())));
	}

	return visit(context->unaryExpression());
}

std::any GS1Visitor::visitExpressionPostfix(GS1Parser::ExpressionPostfixContext* context)
{
	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("ExpressionPostfix has no operation");

	SetAndRestore sar{ expectingTimeoutAsVariable, true };

	auto anyval = visit(context->children[0]);
	auto left = getGameVariableFromAny(anyval);
	auto value = left.first.get<double>(left.second).value_or(0.0);

	// Perform the operation.
	switch (op.value())
	{
		case GS1Parser::OP_INC:
			left.first.assign(value + 1.0, left.second);
			break;
		case GS1Parser::OP_DEC:
			left.first.assign(value - 1.0, left.second);
			break;
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
	auto objectSource = getSourceFromGS1ScriptValue(value);
	if (!objectSource.has_value())
		RECOVERABLE_PARSE_ERROR(std::format("Identifier did not contain a script object: {}.", context->children[0]->getText()), 0.0);

	size_t pos = 1;
	size_t identifierCount = context->identifier_value().size();

	// Iterate through the identifier values, adjusting our current source object as we go.
	do
	{
		// Temporarily push the current source onto the stack and get the next identifier value.
		// We don't need to keep it on the stack so pop it after we're done.
		m_currentSource.push_back(objectSource.value());
		{
			first = std::move(visit(context->identifier_value(pos++)));
		}
		m_currentSource.pop_back();

		// Check if the result is a ScriptObject.
		value = getGS1ScriptValueFromAny(first);
		objectSource = getSourceFromGS1ScriptValue(value);

		// If not, we might be done.
		if (!objectSource.has_value())
		{
			if (pos > identifierCount)
				throw std::runtime_error("IdentifierAccess has no valid identifier value.");
			return std::make_any<GS1ScriptValue>(std::move(value));
		}
	}
	while (pos < identifierCount);

	// If we made it here somehow, just return an empty GS1ScriptValue.
	return std::make_any<GS1ScriptValue>(0.0);
}

std::any GS1Visitor::visitIdentifierValue(GS1Parser::IdentifierValueContext* context)
{
	std::optional<size_t> storage = std::nullopt;
	if (context->storage_token() != nullptr)
	{
		visit(context->storage_token());
		storage = getSymbolType(context->storage_token());
	}

	auto identifier_any = visit(context->compound_identifier());
	auto* identifier = std::any_cast<std::string>(&identifier_any);
	if (identifier == nullptr)
		throw std::runtime_error("IdentifierValue has no valid compound_identifier");

	auto conditionalExpressions = context->conditionalExpression();
	std::optional<int64_t> index = std::nullopt;

	// Test for tiles[x,y].
	// Since tiles[x,y] is a unique case, we encode the index with the X/Y.
	if (*identifier == "tiles" && conditionalExpressions.size() == 2)
	{
		auto param1 = visit(conditionalExpressions[0]);
		auto param2 = visit(conditionalExpressions[1]);
		auto x = static_cast<uint32_t>(std::max(0.0, getReadOnlyGameValueFromAnyAs<double>(param1)));
		auto y = static_cast<uint32_t>(std::max(0.0, getReadOnlyGameValueFromAnyAs<double>(param2)));
		index = (static_cast<size_t>(x) << 32) | y;
	}
	else if (conditionalExpressions.size() == 1)
	{
		// Get the array index.
		auto expression_any = visit(conditionalExpressions[0]);
		index = static_cast<int64_t>(getReadOnlyGameValueFromAnyAs<double>(expression_any));
	}

	// If we have an identifier, and the flag store has a matching value, return that.
	if (!identifier->empty() && flagStore.contains(*identifier))
	{
		// Timeout is annoying, so make sure we are not doing something that needs the NPC timeout.
		if (*identifier != "timeout" || !expectingTimeoutAsVariable)
		{
			if (auto flag = flagStore.get(*identifier).lock(); flag != nullptr)
				return std::make_any<GS1ScriptValue>(GameValue{ flag->get<bool>().value_or(false) });
		}
	}

	// Append the storage modifier to certain variable names.
	// This is because they have special considerations.
	if (storage.has_value())
		applyStorageToIdentifier(storage.value(), *identifier);

	// If we have no storage value, and we are expecting a flag, force client storage.
	if (!storage.has_value() && expectingFlag)
		storage = GS1Parser::STORAGE_CLIENT;

	// Get the game variable store for the identifier.
	// If there is no storage type, it pulls from the built-in variable store (saved on the script context).
	auto variable = getGameValueFromStorage(*identifier, storage);
	{
		// If it is temp storage, make sure the variable is marked as temporary so it isn't saved.
		if (storage.value_or(GS1Parser::STORAGE_THIS) == GS1Parser::STORAGE_TEMP)
			variable.temporary = true;

		return std::make_any<GS1ScriptValue>(std::make_pair(variable, index));
	}

	// Return a default value if the identifier is not found.
	return std::make_any<GS1ScriptValue>(GameValue{ 0.0 });
}

std::any GS1Visitor::visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context)
{
	std::string compoundIdentifier;

	// Temporarily turn off the flag expectation while we build the final identifier.
	// This allows things like server.player_#v(playerid) to read from the correct storage area.
	bool oldExpectingFlag = expectingFlag;
	expectingFlag = false;

	for (auto& tree : context->children)
	{
		if (tree->getTreeType() == antlr4::tree::ParseTreeType::TERMINAL)
			compoundIdentifier.append(tree->getText());
		else
		{
			auto piece = tree->accept(this);
			compoundIdentifier.append(getReadOnlyGameValueFromAnyAs<std::string>(piece));
		}
	}

	expectingFlag = oldExpectingFlag;

	string::trimMutate(compoundIdentifier);
	return std::make_any<std::string>(compoundIdentifier);
}

std::any GS1Visitor::visitCompoundString(GS1Parser::CompoundStringContext* context)
{
	std::string compoundString;

	for (auto& tree : context->children)
	{
		if (tree->getTreeType() == antlr4::tree::ParseTreeType::TERMINAL)
			compoundString.append(tree->getText());
		else
		{
			auto piece = tree->accept(this);
			if (auto* gs1Val = std::any_cast<GS1ScriptValue>(&piece); gs1Val != nullptr)
			{
				// If this is a GS1GameVariable and the results size is 1, just return the piece.
				if (auto* gs1GameVariable = std::get_if<GS1GameVariable>(gs1Val); gs1GameVariable != nullptr && context->children.size() == 1)
					return piece;
			}
			compoundString.append(getReadOnlyGameValueFromAnyAs<std::string>(piece));
		}
	}

	string::trimMutate(compoundString);
	return std::make_any<GS1ScriptValue>(compoundString);
}

std::any GS1Visitor::visitMessageCode(GS1Parser::MessageCodeContext* context)
{
	auto results = visitChildrenAndCollect(context);
	auto messageCode = context->MESSAGECODE()->getText();
	if (messageCode.empty())
		RECOVERABLE_PARSE_ERROR(std::format("Message code '{}' is not a valid message code.", messageCode), ""s);

	// Trim out the message code.
	std::string_view messageCodeView{ messageCode };
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
		return GS1ScriptValue{ ""s };
	}
	catch (const std::logic_error& e)
	{
		reportError(e.what(), context, false);
		return GS1ScriptValue{ ""s };
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
		auto text = context->LITERAL()->getText();
		if (text == "true") return std::make_any<GS1ScriptValue>(true);
		if (text == "false") return std::make_any<GS1ScriptValue>(false);
		return std::make_any<GS1ScriptValue>(std::stod(text));
	}
	else if (context->ALLFEATURES() != nullptr)
		return std::make_any<GS1ScriptValue>(static_cast<double>(0xFFFF));
	else if (context->ALLSTATS() != nullptr)
		return std::make_any<GS1ScriptValue>(static_cast<double>(0xFFFF));

	return 0.0;
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
	for (size_t i = 0; i < context->children.size(); ++i)
	{
		auto child = context->children[i];
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
			values.push_back(getReadOnlyGameValueFromAnyAs<double>(result));
		}
	}

	// This covers the case of {} and {1,}, which should result in {0} and {1,0}, respectively.
	if (valueIndex == values.size())
		values.push_back(0.0);

	return std::make_any<GS1ScriptValue>(std::move(values));
}

std::any GS1Visitor::visitItemLiteral(GS1Parser::ItemLiteralContext* context)
{
	auto text = context->ITEM()->getText();
	auto it = std::ranges::find(ItemNames, text);
	if (it == ItemNames.end())
		it = ItemNames.begin();

	return std::make_any<GS1ScriptValue>(static_cast<double>(std::distance(ItemNames.begin(), it)));
}

std::any GS1Visitor::visitCarryLiteral(GS1Parser::CarryLiteralContext* context)
{
	auto text = context->CARRY()->getText();
	auto it = std::ranges::find(carryNames, text);
	if (it == carryNames.end())
		it = carryNames.begin();

	return std::make_any<GS1ScriptValue>(static_cast<double>(std::distance(carryNames.begin(), it)));
}

std::any GS1Visitor::visitDirectionLiteral(GS1Parser::DirectionLiteralContext* context)
{
	ptrdiff_t index = 0;
	auto text = context->DIRECTION()->getText();
	if (auto it = std::ranges::find(directionNames, text); it != directionNames.end())
		index = std::distance(directionNames.begin(), it);
	else
	{
		index = static_cast<ptrdiff_t>(string::toNumber(text));
		index = index % 4;
	}

	return std::make_any<GS1ScriptValue>(static_cast<double>(index));
}

std::any GS1Visitor::visitGenderLiteral(GS1Parser::GenderLiteralContext* context)
{
	auto text = context->GENDER()->getText();
	auto it = std::ranges::find(genderNames, text);
	if (it == genderNames.end())
		it = genderNames.begin();

	return std::make_any<GS1ScriptValue>(static_cast<double>(std::distance(genderNames.begin(), it)));
}

std::any GS1Visitor::visitColorLiteral(GS1Parser::ColorLiteralContext* context)
{
	return std::make_any<GS1ScriptValue>(getColorValueFromString(context->COLOR()->getText()));
}

std::any GS1Visitor::visitBaddyLiteral(GS1Parser::BaddyLiteralContext* context)
{
	auto text = context->BADDY()->getText();
	auto it = std::ranges::find(BaddyNames, text);
	if (it == BaddyNames.end())
		it = BaddyNames.begin();

	return std::make_any<GS1ScriptValue>(static_cast<double>(std::distance(BaddyNames.begin(), it)));
}

std::any GS1Visitor::visitStorageToken(GS1Parser::StorageTokenContext* context)
{
	return visitChildren(context);
}

std::any GS1Visitor::visitPrimaryExpression(GS1Parser::PrimaryExpressionContext* context)
{
	if (context->children.size() == 1)
		return visitChildren(context);
	return visit(context->expression());
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
