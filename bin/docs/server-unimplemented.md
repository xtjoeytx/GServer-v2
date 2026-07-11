# GS2Emu Unimplemented Features

## Classic era
###### 1.x

Support for client versions below 1.38 is not supported and may not work.

Clients 1.38 - 1.411 are fully supported with no known issues.

## Newmain era
###### 2.x / 3.x

Client authoritative:
- Full support with official with no known limitations.

Server authoritative (NPC-server):
- No support for trial accounts or ghost mode.
- Translations for NPC messages in GS1 scripts are not supported.
- Lacking some script commands / features that may or may not have been implemented on official servers (lack of documentation to make full judgement).
- tiles[] with negative indices does not work.

## Modern era
###### 4.x - 6.037

Clientside GS1 only works up to and including the 5.007 client.

Clientside GS2 scripts do not work on 4.x clients.
There are opcode differences that still need to be figured out.

Clientside GS2 on the 5.007 client works and there is no known limitations.

Serverside GS2 scripts are not supported.

Translations using the modern PO (Portable Object) file format are not supported.
