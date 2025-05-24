#include <scripting/gs1/GS1Visitor.h>

#include <cstdint>
#include <cmath>
#include <variant>
#include <format>
#include <string>
#include <string_view>
#include <exception>
#include <utility>
#include <vector>
#include <ranges>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::grammar::gs1
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

static ScriptIdentifier& checkAndAssignIdentifierType(ScriptIdentifier& identifier, const ScriptVariable& value)
{
	const auto dataTypeVisitor = visit_functions
	{
		[](const std::string&) -> std::string { return "|string"; },
		[](const double&) -> std::string { return "|double"; },
		[](const std::vector<double>&) -> std::string { return "|array"; },
	};

	const auto identifierVisitor = visit_functions
	{
		[&value, &dataTypeVisitor](std::string& str) -> void
		{
			if (!str.contains('|'))
				str += std::visit(dataTypeVisitor, value);
		},
		[&value](std::pair<std::string, size_t>& pair) -> void
		{
			if (!pair.first.contains('|'))
				pair.first += "|array";
		},
	};

	std::visit(identifierVisitor, identifier);
	return identifier;
}

static ScriptVariableContainer& checkAndAssignIdentifierType(ScriptVariableContainer& container, const ScriptVariable& value)
{
	if (!container.hasIdentifier())
		return container;

	auto& identifier = container.getMutableIdentifier();
	if (!identifier.has_value())
		return container;

	checkAndAssignIdentifierType(identifier.value(), value);
	return container;
}

static ScriptVariableContainer& checkAndAssignIdentifierType(ScriptVariableContainer& container)
{
	if (!container.hasIdentifier())
		return container;

	auto& identifier = container.getMutableIdentifier();
	if (!identifier.has_value())
		return container;

	checkAndAssignIdentifierType(identifier.value(), container.get());
	return container;
}

static std::any makeScriptVariableContainerFrom(auto&& value)
{
	ScriptVariableContainer container{ ScriptVariable{ std::forward<decltype(value)>(value) } };
	return std::make_any<ScriptVariableContainer>(std::move(container));
}

static std::vector<std::any> visitChildrenAndCollect(GS1Visitor* visitor, antlr4::tree::ParseTree* node)
{
	if (node == nullptr) return {};
	std::vector<std::any> results;
	for (size_t i = 0; i < node->children.size(); ++i)
	{
		auto ret = node->children[i]->accept(visitor);
		if (ret.has_value())
			results.emplace_back(std::move(ret));
	}
	return results;
}

///////////////////////////////////////////////////////////////////////////////
// Static member functions.

bool GS1Visitor::identifierHasDataType(const ScriptIdentifier& identifier)
{
	if (std::holds_alternative<std::string>(identifier))
		return std::get<std::string>(identifier).contains('|');
	else if (std::holds_alternative<std::pair<std::string, size_t>>(identifier))
		return std::get<std::pair<std::string, size_t>>(identifier).first.contains('|');
	return false;
}

// Gets an identifier which has the data type tacked on.
// GS1 has separate variable stores per data type.  So dumb.
std::string GS1Visitor::getTypedIdentifier(const ScriptIdentifier& identifier, std::string_view dataType)
{
	std::string result;

	if (std::holds_alternative<std::string>(identifier))
		result = std::get<std::string>(identifier);
	else if (std::holds_alternative<std::pair<std::string, size_t>>(identifier))
		result = std::get<std::pair<std::string, size_t>>(identifier).first;
	else throw std::exception("getTypedIdentifier did not receive a valid identifier");

	result += "|";
	result += dataType;

	return result;
}

