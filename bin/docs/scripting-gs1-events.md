# GS1 Events

Anything that is marked as executing on an NPC will also execute on clientside weapons, which are technically NPCs on the client.

This does NOT include serverside weapon scripts, which only respond to "serverside" action events that explicitly target the weapon.

Events with an empty "introduced" field have no known release date since they were serverside-only events that were never documented in release notes.

## compusdied

> introduced: beta 2<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when all of the baddies in the level have died.

## created

> introduced: 1.27<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 🌐 npc, control-npc<br>

Triggered when NPC is created or its script has been changed.  It will only be fired once for an NPC.

## exploded

> introduced: beta 5<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggered when an NPC is touched by an explosion.

## firedonhorse

> introduced: 1.34r1<br>
gserver support: ❌ (clientside)<br>
scope: 🧑 client<br>
executed on: ⚔️ weapon npc<br>

Triggered on a weapon NPC when the player fires the weapon while on a horse.

## gotaxe

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the `axe` item.

## gotbomb

> introduced: beta 2<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the "bomb" weapon.

## gotbombs

> introduced: beta 2<br>
removed: around 1.20<br>
special parameters: gotbombs [integer]<br>
gserver support: ❌ (never, too unusual)<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets bombs greater than or equal to the amount specified.

## gotbow

> introduced: beta 2<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the "bow" weapon.

## gotdarts

> introduced: beta 2<br>
removed: around 1.20<br>
special parameters: gotdarts [integer]<br>
gserver support: ❌ (never, too unusual)<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets arrows greater than or equal to the amount specified.

## gotglove1

> introduced: beta 2<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the "glove1" item.

## gotglove2

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the `glove2` item.

## gotgoldensword

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the `goldensword` item.

## gotlizardshield

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the `lizardshield` item.

## gotlizardsword

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the `lizardsword` item.

## gotmirrorshield

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the `mirrorshield` item.

## gotrupees

> introduced: beta 2<br>
removed: around 1.2<br>
special parameters: gotrupees [integer]<br>
gserver support: ❌ (never, too unusual)<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets gralats greater than or equal to the amount specified.

## gotshield

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the `shield` item.

## gotsuperbomb

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the `superbomb` weapon.

## gotsword

> introduced: beta 5<br>
removed: 1.38<br>
gserver support: ❌<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when the player gets the `sword` item.

## initialized

> introduced: -<br>
gserver support: ✅ 4.0<br>
scope: 💻 server<br>
executed on: 🤖 🌐 npc, control-npc<br>

Triggers when a serverside NPC is loaded from the disk on server start.

## itemdrop

> introduced: -<br>
gserver support: ✅ 4.0<br>
scope: 💻 server<br>
executed on: 🌐 control-npc<br>

Triggers when an item is dropped on the ground, assuming the `itemdropevents` option is enabled in `serveroptions.txt`.

## keypressed

> introduced: 2.14<br>
gserver support: ❌ (clientside)<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggered on an NPC when a key is pressed.

Related message codes:
- `#p(0)` - keycode
- `#p(1)` - character

## mousedown

> introduced: 2.14<br>
gserver support: ❌ (clientside)<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggered on an NPC when the player presses the mouse button down.

Related flags:
- `leftmousebutton`
- `middlemousebutton`
- `rightmousebutton`

Related variables:
- `mousex`
- `mousey`
- `mousebuttons`
- `mousescreenx`
- `mousescreeny`

## mouseup

> introduced: 2.14<br>
gserver support: ❌ (clientside)<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggered on an NPC when the player releases the mouse button.

Related flags:
- `leftmousebutton`
- `middlemousebutton`
- `rightmousebutton`

Related variables:
- `mousex`
- `mousey`
- `mousebuttons`
- `mousescreenx`
- `mousescreeny`

## mousewheel

> introduced: 2.14<br>
gserver support: ❌ (clientside)<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggered on an NPC when the player scrolls the mouse wheel.

Related flags: `mousewheeldelta`

## movementfinished

> introduced: 2.03<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers after all movement commands have finished executing, assuming the 'inform when done' option was set.

## npcwarped

> introduced: -<br>
gserver support: ✅ 4.0<br>
scope: 💻 server<br>
executed on: 🤖 npc<br>

Triggers when an NPC changes level.

## playerchats

> introduced: 1.21<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when a player's chat text changes.

## playerdies

> introduced: 2.01<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when a player dies.

## playerendsreading

> introduced: beta 3<br>
gserver support: ❌ (clientside)<br>
scope: 🧑 client<br>
executed on: 🤖 npc<br>

Triggers when a player finishes reading a sign.

## playerenters

> introduced: beta 2<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when a player enters the level.

## playerhurt

> introduced: 2.01<br>
alternative: playerhurted (GR serverside supported)<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when a player loses health.

