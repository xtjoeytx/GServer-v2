# Tileset - Terrain (Type 5)

Default tileset: `picso.png`

The terrain tileset is used in combination with 3D terrain.
3D terrain is a gmap feature, and terrain gmaps can be generated using the `TerrainGenerator.exe` tool. 

This tileset has no default client interactions, and every tile is `non-blocking` ([tiletype 0](tileset.md#tile-types)).
Levels using 3D terrain must use NPCs to control blocking behavior.

### How to use

The tileset can be activated using the [addtiledef](scripting-gs1-commands.md#addtiledef) command, using `5` as the tileset type.

```
addtiledef picso.png,levelprefix_,5;
```

### Rendering

The game engine will automatically render tiles in the level based on the terrain height.
Game variables exist to control how the terrain is rendered.

The [groundheights\[\]](scripting-gs1-variables.md#levels) array consists of 7 values.
The client starts with index 0 and draws tiles until it reaches the set Z level.
It then switches to the next index and draws the next set of tiles, continuing until it reaches the end of the array.
At the end of the array, all further tiles will be drawn with the final tile page (index 7, not in the array).
If an index's Z level is identical or less than the previous index's Z level, that tile page will be skipped.

The tileset is paged from top to bottom, left to right.
```
0 2 4 6
|/|/|/|
1 3 5 7
```

| Index | Z  | Tiles                       |
|-------|----|-----------------------------|
| 0     | 0  | Water                       |
| 1     | 3  | Beach                       |
| 2     | 4  | Beach to Grass - Beach Side |
| 3     | 5  | Beach to Grass - Grass Side |
| 4     | 25 | Grass 1                     |
| 5     | 55 | Grass 2                     |
| 6     | 65 | Mountains                   |
| -     | -  | Ice                         |

The [waterheight](scripting-gs1-variables.md#levels) variable controls the height that water is drawn at on 3D terrain (global value).
The player will switch to the swim gani, but their Z height will still keep lowering until it hits zero.
