# GS2Emu Clientside Join

## What is it?

The clientside join extension allows original and clientside classic mode servers to use the `join` command in clientside scripts.

## How to enable

Enable the following option in your `serveroptions.txt` file:

    clientsidejoins = true

Then, use the `join` command in your clientside scripts:

    if (created) join myClass;

The server will look for a `myClass.txt` file in the file system and append the contents of the file to the script.
Make sure the `.txt` file is located in a directory listed in `foldersconfig.txt`.
NOTE: You MUST include the semi-colon at the end of the command.  This is NOT optional.

## Implementation Considerations

The code looks for the `join` keyword and tests to see if it is the first word in the line, or if it was immediately preceeded by a `;`, `{`, or `)`.
If that check fails, it will not process the `join` command.
It will then also check if there is a `#b` message code within the same line as the `join` command.
If so, it will assume that it is the word "join" is within a `say2` command and will not process the `join` command.

Based on these checks, the following code will FAIL.

    say2 This#b
    is a test#b
    join things;

The clientside hack is not intelligent enough to determine that it is within the `say2` command.

Instead, you can use one of these workarounds:

    say2 This
    #bis a test
    #bjoin things;

    say2 This#b
    is a test#b
    join things; // #b

In the first example, the `join` command is NOT the first word in the line and it will not be processed as a `join` command.
In the second example, a `#b` message code is technically within the line (but commented out), so it will not be processed.