## playerlaysitem

> introduced: beta 2<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when a player drops an item on the ground.

## playerleaves

> introduced: -<br>
gserver support: ✅ 4.0<br>
scope: 💻 server<br>
executed on: 🤖 npc<br>

Triggers when a player leaves the level.

## playerlogin

> introduced: -<br>
gserver support: ✅ 4.0<br>
scope: 💻 server<br>
executed on: 🌐 control-npc<br>

Triggers when a player logs into the server.

## playerlogout

> introduced: -<br>
gserver support: ✅ 4.0<br>
scope: 💻 server<br>
executed on: 🌐 control-npc<br>

Triggers when a player logs out of the server.

## playertouchsme

> introduced: beta 2<br>
alternative: playertouchesme<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client and server<br>
executed on: 🤖 npc<br>

Triggers on an NPC when a player touches it.

## playertouchsother

> introduced: beta 2<br>
alternative: playertouchesother<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client and server<br>
executed on: 🤖 npc<br>

Triggers on an NPC when a player touches some other NPC.

## pm

> introduced: -<br>
gserver support: ✅ 4.0<br>
scope: 💻 server<br>
executed on: 🌐 control-npc<br>

Triggers when a player sends a private message to the NPC-Server player.

## rcchat

> introduced: -<br>
gserver support: ✅ 4.0<br>
scope: 💻 server<br>
executed on: 🌐 control-npc<br>

Triggers whenever an RC player sends an RC chat message starting with `/npc`.

## serverlistconnect

> introduced: -<br>
gserver support: ❌<br>
scope: 💻 server<br>
executed on: 🌐 control-npc<br>

Triggers when the server makes a connection to the server list.

## timeout

> introduced: beta 5<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 🌐 npc, control-npc<br>

Triggers when a timeout occurs on the NPC.

## triggeraction events (action...)

> introduced: 2.03<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 🌐 npc, control-npc<br>

| event | introduced | description |
| ----- | ---------- | ----------- |
| actionplayeronline | | triggers on Control-NPCs when a player logs in |
| actionclientside  | 2.21 | triggers on clientside weapon scripts |
| actionserverside  | | triggers on a serverside weapon when a client sends the `serverside` action |
| actionserverside  | 5.006 | triggers on a serverside NPC when a client sends the `servernpc` action |
| actionserver...   | | triggers on Control-NPCs |
| actionpushed      | | triggers when an NPC is pushed, `#p(0)` is the direction of the push |
| actionpulled      | | triggers when an NPC is pulled, `#p(0)` is the direction of the pull |
| actionprojectile  | 2.14 | - triggers on clients when a projectile collides with an npc/player<br>- triggers on Control-NPCs when a player spawned projectile collides or lands |
| actionsprojectile | 2.14 | triggers on Control-NPCs when a serverside spawned projectile collides or lands |
| actionprojectile2 | 2.19 | triggers on clients when a projectile lands on the ground |
| actionleftmouse   | 2.14 | triggers on NPCs the player clicks with the left mouse button |
| actionrightmouse  | 2.14 | triggers on NPCs the player clicks with the right mouse button |
| actionmiddlemouse | 2.14 | triggers on NPCs the player clicks with the middle mouse button |
| actiondoublemouse | 2.14 | triggers on NPCs the player double-clicks |
| actionserverstartparams | 2.30 | triggers on the Control-NPC when a player connects via the `graal://` or `graal3://` browser protocols:<pre>graal://servername/param1,param2,ect...</pre> |
| action... | | triggers on NPCs that the event touches |

## updategani

> introduced: -<br>
gserver support: ✅ 4.0<br>
scope: 💻 server<br>
executed on: 🤖 npc<br>

Special event that gets triggered by the server when an item-class NPC needs to be updated.
For example, the item NPCs created from the "gralats" class will receive this event when the NPC's gralats property is changed.

## washit

> introduced: beta 5<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when an NPC is hit by `hitnpc` or `hitobjects`.

On clientside, this will also trigger by a player's sword attack.

## waspelt

> introduced: beta 5<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when an NPC is hit by a thrown object.

## wasshot

> introduced: beta 5 (wasshooted), beta 7<br>
alternative: wasshooted<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when an NPC was shot with an arrow.

Does not trigger for `shoot` based projectiles, which trigger `actionprojectile` style events instead.

## wasthrown

> introduced: beta 7<br>
gserver support: ✅ 4.0<br>
scope: 🧑 💻 client, server<br>
executed on: 🤖 npc<br>

Triggers when an NPC is thrown by a player.

## weaponfired

> introduced: around 1.19 to 1.21<br>
gserver support: ❌ (clientside)<br>
scope: 🧑 client<br>
executed on: ⚔️ weapon npc<br>

Triggered on a weapon NPC when the player fires the weapon.
