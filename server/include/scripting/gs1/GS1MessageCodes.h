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

// player.upgradestatus #G(index)
// String variable #v(floating point) has been improved to display also integers with more than 6 decimals correctly, which makes it possible to fix the item sorting on Graal Kingdoms
// in string operations the #b message code is passed instead of removed, so you can add lines to text that you want display with say2, like

///////////////////////////////////////////////////////////////////////////////
}
#endif // GS1MESSAGECODES_H
