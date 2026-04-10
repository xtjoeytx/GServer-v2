# GS2Emu Level Group Maps

## What are they?

Group maps provides a way for a group of players to play on their own private copy of a level.
All players will only see and interact with other players who have the same group name assigned to them.

## How to enable

Enable the triggeraction extension in the `serveroptions.txt` file:

    triggerhack_groups = true

Then, list all the gmaps and levels that are group maps in the `serveroptions.txt` file:

    groupmaps = grouplevels_*.nw,groupgmap.gmap

Finally, assign each player the same group name.

    triggeraction 0,0,gr.setgroup,some unique group name;

## How to manage groups

Use the following triggeractions to manage groups:

    gr.setgroup,group
        Assigns the group name to a player.
    gr.setlevelgroup,group
        Assigns the group name to all current players in the level.
    gr.setplayergroup,account,group
        Assigns a group name to a specific player.

Setting the group name to an empty string will remove the player from any group map and put them back in the main level.

    triggeraction 0,0,gr.setgroup,;
