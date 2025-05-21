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

// Gets our mangled identifier which has the data type tacked on.
// GS1 has separate variable stores per data type.  So dumb.
static std::string getMangledIdentifier(ScriptIdentifier& identifier, ScriptVariable& value)
{
	std::string result;

	if (std::holds_alternative<std::string>(identifier))
		result = std::get<std::string>(identifier);
	else if (std::holds_alternative<std::pair<std::string, size_t>>(identifier))
		result = std::get<std::pair<std::string, size_t>>(identifier).first;
	else throw std::exception("getMangledIdentifier did not receive a valid identifier");

	if (std::holds_alternative<double>(value))
		result += "|double";
	else if (std::holds_alternative<std::string>(value))
		result += "|string";
	else if (std::holds_alternative<std::vector<double>>(value))
		result += "|array";
	else throw std::exception("getMangledIdentifier received a ScriptVariable with an unknown data type");

	return result;
}

// Gets our mangled identifier which has the data type tacked on.
static std::string getMangledIdentifier(ScriptIdentifier& identifier, std::string_view dataType)
{
	std::string result;

	if (std::holds_alternative<std::string>(identifier))
		result = std::get<std::string>(identifier);
	else if (std::holds_alternative<std::pair<std::string, size_t>>(identifier))
		result = std::get<std::pair<std::string, size_t>>(identifier).first;
	else throw std::exception("getMangledIdentifier did not receive a valid identifier");

	result += "|";
	result += dataType;

	return result;
}

