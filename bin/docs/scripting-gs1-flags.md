# GS1 Flags

Flags are boolean values that are set by the game engine before every script run.

## canspin

> introduced: ? (beta 7 or 1.32)<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player has the spinattack.

## carriesblackstone

> introduced: beta 4<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is carrying a blackstone object.

## carriesbush

> introduced: beta 4<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is carrying a bush object.

## carriesnpc

> introduced: around 1.20<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is carrying an NPC.

## carriessign

> introduced: beta 4<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is carrying a sign object.

## carriesstone

> introduced: beta 4<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is carrying a stone object.

## carriesvase

> introduced: beta 4<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is carrying a vase object.

## carrying

> introduced: around 1.20<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is carrying anything.

## compsdead

> introduced: beta 7<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when all the baddies are dead in the level.

## followsplayer

> introduced: beta 7<br>
removed: -<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>

True when the NPC is following a player.

## gotaxe

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's swordpower is 2.

## gotbomb

> introduced: beta 2<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's bombpower is 1.

## gotbombs `<integer>`

> introduced: beta 2<br>
removed: around 1.20<br>
gserver support: ❌ (won't implement)<br>
official support: ❌<br>

True when the NPC has taken `>= integer` bombs.

## gotbow

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's bowpower is 1.

## gotdarts `<integer>`

> introduced: beta 2<br>
removed: around 1.20<br>
gserver support: ❌ (won't implement)<br>
official support: ❌<br>

True when the NPC has taken `>= integer` arrows.

## gotglove1

> introduced: beta 2<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's glovepower is 2.

## gotglove2

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's glovepower is 3.

## gotgoldensword

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's swordpower is 4.

## gotlizardshield

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's shieldpower is 3.

## gotlizardsword

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's swordpower is 3.

## gotmirrorshield

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's shieldpower is 2.

## gotrupees `<integer>`

> introduced: beta 2<br>
removed: around 1.20<br>
gserver support: ❌ (won't implement)<br>
official support: ❌<br>

True when the NPC has taken `>= integer` gralats.

## gotshield

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's shieldpower is 1.

## gotsuperbomb

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's bombpower is 2.

## gotsword

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC's swordpower is 1.

## isfocused

> introduced: 2.16rev5<br>
removed: -<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>

True when the `setfocus` command has changed where the camera is looking.

## isinguild `<guild>`

> introduced: beta 5<br>
removed: beta 7<br>
gserver support: ❌ (won't implement)<br>
official support: ❌<br>

True when the NPC is in the specified guild.

## isleader

> introduced: 1.22<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is the level leader.
The level leader is the player who has been inside the level the longest.
They are in charge of processing baddies and can trigger `timeout` events on NPCs that didn't issue the `timereverywhere` command.

## isonmap

> introduced: 1.38<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is on a map.
It is currently unknown if it should be `true` if the player is on a gmap or not.

## issparringzone

> introduced: 2.02<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is in a sparring zone level.

## isweapon

> introduced: 1.34<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True if the current script is executing on a weapon.

## leftmousebutton

> introduced: 2.14<br>
removed: -<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>

True when the left mouse button is pressed.

## lighteffectsenabled

> introduced: 2.03<br>
removed: -<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>

True when the client has enabled light effects.

## middlemousebutton

> introduced: 2.14<br>
removed: -<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>

True when the middle mouse button is pressed.

## nopkzone

> introduced: 2.02<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is in a `noplayerkilling` zone.

## peltwithblackstone

> introduced: beta 5<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC has been hit by a thrown blackstone object.

## peltwithbush

> introduced: beta 5<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC has been hit by a thrown bush object.

## peltwithnpc

> introduced: around 1.20<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC has been hit by another thrown NPC.

## peltwithperson

> introduced: beta 7<br>
removed: around 1.20<br>
gserver support: ❌<br>
official support: ❌<br>

True when the NPC has been hit by a thrown player.
This might have been the same as `peltwithnpc`; much older versions of the game would sometimes refer to NPCs as a "person".

## peltwithplayer
> introduced: -<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC has been hit by a thrown player.

## peltwithsign

> introduced: beta 5<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC has been hit by a thrown sign object.

## peltwithstone

> introduced: beta 5<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC has been hit by a thrown stone object.

## peltwithvase

> introduced: beta 5<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC has been hit by a thrown vase object.

## playerattached

> introduced: 2.04<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is attached to an NPC.

## playeringuild `<guild>`

> introduced: beta 7<br>
removed: around 1.25<br>
gserver support: ❌ (won't implement)<br>
official support: ❌<br>

True when the player is in the specified guild.

## playerisfemale

> introduced: 1.36<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is "female".

## playerismale

> introduced: 1.36<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is "male".

## playerkiller

> introduced: beta 5<br>
removed: 1.32<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the player has killed another player.

## playermap

> introduced: 2.14rev7<br>
removed: -<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>

True when the player is looking at the map.

## playeronhorse

> introduced: around 1.20<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is riding a horse.

## playeronline

> introduced: -<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ✅<br>

True when the player is playing online.

## playerpaused

> introduced: 2.14rev7<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is currently paused.

## playerreading

> introduced: 2.03<br>
removed: -<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>

True when the player is reading a sign.

## playerswimming

> introduced: around 1.32<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player is swimming in water.

## rightmousebutton

> introduced: 2.14<br>
removed: -<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>

True when the right mouse button is pressed.

## shotbybaddy

> introduced: beta 7<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the NPC has been hit by an arrow shot by a baddie.

## shotbynpc

> introduced: -<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ❌<br>

True when the NPC has been hit by an arrow shot by another NPC.

## shotbyplayer

> introduced: beta 7<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the NPC has been hit by an arrow shot by a player.

## visible

> introduced: beta 5<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ✅<br>

True when the NPC is visible.
Being outside of camera range does not make this flag false.
It instead refers to an NPC that has been made invisible or not by a command like `hide`.

## weaponsenabled

> introduced: beta 5<br>
removed: -<br>
gserver support: ✅ 4.0<br>
official support: ???<br>

True when the player's weapons are enabled.
A player's weapon can be disabled with the `disableweapons` command.
