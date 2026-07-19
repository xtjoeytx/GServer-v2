# GS1 Message Codes

Message codes with an `index` that target a `character` can accept `-1` as an index value.
It will target the active player or NPC, depending on context.

Message codes that target a `player` will always target the currently active player when the index is `0`.
In clientside scripts, this will always be the player's character.

---
## \#\#

`##`

> introduced: ???<br>

Inserts a single `#` into the string.  It is an escape character for message codes.

---
## #1

`#1`
`#1(index)`

> introduced: 1.24 or 1.25<br>
permissions: [RW]<br>
targets: character<br>

The character's sword image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #2

`#2`
`#2(index)`

> introduced: 1.24 or 1.25<br>
permissions: [RW]<br>
targets: character<br>

The character's shield image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #3

`#3`
`#3(index)`

> introduced: 1.24 or 1.25<br>
permissions: [RW]<br>
targets: character<br>

The character's head image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #5

`#5`
`#5(index)`

> introduced: 1.24 or 1.25<br>
permissions: [RW]<br>
targets: character<br>

The character's horse image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #6

`#6`
`#6(index)`

> introduced: 1.24 or 1.25<br>
permissions: [R-]<br>
targets: player<br>

The image of the NPC the player is carrying.

`index` targets the player in the `players[]` array.

---
## #7

`#7`
`#7(index)`

> introduced: 1.24 or 1.25<br>
removed: 2.00<br>
permissions: [RW]<br>
targets: character<br>

The character's bow image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #8

`#8`
`#8(index)`

> introduced: 1.34 [R], 1.40 [RW]<br>
permissions: [RW]<br>
targets: character<br>

The character's body image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #a

`#a`
`#a(index)`

> introduced: 1.30<br>
permissions: [R-]<br>
targets: player<br>

The player's account name.

`index` targets the player in the `players[]` array.

---
## #b

`#b`

> introduced: 2.16<br>

Represents a line break for certain commands, like `say2`.

---
## #c

`#c`
`#c(index)`

> introduced: 1.24 or 1.25<br>
permissions: [RW]<br>
targets: character<br>

The character's chat text.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #Cn

`#C0 - #C4(index)`

> introduced: 1.24 or 1.25<br>
permissions: [RW]<br>
targets: character<br>

`#C5 - #C7(index)`

> introduced: newworld<br>
restriction: (client) newworld<br>
permissions: [RW]<br>
targets: character<br>

The character's body colors.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

| Code | Body Part |
| ---- | --------- |
| #C0 | Skin |
| #C1 | Coat |
| #C2 | Sleeves |
| #C3 | Shoes |
| #C4 | Belt |

| Code | Body Part |
| ---- | --------- |
| #C5 | Pullover |
| #C6 | Pants |
| #C7 | Border (outline) |

---
## #D

`#D`

> introduced: 2.14<br>
restriction: 🧑 clientside<br>

The name of the file currently being downloaded.
Paired with the variables `downloadpos` and `downloadsize`.

---
## #E

`#E`

> introduced: after 2.10, before 2.17rev1, maybe 2.13rev3?<br>
restriction: 🧑 clientside<br>

The current emoticon being displayed by the player.
Emoticons are displayed by holding the `CTRL` key down and pressing a key, like `A`.
When that happens, the `#E` message code will return `A`.

---
## #E(string)

`#E(string)`

> introduced: (npcserver)<br>
restriction: 💻 serverside<br>

Password hashes the given string.

---
## #e

`#e(start_index, length, string)`

> introduced: 2.02<br>

Extracts a substring from the given string.

---
## #F

`#F`

> introduced: (npcserver)<br>
restriction: 💻 serverside<br>
permissions: [R-]<br>
targets: player<br>

The level of the player.

---
## #f

`#f`

> introduced: 1.24 or 1.25<br>
permissions: [R-]<br>
targets: npc<br>

`#f(index)`

> introduced: 2.12<br>
permissions: [R-]<br>
targets: npc<br>

The image of the NPC.

`index` targets the NPC in the `npcs[]` array.

---
## #G

