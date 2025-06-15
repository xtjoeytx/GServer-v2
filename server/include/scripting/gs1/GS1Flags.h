#ifndef GS1FLAGS_H
#define GS1FLAGS_H

#include <object/NPC.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

void setEventFlags(ScriptEventType event, GameVariableStore& variableStore);
void setPlayerFlags(GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player);
void setNPCFlags(GameVariableStore& variableStore, NPCPtr npc);
void setLevelFlags(GameVariableStore& variableStore, NPCPtr npc, LevelPtr level);
void setOtherFlags(GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player, LevelPtr level);

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1

#endif // GS1FLAGS_H
