#ifndef GS1VISITOR_H
#define GS1VISITOR_H

#include <any>
#include <deque>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <GS1Parser.h>
#include <GS1ParserBaseVisitor.h>
#include <tree/ParseTree.h>

#include <scripting/ScriptContainers.h>
#include <scripting/gs1/ScriptEngineGS1.h>

namespace preagonal
{
class Level;
class Character;
}

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

class GS1Visitor : public GS1ParserBaseVisitor
{
public:
	void execute(const ScriptEvent& event, ScriptObjectSource source, GS1Parser& parser, antlr4::tree::ParseTree& startNode);
	void reportError(std::string_view message, antlr4::tree::ParseTree* node = nullptr, bool abort = true);

public:
	std::vector<std::string> tokenizeTokens;
	GameVariableStore* builtInStore = nullptr;
	bool expectingFlag = false;
	std::string who;

public:
	[[inline]] const ScriptObjectSource& getOriginalSource() const;
	[[inline]] const ScriptObjectSource& getCurrentSource(bool defaultToInitiator = false) const;
	[[inline]] const ScriptObjectSource& popSource();
	[[inline]] const void pushSource(ScriptObjectSource source);
	[[inline]] const ScriptEvent& getEvent() const;
	[[inline]] auto sourceStack() const;
	std::optional<ScriptObjectSource> findNearestScriptObjectSourceFromStack(ScriptObjectSourceType type) const;
	std::shared_ptr<Level> findCurrentLevel() const;

public:
	template<ValidGameValue T>
	[[inline]] static T getGameValueAs(const GS1ScriptValue& value);

	template<ValidGameValue T>
	[[inline]] T getReadOnlyGameValueFromAnyAs(const std::any& value);

	GameVariable* getGameVariableFromGS1ScriptValue(GS1ScriptValue& value);
	GameVariable* getGameVariableFromVariant(GameVariableVariant& variant);
	std::optional<GameVariable> getGameVariableFromSource(const ScriptObjectSource& source, std::string_view identifier);
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
	GameVariableStore* findGameVariableStoreFromSourceStack(ScriptObjectSourceType type) const;
	GameVariableStore* getGameVariableStoreForStorageType(size_t type);
	GS1GameVariable getGameVariableFromAny(std::any& value);
	GameValue getReadOnlyGameValueFromGS1ScriptValue(const GS1ScriptValue& value);
	GameValue getReadOnlyGameValueFromAny(const std::any& value);
	std::optional<ScriptObjectSource> getSourceFromGS1ScriptValue(GS1ScriptValue& value);

protected:
	void setCurrentPlayerVariables(std::optional<ScriptObjectSource> source);

public:
	virtual std::any visitStatementIf(GS1Parser::StatementIfContext* context) override;
	virtual std::any visitStatementFor(GS1Parser::StatementForContext* context) override;
	virtual std::any visitStatementWhile(GS1Parser::StatementWhileContext* context) override;
	virtual std::any visitStatementWith(GS1Parser::StatementWithContext* context) override;
	virtual std::any visitStatementFunctionDefinition(GS1Parser::StatementFunctionDefinitionContext* context) override;
	virtual std::any visitStatementUserFunctionCall(GS1Parser::StatementUserFunctionCallContext* context) override;
	virtual std::any visitStatementBuiltInCommand(GS1Parser::StatementBuiltInCommandContext* context) override;
	virtual std::any visitStatementAssignment(GS1Parser::StatementAssignmentContext* context) override;
	//
	virtual std::any visitExpressionIn(GS1Parser::ExpressionInContext* context) override;
	virtual std::any visitExpressionTernary(GS1Parser::ExpressionTernaryContext* context) override;
	virtual std::any visitExpressionLogicOr(GS1Parser::ExpressionLogicOrContext* context) override;
	virtual std::any visitExpressionLogicAnd(GS1Parser::ExpressionLogicAndContext* context) override;
	virtual std::any visitExpressionEquality(GS1Parser::ExpressionEqualityContext* context) override;
	virtual std::any visitExpressionRelational(GS1Parser::ExpressionRelationalContext* context) override;
	virtual std::any visitExpressionAdditive(GS1Parser::ExpressionAdditiveContext* context) override;
	virtual std::any visitExpressionMultiplicative(GS1Parser::ExpressionMultiplicativeContext* context) override;
	virtual std::any visitExpressionExponentiation(GS1Parser::ExpressionExponentiationContext* context) override;
	virtual std::any visitExpressionUnary(GS1Parser::ExpressionUnaryContext* context) override;
	virtual std::any visitExpressionPostfix(GS1Parser::ExpressionPostfixContext* context) override;
	//
	virtual std::any visitBuiltInFunctionCall(GS1Parser::BuiltInFunctionCallContext* context) override;
	virtual std::any visitIdentifierAccess(GS1Parser::IdentifierAccessContext* context) override;
	virtual std::any visitIdentifierValue(GS1Parser::IdentifierValueContext* context) override;
	virtual std::any visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context) override;
	virtual std::any visitCompoundString(GS1Parser::CompoundStringContext* context) override;
	virtual std::any visitMessageCode(GS1Parser::MessageCodeContext* context) override;
	//
	virtual std::any visitFlowReturn(GS1Parser::FlowReturnContext* context) override;
	virtual std::any visitFlowBreak(GS1Parser::FlowBreakContext* context) override;
	virtual std::any visitFlowContinue(GS1Parser::FlowContinueContext* context) override;
	//
	virtual std::any visitLiteral(GS1Parser::LiteralContext* context) override;
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

inline const ScriptObjectSource& GS1Visitor::getCurrentSource(bool defaultToInitiator) const
{
	return m_currentSource.empty() ? (defaultToInitiator ? m_event->initiator : m_originalSource) : m_currentSource.back();
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

inline auto GS1Visitor::sourceStack() const
{
	// Save me C++26...
	std::vector<ScriptObjectSource> sources{ m_currentSource.rbegin(), m_currentSource.rend() };
	sources.push_back(m_event->initiator);
	sources.push_back(m_originalSource);
	return sources;
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

struct unimplemented_error : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar

#endif // GS1VISITOR_H
