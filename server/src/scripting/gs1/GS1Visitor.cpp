#include <algorithm>
#include <any>
#include <cmath>
#include <cstdint>
#include <exception>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <BabyDI.h>
#include <GS1Parser.h>
#include <tree/ParseTree.h>
#include <tree/TerminalNode.h>

#include <Server.h>
#include <level/Level.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <scripting/gs1/GS1Commands.h>
#include <scripting/gs1/GS1Functions.h>
#include <scripting/gs1/GS1MessageCodes.h>
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

constexpr size_t MAX_LOOPS = 10000;

///////////////////////////////////////////////////////////////////////////////

struct break_exception : public std::exception {};
struct continue_exception : public std::exception {};
struct return_exception : public std::exception {};

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

static GameVariableStore* getGameVariableStoreFromSource(ScriptObjectSource source)
{
	static GameVariableStore invalidStore;

	auto* server = BabyDI::Get<Server>();
	switch (source.second)
	{
		case ScriptObjectSourceType::PLAYER:
			if (auto player = server->getPlayer(source.first); player != nullptr)
				return &player->account.variables;
			break;
		case ScriptObjectSourceType::NPC:
			if (auto npc = server->getNPC(source.first); npc != nullptr)
				return &npc->scripting.variables;
			break;
		case ScriptObjectSourceType::LEVEL:
		{
			auto& levelList = server->getLevelList();
			if (auto it = levelList.find(source.first); it != levelList.end())
				return &it->second->scripting.variables;
			log::printLine(log::script, "Could not find level for source.");
			return &invalidStore;
		}
		case ScriptObjectSourceType::SERVER:
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

static void applyStorageToIdentifier(std::optional<size_t> storage, std::string& identifier)
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
// Static member functions.


///////////////////////////////////////////////////////////////////////////////
// Public member functions.

GameVariable* GS1Visitor::getGameVariableFromGS1ScriptValue(GS1ScriptValue& value)
{
	if (auto* gs1GameVariable = std::get_if<GS1GameVariable>(&value); gs1GameVariable != nullptr)
	{
		auto* gameVariable = getGameVariableFromVariant(gs1GameVariable->first);
		if (gameVariable != nullptr)
			return gameVariable;
	}
	return nullptr;
}

GameVariable* GS1Visitor::getGameVariableFromVariant(GameVariableVariant& variant)
{
	if (auto* byVal = std::get_if<GameVariable>(&variant); byVal != nullptr)
		return byVal;
	else if (auto* byPtr = std::get_if<std::weak_ptr<GameVariable>>(&variant); byPtr != nullptr)
	{
		if (auto lock = byPtr->lock(); lock != nullptr)
			return lock.get();
	}
	return nullptr;
}

std::optional<GameVariable> GS1Visitor::getGameVariableFromSource(const ScriptObjectSource& source, std::string_view identifier)
{
	auto* server = BabyDI::Get<Server>();
	IScriptObject* scriptObject = nullptr;

	switch (source.second)
	{
		case ScriptObjectSourceType::PLAYER:
			if (auto player = server->getPlayer(source.first); player != nullptr)
				scriptObject = player.get();
			break;
		case ScriptObjectSourceType::NPC:
			if (auto npc = server->getNPC(source.first); npc != nullptr)
				scriptObject = npc.get();
			break;
	}

	if (scriptObject != nullptr)
		return scriptObject->getScriptObjectParameter(identifier);

	return std::nullopt;
}

GameVariableVariant GS1Visitor::getGameVariableFromStorage(std::string_view identifier, std::optional<size_t> type)
{
	// If we have a specific storage type, try to get the store for it.
	if (type.has_value())
	{
		if (auto* store = getGameVariableStoreForStorageType(type.value()); store != nullptr)
			return store->get_or_stub(identifier);
	}

	std::weak_ptr<GameVariable> builtIn;
	auto mergeWithBuiltInBoolean = [](std::shared_ptr<GameVariable>& existing, std::shared_ptr<GameVariable>& builtIn) -> int
	{
		// No built-in variable, return 0: stub built-in.
		if (builtIn == nullptr)
			return 0;
		// Built-in variable is not a boolean, or we have no existing, return 1: use built-in.
		if (!builtIn->has<bool>() || existing == nullptr)
			return 1;
		// Merge the boolean value from the built-in variable into the existing variable, return 2: use existing.
		existing->assign(builtIn->get<bool>().value_or(false), std::nullopt);
		return 2;
	};

	// First, try to get a built-in variable.
	if (builtInStore != nullptr)
		builtIn = builtInStore->get(identifier);

	// Lock the built-in variable.
	auto builtInResult = builtIn.lock();

	// Look in the current source's store.
	if (auto* currentStore = getGameVariableStoreFromSource(getCurrentSource()); currentStore != nullptr)
	{
		// For the current store, we only want to return a variable if it exists.
		// If the variable does not exist, it will be added to the source store.
		if (currentStore->contains(identifier))
		{
			// If we have a built-in variable, and the current store had a boolean version of it, add the boolean version to the result.
			auto currentStoreResult = currentStore->get(identifier).lock();
			switch (mergeWithBuiltInBoolean(currentStoreResult, builtInResult))
			{
				case 0: return currentStore->get_or_stub(identifier);
				case 1: return builtInResult;
				case 2: return currentStoreResult;
			}
		}
		else if (auto property = getGameVariableFromSource(getCurrentSource(), identifier); property.has_value())
			return property.value().update();
	}

	GameVariableStore* targetStoreForStub = nullptr;

	// Now look in the original source's store.
	{
		if (auto* sourceStore = getGameVariableStoreFromSource(getOriginalSource()); sourceStore != nullptr)
		{
			auto sourceStoreResult = sourceStore->get(identifier).lock();
			switch (mergeWithBuiltInBoolean(sourceStoreResult, builtInResult))
			{
				case 0: break;
				case 1: return builtInResult;
				case 2: return sourceStoreResult;
			}
		}
		if (targetStoreForStub == nullptr)
		{
			if (auto property = getGameVariableFromSource(getOriginalSource(), identifier); property.has_value())
				return property.value().update();
		}
	}

	// Now look at the initiator's store.
	if (m_event->initiator != getOriginalSource())
	{
		if (auto* initiatorStore = getGameVariableStoreFromSource(m_event->initiator); initiatorStore != nullptr)
		{
			auto initiatorStoreResult = initiatorStore->get(identifier).lock();
			switch (mergeWithBuiltInBoolean(initiatorStoreResult, builtInResult))
			{
				case 0: break;
				case 1: return builtInResult;
				case 2: return initiatorStoreResult;
			}
		}
		if (targetStoreForStub == nullptr)
		{
			if (auto property = getGameVariableFromSource(m_event->initiator, identifier); property.has_value())
				return property.value().update();
		}
	}

	// Stub our variable.
	if (targetStoreForStub != nullptr)
		return targetStoreForStub->get_or_stub(identifier);

	// We have a built-in variable, but our sources don't have a store, just return it.
	if (!builtIn.expired())
		return builtIn;

	// If we still don't have a store, use the built-in store.
	if (builtInStore != nullptr)
		return builtInStore->get_or_stub(identifier);

	// Still nothing?  Just return empty.
	return {};
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

std::optional<ScriptObjectSource> GS1Visitor::findNearestScriptObjectSourceFromStack(ScriptObjectSourceType type) const
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
	auto testSource = [server](const ScriptObjectSource& source) -> std::shared_ptr<Level>
	{
		if (source.second == ScriptObjectSourceType::NPC)
		{
			if (auto npc = server->getNPC(source.first); npc != nullptr)
				return npc->level.lock();
		}
		else if (source.second == ScriptObjectSourceType::PLAYER)
		{
			if (auto player = server->getPlayer(source.first); player != nullptr)
				return server->getLevel(player->account.level);
		}
		else if (source.second == ScriptObjectSourceType::LEVEL)
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

GameVariableStore* GS1Visitor::findGameVariableStoreFromSourceStack(ScriptObjectSourceType type) const
{
	for (const auto& source : sourceStack())
	{
		if (source.second == type)
			return getGameVariableStoreFromSource(source);
	}
	return nullptr;
}

GameVariableStore* GS1Visitor::getGameVariableStoreForStorageType(size_t type)
{
	GameVariableStore* store = nullptr;
	switch (type)
	{
		case GS1Parser::STORAGE_THIS:
		case GS1Parser::STORAGE_LOCAL:
		case GS1Parser::STORAGE_TEMP:
			store = findGameVariableStoreFromSourceStack(ScriptObjectSourceType::NPC);
			break;
		case GS1Parser::STORAGE_THISO:
			store = getGameVariableStoreFromSource(m_originalSource);
			break;
		case GS1Parser::STORAGE_CLIENT:
		case GS1Parser::STORAGE_CLIENTR:
		case GS1Parser::STORAGE_CLIENTO:	// Not supported yet.  GR extension.
		case GS1Parser::STORAGE_CLIENTRO:	// Not supported yet.  GR extension.
			store = findGameVariableStoreFromSourceStack(ScriptObjectSourceType::PLAYER);
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
				[&server](PlayerPtr& player) -> LevelPtr { return server->getLevel(player->account.level); },
				[&server](NPCPtr& npc) -> LevelPtr { return npc->level.lock(); }
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
		const GameValue* gameValue = nullptr;

		if (auto* byVal = std::get_if<GameVariable>(&gs1GameVariable->first); byVal != nullptr)
			gameValue = &byVal->get_underlying();
		else if (auto* byPtr = std::get_if<std::weak_ptr<GameVariable>>(&gs1GameVariable->first); byPtr != nullptr)
		{
			if (auto var = byPtr->lock(); var != nullptr)
				gameValue = &var->get_underlying();
		}

		if (gameValue != nullptr)
		{
			if (!gs1GameVariable->second.has_value())
				return *gameValue;
			return GameValue{ gameValue->get<double>(gs1GameVariable->second.value()).value_or(0.0) };
		}
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

///////////////////////////////////////////////////////////////////////////////

void GS1Visitor::execute(const ScriptEvent& event, ScriptObjectSource source, GS1Parser& parser, antlr4::tree::ParseTree& startNode)
{
	m_parser = &parser;
	m_event = &event;
	m_originalSource = source;

	m_serverStore = getGameVariableStoreFromSource(source::FromServer());

	// Execute!
	visit(&startNode);
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

///////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitMathExpression(GS1Parser::MathExpressionContext* context)
{
	auto results = visitChildrenAndCollect(context);
	if (results.size() != 2)
		throw std::runtime_error("MathExpression is not a binary expression");

	auto op = dynamic_cast<antlr4::tree::TerminalNode*>(context->children[1]);
	if (op == nullptr)
		throw std::runtime_error("MathExpression does not have an operator");

	auto left = getReadOnlyGameValueFromAnyAs<double>(results[0]);
	auto right = getReadOnlyGameValueFromAnyAs<double>(results[1]);

	switch (op->getSymbol()->getType())
	{
		case GS1Parser::OP_POW:
			return std::make_any<GS1ScriptValue>(std::pow(left, right));
		case GS1Parser::OP_MUL:
			return std::make_any<GS1ScriptValue>(left * right);
		case GS1Parser::OP_DIV:
			return std::make_any<GS1ScriptValue>(left / right);
		case GS1Parser::OP_MOD:
			return std::make_any<GS1ScriptValue>(static_cast<double>(static_cast<int64_t>(left) % static_cast<int64_t>(right)));
		case GS1Parser::OP_ADD:
			return std::make_any<GS1ScriptValue>(left + right);
		case GS1Parser::OP_SUB:
			return std::make_any<GS1ScriptValue>(left - right);
	}

	throw std::runtime_error("MathExpression has an unknown operator");
}

std::any GS1Visitor::visitComparisonExpression(GS1Parser::ComparisonExpressionContext* context)
{
	auto results = visitChildrenAndCollect(context);
	if (results.size() != 2)
		throw std::runtime_error("ComparisonExpression is not a binary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("ComparisonExpression does not have an operator");

	auto left = getReadOnlyGameValueFromAny(results[0]);
	auto right = getReadOnlyGameValueFromAny(results[1]);

	auto* left_vector = left.get_unsafe<std::vector<double>>();
	auto* right_vector = right.get_unsafe<std::vector<double>>();

	// Vector equality checks.
	if (left_vector != nullptr && right_vector != nullptr)
	{
		switch (op.value())
		{
			case GS1Parser::OP_EQUAL:
			case GS1Parser::OP_ASSIGN:
				return std::make_any<GS1ScriptValue>((*left_vector == *right_vector) ? 1.0 : 0.0);
			case GS1Parser::OP_NOTEQ:
				return std::make_any<GS1ScriptValue>((*left_vector != *right_vector) ? 1.0 : 0.0);
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
			return std::make_any<GS1ScriptValue>((left_double == right_double) ? 1.0 : 0.0);
		case GS1Parser::OP_NOTEQ:
			return std::make_any<GS1ScriptValue>((left_double != right_double) ? 1.0 : 0.0);
		case GS1Parser::OP_LESS:
			return std::make_any<GS1ScriptValue>((left_double < right_double) ? 1.0 : 0.0);
		case GS1Parser::OP_GREAT:
			return std::make_any<GS1ScriptValue>((left_double > right_double) ? 1.0 : 0.0);
		case GS1Parser::OP_LESS_EQ:
			return std::make_any<GS1ScriptValue>((left_double <= right_double) ? 1.0 : 0.0);
		case GS1Parser::OP_GREAT_EQ:
			return std::make_any<GS1ScriptValue>((left_double >= right_double) ? 1.0 : 0.0);
	}

	throw std::runtime_error("ComparisonExpression has an unknown operator");
}

std::any GS1Visitor::visitLogicExpression(GS1Parser::LogicExpressionContext* context)
{
	if (context->children.size() != 3)
		throw std::runtime_error("LogicExpression does not have 3 children");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("LogicExpression does not have an operator");

	bool left = (bool)getReadOnlyGameValueFromAny(visit(context->binary_expression(0)));

	// Early outs if we know the result already.
	if (op.value() == GS1Parser::OP_LOGICALAND && left == false)
		return std::make_any<GS1ScriptValue>(0.0);
	if (op.value() == GS1Parser::OP_LOGICALOR && left == true)
		return std::make_any<GS1ScriptValue>(1.0);

	// Return the result of the second expression.
	bool right = (bool)getReadOnlyGameValueFromAny(visit(context->binary_expression(1)));
	return std::make_any<GS1ScriptValue>(right ? 1.0 : 0.0);
}

std::any GS1Visitor::visitTernaryExpression(GS1Parser::TernaryExpressionContext* context)
{
	if ((bool)getReadOnlyGameValueFromAny(visit(context->binary_expression(0))))
		return visit(context->binary_expression(1));
	return visit(context->binary_expression(2));
}

std::any GS1Visitor::visitInExpression(GS1Parser::InExpressionContext* context)
{
	std::vector<double> values;
	for (auto& be : context->binary_expression())
		values.emplace_back(getReadOnlyGameValueFromAnyAs<double>(visit(be)));

	auto right_any = visit(context->in_expression());
	auto* right_range = std::any_cast<std::pair<std::any, std::any>>(&right_any);
	auto right_value = getReadOnlyGameValueFromAny(right_any);
	auto* right_vector = right_value.get_unsafe<std::vector<double>>();

	size_t range_op_left = GS1Parser::TOKEN_PIPE;
	size_t range_op_right = GS1Parser::TOKEN_PIPE;
	if (right_range != nullptr)
	{
		range_op_left = getSymbolType(context->in_expression()->range_literal()->children[0]).value_or(GS1Parser::TOKEN_PIPE);
		range_op_right = getSymbolType(context->in_expression()->range_literal()->children[4]).value_or(GS1Parser::TOKEN_PIPE);
	}
	// Check for an early exit.
	else if (right_vector == nullptr)
		return std::make_any<GS1ScriptValue>(0.0);

	bool range_met = true;
	for (const auto& check : values)
	{
		if (right_range != nullptr)
		{
			double first = getReadOnlyGameValueFromAnyAs<double>(right_range->first);
			double second = getReadOnlyGameValueFromAnyAs<double>(right_range->second);
			bool test_left = (range_op_left == GS1Parser::TOKEN_PIPE) ? (first <= check) : (first < check);
			bool test_right = (range_op_right == GS1Parser::TOKEN_PIPE) ? (check <= second) : (check < second);
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

	return std::make_any<GS1ScriptValue>(range_met ? 1.0 : 0.0);
}

std::any GS1Visitor::visitParenthesesExpression(GS1Parser::ParenthesesExpressionContext* context)
{
	return visit(context->binary_expression());
}

std::any GS1Visitor::visitIdentifierAccess(GS1Parser::IdentifierAccessContext* context)
{
	auto first = visit(context->identifier_value(0));
	if (context->children.size() == 1)
	{
		// No accessors, just return the first identifier value.
		return first;
	}

	// The first identifier value should be a ScriptObjectSource.
	auto value = getGS1ScriptValueFromAny(first);
	if (!std::holds_alternative<ScriptObjectSource>(value))
		throw std::runtime_error("IdentifierAccess first identifier value is not a valid ScriptObjectSource.");

	auto* object = std::get_if<ScriptObjectSource>(&value);
	std::any result;
	size_t pos = 1;
	size_t identifierCount = context->identifier_value().size();

	// Iterate through the identifier values, adjusting our current source object as we go.
	do
	{
		// Temporarily push the current source onto the stack and get the next identifier value.
		// We don't need to keep it on the stack so pop it after we're done.
		m_currentSource.push_back(*object);
		{
			result = std::move(visit(context->identifier_value(pos++)));
		}
		m_currentSource.pop_back();

		// Check if the result is a ScriptObjectSource.
		value = getGS1ScriptValueFromAny(result);
		object = std::get_if<ScriptObjectSource>(&value);

		// If not, we might be done.
		if (object == nullptr)
		{
			if (pos == identifierCount)
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

	// Get the array index.
	std::optional<size_t> index = std::nullopt;
	if (context->unary_expression() != nullptr)
	{
		auto expression_any = visit(context->unary_expression());
		index = static_cast<size_t>(getReadOnlyGameValueFromAnyAs<double>(expression_any));
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
	auto variable = getGameVariableFromStorage(*identifier, storage);
	auto* gameVariable = getGameVariableFromVariant(variable);
	if (gameVariable != nullptr)
		return std::make_any<GS1ScriptValue>(std::make_pair(variable, index));

	// Return a default value if the identifier is not found.
	return std::make_any<GS1ScriptValue>(0.0);
}

std::any GS1Visitor::visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context)
{
	std::string compoundIdentifier;
	std::optional<size_t> index = std::nullopt;
	bool wasArray = false;

	auto results = visitChildrenAndCollect(context);
	for (auto& piece : results)
	{
		auto gs1Value = getReadOnlyGameValueFromAnyAs<std::string>(piece);
		compoundIdentifier.append(gs1Value);
	}
	string::trimMutate(compoundIdentifier);
	return std::make_any<std::string>(compoundIdentifier);
}

std::any GS1Visitor::visitCompoundString(GS1Parser::CompoundStringContext* context)
{
	std::string compoundString;

	auto results = visitChildrenAndCollect(context);
	for (auto& piece : results)
	{
		if (auto* str = std::any_cast<std::string>(&piece); str != nullptr)
		{
			compoundString.append(*str);
			continue;
		}
		if (auto* gs1Val = std::any_cast<GS1ScriptValue>(&piece); gs1Val != nullptr)
		{
			// If this is a GS1GameVariable and the results size is 1, just return the piece.
			if (auto* gs1GameVariable = std::get_if<GS1GameVariable>(gs1Val); gs1GameVariable != nullptr && results.size() == 1)
				return piece;

			compoundString.append(getReadOnlyGameValueFromGS1ScriptValue(*gs1Val).get<std::string>().value_or({}));
		}
	}
	string::trimMutate(compoundString);
	return std::make_any<GS1ScriptValue>(compoundString);
}

std::any GS1Visitor::visitIncDecOperation(GS1Parser::IncDecOperationContext* context)
{
	auto results = visitChildrenAndCollect(context);
	if (results.size() == 0 || context->children.size() != 2)
		throw std::runtime_error("IncDecOperation is not a unary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("IncDecOperation has no operation");

	auto left = getGameVariableFromAny(results[0]);
	if (auto* leftVar = getGameVariableFromVariant(left.first); leftVar != nullptr)
	{
		auto value = leftVar->get<double>(left.second).value_or(0.0);

		// Perform the operation.
		switch (op.value())
		{
			case GS1Parser::OP_INC:
				leftVar->assign(value + 1.0, left.second);
				break;
			case GS1Parser::OP_DEC:
				leftVar->assign(value - 1.0, left.second);
				break;
		}
	}

	// GS1 assignment operations are statements and can't be used inside expressions.
	// So don't return anything.
	return {};
}

std::any GS1Visitor::visitBuiltInCommand(GS1Parser::BuiltInCommandContext* context)
{
	// Get the command.
	auto command = context->COMMAND()->getText();
	string::trimRightMutate(command);

	try
	{
		// Process the built-in command.
		processBuiltInCommand(this, context, command);
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

std::any GS1Visitor::visitFunctionDefinition(GS1Parser::FunctionDefinitionContext* context)
{
	// Don't execute user functions while walking through the tree.
	return {};
}

std::any GS1Visitor::visitUserFunctionCall(GS1Parser::UserFunctionCallContext* context)
{
	if (m_parser == nullptr)
		throw std::runtime_error("GS1Visitor is missing the link to the parser");

	auto anyval = visit(context->identifier_literal());
	auto value = getReadOnlyGameValueFromAny(anyval);
	auto* identifier = value.get_unsafe<std::string>();
	if (identifier == nullptr || identifier->empty())
		throw std::runtime_error("UserFunctionCall has no valid identifier");

	auto function = m_parser->userFunctions.find(*identifier);
	if (function == m_parser->userFunctions.end())
		throw std::runtime_error("UserFunctionCall could not find user function");

	return visit(function->second);
}

std::any GS1Visitor::visitBuiltInFunctionCall(GS1Parser::BuiltInFunctionCallContext* context)
{
	auto results = visitChildrenAndCollect(context);

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

std::any GS1Visitor::visitIfCondition(GS1Parser::IfConditionContext* context)
{
	if ((bool)getReadOnlyGameValueFromAny(visit(context->expression())))
		return visit(context->block(0));
	else
		return safeVisit(context->block(1));
}

std::any GS1Visitor::visitForLoop(GS1Parser::ForLoopContext* context)
{
	// Assignment.
	safeVisit(context->assignment_operation());

	// Condition.
	size_t loopCount = 0;
	while (loopCount++ < MAX_LOOPS && (bool)getReadOnlyGameValueFromAny(safeVisit(context->expression(0))))
	{
		// Block.
		try
		{
			visit(context->block());
		}
		catch (break_exception&) { break; }
		catch (continue_exception&) { continue; }

		// Increment.
		safeVisit(context->expression(1));
	}

	return {};
}

std::any GS1Visitor::visitWhileLoop(GS1Parser::WhileLoopContext* context)
{
	// Condition.
	size_t loopCount = 0;
	while (loopCount++ < MAX_LOOPS && (bool)getReadOnlyGameValueFromAny(visit(context->expression())))
	{
		// Block.
		try
		{
			visit(context->block());
		}
		catch (break_exception&) { break; }
		catch (continue_exception&) { continue; }
	}

	return {};
}

std::any GS1Visitor::visitWithStatement(GS1Parser::WithStatementContext* context)
{
	auto expression = visit(context->expression());
	auto value = getGS1ScriptValueFromAny(expression);
	auto* object = std::get_if<ScriptObjectSource>(&value);

	// If we have a source, push it onto the source stack.
	if (object != nullptr)
		m_currentSource.emplace_back(*object);

	// Execute the block with the new source.
	auto result = visit(context->block());

	// If we have a source, pop it off the source stack.
	if (object != nullptr)
		m_currentSource.pop_back();

	return result;
}

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

std::any GS1Visitor::visitAssignmentOperation(GS1Parser::AssignmentOperationContext* context)
{
	auto results = visitChildrenAndCollect(context);
	if (results.size() != 2 || context->children.size() != 3)
		throw std::runtime_error("AssignmentOperation is not a binary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::runtime_error("AssignmentOperation has no operation");

	auto left = getGameVariableFromAny(results[0]);
	auto right = getReadOnlyGameValueFromAny(results[1]);

	auto left_var = getGameVariableFromVariant(left.first);
	if (left_var == nullptr)
		throw std::runtime_error("AssignmentOperation left side is not a valid assignable value");

	// Do the assignment operation separately as everything else runs on doubles.
	if (op.value() == GS1Parser::OP_ASSIGN)
	{
		if (left.second.has_value())
			left_var->assign<double>(right, left.second);
		else left_var->assign<double, std::vector<double>>(right);
		return {};
	}

	double leftD = left_var->get<double>(left.second).value_or(0.0);
	double rightD = right.get<double>().value_or(0.0);

	// Perform the operation.
	switch (op.value())
	{
		case GS1Parser::OP_ASSIGN_ADD:
			left_var->assign(leftD + rightD, left.second);
			break;
		case GS1Parser::OP_ASSIGN_SUB:
			left_var->assign(leftD - rightD, left.second);
			break;
		case GS1Parser::OP_ASSIGN_MUL:
			left_var->assign(leftD * rightD, left.second);
			break;
		case GS1Parser::OP_ASSIGN_DIV:
			left_var->assign(leftD / rightD, left.second);
			break;
		case GS1Parser::OP_ASSIGN_MOD:
			left_var->assign(static_cast<double>(static_cast<int64_t>(leftD) % static_cast<int64_t>(rightD)), left.second);
			break;
		case GS1Parser::OP_ASSIGN_POW:
			left_var->assign(std::pow(leftD, rightD), left.second);
			break;
	}

	// Assignment operations are statements and can't be used inside expressions.
	return {};
}

std::any GS1Visitor::visitUnaryOperation(GS1Parser::UnaryOperationContext* context)
{
	auto left = visit(context->unary_operator());
	auto right = visit(context->expression());

	auto symbol = getSymbolType(context->children[0]);
	if (!symbol.has_value())
		throw std::runtime_error("UnaryOperation does not have an operator");

	auto rightValue = getReadOnlyGameValueFromAnyAs<double>(right);
	switch (symbol.value())
	{
		case GS1Parser::OP_ADD:
			return right;
		case GS1Parser::OP_SUB:
			return std::make_any<GS1ScriptValue>(-rightValue);
		case GS1Parser::OP_LOGICALNOT:
			return std::make_any<GS1ScriptValue>(rightValue == 0.0 ? 1.0 : 0.0);
	}

	throw std::runtime_error("UnaryOperation has an unknown operator");
}

std::any GS1Visitor::visitMessageCode(GS1Parser::MessageCodeContext* context)
{
	auto results = visitChildrenAndCollect(context);
	auto messageCode = context->MESSAGECODE()->getText();
	if (messageCode.empty())
		throw std::runtime_error("MessageCode is not a valid message code");

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

std::any GS1Visitor::visitLiteral(GS1Parser::LiteralContext* context)
{
	auto text = context->LITERAL()->getText();
	if (text == "true") return std::make_any<GS1ScriptValue>(1.0);
	if (text == "false") return std::make_any<GS1ScriptValue>(0.0);
	return std::make_any<GS1ScriptValue>(std::stod(text));
}

std::any GS1Visitor::visitLiteralAllFeatures(GS1Parser::LiteralAllFeaturesContext* context)
{
	return std::make_any<GS1ScriptValue>(static_cast<double>(0xFFFF));
}

std::any GS1Visitor::visitLiteralAllStats(GS1Parser::LiteralAllStatsContext* context)
{
	return std::make_any<GS1ScriptValue>(static_cast<double>(0xFFFF));
}

std::any GS1Visitor::visitStringLiteral(GS1Parser::StringLiteralContext* context)
{
	return std::make_any<GS1ScriptValue>(GameValue{ context->STRING()->getText() });
}

std::any GS1Visitor::visitIdentifierLiteral(GS1Parser::IdentifierLiteralContext* context)
{
	return std::make_any<GS1ScriptValue>(GameValue{ context->IDENTIFIER()->getText() });
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

	auto results = visitChildrenAndCollect(context);
	for (auto& child : results)
	{
		auto result = getReadOnlyGameValueFromAnyAs<double>(child);
		values.push_back(result);
	}
	return std::make_any<GS1ScriptValue>(std::move(values));
}

std::any GS1Visitor::visitItemLiteral(GS1Parser::ItemLiteralContext* context)
{
	auto text = context->ITEM()->getText();
	auto it = std::ranges::find(itemNames, text);
	if (it == itemNames.end())
		it = itemNames.begin();

	return std::make_any<GS1ScriptValue>(static_cast<double>(std::distance(itemNames.begin(), it)));
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
	auto it = std::ranges::find(baddyNames, text);
	if (it == baddyNames.end())
		it = baddyNames.begin();

	return std::make_any<GS1ScriptValue>(static_cast<double>(std::distance(baddyNames.begin(), it)));
}

std::any GS1Visitor::visitStorageToken(GS1Parser::StorageTokenContext* context)
{
	return visitChildren(context);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar
