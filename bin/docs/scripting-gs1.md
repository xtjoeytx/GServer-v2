# GS1 Scripting Reference

## Introduction

This is a reference of all known GS1 commands and how to use them.

---
## Flow Control and Operators

Flow control elements are used to direct the flow of the script.
Operators are used to perform math on numbers and make comparisons between them.

For example:
```
for (i = 0; i < 10; i++) {
  // do stuff
}
```

Reference: [GS1 Flow Control and Operators](scripting-gs1-flow-control-operators.md)

---
## Events

Events are triggered by the game engine when certain conditions are met.
Scripts are only executed when an event is triggered.
When an event is triggered, it sets a flag (with the same name as the event) that can be checked in the script.

For example:
```
if (playerenters) { }
```

Reference: [GS1 Events](scripting-gs1-events.md)

---
## Flags

Flags are set by the game engine when certain conditions are met.
They do not trigger scripts to execute, but can be checked in scripts like events.

For example:
```
if (playerenters && issparringzone) { }
```

Reference: [GS1 Flags](scripting-gs1-flags.md)

---
## Message Codes

Message codes are functions that can be used inside a string.
They all start with `#` and they all return a string value.

For example, this will set the string `mystring` to include the player's account name:
```
setstring mystring,My account is #a;
```

Reference: [GS1 Message Codes](scripting-gs1-messagecodes.md)

---
## Functions

Functions can be used in expressions and return a non-string value.

For example:
```
if (!onwall(x,y)) {
  type = tiletype(x,y);
}
```

Reference: [GS1 Functions](scripting-gs1-functions.md)

---
## Commands

Commands are statements that can be used in scripts to perform actions.
A command cannot be used in an expression and does not return a value.

For example:
```
setsword sword3.png;
```

Reference: GS1 Commands
