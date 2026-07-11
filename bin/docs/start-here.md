# Steps to setting up a server

## Set up your initial player account

1. Under the accounts folder, rename the text file `YOURACCOUNT.txt` to your account name.  For example: `KuJi.txt`
1. Edit the `config/serveroptions.txt` file, find the line that starts with `staff=`, and replace `YOURACCOUNT` with your account name.

## Configure your server

1. Modify the `config/serveroptions.txt` file:
   1. Set the `name` and `description` of your server.  Servers must have unique names or they will be rejected from the server list.
   1. Set the `generation` of your server.  This controls how the server behaves and which clients can connect.  See: [Generations](server.md#server-generation).
   1. Set the `serverside` setting to `true` if you want to run an NPC-Server.  Only the `classic` and `newmain` generations can run without one.
1. If you wish to change which clients are allowed, modify the `allowedverions.txt` file to adjust which client versions are allowed for your chosen generation.
1. Modify the `config/foldersconfig.txt` to adjust where the server looks for various files.  Make sure to add any changes to your account file's `FOLDERRIGHT` entries if you want to use RC to upload files.

# FAQ

## Making a private, hidden server

1. Modify the `config/adminconfig.txt` file and set `hq_level` to `0`.
1. Modify the `config/serveroptions.txt` file and set `onlystaff` to `true`.

## Adding new staff accounts

1. Edit the `config/serveroptions.txt` file (or use RC) and add the new account name to the `staff=` line, separating multiple accounts with commas.
1. Add or edit the player's account file in the `accounts` folder (or use RC) and:
   1. Adjust the `IPRANGE` setting.  A value of `*.*.*.*` would allow the account to connect from any IP address.
   1. Set the `LOCALRIGHTS` setting.  A value of `1040383` would give all rights.  It is easiest to use RC to selectively give rights.
   1. Add `FOLDERRIGHT` entries for RC File Browser access.  It is easiest to copy from your own staff account and modify as necessary.

# Further reading

- [Server](server.md) - Learn about server generations, client versions, and other server options.
- [Unimplemented Features](unimplemented.md) - See which features are not yet implemented on the new server.
- [NPC-Server](npcserver.md) - Learn about the NPC-Server, which is required for some generations and optional for others.
- Everything else in the `docs` directory.
