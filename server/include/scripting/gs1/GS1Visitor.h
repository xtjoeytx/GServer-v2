#ifndef GS1VISITOR_H
#define GS1VISITOR_H

#include <any>
#include <cstdint>
#include <deque>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <GS1Parser.h>
#include <GS1ParserBaseVisitor.h>
#include <tree/ParseTree.h>

#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

namespace preagonal
{
class Character;
class GuildManager;
class ITranslationManager;
class Level;
class Server;
class StaticLevelData;
class SubLevel;
} // namespace preagonal

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

enum class StorageType : uint8_t
{
	THIS,
	THISO,
	CLIENT,
	CLIENTR,
	CLIENTO,
	CLIENTRO,
	SERVER,
	SERVERR,
	LEVEL,
	LOCAL,
	TEMP,
};

class GS1Visitor : public GS1ParserBaseVisitor
{
public:
	GS1Visitor();

public:
	void execute(const ScriptEvent& event, ScriptObject source, GS1Parser& parser, ScriptExecutionContext& context, antlr4::tree::ParseTree* startNode);
	void reportError(std::string_view message, antlr4::tree::ParseTree* node = nullptr, bool abort = true);

public:
	std::vector<std::string> tokenizeTokens;
	GameVariableStore flagStore;
	GameVariableStore* builtInStore = nullptr;
	ScriptExecutionContext* scriptContext = nullptr;
	bool expectingFlag = false;
	bool expectingTimeoutAsVariable = false;
	std::string who;

public:
	Server* server = nullptr;
	ITranslationManager* translationManager = nullptr;
	GuildManager* guildManager = nullptr;

public:
	[[inline]] static std::optional<size_t> getStorageTypeFromIdentifier(std::string_view identifier, std::optional<size_t> defaultValue = {}) noexcept;
	[[inline]] static void applyStorageNameToIdentifier(std::optional<size_t> storage, std::string& identifier) noexcept;
	[[inline]] static void stripStorageNameFromIdentifier(std::string& identifier) noexcept;
	static double getColorValueFromString(std::string_view colorString);
	static GameVariable* getGameVariable(std::any& value);
	static GameVariable* getGameVariable(GS1ScriptValue& value);
	static const GameVariable* getGameVariable(const std::any& value);
	static const GameVariable* getGameVariable(const GS1ScriptValue& value);
	static std::optional<ScriptObject> getScriptObject(std::any& value);
	static std::optional<ScriptObject> getScriptObject(GS1ScriptValue& value);
	static std::optional<ScriptObject> getScriptObject(GameVariable& value);
	static bool isGameValue(const GS1ScriptValue& value);
	static bool isScriptObject(const GS1ScriptValue& value);

	template<StoresInGameValue T>
	[[inline]] static bool scriptValueContains(std::any& value);

	template<StoresInGameValue T>
	[[inline]] static bool scriptValueContains(GS1ScriptValue& value);

	template<StoresInGameValue T>
	[[inline]] static std::optional<std::reference_wrapper<T>> getScriptValueAs(std::any& value);

	template<StoresInGameValue T>
	[[inline]] static std::optional<std::reference_wrapper<T>> getScriptValueAs(GS1ScriptValue& value);

	template<StoresInGameValue T>
	[[inline]] static std::optional<T> getScriptValueAsCopy(const std::any& value);

	template<StoresInGameValue T>
	[[inline]] static std::optional<T> getScriptValueAsCopy(const GS1ScriptValue& value);

public:
	[[inline]] const ScriptEvent& getEvent() const;
	[[inline]] const ScriptObject& getOriginalSource() const;
	[[inline]] const ScriptObject& getInitiatingSource() const;
	[[inline]] const ScriptObject& getCurrentSource(bool defaultToInitiator = false) const;
	[[inline]] const ScriptObject& popSource();
	[[inline]] const void pushSource(ScriptObject source);
	[[inline]] auto sourceStack() const;
	[[inline]] bool hasSleepStack() const;
	std::optional<ScriptObject> findNearestScriptObjectSourceFromStack(ScriptObjectType type) const;
	GameVariableStore* findGameVariableStoreFromStack(ScriptObjectType type, int skip = 0) const;
	GameVariableStore* getGameVariableStoreForStorageType(size_t type);

public:
	std::shared_ptr<Level> findCurrentLevel() const;
	std::tuple<std::shared_ptr<Level>, std::shared_ptr<SubLevel>, std::shared_ptr<StaticLevelData>> findCurrentLevelData() const;

public:
	GameVariable* getGameVariableFromSource(const ScriptObject& source, std::string_view identifier) const;
	GameVariable* getGameVariableFromStorage(std::string_view identifier, std::optional<size_t> type = std::nullopt);

public:
	GameValue translateSourceText(antlr4::tree::ParseTree* node, std::string_view language);
	GameValue translateSourceText(std::string_view sourceText, std::string_view language);
	GameValue processStringExpression(std::string_view expression);
	GameValue processMathExpression(std::string_view expression);

