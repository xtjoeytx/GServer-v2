# Translation - Classic Mode

## Overview

Classic translation mode is enabled when the [server Generation](server.md#server-generation) is not set to `modern`.
Translations are simple direct text replacements, without any advanced features.

## File format

File name: `slanguageDomain.txt`

The language domain is one of the built-in languages of the older clients:
- Deutsch
- English
- Español
- Français
- Italiano
- Nederlands
- Norsk
- Português
- Svenska

The special domain `Original` is used for the original text.

The file consists of an MD5 hash of the text, followed by the translated text within quotes (with quotes and backslashes escaped).

`12345678901234567890123456: "Translated \"Text\""`

## Coverage

Currently, signs (including both `say` and `say2`), RPG messages, the `#U(...)` message code,
and server generated PMs (jail message, `sendpm` command, and `setpm` replies) are translated.

Translation currently happens AFTER strings are fully processed.

## Missing features

For full compliance:
- Translation needs to happen BEFORE strings are fully processed (so unprocessed message codes are included).
- NPC chat/messages need to be translated.
