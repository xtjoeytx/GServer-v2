parser grammar GS1Parser;

options
{
	tokenVocab=GS1Lexer;
}

@parser::header
{
// --------------------------------------------------------
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
// --------------------------------------------------------
}

@parser::context
{
// --------------------------------------------------------
struct string_hash
{
	using hash_type = std::hash<std::string_view>;
	using is_transparent = void;

	[[nodiscard]] size_t operator()(const char* str) const noexcept
	{
		return hash_type{}(str);
	}
	[[nodiscard]] size_t operator()(const std::string_view& str) const noexcept
	{
		return hash_type{}(str);
	}
	[[nodiscard]] size_t operator()(const std::string& str) const noexcept
	{
		return hash_type{}(str);
	}
	[[nodiscard]] size_t operator()(const size_t& hash) const noexcept
	{
		return hash;
	}
};
// --------------------------------------------------------
}

@parser::members
{
// --------------------------------------------------------
std::map<std::string, antlr4::tree::ParseTree*> userFunctions;
std::unordered_set<std::string, string_hash, std::equal_to<>> identifiers;
// --------------------------------------------------------
void add_user_function(std::string funcName, antlr4::tree::ParseTree* treeNode)
{
	userFunctions.insert_or_assign(funcName, treeNode);
}
void add_identifier(std::string identifier)
{
	identifiers.insert(identifier);
}
// --------------------------------------------------------
}

program
	: EOF
	| (statement END?)+
	;

block
	: TOKEN_BRACE_LEFT (statement END?)* TOKEN_BRACE_RIGHT
	| statement END?
	;

statement
	: TOKEN_BRACE_LEFT (statement END?)* TOKEN_BRACE_RIGHT
	| ( END
		| ifStatement
		| forStatement
		| whileStatement
		| withStatement
		| functionDefinition
		| flowStatement
		| builtinCommandStatement
		| userFunctionStatement
		| assignmentStatement
		| expression
		)
	;

//----------------------------------------------------------

ifStatement
	: KW_IF TOKEN_PAREN_LEFT expression TOKEN_PAREN_RIGHT block (KW_ELSE block)?			# StatementIf
	;

forStatement
    : KW_FOR TOKEN_PAREN_LEFT
		assignmentStatement? END
		expression? END
		(assignmentStatement | expression)? TOKEN_PAREN_RIGHT
		block																				# StatementFor
    ;

whileStatement
    : KW_WHILE TOKEN_PAREN_LEFT expression TOKEN_PAREN_RIGHT block							# StatementWhile
    ;

withStatement
	: KW_WITH TOKEN_PAREN_LEFT expression TOKEN_PAREN_RIGHT block							# StatementWith
	;

flowStatement
    : KW_RETURN																				# FlowReturn
    | KW_BREAK																				# FlowBreak
    | KW_CONTINUE																			# FlowContinue
    ;

//----------------------------------------------------------

functionDefinition
	: KW_FUNCTION compound_identifier TOKEN_PAREN_LEFT TOKEN_PAREN_RIGHT block
		{ add_user_function($compound_identifier.ctx->getText(), $block.ctx); }				# StatementFunctionDefinition
	;

//----------------------------------------------------------

userFunctionStatement
	: compound_identifier TOKEN_PAREN_LEFT TOKEN_PAREN_RIGHT								# StatementUserFunctionCall
	;

//----------------------------------------------------------

builtinCommandStatement
	: COMMAND builtInCommandExpression (TOKEN_COMMA builtInCommandExpression?)*				# StatementBuiltInCommand
	| COMMAND																				# StatementBuiltInCommand
	;

builtInCommandExpression
	: special_literal
	| expression
	;

//----------------------------------------------------------

assignmentStatement
	: identifier_access assignment_operator expression										# StatementAssignment
	;

//----------------------------------------------------------

expression
	: conditionalExpression
	;

conditionalExpression
	: logicalOrExpression (TOKEN_QUESTION expression TOKEN_COLON conditionalExpression)*	# ExpressionTernary
	;

logicalOrExpression
	: logicalAndExpression (OP_LOGICALOR logicalAndExpression)*								# ExpressionLogicOr
	;

logicalAndExpression
	: equalityExpression (OP_LOGICALAND equalityExpression)*								# ExpressionLogicAnd
	;

