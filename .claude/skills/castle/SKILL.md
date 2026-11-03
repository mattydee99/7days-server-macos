---
name: castle
description: Run server console commands and use the CastleBuilder mod (castle build/check/undo/survey/repair) — the steel horde castle's layout, repair procedure and lessons learned. Use for any castle building, inspection, repair or zombie spawn test.
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

**The castle:** `tools/castle_plan.py` is the source of truth for the layout: a steel castle with a moat, a keep, a gate and bridge on the east side, and a spike pit below the bridge. Notes about the castle actually built on this server (where it is, which save, horde test results) are in `CLAUDE.local.md` if it exists.

**Repairing the castle:** `python3 tools/castle_plan.py repair` writes every solid castle block (steel, bars, spikes) in its final state, then `castle build repair.txt repair-<n>` re-places them all at full HP. Air positions are left out, so player-placed blocks in open space and the workstations are untouched. A player block that was destroyed is not restored. No restart is needed, so it can be run between horde waves. Run `castle check repair.txt` before (to see what was damaged) and after (should report `wrong=0 damaged=0`).

**Test spawns:** `spawnentityat <zombieName> 587 60 <z>` puts zombies on the road east of the gate, and `spawnwandering h` spawns a real wandering horde, which heads for players from wherever the game chooses.

Things learned building it:
- `steelShapes:<shape>` gives every shape in steel (10,000 HP), e.g. `steelShapes:barsCentered`. For bars, rotation 0 runs the panel along x (east–west), rotation 1 along z.
- Iron spikes (`trapSpikesIronDmg0`) have `Path="solid"`: zombie pathing treats them as obstacles, and they only have 200 HP each.
- Tile-entity blocks (`workbench`, `combineStation`) placed by `castle build` come out complete: the game's `OnBlockAdded` creates the multi-block children and the tile entity. Doors were not tried.
- Removed terrain is set to `MarchingCubes.DensityAir`; placed non-terrain blocks also use `DensityAir`.
- To check game APIs, decompile with ilspycmd in the .NET SDK container (`DOTNET_ROLL_FORWARD=Major`, `ilspycmd -r <Managed> -t <Type> Assembly-CSharp.dll`), memory-capped so it can't starve the server.
