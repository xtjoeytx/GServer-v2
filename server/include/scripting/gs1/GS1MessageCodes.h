#ifndef GS1MESSAGECODES_H
#define GS1MESSAGECODES_H

#include <scripting/ScriptContainers.h>

namespace preagonal::grammar::gs1
{
class GS1Visitor;
}

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

ScriptVariable processMessageCode(preagonal::grammar::gs1::GS1Visitor* visitor, std::string_view messageCode, const std::vector<ScriptVariableContainer*>& arguments);

///////////////////////////////////////////////////////////////////////////////
}
#endif // GS1MESSAGECODES_H
