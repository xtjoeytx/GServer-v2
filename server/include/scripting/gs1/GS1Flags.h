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
void setPlayerFlags(GameVariableStore& variableStore, const NPCPtr& npc, const PlayerPtr& player);
void setNPCFlags(const ScriptEvent& event, GameVariableStore& variableStore, const NPCPtr& npc);
void setLevelFlags(GameVariableStore& variableStore, const NPCPtr& npc, const LevelPtr& level);
void setWeaponFlags(ScriptEvent& event, const ScriptObject& source, GameVariableStore& variableStore);
void setOtherFlags(const ScriptEvent& event, const ScriptObject& source, GameVariableStore& variableStore, const PlayerPtr& player, const LevelPtr& level);

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1

#endif // GS1FLAGS_H
