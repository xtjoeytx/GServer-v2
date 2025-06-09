#ifndef GS1VISITOR_H
#define GS1VISITOR_H

#include <optional>
#include <string_view>
#include <any>
#include <deque>

#undef ERROR
#include <GS1ParserBaseVisitor.h>

#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/ScriptContainers.h>

using namespace preagonal::gs1;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::grammar::gs1
{
///////////////////////////////////////////////////////////////////////////////

class GS1Visitor : public GS1ParserBaseVisitor
{
public:
	void execute(const ScriptEvent& event, ScriptObjectSource source, GS1Parser& parser, antlr4::tree::ParseTree& startNode);

public:
	std::vector<std::string> tokenizeTokens;
	GameVariableStore* builtInStore = nullptr;

public:
	[[inline]] const ScriptObjectSource& getOriginalSource() const;
	[[inline]] const ScriptObjectSource& getCurrentSource() const;
	[[inline]] const ScriptObjectSource& popSource();
	[[inline]] const void pushSource(ScriptObjectSource source);
	[[inline]] const ScriptEvent& getEvent() const;
	std::optional<ScriptObjectSource> findNearestScriptObjectSourceFromStack(ScriptObjectSourceType type);

public:
	template<ValidGameValue T>
	[[inline]] static T getGameValueAs(const GS1ScriptValue& value);

	template<ValidGameValue T>
	[[inline]] T getReadOnlyGameValueFromAnyAs(const std::any& value);

	GameVariable* getGameVariableFromGS1ScriptValue(GS1ScriptValue& value);
	GameVariable* getGameVariableFromVariant(GameVariableVariant& variant);
	GameVariableVariant getGameVariableFromStorage(std::string_view identifier, std::optional<size_t> type = std::nullopt);

public:
	double getColorValueFromString(std::string_view colorString);

public:
	std::vector<std::any> visitChildrenAndCollect(antlr4::tree::ParseTree* node);

protected:
	GS1Parser* m_parser = nullptr;
	const ScriptEvent* m_event = nullptr;
	ScriptObjectSource m_originalSource;
	std::deque<ScriptObjectSource> m_currentSource;
	GameVariableStore* m_serverStore = nullptr;

protected:
	std::any safeVisit(antlr4::tree::ParseTree* node);

protected:
	GameVariableStore* findGameVariableStoreFromSourceStack(ScriptObjectSourceType type);
	GameVariableStore* getGameVariableStoreForStorageType(size_t type);
	GS1GameVariable getGameVariableFromAny(std::any& value);
	GameValue getReadOnlyGameValueFromGS1ScriptValue(const GS1ScriptValue& value);
	GameValue getReadOnlyGameValueFromAny(const std::any& value);
	
public:
	virtual std::any visitMathExpression(GS1Parser::MathExpressionContext* context) override;
	virtual std::any visitComparisonExpression(GS1Parser::ComparisonExpressionContext* context);
	virtual std::any visitLogicExpression(GS1Parser::LogicExpressionContext* context) override;
	virtual std::any visitTernaryExpression(GS1Parser::TernaryExpressionContext* context) override;
	virtual std::any visitInExpression(GS1Parser::InExpressionContext* context) override;
	virtual std::any visitParenthesesExpression(GS1Parser::ParenthesesExpressionContext* context) override;
	virtual std::any visitIdentifierAccess(GS1Parser::IdentifierAccessContext* context) override;
	virtual std::any visitIdentifierValue(GS1Parser::IdentifierValueContext* context) override;
	virtual std::any visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context) override;
	virtual std::any visitCompoundString(GS1Parser::CompoundStringContext* context) override;
	virtual std::any visitIncDecOperation(GS1Parser::IncDecOperationContext* context) override;
	virtual std::any visitBuiltInCommand(GS1Parser::BuiltInCommandContext* context) override;
	virtual std::any visitUserFunctionCall(GS1Parser::UserFunctionCallContext* context) override;
	virtual std::any visitBuiltInFunctionCall(GS1Parser::BuiltInFunctionCallContext* context) override;
	virtual std::any visitIfCondition(GS1Parser::IfConditionContext* context) override;
	virtual std::any visitForLoop(GS1Parser::ForLoopContext* context) override;
	virtual std::any visitWhileLoop(GS1Parser::WhileLoopContext* context) override;
	virtual std::any visitWithStatement(GS1Parser::WithStatementContext* context) override;
	virtual std::any visitFlowReturn(GS1Parser::FlowReturnContext* context) override;
	virtual std::any visitFlowBreak(GS1Parser::FlowBreakContext* context) override;
	virtual std::any visitFlowContinue(GS1Parser::FlowContinueContext* context) override;
	virtual std::any visitAssignmentOperation(GS1Parser::AssignmentOperationContext* context) override;
	virtual std::any visitUnaryOperation(GS1Parser::UnaryOperationContext* context) override;
	virtual std::any visitMessageCode(GS1Parser::MessageCodeContext* context) override;
	virtual std::any visitLiteral(GS1Parser::LiteralContext* context) override;
	virtual std::any visitLiteralAllFeatures(GS1Parser::LiteralAllFeaturesContext* context) override;
	virtual std::any visitLiteralAllStats(GS1Parser::LiteralAllStatsContext* context) override;
	virtual std::any visitStringLiteral(GS1Parser::StringLiteralContext* context) override;
	virtual std::any visitIdentifierLiteral(GS1Parser::IdentifierLiteralContext* context) override;
	virtual std::any visitRangeLiteral(GS1Parser::RangeLiteralContext* context) override;
	virtual std::any visitArrayLiteral(GS1Parser::ArrayLiteralContext* context) override;
	virtual std::any visitItemLiteral(GS1Parser::ItemLiteralContext* context) override;
	virtual std::any visitCarryLiteral(GS1Parser::CarryLiteralContext* context) override;
	virtual std::any visitDirectionLiteral(GS1Parser::DirectionLiteralContext* context) override;
	virtual std::any visitGenderLiteral(GS1Parser::GenderLiteralContext* context) override;
	virtual std::any visitColorLiteral(GS1Parser::ColorLiteralContext* context) override;
	virtual std::any visitBaddyLiteral(GS1Parser::BaddyLiteralContext* context) override;
	virtual std::any visitStorageToken(GS1Parser::StorageTokenContext* context) override;
};