equalityExpression
	: relationalExpression ((OP_EQUAL | OP_ASSIGN | OP_NOTEQ) relationalExpression)?		# ExpressionEquality
	;

relationalExpression
	: additiveExpression
		((OP_LESS | OP_GREAT | OP_LESS_EQ | OP_GREAT_EQ) additiveExpression)?				# ExpressionRelational
	;

additiveExpression
	: multiplicativeExpression ((OP_ADD | OP_SUB) multiplicativeExpression)*				# ExpressionAdditive
	;

multiplicativeExpression
	: inExpression ((OP_MUL | OP_DIV | OP_MOD) inExpression)*								# ExpressionMultiplicative
	;

inExpression
	: exponentiationExpression
		((TOKEN_COMMA exponentiationExpression)*
			OP_IN (range_literal | primaryExpression))?										# ExpressionIn
	;

exponentiationExpression
	: unaryExpression (OP_POW unaryExpression)*												# ExpressionExponentiation
	;

unaryExpression
	: (OP_ADD | OP_SUB | OP_LOGICALNOT) unaryExpression										# ExpressionUnary
	| postfixExpression																		# ignoreExpressionUnaryPrimary
	;

postfixExpression
	: primaryExpression (OP_INC | OP_DEC)													# ExpressionPostfix
	| primaryExpression																		# ignoreExpressionPostfixPrimary
	;

primaryExpression
	: TOKEN_PAREN_LEFT expression TOKEN_PAREN_RIGHT
	| builtin_function
	| array_literal
	| literal_literal
	| identifier_access
	| compound_string
	;

//----------------------------------------------------------

builtin_function
	: FUNCTION TOKEN_PAREN_LEFT expression (TOKEN_COMMA expression?)* TOKEN_PAREN_RIGHT		# BuiltInFunctionCall
	;

identifier_access
	: identifier_value (TOKEN_PERIOD identifier_value)*										# IdentifierAccess
	;

identifier_value
	: (storage_token | IDENTIFIER storage_token) compound_identifier
		(TOKEN_BRACKET_LEFT conditionalExpression TOKEN_BRACKET_RIGHT)?
		{ add_identifier($compound_identifier.ctx->getText()); }							# IdentifierValue
	| compound_identifier (TOKEN_BRACKET_LEFT conditionalExpression TOKEN_BRACKET_RIGHT)?
		{ add_identifier($compound_identifier.ctx->getText()); }							# IdentifierValue
	;

compound_identifier
	: IDENTIFIER (IDENTIFIER | messagecode_string | REAL)*									# CompoundIdentifier
	;

compound_string
	: (STRING | messagecode_string)+														# CompoundString
	;

messagecode_string
	: MESSAGECODE
		(TOKEN_PAREN_LEFT expression (TOKEN_COMMA expression)* TOKEN_PAREN_RIGHT)?			# MessageCode
	;

//----------------------------------------------------------

assignment_operator
	: ( OP_ASSIGN
		| OP_ASSIGN_MUL
		| OP_ASSIGN_DIV
		| OP_ASSIGN_MOD
		| OP_ASSIGN_ADD
		| OP_ASSIGN_SUB
		| OP_ASSIGN_POW
		)
	;

array_literal
	: TOKEN_BRACE_LEFT expression (TOKEN_COMMA expression)* TOKEN_BRACE_RIGHT				# ArrayLiteral
	;

literal_literal
	: ( LITERAL
		| ALLFEATURES
		| ALLSTATS )																		# Literal
	;

range_literal
	: (TOKEN_PIPE | OP_LESS) expression TOKEN_COMMA expression (TOKEN_PIPE | OP_GREAT)		# RangeLiteral
	;

special_literal
	: ITEM																					# ItemLiteral
	| CARRY																					# CarryLiteral
	| DIRECTION																				# DirectionLiteral
	| GENDER																				# GenderLiteral
	| COLOR																					# ColorLiteral
	| BADDY																					# BaddyLiteral
	;

storage_token
	: (	STORAGE_THIS
		| STORAGE_THISO
		| STORAGE_CLIENT
		| STORAGE_CLIENTR
		| STORAGE_SERVER
		| STORAGE_SERVERR
		| STORAGE_LEVEL
		| STORAGE_LOCAL
		| STORAGE_TEMP )																	# StorageToken
	;
