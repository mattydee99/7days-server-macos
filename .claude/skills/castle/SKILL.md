---
name: castle
description: Run server console commands and use the CastleBuilder mod (castle build/check/undo/survey/repair) — the steel horde castle's layout, repair procedure, horde test results and lessons learned. Use for any castle building, inspection, repair or zombie spawn test.
---

# Server console and the CastleBuilder mod

Telnet is enabled but only on the container's loopback (port 8081 is not published), so console commands run via:

```bash
docker exec 7days console.sh "lp"                    # list players with positions
docker exec 7days console.sh -w 10 "<command>"       # wait longer for slow output
```

`mods/CastleBuilder` adds a `castle` console command that places blocks from plan files. It works while players are online (changes sync live); only changing the mod's code needs a restart. Typical workflow:

```bash
docker exec 7days backup.sh                                   # back up first
docker exec 7days console.sh "castle load 542 335 3"          # keep the area's chunks loaded
python3 tools/castle_plan.py <phase> [...]                    # writes castle/<phase>.txt
docker exec 7days console.sh "castle build <phase>.txt <name>" # applies it, saves castle/undo-<name>.txt
docker exec 7days console.sh -w 8 "castle survey x1 z1 x2 z2" # verify: ground y, top y, top block per column
docker exec 7days console.sh "castle undo <name>"             # exact rollback if needed
docker exec 7days console.sh "castle unload 542 335"          # always remove the observer afterwards (memory)
```

`castle check <plan>` compares every position in a plan with the world without changing anything and lists missing/swapped blocks (`wrong x y z expected -> actual`) and damaged ones (`damaged x y z name damage/maxDamage`); use it to find damage, since `survey` only sees the top block of each column. Other subcommands: `castle scan`/`castle strip x1 z1 x2 z2` (list/remove trees, plants and rocks above the terrain, leaving terrain, water and steel), `castle poc`/`castle clear` (3x3x3 test cube). Check `lp` before building so no player is standing inside the area being changed.

**The castle** (built 2026-10-24 in Navezgane, save `3.2new`, near the pine/burnt forest border by the paved road): footprint x 522–562, z 318–352, ground y=60. All steel. 2-thick outer walls 8 high with footing to y=55 and an outward overhang, four 5x5 corner towers with barred, roofed nests, a 3x3 gate in the east wall (z 334–336), a 4-deep steel-lined moat around a 21x19 keep (x 528–548, z 326–344) whose roof is level with the wall walk, a 1-wide bridge from the gate landing to a doorway in the keep's east wall (the player puts a wooden door there as the zombies' weak point), iron spikes on the moat floor at x 549–558, z 330–340, steps back up at the gate, a catwalk from the keep roof to the south wall walk, a workbench and combine station in the keep, and two 2x2 turret ledges (top y=64) on the inside of the east wall either side of the gate. The player added a vault door in the keep doorway, three wooden doors on the bridge as consumable choke points, shotgun turrets on the ledges and ladders up to the wall walk. `tools/castle_plan.py` is the source of truth for the layout (phases `phase2a`, `phase2b 0 1`, `phase3`, `phase3b`, `phase4 0 1`, `stations`, `ledges`; `phase3c` was a one-off spike extension written by hand, and `moat()`/`spikes()` now generate the final spike area).

**Repairing the castle:** `python3 tools/castle_plan.py repair` writes every solid castle block (steel, bars, spikes) in its final state, then `castle build repair.txt repair-<n>` re-places them all at full HP. Air positions are left out, so player-placed blocks in open space and the workstations are untouched. A player block that was destroyed is not restored. No restart is needed, so it can be run between horde waves. Run `castle check repair.txt` before (to see what was damaged) and after (should report `wrong=0 damaged=0`).

**Test spawns:** `spawnentityat <zombieName> 587 60 <z>` puts zombies on the road east of the gate, and `spawnwandering h` spawns a real wandering horde, which heads for players from wherever the game chooses. In testing (2026-10-25, 50+ zombies including ferals, spiders and a night horde), every zombie that reached the castle used the gate and the bridge. None touched the walls, and most died on the spikes after being shoved off the bridge while stacking at a door. Spiders jump but don't climb. Turrets on the y=64 ledges can't hit the landing or the nearest door: they tilt at most 45° down, and also need 15W of power and ammo. The first real blood moon (day 102) did no damage to the outside, but `castle check` found 7 spikes destroyed under the east end of the bridge, the bridge worn 10–25%, steel destroyed at the gate edges of the east wall, and 4 blocks dug out of the inner face of the west wall, by zombies that had fallen into the moat and walked the loop around the keep. The day-104 blood moon damaged the same places (8 steel blocks at the east-wall gate edges and the inner west wall, 5 spikes destroyed and 16 worn under the bridge); `repair` fixed it all.

Things learned building it:
- `steelShapes:<shape>` gives every shape in steel (10,000 HP), e.g. `steelShapes:barsCentered`. For bars, rotation 0 runs the panel along x (east–west), rotation 1 along z.
- Iron spikes (`trapSpikesIronDmg0`) have `Path="solid"`: zombie pathing treats them as obstacles, and they only have 200 HP each.
- Tile-entity blocks (`workbench`, `combineStation`) placed by `castle build` come out complete: the game's `OnBlockAdded` creates the multi-block children and the tile entity. Doors were not tried.
- Removed terrain is set to `MarchingCubes.DensityAir`; placed non-terrain blocks also use `DensityAir`.
- To check game APIs, decompile with ilspycmd in the .NET SDK container (`DOTNET_ROLL_FORWARD=Major`, `ilspycmd -r <Managed> -t <Type> Assembly-CSharp.dll`), memory-capped so it can't starve the server.
