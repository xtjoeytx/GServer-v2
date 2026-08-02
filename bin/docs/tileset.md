# Tilesets

The game supports three different types of tilesets.

### Classic - Type 0

Default: `pics1.png`

The type 0 tileset is the original, classic tileset.
Tiles were added over time, resulting in a very disorganized tileset.
This tileset has many built-in client interactions, such as cutting bushes,
picking up signs, and others.

### New order - Type 1

The new order tileset was created as part of the New World project.
It features a reorganized tile structure, built to make custom tilesets easy.

See: [New order tileset](tileset-neworder.md)

### Terrain - Type 5

Default: `picso.png`

The terrain tileset is used in combination with 3D terrain.
3D terrain is a gmap feature, and terrain gmaps can be generated using the `TerrainGenerator.exe` tool.

See: [Terrain tileset](tileset-terrain.md)

---
## Tile types

| Index | Tile Type                            | Effect                                                   |
|-------|--------------------------------------|----------------------------------------------------------|
| 0     | non-blocking                         |                                                          |
| 2     | hurting underground                  | Hurts the player over time (type 0 only)                 |
| 3     | chair                                | Player switches to the `sit` animation.                  |
| 4     | bed upper                            | Heals the player over time.                              |
| 5     | bed lower                            | Heals the player over time.                              |
| 6     | swamp                                | Displays a grass animation at the character's feet.      |
| 7     | lava swamp                           | Displays a lava grass animation at the character's feet. |
| 8     | near water                           | Displays a water animation at the character's feet.      |
| 9     | near lava                            | Displays a lava animation at the character's feet.       |
| 10    | desert                               | Displays a lava animation at the character's feet.       |
| 11    | water                                | Makes the character swim.                                |
| 12    | lava                                 | Makes the character swim and hurts them over time.       |
| 20    | throw through                        | Thrown objects do not collide.                           |
| 21    | jump stone                           | Causes a jump when the player pushes against it.         |
| 22    | blocking                             |                                                          |
| 23    | blocking foreground (new world only) |                                                          |

The `desert` tile type is rendered exactly like `near lava` on the client, but has its own tile type.