# GS1 Message Codes

Message codes with an `index` that target a `character` can accept `-1` as an index value.
It will target the active player or NPC, depending on context.

Message codes that target a `player` will always target the currently active player when the index is `0`.
In clientside scripts, this will always be the player's character.

---
## \#\#

`##`

> introduced: ???

Inserts a single `#` into the string.  It is an escape character for message codes.

---
## #1

`#1`
`#1(index)`

> introduced: 1.24 or 1.25<br>
> permissions: [RW]<br>
> targets: character

The character's sword image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #2

`#2`
`#2(index)`

> introduced: 1.24 or 1.25<br>
> permissions: [RW]<br>
> targets: character

The character's shield image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #3

`#3`
`#3(index)`

> introduced: 1.24 or 1.25<br>
> permissions: [RW]<br>
> targets: character

The character's head image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #5

`#5`
`#5(index)`

> introduced: 1.24 or 1.25<br>
> permissions: [RW]<br>
> targets: character

The character's horse image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #6

`#6`
`#6(index)`

> introduced: 1.24 or 1.25<br>
> permissions: [R-]<br>
> targets: player

The image of the NPC the player is carrying.

`index` targets the player in the `players[]` array.

---
## #7

`#7`
`#7(index)`

> introduced: 1.24 or 1.25<br>
> removed: 2.00<br>
> permissions: [RW]<br>
> targets: character

The character's bow image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #8

`#8`
`#8(index)`

> introduced: 1.34 [R], 1.40 [RW]<br>
> permissions: [RW]<br>
> targets: character

The character's body image.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #a

`#a`
`#a(index)`

> introduced: 1.30<br>
> permissions: [R-]<br>
> targets: player

The player's account name.

`index` targets the player in the `players[]` array.

---
## #b

`#b`

> introduced: 2.16

Represents a line break for certain commands, like `say2`.

---
## #c

`#c`
`#c(index)`

> introduced: 1.24 or 1.25<br>
> permissions: [RW]<br>
> targets: character

The character's chat text.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #Cn

`#C0 - #C4(index)`

> introduced: 1.24 or 1.25<br>
> permissions: [RW]<br>
> targets: character

`#C5 - #C7(index)`

> introduced: newworld<br>
> restriction: (client) New World mode<br>
> permissions: [RW]<br>
> targets: character

The character's body colors.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

| Code  | Body Part  |
|-------|------------|
| #C0   | Skin       |
| #C1   | Coat       |
| #C2   | Sleeves    |
| #C3   | Shoes      |
| #C4   | Belt       |

Body colors 5-7 require the server to be set in New World mode, and require the New World client.

See: `serveroptions.txt` - `servermode` option.

| Code  | Body Part        |
|-------|------------------|
| #C5   | Pullover         |
| #C6   | Pants            |
| #C7   | Border (outline) |

---
## #D

`#D`

> introduced: 2.14<br>
> restriction: 🧑 clientside

The name of the file currently being downloaded.
Paired with the variables `downloadpos` and `downloadsize`.

---
## #E

`#E`

> introduced: after 2.10, before 2.17rev1, maybe 2.13rev3?<br>
> restriction: 🧑 clientside

The current emoticon being displayed by the player.
Emoticons are displayed by holding the `CTRL` key down and pressing a key, like `A`.
When that happens, the `#E` message code will return `A`.

---
## #E(string)

`#E(string)`

> introduced: (npcserver)<br>
> restriction: 💻 serverside

Password hashes the given string.

---
## #e

`#e(start_index, length, string)`

> introduced: 2.02

Extracts a substring from the given string.

---
## #F

`#F`

> introduced: (npcserver)<br>
> restriction: 💻 serverside<br>
> permissions: [R-]<br>
> targets: player

The level of the player.

---
## #f

`#f`

> introduced: 1.24 or 1.25<br>
> permissions: [R-]<br>
> targets: npc

`#f(index)`

> introduced: 2.12<br>
> permissions: [R-]<br>
> targets: npc

The image of the NPC.

`index` targets the NPC in the `npcs[]` array.

---
## #G

`#G`
`#G(index)`

> introduced: (npcserver)<br>
> restriction: 💻 serverside<br>
> permissions: [R-]<br>
> targets: player

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
> permissions: [RW]<br>
> targets: character

The guild name of the character.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #i

`#i(image)`
`#i(image, x, y, width, height)`

> introduced: 2.02<br>
> restriction: 🧑 clientside

Displays an image or part of an image when used in a sign.

---
## #I

`#I(identifier, index)`

> introduced: 2.10

Returns the string at the given index from the string list.

```
setstring test,This,is,a,test;
// #I(test,2) == a
```

---
## #K

`#K(ascii_number)`

> introduced: 2.00

Returns the character represented by the given ASCII code.

```
#K(65) = A
```

---
## #k

`#k(key_index)`

> introduced: ???

The description of the specified key (in client language/key assignments).

---
## #L

`#L`

> introduced: 1.24<br>
> permissions: [R-]<br>
> targets: npc

The level of the source NPC.

---
## #M

`#M(mud_object, attribute_name)`

> introduced: (npcserver) (Kingdoms)<br>
> gs2emu serverside: ❌<br>
> restriction: 💻 serverside<br>
> targets: MUD

Retrieves the string value of a MUD attribute from the specified MUD object.

