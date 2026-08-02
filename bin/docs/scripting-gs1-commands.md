# GS1 Commands

---
## addguildmember

`addguildmember guild,account,nickname;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Adds an `account` and `nickname` combination to the specified `guild`.

---
## addstring

`addstring identifier,text;`

> introduced: 2.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Adds `text` to the string `identifier`.
The string will be formatted in the CSV format.

```
setstring test,Hello;
addstring test,World;
// test = Hello,World
```

---
## addtiledef

`addtiledef filename,levelstart,type;`

> introduced: 2.04<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Swaps the tileset for all levels whose filenames start with `levelstart`.
The `type` parameter identifies how the tiles are layed out.

| Type | Format                 |
|------|------------------------|
| 0    | pics1.png              |
| 1    | (new order)            |
| 5    | picso.png (3D terrain) |

---
## addtiledef2

`addtiledef2 filename,levelstart,x,y;`

> introduced: 2.04<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Like [addtiledef](#addtiledef), but replaces sections of a tileset.
The `x` and `y` parameters are the pixel coordinates on the tileset where the new image will be overlayed.

Supports animated images.

---
## addweapon

`addweapon name;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Gives the named weapon to the player.

---
## attachplayertoobj

`attachplayertoobj objecttype,id;`

> introduced: 2.04<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Attaches a player to an NPC.
`objecttype` is the type of the object to attach to, but only NPCs were supported (value `0`).
The `id` is the NPC's ID (`npcs[].id`).

```
if (playertouchsme) {
  attachplayertoobj 0,id;
}
```

---
## blockagain

`blockagain;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Enables the NPCs collision bounding box.
Players can collide with the NPC and the NPC will be picked up by functions like [testnpc()](scripting-gs1-functions.md#testnpc).

---
## blockagainlocal

`blockagainlocal;`

> introduced: 1.34<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Enables the NPC's collision bounding box, but only for the individual player.

---
## callnpc

`callnpc index,eventname,params;`

> introduced: 1.39rev2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Manually executes a named event on an NPC.

| Parameter | Description |
| --------- | ----------- |
| index     | The level index of the npc.  See: [npcscount](scripting-gs1-variables.md#npcs).
| eventname | The name of an event to trigger, like [playertouchsme](scripting-gs1-events.md#playertouchsme). |
| params    | Optional parameters that can be accessed in the target NPC through [#p(index)](scripting-gs1-messagecodes.md#p) and [paramscount](scripting-gs1-variables.md#triggeraction). |

```
for (i = 0; i < npcscount; i++) {
  if (npcs[i].save[0] == 10) {
    // Call playertouchsme with the player's id and account name.
    callnpc i,playertouchsme,playerid,#a;
  }
}
```

---
## callweapon

`callweapon index,eventname,params;`

> introduced: possibly 2.04<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Manually executes a named event on a weapon.

| Parameter | Description |
| --------- | ----------- |
| index     | The index of the weapon.  See: [weaponscount](scripting-gs1-variables.md#game-client).
| eventname | The name of an event to trigger, like [playertouchsme](scripting-gs1-events.md#playertouchsme). |
| params    | Optional parameters that can be accessed in the target NPC through [#p(index)](scripting-gs1-messagecodes.md#p) and [paramscount](scripting-gs1-variables.md#triggeraction). |

---
## canbecarried

`canbecarried;`

> introduced: beta 7<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Allows the NPC to be carried.

---
## canbepulled

`canbepulled;`

> introduced: 1.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Allows the NPC to be pulled.

---
## canbepushed

`canbepushed;`

> introduced: 1.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Allows the NPC to be pushed.

---
## cannotbecarried

`cannotbecarried;`

> introduced: beta 7<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Prevents an NPC from being carried.

---
## cannotbepulled

`cannotbepulled;`

> introduced: 1.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Prevents an NPC from being pulled.

---
## cannotbepushed

`cannotbepushed;`

> introduced: 1.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Prevents an NPC from being pushed.

---
## cannotwarp

`cannotwarp;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Prevents an NPC from changing levels via level links.

---
## canwarp

`canwarp;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Allows an NPC to change levels via any level link.

---
## canwarp2

`canwarp2;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Allows an NPC to change levels, but only if the level is on the current bigmap.
Used to allow travel to other overworld levels.

---
## carryobject

`carryobject carryobjecttype;`

> introduced: 1.37<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ⚠️<br>
official serverside: ❌<br>