//----------------------------

template<ValidGameValue T>
inline auto makeDefault() -> T
{
	if constexpr (std::is_same_v<T, double>)
		return 0.0;
	else if constexpr (std::is_same_v<T, std::string>)
		return std::string{};
	else
		return T{};
}

//----------------------------

inline const ScriptObjectSource& GS1Visitor::getOriginalSource() const
{
	return m_originalSource;
}

inline const ScriptObjectSource& GS1Visitor::getCurrentSource() const
{
	return m_currentSource.empty() ? m_originalSource : m_currentSource.back();
}

inline const ScriptObjectSource& GS1Visitor::popSource()
{
	m_currentSource.pop_back();
	return getCurrentSource();
}

inline const void GS1Visitor::pushSource(ScriptObjectSource source)
{
	m_currentSource.emplace_back(std::move(source));
}

inline const ScriptEvent& GS1Visitor::getEvent() const
{
	return *m_event;
}

template<ValidGameValue T>
inline static T GS1Visitor::getGameValueAs(const GS1ScriptValue& value)
{
	if (const auto* gs1Pair = std::get_if<GS1GameVariable>(&value); gs1Pair != nullptr)
	{
		const auto* gameVariant = &gs1Pair->first;
		const GameVariable* gameVar = nullptr;
		if (const auto* byVal = std::get_if<GameVariable>(gameVariant); byVal != nullptr)
			gameVar = byVal;
		else if (const auto* byPtr = std::get_if<std::weak_ptr<GameVariable>>(gameVariant); byPtr != nullptr)
		{
			if (auto lock = byPtr->lock(); lock != nullptr)
				gameVar = lock.get();
		}

		if (gameVar != nullptr)
			return gameVar->get<T>(gs1Pair->second).value_or(makeDefault<T>());
	}
	else if (auto* gameValue = std::get_if<GameValue>(&value); gameValue != nullptr)
		return gameValue->get<T>().value_or(makeDefault<T>());
	return makeDefault<T>();
}

//----------------------------

template<ValidGameValue T>
inline T GS1Visitor::getReadOnlyGameValueFromAnyAs(const std::any& value)
{
	auto gameval = getReadOnlyGameValueFromAny(value);
	return gameval.get<T>().value_or(makeDefault<T>());
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::grammar::gs1

#endif // GS1VISITOR_H
