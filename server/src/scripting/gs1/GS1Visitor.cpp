#include <scripting/gs1/GS1Visitor.h>

#include <cstdint>
#include <cmath>
#include <variant>
#include <format>
#include <string>
#include <exception>
#include <utility>
#include <vector>
#include <ranges>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////

namespace preagonal::grammar::gs1
{

constexpr size_t MAX_LOOPS = 10000;

///////////////////////////////////////////////////////////////////////////////

struct break_exception : public std::exception {};
struct continue_exception : public std::exception {};
struct return_exception : public std::exception {};

///////////////////////////////////////////////////////////////////////////////

static std::optional<size_t> getSymbolType(antlr4::tree::ParseTree* tree)
{
	if (tree == nullptr) return {};

	// We might be looking for the direct child.
	if (tree->children.size() == 1)
		tree = tree->children[0];

	// Find the symbol type if this is a TerminalNode.
	if (auto* node = dynamic_cast<antlr4::tree::TerminalNode*>(tree); node != nullptr)
		return node->getSymbol()->getType();

	return {};
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

void GS1Visitor::execute(ScriptEventSource source, antlr4::tree::ParseTree* startNode, ScriptVariableStore* objectVariables, ScriptVariableStore* levelVariables)
{
	m_source = source;

	m_variableContainers.clear();
	if (objectVariables != nullptr)
		m_variableContainers.push_back(objectVariables);
	if (levelVariables != nullptr)
		m_variableContainers.push_back(levelVariables);

	// Execute!
	visit(startNode);
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
	auto& left = getScriptVariableOr(results[0], zero);
	auto& right = getScriptVariableOr(results[1], zero);

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

std::any GS1Visitor::visitTernaryExpression(GS1Parser::TernaryExpressionContext* context)
{
	auto condition = visit(context->binary_expression(0));
	if (getScriptVariable<bool>(condition).value_or(false))
		return visit(context->binary_expression(1));
	return visit(context->binary_expression(2));
}

std::any GS1Visitor::visitLogicExpression(GS1Parser::LogicExpressionContext* context)
{
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() != 2)
		throw std::exception("LogicExpression is not a binary expression");

	auto op = dynamic_cast<antlr4::tree::TerminalNode*>(context->children[1]);
	if (op == nullptr)
		throw std::exception("LogicExpression does not have an operator");

	auto left = getScriptVariable<bool>(results[0]).value_or(false);
	auto right = getScriptVariable<bool>(results[1]).value_or(false);

	switch (op->getSymbol()->getType())
	{
		case GS1Parser::OP_LESS:
			return ScriptVariable{ (left < right) ? 1.0 : 0.0 };
		case GS1Parser::OP_GREAT:
			return ScriptVariable{ (left > right) ? 1.0 : 0.0 };
		case GS1Parser::OP_LESS_EQ:
			return ScriptVariable{ (left <= right) ? 1.0 : 0.0 };
		case GS1Parser::OP_GREAT_EQ:
			return ScriptVariable{ (left >= right) ? 1.0 : 0.0 };
		case GS1Parser::OP_LOGICALAND:
			return ScriptVariable{ (left && right) ? 1.0 : 0.0 };
		case GS1Parser::OP_LOGICALOR:
			return ScriptVariable{ (left || right) ? 1.0 : 0.0 };
	}

	throw std::exception("LogicExpression has an unknown operator");
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

	auto left = getScriptVariable<double>(results[0]).value_or(0.0);
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

std::any GS1Visitor::visitPrimary_expression(GS1Parser::Primary_expressionContext* context)
{
	auto result = visitChildren(context);

	// Handle IdentifierArray results.
	// It has to return a double* for assignment operations to work.
	if (auto* v = std::any_cast<double*>(&result); v != nullptr && *v != nullptr)
		return ScriptVariable{ **v };

	return result;
}

std::any GS1Visitor::visitIdentifier(GS1Parser::IdentifierContext* context)
{
	auto visited = visit(context->compound_identifier());
	auto result = getScriptVariable(visited);
	if (!result.has_value() || !std::holds_alternative<std::string>(result.value()))
		throw std::exception("Identifier received an invalid compound_identifier");

	std::string& identifier = std::get<std::string>(result.value());

	// TODO(Nalin): Built-in variables.
	// TODO(Nalin): Handle prefixed variables.

	// Look for the variable in our containers.
	for (auto* container : m_variableContainers)
	{
		if (auto* var = container->get(identifier); var != nullptr)
			return var;
	}

	// No variable found, create a new one.
	if (!m_variableContainers.empty())
		return &m_variableContainers.front()->add(identifier, ScriptVariable{ 0.0 });

	// No variable found, just return 0.
	return ScriptVariable{ 0.0 };
}

std::any GS1Visitor::visitIdentifierArray(GS1Parser::IdentifierArrayContext* context)
{
	auto compound_any = visit(context->compound_identifier());
	auto compound_identifier = getScriptVariable(compound_any);
	if (!compound_identifier.has_value() || !std::holds_alternative<std::string>(compound_identifier.value()))
		throw std::exception("IdentifierArray received an invalid compound_identifier");

	auto expression_any = visit(context->primary_expression());
	auto expression_index = getScriptVariable(expression_any);
	if (!expression_index.has_value() || !std::holds_alternative<double>(expression_index.value()))
		throw std::exception("IdentifierArray received an invalid array index");

	std::string& identifier = std::get<std::string>(compound_identifier.value());
	size_t index = static_cast<size_t>(std::get<double>(expression_index.value()));

	// TODO(Nalin): Built-in variables.

	// Look for the variable in our containers.
	for (auto* container : m_variableContainers)
	{
		if (auto* var = container->get(identifier); var != nullptr)
		{
			if (!std::holds_alternative<std::vector<double>>(*var))
				throw std::exception("IdentifierArray identifier is not an array");
			std::vector<double>& array = std::get<std::vector<double>>(*var);
			if (index >= array.size())
				throw std::exception("IdentifierArray index is out of range");
			return &array[index];
		}
	}

	// No variable found, create a new one.
	if (!m_variableContainers.empty())
		return m_variableContainers.front()->add(identifier, ScriptVariable{ 0.0 });

	// No variable found, just return 0.
	return ScriptVariable{ 0.0 };
}

std::any GS1Visitor::visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context)
{
	std::string identifier;

	auto results = visitChildrenAndCollect(this, context);
	for (auto& piece : results)
	{
		auto var = getScriptVariable(piece);
		if (!var.has_value() || !std::holds_alternative<std::string>(var.value()))
			throw std::exception("CompoundIdentifier chunk was not a valid data type");

		auto& str = std::get<std::string>(var.value());
		identifier.append(str);
	}

	return ScriptVariable{ identifier };
}

std::any GS1Visitor::visitIncDecOperation(GS1Parser::IncDecOperationContext* context)
{
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() == 0 || context->children.size() != 2)
		throw std::exception("IncDecOperation is not a unary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::exception("IncDecOperation has no operation");

	// Try to get our variable.
	// We need to handle double* due to IdentifierArray.
	double* value = nullptr;

	// Try to get our variable.
	ScriptVariable* variable = getScriptVariableUnsafe(results[0]);
	if (variable != nullptr)
	{
		// It needs to hold a double.
		if (!std::holds_alternative<double>(*variable))
			throw std::exception("IncDecOperation has no valid value");

		// Link to the double stored in the variant.
		value = &std::get<double>(*variable);
	}
	// We had no variable, so let's check for a double*.
	else
	{
		auto** arrayvar = std::any_cast<double*>(&results[0]);
		if (arrayvar == nullptr || *arrayvar == nullptr)
			throw std::exception("IncDecOperation has no valid value");

		value = *arrayvar;
	}

	switch (op.value())
	{
		case GS1Parser::OP_INC:
			++(*value);
			break;
		case GS1Parser::OP_DEC:
			--(*value);
			break;
	}

	return variable;
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

		auto* identifier = getScriptVariableUnsafe(results[0]);
		auto value = getScriptVariable<std::string>(results[1]).value_or({});
		if (identifier != nullptr)
			*identifier = value;
	}

	return {};
}

std::any GS1Visitor::visitFunctionDefinition(GS1Parser::FunctionDefinitionContext* context)
{
	throw std::exception("visitFunctionDefinition not implemented");
	return {};
}

std::any GS1Visitor::visitUserFunctionCall(GS1Parser::UserFunctionCallContext* context)
{
	throw std::exception("visitUserFunctionCall not implemented");
	return {};
}

std::any GS1Visitor::visitBuiltInFunctionCall(GS1Parser::BuiltInFunctionCallContext* context)
{
	throw std::exception("visitBuiltInFunctionCall not implemented");
	return {};
}

std::any GS1Visitor::visitIfCondition(GS1Parser::IfConditionContext* context)
{
	auto condition = visit(context->expression());
	if (getScriptVariable<bool>(condition).value_or(false))
		return visit(context->if_true_block());
	else if (auto elseblock = context->else_false_block(); elseblock != nullptr)
		return visit(elseblock);

	return {};
}

std::any GS1Visitor::visitForLoop(GS1Parser::ForLoopContext* context)
{
	// Assignment.
	visit(context->expression(0));

	// Condition.
	size_t loopCount = 0;
	while (loopCount++ < MAX_LOOPS && getScriptVariable<bool>(visit(context->expression(1))).value_or(false))
	{
		// Block.
		try
		{
			visit(context->block());
		}
		catch (break_exception&) { break; }
		catch (continue_exception&) { continue; }

		// Increment.
		visit(context->expression(2));
	}

	return {};
}

std::any GS1Visitor::visitWhileLoop(GS1Parser::WhileLoopContext* context)
{
	// Condition.
	size_t loopCount = 0;
	while (loopCount++ < MAX_LOOPS && getScriptVariable<bool>(visit(context->expression())).value_or(false))
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
	// identifier_ assignment_operator assignment
	auto results = visitChildrenAndCollect(this, context);
	if (results.size() != 2 || context->children.size() != 3)
		throw std::exception("AssignmentOperation is not a binary expression");

	auto op = getSymbolType(context->children[1]);
	if (!op.has_value())
		throw std::exception("AssignmentOperation has no operation");

	// Try to get our identifier.
	// We need to handle double* due to IdentifierArray.
	double* identifier_value_double = nullptr;

	// Try to get our identifier.
	ScriptVariable* identifier = getScriptVariableUnsafe(results[0]);
	if (identifier == nullptr)
	{
		// We had no variable, so let's check for a double*.
		auto** arrayvar = std::any_cast<double*>(&results[0]);
		if (arrayvar == nullptr || *arrayvar == nullptr)
			throw std::exception("AssignmentOperation has no valid value");

		identifier_value_double = *arrayvar;
	}

	// Get the assignment variable.
	auto assignment = getScriptVariable(results[1]);
	if (!assignment.has_value())
		throw std::exception("AssignmentOperation has no assignment value");

	// Direct assignment?  We can abort early.
	if (op.value() == GS1Parser::OP_ASSIGN)
	{
		// Array still needs a special case.
		if (identifier == nullptr)
		{
			if (!std::holds_alternative<double>(assignment.value()))
				throw std::exception("AssignmentOperation assignment does not hold a number");

			*identifier_value_double = std::get<double>(assignment.value());
			return results[0];
		}

		*identifier = assignment.value();
		return identifier;
	}

	// Get a pointer to our identifier so we can alter the variable.
	if (identifier_value_double == nullptr)
	{
		if (!std::holds_alternative<double>(*identifier))
			throw std::exception("AssignmentOperation identifier does not hold a number");
		identifier_value_double = &std::get<double>(*identifier);
	}

	// The mathematical assignments require a double.
	if (!std::holds_alternative<double>(assignment.value()))
		throw std::exception("AssignmentOperation assignment does not hold a number");

	// Handle our values that require a double.
	double assignment_value = std::get<double>(assignment.value());
	switch (op.value())
	{
		case GS1Parser::OP_ASSIGN_ADD:
			*identifier_value_double += assignment_value;
			break;
		case GS1Parser::OP_ASSIGN_SUB:
			*identifier_value_double -= assignment_value;
			break;
		case GS1Parser::OP_ASSIGN_MUL:
			*identifier_value_double *= assignment_value;
			break;
		case GS1Parser::OP_ASSIGN_DIV:
			*identifier_value_double /= assignment_value;
			break;
		case GS1Parser::OP_ASSIGN_MOD:
			*identifier_value_double = static_cast<double>(static_cast<int64_t>(*identifier_value_double) % static_cast<int64_t>(assignment_value));
			break;
		case GS1Parser::OP_ASSIGN_POW:
			*identifier_value_double = std::pow(*identifier_value_double, assignment_value);
			break;
	}

	if (identifier != nullptr)
		return identifier;
	if (identifier_value_double != nullptr)
		return identifier_value_double;

	throw std::exception("AssignmentOperation reached the end");
}

std::any GS1Visitor::visitUnaryOperation(GS1Parser::UnaryOperationContext* context)
{
	auto left = visit(context->unary_operator());
	auto right = visit(context->expression());

	auto symbol = getSymbolType(context->children[0]);
	if (!symbol.has_value())
		throw std::exception("UnaryOperation does not have an operator");

	ScriptVariable zero{ 0.0 };
	auto& value = getScriptVariableOr(right, zero);

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
	return std::make_any<ScriptVariable>(std::move(string::trimMutate(text)));
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
