#ifndef GS1VISITOR_H
#define GS1VISITOR_H

#include <optional>
#include <string_view>
#include <any>

#include <GS1ParserBaseVisitor.h>

#include <scripting/ScriptContainers.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::grammar::gs1
{
///////////////////////////////////////////////////////////////////////////////

class GS1Visitor : public GS1ParserBaseVisitor
{
public:
	void execute(const ScriptEvent& event, ScriptEventSource source, GS1Parser& parser, antlr4::tree::ParseTree& startNode, ScriptVariableStore* defaultStore, ScriptVariableStoreMap* variableStores);

protected:
	GS1Parser* m_parser = nullptr;
	ScriptEventSource m_source{};
	ScriptVariableStore* m_defaultStore = nullptr;
	ScriptVariableStoreMap* m_variableStores = nullptr;

protected:
	std::any safeVisit(antlr4::tree::ParseTree* node);
	ScriptVariableContainer& bindLinkToVariableStore(ScriptVariableContainer& container);
	ScriptVariable getVariableFromStores(const ScriptIdentifier& identifier);

protected:
	static bool identifierHasDataType(const ScriptIdentifier& identifier);
	static std::string getTypedIdentifier(const ScriptIdentifier& identifier, std::string_view dataType);
	static ScriptVariableContainer& assignIdentifierType(ScriptVariableContainer& container, std::string_view dataType);

protected:
	template<typename T = double>
	ScriptVariableContainer getVariableContainerFromStores(const ScriptIdentifier& identifier)
	{
		if (auto* str = std::get_if<std::string>(&identifier); str != nullptr && str->contains('|'))
		{
			if (auto var = retrieveVariableFromStore(identifier, m_defaultStore, m_variableStores); var.has_value())
				return var.value();
		}

		if (auto* pair = std::get_if<std::pair<std::string, size_t>>(&identifier); pair != nullptr && pair->first.contains('|'))
		{
			if (auto var = retrieveVariableFromStore(identifier, m_defaultStore, m_variableStores); var.has_value())
			{
				if (auto* doubles = std::get_if<std::vector<double>>(&var.value().get()); doubles != nullptr)
				{
					if (pair->second < doubles->size())
						return ScriptVariableContainer{ doubles->at(pair->second) };
				}
			}
		}

		if constexpr (std::same_as<T, double> || std::same_as<T, bool>)
			return ScriptVariableContainer{ 0.0 };
		if constexpr (std::same_as<T, std::string>)
			return ScriptVariableContainer{ ScriptVariable{ std::string{ "" } } };
		if constexpr (std::same_as<T, std::vector<double>>)
			return ScriptVariableContainer{ std::vector<double>{} };

		throw std::exception("getVariableFromStores variable was not in store and the data type is not valid");
	}

	/// <summary>
	/// Tries to get the value within a ScriptVariable inside a ScriptVariableContainer.
	/// If no identifier data type is set, it will try to guess the type based on the default value passed.
	/// </summary>
	/// <param name="container">The container to retrieve the value from.</param>
	/// <param name="defaultValue">The default value to return if no value could be found.</param>
	/// <returns>A value contained in the ScriptVariable, or defaultValue if none found.</returns>
	auto gs1TryGetScriptVariableValueFromContainer(ScriptVariableContainer& container, auto defaultValue) -> decltype(defaultValue)
	{
		if (container.hasIdentifier())
		{
			// Try to assign the data type if we don't have one.
			if (!identifierHasDataType(container.getIdentifier().value()))
			{
				if constexpr (std::same_as<decltype(defaultValue), double> || std::same_as<decltype(defaultValue), bool>)
				{
					const auto& identifier = container.getIdentifier();
					if (std::holds_alternative<std::string>(identifier.value()))
						assignIdentifierType(container, "double");
					else assignIdentifierType(container, "array");
				}
				else if constexpr (std::same_as<decltype(defaultValue), std::string>)
					assignIdentifierType(container, "string");
			}

			// Attempt to link the identifier to a variable store.
			bindLinkToVariableStore(container);
			if (!container.hasGetter())
			{
				container.setGetter(bindGetter());
				container.retrieveFromGetter();
			}
		}
		return container.get<decltype(defaultValue)>();
	}

	/// <summary>
	/// Tries to get the value within a ScriptVariable inside a ScriptVariableContainer contained inside a std::any.
	/// If no identifier data type is set, it will try to guess the type based on the default value passed.
	/// </summary>
	/// <param name="anyval">The std::any instance that contains the ScriptVariableContainer.</param>
	/// <param name="defaultValue">The default value to return if no value could be found.</param>
	/// <returns>A value contained in the ScriptVariableContainer, or defaultValue if none found.</returns>
	auto gs1TryGetScriptVariableValueFromAny(std::any& anyval, auto defaultValue) -> decltype(defaultValue)
	{
		auto* container = getScriptVariableContainerUnsafe(anyval);
		if (container == nullptr) return defaultValue;
		return gs1TryGetScriptVariableValueFromContainer(*container, defaultValue);
	}

	/// <summary>
	/// Tries to get the value within a ScriptVariable inside a ScriptVariableContainer contained inside a std::any.
	/// If no identifier data type is set, it will try to guess the type based on the default value passed.
	/// </summary>
	/// <param name="anyval">The std::any instance that contains the ScriptVariableContainer.</param>
	/// <param name="defaultValue">The default value to return if no value could be found.</param>
	/// <returns>A value contained in the ScriptVariableContainer, or defaultValue if none found.</returns>
	auto gs1TryGetScriptVariableValueFromAny(std::any&& anyval, auto defaultValue) -> decltype(defaultValue)
	{
		auto* attempt = std::any_cast<ScriptVariableContainer>(&anyval);
		if (attempt == nullptr) return defaultValue;
		ScriptVariableContainer container{ *attempt };
		return gs1TryGetScriptVariableValueFromContainer(container, defaultValue);
	}

	constexpr auto bindGetter()
	{
		return std::bind(&GS1Visitor::getVariableFromStores, this, std::placeholders::_1);
	}

public:
	//virtual std::any visitProgram(GS1Parser::ProgramContext* context) = 0;
	//virtual std::any visitBlock(GS1Parser::BlockContext* context) = 0;
	//virtual std::any visitStatement(GS1Parser::StatementContext* context) = 0;
	//virtual std::any visitExpression(GS1Parser::ExpressionContext* context) = 0;
	//virtual std::any visitUnary_expression(GS1Parser::Unary_expressionContext* context) = 0;
	//virtual std::any visitPostfix_expression(GS1Parser::Postfix_expressionContext* context) = 0;
	//virtual std::any visitIgnoreUnaryExpression(GS1Parser::IgnoreUnaryExpressionContext* context) = 0;
	//virtual std::any visitIdentifier_(GS1Parser::Identifier_Context* context) = 0;
	//virtual std::any visitIn_expression(GS1Parser::In_expressionContext* context) = 0;
	//virtual std::any visitBuiltin_command_expression(GS1Parser::Builtin_command_expressionContext* context) = 0;
	//virtual std::any visitBuiltin_function_parameters(GS1Parser::Builtin_function_parametersContext* context) = 0;
	//virtual std::any visitIf_true_block(GS1Parser::If_true_blockContext* context) = 0;
	//virtual std::any visitElse_false_block(GS1Parser::Else_false_blockContext* context) = 0;
	//virtual std::any visitAssignment_operator(GS1Parser::Assignment_operatorContext* context) = 0;
	//virtual std::any visitCompound_string(GS1Parser::Compound_stringContext* context) = 0;
	//virtual std::any visitUnary_operator(GS1Parser::Unary_operatorContext* context) = 0;
	//virtual std::any visitAssignment(GS1Parser::AssignmentContext* context) = 0;
	//virtual std::any visitFunction_definition(GS1Parser::Function_definitionContext* context) = 0;
	//virtual std::any visitPrimary_expression(GS1Parser::Primary_expressionContext* context) = 0;
	//virtual std::any visitIdentifier(GS1Parser::IdentifierContext* context) = 0;
	virtual std::any visitMathExpression(GS1Parser::MathExpressionContext* context) override;
	virtual std::any visitComparisonExpression(GS1Parser::ComparisonExpressionContext* context);
	virtual std::any visitLogicExpression(GS1Parser::LogicExpressionContext* context) override;
	virtual std::any visitTernaryExpression(GS1Parser::TernaryExpressionContext* context) override;
	virtual std::any visitInExpression(GS1Parser::InExpressionContext* context) override;
	virtual std::any visitParenthesesExpression(GS1Parser::ParenthesesExpressionContext* context) override;
	virtual std::any visitIdentifierArray(GS1Parser::IdentifierArrayContext* context) override;
	virtual std::any visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context) override;
	virtual std::any visitIncDecOperation(GS1Parser::IncDecOperationContext* context) override;
	virtual std::any visitBuiltInCommand(GS1Parser::BuiltInCommandContext* context) override;
	virtual std::any visitUserFunctionCall(GS1Parser::UserFunctionCallContext* context) override;
	virtual std::any visitBuiltInFunctionCall(GS1Parser::BuiltInFunctionCallContext* context) override;
	virtual std::any visitIfCondition(GS1Parser::IfConditionContext* context) override;
	virtual std::any visitForLoop(GS1Parser::ForLoopContext* context) override;
	virtual std::any visitWhileLoop(GS1Parser::WhileLoopContext* context) override;
	virtual std::any visitFlowReturn(GS1Parser::FlowReturnContext* context) override;
	virtual std::any visitFlowBreak(GS1Parser::FlowBreakContext* context) override;
	virtual std::any visitFlowContinue(GS1Parser::FlowContinueContext* context) override;
	virtual std::any visitAssignmentOperation(GS1Parser::AssignmentOperationContext* context) override;
	virtual std::any visitUnaryOperation(GS1Parser::UnaryOperationContext* context) override;
	virtual std::any visitMessageCode(GS1Parser::MessageCodeContext* context) override;
	virtual std::any visitLiteral(GS1Parser::LiteralContext* context) override;
	virtual std::any visitLiteralAllFeatures(GS1Parser::LiteralAllFeaturesContext* context) override;
	virtual std::any visitStringLiteral(GS1Parser::StringLiteralContext* context) override;
	virtual std::any visitIdentifierLiteral(GS1Parser::IdentifierLiteralContext* context) override;
	virtual std::any visitRangeLiteral(GS1Parser::RangeLiteralContext* context) override;
	virtual std::any visitArrayLiteral(GS1Parser::ArrayLiteralContext* context) override;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::grammar::gs1

#endif // GS1VISITOR_H
