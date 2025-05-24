#ifndef GS1VARIABLES_H
#define GS1VARIABLES_H

#include <scripting/ScriptContainers.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////


/* Variable prefixes
	client.flag		- client flags changed client and serverside, synchronized.
	clientr.flag	- client flags changed serverside only, sent to client.
	local.flag		- client flags not sent to the server.
	server.flag		- global flags that, when using an npcserver, is only visible serverside.
	serverr.flag	- global flags that can be read by the client.
	this.flag		- npc flags, not shared with the server or the client.
	thiso.flag		- npc flags, same as above, but refer to the source npc inside a 'with' block.
	flag			- (classic) level flags that aren't sent to the server (npcserver) saved to the player account and can only be read serverside
*/

// Create new variant of ScriptVariable that is a getter/setter function pair.
// Create a new function to facilitate get/set of the variables.

///////////////////////////////////////////////////////////////////////////////
}
#endif // GS1VARIABLES_H
