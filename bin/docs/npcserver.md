# GS2Emu NPC-Server

## What is it?

The NPC-Server manages NPCs and executes serverside scripts.
When an NPC-Server is enabled, many clientside features are disabled.
The game becomes more server-authoritative.

## How to enable

Set the following option in the `serveroptions.txt` file:

    serverside = true

## Requirements

Original servers may not use an NPC-Server.  Only classic servers and beyond are supported.
Make sure the server generation is set appropriately in the `serveroptions.txt` file:

    generation = classic

See: [server.md](server.md#server-generation) for more information about server generations.

## Script languages

The following languages are supported:

    gs1
        [Clientside] Clients version 5.007 and earlier.
        [Serverside] Any client version.

## Client behavior changes

When the NPC-Server is enabled, the client will no longer send NPC property updates.

## Server behavior changes

The server will take over processing of level signs and links.
It will also start to keep track of level objects, such as arrows, bombs, items, etc, for use in serverside scripts.
Various client requests will also be rejected.

The following changes will be made to how the server responds to client requests:

- NPC property edits are ignored.
- New NPCs (via `putnpc`) are ignored.
- Deleting NPCs is ignored.
- Only `client.` flags will be accepted (`server.` flag changes will be ignored).
- New items will be taken from the player's inventory (under the assumption the player dropped the item).
- `toweapons` is ignored.
- Various player property updates are ignored (`maxpower`, `rupees`, `glovepower`, `swordpower`, `shieldpower`, `mp`, `ap`, `kills`, `deaths`, `rating`).

Any of the above should be handled by serverside scripts.

## Options

The following options are available in the `serveroptions.txt` file:

    clientsidesigns = false
        Disables serverside handling of signs if true.
    clientsidelinks = false
        Disables serverside handling of level links if true.
    runallscriptevents = false
        By default, NPCs will only respond to script events referenced in their scripts.  If true, the NPC will respond to all events.
    sleepwhennoplayers = true
        If true, the NPC-Server will not process scripts when there are no players connected.

---
# Serverside NPC programming

Inside a script, anything above the `//#CLIENTSIDE` separator is serverside.
The serverside script does not have access to all of the commands and events that clientside scripts can use.
Things like `putleaps` or `seteffectmode` are only visible on the client, so you need to issue those commands from the clientside script, same with events like `weaponfired`.

### Communicating with a serverside script

In order to communicate with the serverside portion of a script, you must make use of the `triggeraction` command.

    triggeraction x,y,dig,;
    triggeraction 0,0,serverside,Shovel,;
    triggeraction 0,0,servernpc,DatabaseNPCName,;
    triggeraction 0,0,serverwhatever,;

In the first case, a "dig" action will be triggered at the (x, y) location of the level.  Any NPC at those coordinates will get an `actiondig` event, including the NPC itself that issued the action.

In the second case, the serverside script of the "Shovel" weapon will get triggered.  The shovel weapon will get an `actionserverside` event.

In the third case, the serverside script of the database NPC with the name "DatabaseNPCName" will get triggered.  The NPC will get an `actionserverside` event.

In the fourth case, the Control-NPC will get an `actionserverwhatever` event.

So, for a level NPC without a name, you would use the first case to trigger its own serverside script.
To talk to the Control-NPC, you would use the fourth case; any action that starts with `server` and is not `serverside` or `servernpc` will be sent to the Control-NPC.

### Communicating with a clientside script

It is not easy to communicate with a NPC's clientside script.
The client will not process any `triggeraction` events that did not originate from the client itself, so you can't trigger the NPC's clientside script directly.
Hacks can be done using the `shoot` command, but that is not recommended; instead, you should rewrite scripts to communicate in the other direction.

One exception to this is the ability to `triggeraction` a client's weapon script:

    triggeraction 0,0,clientside,weapon name,params;

The weapon's clientside code can listen for the `actionclientside` event.
This was introduced in the 2.21 client, so it won't work for any versions prior to that.

### Collision detection

In order for serverside collision detection to work, you must provide an NPC with a shape:

    if (created) { setshape 1,32,32; }

That will set the width and height of the NPC to 32 pixels.  This will enable collision detection and let events such as `playertouchsme` and `exploded` work.
NPCs that use the `showcharacter;` command will get a normal player's collision box, although you may call `setshape` again to overwrite it.
Triggeractions will also hit within the collision bounds of an NPC rather than at the exact coordinates.

### Script classes

Script classes are used to modularize code.
By issuing a `join` command in the serverside script, the class is joined to the script.
Events are called on the NPC or weapon script, then the event is passed to all joined classes.
Since the joined classes are separate script execution contexts, unprefixed variables (like `i`) are local only to the class.
If you want to make a variable accessible to both the NPC's main script and the class, use the `this.` prefix to assign the variable to the NPC itself.

### Timeouts

Serverside timeouts are limited to 0.1 seconds, unlike clientside timeouts which have 0.05 second resolution.

### Variables

A `flag` is a string value that is set by `set` or `setstring`.
A `var` is a numeric value that is assigned by normal variable assignment.
Clients and serverside scripts only share `flags` that are prefixed with `client.`, `clientr.`, and `serverr.`.

Additionally, there are two special prefixes: `thiso.` and `cliento.`
When using the `with()` statement to access a different NPC or player in serverside script, the `this.` and `client.` variables point to the accessed NPC or player.
The `thiso.` and `cliento.` prefixes will let you access variables linked to the original NPC.

Variable prefixes:

    client.flag - stored on the player account and writable by the client
    clientr.flag - stored on the player account and read-only by the client
    local.flag - stored on the npc
    local.var - stored on the npc
    server.flag - stored on the server and NOT sent to the client (this differs from classic mode)
    serverr.flag - stored on the server and read-only by the client
    level.flag - stored on the level
    level.var - stored on the level
    this.flag - stored on the npc
    this.var - stored on the npc
    flag - stored on the player account
    var - stored in the script execution context (not visible to joined classes)
