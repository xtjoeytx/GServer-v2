#ifndef GS1FUNCTIONS_H
#define GS1FUNCTIONS_H

#include <scripting/gs1/ScriptEngineGS1.h>

namespace preagonal::grammar::gs1
{
class GS1Visitor;
}

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

GS1ScriptValue processBuiltInFunction(preagonal::grammar::gs1::GS1Visitor* visitor, std::string_view functionName, const std::vector<GS1ScriptValue*>& arguments);

///////////////////////////////////////////////////////////////////////////////
}
#endif // GS1FUNCTIONS_H
