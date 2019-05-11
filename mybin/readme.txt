Graal Reborn GServer
Created by: Joey, Nalin, dufresnep, Codr, Marlon.
Based off the original work by 39ster.
For their additional work on the old gserver, special thanks go to:
	Agret, Beholder, Joey, Marlon, Nalin, and Pac.

----------------------------
| Quick Start Instructions |
----------------------------

How-to setup a server:

1) Under the accounts folder, rename the text file 'YOURACCOUNT.txt' to your account name.  For example: 'KuJi.txt'
2) Modify defaultaccount.txt to your liking.  This is the default settings new players will start with.  It can also be modified via RC.
3) Open config/serveroptions.txt and edit it to your liking.  Be sure to modify the settings under "Private server options".  Help for what these options do are available on the forums and in the file itself.
4) Find the line that starts with "staff=" in config/serveroptions.txt.  Replace YOURACCOUNT with your account name.  Anybody who needs RC access must be added to this line with their account names separated by commas.  Additionally, RC users must have their IP range changed to at least *.*.*.* in their account to connect.
5) Port forward if needed. (Many threads on this topic exist in the forums.  If you are having trouble, seek them out.  Try the tutorials forum.)  Basically, if you are behind a router and your computer isn't set to be the "DMZ", you will need to port forward.
6) Run gserver2.exe -- enjoy.
7) Report any bugs on http://www.graal.in/


---------------
| servers.txt |
---------------

The gserver can run multiple servers at once without needing to spawn separate processes.  This is accomplished by the servers.txt file.  This file will tell the gserver how many servers to run and where they are located, as well as some optional ip and port overrides.

The file looks like this:
    servercount = 1
    server_1 = default
    server_1_ip = myserver.com
    server_1_port = 12345
    server_1_localip = 127.0.0.1
    server_1_interface = 192.168.2.1

servercount specifies the number of servers.  In the default file, that is 1 server.
server_# specifies the directory the server is under.
server_#_ip specifies an optional ip address override.
server_#_port specifies an optional port override.
server_#_localip specifies an optional localip override.
server_#_interface specifies an optional interface override.

All of the optional overrides will take precedence over the options defined in serveroptions.txt.


-------------------------------------
| Special Graal Reborn NPC commands |
-------------------------------------

The Graal Reborn gserver has a couple special NPC commands built in.

join somefile;
    Much like official Graal's server-side join command, this command searches for somefile.txt and appends the contents to the end of the NPC script.

singleplayer
    This command is like the sparringzone command.  When placed by itself with no semi-colon inside an NPC, it signifies that the level is "singleplayer."  (SEE: Singleplayer Levels).


-----------------------
| Singleplayer Levels |
-----------------------

The Graal Reborn gserver has the ability to toggle a level as "singleplayer."  In this mode, the user cannot see any other player in the level.  Any changes they make to the level are not replicated to other users.  They are, in essence, in a level by themselves.

To activate singleplayer mode, add an NPC to the level and add the single command "singleplayer" to the level, much like how the "sparringzone" command works.


----------------
| Group Maps |
----------------

Like singleplayer levels, group maps allow only players in a group to see each other in a level.  Player groups can be managed via the gr.setgroup and gr.setlevelgroup triggeractions (SEE: Graal Reborn special triggeractions).

Individual levels cannot be set as group levels; instead, an entire map must be specified as a group map.  The "groupmaps" server option specifies a comma-delimited list of maps that can contain groups.


-------------------------------------
| Graal Reborn special client flags |
-------------------------------------

There are a few special client flags built into the gserver.  These are:
gr.x
gr.y
gr.z

These flags are used by the -gr_movement weapon included in the server weapons folder to simulate the smooth movement as found in the Graal clients 2.3 and up.

If you don't want the gserver to recognize these flags, set the flaghack_movement setting to false in serveroptions.txt.

Also, if flaghack_ip is enabled in the serveroptions.txt file, you can gain access to the following:
gr.ip


