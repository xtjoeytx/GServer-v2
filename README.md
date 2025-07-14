# GServer-v2
[![Build Status](https://jenkins.amigadev.com/job/GServer-v2/job/master/badge/icon)](https://jenkins.amigadev.com/job/GServer-v2/job/master/)

Created by: Joey, Nalin, dufresnep, Codr, Marlon.
Based off the original work by 39ster.
For their additional work on the old gserver, special thanks go to:
	Agret, Beholder, Joey, Marlon, Nalin, and Pac.
	
## Building

### Required dependencies
- C++23 compiler (min supported: GCC 14, Clang 18, MSVC 2022 17.7)
- Java JRE
- CMake
- Ninja build system
- vcpkg (https://vcpkg.io/en/getting-started)

Linux users can install `openjdk-21-jre` or similar via their package manager, Windows users can install from [Microsoft](https://learn.microsoft.com/en-us/java/openjdk/download) or via `winget`.

vcpkg needs to be installed and the `VCPKG_ROOT` environment variable needs to be set to the location where it exists.
The directory should be writable by the user running the build (unless you want to spend extra time reading documentation and configuring the software).

### Build with CMake

If using command-line cmake, you would start a build like so:
```
cmake --preset "Release x64" -G Ninja -B build -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

On a Linux build, you would use `x64-linux` as the `VCPKG_TARGET_TRIPLET`.

IDEs such as Visual Studio and CLion have CMake support built-in and can be used to easily build and configure the projects.

## Quick Start Instructions

How-to setup a server:

1. Under the accounts folder, rename the text file 'YOURACCOUNT.txt' to your account name.  For example: 'KuJi.txt'
2. Modify defaultaccount.txt to your liking.  This is the default settings new players will start with.  It can also be modified via RC.
3. Open config/serveroptions.txt and edit it to your liking.  Be sure to modify the settings under "Private server options".  Help for what these options do are available on the forums and in the file itself.
4. Find the line that starts with "staff=" in config/serveroptions.txt.  Replace YOURACCOUNT with your account name.  Anybody who needs RC access must be added to this line with their account names separated by commas.  Additionally, RC users must have their IP range changed to at least *.*.*.* in their account to connect.
5. Run gserver2.exe -- enjoy.
6. Report any bugs on http://www.graal.in/

## Special Graal Reborn NPC commands

The Graal Reborn gserver has a couple special NPC commands built in.

join somefile;
    Clientside support for the join command.  When in classic mode, this command searches for somefile.txt and appends the contents to the end of the NPC script.  When in npc-server mode, this appends the script of the class.

singleplayer
    This command is like the sparringzone command.  When placed by itself with no semi-colon inside an NPC, it signifies that the level is "singleplayer."  (SEE: Singleplayer Levels).

## Singleplayer Levels

The Graal Reborn gserver has the ability to toggle a level as "singleplayer."  In this mode, the user cannot see any other player in the level.
Any changes they make to the level are not replicated to other users.  They are, in essence, in a level by themselves.

To activate singleplayer mode, add an NPC to the level and add the single command "singleplayer" to the level, much like how the "sparringzone" command works.

## Group Maps

Like singleplayer levels, group maps allow only players in a group to see each other in a level.
Player groups can be managed via the gr.setgroup and gr.setlevelgroup triggeractions (SEE: Graal Reborn special triggeractions).

Individual levels cannot be set as group levels; instead, an entire map must be specified as a group map.
The "groupmaps" server option specifies a comma-delimited list of maps that can contain groups.

## Graal Reborn special client flags

There are a few special client flags built into the gserver.  These are:
- gr.x
- gr.y
- gr.z

These flags are used by the `-gr_movement` weapon included in the server weapons folder to simulate the smooth movement as found in the Graal clients 2.3 and up.

If you don't want the gserver to recognize these flags, set the flaghack_movement setting to false in serveroptions.txt.

Also, if flaghack_ip is enabled in the serveroptions.txt file, you can gain access to the following:
- gr.ip

## Graal Reborn special triggeractions

The Graal Reborn gserver has a couple unique triggeractions built into it.  They can be enabled/disabled by altering the setting that controls their group in serveroptions.txt.  They are as follows:

### Controlled by the setting triggerhack_weapons:
    triggeraction 0,0,gr.addweapon,weapon1,weapon2,weapon3;
        Adds weapon1, weapon2, and weapon3 to the player's account.

    triggeraction 0,0,gr.deleteweapon,weapon1,weapon2,weapon3;
        Removes weapon1, weapon2, and weapon3 from the player's account.

### Controlled by the setting triggerhack_guilds:
    triggeraction 0,0,gr.addguildmember,guild,account,nickname;
        Adds a player to the specified guild.  Nickname is optional.

    triggeraction 0,0,gr.removeguildmember,guild,account;
        Removes a player from the specified guild.

    triggeraction 0,0,gr.removeguild,guild;
        Removes the guild from the server.

    triggeraction 0,0,gr.setguild,guild,account;
        Sets the player's guild tag to the specified guild.

### Controlled by the setting triggerhack_groups:
    triggeraction 0,0,gr.setgroup,group;
        Adds the player to the specified group.

    triggeraction 0,0,gr.setlevelgroup,group;
        Adds all the players in the level to the specified group.

    triggeraction 0,0,gr.setplayergroup,account,group;
        Adds the specified player to the specified group.

### Controlled by the setting triggerhack_files:
    triggeraction 0,0,gr.appendfile,filename,text;
        Opens the file specified, located in the server's logs directory, and appends a line of text.

    triggeraction 0,0,gr.writefile,filename,text;
        Opens the file specified, located in the server's logs directory, erases all of its contents, and writes a line of text.

    triggeraction 0,0,gr.readfile,filename,line_pos;
        Opens the file specified, located in the server's logs directory, reads the given line number, and returns the contents to the player.
        File contents are returned on the following flags:
            gr.fileerror: String list.  First index is a random number, subsequent indexes are error values.  Error 1 = line_pos was outside of range.  In this case, the next value is the line number returned.
            gr.filedata: The file data.

### Controlled by the setting triggerhack_rc:
    triggeraction 0,0,gr.rcchat,Some chat text;
        Sends some chat text to any logged in RC's.

### Controlled by the setting triggerhack_execscript:
    triggeraction 0,0,gr.es_clear;
        Clears the execscript parameter list.

    triggeraction 0,0,gr.es_set,param1,param2,...;
        Sets the execscript parameter list.

    triggeraction 0,0,gr.es_append,phrase;
        Appends phrase directly to the end of the set parameter list.

    triggeraction 0,0,gr.es,account,script_name;
        Sends the execscript to the specified account, or everybody if ALLPLAYERS was specified.
        View the execscript/readme.txt file for more information.

### Controlled by the setting triggerhack_props:
    triggeraction 0,0,gr.attr1,data;
        Sets data on the specified attribute.  gr.attr1 - gr.attr30 work.

    triggeraction 0,0,gr.fullhearts,amount;
        Sets the player's fullhearts to the specified amount.

### Controlled by the setting triggerhack_levels:
    triggeraction 0,0,gr.updatelevel;
        Updates the current level.

    triggeraction 0,0,gr.updatelevel,levelname;
        Updates the specified level.

### Not controlled by any option:
    triggeraction 0,0,gr.npc.move,id,dx,dy,duration,options;
        Creates a serverside move command for the specified NPC.

    triggeraction 0,0,gr.npc.setpos,id,x,y;
        Sets an NPC's position.