ScriptVariableContainer& GS1Visitor::assignIdentifierType(ScriptVariableContainer& container, std::string_view dataType)
{
	if (!container.hasIdentifier())
		return container;

	auto& identifier = container.getMutableIdentifier();
	if (!identifier.has_value())
		return container;

	std::string typedIdentifier = getTypedIdentifier(identifier.value(), dataType);
	if (std::holds_alternative<std::string>(identifier.value()))
		std::get<std::string>(identifier.value()) = typedIdentifier;
	else if (std::holds_alternative<std::pair<std::string, size_t>>(identifier.value()))
		std::get<std::pair<std::string, size_t>>(identifier.value()).first = typedIdentifier;

	return container;
}

///////////////////////////////////////////////////////////////////////////////
// Member functions.

std::any GS1Visitor::safeVisit(antlr4::tree::ParseTree* node)
{
	if (node == nullptr)
		return {};
	return visit(node);
}

ScriptVariableContainer& GS1Visitor::bindLinkToVariableStore(ScriptVariableContainer& container)
{
	// Early out if we aren't saving variables anywhere.
	if (m_variableStores == nullptr && m_defaultStore == nullptr)
		return container;

	auto identifier = container.getIdentifier();
	if (!identifier.has_value())
		return container;

	std::string identifierName = getIdentifierName(identifier.value());
	if (identifierName.empty() || !identifierName.contains('|'))
		return container;

	// Find the store that would hold this identifier.
	if (m_variableStores != nullptr)
	{
		for (auto& [prefix, storePicker] : *m_variableStores)
		{
			if (prefix.empty() || identifierName.starts_with(prefix))
			{
				if (std::holds_alternative<ScriptVariableStore*>(storePicker))
				{
					auto store = std::get<ScriptVariableStore*>(storePicker);
					container.setSetter(store->bindSetter());
					return container;
				}
			}
		}
	}

	// If we have a default store, bind to it.
	if (m_defaultStore != nullptr)
		container.setSetter(m_defaultStore->bindSetter());

	return container;
}

ScriptVariable GS1Visitor::getVariableFromStores(const ScriptIdentifier& identifier)
{
	if (auto* str = std::get_if<std::string>(&identifier); str != nullptr && str->contains('|'))
	{
		auto var = retrieveVariableFromStore(identifier, m_defaultStore, m_variableStores);
		if (!var.has_value())
			return ScriptVariable{ 0.0 };
		return var.value().get();
	}

	if (auto* pair = std::get_if<std::pair<std::string, size_t>>(&identifier); pair != nullptr && pair->first.contains('|'))
	{
		auto var = retrieveVariableFromStore(identifier, m_defaultStore, m_variableStores);
		if (auto* doubles = std::get_if<std::vector<double>>(&var.value().get()))
		{
			if (pair->second < doubles->size())
				return ScriptVariable{ doubles->at(pair->second) };
		}
		return ScriptVariable{ 0.0 };
	}

	//return ScriptVariable{ 0.0 };
	throw std::exception("getVariableFromStores identifier has no data type");
}

///////////////////////////////////////////////////////////////////////////////

void GS1Visitor::execute(const ScriptEvent& event, ScriptEventSource source, GS1Parser& parser, antlr4::tree::ParseTree& startNode, ScriptVariableStore* defaultStore, ScriptVariableStoreMap* variableStores)
{
	m_parser = &parser;
	m_source = source;
	m_defaultStore = defaultStore;
	m_variableStores = variableStores;

	// Execute!
	visit(&startNode);
}

///////////////////////////////////////////////////////////////////////////////