The mud object is a string that represents the name of an object directly, or an ID number of an instance of the object.

```
#M(money,this.price)
#M(#s(clientr.selecteditem),weight)
```

---
## #m

`#m`
`#m(index)`

> introduced: 2.00<br>
> permissions: [RW]<br>
> targets: character

The gani animation of the character.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #N

`#N`
`#N(index)`

> introduced: (npcserver)<br>
> restriction: 💻 serverside<br>
> permissions: [R-]<br>
> targets: npc

The name of a database NPC.

`index` targets the NPC in the `npcs[]` array.

---
## #n

`#n`
`#n(index)`

> introduced: 1.24 or 1.25<br>
> permissions: [RW]<br>
> targets: character

The nickname of the character.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #p

`#p(index)`

> introduced: 2.03

The value of a parameter, set by something like the `triggeraction` command.

---
## #Pn

`#P1 - #P30(index)`

> introduced: 2.02 (1-5), 2.13 (6-9), 2.16 (10-30)<br>
> permissions: [RW]<br>
> targets: character

The gani attributes of the character.

`index` targets the character in the `players[]` or `npcs[]` array, depending on context.

---
## #Q

`#Q(guild_name, account_name)`

> introduced: (npcserver)<br>
> restriction: 💻 serverside

The assigned nickname of the player in a guild.

---
## #R

`#R(string,...)`

> introduced: 2.19

Randomly selects a string from the given string list.

---
## #S

`#S`

> introduced: newworld<br>
> restriction: (client) New World mode, 🧑 clientside<br>
> permissions: [R-]<br>
> targets: player

The name of the player's currently selected sword.

---
## #s

`#s(identifier)`

> introduced: 1.27

Returns the string value of the identifier.

---
## #t

`#t(index)`

> introduced: 2.02

The value of a token created by the `tokenize` or `tokenize2` commands.

---
## #T

`#T(string)`

> introduced: 2.02

Trims the string.

---
## #U

`#U(string)`

> introduced: (npcserver)<br>
> restriction: 💻 serverside

Replaces the string with a translated version of it.

---
## #U2

`#U2(string)`

> introduced: (npcserver) (Kingdoms)<br>
> restriction: 💻 serverside

Replaces the string with a translated version of it. 
The string is first processed before looking up the translation.

Take this example:
```
setstring this.str,#U(Your pet type is: #U2(#s(clientr.petname)));
```

The `#U` message code will create the following translation key:
> Your pet type is: #U2(#s(clientr.petname))

When translated, the `#U2` message code will first process the `#s(clientr.petname)` message code, and then look up the translation for the resulting string.

Assuming the `clientr.petname` identifier has the value `cat`, and the language is Spanish, the final translation key will be:
> Your pet type is: gato

The message codes can be chained:
```
setstring this.type,#U(the animal is #U2(#U2(#U(#s(client.animal)))));
```

Assume the following:
1. The `client.animal` identifier has an animal type for the default locale.
2. The `client.animal.es` identifier has an animal type of the Spanish local.
3. `client.animal` = `cat` and `client.animal.es` = `dog`.
4. There is a translation key on `#s(client.animal)` that returns `#s(client.animal.es)` for Spanish.
5. There are translation keys for `cat` and `dog` in Spanish that return `gato` and `perro`.
6. `the animal is #U2(#U2(#U(#s(client.animal))))` = `el animal es #U2(#U2(#U(#s(client.animal))))`.

This will break down as follows:
1. The outer `#U(...)` will process the whole string inside, returning `el animal es #U2(#U2(#U(#s(client.animal))))`.
2. The inner `#U2(#U(#s(client.animal))))` segment will process the string inside.
3. `#U(#s(client.animal))` will look for the Spanish translation for the key `#s(client.animal)`, returning `#s(client.animal.es)`.
4. The inner `#U2(...)` will then translate `#s(client.animal.es)`.  No key exists, so it will return the string as-is.
5. The outer `#U2(...)` will process and evaluate `#s(client.animal.es)`, which returns `dog`, then translates to `perro`.
6. The final result will be: `el animal es perro`.

The important thing to note is that `#U(...)` will treat all the text inside as a single translation key.
`#U(some #U(stuff))` will result in a translation key of `some #U(stuff)`, and will not process the inner `#U(stuff)` message code.
This is why `#U2(...)` is needed to process the inner message codes before looking up the translation, and  why this example needed to chain them together.

---
## #v

`#v(identifier)`

> introduced: 1.24 or 1.25

The value of the identifier transformed into a string.

---
## #W

`#W`

> introduced: possibly 2.04<br>
> restriction: 🧑 clientside<br>
> permissions: [R-]<br>
> targets: player

`#W(index)`

> introduced: 2.04<br>
> restriction: 🧑 clientside<br>
> permissions: [R-]<br>
> targets: player

The image of the player's currently selected weapon.

`index` targets a weapon by index.  Use `weaponscount` to determine the range.

---
## #w

`#w`

> introduced: 1.24 or 1.25<br>
> restriction: 🧑 clientside<br>
> permissions: [R-]<br>
> targets: player

`#w(index)`

> introduced: 2.04<br>
> restriction: 🧑 clientside<br>
> permissions: [R-]<br>
> targets: player

The name of the player's currently selected weapon.

`index` targets a weapon by index.  Use `weaponscount` to determine the range.
