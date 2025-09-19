#ifndef GS1VISITOR_H
#define GS1VISITOR_H

#include <any>
#include <deque>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <GS1Parser.h>
#include <GS1ParserBaseVisitor.h>
#include <tree/ParseTree.h>

#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/StringUtils.h>

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
	void execute(const ScriptEvent& event, ScriptObject source, GS1Parser& parser, antlr4::tree::ParseTree* startNode);
	void reportError(std::string_view message, antlr4::tree::ParseTree* node = nullptr, bool abort = true);

public:
	std::vector<std::string> tokenizeTokens;
	GameVariableStore flagStore;
	GameVariableStore* builtInStore = nullptr;
	bool expectingFlag = false;
	bool expectingTimeoutAsVariable = false;
	std::string who;

public:
	[[inline]] const ScriptObject& getOriginalSource() const;
	[[inline]] const ScriptObject& getInitiatingSource() const;
	[[inline]] const ScriptObject& getCurrentSource(bool defaultToInitiator = false) const;
	[[inline]] const ScriptObject& popSource();
	[[inline]] const void pushSource(ScriptObject source);
	[[inline]] const ScriptEvent& getEvent() const;
	[[inline]] auto sourceStack() const;
	std::optional<ScriptObject> findNearestScriptObjectSourceFromStack(ScriptObjectType type) const;
	std::shared_ptr<Level> findCurrentLevel() const;

public:
	template<ValidGameValue T>
	[[inline]] static T getGameValueAs(const GS1ScriptValue& value);

	template<ValidGameValue T>
	[[inline]] T getReadOnlyGameValueFromAnyAs(const std::any& value);

	GameValue* getGameValueFromGS1ScriptValue(GS1ScriptValue& value);
	std::optional<GameValue> getGameValueFromSource(const ScriptObject& source, std::string_view identifier);
	GameValue getGameValueFromStorage(std::string_view identifier, std::optional<size_t> type = std::nullopt);

public:
	[[inline]] size_t getStorageFromTypeString(std::string_view storageType) const;
	GameVariableStore* getGameVariableStoreForStorageType(size_t type);
	double getColorValueFromString(std::string_view colorString);

public:
	std::vector<std::any> visitChildrenAndCollect(antlr4::tree::ParseTree* node);

protected:
	GS1Parser* m_parser = nullptr;
	const ScriptEvent* m_event = nullptr;
	ScriptObject m_originalSource;
	GameVariableStore* m_serverStore = nullptr;
	std::deque<ScriptObject> m_currentSource;
	std::deque<ScriptObject> m_sleepCurrentSource;
	std::vector<std::pair<antlr4::tree::ParseTree*, size_t>> m_callStack;
	std::vector<std::pair<antlr4::tree::ParseTree*, size_t>> m_sleepCallStack;

protected:
	std::any safeVisit(antlr4::tree::ParseTree* node);

protected:
	GameVariableStore* findGameVariableStoreFromSourceStack(ScriptObjectType type) const;
	GS1GameVariable getGameVariableFromAny(std::any& value);
	GameValue getReadOnlyGameValueFromGS1ScriptValue(const GS1ScriptValue& value);
	GameValue getReadOnlyGameValueFromAny(const std::any& value);
	std::optional<ScriptObject> getSourceFromGS1ScriptValue(GS1ScriptValue& value);

protected:
	void setCurrentPlayerVariables(std::optional<ScriptObject> source);

public:
	virtual std::any visitBlock(GS1Parser::BlockContext* ctx) override;
	//
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
	//
	virtual std::any visitPrimaryExpression(GS1Parser::PrimaryExpressionContext* context) override;
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

inline const ScriptObject& GS1Visitor::getOriginalSource() const
{
	return m_originalSource;
}

inline const ScriptObject& GS1Visitor::getInitiatingSource() const
{
	return m_event->initiator;
}

inline const ScriptObject& GS1Visitor::getCurrentSource(bool defaultToInitiator) const
{
	if (m_event->initiator.second == ScriptObjectType::NPC)
		defaultToInitiator = false;
	return m_currentSource.empty() ? (defaultToInitiator ? m_event->initiator : m_originalSource) : m_currentSource.back();
}

inline const ScriptObject& GS1Visitor::popSource()
{
	m_currentSource.pop_back();
	return getCurrentSource();
}

inline const void GS1Visitor::pushSource(ScriptObject source)
{
	m_currentSource.emplace_back(std::move(source));
}

inline auto GS1Visitor::sourceStack() const
{
	// Save me C++26...
	std::vector<ScriptObject> sources{ m_currentSource.rbegin(), m_currentSource.rend() };
	if (m_event->initiator.second != ScriptObjectType::NPC)
		sources.push_back(m_event->initiator);
	sources.push_back(m_originalSource);
	return sources;
}

inline const ScriptEvent& GS1Visitor::getEvent() const
{
	return *m_event;
}

template<ValidGameValue T>
inline T GS1Visitor::getGameValueAs(const GS1ScriptValue& value)
{
	if (const auto* gs1Pair = std::get_if<GS1GameVariable>(&value); gs1Pair != nullptr)
		return gs1Pair->first.get<T>(gs1Pair->second).value_or(makeDefault<T>());
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

//----------------------------

inline size_t GS1Visitor::getStorageFromTypeString(std::string_view storageType) const
{
	if (storageType.empty())
		return GS1Parser::STORAGE_CLIENT;
	if (string::comparei(storageType, "this"sv) == 0)
		return GS1Parser::STORAGE_THIS;
	if (string::comparei(storageType, "thiso"sv) == 0)
		return GS1Parser::STORAGE_THISO;
	if (string::comparei(storageType, "client"sv) == 0)
		return GS1Parser::STORAGE_CLIENT;
	if (string::comparei(storageType, "clientr"sv) == 0)
		return GS1Parser::STORAGE_CLIENTR;
	if (string::comparei(storageType, "cliento"sv) == 0)
		return GS1Parser::STORAGE_CLIENTO;
	if (string::comparei(storageType, "clientro"sv) == 0)
		return GS1Parser::STORAGE_CLIENTRO;
	if (string::comparei(storageType, "server"sv) == 0)
		return GS1Parser::STORAGE_SERVER;
	if (string::comparei(storageType, "serverr"sv) == 0)
		return GS1Parser::STORAGE_SERVERR;
	if (string::comparei(storageType, "local"sv) == 0)
		return GS1Parser::STORAGE_LOCAL;
	if (string::comparei(storageType, "temp"sv) == 0)
		return GS1Parser::STORAGE_TEMP;

	return GS1Parser::STORAGE_CLIENT;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar

#endif // GS1VISITOR_H
