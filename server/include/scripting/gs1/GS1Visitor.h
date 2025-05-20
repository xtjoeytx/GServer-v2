#ifndef GS1VISITOR_H
#define GS1VISITOR_H

#include <GS1ParserBaseVisitor.h>

#include <scripting/ScriptContainers.h>

///////////////////////////////////////////////////////////////////////////////

namespace preagonal::grammar::gs1
{

///////////////////////////////////////////////////////////////////////////////

class GS1Visitor : public GS1ParserBaseVisitor
{
public:
	void execute(ScriptEventSource source, antlr4::tree::ParseTree* startNode, ScriptVariableStore* objectVariables, ScriptVariableStore* levelVariables);

private:
	ScriptEventSource m_source{};
	std::vector<ScriptVariableStore*> m_variableContainers;

public:
	//virtual std::any visitProgram(GS1Parser::ProgramContext* context) = 0;
	//virtual std::any visitBlock(GS1Parser::BlockContext* context) = 0;
	//virtual std::any visitStatement(GS1Parser::StatementContext* context) = 0;
	//virtual std::any visitExpression(GS1Parser::ExpressionContext* context) = 0;
	//virtual std::any visitUnary_expression(GS1Parser::Unary_expressionContext* context) = 0;
	//virtual std::any visitPostfix_expression(GS1Parser::Postfix_expressionContext* context) = 0;
	//virtual std::any visitIgnoreUnaryExpression(GS1Parser::IgnoreUnaryExpressionContext* context) = 0;
	//virtual std::any visitIgnoreParenthesesExpression(GS1Parser::IgnoreParenthesesExpressionContext* context) = 0;
	//virtual std::any visitIdentifier_(GS1Parser::Identifier_Context* context) = 0;
	//virtual std::any visitIn_expression(GS1Parser::In_expressionContext* context) = 0;
	//virtual std::any visitBuiltin_command_expression(GS1Parser::Builtin_command_expressionContext* context) = 0;
	//virtual std::any visitBuiltin_function_parameters(GS1Parser::Builtin_function_parametersContext* context) = 0;
	//virtual std::any visitIf_true_block(GS1Parser::If_true_blockContext* context) = 0;
	//virtual std::any visitElse_false_block(GS1Parser::Else_false_blockContext* context) = 0;
	//virtual std::any visitAssignment_operator(GS1Parser::Assignment_operatorContext* context) = 0;
	//virtual std::any visitCompound_string(GS1Parser::Compound_stringContext* context) = 0;
	//virtual std::any visitUnary_operator(GS1Parser::Unary_operatorContext* context) override;
	//virtual std::any visitAssignment(GS1Parser::AssignmentContext* context) override;
	virtual std::any visitMathExpression(GS1Parser::MathExpressionContext* context) override;
	virtual std::any visitComparisonExpression(GS1Parser::ComparisonExpressionContext* context);
	virtual std::any visitLogicExpression(GS1Parser::LogicExpressionContext* context) override;
	virtual std::any visitTernaryExpression(GS1Parser::TernaryExpressionContext* context) override;
	virtual std::any visitInExpression(GS1Parser::InExpressionContext* context) override;
	virtual std::any visitPrimary_expression(GS1Parser::Primary_expressionContext* context) override;
	virtual std::any visitIdentifier(GS1Parser::IdentifierContext* context) override;
	virtual std::any visitIdentifierArray(GS1Parser::IdentifierArrayContext* context) override;
	virtual std::any visitCompoundIdentifier(GS1Parser::CompoundIdentifierContext* context) override;
	virtual std::any visitIncDecOperation(GS1Parser::IncDecOperationContext* context) override;
	virtual std::any visitBuiltInCommand(GS1Parser::BuiltInCommandContext* context) override;
	virtual std::any visitFunctionDefinition(GS1Parser::FunctionDefinitionContext* context) override;
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
