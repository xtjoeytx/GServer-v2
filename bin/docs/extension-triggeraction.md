# GS2Emu Triggeraction Extensions

## What are they?

The server can process special triggeractions received from the client.
They have a wide variety of uses from writing files or enabling smooth movement for earlier clients.

## How to enable

Triggeraction extensions are currently enabled in the `serveroptions.txt` file in your playerworld.

Options:

    triggerhack_weapons = false
    triggerhack_guilds = false
    triggerhack_groups = false
    triggerhack_files = false
    triggerhack_rc = false
    triggerhack_execscript = false
    triggerhack_props = false
    triggerhack_levels = false

## Always enabled

The following actions are always enabled:

    gr.serverlist
        Returns the server list to the player via a clientside triggeraction to the -Serverlist_v4 weapon.

## triggerhack_weapons

Enables the following actions:

    gr.addweapon,weapon1,weapon2,...
        Adds weapons to the player's inventory.
    gr.deleteweapon,weapon1,weapon2,...
        Removes weapons from the player's inventory.

## triggerhack_guilds

Enables the following actions:

    gr.addguildmember,guild,nickname,account
        Adds a member to a guild, creating it if it doesn't exist.
    gr.removeguildmember,guild,account
        Removes a member from a guild.
    gr.removeguild,guild
        Removes a guild and all its members.
    gr.setguild,guild,account
        Sets the guild of a player, but does not create the guild or add them to it.

## triggerhack_groups

Controls the "group map" extension feature of the server which lets a group of players have their own private copy of a level.
All players will only see and interact with other players who have the same group name.

See: [extension-level-groupmap.md](extension-level-groupmap.md) for more information.

Enables the following actions:

    gr.setgroup,group
        Assigns the group name to a player.
    gr.setlevelgroup,group
        Assigns the group name to all current players in the level.
    gr.setplayergroup,account,group
        Assigns a group name to a specific player.

## triggerhack_files

Allows reading and writing to files in the `logs/` directory.

Enables the following actions:

    gr.appendfile,filename,content
        Appends content to a file.
    gr.writefile,filename,content
        Writes content to a file, overwriting any existing content.
    gr.readfile,filename,line number
        Reads the content of the specified line in a file.

The `gr.readfile` action will write data to the following player flags:

    gr.fileerror
    gr.filedata

The `gr.fileerror` flag will be set to `0` on no error, or `1,last line number` (e.g., `1,13`).

## triggerhack_rc

Enables the following actions:

    gr.rcchat,content
        Sends a message to the RC chat.

## triggerhack_execscript

Controls the "execscript" extension feature of the server which allows executing scripts on the player with custom parameters.

See: [extension-execscript.md](extension-execscript.md) for more information.

Enables the following actions:

    gr.es_clear
        Clears the execscript parameters.
    gr.es_set,param1,param2,...
        Sets the execscript parameters.
    gr.es_append,param1,param2,...
        Appends the parameters to the execscript parameter list.
    gr.es,account,script name
        Executes the execscript on the player and clears the parameter list.

## triggerhack_props

Enables the following actions:

    gr.attr#,value
        Sets the player's specified gani attribute.  gr.attr1 - gr.attr30
    gr.fullhearts,heart count
        Sets the player's full hearts count.

## triggerhack_levels

Enables the following actions:

    gr.updatelevel,level
        Updates a level.
    gr.npc.move,id,dx,dy,duration,options
        Executes a "move" command on the specified NPC.
    gr.npc.setpos,id,x,y
        Sets the position of an NPC.
