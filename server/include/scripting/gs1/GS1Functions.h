#ifndef GS1FUNCTIONS_H
#define GS1FUNCTIONS_H

#include <string_view>
#include <tree/ParseTree.h>
#include <scripting/gs1/ScriptEngineGS1.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1::grammar
{
///////////////////////////////////////////////////////////////////////////////

class GS1Visitor;
GS1ScriptValue processBuiltInFunction(GS1Visitor* visitor, antlr4::tree::ParseTree* node, std::string_view functionName);

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1::grammar

#endif // GS1FUNCTIONS_H
