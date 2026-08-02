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

void setEventFlags(ScriptEventType event, std::vector<ScriptEventType>* additionalEventTypes, GameVariableStore& variableStore);
void setTriggerActionAndCustomEventFlags(ScriptEvent& event, GameVariableStore& variableStore);
void setPlayerFlags(GameVariableStore& variableStore, NPCPtr npc, PlayerPtr player);
void setNPCFlags(ScriptEvent& event, GameVariableStore& variableStore, NPCPtr npc);
void setLevelFlags(GameVariableStore& variableStore, NPCPtr npc, LevelPtr level);
void setWeaponFlags(ScriptEvent& event, ScriptObject source, GameVariableStore& variableStore);
void setOtherFlags(ScriptEvent& event, ScriptObject source, GameVariableStore& variableStore, NPCPtr npc, PlayerPtr player, LevelPtr level);

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1

#endif // GS1FLAGS_H
