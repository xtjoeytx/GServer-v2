# GS2Emu Special Flags

## Smooth movement

If the `flaghack_movement` option is enabled in the `serveroptions.txt` file,
the server will pass pixel-perfect movement updates to older clients via client flags.

Used flags:

    gr.x
    gr.y
    gr.z

## File reading

When the `triggerhack_files` option is enabled in the `serveroptions.txt` file,
the following flags will be used to return file contents to the client:

    gr.fileerror
    gr.filedata

The `gr.fileerror` flag will be set to `0` on no error, or `1,last line number` (e.g., `1,13`).
The `gr.filedata` flag will contain the contents of the requested line in the file.

## IP

When the `flaghack_ip` option is enabled in the `serveroptions.txt` file,
the following flag will be set with the player's remote IP address:

    gr.ip