`#G`
`#G(index)`

> introduced: (npcserver)<br>
restriction: 💻 serverside<br>
permissions: [R-]<br>
targets: player<br>

The player's account level.

`index` targets the player in the `players[]` array.

| Levels |
| ------ |
| Trial   |
| Classic |
| Gold    |

---
## #g

`#g`
`#g(index)`

> introduced: 1.24 or 1.25<br>
permissions: [RW]<br>
targets: character<br>

The guild name of the character.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #i

`#i(image)`
`#i(image, x, y, width, height)`

> introduced: 2.02<br>
restriction: 🧑 clientside<br>

Displays an image or part of an image when used in a sign.

---
## #I

`#I(identifier, index)`

> introduced: 2.10<br>

Returns the string at the given index from the string list.

```
setstring test,This,is,a,test;
// #I(test,2) == a
```

---
## #K

`#K(ascii_number)`

> introduced: 2.00<br>

Returns the character represented by the given ASCII code.

```
#K(65) = A
```

---
## #k

`#k(key_index)`

> introduced: ???<br>

The description of the specified key (in client language/key assignments).

---
## #L

`#L`

> introduced: 1.24<br>
permissions: [R-]<br>
targets: npc<br>

The level of the source NPC.

---
## #m

`#m`
`#m(index)`

> introduced: 2.00<br>
permissions: [RW]<br>
targets: character<br>

The gani animation of the character.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #N

`#N`
`#N(index)`

> introduced: (npcserver)<br>
restriction: 💻 serverside<br>
permissions: [R-]<br>
targets: npc<br>

The name of a database NPC.

`index` targets the NPC in the `npcs[]` array.

---
## #n

`#n`
`#n(index)`

> introduced: 1.24 or 1.25<br>
permissions: [RW]<br>
targets: character<br>

The nickname of the character.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #p

`#p(index)`

> introduced: 2.03<br>

The value of a parameter, set by something like the `triggeraction` command.

---
## #Pn

`#P1 - #P30(index)`

> introduced: 2.02 (1-5), 2.13 (6-9), 2.16 (10-30)<br>
permissions: [RW]<br>
targets: character<br>

The gani attributes of the character.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #Q

`#Q(guild_name, account_name)`

> introduced: (npcserver)<br>
restriction: 💻 serverside<br>

The assigned nickname of the player in a guild.

---
## #R

`#R(string,...)`

> introduced: 2.19<br>

Randomly selects a string from the given string list.

---
## #S

`#S`

> introduced: newworld<br>
restriction: (client) newworld, 🧑 clientside<br>
permissions: [R-]<br>
targets: player<br>

The name of the player's currently selected sword.

---
## #s

`#s(identifier)`

> introduced: 1.27<br>

Returns the string value of the identifier.

---
## #t

`#t(index)`

> introduced: 2.02<br>

The value of a token created by the `tokenize` or `tokenize2` commands.

---
## #T

`#T(string)`

> introduced: 2.02<br>

Trims the string.

---
## #U

`#U(string)`

> introduced: (npcserver)<br>
restriction: 💻 serverside<br>

Replaces the string with a translated version of it.

---
## #v

`#v(identifier)`

> introduced: 1.24 or 1.25<br>

The value of the identifier transformed into a string.

---
## #W

`#W`

> introduced: possibly 2.04<br>
restriction: 🧑 clientside<br>
permissions: [R-]<br>
targets: player<br>

`#W(index)`

> introduced: 2.04<br>
restriction: 🧑 clientside<br>
permissions: [R-]<br>
targets: player<br>

The image of the player's currently selected weapon.

`index` targets a weapon by index.  Use `weaponscount` to determine the range.

---
## #w

`#w`

> introduced: 1.24 or 1.25<br>
restriction: 🧑 clientside<br>
permissions: [R-]<br>
targets: player<br>

`#w(index)`

> introduced: 2.04<br>
restriction: 🧑 clientside<br>
permissions: [R-]<br>
targets: player<br>

The name of the player's currently selected weapon.

`index` targets a weapon by index.  Use `weaponscount` to determine the range.