std::any GS1Visitor::visitMathExpression(GS1Parser::MathExpressionContext* context)
{
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() != 2)
		throw std::exception("MathExpression is not a binary expression");

	auto op = dynamic_cast<antlr4::tree::TerminalNode*>(context->children[1]);
	if (op == nullptr)
		throw std::exception("MathExpression does not have an operator");

	auto left = gs1TryGetScriptVariableValueFromAny<double>(results[0], 0.0);
	auto right = gs1TryGetScriptVariableValueFromAny<double>(results[1], 0.0);

	switch (op->getSymbol()->getType())
	{
		case GS1Parser::OP_POW:
			return std::make_any<ScriptVariableContainer>(std::pow(left, right));
		case GS1Parser::OP_MUL:
			return std::make_any<ScriptVariableContainer>(left * right);
		case GS1Parser::OP_DIV:
			return std::make_any<ScriptVariableContainer>(left / right);
		case GS1Parser::OP_MOD:
			return std::make_any<ScriptVariableContainer>(static_cast<double>(static_cast<int64_t>(left) % static_cast<int64_t>(right)));
		case GS1Parser::OP_ADD:
			return std::make_any<ScriptVariableContainer>(left + right);
		case GS1Parser::OP_SUB:
			return std::make_any<ScriptVariableContainer>(left - right);
	}

	throw std::exception("MathExpression has an unknown operator");
}

std::any GS1Visitor::visitComparisonExpression(GS1Parser::ComparisonExpressionContext* context)
{
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() != 2)
		throw std::exception("ComparisonExpression is not a binary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::exception("ComparisonExpression does not have an operator");

	auto left = gs1TryGetScriptVariableValueFromAny<double>(results[0], 0.0);
	auto right = gs1TryGetScriptVariableValueFromAny<double>(results[1], 0.0);

	switch (op.value())
	{
		case GS1Parser::OP_EQUAL:
		case GS1Parser::OP_ASSIGN:
			return std::make_any<ScriptVariableContainer>((left == right) ? 1.0 : 0.0);
		case GS1Parser::OP_NOTEQ:
			return std::make_any<ScriptVariableContainer>((left != right) ? 1.0 : 0.0);
		case GS1Parser::OP_LESS:
			return std::make_any<ScriptVariableContainer>((left < right) ? 1.0 : 0.0);
		case GS1Parser::OP_GREAT:
			return std::make_any<ScriptVariableContainer>((left > right) ? 1.0 : 0.0);
		case GS1Parser::OP_LESS_EQ:
			return std::make_any<ScriptVariableContainer>((left <= right) ? 1.0 : 0.0);
		case GS1Parser::OP_GREAT_EQ:
			return std::make_any<ScriptVariableContainer>((left >= right) ? 1.0 : 0.0);
	}

	throw std::exception("ComparisonExpression has an unknown operator");
}

std::any GS1Visitor::visitLogicExpression(GS1Parser::LogicExpressionContext* context)
{
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() != 2)
		throw std::exception("LogicExpression is not a binary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::exception("LogicExpression does not have an operator");

	auto left = gs1TryGetScriptVariableValueFromAny<bool>(results[0], false);
	auto right = gs1TryGetScriptVariableValueFromAny<bool>(results[1], false);

	switch (op.value())
	{
		case GS1Parser::OP_LOGICALAND:
			return std::make_any<ScriptVariableContainer>((left && right) ? 1.0 : 0.0);
		case GS1Parser::OP_LOGICALOR:
			return std::make_any<ScriptVariableContainer>((left || right) ? 1.0 : 0.0);
	}

	throw std::exception("LogicExpression has an unknown operator");
}

std::any GS1Visitor::visitTernaryExpression(GS1Parser::TernaryExpressionContext* context)
{
	auto condition = gs1TryGetScriptVariableValueFromAny<bool>(visit(context->binary_expression(0)), false);
	if (condition == true)
		return visit(context->binary_expression(1));
	return visit(context->binary_expression(2));
}

