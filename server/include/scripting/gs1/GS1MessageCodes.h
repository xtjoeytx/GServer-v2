#ifndef GS1MESSAGECODES_H
#define GS1MESSAGECODES_H

#include <scripting/gs1/ScriptEngineGS1.h>

namespace preagonal::grammar::gs1
{
class GS1Visitor;
}

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

GS1ScriptValue processMessageCode(preagonal::grammar::gs1::GS1Visitor* visitor, std::string_view messageCode, const std::vector<GS1ScriptValue*>& arguments);

///////////////////////////////////////////////////////////////////////////////
}
#endif // GS1MESSAGECODES_H
