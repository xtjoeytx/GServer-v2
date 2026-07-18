# GS1 Events

Anything that is marked as executing on an NPC will also execute on clientside weapons, which are technically NPCs on the client.

This does NOT include serverside weapon scripts, which only respond to "serverside" action events that explicitly target the weapon.

Events with an empty "introduced" field have no known release date since they were serverside-only events that were never documented in release notes.

---
## compusdied

> introduced: beta 2<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ???<br>
executed on: 🤖 npc<br>

Triggers when all of the baddies in the level have died.

---
## created

> introduced: 1.27<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 🌐 npc, control-npc<br>

Triggered when NPC is created or its script has been changed.  It will only be fired once for an NPC.

---
## emoticon

> introduced: after 2.10, before 2.17rev1, maybe 2.13rev3?<br>
scope: 🧑 client<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>
executed on: 🤖 npc<br>

Triggered when the player displays an emoticon.

---
## exploded

> introduced: beta 5<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 npc<br>

Triggered when an NPC is touched by an explosion.

---
## firedonhorse

> introduced: 1.34r1<br>
scope: 🧑 client<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>
executed on: ⚔️ weapon npc<br>

Triggered on a weapon NPC when the player fires the weapon while on a horse.

---
## initialized

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 🌐 npc, control-npc<br>

Triggers when a serverside NPC is loaded from the disk on server start.

---
## itemdrop

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🌐 control-npc<br>

Triggers when an item is dropped on the ground, assuming the `itemdropevents` option is enabled in `serveroptions.txt`.

---
## keypressed

> introduced: 2.14<br>
scope: 🧑 client<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>
executed on: 🤖 npc<br>

Triggered on an NPC when a key is pressed.

Related message codes:
- `#p(0)` - keycode
- `#p(1)` - character

---
## mousedown

> introduced: 2.14<br>
scope: 🧑 client<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>
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

---
## mouseup

> introduced: 2.14<br>
scope: 🧑 client<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>
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

---
## mousewheel

> introduced: 2.14<br>
scope: 🧑 client<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>
executed on: 🤖 npc<br>

Triggered on an NPC when the player scrolls the mouse wheel.

Related flags: `mousewheeldelta`

---
## movementfinished

> introduced: 2.03<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 npc<br>

Triggers after all movement commands have finished executing, assuming the 'inform when done' option was set.

---
## npcwarped

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 npc<br>

Triggers when an NPC changes level.

---
## playerchats

> introduced: 1.21<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 npc<br>

Triggers when a player's chat text changes.

---
## playerdies

> introduced: 2.01<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ???<br>
executed on: 🤖 npc<br>

Triggers when a player dies.

---
## playerendsreading

> introduced: beta 3<br>
scope: 🧑 client<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>
executed on: 🤖 npc<br>

Triggers when a player finishes reading a sign.

---
## playerenters

> introduced: beta 2<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 npc<br>

Triggers when a player enters the level.

---
## playerhurt

#### alt: playerhurted (GR serverside supported)

> introduced: 2.01<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ???<br>
executed on: 🤖 npc<br>

Triggers when a player loses health.

---
## playerlaysitem

> introduced: beta 2<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ???<br>
executed on: 🤖 npc<br>

Triggers when a player drops an item on the ground.

---
## playerleaves

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 npc<br>

Triggers when a player leaves the level.

---
## playerlogin

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🌐 control-npc<br>

Triggers when a player logs into the server.

---
## playerlogout

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🌐 control-npc<br>

Triggers when a player logs out of the server.

---
## playertouchsme

#### alt: playertouchesme

> introduced: beta 2<br>
scope: 🧑 💻 client and server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 npc<br>

Triggers on an NPC when a player touches it.

---
## playertouchsother

#### alt: playertouchesother

> introduced: beta 2<br>
scope: 🧑 💻 client and server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 npc<br>

Triggers on an NPC when a player touches some other NPC.

---
## pm

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🌐 control-npc<br>

Triggers when a player sends a private message to the NPC-Server player.

---
## rcchat

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🌐 control-npc<br>

Triggers whenever an RC player sends an RC chat message starting with `/npc`.

---
## serverlistconnect

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🌐 control-npc<br>

Triggers when the server makes a connection to the server list.

---
## timeout

> introduced: beta 5<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 🌐 npc, control-npc<br>

Triggers when a timeout occurs on the NPC.

---
## triggeraction events

`action...`

> introduced: 2.03<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
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

---
## updategani

> introduced: -<br>
scope: 💻 server<br>
gserver serverside: ✅<br>
official serverside: ✅<br>
executed on: 🤖 npc<br>

Special event that gets triggered by the server when an item-class NPC needs to be updated.
For example, the item NPCs created from the "gralats" class will receive this event when the NPC's gralats property is changed.

---
## washit

> introduced: beta 5<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ???<br>
executed on: 🤖 npc<br>

Triggers when an NPC is hit by `hitnpc` or `hitobjects`.

On clientside, this will also trigger by a player's sword attack.

---
## waspelt

> introduced: beta 5<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ???<br>
executed on: 🤖 npc<br>

Triggers when an NPC is hit by a thrown object.

---
## wasshot

#### alt: wasshooted

> introduced: beta 5 (wasshooted), beta 7<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ???<br>
executed on: 🤖 npc<br>

Triggers when an NPC was shot with an arrow.

Does not trigger for `shoot` based projectiles, which trigger `actionprojectile` style events instead.

---
## wasthrown

> introduced: beta 7<br>
scope: 🧑 💻 client, server<br>
gserver serverside: ✅<br>
official serverside: ❌<br>
executed on: 🤖 npc<br>

Triggers when an NPC is thrown by a player.

---
## weaponfired

> introduced: around 1.19 to 1.21<br>
scope: 🧑 client<br>
gserver support: ❌ (clientside)<br>
official support: ❌<br>
executed on: ⚔️ weapon npc<br>

Triggered on a weapon NPC when the player fires the weapon.
