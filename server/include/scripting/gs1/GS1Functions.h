#ifndef GS1FUNCTIONS_H
#define GS1FUNCTIONS_H

#include <string_view>
#include <tree/ParseTree.h>
#include <scripting/gs1/ScriptEngineGS1.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

/*
Possible extensions:
	bodyexists(str) -> bool
	headexists(str) -> bool
	shieldexists(str) -> bool
	swordexists(str) -> bool
	checksum(str) -> float
	extractfilebase(str) -> string
	extractfileext(str) -> string
	extractfilename(str) -> string
	extractfilepath(str) -> string
	fileexists(str) -> bool
	filesize(str) -> integer
	hasright(str, str) -> bool
	lowercase(str) -> string
	uppercase(str) -> string
	isinclass(str) -> float
*/

class GS1Visitor;
GS1ScriptValue processBuiltInFunction(GS1Visitor* visitor, antlr4::tree::ParseTree* node, std::string_view functionName);

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar

#endif // GS1FUNCTIONS_H
