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