static std::optional<ScriptIdentifier> getIdentifier(std::any& anyval)
{
	auto* identifier = std::any_cast<ScriptIdentifier>(&anyval);
	if (identifier != nullptr)
		return *identifier;
	auto* stringval = std::any_cast<std::string>(&anyval);
	if (stringval != nullptr)
		return ScriptIdentifier{ *stringval };
	return std::nullopt;
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

std::optional<std::variant<ScriptVariable*, double*>> GS1Visitor::lookInVariableStore(const ScriptIdentifier& identifier)
{
	// Early out if we aren't saving variables anywhere.
	if (m_variableStores == nullptr && m_defaultStore == nullptr)
		return std::nullopt;

	std::string identifierName;
	std::optional<size_t> index = std::nullopt;

	// Get our identifier name and index;
	if (std::holds_alternative<std::string>(identifier))
		identifierName = std::get<std::string>(identifier);
	else if (std::holds_alternative<std::pair<std::string, size_t>>(identifier))
	{
		auto& pair = std::get<std::pair<std::string, size_t>>(identifier);
		identifierName = pair.first;
		index = pair.second;
	}
	else throw std::exception("getScriptVariableFromStore received an invalid identifier");

	// If we don't have a data type on our identifier, pick a default.
	if (!identifierName.contains('|'))
	{
		if (index.has_value())
			identifierName += "|array";
		else
			identifierName += "|double";
	}

	// Look through all the variable stores for the variable.
	if (m_variableStores != nullptr)
	{
		for (auto& [prefix, storePicker] : *m_variableStores)
		{
			if (prefix.empty() || identifierName.starts_with(prefix))
			{
				if (std::holds_alternative<ScriptVariableStore*>(storePicker))
				{
					auto store = std::get<ScriptVariableStore*>(storePicker);
					auto variable = store->get(identifierName);

					// No variable found, create a new one.
					if (variable == nullptr)
						return &store->add(identifierName, ScriptVariable{ 0.0 });

					// Check if the variable is an array.
					if (std::holds_alternative<std::vector<double>>(*variable))
					{
						auto safeIndex = index.value_or(0);
						auto& array = std::get<std::vector<double>>(*variable);

						// Bad index access just returns 0.
						if (safeIndex >= array.size())
							return std::nullopt;

						return &array[safeIndex];
					}

					return variable;
				}
				else if (std::holds_alternative<ScriptVariableFromServer>(storePicker))
				{
					auto picker = std::get<ScriptVariableFromServer>(storePicker);
					return picker(identifierName, index.value_or(0));
				}
			}
		}
	}

	// Check the default store.
	if (m_defaultStore != nullptr)
		return &m_defaultStore->get_or_add(identifierName);

	// No variable found.
	return std::nullopt;
}

std::optional<double*> GS1Visitor::getIdentifierValueForAssignment(std::any anyval)
{
	// Get our identifier.
	auto identifier = getIdentifier(anyval);
	if (!identifier.has_value())
		return std::nullopt;

	return getIdentifierValueForAssignment(identifier.value());
}

std::optional<double*> GS1Visitor::getIdentifierValueForAssignment(ScriptIdentifier& identifier)
{
	// Try to get our variable.
	// Our value can be just a double* for updating an array, so lets try and get that.
	double* value = nullptr;

	// Try to get our variable.
	auto identifier_value = lookInVariableStore(identifier);
	if (!identifier_value.has_value())
		return std::nullopt;

	if (std::holds_alternative<ScriptVariable*>(identifier_value.value()))
	{
		auto* variable = std::get<ScriptVariable*>(identifier_value.value());
		if (variable == nullptr)
			return std::nullopt;

		// It needs to hold a double.
		if (!std::holds_alternative<double>(*variable))
			return std::nullopt;

		// Link to the double stored in the variant.
		value = &std::get<double>(*variable);
	}
	else if (std::holds_alternative<double*>(identifier_value.value()))
	{
		// Pull the double* out.
		auto* arrayvar = std::get<double*>(identifier_value.value());
		if (arrayvar == nullptr)
			return std::nullopt;

		value = arrayvar;
	}

	if (value == nullptr)
		return std::nullopt;

	return value;
}

///////////////////////////////////////////////////////////////////////////////

ScriptVariable* GS1Visitor::getGS1ScriptVariableUnsafe(std::any& anyval)
{
	auto* identifier_test = std::any_cast<ScriptIdentifier>(&anyval);
	if (identifier_test == nullptr)
		return getScriptVariableUnsafe(anyval);

	auto variable = lookInVariableStore(*identifier_test);
	if (!variable.has_value())
		return nullptr;

	if (std::holds_alternative<double*>(variable.value()))
		throw std::exception("getGS1ScriptVariableUnsafe had a double*, consider using a safer version");

	return std::get<ScriptVariable*>(variable.value());
}

const ScriptVariable* GS1Visitor::getGS1ScriptVariableUnsafe(const std::any& anyval)
{
	auto* identifier_test = std::any_cast<ScriptIdentifier>(&anyval);
	if (identifier_test == nullptr)
		return getScriptVariableUnsafe(anyval);

	auto variable = lookInVariableStore(*identifier_test);
	if (!variable.has_value())
		return nullptr;

	if (std::holds_alternative<double*>(variable.value()))
		throw std::exception("getGS1ScriptVariableUnsafe had a double*, consider using a safer version");

	return std::get<ScriptVariable*>(variable.value());
}

ScriptVariable& GS1Visitor::getGS1ScriptVariableOr(std::any& anyval, ScriptVariable& defaultValue)
{
	auto* identifier_test = std::any_cast<ScriptIdentifier>(&anyval);
	if (identifier_test == nullptr)
		return getScriptVariableOr(anyval, defaultValue);

	auto variable = lookInVariableStore(*identifier_test);
	if (!variable.has_value())
		return defaultValue;

	if (std::holds_alternative<double*>(variable.value()))
		throw std::exception("getGS1ScriptVariableOr had a double*, consider using a safer version");

	auto* variable_value = std::get<ScriptVariable*>(variable.value());
	if (variable_value == nullptr)
		return defaultValue;

	return *std::get<ScriptVariable*>(variable.value());
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

	auto zero = ScriptVariable{ 0.0 };
	auto& left = getGS1ScriptVariableOr(results[0], zero);
	auto& right = getGS1ScriptVariableOr(results[1], zero);

	auto getAsDoubles = [&zero](ScriptVariable& left, ScriptVariable& right) -> std::pair<double, double>
	{
		auto* left_double = &left;
		auto* right_double = &right;
		if (!std::holds_alternative<double>(left)) left_double = &zero;
		if (!std::holds_alternative<double>(right)) right_double = &zero;
		return std::make_pair(std::get<double>(*left_double), std::get<double>(*right_double));
	};

	switch (op->getSymbol()->getType())
	{
		case GS1Parser::OP_POW:
		{
			auto doubles = getAsDoubles(left, right);
			return ScriptVariable{ std::pow(doubles.first, doubles.second) };
		}
		case GS1Parser::OP_MUL:
			return left * right;
		case GS1Parser::OP_DIV:
			return left / right;
		case GS1Parser::OP_MOD:
			return left % right;
		case GS1Parser::OP_ADD:
			return left + right;
		case GS1Parser::OP_SUB:
			return left - right;
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

	ScriptVariable zero{ 0.0 };
	auto left = getGS1ScriptVariableOr(results[0], zero);
	auto right = getGS1ScriptVariableOr(results[1], zero);

	switch (op.value())
	{
		case GS1Parser::OP_EQUAL:
		case GS1Parser::OP_ASSIGN:
			return ScriptVariable{ (left == right) ? 1.0 : 0.0 };
		case GS1Parser::OP_NOTEQ:
			return ScriptVariable{ (left != right) ? 1.0 : 0.0 };
		case GS1Parser::OP_LESS:
			return ScriptVariable{ (left < right) ? 1.0 : 0.0 };
		case GS1Parser::OP_GREAT:
			return ScriptVariable{ (left > right) ? 1.0 : 0.0 };
		case GS1Parser::OP_LESS_EQ:
			return ScriptVariable{ (left <= right) ? 1.0 : 0.0 };
		case GS1Parser::OP_GREAT_EQ:
			return ScriptVariable{ (left >= right) ? 1.0 : 0.0 };
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

	auto left = getGS1ScriptVariable<bool>(results[0]).value_or(false);
	auto right = getGS1ScriptVariable<bool>(results[1]).value_or(false);

	switch (op.value())
	{
		case GS1Parser::OP_LOGICALAND:
			return ScriptVariable{ (left && right) ? 1.0 : 0.0 };
		case GS1Parser::OP_LOGICALOR:
			return ScriptVariable{ (left || right) ? 1.0 : 0.0 };
	}

	throw std::exception("LogicExpression has an unknown operator");
}

std::any GS1Visitor::visitTernaryExpression(GS1Parser::TernaryExpressionContext* context)
{
	auto condition = visit(context->binary_expression(0));
	if (getGS1ScriptVariable<bool>(condition).value_or(false))
		return visit(context->binary_expression(1));
	return visit(context->binary_expression(2));
}

std::any GS1Visitor::visitInExpression(GS1Parser::InExpressionContext* context)
{
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() != 2)
		throw std::exception("InExpression is not a binary expression");

	auto* right_range = std::any_cast<ScriptVariablePair>(&results[1]);
	auto* right_value = std::any_cast<ScriptVariable>(&results[1]);
	bool right_array = (right_value != nullptr && std::holds_alternative<std::vector<double>>(*right_value));

	if (right_range == nullptr && !right_array)
		throw std::exception("InExpression does not have a range or array");

	auto left = getGS1ScriptVariable<double>(results[0]).value_or(0.0);
	if (right_range != nullptr)
	{
		bool in_range = (right_range->first <= left && left <= right_range->second);
		return ScriptVariable{ in_range ? 1.0 : 0.0 };
	}
	else
	{
		auto& right_vector = std::get<std::vector<double>>(*right_value);
		return ScriptVariable{ std::ranges::contains(right_vector, left) ? 1.0 : 0.0 };
	}
}

std::any GS1Visitor::visitIdentifierArray(GS1Parser::IdentifierArrayContext* context)
{
	auto compound_any = visit(context->compound_identifier());
	const auto* identifier = std::any_cast<ScriptIdentifier>(&compound_any);
	if (identifier == nullptr || !std::holds_alternative<std::string>(*identifier))
		throw std::exception("IdentifierArray received an invalid compound_identifier");

	auto expression_any = visit(context->primary_expression());
	auto expression_index = getGS1ScriptVariable(expression_any);
	if (!expression_index.has_value() || !std::holds_alternative<double>(expression_index.value()))
		throw std::exception("IdentifierArray received an invalid array index");

	auto index = static_cast<size_t>(std::get<double>(expression_index.value()));
	return ScriptIdentifier{ std::make_pair(std::get<std::string>(*identifier), index) };
}

std::any GS1Visitor::visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context)
{
	std::string identifier;

	auto results = visitChildrenAndCollect(this, context);
	for (auto& piece : results)
	{
		auto var = getIdentifier(piece);
		if (!var.has_value())
			throw std::exception("CompoundIdentifier chunk was not a valid identifier");

		// If this is an array identifier, don't allow any compounding after it.
		if (std::holds_alternative<std::pair<std::string, size_t>>(var.value()))
		{
			auto pair = std::get<std::pair<std::string, size_t>>(var.value());
			identifier.append(pair.first);
			return ScriptIdentifier{ std::make_pair(identifier, pair.second) };
		}

		if (!std::holds_alternative<std::string>(var.value()))
			throw std::exception("CompoundIdentifier chunk was not a valid data type");

		auto& str = std::get<std::string>(var.value());
		identifier.append(str);
	}

	return ScriptIdentifier{ identifier };
}

std::any GS1Visitor::visitIncDecOperation(GS1Parser::IncDecOperationContext* context)
{
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() == 0 || context->children.size() != 2)
		throw std::exception("IncDecOperation is not a unary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::exception("IncDecOperation has no operation");

	// Get our identifier value.
	auto identifier = getIdentifierValueForAssignment(results[0]);
	if (!identifier.has_value() || identifier.value() == nullptr)
		throw std::exception("IncDecOperation has no identifier value");

	double* identifier_value = identifier.value();
	switch (op.value())
	{
		case GS1Parser::OP_INC:
			++(*identifier_value);
			break;
		case GS1Parser::OP_DEC:
			--(*identifier_value);
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
		auto identifier_name = getIdentifier(results[0]);
		if (!identifier_name.has_value())
			throw std::exception("setstring identifier is not a valid identifier");

		// Find the identifier variable in the store.
		auto identifier = lookInVariableStore(ScriptIdentifier{ getMangledIdentifier(identifier_name.value(), "string") });
		if (!identifier.has_value() || !std::holds_alternative<ScriptVariable*>(identifier.value()))
			return {};

		// Get the link to the value.
		auto identifier_value = std::get<ScriptVariable*>(identifier.value());
		if (identifier_value == nullptr)
			return {};

		// Assign the string.
		auto value = getGS1ScriptVariable<std::string>(results[1]).value_or({});
		*identifier_value = value;
	}

	return {};
}

std::any GS1Visitor::visitUserFunctionCall(GS1Parser::UserFunctionCallContext* context)
{
	//throw std::exception("visitUserFunctionCall not implemented");
	auto identifier = visit(context->identifier_literal());
	auto function_name = getGS1ScriptVariable(identifier);
	if (!function_name.has_value() || !std::holds_alternative<std::string>(function_name.value()))
		throw std::exception("UserFunctionCall has no valid function name");

	if (m_parser == nullptr)
		throw std::exception("GS1Visitor is missing the link to the parser");

	std::string& name = std::get<std::string>(function_name.value());
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
	if (getGS1ScriptVariable<bool>(condition).value_or(false))
		return visit(context->if_true_block());
	else if (auto elseblock = context->else_false_block(); elseblock != nullptr)
		return visit(elseblock);

	return {};
}

std::any GS1Visitor::visitForLoop(GS1Parser::ForLoopContext* context)
{
	// Assignment.
	safeVisit(context->assignment_operation());

	// Condition.
	size_t loopCount = 0;
	while (loopCount++ < MAX_LOOPS && getGS1ScriptVariable<bool>(safeVisit(context->expression(0))).value_or(false))
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
	while (loopCount++ < MAX_LOOPS && getGS1ScriptVariable<bool>(visit(context->expression())).value_or(false))
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
	auto identifier = getIdentifier(results[0]);
	if (!identifier.has_value())
		throw std::exception("IncDecOperation has no identifier value");

	// Get the assignment variable.
	auto assignment = getGS1ScriptVariable(results[1]);
	if (!assignment.has_value())
		throw std::exception("AssignmentOperation has no assignment value");

	// Determine the identifier name mangle for searching our variable stores.
	auto mangled = getMangledIdentifier(identifier.value(), assignment.value());

	// Get our identifier variable.
	auto identifier_variable = lookInVariableStore(ScriptIdentifier{ mangled });
	if (!identifier_variable.has_value())
		throw std::exception("AssignmentOperation has no identifier variable");

	// Do a direct assignment first as the rest rely on doubles exclusively.
	if (op.value() == GS1Parser::OP_ASSIGN || op.value() == GS1Parser::OP_ASSIGN2)
	{
		// Handle array assignment.
		if (std::holds_alternative<double*>(identifier_variable.value()))
		{
			double* value = std::get<double*>(identifier_variable.value());
			if (!std::holds_alternative<double>(assignment.value()))
				*value = 0.0;
			else *value = std::get<double>(assignment.value());
			return {};
		}

		// Otherwise, just set.
		*std::get<ScriptVariable*>(identifier_variable.value()) = assignment.value();
		return {};
	}

	auto* identifier_value = std::get<ScriptVariable*>(identifier_variable.value());
	if (identifier_value == nullptr)
		throw std::exception("AssignmentOperation identifier value is null");

	// The mathematical assignments require a double.
	if (!std::holds_alternative<double>(*identifier_value) || !std::holds_alternative<double>(assignment.value()))
		throw std::exception("AssignmentOperation is can only perform math on doubles");

	// Handle our values that require a double.
	auto& identifier_value_double = std::get<double>(*identifier_value);
	double assignment_value = std::get<double>(assignment.value());
	switch (op.value())
	{
		case GS1Parser::OP_ASSIGN_ADD:
			identifier_value_double += assignment_value;
			break;
		case GS1Parser::OP_ASSIGN_SUB:
			identifier_value_double -= assignment_value;
			break;
		case GS1Parser::OP_ASSIGN_MUL:
			identifier_value_double *= assignment_value;
			break;
		case GS1Parser::OP_ASSIGN_DIV:
			identifier_value_double /= assignment_value;
			break;
		case GS1Parser::OP_ASSIGN_MOD:
			identifier_value_double = static_cast<double>(static_cast<int64_t>(identifier_value_double) % static_cast<int64_t>(assignment_value));
			break;
		case GS1Parser::OP_ASSIGN_POW:
			identifier_value_double = std::pow(identifier_value_double, assignment_value);
			break;
	}

	return {};
}

std::any GS1Visitor::visitUnaryOperation(GS1Parser::UnaryOperationContext* context)
{
	auto left = visit(context->unary_operator());
	auto right = visit(context->expression());

	auto symbol = getSymbolType(context->children[0]);
	if (!symbol.has_value())
		throw std::exception("UnaryOperation does not have an operator");

	ScriptVariable zero{ 0.0 };
	auto& value = getGS1ScriptVariableOr(right, zero);

	switch (symbol.value())
	{
		case GS1Parser::OP_ADD:
			return right;
		case GS1Parser::OP_SUB:
			return -zero;
		case GS1Parser::OP_LOGICALNOT:
			return !zero;
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
	if (text == "true") return std::make_any<ScriptVariable>(1.0);
	if (text == "false") return std::make_any<ScriptVariable>(0.0);
	return std::make_any<ScriptVariable>(std::stod(text));
}

std::any GS1Visitor::visitLiteralAllFeatures(GS1Parser::LiteralAllFeaturesContext* context)
{
	return std::make_any<ScriptVariable>(static_cast<double>(0xFFFF));
}

std::any GS1Visitor::visitStringLiteral(GS1Parser::StringLiteralContext* context)
{
	std::string text{ std::move(context->STRING()->getText()) };
	return std::make_any<ScriptVariable>(std::move(string::trimMutate(text)));
}

std::any GS1Visitor::visitIdentifierLiteral(GS1Parser::IdentifierLiteralContext* context)
{
	std::string text{ std::move(context->IDENTIFIER()->getText()) };
	return std::make_any<ScriptIdentifier>(std::move(string::trimMutate(text)));
}

std::any GS1Visitor::visitRangeLiteral(GS1Parser::RangeLiteralContext* context)
{
	auto left = visit(context->expression(0));
	auto right = visit(context->expression(1));
	if (!left.has_value() || left.type() != typeid(ScriptVariable))
		throw std::exception("range literal (left) is not a valid data type");
	if (!right.has_value() || right.type() != typeid(ScriptVariable))
		throw std::exception("range literal (right) is not a valid data type");
	return std::make_pair(std::any_cast<ScriptVariable>(std::move(left)), std::any_cast<ScriptVariable>(std::move(right)));
}

std::any GS1Visitor::visitArrayLiteral(GS1Parser::ArrayLiteralContext* context)
{
	std::vector<double> results;
	for (auto& child : context->children)
	{
		auto result = visit(child);
		if (result.type() != typeid(ScriptVariable))
			continue;
		auto value = std::any_cast<ScriptVariable>(&result);
		if (!std::holds_alternative<double>(*value))
			continue;
		results.push_back(std::get<double>(*value));
	}
	return std::make_any<ScriptVariable>(std::move(results));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::grammar::gs1
