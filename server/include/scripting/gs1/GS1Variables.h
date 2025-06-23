#ifndef GS1VARIABLES_H
#define GS1VARIABLES_H

#include <object/NPC.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptContainers.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

// All variables set with setstring that don't have a special leading (this., thiso., server., serverr.) are added to the player (like in GS1)

/* Variable prefixes
flag = setstring/set
var  = numeric/array

[->] transmits to the server
[<-] transmits to the client
[  ] does not transmit

global  = stored on the server (not saved)
server  = stored on the server
level   = stored on the level
player  = stored on the player
npc     = stored on the npc
class   = stored on the script/class

client.flag     (client) [->] player
                (server) [<-] player
clientr.flag    (client) [  ] player
                (server) [<-] player
local.flag      (client) [  ] player
                (server) [  ] npc
local.var       (client) [  ] player
                (server) [  ] npc
server.flag     (client) [  ] player
                (server) [  ] server
serverr.flag    (client) [  ] player
                (server) [<-] server
level.flag      (client) [  ] player?
                (server) [  ] level
level.var       (client) [  ] player?
                (server) [  ] level
this.flag       (client) [  ] npc
                (server) [  ] npc
this.var        (client) [  ] npc
                (server) [  ] npc
flag            (client) [  ] player
                (server) [  ] player
var             (client) [  ] level
                (server) [  ] (GS1) class (GS2) global

In classic mode, all flags (except local.flags) are sent to the server from the client.
Global mode variables are not saved.

unset flag;			looks for flag and deletes if found.
setstring flag,;	looks for flag= and deletes if found.

set flag;
setstring flag,value;	overwrites flag with flag=value.

setstring flag,value;
set flag;				does not erase flag's value, since it is technically already set.

FLAG name
FLAG name=value
VAR name=value
VAR name={value,value}
*/

/*
players[index]		On gmaps, it includes players in a 3x3 area around the player.  Probably limited by syncdistancex / syncdistancey.
*/

void setReadOnlyGlobalVariables(GameVariableStore& variableStore);
void setPlayerVariables(GameVariableStore& variableStore, PlayerClientPtr player);
void setLevelVariables(GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player, LevelPtr level);
void setOtherVariables(GameVariableStore& variableStore, ScriptEvent& event);

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1

#endif // GS1VARIABLES_H