std::any GS1Visitor::visitInExpression(GS1Parser::InExpressionContext* context)
{
	std::vector<double> values;
	for (auto& be : context->binary_expression())
		values.emplace_back(gs1TryGetScriptVariableValueFromAny<double>(visit(be), 0.0));

	auto right_any = visit(context->in_expression());
	auto* right_range = std::any_cast<std::pair<std::any, std::any>>(&right_any);
	auto* right_value = std::any_cast<ScriptVariableContainer>(&right_any);
	bool right_array = (right_value != nullptr && std::holds_alternative<std::vector<double>>(right_value->get()));

	if (right_range == nullptr && !right_array)
		throw std::exception("InExpression does not have a range or array");

	size_t range_op_left = GS1Parser::TOKEN_PIPE;
	size_t range_op_right = GS1Parser::TOKEN_PIPE;
	if (right_range != nullptr)
	{
		range_op_left = getSymbolType(context->in_expression()->range_literal()->children[0]).value_or(GS1Parser::TOKEN_PIPE);
		range_op_right = getSymbolType(context->in_expression()->range_literal()->children[4]).value_or(GS1Parser::TOKEN_PIPE);
	}

	bool range_met = true;
	for (const auto& check : values)
	{
		if (right_range != nullptr)
		{
			double first = gs1TryGetScriptVariableValueFromAny<double>(right_range->first, 0.0);
			double second = gs1TryGetScriptVariableValueFromAny<double>(right_range->second, 0.0);
			bool test_left = (range_op_left == GS1Parser::TOKEN_PIPE) ? (first <= check) : (first < check);
			bool test_right = (range_op_right == GS1Parser::TOKEN_PIPE) ? (check <= second) : (check < second);
			bool in_range = test_left && test_right;
			range_met = range_met && in_range;
		}
		else
		{
			auto& right_vector = std::get<std::vector<double>>(right_value->get());
			range_met = range_met && (std::ranges::contains(right_vector, check));
		}

		// Early out if we already know the result.
		if (!range_met)
			break;
	}

	return std::make_any<ScriptVariableContainer>(range_met ? 1.0 : 0.0);
}

std::any GS1Visitor::visitParenthesesExpression(GS1Parser::ParenthesesExpressionContext* context)
{
	return visit(context->binary_expression());
}

std::any GS1Visitor::visitIdentifierArray(GS1Parser::IdentifierArrayContext* context)
{
	// Get the identifier.
	auto compound_any = visit(context->compound_identifier());
	const auto* identifier = std::any_cast<ScriptVariableContainer>(&compound_any);
	if (identifier == nullptr || !identifier->getIdentifier().has_value() || !std::holds_alternative<std::string>(identifier->getIdentifier().value()))
		throw std::exception("IdentifierArray received an invalid compound_identifier");

	// Get the array index.
	auto expression_any = visit(context->primary_expression());
	auto index = gs1TryGetScriptVariableValueFromAny<double>(expression_any, 0.0);
	std::string identifier_name = std::get<std::string>(identifier->getIdentifier().value()) + "|array";

	auto identifier_pair = std::make_pair(identifier_name, static_cast<size_t>(index));
	ScriptVariableContainer container{ ScriptIdentifier{ std::move(identifier_pair) }, bindGetter()};
	return std::make_any<ScriptVariableContainer>(std::move(container));
}

std::any GS1Visitor::visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context)
{
	std::string compoundIdentifier;
	std::optional<size_t> index = std::nullopt;
	bool wasArray = false;

	auto results = visitChildrenAndCollect(this, context);
	for (auto& piece : results)
	{
		auto* identifier = std::any_cast<ScriptIdentifier>(&piece);
		if (identifier == nullptr)
			throw std::exception("CompoundIdentifier chunk was not a valid identifier");

		// If this is an array identifier, don't allow any compounding after it.
		if (std::holds_alternative<std::pair<std::string, size_t>>(*identifier))
		{
			auto pair = std::get<std::pair<std::string, size_t>>(*identifier);
			compoundIdentifier.append(pair.first);
			index = pair.second;
			wasArray = true;
			break;
		}

		if (!std::holds_alternative<std::string>(*identifier))
			throw std::exception("CompoundIdentifier chunk was not a valid data type");

		auto& str = std::get<std::string>(*identifier);
		compoundIdentifier.append(str);
	}

	// We know it is an array, so we can assign the type now.
	if (wasArray)
	{
		ScriptVariableContainer container{ ScriptIdentifier{ compoundIdentifier + "|array"}, bindGetter()};
		return std::make_any<ScriptVariableContainer>(std::move(container));
	}

	ScriptVariableContainer container{ ScriptIdentifier{ compoundIdentifier } };
	return std::make_any<ScriptVariableContainer>(std::move(container));
}

