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

## RC commands

- `/synctranslation [language]`:
Synchronizes an existing translation files with the `Original` domain.
If the language is not specified, all loaded languages are synchronized.
The language must be one of the endonyms listed above.
You can also use the ISO 639 language code (e.g. `en` for `English`, `de` for `Deutsch`, etc),
or an ISO Language Name (e.g. `English`, `German`, `Spanish`, etc).
- `/generatetranslationstubs`:
Generates translation files for all supported languages.

Any unused translations (they no longer exist in the `Original` domain) are written to `slanguageDomain.unused` and removed from the language file during all operations.

## Coverage

Currently, signs (including both `say` and `say2`), RPG messages, the `#U(...)` message code,
and server generated PMs (jail message, `sendpm` command, and `setpm` replies) are translated.

Translation happens before strings are fully processed so message codes can be included.

## Missing features

For full compliance:
- NPC chat/messages need to be translated.
