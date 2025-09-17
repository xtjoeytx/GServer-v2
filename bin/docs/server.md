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

## Configuration

### accounts/(npcserver).txt

This is the account for the NPC-Server player.

### accounts/defaultaccount.txt

Whenever a new player joins the server, a new account is created for them by cloning this file.
Configure where the player should start, which weapons they should have, and any other defaults with this account.

### config/adminconfig.txt

Controls how the server is displayed in the server list.

### config/allowedversions.txt

This file lists which client versions are allowed in the server.

If the file starts with `[generation-range]`, then each generation has a list of accept client ranges.
Each generation contains a comma-separated list of allowed versions.
Putting a colon (:) between two versions creates a range.
See the examples already in the file.

If the file does not have the generation indicator, then it uses an older format where each line lists a client version to allow.
Putting a colon (:) between two versions creates a range.

### config/foldersconfig.txt

Controls where types of files can be found on the file system.

Format: `filetype folder`

Valid file types:
- `file`: generic files
- `level`: level files
- `head`: head images
- `body`: body images
- `sword`: sword images
- `shield`: shield images
- ~~`sound`: sound files~~ (not supported)

The folder supports wildcards.

Example: `file images/*.png`

### config/ipbans.txt

A list of IP addresses that are banned from connecting to the server.
IP addresses are listed one per line and wildcards are supported.

### config/rchelp.txt

Contains the help text that gets sent to a player when they issue the `/help` command in RC Chat.

### config/rcmessage.txt

Contains the message that gets sent to a player when they join RC Chat.

### config/rules.txt

Chat filter rules.  See `rules.example.txt` for examples.

### config/servermessage.html

Contains the welcome message that gets sent to a player when they join the server.
The server message gets cached on the client so they will not see it again until it gets updated (or they visit a different server).

### config/serveroptions.txt

Controls various server options.  See the file itself for details.