std::any GS1Visitor::visitIncDecOperation(GS1Parser::IncDecOperationContext* context)
{
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() == 0 || context->children.size() != 2)
		throw std::exception("IncDecOperation is not a unary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::exception("IncDecOperation has no operation");

	// Get our identifier.
	auto* left = getScriptVariableContainerUnsafe(results[0]);
	if (left == nullptr)
		throw std::exception("IncDecOperation has no wrapped identifier");

	// Fix our identifier type if needed.
	checkAndAssignIdentifierType(*left);
	bindLinkToVariableStore(*left);

	// Perform the operation.
	switch (op.value())
	{
		case GS1Parser::OP_INC:
			left->set(left->get() + 1.0);
			break;
		case GS1Parser::OP_DEC:
			left->set(left->get() - 1.0);
			break;
	}

	// GS1 assignment operations are statements and can't be used inside expressions.
	// So don't return anything.
	return {};
}

std::any GS1Visitor::visitBuiltInCommand(GS1Parser::BuiltInCommandContext* context)
{
	auto results = visitChildrenAndCollect(this, context);

	// Get the command.
	auto command = context->COMMAND()->getText();
	string::trimRightMutate(command);

	// TODO(Nalin): Actually implement this!
	if (command == "setstring")
	{
		if (results.size() != 2)
			throw std::exception("setstring identifier,string;");

		// Get the identifier.
		auto identifier = getScriptVariableContainer(results[0]);
		if (!identifier.has_value() || !identifier->hasIdentifier())
			throw std::exception("setstring identifier is not a valid identifier");

		// Fix the identifier if needed.
		assignIdentifierType(identifier.value(), "string");
		bindLinkToVariableStore(identifier.value());

		// Get the value.
		auto value = getScriptVariableContainer(results[1]);
		if (!value.has_value() || !std::holds_alternative<std::string>(value.value().get()))
			throw std::exception("setstring value is not a valid identifier");

		// Assign the string.
		identifier.value().set(value.value().get());
	}

	return {};
}

std::any GS1Visitor::visitUserFunctionCall(GS1Parser::UserFunctionCallContext* context)
{
	if (m_parser == nullptr)
		throw std::exception("GS1Visitor is missing the link to the parser");

	auto anyval = visit(context->identifier_literal());
	auto* identifier = std::any_cast<ScriptIdentifier>(&anyval);
	if (identifier == nullptr || !std::holds_alternative<std::string>(*identifier))
		throw std::exception("UserFunctionCall has no valid identifier");

	std::string& name = std::get<std::string>(*identifier);
	auto function = m_parser->userFunctions.find(name);
	if (function == m_parser->userFunctions.end())
		throw std::exception("UserFunctionCall could not find user function");

	return visit(function->second);
}

std::any GS1Visitor::visitBuiltInFunctionCall(GS1Parser::BuiltInFunctionCallContext* context)
{
	throw std::exception("visitBuiltInFunctionCall not implemented");
	return {};
}

std::any GS1Visitor::visitIfCondition(GS1Parser::IfConditionContext* context)
{
	auto condition = visit(context->expression());
	if (gs1TryGetScriptVariableValueFromAny<bool>(condition, false))
		return visit(context->if_true_block());
	else return safeVisit(context->else_false_block());

	return {};
}

