#ifndef GS1VARIABLES_H
#define GS1VARIABLES_H

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
flag            (client) [  ] level
                (server) [  ] player
var             (client) [  ] level
                (server) [  ] (GS1) npc (GS2) global

In classic mode, all flags (except local.flags) are sent to the server from the client.
Global mode variables are not saved.
*/

///////////////////////////////////////////////////////////////////////////////
}
#endif // GS1VARIABLES_H
