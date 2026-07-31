# GS1 Functions

Functions are used inside *expression*s and return a non-string value.

If a function lacks a `scope` tag, it is supported both clientside 🧑 and serverside 💻.

If it is known that official does not support this function serverside, it will be marked as such.

---
## abs

`abs(value)`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the absolute value of the given value.

---
## aindexof

`aindexof(value, array)`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the index in the array where the value can be found.

---
## arctan

`arctan(value)`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the mathematical arctangent of the given value.

---
## arraylen

`arraylen(array)`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns how many elements the array contains.

---
## ascii

`ascii(character)`

> introduced: 2.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the ASCII code of the given character.

`ascii(A)` returns `65`.

---
## base64decode

`base64decode(string)`

> introduced: ???<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Decodes a base64 encoded string.

---
## base64encode

`base64encode(string)`

> introduced: ???<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Encodes the given string in base64 encoding.

---
## cos

`cos(value)`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the mathematical cosine of the given value.

---
## exp

`exp(value)`

> introduced: ???<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Computes mathematical `e^value`.

---
## findnearestplayer

`findnearestplayer(x, y)`

> introduced: 5.00rev6<br>
scope: 💻 server<br>
gs2emu serverside: ✅<br>
official serverside: ???<br>

Finds the player nearest to the specified level tile position and returns them as an object.

