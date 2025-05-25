#ifndef GS1VISITOR_H
#define GS1VISITOR_H

#include <optional>
#include <string_view>
#include <any>
#include <stack>

#undef ERROR
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

public:
	std::vector<std::string> tokens;

public:
	const ScriptEventSource& getOriginalSource() const
	{
		return m_originalSource;
	}

	const ScriptEventSource& getCurrentSource() const
	{
		return m_currentSource.empty() ? m_originalSource : m_currentSource.top();
	}

	const ScriptEvent& getEvent() const
	{
		return *m_event;
	}

public:
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

	constexpr auto bindGetter()
	{
		return std::bind(&GS1Visitor::getVariableFromStores, this, std::placeholders::_1);
	}

protected:
	GS1Parser* m_parser = nullptr;
	const ScriptEvent* m_event = nullptr;
	ScriptEventSource m_originalSource;
	std::stack<ScriptEventSource> m_currentSource;
	ScriptVariableStore* m_defaultStore = nullptr;
	ScriptVariableStoreMap* m_variableStores = nullptr;

protected:
	std::any safeVisit(antlr4::tree::ParseTree* node);
	ScriptVariableContainer& bindLinkToVariableStore(ScriptVariableContainer& container);
	ScriptVariable getVariableFromStores(const ScriptIdentifier& identifier);
	ScriptVariableContainer& fixBindAndGetVariable(ScriptVariableContainer& container);

protected:
	static bool identifierHasDataType(const ScriptIdentifier& identifier);
	static std::string getTypedIdentifier(const ScriptIdentifier& identifier, std::string_view dataType);
	static ScriptVariableContainer& assignIdentifierType(ScriptVariableContainer& container, std::string_view dataType);

public:
	virtual std::any visitMathExpression(GS1Parser::MathExpressionContext* context) override;
	virtual std::any visitComparisonExpression(GS1Parser::ComparisonExpressionContext* context);
	virtual std::any visitLogicExpression(GS1Parser::LogicExpressionContext* context) override;
	virtual std::any visitTernaryExpression(GS1Parser::TernaryExpressionContext* context) override;
	virtual std::any visitInExpression(GS1Parser::InExpressionContext* context) override;
	virtual std::any visitParenthesesExpression(GS1Parser::ParenthesesExpressionContext* context) override;
	virtual std::any visitIdentifierArray(GS1Parser::IdentifierArrayContext* context) override;
	virtual std::any visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context) override;
	virtual std::any visitCompoundString(GS1Parser::CompoundStringContext* context) override;
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