Makes an NPC carry an object.  `carryobjecttype` is one of the [carry objects](scripting-gs1-variables.md#carry-objects).

The NPC doesn't pass the carry item property, so it doesn't work correctly in online mode.

---
## changeimgcolors

`changeimgcolors index,red,green,blue,alpha;`

> introduced: 2.00<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Default: `changeimgcolors n,1,1,1,1;`

Adjusts the alpha blending of a [showimg](#showimg) image.
`index` is the index number of the [showimg](#showimg) image.

The [showimg](#showimg) equivalent of [setcoloreffect](#setcoloreffect).

The change in alpha blending is only visible to other players on client versions 2.16 and above.

---
## changeimgmode

`changeimgmode index,mode;`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

The change in rendering mode is only visible to other players on client versions 2.16 and above.

See: [seteffectmode](#seteffectmode).

---
## changeimgpart

`changeimgpart index,x,y,width,height;`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Alters a [showimg](#showimg) image to only render a part of the image.
The parameters control the location, in pixels, of the rectangle to render in the image.

---
## changeimgvis

`changeimgvis index,drawingheight;`

> introduced: 1.40<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Default: `changeimgvis n,2;`

Changes the layer a [showimg](#showimg) image renders at.

The change in layer is only visible to other players on client versions 2.16 and above.

See: [Draw layers](scripting-gs1-variables.md#draw-layers).

---
## changeimgzoom

`changeimgzoom index,zoomfactor;`

> introduced: 2.00<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Default: `changeimgzoom n,1;`

Changes the zoom effect of a [showimg](#showimg) image.
`index` is the index number of the [showimg](#showimg) image.

The zoom is applied based on the center of the image, so the actual (X, Y) origin remains unchanged.

The change in zoom level is only visible to other players on client versions 2.16 and above.

---
## copylevel

`copylevel oldfile,newfile;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Makes a copy of a level.

---
## copystrings

`copystrings from-prefix,to-prefix;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Copies a range of strings that start with `from-prefix`, replacing with `to-prefix`.
The old strings still exist.

```
copystrings event_,previousevent_;
// event_level -> previousevent_level
// event_winner -> previousevent_winner
```

---
## deletelevel

`deletelevel filename;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Deletes a level from the server.

---
## deletestring

`deletestring identifier,index;`

> introduced: 2.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Deletes a string at the specified `index` in a CSV formatted string list, stored in `identifier`.

```
setstring test,This,is,a,test;
deletestring test,2;
// test = This,is,test
```

---
## destroy

`destroy;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Deletes the NPC.

---
## detachplayer

`detachplayer;`

> introduced: 2.04<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Detaches the player from the NPC they are attached to.

---
## disabledamagereactions

`disabledamagereactions;`

> introduced: [GR]<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

**gs2emu** only command.

Disables NPC damage reactions (taking damage and being pushed around like a player).

---
## disabledefmovement

`disabledefmovement;`

> introduced: revealed 2.02<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Disables the default player movement and behavior.

---
## disablemap

`disablemap;`

> introduced: 2.14rev7<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Disables the ability for players to use maps.

---
## disablepause

`disablepause;`

> introduced: 2.14rev7<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Disables the ability for players to pause.

---
## disableselectweapons

`disableselectweapons;`

> introduced: 2.04<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Disables the ability for players to select weapons.

---
## disableweapons

`disableweapons;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Disables weapons for a player.

---
## dontblock

`dontblock;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Disables an NPC's collision bounding box.
The NPC will no longer block others, nor be picked up with functions like [testnpc()](scripting-gs1-functions.md#testnpc).

---
## dontblocklocal

`dontblocklocal;`

> introduced: 1.34<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Enables the collision bounding box for the NPC, but only for the local player.

---
## drawaslight

`drawaslight;`

> introduced: 2.00<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Draws the NPC on the 3rd layer (above [seteffect](#seteffect)).

See: [Draw layers](scripting-gs1-variables.md#draw-layers)

---
## drawoverplayer

`drawoverplayer;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Draws the NPC on the 2nd layer (above players and NPCs).

See: [Draw layers](scripting-gs1-variables.md#draw-layers)

---
## drawovertrees

`drawovertrees;`

> introduced: (newworld)<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Normally does nothing.  On **gs2emu**, will draw the NPC on the 1st layer.

See: [Draw layers](scripting-gs1-variables.md#draw-layers)

---
## drawunderplayer

`drawunderplayer;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Draws the NPC on the 0th layer (below players and NPCs).

---
## enabledamagereactions

`enabledamagereactions;`

> introduced: [GR]<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

**gs2emu** only command.

Enables NPC damage reactions (they take damage and are pushed around like a player).

---
## enabledefmovement

`enabledefmovement;`

> introduced: revealed 2.02<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Enables default player movement and behaviors.

---
## enablefeatures

`enablefeatures flags;`

> introduced: 2.16<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Can be used to disable certain features of the game client.  The `flags` parameter is a sum of the following features:

| Value | Feature |
| ----- | ------- |
|      1 | Map (M key) |
|      2 | Pause (P key) |
|      4 | Weapons select (Q key) |
|      8 | Spar ratings (R key) |
|   0x10 | Item dropping (S+A key combination) |
|   0x20 | Weapon switching (S+D key combination) |
|   0x40 | Chat bar (TAB key) |
|   0x80 | All chat text |
|  0x100 | Hearts over player's heads |
|  0x200 | Nicknames under characters |
|  0x400 | toall / PM-icons on the minimap |
|  0x800 | Player profiles (right-click) |
| 0x1000 | Emoticons (CTRL+key) |
| 0x2000 | Alt+5 for making snapshots |
| 0x4000 | Alt+8/9 for zooming |
| 0x8000 | F2 log window (savelog) |

The variable [allfeatures](scripting-gs1-variables.md#game-client) is a sum of all the values.

```
// Turn off profiles.
enablefeatures allfeatures - 0x800;
```

---
## enablemap

`enablemap;`

> introduced: 2.14rev7<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Allows the player to use the map, if previously prevented.

---
## enablepause

`enablepause;`

> introduced: 2.14rev7<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Allows the player to pause, if previously prevented.

---
## enableselectweapons

`enableselectweapons;`

> introduced: 2.04<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Allows the player to select weapons, if previously prevented.

---
## enableweapons

`enableweapons;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Allows the player to use weapons, if previously prevented.

---
## explodebomb

`explodebomb index;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Explodes the specified bomb in the level.

See: [Bombs variables](scripting-gs1-variables.md#bombs).

---
## followplayer

`followplayer;`

> introduced: beta 5<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Makes the NPC follow the player.

---
## freezeplayer

`freezeplayer seconds;`

> introduced: around 1.20<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Freezes the player for the specified number of seconds.
The [playerfreezetime](scripting-gs1-variables.md#players) variable counts down to 0, upon which the player will be unfrozen.

---
## freezeplayer2

`freezeplayer2;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Freezes a player.

---
## hide

`hide;`

> introduced: beta 2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Hides an NPC, disabling all collision.

---
## hideimg

`hideimg index;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Hides the specified [showimg](#showimg) image.

---
## hideimgs

`hideimgs index_start,index_end;`

> introduced: 2.17<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Hides [showimg](#showimg) images in the range `[index_start..index_end]`.

---
## hidelocal

`hidelocal;`

> introduced: 1.34<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Hides an NPC, disabling all collision, but only for the local player.

---
## hideplayer

`hideplayer seconds;`

> introduced: 1.22<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Hides the player for the specified number of seconds.

---
## hidesword

`hidesword seconds;`

> introduced: around 1.20<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Hides the player's sword for the specified number of seconds.

---
## hitcompu

`hitcompu index,halfhearts,fromx,fromy;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Hits a baddy for `halfhearts` damage, with the push-back being calculated from the tile position (`fromx`, `fromy`).
The `index` is the baddy's position in the [compus\[\]](scripting-gs1-variables.md#baddies) array.

---
## hitnpc

`hitnpc index,halfhearts,fromx,fromy;`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Hits an NPC for `halfhearts` damage, with the push-back being calculated from the tile position (`fromx`, `fromy`).
The `index` is the NPC's position in the [npcs\[\]](scripting-gs1-variables.md#npcs) array.

---
## hitobjects

`hitobjects halfhearts,x,y;`

> introduced: 2.01<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Hits any object at tile position (`x`, `y`), for `halfhearts` damage.
Objects hit are players, NPCs, and baddies.

---
## hitplayer

`hitplayer index,halfhearts,fromx,fromy;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Hits a player for `halfhearts` damage, with the push-back being calculated from the tile position (`fromx`, `fromy`).
The `index` is the player's position in the [players\[\]](scripting-gs1-variables.md#players) array.

---
## hurt

`hurt halfhearts;`

> introduced: beta 2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Hurts the current player for `halfhearts` damage.

---
## insertstring

`insertstring identifier,index,text;`

> introduced: 2.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Inserts a string of `text` at the specified `index` in the CSV formatted string `identifier`.

```
setstring test,This,is,test;
insertstring test,2,a;
// test = This,is,a,test;
```

---
## join

`join class;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Adds a script class to a NPC or weapon.

---
## lay

`lay itemname;`

> introduced: beta 2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Drops an item on the ground at the feet of the NPC.
`itemname` is one of the valid [Item names](scripting-gs1-variables.md#item-names).

---
## lay2

`lay2 itemname,x,y;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Drops an item on the ground at the specified tile position.
`itemname` is one of the valid [Item names](scripting-gs1-variables.md#item-names).

---
## loadmap

`loadmap map;`

> introduced: possibly 2.12, revealed (???)<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Loads a gmap into memory in the client.  The `map` parameter should not include the `.gmap` extension.

---
## message

`message text;`

> introduced: 1.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the chat message of the NPC.

---
## move

`move dx,dy,time,options;`

> introduced: 2.03<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Moves the NPC.

| Parameter | Description |
| --------- | ----------- |
| dx      | The number of tiles to move in the X direction. |
| dy      | The number of tiles to move in the Y direction. |
| time    | How many seconds the move will take to finish. |
| options | Sum of flags to control how the movement will be processed. |

`options` is the sum of: `cachingmode` + `blockcheck` + `informmewhendone` + `applydirection`

| Caching mode | Description |
| ------------ | ----------- |
| 0 | Previous movements will be finished immediately. |
| 1 | Cache movements, but immediately finish previous movements if remaining distance is over 5 tiles.
| 2 | Append movement (limit to 100 cached movements). |

| Block check | Description |
| ----------- | ----------- |
| 0 | No collision detection. |
| 4 | Stop when the NPC collides with an object. |

| Inform when done | Description |
| ---------------- | ----------- |
| 0 | Do not inform. |
| 8 | Trigger the [movementfinished](scripting-gs1-events.md#movementfinished) event when the movement is done. |

| Apply direction | Description |
| --------------- | ----------- |
| 0  | Do nothing. |
| 16 | Change the NPC's `dir` property to face towards the direction of travel. |

```
// Queue up a move 10 tiles to the right, in 0.5 seconds,
//   make the NPC look to the right,
//   walk through walls,
//   and announce when finished, so the gani can be reset.
setcharani walk,;
move 10, 0, 0.5, 2 + 8 + 16;

if (movementfinished) {
  setcharani idle,;
}
```

---
## noplayerkilling

`noplayerkilling;`

> introduced: 1.00<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Disables player killing in the level.
Also disables collision detection for players, which makes [testplayer()](scripting-gs1-functions.md#testplayer) not identify players.

---
## noplayeronwall

`noplayeronwall;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Prevents [onwall()](scripting-gs1-functions.md#onwall) checks from including players.

---
## openurl

`openurl url;`

> introduced: beta 5<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Opens the specified `url` in your web browser.
The `url` should not include the leading `http://`.

---
## openurl2

`openurl2 url,width,height;`

> introduced: 2.00<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Opens the specified `url` in your web browser, in a window of the specified dimensions.
The `url` should not include the leading `http://`.

---
## play

`play filename;`

> introduced: beta 2<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Plays sound.

---
## play2

`play2 filename,x,y,volume;`

> introduced: 2.10<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Plays sound originating at the specified tile position on the level.
The `volume` is a value between `[0.0, 1.0]`.

---
## playlooped

`playlooped filename;`

> introduced: 1.37<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Plays a looping sound.

---
## putbomb

`putbomb power,x,y;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Places a bomb at the specified tile position.

| Power | Bomb type |
| ----- | --------- |
| 1 | Normal    |
| 2 | Superbomb |
| 3 | Joltbomb  |

---
## putcomp

`putcomp baddyname,x,y;`

> introduced: beta 4<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Places a new baddy in the level at the specified position.
`baddyname` must be one of the [Baddy names](scripting-gs1-variables.md#baddy-names).

---
## putexplosion

`putexplosion radius,x,y;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Places an explosion in the level at the specified position.

---
## putexplosion2

`putexplosion2 power,radius,x,y;`

> introduced: 1.40<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Places an explosion in the level at the specified position.

| Power | Bomb type |
| ----- | --------- |
| 1 | Normal    |
| 2 | Superbomb |
| 3 | Joltbomb  |

---
## puthorse

`puthorse imagefile,x,y;`

> introduced: beta 7<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Places a horse in the level at the specified position.

---
## putleaps

`putleaps leaptype,x,y;`

> introduced: beta 5<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Plays a "leap" at the specified position.
A "leap" is the effect that plays when an object is destroyed.

| Leaptype | Description |
| -------- | ----------- |
| 0 | Bush  |
| 1 | Swamp |
| 2 | Stone |
| 3 | Sign  |
| 4 | Ball ([shootball](#shootball)) |
| 5 | Water splash |

---
## putnewcomp

`putnewcomp baddyname,x,y,imagefile,hearts;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Places a new baddy in the level at the specified position.
`baddyname` must be one of the [Baddy names](scripting-gs1-variables.md#baddy-names).
The `imagefile` and `hearts` of the baddy can be altered, but the baddy will still perform like the chosen type.

---
## putnpc

`putnpc imagefile,scriptfile,x,y;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Adds a new NPC to the level at the specified position.
The `imagefile` is the default image of the NPC, and the `scriptfile` is a `.txt` file with the script it should use.

---
## putnpc2

`putnpc2 x,y,{ script };`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Adds a new NPC to the level at the specified position.
`script` is directly embedded GS1 script for the new baddy.

```
putnpc2 x, y, { if (created) join bomb; };
```

---
## putobject

`putobject objectname,x,y;`

> introduced: around 1.20<br>
removed: (???)<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Puts an object on the level at the specified position.
The `objectname` is the name of a pre-defined object in the level editor.

---
## reducebombs

`reducebombs amount;`

> introduced: beta 3<br>
removed: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Reduces the NPCs `bombs` property by the specified amount.

---
## reducedarts

`reducedarts amount;`

> introduced: beta 3<br>
removed: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Reduces the NPCs `darts` property by the specified amount.

---
## reducerupees

`reducerupees amount;`

> introduced: beta 3<br>
removed: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Reduces the NPCs `rupees` property by the specified amount.

---
## reflectarrow

`reflectarrow index;`

> introduced: 1.36<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Reflects the arrow at the specified `index`.
The `index` is the arrow's position in the [arrows\[\]](scripting-gs1-variables.md#arrows) array.
Reflected arrows are not synchronized with other clients.

---
## removearrow

`removearrow index;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Removes the arrow at the specified `index`.
The `index` is the arrow's position in the [arrows\[\]](scripting-gs1-variables.md#arrows) array.

---
## removebomb

`removebomb index;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Removes the bomb at the specified `index`.
The `index` is the bomb's position in the [bombs\[\]](scripting-gs1-variables.md#bombs) array.

---
## removecompus

`removecompus;`

> introduced: beta 3<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Removes the baddy at the specified `index`.
The `index` is the baddy's position in the [compus\[\]](scripting-gs1-variables.md#baddies) array.

---
## removeexplo

`removeexplo index;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Removes the explosion at the specified `index`.
The `index` is the explosion's position in the [explos\[\]](scripting-gs1-variables.md#explos) array.

---
## removeguild

`removeguild guild;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Removes the specified guild from the server.

---
## removeguildmember

`removeguildmember guild,account,nick;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Removes a player from the specified guild.

---
## removehorse

`removehorse index;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Removes the horse at the specified `index`.
The `index` is the horse's position in the [horses\[\]](scripting-gs1-variables.md#horses) array.

---
## removeitem

`removeitem index;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Removes the item at the specified `index`.
The `index` is the item's position in the [items\[\]](scripting-gs1-variables.md#items) array.

---
## removestring

`removestring identifier,text;`

> introduced: 2.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Removes all instances of `text` from the CSV formatted string `identifier`.

```
setstring test,This,is,a,is,a,test;
removestring test,is;
// test = This,a,a,test
```

---
## removetiledefs

`removetiledefs levelstart;`

> introduced: 2.04<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Removes all tile overrides by the [addtiledef](#addtiledef) and [addtiledef2](#addtiledef2) commands for level filenames that start with `levelstart`.

---
## removeweapon

`removeweapon name;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Removes the specified weapon from the player.

---
## replaceani

`replaceani oldgani,newgani;`

> introduced: 2.04<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Overrides a default gani animation for the player.

```
replaceani walk,mywalk;
```

---
## replacestring

`replacestring identifier,index,text;`

> introduced: 2.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Replaces a string at the specified `index` inside a CSV formatted string `identifier`.

```
setstring test,This,is,a,test;
replacestring test,3,triumph!;
// test = This,is,a,triumph!
```

---
## resetfocus

`resetfocus;`

> introduced: possibly 2.12, revealed 2.16<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Changes the current focus point back to the player's character.

---
## saveinfo

`saveinfo key,value;`

> introduced: ???
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ❌ (won't implement)
official serverside: ✅ (newmain) ❌ (modern)<br>

Internal command to save data directly to the official database.

---
## savelevel

`???`

> introduced: [GR]<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ❌<br>
official serverside: ⚠️<br>

Saves the level.

Not currently implemented, and it is currently unknown if official supported this.

---
## savelog

`savelog text;`

> introduced: possibly 2.12, revealed (???)<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Saves the specified `text` string into the `logs/npclog.txt` file.

---
## savelog2

`savelog2 filename,text;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Saves the specified `text` string into the specified log file.

---
## say

`say signindex;`

> introduced: beta 2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Displays the level sign at the specified index.
The `index` is the sign's position in the [signs\[\]](scripting-gs1-variables.md#signs) array.

---
## say2

`say2 text;`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Displays a custom sign with the specified `text`.
Use the [#b](scripting-gs1-messagecodes.md#b) message code to insert line-breaks.

---
## sendpm

`sendpm text;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sends a private message to the currently active player.

```
with (getplayer(name)) {
  sendpm Oh no!;
}
```

---
## sendrpgmessage

`sendrpgmessage text;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sends the `text` to the player's F2 log window.

---
## sendtonc

`sendtonc text;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sends the `text` to the NC (NPC Control) chat.

---
## sendtorc

`sendtorc text;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sends the `text` to the RC (Remote Control) chat.

---
## serverwarp

`serverwarp servername;`

> introduced: 2.14<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Warps the player to a new server.

---
## set

`set flag;`

> introduced: beta 2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets a flag.  The storage location of the flag depends on the flag's prefix.

See: [Variable prefixes](scripting-gs1-variables.md#variable-prefixes).

---
## setani

`setani gani;`
`setani gani,attribs;`

> introduced: 2.00<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the player's gani animation.

---
## setarray

`setarray identifier,size;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Creates a new `identifier` array of the specified `size`.
Can resize existing arrays, keeping the contents.

---
## setbackpal

`setbackpal filename;`

> introduced: 1.27<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Swaps the tileset's palette with the palette of the specified file.

---
## setbacktile

`setbacktile tileindex;`

> introduced: (newworld)<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Changes which tile is rendered behind tiles with transparent elements.
Only valid for newworld clients.

---
## setbacktile2

`setbacktile2 tileindex,x,y,width,height;`

> introduced: (newworld)<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Changes which tile is rendered behind tiles with transparent elements for a given region of the level.
Only valid for newworld clients.

---
## setbeltcolor

`setbeltcolor color;`

> introduced: beta 3<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the player's belt color.

`color` must be a [Color name](scripting-gs1-variables.md#colors).

---
## setbody

`setbody filename;`

> introduced: 1.40<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the player's body image.

---
## setbow

`setbow filename;`

> introduced: around 1.20<br>
removed: 2.00<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the player's bow image.

The bow image is only supported for the `classic` generation.

---
## setcharani

`setcharani gani;`
`setcharani gani,attribs;`

> introduced: 2.00<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the NPC's gani animation.

---
## setchargender

`setchargender gender;`

> introduced: 2.00<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the NPC's gender.
`gender` is either `male` or `female`.
The gender property only affects the pitch of the voice for certain actions.

---
## setcharprop

`setcharprop messagecode,text;`

> introduced: 1.25<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets an NPC's property value.

Any messagecode that targets the character or NPC, and is writable, can be used.

See: [Message codes](scripting-gs1-messagecodes.md).

---
## setcoatcolor

`setcoatcolor color;`

> introduced: beta 3<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the player's coat color.

`color` must be a [Color name](scripting-gs1-variables.md#colors).

---
## setcoloreffect

`setcoloreffect red,green,blue,alpha;`

> introduced: 2.00<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Default: `setcoloreffect 1,1,1,1;`

Draws the NPC using alpha blending.
The parameters are in the range of `[0.0..1.0]`.

---
## setcursor

`setcursor cursornumber;`

> introduced: possibly 2.12, revealed (???)<br>
removed: 4.x<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Changes the player's mouse cursor.

| Cursor Number | Description |
| ------------- | ----------- |
|  1 | Hidden |
|  2 | Normal |
|  3 | Cross |
|  4 | Text |
|  5 | Hidden? |
|  6 | Resize Lower Left to Upper Right |
|  7 | Resize Up Down |
|  8 | Resize Upper Left to Lower Right |
|  9 | Resize Left Right |
| 10 | Up arrow |
| 11 | Hourglass |
| 12 | File |
| 13 | Not allowed |
| 14 | Break adjust Left Right |
| 15 | Break adjust Up Down |
| 16 | Multiple files |
| 17 | SQL Hourglass |
| 18 | Not allowed |
| 19 | Mouse + Hourglass |
| 20 | Mouse + ? |
| 21 | Pointing Hand |
| 22 | Four directional arrow |

---
## setcursor2

`setcursor2 filename;`

> introduced: possibly 2.12, revealed (???)<br>
removed: 4.x<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Sets the player's mouse cursor to a custom cursor image.

The cursor must be in the `.cur` format.

---
## seteffect

`seteffect red,green,blue,alpha;`

> introduced: 2.00<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Default: `seteffect 0,0,0,0;`

Adds a tinted overlay to the game.
The `red`, `green`, `blue`, and `alpha` values are a floating-point number in the range `[0.0..1.0]`.
Using values of 0.0 and increasing the alpha makes the screen darker.

---
## seteffect

`seteffect red,green,blue;`

> introduced: around 1.20<br>
removed: 1.33rev1<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Adds a tinted overlay to the game.
The `red`, `green`, and `blue` values are a whole number in the range `[0..255]`.
Use `unseteffect;` to remove.

---
## seteffectmode

`seteffectmode mode;`

> introduced: 2.16<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Default: `seteffectmode 0;`

Changes the rendering method of NPCs altered by [seteffect](#seteffect).

| Mode | Effect |
| ---- | ------ |
| 0 | Lights.  Colors are added, with alpha specifying intensity. |
| 1 | Transparency.  Alpha specifies how much is visible. |
| 2 | Holes.  Colors are subtracted. |

---
## setfocus

`setfocus x,y;`

> introduced: possibly 2.12, revealed 2.16<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Adjusts the focus of the game camera to the specified tile position.

---
## setgender

`setgender gender;`

> introduced: 2.00<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the player's gender.
`gender` is either `male` or `female`.
The gender property only affects the pitch of the voice for certain actions.

---
## setgif

`setgif image;`

> introduced: beta 2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the NPC image.

Identical to: [setimg](#setimg).

---
## setgifpart

`setgifpart filename,x,y,width,height;`

> introduced: 1.30<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the NPC image.

Identical to: [setimgpart](#setimgpart).

---
## sethead

`sethead filename;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the player's head image.

---
## setimg

`setimg filename;`

> introduced: 1.40<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the NPC's image.

Identical to: [setgif](#setgif).

---
## setimgpart

`setimgpart filename,x,y,width,height;`

> introduced: 1.40<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the NPC's image.

Identical to: [setgifpart](#setgifpart).

---
## setletters

`setletters filename;`

> introduced: 1.27<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Changes the `letters.png` file the client uses to render signs.

---
## setlevel

`setlevel filename;`

> introduced: beta 3<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the player's level.
Does not change their X and Y position.

---
## setlevel2

`setlevel2 filename,x,y;`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the player's level and positions them at the specified tile position.

---
## setmap

`setmap imgfile,levelsfile,offmap-x,offmap-y;`

> introduced: 1.23<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the bigmap.

The `imgfile` is the image displayed when the player uses the Map (M) key.

The `levelsfile` is a CSV formatted list of levels that make up the map.

Players that are not on the map are rendered at the pixel position specified by `offmap-x` and `offmap-y`.

---
## setminimap

`setminimap imgfile,levelsfile,x,y;`

> introduced: 1.23<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the minimap.

The `imgfile` is the image displayed in the corner of the game client.

The `levelsfile` is a CSV formatted list of levels that make up the map.

Players that are not on the map are rendered at the pixel position specified by `offmap-x` and `offmap-y`.

---
## setmusicvolume

`setmusicvolume left,right;`

> introduced: 2.00<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Adjusts the music volume for the left and right speakers.
Use a value between 0.0 and 1.0.

---
## setplayerdir

`setplayerdir dir;`

> introduced: beta 3<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the player's direction.

---
## setplayerprop

`setplayerprop messagecode,text;`

> introduced: 1.25<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets a player's property value.

Any messagecode that targets the character or player, and is writable, can be used.

See: [Message codes](scripting-gs1-messagecodes.md).

---
## setplayerx

`setplayerx value;`

> introduced: beta 3<br>
removed: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the player's X position.

---
## setplayery

`setplayery value;`

> introduced: beta 3<br>
removed: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the player's Y position.

---
## setpm

`setpm text;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the `text` that will be sent to the player when they PM the NPC-Server player.

---
## setshape

`setshape type,width,height;`

> introduced: 2.04<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the collision boundaries of the NPC.

Only one `type` is supported: 1 = rectangle

---
## setshape2

`setshape2 width,height,tiletypes;`

> introduced: 2.04<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Sets the shape of the NPC and the tiles that make up that shape.

`tiletypes` is an array of tiles.  The dimensions must match `width * height`.

See: [Tile types](scripting-gs1-variables.md#tile-types).

```
setimg chair.png;
setshape2 2,2,{3,3,3,3};
```

---
## setshield

`setshield image,power;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the shield of the player.
`power` can be `0 <= power <= 10`.

---
## setshoecolor

`setshoecolor color;`

> introduced: beta 3<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the player's shoe color.

`color` must be a [Color name](scripting-gs1-variables.md#colors).

---
## setshootparams

`setshootparams params,...;`

> introduced: revealed 2.14<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets string parameters that are sent with a shoot projectile.  Parameters are a CSV formatted string.

---
## setskincolor

`setskincolor color;`

> introduced: beta 3<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the player's skin color.

`color` must be a [Color name](scripting-gs1-variables.md#colors).

---
## setsleevecolor

`setsleevecolor color;`

> introduced: beta 3<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets the player's sleeve color.

`color` must be a [Color name](scripting-gs1-variables.md#colors).

---
## setspritesimage

`setspritesimage filename;`

> introduced: 5.00rev6<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Overrides the `sprites.png` image.

---
## setstatusimage

`setstatusimage filename;`

> introduced: 5.00rev6<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Overrides the `state.png` image.

---
## setstring

`setstring identifier,text;`

> introduced: 1.27<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sets a string flag.  The storage location of the flag depends on the flag's prefix.

See: [Variable prefixes](scripting-gs1-variables.md#variable-prefixes).

---
## setsword

`setsword image,power;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the player's sword.
The `power` can be `-20 <= power <= 20`.

---
## seturllevel

`seturllevel url;`

> introduced: beta 5<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Warps the player to a level hosted on a website.
The `url` must not include the leading `http://`.

---
## setx

`setx value;`

> introduced: beta 2<br>
removed: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the NPC's X position.

---
## sety

`sety value;`

> introduced: beta 2<br>
removed: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Sets the NPC's Y position.

---
## setz

`setz x,y,width,height,a,b,c,d;`

> introduced: possibly 2.12, revealed (???)<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Alters the Z height of the terrain.

The `x`, `y`, `width`, and `height` parameters specify a section of the level to adjust.

The `a`, `b`, `c`, and `d` parameters set the Z values of the corners of the rectangle.

[updateterrain](#updateterrain) must be called for the effect to become visible.

The terrain modifications are not sent to the server.

---
## setzoomeffect

`setzoomeffect zoomfactor;`

> introduced: 2.00<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Default: `setzoomfactor 1;`

Zooms an NPC in or out.
The zoom happens from the center of the NPC and is only a visual effect.
Zooming does not change the NPC's actual position or collision boundaries.

---
## shoot

`shoot x,y,z,angle,zangle,power,gani,ganiattribs;`

> introduced: revealed 2.14<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Shoots a projectile.

| Param | Description |
| ----- | ----------- |
| X      | The starting X tile position of the projectile. |
| Y      | The starting Y tile position of the projectile. |
| Z      | The starting Z position of the projectile, from ground level.  Do not manually add the ground Z height for 3D terrain.  This is relative to ground level. |
| Angle  | The direction of the projectile, in radians, where east is `0`, north is `pi/2`, west is `pi`, and south is `3*pi/2`.  Range is `[0..2*pi]`. |
| ZAngle | The upward angle of the projectile, in radians, where `0` is flat horizontal, straight up is `pi/2`, and flat horizontal, backwards, is `pi`.  Can be negative to shoot downwards.  Range of `[-pi..pi]`. |
| Power  | Launch strength of the projectile.  Value is tiles traveled every 0.05 seconds, up to 5 tiles.  When power is `0`, the projectile has no gravity and flies like a classic arrow, moving 1 tile per 0.05 seconds (20 per second).  Range of `[0.0..5.0]`. |
| Gani   | The gani animation to play for the projectile. |
| Ganiattribs | A CSV formatted string of gani attributes. |

Until client 5.1, the `X`, `Y`, and `Z` parameters have half tile (`0.5`) resolution.
On client 5.1 and up, they have pixel resolution.

The resolution of the `power` parameter is 1/44'th of a tile (`0.022727...`).

If the gani is multi-directional, the game chooses the direction based on the travel direction of the projectile.
The gani can have 1 frame (static) or 7 frames of animation (rising and falling).
Maximum ascent is frame 1, horizontal is frame 4, and maximum descent is frame 7, with the client transitioning between the frames as it travels in an arc.

Various events are spawned when the projectile lands or hits something.

| Event | Scope | Versions | Condition | Parameters `#p(index)` |
| ----- | ----- | -------- | --------- | ---------- |
| actionprojectile  | 🧑 | <2.17<br>>=2.19 | The projectile hits a player or NPC. | [setshootparams](#setshootparams) |
| actionprojectile  | 🧑 | 2.17x - 2.18x | The projectile hits the ground or hits a player or NPC. | `#p(0)` = X<br>`#p(1)` = Y<br>`#p(2+)` = [setshootparams](#setshootparams) |
| actionprojectile2 | 🧑 | 2.19+ | The projectile hits the ground. | `#p(0)` = X<br>`#p(1)` = Y<br>`#p(2+)` = [setshootparams](#setshootparams) |
| actionprojectile  | 💻 | | A projectile launched by a client hits the ground or hits an NPC. | `#p(0)` = X<br>`#p(1)` = Y<br>`#p(2+)` = [setshootparams](#setshootparams) |
| actionsprojectile | 💻 | | A projectile launched by a serverside NPC hits the ground or hits an NPC. | `#p(0)` = X<br>`#p(1)` = Y<br>`#p(2+)` = [setshootparams](#setshootparams) |

Clients 2.17 through 2.18rev1 briefly triggered `actionprojectile` upon hitting the ground, but this caused problems and was reverted in 2.19.
Instead, `actionprojectile2` was created to handle ground hits.

The [setshootparams](#setshootparams) command can be used to embed launch parameters into the projectile.
It must be called _BEFORE_ the projectile is shot.

Shoot projectiles are affected by the [gravity](scripting-gs1-variables.md#levels) variable.
Until client version 5.1, this variable had to be changed on both the client and server, or else projectiles would get out of sync.
Starting on client 5.1, the [gravity](scripting-gs1-variables.md#levels) variable is embedded in the projectile, so each projectile can have its own gravity.

---
## shootarrow

`shootarrow direction;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Shoots an arrow projectile in the specified [direction](scripting-gs1-variables.md#directions).

---
## shootball

`shootball;`
`[GR] shootball dir;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Shoots a ball projectile at the player.

**gs2emu** adds an optional [direction](scripting-gs1-variables.md#directions) in the serverside version.

---
## shootfireball

`shootfireball dir;`

> introduced: beta 9<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Shoots a fireball projectile in the specified [direction](scripting-gs1-variables.md#directions).

---
## shootfireblast

`shootfireblast dir;`

> introduced: beta 9<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Shoots a fireblast projectile in the specified [direction](scripting-gs1-variables.md#directions).

---
## shootnuke

`shootnuke dir;`

> introduced: beta 9<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Shoots a nuke projectile in the specified [direction](scripting-gs1-variables.md#directions).

---
## show

`show;`

> introduced: beta 2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Makes an NPC visible.

---
## showani

`showani index,x,y,direction,gani,params;`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Shows a gani animation as a [showimg](#showimg) image.

`index` ranges between `[0..199]` cause the showimg to appear for other players,
while ranges `[200...]` and above are only visible to the player who issues the command.

See: [showimg](#showimg).

---
## showani2

`showani2 index,x,y,z,direction,gani,params;`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Shows a gani animation as a [showimg](#showimg) image.

`index` ranges between `[0..199]` cause the showimg to appear for other players,
while ranges `[200...]` and above are only visible to the player who issues the command.

See: [showimg](#showimg).

---
## showcharacter

`showcharacter;`

> introduced: 1.25<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Turns the NPC into a character.
The NPC can be configured using the normal character properties, like head, sword, and shield.

---
## showfile

`showfile filename;`

> introduced: 1.26rev2<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Opens the specified `filename`, using whatever program the file is associated with.

---
## showimg

`showimg index,filename,x,y;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Shows an image on the screen at the specified position.

`index` ranges between `[0..199]` cause the showimg to appear for other players,
while ranges `[200...]` and above are only visible to the player who issues the command.

See also:
- [changeimgcolors](#changeimgcolors)
- [changeimgmode](#changeimgmode)
- [changeimgpart](#changeimgpart)
- [changeimgvis](#changeimgvis)
- [changeimgzoom](#changeimgzoom)

```
this.px = playerx + 1.5 + vecx(playerdir) * 2;
this.py = playery + 2.0 + vecy(playerdir) * 2;
showimg 1, wbowi2.png, this.px, this.py;
```

##### Text Mode

Text can also be showing instead of an image file by using one of the following formats for `filename`:
- `@text`
- `@font@style@text`

The `style` is described in the [showtext](#showtext) command, which is a simpler way of creating a text mode `showani`.

```
showimg 2, @Charging..., playerx, playery;
```

##### Gani Mode

Ganis can be shown instead of an image file by using the `filename` format of:
- `&direction,animation`

`direction` is a value from `0` to `3`, while `animation` is a gani.

The [showani](#showani) command is a simpler way of creating a gani mode `showani`.

```
setstring anim,&2,walk;
showimg 3, #s(anim), playerx, playery;
```

##### Polygon mode

Polygons can be shown instead of an image file by using the `filename` format of:
- 2D: `#2,point-x,point-y,...`
- 3D: `#3,point-x,point-y,point-z,...`

> NOTE: 3D mode doesn't seem to show the polygon correctly to other players, at least in the 2.x clients.

`point-x`, `point-y`, and `point-z` values are the numerical points of the polygon and are repeated until finished.

The [showpoly](#showpoly) command is a simpler way of creating a polygon mode `showani`.

```
setstring points,##2;
addstring points,10;
addstring points,15;
addstring points,20;
addstring points,25;
showimg 5, #s(points), 0, 0;
```

---
## showimg2

`showimg2 index,filename,x,y,z;`

> introduced: 2.15<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Shows an image on the screen at the specified position.

`index` ranges between `[0..199]` cause the showimg to appear for other players,
while ranges `[200...]` and above are only visible to the player who issues the command.

See: [showimg](#showimg).

---
## showlocal

`showlocal;`

> introduced: 1.34<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Shows a hidden NPC, but only for the local player.

---
## showpoly

`showpoly index,{ x1,y1,...,xn,yn };`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Draws a polygon on the screen.  At least two sets of coordinates are required, which will draw a line.
With three or more sets of coordinates, the polygon will automatically be closed with a line drawn from the last coordinate to the first.

`index` ranges between `[0..199]` cause the showimg to appear for other players,
while ranges `[200...]` and above are only visible to the player who issues the command.

The [changeimgcolors](#changeimgcolors) command cannot change the alpha value of the polygon.

See: [showimg](#showimg).

---
## showpoly2

`showpoly2 index,{ x1,y1,z1,...,xn,yn,zn };`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Draws a polygon on the screen.  At least two sets of coordinates are required, which will draw a line.
With three or more sets of coordinates, the polygon will automatically be closed with a line drawn from the last coordinate to the first.

> NOTE: This command doesn't seem to show the polygon correctly to other players, at least in the 2.x clients.

`index` ranges between `[0..199]` cause the showimg to appear for other players,
while ranges `[200...]` and above are only visible to the player who issues the command.

The [changeimgcolors](#changeimgcolors) command cannot change the alpha value of the polygon.

See: [showimg](#showimg).

---
## showstats

`showstats bitflag;`

> introduced: 1.41<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Selects which parts of the game window are visible.
`bitflag` is a sum of values that enable the drawing of that specified element.

| Integer | Element |
| ------- | ------- |
|    1 | ASD |
|    2 | Icons for gralats, bombs, and arrows |
|    4 | Gralat count |
|    8 | Bombs count |
|   16 | Arrows count |
|   32 | Hearts |
|   64 | Alignment (AP) bar |
|  128 | Magic points (MP) bar |
|  256 | Minimap (player can still disable with ALT+3) |
|  512 | Inventory npcs |
| 1024 | Players |

The variable [allstats](scripting-gs1-variables.md#game-client) is a sum of all the values.

```
// Hide the hearts.
showstats allstats - 32;
```

---
## showtext

`showtext index,x,y,font,style,text;`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Shows text in a [showimg](#showimg) image.

`index` ranges between `[0..199]` cause the showimg to appear for other players,
while ranges `[200...]` and above are only visible to the player who issues the command.

`style` is a combination of letters that enable different styles.

| Style | Letter |
| ----- | ------ |
| Bold          | b |
| Italic        | i |
| Right-aligned | r |
| Underscore    | u |
| Strikeout     | s |
| Centered      | c |

Text renders 24 pixels tall, by default.

The [changeimgcolors](#changeimgcolors) command cannot change the alpha value of the polygon.

```
// Render text bold and underscored.
showtext 1,x,y,Arial,bu,Hello!;
```

See: [showimg](#showimg).

---
## showtext2

`showtext2 index,x,y,z,font,style,text;`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Shows text in a [showimg](#showimg) image.

See: [showtext](#showtext).

---
## sleep

`sleep seconds;`

> introduced: 1.22<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Suspends execution of the script for the set number of `seconds`.
If an NPC with a sleeping script receives an event and sets a `timeout`, then the sleep will be cancelled as the `sleep` command makes use of the `timeout` system.

---
## spyfire

`spyfire length,power;`

> introduced: sometime in 2.0x<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

The player shoots a line of explosions in the direction they are facing.
`length` is the number of segments of the explosion (each segment is a 2x2 tile rectangle).
`power` is the explosion power (`1` = normal, `3` = joltbomb explosion).

---
## stopmidi

`stopmidi;`

> introduced: beta 5<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Stops the currently playing MIDI file.

See: [play](#play).

---
## stopsound

`stopsound filename;`

> introduced: 1.37<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Stops playing the specified looped WAV file.

See: [playlooped](#playlooped).

---
## take

`take itemname;`

> introduced: beta 2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

The NPC takes a nearby item matching the `itemname` type.
The grab distance is 10 tiles.

See: [Item names](scripting-gs1-variables.md#item-names).

---
## take2

`take2 index;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

The NPC takes the specified item in the level.

The `index` is the item's position in the [items\[\]](scripting-gs1-variables.md#items) array.

---
## takehorse

`takehorse index;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

The NPC takes the specified horse in the level.  The NPC will mount the horse.

The `index` is the horse's position in the [horses\[\]](scripting-gs1-variables.md#horses) array.

---
## takeplayercarry

`takeplayercarry;`

> introduced: beta 4<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Takes away the item the player is currently carrying.

---
## takeplayerhorse

`takeplayerhorse;`

> introduced: 1.36<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Takes away the player's horse.

---
## throwcarry

`throwcarry;`

> introduced: 1.37<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

The NPC throws away their carried object.

---
## timereverywhere

`timereverywhere;`

> introduced: 1.22<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Allows all players in the level to process the NPC `timeout` and recieve the `timeout` event.

In classic mode, by default, only the level leader (see: [isleader](scripting-gs1-flags.md#isleader)) processes timeouts.

---
## timershow

`timershow;`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Shows the value of `timeout` above the NPC's head.

---
## toinventory

`toinventory flag;`

> introduced: beta 5<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Sets the `flag` and places the NPC inside a special inventory section on the left side of the screen.
The NPC will stay there until the `flag` is [unset](#unset).

If a player leaves the server and comes back. the NPC will NOT be in the inventory slot.
The `flag` will also still be set.

---
## tokenize

`tokenize text;`

> introduced: 2.02<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Splits `text` into tokens, splitting on whitespace and commas.
Text wrapped inside quotation marks are counted as a single token.

The [tokenscount](scripting-gs1-variables.md#tokenize) variable will contain the number of tokens,
and the [#t(index)](scripting-gs1-messagecodes.md#t) message codes will contain the token values.

```
if (playerchats && startswith(/summon,#c)) {
  tokenize #c;
  this.loc = {playerx, playery};
  with (getplayer(#t(1)) {
    setlevel2 #F,thiso.loc[0],thiso.loc[1];
  }
}
```

---
## tokenize2

`tokenize2 delims,text;`

> introduced: 2.02<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Splits `text` into tokens, splitting on whitespace, commas, and the delimiters specified in `delims`.
Text wrapped inside quotation marks are counted as a single token.

The [tokenscount](scripting-gs1-variables.md#tokenize) variable will contain the number of tokens,
and the [#t(index)](scripting-gs1-messagecodes.md#t) message codes will contain the token values.

---
## toweapons

`toweapons name;`

> introduced: 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Adds the NPC to the player's weapons.
The script of the NPC will replace whatever script is currently registered for that weapon on the server.
As such, it is recommended to use [addweapon](#addweapon) on the serverside.

---
## triggeraction

`triggeraction x,y,event,params;`

> introduced: 2.03<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Sends an action event at the specified tile position.
`params` is a CSV formatted string of additional data to package with the event.

Objects hit by the triggeraction will have their script processed with the event flag set.
The event flag prefixes the chosen `event` with the word `action`.
For example, if issuing an `attack` event, the script will have the `actionattack` flag set.

There are also special actions processed by the NPC-Server.

| Action | Description |
| ------ | ----------- |
| serverside | The first `params` is the name of a weapon.  The serverside script of that weapon will get an `actionserverside` event. |
| servernpc  | The frist `params` is the name of a database NPC.  That NPC will get an `actionserverside` event. |
| server...  | Any other action that starts with `server` will get processed by all the Control-NPCs.  Sending a `serverstuff` event will trigger `actionserverstuff` on the Control-NPCs. |
| clientside | When sent from the NPC-Server to a player, it will trigger on their clientside weapon scripts.  They will receive an `actionclientside` event. |

See:
- [Triggeraction events](scripting-gs1-events.md#triggeraction-events)
- [Triggeraction variables](scripting-gs1-variables.md#triggeraction)
- [Communicating with a serverside script](npcserver.md#communicating-with-a-serverside-script)

---
## unfreezeplayer

`unfreezeplayer;`

> introduced: (npcserver)<br>
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Unfreezes a player frozen with [freezeplayer2](#freezeplayer2).

---
## unset

`unset flag;`

> introduced: beta 2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Unsets a flag.

See: [set](#set).

---
## unseteffect

`unseteffect;`

> introduced: around 1.20<br>
removed: 1.33rev1<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Removes the [seteffect](#seteffect).

Note that this is related to the older `seteffect` command, with three arguments.

---
## updateboard

`updateboard x,y,width,height;`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Applies the result of a script changing the level tiles.

When a script changes the level's tiles, those changes will not take affect until `updateboard` or `updateboard2` are called.

The board changes will be reverted after some time has passed.

---
## updateboard2

`updateboard2 x,y,width,height;`

> introduced: 2.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Applies the result of a script changing the level tiles, permanently.  The tiles will not revert back after time has passed.

When a script changes the level's tiles, those changes will not take affect until `updateboard` or `updateboard2` are called.

---
## updateterrain

`updateterrain;`

> introduced: possibly 2.17, revealed (???)<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Applies the result of changes to the 3D terrain's geometry.
Must be used after [setz](#setz) is called, or after changes to the [groundheights](scripting-gs1-variables.md#levels) or [waterheight](scripting-gs1-variables.md#levels) variables.

---
## warpto

`warpto levelname,x,y;`

> introduced: (npcserver)
scope: 💻 serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Warps the NPC to the specified level and tile position.

---
## wraptext

`wraptext linelength,delimiters,text;`

> introduced: 2.12<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Word wraps the `text` using the `delimiters` to try to stay within a budget of `linelength` characters per line.

The resulting text is placed in the [#t(index)](scripting-gs1-messagecodes.md#t) message codes,
and the number of lines can be read with the [tokenscount](scripting-gs1-variables.md#tokenize) variable.

There are issues when `text` contains `@` characters.
If there are any `@` characters in your text, you must prefix your text with `@@@`.
This is because `wraptext` is using some of the same code paths as [showimg](#showimg) text strings,
which embed the font and style like so: `@font@style@text`.
As such, it will try to strip out the font and style from the text, and mess up your string.

---
## wraptext2

`wraptext2 pixelwidth,zoomfactor,delimiters,text;`

> introduced: 2.12<br>
scope: 🧑 clientside<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Divides `text` into segments that try to be `pixelwidth` pixels wide.

The `text` can contain [showimg](#showimg) style text strings.

For example,
```
wraptext2 100,1, ,@TimesRoman@b@Some bold text;
```