Client 5.00rev6 added this function clientside, but due to the nature of returning an object,
it would only work serverside using the [with()](scripting-gs1-flow-control-operators.md#flow-control-statements) statement.

It is unknown if official supported this function serverside, either.
This gserver _DOES_ support this function serverside, and is used like so:

```
with (findnearestplayer(x+1.5, y+2)) {
  // ...
}
```

---
## getangle

`getangle(dx, dy)`

> introduced: 2.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the angle in radians from (0, 0) to the position specified by dx and dy.

| Input | Output |
| ----- | ------ |
| ( 0, -1) up    | 1.570796 (pi/2) |
| (-1,  0) left  | 3.141593 (pi) |
| ( 0,  1) down  | 4.712389 (3pi/2) |
| ( 1,  0) right | 0.000000 (0) |

---
## getareanpcs

`getareanpcs(x, y, width, height)`

> introduced: (npcserver)<br>
scope: 💻 server<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns an array with the indices of all NPCs contained within the search region.

The NPCs can then be accessed with:
```
this.npclist = getareanpcs(10, 10, 5, 5);
for (i=0; i<arraylen(this.npclist); i++) {
  npcid = this.npclist[i];
  with (npcs[npcid]) {
    // ...
  }
}
```

---
## getdir

`getdir(dx, dy)`

> introduced: 2.16<br>
official serverside: ???<br>

Returns the direction (0..3) that a character should face to "look" in the direction specified.

Diagonals are biased to looking up (0) and down (2).

| Input | Output |
| ----- | ------ |
| ( 0, -1) up   | 0 |
| (-1, 0) left  | 1 |
| ( 0, 1) down  | 2 |
| ( 1, 0) right | 3 |
| ( 0.5,  0.5) SE | 2 |
| ( 0.5, -0.5) NE | 0 |
| (-0.5, -0.5) NW | 0 |
| (-0.5,  0.5) SW | 2 |
| ( 0.5,  0.4) SWW | 3 |

---
## getflagkeys

`getflagkeys(prefix)`

> introduced: possibly 2.12<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Searches for all flags in the format of prefix### and returns an array of all the ###.

For example, assuming you have the flags `bankaccount_0`, `bankaccount_1`, `bankaccount_3`, etc:

```
this.acc = getflagkeys(bankaccount_);
// this.acc == {0, 1, 3, ...};
```

---
## getnearestplayer

`getnearestplayer(x, y)`

> introduced: 5.00rev6<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Finds the player nearest to the specified level tile position and returns their index in the [players\[\]](scripting-gs1-variables.md#players) array.

```
this.pid = getnearestplayer(x+1.5, y+2);
with (players[this.pid]) {
  // ...
}
```

---
## getnearestplayers

`getnearestplayers(x, y, condition)`

> introduced: 5.00rev6<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns an array of all the level players sorted by how close they are to the specified position.

The condition is an optional _expression_ that is executed for each player.
If not supplied, every player will be returned.
If `false`, the player will be excluded from the results.

```
// All players, except me, who own a dog.
pid = playerid;
dogowners = getnearestplayers(x+1.5, y+2, playerid != pid && strequals(#s(client.pet), dog));
for (i = 0; i < arraylen(dogowners); i++) {
  idx = dogowners[i];
  with (players[idx]) {
    // ...
  }
}
```

---
## getnpc

`getnpc(name)`

> introduced: (npcserver)<br>
scope: 💻 server<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns a database NPC with the given name.
The return value is an object, so it must be used in conjunction with the [with()](scripting-gs1-flow-control-operators.md#flow-control-statements) statement.

```
with (getnpc(Bank Teller)) {
  // ...
}
```

---
## getplayer

`getplayer(account)`

> introduced: (npcserver)<br>
scope: 💻 server<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the player associated with the given account name.
The reeturn value is an object, so it must be used in conjunction with the [with()](scripting-gs1-flow-control-operators.md#flow-control-statements) statement.

```
with (getplayer(#c)) {
  // ...
}
```

---
## getz

`getz(x, y)`

> introduced: possibly 2.12, revealed ???<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the Z height at the specified level tile position.

---
## hasright

`hasright(rw, path)`

> introduced: (npcserver)<br>
scope: 💻 server<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Checks if a player has permissions to an RC File Browser file or folder.

`rw` can be a combination of `r` (read), `w` (write), and `rw` (read+write).

`path` is a file or folder to check.

Assuming the player has the following right:
```
FOLDERRIGHT rw world/heads/*
```

Then the following would all be `true`:
```
hasright(r, world/heads/)
hasright(w, world/heads/head123.png)
hasright(rw, world/heads/head)
```

---
## hasweapon

`hasweapon(name)`

> introduced: 1.37<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Checks if the player has a weapon with the given name.

---
## imgheight

`imgheight(file)`

> introduced: 2.13<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Returns the height of the specified image.

---
## imgwidth

`imgwidth(file)`

> introduced: 2.13<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Returns the width of the specified image.

---
## indexof

`indexof(substring, string)`

> introduced: 2.02<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the position in which `string` can be found in `substring`.

```
setstring test, thisisatest;
pos = indexof(isa, #s(test));
// pos == 4
```

---
## int

`int(value)`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Removes the floating point part of a number.  Does not round.

```
int(2) == 2
int(2.345) == 2
int(2.954) == 2
```

---
## keycode

`keycode(character)`

> introduced: 2.14<br>
official serverside: ???<br>

Returns the Windows Virtual Key Code value for the given character.

See: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

```
keycode(3) == 51
```

---
## keydown

`keydown(value)`

> introduced: 1.39rev2<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Returns `true` if the specified key is being pressed.
`Value` is a number from 0 through 10 and corresponds to one of the game's default control functions.

| Key | Function | Default |
| --- | -------- | ------- |
| 0 | Move Up | Up Arrow |
| 1 | Move Left | Left Arrow |
| 2 | Move Down | Down Arrow |
| 3 | Move Right | Right Arrow |
| 4 | Use Weapon | D |
| 5 | Use Sword | S |
| 6 | Grab | A |
| 7 | Use Map | M |
| 8 | Toggle Chat Bar | Tab |
| 9 | Open Inventory | Q |
| 10 | Pause | P |

```
if (keypressed && keydown(5)) { }
```

---
## keydown2

`keydown2(keycode, allowmodifiers)`

> introduced: 2.14<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Returns true if the specified key is being pressed.

`keycode` is the Windows Virtual Key Code value for the key to check.

When `allowmodifiers` is `true`, modifier keys like `SHIFT` and `ALT` will be allowed.

```
// User presses CTRL+A
istrue = keydown2(keycode(A), true);
isfalse = keydown2(keycode(A), false);
```

---
## lindexof

`lindexof(string, list)`

> introduced: 2.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the index in which `string` can be found in `list`, a CSV formatted string list.
Returns `-1` if not found.

```
setstring test, Testing;
addstring test, More;
addstring test, Stuff;
isFound = lindexof(#s(test), More);   // 1
isNotFound = lindexof(#s(test), Hey); // -1
```

---
## log

`log(base, value)`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Inverse of the mathematical power function (`^` operator).
Can be used to determine what exponent the `base` must be powered by to reach `value`.
```
base^log(base, val) == val
log(3, 9) == 2
2^3 == 9
```

---
## max

`max(value1, value2)`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the maximum of two values.

---
## min

`min(value1, value2)`

> introduced: 2.16<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the minimum of two values.

---
## onmapx

`onmapx(level)`

> introduced: 2.03<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the X position of the level on the currently set bigmap.
Returns `-1` if the level is not on a bigmap.

---
## onmapy

`onmapy(level)`

> introduced: 2.03<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the Y position of the level on the currently set bigmap.
Returns `-1` if the level is not on a bigmap.

---
## onwall

`onwall(x, y)`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns true if the specified coordinate is blocking.

It tests:
- If the tile at the level position is a blocking tile.
- If an NPC collision bounding box blocks the location (hidden NPCs do not block).
- If a player's collision bounding box blocks the location ([noplayerkilling](scripting-gs1-commands.md#noplayerkilling) levels disable player blocking).

---
## onwall2

`onwall2(x, y, width, height)`

> introduced: 2.30<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Performs [onwall()](#onwall) tests across a region.

---
## onwater

`onwater(x, y)`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns true if the specified coordinate is a water tile.

---
## onwater2

`onwater2(x, y, width, height)`

> introduced: 2.30<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Performs [onwater()](#onwater) tests across a region.

---
## passwordmatches

`passwordmatches(hashed, string)`

> introduced: (npcserver)<br>
scope: 💻 server<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Checks if the given `string`, once password hashed, matches `hashed`.

Used in conjunction with the [#E()](scripting-gs1-messagecodes.md#estring) message code.

```
setstring hashed, #E(hunter2);
if (playerchats && passwordmatches(#s(hashed), #c)) {
  setstring #c, *******;
}
```

---
## playersays

`playersays(index, text)`

> introduced: 1.21<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Checks if the player at the given `index` in the level has a chat message equal to `text`.

Equivalent to: `strequals(#c(index), text)`

---
## playersays

`playersays(text)`

> introduced: 1.21<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Checks if the player has a chat message equal to `text`.

Equivalent to: `strequals(#c, text)`

---
## playersays2

`playersays2(index, text)`

> introduced: 1.21<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Checks if the player at the given `index` in the level has a chat message that contains `text`.

Equivalent to: `strcontains(text, #c(index))`

---
## playersays2

`playersays2(text)`

> introduced: 1.21<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Checks if the player has a chat message that contains `text`.

Equivalent to: `strcontains(text, #c)`

---
## random

`random(min, max)`

> introduced: beta 5<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Produces a random value in the range:
```
min <= value < max
```

---
## sarraylen

`sarraylen(list)`

> introduced: 2.10<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the number of elements inside a CSV formatted string list.

```
setstring test, This, "is, a", test;
num = sarraylen(test); // 3
```

---
## screenx

`screenx(x, y)`

> introduced: 2.16<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Converts the given level tile position to screen coordinates (pixels from top-left corner of the game window) and returns the X pixel position.

Requires both X and Y tile positions to work with 3D terrain.

---
## screeny

`screeny(x, y)`

> introduced: 2.16<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Converts the given level tile position to screen coordinates (pixels from top-left corner of the game window) and returns the Y pixel position.

Requires both X and Y tile positions to work with 3D terrain.

---
## sin

`sin(value)`

> introduced: around 1.20<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Computes the mathematical sine of the given `value`.

---
## startswith

`startswith(prefix, string)`

> introduced: 2.02<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Checks if `string` starts with `prefix`, in a case-insensitive manner.

The order of the parameters is swapped compared to [strcontains](#strcontains), which can be confusing.

---
## strcontains

`strcontains(string, substring)`

> introduced: 1.27<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Checks if `string` contains `substring`, in a case-insensitive manner.

---
## strequals

`strequals(string1, string2)`

> introduced: 1.24<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Checks if `string1` is equal to `string2`, in a case-insensitive manner.

---
## strlen

`strlen(string)`

> introduced: 2.02<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the number of characters in `string`.

---
## strtofloat

`strtofloat(string)`

> introduced: 1.27<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Converts `string` into a number.

```
if (playerchats && startswith(/x, #c)) {
  tokenize #c;
  playerx = strtofloat(#t(1));
}
```

---
## testbomb

`testbomb(x, y)`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Returns the index of the bomb at the level tile position, or `-1` if no bomb was found.

The index can be used with the [bombs\[\]](scripting-gs1-variables.md#bombs) array.

---
## testcompu

`testcompu(x, y)`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Returns the index of the baddy at the level tile position, or `-1` if no baddy was found.

The index can be used with the [compus\[\]](scripting-gs1-variables.md#compus) array.

---
## testexplo

`testexplo(x, y)`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Returns the index of the explosion tile at the level tile position, or `-1` if no explosion tile was found.

The index can be used with the [explos\[\]](scripting-gs1-variables.md#explos) array.

This command tests explosion tiles.  When a standard bomb explodes, it creates five 2x2 explosion tiles, not one explosion.

---
## testhorse

`testhorse(x, y)`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Returns the index of the horse at the level tile position, or `-1` if no horse is found.

The index can be used with the [horses\[\]](scripting-gs1-variables.md#horses) array.

Only tests horses in the level.  If a player mounts a horse, it is removed from the level, and thus it is not detected.

---
## testitem

`testitem(x, y)`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ❌<br>

Returns the index of the item at the level tile position, or `-1` if no item is found.

The index can be used with the [items\[\]](scripting-gs1-variables.md#items) array.

---
## testnpc

`testnpc(x, y)`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the index of the NPC at the level tile position, or `-1` if no NPC is found.
The NPC must be visible and have a collision boundary to be discovered by this function.

The index can be used with the [npcs\[\]](scripting-gs1-variables.md#npcs) array.

---
## testplayer

`testplayer(x, y)`

> introduced: 1.38<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

It returns the returns the index of the player at the level tile position.

Serverside, if no player is found, it returns `-2`.

Clientside, it returns either `-2` or `-1`, depending on your client version.
In earlier versions of game, the client would include [showcharacter](scripting-gs1-commands.md#showcharacter) NPCs in the [players\[\]](scripting-gs1-variables.md#players) array.
For versions of the client that did that, it would return `-2` when it finds nothing, or `-1` if it finds the current NPC.
For later clients, it will return `-1` if nothing is found.

The player must be visible and have a collision boundary to be discovered by this function.
Players in [noplayerkilling](scripting-gs1-commands.md#noplayerkilling) levels or are currently paused will not be discovered.

The index can be used with the [players\[\]](scripting-gs1-variables.md#players) array.

---
## testsign

`testsign(x, y)`

> introduced: 1.39rev2<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the index of the sign at the level tile position, or `-1` if no sign is found.

---
## textheight

`textheight(zoom, font, style)`

> introduced: 2.20<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Returns the number of vertical pixels required to draw text in the given `zoom` level, `font`, and `style`.

---
## textwidth

`textwidth(zoom, font, style, text)`

> introduced: 2.19<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Returns the number of horizontal pixels required to draw the given `text` string in the given `zoom` level, `font`, and `style`.

---
## tiletype

`tiletype(x, y)`

> introduced: possibly 2.10 to 2.12, revealed ???<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the type of tile that is at the specified level tile position.
Any NPCs at the location that have used [setshape2](scripting-gs1-commands.md#setshape2) to change the types of tiles at that position will also be discovered by this function.

---
## vecx

`vecx(direction)`

> introduced: 2.03<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the X velocity an NPC will travel for the given `direction`.

| Input | Output |
| ----- | ------ |
| 0 (up)    |  0 |
| 1 (left)  | -1 |
| 2 (down)  |  0 |
| 3 (right) |  1 |

---
## vecy

`vecy(dir)`

> introduced: 2.03<br>
scope: 🧑💻 clientside, serverside<br>
gs2emu serverside: ✅<br>
official serverside: ✅<br>

Returns the Y velocity an NPC will travel for the given `direction`.

| Input | Output |
| ----- | ------ |
| 0 (up)    | -1 |
| 1 (left)  |  0 |
| 2 (down)  |  1 |
| 3 (right) |  0 |

---
## worldx

`worldx(x, y)`

> introduced: 2.16<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Converts the given screen coordinates (pixels from top-left corner of the game window) to a level position and returns the level tile X position.

Requires both X and Y tile positions to work with 3D terrain.

---
## worldy

`worldy(x, y)`

> introduced: 2.16<br>
scope: 🧑 client<br>
gs2emu serverside: ❌<br>
official serverside: ❌<br>

Converts the given screen coordinates (pixels from top-left corner of the game window) to a level position and returns the level tile Y position.

Requires both X and Y tile positions to work with 3D terrain.
