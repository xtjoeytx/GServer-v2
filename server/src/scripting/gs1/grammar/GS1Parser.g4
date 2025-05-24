parser grammar GS1Parser;

options
{
	tokenVocab=GS1Lexer;
}

@parser::header
{
// --------------------------------------------------------
#include <map>
#include <string>
#include <string_view>
#include <optional>
// --------------------------------------------------------
}

@parser::members
{
// --------------------------------------------------------
std::map<std::string, antlr4::tree::ParseTree*> userFunctions;
// --------------------------------------------------------
void add_user_function(std::string funcName, antlr4::tree::ParseTree* treeNode)
{
	userFunctions.insert_or_assign(funcName, treeNode);
}
// --------------------------------------------------------
}

program
	: (statement | block)+ EOF
	;

block
	: TOKEN_BRACE_LEFT (statement | block)* TOKEN_BRACE_RIGHT
	| statement
	;

statement
	: if_condition
	| for_loop
	| while_loop
	| function_definition END?
	| user_function END?
	| builtin_command END?
	| flow_return END?
	| flow_break END?
	| flow_continue END?
	| assignment_operation
	| expression END?
	| END
	;

expression
	: binary_expression
	| unary_expression
	;

unary_expression
	: postfix_expression
	| unary_operation
	;

postfix_expression
	: primary_expression
	| inc_dec_expression
	;

primary_expression
	: builtin_function
	| array_literal
	| literal_literal
	| identifier_
	| compound_string+
	;

binary_expression
	: binary_expression OP_POW binary_expression											# MathExpression
	| binary_expression (OP_MUL | OP_DIV | OP_MOD) binary_expression						# MathExpression
	| binary_expression (OP_ADD | OP_SUB) binary_expression									# MathExpression
	| binary_expression (OP_LESS | OP_GREAT | OP_LESS_EQ | OP_GREAT_EQ) binary_expression	# ComparisonExpression
	| binary_expression (OP_EQUAL | OP_ASSIGN | OP_NOTEQ) binary_expression					# ComparisonExpression
	| binary_expression OP_LOGICALAND binary_expression										# LogicExpression
	| binary_expression OP_LOGICALOR binary_expression										# LogicExpression
	| binary_expression TOKEN_QUESTION binary_expression TOKEN_COLON binary_expression		# TernaryExpression
	| binary_expression (TOKEN_COMMA binary_expression)* OP_IN in_expression				# InExpression
	| TOKEN_PAREN_LEFT binary_expression TOKEN_PAREN_RIGHT									# ParenthesesExpression
	| unary_expression																		# ignoreUnaryExpression
	;

identifier_
	: compound_identifier																# Identifier
	| compound_identifier TOKEN_BRACKET_LEFT primary_expression TOKEN_BRACKET_RIGHT		# IdentifierArray
	;

compound_identifier
	: identifier_literal (identifier_literal | messagecode_string)*?					# CompoundIdentifier
	;

inc_dec_expression
	: identifier_ OP_INC	# IncDecOperation
	| identifier_ OP_DEC	# IncDecOperation
	;

in_expression
	: range_literal
	| array_literal
	;

assignment
	: array_literal
	| expression
	;

builtin_command
	: COMMAND WS*? builtin_command_expression? (TOKEN_COMMA builtin_command_expression?)*	# BuiltInCommand
	;

builtin_command_expression
	: expression
	| ITEM
	| CARRY
	| DIRECTION
	| GENDER
	| COLOR
	| BADDY
	;

function_definition
	: KW_FUNCTION identifier_literal TOKEN_PAREN_LEFT TOKEN_PAREN_RIGHT block	{add_user_function($identifier_literal.ctx->getText(), $block.ctx);}
	;

user_function
	: identifier_literal TOKEN_PAREN_LEFT TOKEN_PAREN_RIGHT							# UserFunctionCall
	;

builtin_function
	: FUNCTION TOKEN_PAREN_LEFT builtin_function_parameters TOKEN_PAREN_RIGHT		# BuiltInFunctionCall
	;

builtin_function_parameters
	: (compound_string+ | expression) (TOKEN_COMMA (compound_string+ | expression))*
	;

if_condition
	: KW_IF TOKEN_PAREN_LEFT expression TOKEN_PAREN_RIGHT if_true_block else_false_block?	# IfCondition
	;

if_true_block
	: block
	;

else_false_block
	: KW_ELSE block
	;

for_loop
    : KW_FOR TOKEN_PAREN_LEFT assignment_operation? END expression? END expression? TOKEN_PAREN_RIGHT block	# ForLoop
    ;

while_loop
    : KW_WHILE TOKEN_PAREN_LEFT expression TOKEN_PAREN_RIGHT block									# WhileLoop
    ;

flow_return
    : KW_RETURN			# FlowReturn
    ;

flow_break
    : KW_BREAK			# FlowBreak
    ;
	
flow_continue
    : KW_CONTINUE		# FlowContinue
    ;

assignment_operator
	: OP_ASSIGN
	| OP_ASSIGN2
	| OP_ASSIGN_MUL
	| OP_ASSIGN_DIV
	| OP_ASSIGN_MOD
	| OP_ASSIGN_ADD
	| OP_ASSIGN_SUB
	| OP_ASSIGN_POW
	;

assignment_operation
	: identifier_ assignment_operator assignment	# AssignmentOperation
	;

unary_operator
	: OP_ADD
	| OP_SUB
	| OP_LOGICALNOT
	;

unary_operation
	: unary_operator expression			# UnaryOperation
	;

compound_string
	: string_literal
	| messagecode_string
	;

messagecode_string
	: MESSAGECODE																				# MessageCode
	| MESSAGECODE TOKEN_PAREN_LEFT? expression (TOKEN_COMMA expression)* TOKEN_PAREN_RIGHT		# MessageCode
	;

literal_literal
	: LITERAL		# Literal
	| ALLFEATURES	# LiteralAllFeatures
	;

string_literal
	: STRING		# StringLiteral
	;

identifier_literal
	: IDENTIFIER	# IdentifierLiteral
	;

range_literal
	: (TOKEN_PIPE | OP_LESS) expression TOKEN_COMMA expression (TOKEN_PIPE | OP_GREAT)			# RangeLiteral
	;

array_literal
	: TOKEN_BRACE_LEFT expression (TOKEN_COMMA expression)* TOKEN_BRACE_RIGHT	# ArrayLiteral
	;