	template<class T = GS1Parser::CompoundStringContext>
	[[inline]] T* walkToContext(antlr4::tree::ParseTree* node);

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
	bool m_reparsingStringExpression = false;
	//bool m_reparsingMathExpression = false;

protected:
	std::any safeVisit(antlr4::tree::ParseTree* node);
	std::any reparseExpression(std::string_view expression, std::string_view lexerMode, std::function<antlr4::tree::ParseTree*(GS1Parser&)> node);
	void setCurrentPlayerVariables(std::optional<ScriptObject> source);

public:
	virtual std::any visitProgram(GS1Parser::ProgramContext* ctx) override;
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
	//
	virtual std::any visitPrimaryExpression(GS1Parser::PrimaryExpressionContext* context) override;
};

//----------------------------

template<StoresInGameValue T>
inline auto makeDefault() -> T
{
	if constexpr (std::is_same_v<T, double>)
		return 0.0;
	else if constexpr (std::is_same_v<T, std::string>)
		return std::string{};
	else
		return T{};
}

//---[ STATIC ]---------------

inline std::optional<size_t> GS1Visitor::getStorageTypeFromIdentifier(std::string_view identifier, std::optional<size_t> defaultValue) noexcept
{
	if (identifier.empty() || !identifier.contains('.'))
		return defaultValue;
	if (string::starts_withi(identifier, "this."sv))
		return ENUM(StorageType::THIS);
	if (string::starts_withi(identifier, "thiso."sv))
		return ENUM(StorageType::THISO);
	if (string::starts_withi(identifier, "client."sv))
		return ENUM(StorageType::CLIENT);
	if (string::starts_withi(identifier, "clientr."sv))
		return ENUM(StorageType::CLIENTR);
	if (string::starts_withi(identifier, "cliento."sv))
		return ENUM(StorageType::CLIENTO);
	if (string::starts_withi(identifier, "clientro."sv))
		return ENUM(StorageType::CLIENTRO);
	if (string::starts_withi(identifier, "server."sv))
		return ENUM(StorageType::SERVER);
	if (string::starts_withi(identifier, "serverr."sv))
		return ENUM(StorageType::SERVERR);
	if (string::starts_withi(identifier, "local."sv))
		return ENUM(StorageType::LOCAL);
	if (string::starts_withi(identifier, "temp."sv))
		return ENUM(StorageType::TEMP);

	return defaultValue;
}

inline void GS1Visitor::applyStorageNameToIdentifier(std::optional<size_t> storage, std::string& identifier) noexcept
{
	if (!storage.has_value())
		return;

	switch (storage.value())
	{
		case ENUM(StorageType::CLIENT):
		case ENUM(StorageType::CLIENTO):
			identifier = std::format("client.{}", identifier);
			break;
		case ENUM(StorageType::CLIENTR):
		case ENUM(StorageType::CLIENTRO):
			identifier = std::format("clientr.{}", identifier);
			break;
		case ENUM(StorageType::SERVER):
			identifier = std::format("server.{}", identifier);
			break;
		case ENUM(StorageType::SERVERR):
			identifier = std::format("serverr.{}", identifier);
			break;
	}
}

inline void GS1Visitor::stripStorageNameFromIdentifier(std::string& identifier) noexcept
{
	auto storage = GS1Visitor::getStorageTypeFromIdentifier(identifier);
	if (!storage.has_value()) return;
	auto period = identifier.find('.');
	if (period == std::string::npos) return;

	switch (storage.value())
	{
		// Erase the storage prefix from the identifier, leaving only the actual variable name.
		case ENUM(StorageType::THIS):
		case ENUM(StorageType::THISO):
		case ENUM(StorageType::LOCAL):
		case ENUM(StorageType::TEMP):
			identifier.erase(0, period + 1);
			break;

		// Strip the "o" before the period for object storage types, leaving the "client." or "clientr." prefix.
		case ENUM(StorageType::CLIENTO):
		case ENUM(StorageType::CLIENTRO):
			if (period > 1 && identifier[period - 1] == 'o')
				identifier.erase(period - 1, 1);
			break;
	}
}

template<StoresInGameValue T>
inline bool GS1Visitor::scriptValueContains(std::any& value)
{
	if (auto gs1ScriptValue = std::any_cast<GS1ScriptValue>(&value); gs1ScriptValue != nullptr)
		return scriptValueContains<T>(*gs1ScriptValue);
	if (auto gameVariable = std::any_cast<GameVariable*>(&value); gameVariable != nullptr)
		return (*gameVariable)->has<T>();
	if (auto gameVariable = std::any_cast<GameVariable>(&value); gameVariable != nullptr)
		return gameVariable->has<T>();
	if (auto gameValue = std::any_cast<GameValue>(&value); gameValue != nullptr)
		return gameValue->has<T>();
	return false;
}

