# GS1 Flow Control and Operators

GS1 uses a C-like syntax.  Braces (`{` and `}`) are used to wrap blocks of code and semi-colons (`;`) are used to separate statements.

It uses C99/C++ style comments:
> // everything until the end of the line.<br>
> /* everything until<br>
> the closing */<br>

Strings are trimmed.
Whitespace in expressions and statements is ignored, except in a few circumstances (which will be mentioned when relevant).

Unlike other languages, statements are not expressions.
This means that the variable increment statement cannot be used anywhere an expression is used.
Something like this is not possible in GS1:

```
// Not possible
this.count = this.i++;
```

A GS1 script consists of a list of statements, each separated by a semi-colon (`;`).
Statements can be blocks of code, which takes this form:

```
'{'
    statement ';'
    statement ';'
'}'
```

Blocks are most commonly seen in response to a flow control statement.
For example, an `if` statement will execute the next statement if the tested condition is true.
By using a block as the next statement, multiple statements will be executed in response to the condition.

```
if (true) statement;
if (true) {
  statement;
  statement;
}
if (true)
  if (true) {
    statement;
    statement;
  }
```

## Flow control statements

| Directive | Introduced | Description |
| --------- | ---------- | ----------- |
| `if` `(` _expression_ `)` _statement_ | Beta 2 | If the _expression_ is `true`, the _statement_ is executed. |
| `if` `(` _expression_ `)` _statement_ `else` _statement_ | Beta 5 | If the _expression_ is `true`, the first _statement_ is executed. Otherwise, the `else` _statement_ is executed. |
| `for` `(` _init-op_ `;` _expression_ `;` _incr-op_ `)` _statement_ | around 1.20 | Performs _init-op_ (an assignment statement), then executes the _statement_ if the _expression_ is `true`.  After each time the _statement_ was executed, _incr-op_ is performed (an assigment statement). |
| `while` `(` _expression_ `)` _statement_ | 1.2.2 | Continuously executes the _statement_ while the _expression_ is `true`. |
| `with` `(` _expression_ `)` _statement_ | (npcserver) | Executes _statement_ under the context of the object returned from _expression_. |

#### With

Changes the current source object the script is running in relation to.

```
if (playertouchsme) {
  playerx = 10;    // Moves the player who touched the NPC.
  with (getplayer(Bob)) {
    playerx = 15;  // Moves Bob, not the player who touched the NPC.
  }
}
```

#### Loop control

Introduced in 1.39rev2.

| Command | Description |
| ------- | ----------- |
| `continue` | Skips to the end of the loop. |
| `break` | Breaks out of a loop. |
| `return` | Breaks out of a function. |

## Assignment statements

| Directive | Introduced | Description |
| --------- | ---------- | ----------- |
| _identifier_ `=` _expression_ | Beta 5 | Assigns the value of the _expression_ to the _identifier_. |
| _identifier_ `+=` _expression_ | Beta 5 | Adds the value of the _expression_ to the _identifier_. |
| _identifier_ `-=` _expression_ | Beta 5 | Subtracts the value of the _expression_ from the _identifier_. |
| _identifier_ `*=` _expression_ | Beta 5 | Multiplies the _identifier_ by the value of the _expression_. |
| _identifier_ `/=` _expression_ | 2.16rev5 | Divides the _identifier_ by the value of the _expression_. |
| _identifier_ `++` | Beta 5 | Increments (adds) the _identifier_ by 1. |
| _identifier_ `--` | Beta 5 | Decrements (subtracts) the _identifier_ by 1. |

#### Alternatives were introduced around version 1.20:

| Operator | Alternative |
| -------- | ----------- |
| `=` | `:=` |

## Logical expression operators

| Operator | Introduced | Description |
| -------- | ---------- | ----------- |
| `!` _expression_ | Beta 2 | Logical NOT.  If the _expression_ is `true`, it becomes `false`, and vice-versa. |
| _expression_ `&&` _expression_ | Beta 2 | Logical AND.  Returns `true` if both *expression*s are `true`.  If the first _expression_ is false, the second will not be evaluated. |
| _expression_ `||` _expression_ | Beta 2 | Logical OR.  Returns `true` if one _expression_ is `true`.  If the first _expression_ is `true`, the second will not be evaluated. |
| _expression_ `==` _expression_ | Beta 5 | Returns `true` if both expressions are equal to each other. |
| _expression_ `!=` _expression_ | around 1.20 | Returns `true` if both expressions are **not** equal to each other. |
| _expression_ `>` _expression_ | around 1.20 | Returns `true` if the left _expression_ has a greater value than the right _expression_. |
| _expression_ `>=` _expression_ | around 1.20 | Returns `true` if the left _expression_ has a greater value or is equal to the right _expression_. |
| _expression_ `<` _expression_ | around 1.20 | Returns `true` if the left _expression_ has a lesser value than the right _expression_. |
| _expression_ `<=` _expression_ | around 1.20 | Returns `true` if the left _expression_ has a lesser value or is equal to the right _expression_. |
| _expression_ ` in ` _array_ | 1.38 | Returns `true` if _expression_ is contained within the array. |
| _expression_ [, _expression_ ...] ` in ` _array_ | 1.40 | Returns `true` if every _expression_ is contained within the array. |
| _expression_ [, _expression_ ...] ` in ` _range_ | 1.40 | Returns `true` if every _expression_ is contained within the range. |
| _expression_ `?` _expression_ `:` _expression_ | 1.39rev2 | Ternary expression, shortcut for an if-then statement that can be used in an expression. |

#### range

> `|` _expression_ `,` _expression `|`<br>
`<` _expression_ `,` _expression `>`<br>

`|` denotes an _INCLUSIVE_ boundary.<br>
`<` and `>` denotes an _EXCLUSIVE_ boundary.

Examples:
```
x in |0,64|   - is true when 0 <= x <=64
x in <0,64>   - is true when 0 < x < 64
x,y in <0,10| - is true when 0 < x <= 10 AND 0 < y <= 10
```

#### Alternatives:

| Operator | Alternative | Introduced |
| -------- | ----------- | ---------- |
| `==` | `=` | around 1.20 |
| `!=` | `<>` | 1.2.1 |
| `>=` | `=>` | around 1.20 |
| `<=` | `=<` | around 1.20 |

Array comparisons using `==` and `!=` were added in 1.38.

## Mathematical expression operators

Mathematical expression operators are performed on two expressions.
Both expressions are implicitly converted to a `double` data type, the operation is performed on them, and the result is returned as a `double`.

| Operator | Introduced | Description |
| -------- | ---------- | ----------- |
| _expression_ `+` _expression_ | Beta 5 | Adds the two expressions together. |
| _expression_ `-` _expression_ | Beta 5 | Subtracts the result of the right expression from the first. |
| _expression_ `*` _expression_ | Beta 5 | Multiplies the two expressions together. |
| _expression_ `/` _expression_ | around 1.20 | Divides the left expression by the right expression. |
| _expression_ `%` _expression_ | around 1.20 | Divides the left expression by the right expression and returns the remained (modulus division). |
| _expression_ `^` _expression_ | around 1.20 | Raises the left expression by the power of the right expression (exponentiation).  `x ^ 0.5` is synonymous with taking the square root of the number. |

Versions prior to 2.16rev5 did not follow the order of operations.

## User-defined Functions

> `function` name `(` `)` `{` _statement-list_ `}`

Creates a named block of code that can be called within the script.
Unlike other languages, GS1 functions do not support passing parameters to them.

For example:
```
if (playerenters) {
  myFunc();
  setplayerprop #c,#v(this.i);
}
function myFunc() {
  this.i = 10;
}
```
