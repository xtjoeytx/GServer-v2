#ifndef GS1FLAGS_H
#define GS1FLAGS_H

#include <level/Level.h>
#include <object/NPC.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

void setEventFlags(ScriptEventType event, ScriptVariableStore& variableStore);
void setPlayerFlags(ScriptVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player);
void setNpcFlags(ScriptVariableStore& variableStore, NPCPtr npc);
void setLevelFlags(ScriptVariableStore& variableStore, NPCPtr npc, LevelPtr level);
void setOtherFlags(ScriptVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player, LevelPtr level);

///////////////////////////////////////////////////////////////////////////////
}
#endif // GS1FLAGS_H
