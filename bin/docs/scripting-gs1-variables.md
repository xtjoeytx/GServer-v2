# GS1 Variables

## Variables

The `scope` can be one or both of clientside 🧑 and serverside 💻.

#### Triggeraction

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| actionplayer | 2.03 | 🧑💻 | The player who initiated the triggeraction event. |
| paramscount | 2.03 | 🧑💻 | The number of parameters passed along with the triggeraction event. |

#### Other

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| tokenscount | 2.02 | 🧑💻 | The number of `#t()` message code tokens after a call to `tokenize` or `tokenize2`. |

#### Time

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| timevar   | 2.10 | 🧑💻 | A synchronized time variable that is incremented once every 5 seconds. |
| timevar2  | 2.30 | 🧑💻 | Clientside, it is the time since the start of the machine (in milliseconds, until client 4.20rev6. Serverside, it is the Unix timestamp. |
| nwtime    | (npcserver) | 💻 | Minute in the day [0..1439] |
| nwmin     | (npcserver) | 💻 | Minute of the hour [0..59] |
| nwhour    | (npcserver) | 💻 | Hour of the day [0..23] |
| nwday     | (npcserver) | 💻 | Day of the month [1..28] |
| nwweekday | (npcserver) | 💻 | Day of the week [1..7] |
| nwweek    | (npcserver) | 💻 | Week of the year [1..40] |
| nwmonth   | (npcserver) | 💻 | Month of the year [1..10] |
| nwyear    | (npcserver) | 💻 | Year [1000+] |

#### Game Client

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| focusx          | possibly 2.12, revealed 2.16 | 🧑 | The current tile X focus position after the `setfocus` command was used. |
| focusy          | possibly 2.12, revealed 2.16 | 🧑 | The current tile Y focus position after the `setfocus` command was used. |
| mousebuttons    | 2.14 | 🧑 | Sum which identifies the mouse buttons being pressed. Left mouse = 1, middle mouse = 2, right mouse = 4. |
| mousescreenx    | 2.14 | 🧑 | Pixel X position of the mouse on the game screen. |
| mousescreeny    | 2.14 | 🧑 | Pixel Y position of the mouse on the game screen. |
| mousewheeldelta | 2.14 | 🧑 | The movement of the mouse wheel since the last frame. |
| mousex          | 2.14 | 🧑 | Tile X position of the mouse in the level. |
| mousey          | 2.14 | 🧑 | Tile Y position of the mouse in the level. |
| screenheight    | 1.41 | 🧑 | The vertical height of the game client window. |
| screenwidth     | 1.41 | 🧑 | The horizontal width of the game client window. |
| downloadpos     | 2.14 | 🧑 | How many bytes have been transferred for the current file download. |
| downloadsize    | 2.22 | 🧑 | How many bytes in total will be transferred for the current file download. |
| graalversion    | 2.10 | 🧑 | The current version of the game client. |
| musicpos        | 2.16 | 🧑 | The current playing position in the music track. |
| musiclen        | 2.16 | 🧑 | Length of the current music track. |
| selectedsword   | (newworld) | 🧑 | The index of the currently selected sword. |
| selectedweapon  | 2.04 | 🧑 | The index of the currently selected weapon. |
| weaponscount    | 2.04 | 🧑💻 | How many weapons the player holds. |
| playerfreezetime | possibly around 1.20 | 🧑 | How many seconds until the player stops being frozen after a call to `freezeplayer`. |

#### Levels

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| levelorgx       | 2.04        | 🧑💻 | When a player is attached to an NPC, this is the player's Y offset from level position (0, 0).  When attached, the player's tile position is relative to the NPC. |
| levelorgy       | 2.04        | 🧑💻 | When a player is attached to an NPC, this is the player's Y offset from level position (0, 0).  When attached, the player's tile position is relative to the NPC. |
| board[]         | around 1.20 | 🧑💻 | [Read only] The tile at the specific level board index.  The level board consists of 64x64 tiles. |
| tiles[x,y]      | 2.10        | 🧑💻 | [RW] The tile at the specific tile position.  Positions outside of [0..63] will affect adjacent levels when on a bigmap. |
| gravity         | 2.22        | 🧑💻 | How fast items fall to the ground (defaults to a velocity of 2.0 tiles per second). |
| groundheights[] | possibly 2.12, revealed ??? | 🧑💻 | The height at which 3D terrain will transition to different tiles. |
| waterheight     | possibly 2.12, revealed ??? | 🧑 | The height that water is drawn at on 3D terrain (global value). |

The `groundheights` array consists of 7 values.
The client starts with index 0 and draws tiles until it reaches the set Z level.
It then switches to the next index and draws the next set of tiles, continuing until it reaches the end of the array.
At the end of the array, all further tiles will be drawn with the final tile page (index 7, not in the array).
If an index's Z level is identical or less than the previous index's Z level, that tile page will be skipped.

The `picso.png` tileset is paged from top to bottom, left to right.
```
0 2 4 6
|/|/|/|
1 3 5 7
```

| Index | Z | Tiles |
| ----- | - | ----- |
| 0 |  0 | Water |
| 1 |  3 | Beach |
| 2 |  4 | Beach to Grass - Transition 1 |
| 3 |  5 | Beach to Grass - Transition 2 |
| 4 | 25 | Grass 1 |
| 5 | 55 | Grass 2 |
| 6 | 65 | Mountains |
| - |  - | Ice |

#### Players

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| allplayerscount | (npcserver) | 💻 | The number of players in the `allplayers[]` array. |
| allplayers[]    | (npcserver) | 💻 | Array of objects for all the players in the server.  Can only be used with the `with()` statement. |
| playerscount  | around 1.20 | 🧑💻 | The number of players in the `players[]` array. |
| players[]     | around 1.20 | 💻 | Array of objects for all the players in the level.  Can only be used with the `with()` statement.  In older clients, this included `showcharacter` NPCs. |
| players[].anistep     | possibly 2.12, revealed 2.14 | 🧑 | The frame of the current gani animation. |
| players[].ap          | 1.30 | 🧑💻 | Alignment points of the player. |
| players[].attachid    | 2.04 | 🧑💻 | The ID of the NPC the player is attached to. |
| players[].attachtype  | 2.04 | 🧑💻 | The type of object the player is attached to (0 = NPC, the only supported type). |
| players[].bombs       | beta 5 | 🧑💻 | The number of bombs the player has. |
| players[].bombpower   | beta 5 | 🧑💻 | The power of the player's bomb (classic generation only). |
| players[].carrysprite | [GR] | 💻 | The sprite of the object the player is carrying. |
| players[].darts       | beta 5 | 🧑💻 | The number of arrows the player has. |
| players[].deaths      | possibly 1.39rev1 | 🧑💻 | How many times the player has died. |
| players[].dir         | beta 5 | 🧑💻 | The direction the player is facing.  See: [Directions](#directions) |
| players[].fullhearts  | beta 5 | 🧑💻 | The maximum life of the player. |
| players[].glovepower  | beta 5 | 🧑💻 | The player's glove power (2 = glove1, 3 = glove2). |
| players[].gralats     | sometime before 2.20 | 🧑💻 | How many gralats the player has. |
| players[].headset     | beta 5 | 🧑💻 | The number of the player's current head, when the head matches `head000.png`. |
| players[].hearts      | beta 5 | 🧑💻 | The player's current life total. |
| players[].hp          | possibly 2.12, revealed (npcserver) | 🧑💻 | The player's current life total. |
| players[].hurtdx      | 2.00 | 🧑💻 | The X direction of the player's hurt movement. |
| players[].hurtdy      | 2.00 | 🧑💻 | The Y direction of the player's hurt movement. |
| players[].hurtpower   | 2.01 | 🧑💻 | How much life the player lost when they last got hurt. |
| players[].id          | around 1.20 | 🧑💻 | The player's ID number (persistent only for the session). |
| players[].kills       | possibly 1.39rev1 | 🧑💻 | How many times the player has killed other players. |
| players[].lastdead    | (npcserver) | 🧑💻 | The last time the player died (timevar). |
| players[].logintime   | (npcserver) | 🧑💻 | The last time the player logged in (timevar). |
| players[].maxhp       | possibly 2.12, revealed (npcserver) | 🧑💻 | The maximum life of the player. |
| players[].mp          | 1.22 | 🧑💻 | The number of magic points the player has. |
| players[].rating      | ??? | 💻 | The player's spar rating. |
| players[].ratingd     | ??? | 💻 | The player's spar deviation. |
| players[].rupees      | beta 5 | 🧑💻 | The number of gralats the player has. |
| players[].saysnumber  | 1.21 | 🧑💻 | The current number the player is saying. |
| players[].shieldpower | beta 5 | 🧑💻 | The power of the player's shield. |
| players[].shootpower  | beta 5 | 🧑💻 | The power of the player's bow (classic generation only). |
| players[].sprite      | 1.25 | 🧑💻 | The player's current sprite.  See: [Character sprites](#character-sprites) |
| players[].swordpower  | beta 5 | 🧑💻 | The power of the player's bow. |
| players[].trial       | ??? | 🧑💻 | Identifies if the player is a trial account. |
| players[].x           | beta 5 | 🧑💻 | The X tile position of the player. |
| players[].y           | beta 5 | 🧑💻 | The Y tile position of the player. |
| players[].z           | possibly 2.12, revealed (npcserver) | 🧑💻 | The Z height of the player, from -50 to 170. |

> players[] shorthand: e.g., playerhearts

`playersaysnumber` supports mathematical operations.
Everything up to the first space in the player's chat message is processed through the scripting engine,
and the resulting number is displayed.

#### NPCs

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| npcscount      | 1.38 | 🧑💻 | The number of NPCs in the `npcs[]` array. |
| npcs[]         | 1.38 | 💻 | Array of objects for all the NPCs in the level.  Can only be used with the `with()` statement. |
| npcs[].anistep     | possibly 2.12, revealed 2.14 | 🧑 | The frame of the current gani animation. |
| npcs[].ap          | 1.30 | 🧑💻 | Alignment points of the NPC. |
| npcs[].bombs       | beta 5 | 🧑💻 | The number of bombs the NPC has. |
| npcs[].darts       | beta 5 | 🧑💻 | The number of arrows the NPC has. |
| npcs[].dir         | 1.25 | 🧑💻 | The direction the NPC is facing.  See: [Directions](#directions) |
| npcs[].glovepower  | around 1.20 | 🧑💻 | The NPC's glove power (1 = glove1, 2 = glove2). |
| npcs[].gralats     | sometime before 2.2 | 🧑💻 | How many gralats the NPC has. |
| npcs[].headset     | ??? | 🧑💻 | The number of the NPC's current head, when the head matches `head000.png`. |
| npcs[].hearts      | beta 5 | 🧑💻 | The NPC's current life total. |
| npcs[].height      | 1.38 | 🧑💻 | The height of the NPC, in tiles.  Serverside, this only works if `setshape` was called. |
| npcs[].hp          | possibly 2.12, revealed (npcserver) | 🧑💻 | The NPC's current life total. |
| npcs[].hurtdx      | 1.27 | 🧑💻 | The X direction of the NPC's hurt movement. |
| npcs[].hurtdy      | 1.27 | 🧑💻 | The Y direction of the NPC's hurt movement. |
| npcs[].hurtpower   | ??? | 💻 | How much life the NPC lost when they last got hurt. |
| npcs[].id          | 1.38 | 🧑💻 | The NPC's ID number (persistent only for database NPCs). |
| npcs[].rupees      | beta 5 | 🧑💻 | How many gralats the NPC has. |
| npcs[].save[]      | 1.27 | 🧑💻 | Array of 10 numbers [0..9] saved to the NPC.  Value is in the range of [0..220] |
| npcs[].shieldpower | around 1.20 | 🧑💻 | The power of the NPC's shield. |
| npcs[].sprite      | 1.25 | 🧑💻 | The NPC's current sprite.  See: [Character sprites](#character-sprites) |
| npcs[].swordpower  | around 1.20 | 🧑💻 | The power of the NPC's sword. |
| npcs[].timeout     | beta 5 | 🧑💻 | Time current `timeout` value of the NPC, in seconds. |
| npcs[].width       | 1.38 | 🧑💻 | The width of the NPC, in tiles.  Serverside, this only works if `setshape` was called. |
| npcs[].x           | beta 5 | 🧑💻 | The X tile position of the NPC. |
| npcs[].y           | beta 5 | 🧑💻 | The Y tile position of the NPC. |
| npcs[].z           | possibly 2.12, revealed (npcserver) | 🧑💻 | The Z height of the NPC, from -50 to 170. |

> npcs[] shorthand: e.g., hearts

#### Arrows

> gserver support: ✅ 4.0<br>
official support: ❌<br>

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| arrowscount   | 1.36 | 🧑💻 | The number of NPCs in the `arrows[]` array. |
| arrows[]      | 1.36 |  | Array of objects for all the arrows in the level. |
| arrows[].x    | 1.36 | 🧑💻 | The X tile position of the arrow. |
| arrows[].y    | 1.36 | 🧑💻 | The Y tile position of the arrow. |
| arrows[].dx   | 1.36 | 🧑💻 | The X velocity of the arrow. |
| arrows[].dy   | 1.36 | 🧑💻 | The Y velocity of the arrow. |
| arrows[].dir  | 1.36 | 🧑💻 | The direction the arrow is flying.  See: [Directions](#directions) |
| arrows[].type | 1.36 | 🧑💻 | The type of arrow. |
| arrows[].from | 1.36 | 🧑💻 | When shot by a player, this is `1`, otherwise `0`. |

| arrow[].type | Object |
| ------------ | ------ |
| 0 | Ball      |
| 1 | Arrow     |
| 2 | Fireball  |
| 3 | Fireblast |
| 4 | Nukeshot  |

#### Baddies

> gserver support: ✅ 4.0<br>
official support: ❌<br>

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| compuscount      | around 1.20 | 🧑💻 | The number of baddies in the `compus[]` array. |
| compus[]         | around 1.20 |  | Array of objects for all the baddies in the level. |
| compus[].x       | around 1.20 | 🧑💻 | The X tile position of the baddy. |
| compus[].y       | around 1.20 | 🧑💻 | The Y tile position of the baddy. |
| compus[].type    | around 1.20 | 🧑💻 | The type of the baddy. |
| compus[].dir     | around 1.20 | 🧑💻 | The direction the baddy's body is pointing.  See: [Directions](#directions) |
| compus[].headdir | around 1.20 | 🧑💻 | The direction the baddy's head is pointing.  See: [Directions](#directions) |
| compus[].power   | around 1.20 | 🧑💻 | The baddy's current life total. |
| compus[].mode    | around 1.20 | 🧑💻 | The baddy's current mode. |

| compus[].type | Baddy Type |
| ------------- | ---------- |
| 0 | graysoldier |
| 1 | bluesoldier |
| 2 | redsoldier |
| 3 | shootingsoldier |
| 4 | swampsoldier |
| 5 | frog |
| 6 | octopus / spider |
| 7 | goldenwarrior |
| 8 | lizardon |
| 9 | dragon |

| compus[].mode | Action |
| ------------- | ------ |
| 0 | walking |
| 1 | looking |
| 2 | hunting |
| 3 | hurted |
| 4 | bumped |
| 5 | dying |
| 6 | shooting (swampsoldier) |
| 7 | jumping (frog) |
| 8 | shooting (spider) |
| 9 | dead |

#### Bombs

> gserver support: ✅ 4.0<br>
official support: ❌<br>

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| bombscount    | 1.36 | 🧑💻 | The number of bombs in the `bombs[]` array. |
| bombs[]       | 1.36 |  | Array of objects for all the bombs in the level. |
| bombs[].x     | 1.36 | 🧑💻 | The tile X position of the bomb. |
| bombs[].y     | 1.36 | 🧑💻 | The tile Y position of the bomb. |
| bombs[].power | 1.36 | 🧑💻 | The bomb's power. |
| bombs[].time  | 1.36 | 🧑💻 | The time left until the bomb explodes. |

#### Items

> gserver support: ✅ 4.0<br>
official support: ❌<br>

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| itemscount   | 1.36 | 🧑💻 | The number of items in the `items[]` array. |
| items[]      | 1.36 |  | Array of objects for all the items in the level. |
| items[].x    | 1.36 | 🧑💻 | The tile X position of the item. |
| items[].y    | 1.36 | 🧑💻 | The tile Y position of the item. |
| items[].type | 1.36 | 🧑💻 | The item type.  See: [Item names](#item-names) |
| items[].time | 1.36 | 🧑💻 | The time until the item disappears. |

#### Explosions

> gserver support: ✅ 4.0<br>
official support: ❌<br>

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| exploscount    | 1.36 | 🧑💻 | The number of explosions in the `explos[]` array. |
| explos[]       | 1.36 |  | Array of objects for all the explosions in the level. |
| explos[].x     | 1.36 | 🧑💻 | The tile X position of the explosion. |
| explos[].y     | 1.36 | 🧑💻 | The tile Y position of the explosion. |
| explos[].power | 1.36 | 🧑💻 | The power of the explosion. |
| explos[].time  | 1.36 | 🧑💻 | The time until the explosion disappears. |
| explos[].dir   | 1.36 | 🧑💻 | The direction the explosion is facing.  See: [Directions](#directions) |

#### Horses

> gserver support: ✅ 4.0<br>
official support: ❌<br>

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| horsescount        | 1.36 | 🧑💻 | The number of horses in the `horses[]` array. |
| horses[]           | 1.36 | 💻 | Array of objects for all the horses in the level. |
| horses[].x         | 1.36 | 🧑💻 | The tile X position of the horse. |
| horses[].y         | 1.36 | 🧑💻 | The tile Y position of the horse. |
| horses[].dir       | 1.36 | 🧑💻 | The direction the horse is facing.  See: [Directions](#directions) |
| horses[].bushes    | 1.36 | 🧑💻 | The number of bushes the horse has eaten (speed). |
| horses[].bombs     | 1.36 | 🧑 | The number of bombs the horse has eaten.  Serverside, it is always 0, as the client never sends this data. |
| horses[].bombpower | 1.36 | 🧑 | The power of the bombs the horse has eaten.  Serverside, it is always 0, as the client never sends this data. |
| horses[].type      | 1.36 | 🧑💻 | The type of horse. `0` is a normal horse, `1` is a boat (a horse placed on water). |

#### Signs

| Variable | Introduced | Scope | Description |
| -------- | ---------- | ----- | ----------- |
| signscount | 1.39rev2 |  | 🧑💻 | The number of signs in the `signs[]` array. |
| signs[]    | 1.39rev2 |  | 💻 | Array of objects for all the signs in the level. |
| signs[].x  | 1.39rev2 |  | 🧑💻 | The tile X position of the sign. |
| signs[].y  | 1.39rev2 |  | 🧑💻 | The tile Y position of the sign. |

## Item names

| Item | Index | Introduced |
| ---- | ----- | ---------- |
| greenrupee | 0 | Beta 2 |
| bluerupee | 1 | Beta 2 |
| redrupee | 2 | Beta 2 |
| bombs | 3 | Beta 2 |
| darts | 4 | Beta 2 |
| heart | 5 | Beta 2 |
| glove1 | 6 | Beta 2 |
| bow | 7 | Beta 2 |
| bomb | 8 | Beta 2 |
| shield | 9 | Beta 3 |
| sword | 10 | Beta 3 |
| fullheart | 11 | Beta 3 |
| superbomb | 12 | Beta 4 |
| battleaxe | 13 | Beta 4 |
| goldensword | 14 | Beta 4 |
| mirrorshield | 15 | Beta 4 |
| glove2 | 16 | Beta 4 |
| lizardshield | 17 | Beta 5 |
| lizardsword | 18 | Beta 5 |
| goldrupee | 19 | possibly Beta 9, revealed 1.32 |
| fireball | 20 | Beta 9 |
| fireblast | 21 | Beta 9 |
| nukeshot | 22 | Beta 9 |
| joltbomb | 23 | Beta 9 |
| spinattack | 24 | 1.32 |

## Colors

| Color | Introduced | Integer |
| ----- | ---------- | ------- |
| white | Beta 3 | 0 |
| yellow | Beta 3 | 1 |
| orange | Beta 3 | 2 |
| pink | Beta 3 | 3 |
| red | Beta 3 | 4 |
| darkred | Beta 3 | 5 |
| lightgreen | Beta 3 | 6 |
| green | Beta 3 | 7 |
| darkgreen | Beta 3 | 8 |
| lightblue | Beta 3 | 9 |
| blue | Beta 3 | 10 |
| darkblue | Beta 3 | 11 |
| brown | Beta 3 | 12 |
| cynober | Beta 3 | 13 |
| purple | Beta 3 | 14 |
| darkpurple | Beta 3 | 15 |
| lightgray | Beta 3 | 16 |
| gray | Beta 3 | 17 |
| black | Beta 3 | 18 |
| transparent | 1.38 | 19 |

## Directions

| Direction | Introduced | Integer |
| --------- | ---------- | -------
| up | Beta 3 | 0 |
| left | Beta 3 | 1 |
| down | Beta 3 | 2 |
| right | Beta 3 | 3 |

## Baddy names

| Baddy | Introduced |
| ----- | ---------- |
| graysolder | Beta 4 |
| bluesoldier | Beta 4 |
| redsoldier | Beta 4 |
| shootingsoldier | Beta 4 |
| swampsoldier | Beta 4 |
| frog | Beta 4 |
| octopus / spider | Beta 4 |
| goldenwarrior | Beta 5 |
| lizardon | Beta 5 |
| dragon | Beta 5 |

## Variable prefixes

| Prefix | Introduced |
| ------ | ---------- |
| this.var | 1.1 |
| this.flag | 2.19 |
| level. | (npcserver) |
| local.flag | 2.02 |
| client.flag | 2.19 |
| clientr. | (npcserver) |
| server. | ??? |
| serverr. | (npcserver) |

## Character sprites

| Action | Sprite |
| ------ | ------ |
| no movement | 0 |
| walking | 1-8 |
| sword slaying | 9-13 |
| pushing | 14-18 |
| pulling | 19-22 |
| lifting | 23 |
| no movement, carrying something | 24 |
| walking, carrying something | 25-32 |
| shooting | 33 |
| riding | 34-36 |
| sitting | 37 |
| sleeping | 38 |
| hurted | 39 |
| dead | 40 |
