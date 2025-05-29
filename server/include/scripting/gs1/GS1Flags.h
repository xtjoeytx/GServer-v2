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

void setEventFlags(ScriptEventType event, GameVariableStore& variableStore);
void setPlayerFlags(GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player);
void setNpcFlags(GameVariableStore& variableStore, NPCPtr npc);
void setLevelFlags(GameVariableStore& variableStore, NPCPtr npc, LevelPtr level);
void setOtherFlags(GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player, LevelPtr level);

///////////////////////////////////////////////////////////////////////////////
}
#endif // GS1FLAGS_H