template<StoresInGameValue T>
inline bool GS1Visitor::scriptValueContains(GS1ScriptValue& value)
{
	if (auto gameVariable = std::get_if<GameVariable*>(&value); gameVariable != nullptr)
		return (*gameVariable)->has<T>();
	if (auto gameVariable = std::get_if<GameVariable>(&value); gameVariable != nullptr)
		return gameVariable->has<T>();
	if (auto gameValue = std::get_if<GameValue>(&value); gameValue != nullptr)
		return gameValue->has<T>();
	return false;
}

template<StoresInGameValue T>
inline std::optional<std::reference_wrapper<T>> GS1Visitor::getScriptValueAs(std::any& value)
{
	if (auto gs1ScriptValue = std::any_cast<GS1ScriptValue>(&value); gs1ScriptValue != nullptr)
		return getScriptValueAs<T>(*gs1ScriptValue);
	if (auto gameVariable = std::any_cast<GameVariable*>(&value); gameVariable != nullptr)
		return (*gameVariable)->get<T>();
	if (auto gameVariable = std::any_cast<GameVariable>(&value); gameVariable != nullptr)
		return gameVariable->get<T>();
	if (auto gameValue = std::any_cast<GameValue>(&value); gameValue != nullptr)
		return gameValue->get<T>();
	return std::nullopt;
}

template<StoresInGameValue T>
inline std::optional<std::reference_wrapper<T>> GS1Visitor::getScriptValueAs(GS1ScriptValue& value)
{
	if (auto gameVariable = std::get_if<GameVariable*>(&value); gameVariable != nullptr)
		return (*gameVariable)->get<T>();
	if (auto gameVariable = std::get_if<GameVariable>(&value); gameVariable != nullptr)
		return gameVariable->get<T>();
	if (auto gameValue = std::get_if<GameValue>(&value); gameValue != nullptr)
		return gameValue->get<T>();
	return std::nullopt;
}

template<StoresInGameValue T>
inline std::optional<T> GS1Visitor::getScriptValueAsCopy(const std::any& value)
{
	if (auto gs1ScriptValue = std::any_cast<const GS1ScriptValue>(&value); gs1ScriptValue != nullptr)
		return getScriptValueAsCopy<T>(*gs1ScriptValue);
	if (auto gameVariable = std::any_cast<const GameVariable*>(&value); gameVariable != nullptr)
		return (*gameVariable)->getCopy<T>();
	if (auto gameVariable = std::any_cast<const GameVariable>(&value); gameVariable != nullptr)
		return gameVariable->getCopy<T>();
	if (auto gameValue = std::any_cast<const GameValue>(&value); gameValue != nullptr)
		return gameValue->getCopy<T>();
	return std::nullopt;
}

template<StoresInGameValue T>
inline std::optional<T> GS1Visitor::getScriptValueAsCopy(const GS1ScriptValue& value)
{
	if (auto gameVariable = std::get_if<GameVariable*>(&value); gameVariable != nullptr)
		return (*gameVariable)->getCopy<T>();
	if (auto gameVariable = std::get_if<GameVariable>(&value); gameVariable != nullptr)
		return gameVariable->getCopy<T>();
	if (auto gameValue = std::get_if<GameValue>(&value); gameValue != nullptr)
		return gameValue->getCopy<T>();
	return std::nullopt;
}

//----------------------------

inline const ScriptEvent& GS1Visitor::getEvent() const
{
	return *m_event;
}

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
	if (m_event && m_event->initiator.second == ScriptObjectType::NPC)
		defaultToInitiator = false;
	return m_currentSource.empty() ? (defaultToInitiator && m_event != nullptr ? m_event->initiator : m_originalSource) : m_currentSource.back();
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
	std::vector<ScriptObject> sources{m_currentSource.rbegin(), m_currentSource.rend()};
	if (m_event != nullptr && m_event->initiator.second != ScriptObjectType::NPC)
		sources.push_back(m_event->initiator);
	sources.push_back(m_originalSource);
	return sources;
}

inline bool GS1Visitor::hasSleepStack() const
{
	return !m_sleepCallStack.empty();
}

//----------------------------

template<class T>
T* GS1Visitor::walkToContext(antlr4::tree::ParseTree* node)
{
	if (node == nullptr) return nullptr;

	int depthLimit = 50; // Arbitrary depth limit to prevent infinite recursion in malformed trees.
	antlr4::tree::ParseTree* current = node;
	while (depthLimit-- > 0)
	{
		if (auto* context = dynamic_cast<T*>(current); context != nullptr)
			return context;

		if (current->children.size() != 1)
			return nullptr;

		current = current->children[0];
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar

#endif // GS1VISITOR_H