---------------------------------------
| Graal Reborn special triggeractions |
---------------------------------------

The Graal Reborn gserver has a couple unique triggeractions built into it.  They can be enabled/disabled by altering the setting that controls their group in serveroptions.txt.  They are as follows:

Controlled by the setting triggerhack_weapons:
    triggeraction 0,0,gr.addweapon,weapon1,weapon2,weapon3;
        Adds weapon1, weapon2, and weapon3 to the player's account.

    triggeraction 0,0,gr.deleteweapon,weapon1,weapon2,weapon3;
        Removes weapon1, weapon2, and weapon3 from the player's account.

Controlled by the setting triggerhack_guilds:
    triggeraction 0,0,gr.addguildmember,guild,account,nickname;
        Adds a player to the specified guild.  Nickname is optional.

    triggeraction 0,0,gr.removeguildmember,guild,account;
        Removes a player from the specified guild.

    triggeraction 0,0,gr.removeguild,guild;
        Removes the guild from the server.

    triggeraction 0,0,gr.setguild,guild,account;
        Sets the player's guild tag to the specified guild.

Controlled by the setting triggerhack_groups:
    triggeraction 0,0,gr.setgroup,group;
        Adds the player to the specified group.

    triggeraction 0,0,gr.setlevelgroup,group;
        Adds all the players in the level to the specified group.

    triggeraction 0,0,gr.setplayergroup,account,group;
        Adds the specified player to the specified group.

Controlled by the setting triggerhack_files:
    triggeraction 0,0,gr.appendfile,filename,text;
        Opens the file specified, located in the server's logs directory, and appends a line of text.

    triggeraction 0,0,gr.writefile,filename,text;
        Opens the file specified, located in the server's logs directory, erases all of its contents, and writes a line of text.

    triggeraction 0,0,gr.readfile,filename,line_pos;
        Opens the file specified, located in the server's logs directory, reads the given line number, and returns the contents to the player.
        File contents are returned on the following flags:
            gr.fileerror: String list.  First index is a random number, subsequent indexes are error values.  Error 1 = line_pos was outside of range.  In this case, the next value is the line number returned.
            gr.filedata: The file data.

Controlled by the setting triggerhack_rc:
    triggeraction 0,0,gr.rcchat,Some chat text;
        Sends some chat text to any logged in RC's.

Controlled by the setting triggerhack_execscript:
    triggeraction 0,0,gr.es_clear;
        Clears the execscript parameter list.

    triggeraction 0,0,gr.es_set,param1,param2,...;
        Sets the execscript parameter list.

    triggeraction 0,0,gr.es_append,phrase;
        Appends phrase directly to the end of the set parameter list.

    triggeraction 0,0,gr.es,account,script_name;
        Sends the execscript to the specified account, or everybody if ALLPLAYERS was specified.
        View the execscript/readme.txt file for more information.

Controlled by the setting triggerhack_props:
    triggeraction 0,0,gr.attr1,data;
        Sets data on the specified attribute.  gr.attr1 - gr.attr30 work.

    triggeraction 0,0,gr.fullhearts,amount;
        Sets the player's fullhearts to the specified amount.

Controlled by the setting triggerhack_levels:
    triggeraction 0,0,gr.updatelevel;
        Updates the current level.

    triggeraction 0,0,gr.updatelevel,levelname;
        Updates the specified level.

Not controlled by any option:
    triggeraction 0,0,gr.npc.move,id,dx,dy,duration,options;
        Creates a serverside move command for the specified NPC.

    triggeraction 0,0,gr.npc.setpos,id,x,y;
        Sets an NPC's position.


-------------------
| Weapon bytecode |
-------------------
Place weapon bytecode in the weapon_bytecode/ folder.  Inside each weapon file in weapons/, add the following:
BYTECODE name_of_file

The gserver will load weapon_bytecode/name_of_file and use the bytecode contained there-in.