std::any GS1Visitor::visitForLoop(GS1Parser::ForLoopContext* context)
{
	// Assignment.
	safeVisit(context->assignment_operation());

	// Condition.
	size_t loopCount = 0;
	while (loopCount++ < MAX_LOOPS && gs1TryGetScriptVariableValueFromAny<bool>(safeVisit(context->expression(0)), false))
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
	while (loopCount++ < MAX_LOOPS && gs1TryGetScriptVariableValueFromAny<bool>(visit(context->expression()), false))
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
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() != 2 || context->children.size() != 3)
		throw std::exception("AssignmentOperation is not a binary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::exception("AssignmentOperation has no operation");

	// Get our identifier.
	auto* left = getScriptVariableContainerUnsafe(results[0]);
	if (left == nullptr)
		throw std::exception("AssignmentOperation has no wrapped identifier");

	// Get our assignment value.
	auto* right = getScriptVariableContainerUnsafe(results[1]);
	if (right == nullptr)
		throw std::exception("AssignmentOperation has no wrapped assignment value");

	// Fix our identifier type if needed.
	checkAndAssignIdentifierType(*left, right->get());
	bindLinkToVariableStore(*left);

	// Do the assignment operation separately as everything else runs on doubles.
	if (op.value() == GS1Parser::OP_ASSIGN)
	{
		left->set(right->get());
		return {};
	}

	// Perform the operation.
	switch (op.value())
	{
		case GS1Parser::OP_ASSIGN_ADD:
			left->set(left->get() + right->get());
			break;
		case GS1Parser::OP_ASSIGN_SUB:
			left->set(left->get() - right->get());
			break;
		case GS1Parser::OP_ASSIGN_MUL:
			left->set(left->get() * right->get());
			break;
		case GS1Parser::OP_ASSIGN_DIV:
			left->set(left->get() / right->get());
			break;
		case GS1Parser::OP_ASSIGN_MOD:
			left->set(left->get() % right->get());
			break;
		case GS1Parser::OP_ASSIGN_POW:
			left->set(std::pow(left->get<double>(), right->get<double>()));
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
		throw std::exception("UnaryOperation does not have an operator");

	auto* value = getScriptVariableContainerUnsafe(right);
	if (value == nullptr)
		throw std::exception("UnaryOperation value is not wrapped");

	switch (symbol.value())
	{
		case GS1Parser::OP_ADD:
			return right;
		case GS1Parser::OP_SUB:
			return makeScriptVariableContainerFrom(-(value->get()));
		case GS1Parser::OP_LOGICALNOT:
			return makeScriptVariableContainerFrom(!(value->get()));
	}

	throw std::exception("UnaryOperation has an unknown operator");
}

std::any GS1Visitor::visitMessageCode(GS1Parser::MessageCodeContext* context)
{
	throw std::exception("visitMessageCode not implemented");
	return {};
}

std::any GS1Visitor::visitLiteral(GS1Parser::LiteralContext* context)
{
	auto text = context->LITERAL()->getText();
	if (text == "true") return makeScriptVariableContainerFrom(1.0);
	if (text == "false") return makeScriptVariableContainerFrom(0.0);
	return makeScriptVariableContainerFrom(std::stod(text));
}

std::any GS1Visitor::visitLiteralAllFeatures(GS1Parser::LiteralAllFeaturesContext* context)
{
	return makeScriptVariableContainerFrom(static_cast<double>(0xFFFF));
}

std::any GS1Visitor::visitStringLiteral(GS1Parser::StringLiteralContext* context)
{
	std::string text{ std::move(context->STRING()->getText()) };
	return makeScriptVariableContainerFrom(std::move(string::trimMutate(text)));
}

std::any GS1Visitor::visitIdentifierLiteral(GS1Parser::IdentifierLiteralContext* context)
{
	std::string text{ std::move(context->IDENTIFIER()->getText()) };
	ScriptIdentifier identifier{ std::move(string::trimMutate(text)) };
	return std::make_any<ScriptIdentifier>(std::move(identifier));
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

	auto results = visitChildrenAndCollect(this, context);
	for (auto& child : results)
	{
		auto result = gs1TryGetScriptVariableValueFromAny(child, 0.0);
		values.push_back(result);
	}
	return makeScriptVariableContainerFrom(std::move(values));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::grammar::gs1
