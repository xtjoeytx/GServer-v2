# GS2Emu Server

## Supported clients

The server supports clients from version 1.38 to 6.037.

## Server generation

In order to better support such a wide range of clients, a specific generation must be set on the server.
The generation will control how the server behaves, what features are available, how data is sent to clients, etc.

There are currently four identified generations:

    original - 1.x
    classic  - 2.x/3.x
    newmain  - 4.x to 5.007
    modern   - 5.1+

The generation is set in the `serveroptions.txt` file in your playerworld:

    generation = classic

## allowedversions.txt

This file lists which client versions are allowed in the server.

If the file starts with `[generation-range]`, then each generation has a list of accept client ranges.
Each generation contains a comma-separated list of allowed versions.
Putting a colon (:) between two versions creates a range.
See the examples already in the file.

If the file does not have the generation indicator, then it uses an older format where each line lists a client version to allow.
Putting a colon (:) between two versions creates a range.
