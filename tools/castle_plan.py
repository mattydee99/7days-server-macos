#!/usr/bin/env python3
"""Generate CastleBuilder plan files for the hybrid castle.

Writes castle/<phase>.txt, which the server applies with
    docker exec 7days console.sh "castle build <phase>.txt <undo-name>"

Coordinates: +x is east, +z is north, y is height. Ground is y=60, so the
first block standing on the ground is y=61. Later lines override earlier
ones at the same position.

    python3 tools/castle_plan.py <phase> [bars_rot_ns bars_rot_ew]
"""
import os
import sys

STEEL = "steelShapes:cube"
BARS = "steelShapes:barsCentered"

# Footprint and outer wall
X0, X1 = 522, 562          # including the 1-block overhang ring
Z0, Z1 = 318, 352
FOOT = 55                  # wall footing; the moat floor will be at y=56
WALL_TOP = 68              # 8 high above ground
GATE_Z = (334, 336)        # 3-wide gate in the east wall
GATE_TOP = 63              # 3 high

# Corner towers: 5x5, solid to the wall top, then a nest y69-71 and a roof
TOWER = 5
NEST = (69, 71)
ROOF = 72

TOWERS = {  # name: (x0, z0)
    "sw": (X0, Z0), "se": (X1 - TOWER + 1, Z0),
    "nw": (X0, Z1 - TOWER + 1), "ne": (X1 - TOWER + 1, Z1 - TOWER + 1),
}


def fill(out, x0, y0, z0, x1, y1, z1, block, rot=None):
    out.append(f"fill {x0} {y0} {z0} {x1} {y1} {z1} {block}" + (f" {rot}" if rot is not None else ""))


def ring(out, x0, z0, x1, z1, y0, y1, block, rot=None):
    """1-thick rectangle outline from (x0,z0) to (x1,z1)."""
    fill(out, x0, y0, z0, x1, y1, z0, block, rot)
    fill(out, x0, y0, z1, x1, y1, z1, block, rot)
    fill(out, x0, y0, z0 + 1, x0, y1, z1 - 1, block, rot)
    fill(out, x1, y0, z0 + 1, x1, y1, z1 - 1, block, rot)


def in_tower(x, z):
    return any(tx <= x < tx + TOWER and tz <= z < tz + TOWER for tx, tz in TOWERS.values())


def walls(out):
    out.append("# outer wall: 2 thick, footing to y55, top y68")
    wx0, wx1, wz0, wz1 = X0 + 1, X1 - 1, Z0 + 1, Z1 - 1
    for off in (0, 1):
        ring(out, wx0 + off, wz0 + off, wx1 - off, wz1 - off, FOOT, WALL_TOP, STEEL)
    out.append("# overhang ring at the wall top, stops spiders")
    ring(out, X0, Z0, X1, Z1, WALL_TOP, WALL_TOP, STEEL)
    out.append("# merlons on the outer wall line, every other block")
    for x in range(wx0, wx1 + 1):
        for z in (wz0, wz1):
            if x % 2 == 0 and not in_tower(x, z):
                fill(out, x, WALL_TOP + 1, z, x, WALL_TOP + 1, z, STEEL)
    for z in range(wz0, wz1 + 1):
        for x in (wx0, wx1):
            if z % 2 == 0 and not in_tower(x, z):
                fill(out, x, WALL_TOP + 1, z, x, WALL_TOP + 1, z, STEEL)
    out.append("# gate in the east wall")
    fill(out, wx1 - 1, 61, GATE_Z[0], wx1, GATE_TOP, GATE_Z[1], "air")


def doorways(tx, tz):
    """Cells on the tower edge where the inner row of the wall walk meets it."""
    cells = []
    south = tz == Z0
    west = tx == X0
    inner_z = Z0 + 2 if south else Z1 - 2       # inner row of the south/north wall
    inner_x = X0 + 2 if west else X1 - 2        # inner row of the west/east wall
    cells.append((tx + TOWER - 1 if west else tx, inner_z))  # along the south/north wall
    cells.append((inner_x, tz + TOWER - 1 if south else tz))  # along the west/east wall
    return cells


def towers(out, bars_rot):
    for name, (tx, tz) in TOWERS.items():
        x1, z1 = tx + TOWER - 1, tz + TOWER - 1
        out.append(f"# {name} tower")
        fill(out, tx, FOOT, tz, x1, WALL_TOP, z1, STEEL)
        fill(out, tx + 1, NEST[0], tz + 1, x1 - 1, NEST[1], z1 - 1, "air")
        if bars_rot is None:
            ring(out, tx, tz, x1, z1, NEST[0], NEST[1], "air")
        else:
            ns, ew = bars_rot  # rotation for sides running along x, along z
            fill(out, tx + 1, NEST[0], tz, x1 - 1, NEST[1], tz, BARS, ns)
            fill(out, tx + 1, NEST[0], z1, x1 - 1, NEST[1], z1, BARS, ns)
            fill(out, tx, NEST[0], tz + 1, tx, NEST[1], z1 - 1, BARS, ew)
            fill(out, x1, NEST[0], tz + 1, x1, NEST[1], z1 - 1, BARS, ew)
        for cx, cz in ((tx, tz), (x1, tz), (tx, z1), (x1, z1)):
            fill(out, cx, NEST[0], cz, cx, NEST[1], cz, STEEL)
        for dx, dz in doorways(tx, tz):
            fill(out, dx, NEST[0], dz, dx, NEST[0] + 1, dz, "air")
        out.append(f"# {name} roof, overhanging 1 on every side")
        fill(out, tx - 1, ROOF, tz - 1, x1 + 1, ROOF, z1 + 1, STEEL)


# Moat: 4 deep (y57-60 dug out) with a steel floor at y56, all the way round
# the keep pad inside the walls.
INNER = (X0 + 3, Z0 + 3, X1 - 3, Z1 - 3)   # x525-559, z321-349
KEEP = (528, 326, 548, 344)                 # keep pad, x0 z0 x1 z1
MOAT_FLOOR = 56
SPIKES = "trapSpikesIronDmg0"


def moat(out):
    ix0, iz0, ix1, iz1 = INNER
    kx0, kz0, kx1, kz1 = KEEP
    strips = [
        (ix0, iz0, ix1, kz0 - 1),           # south
        (ix0, kz1 + 1, ix1, iz1),           # north
        (ix0, kz0, kx0 - 1, kz1),           # west
        (kx1 + 1, kz0, ix1, kz1),           # east, where the bridge and drop are
    ]
    out.append("# dig the moat, leaving terrain under the keep pad")
    for x0, z0, x1, z1 in strips:
        fill(out, x0, MOAT_FLOOR + 1, z0, x1, 60, z1, "air")
        fill(out, x0, MOAT_FLOOR, z0, x1, MOAT_FLOOR, z1, STEEL)
    out.append("# towers stay solid where they reach into the moat")
    for tx, tz in TOWERS.values():
        fill(out, tx, FOOT, tz, tx + TOWER - 1, 60, tz + TOWER - 1, STEEL)
    out.append("# keep pad: 2-thick steel sides down to the moat floor, steel top at ground level")
    for off in (0, 1):
        ring(out, kx0 + off, kz0 + off, kx1 - off, kz1 - off, MOAT_FLOOR, 60, STEEL)
    fill(out, kx0, 60, kz0, kx1, 60, kz1, STEEL)
    out.append("# remove the old test cube")
    fill(out, 541, 61, 334, 543, 63, 336, "air")
    out.append("# landing inside the gate, with 1-high steps down to the moat floor either side")
    gx = ix1
    fill(out, gx, MOAT_FLOOR + 1, GATE_Z[0] - 1, gx, 60, GATE_Z[1] + 1, STEEL)
    for i, top in enumerate((59, 58, 57)):
        fill(out, gx, MOAT_FLOOR + 1, GATE_Z[0] - 2 - i, gx, top, GATE_Z[0] - 2 - i, STEEL)
        fill(out, gx, MOAT_FLOOR + 1, GATE_Z[1] + 2 + i, gx, top, GATE_Z[1] + 2 + i, STEEL)
    spikes(out)


def spikes(out):
    """Spikes where zombies land off the bridge and landing: as wide as the
    steps and running from the keep pad right up to them."""
    out.append("# spikes under the bridge, from the keep pad to the steps")
    fill(out, KEEP[2] + 1, MOAT_FLOOR + 1, GATE_Z[0] - 4, INNER[2] - 1, MOAT_FLOOR + 1, GATE_Z[1] + 4, SPIKES)


# Keep: 7 high so its roof (y68) is level with the wall walk.
KEEP_TOP = WALL_TOP
BRIDGE_Z = 335
CATWALK_X = 538
KEEP_NEST = (533, 338, 539, 344)            # x0 z0 x1 z1, covers the stair hole


def keep(out, bars_rot):
    kx0, kz0, kx1, kz1 = KEEP
    ns, ew = bars_rot
    out.append("# keep walls, 2 thick, and roof")
    for off in (0, 1):
        ring(out, kx0 + off, kz0 + off, kx1 - off, kz1 - off, 61, KEEP_TOP - 1, STEEL)
    fill(out, kx0, KEEP_TOP, kz0, kx1, KEEP_TOP, kz1, STEEL)
    out.append("# doorway at the end of the bridge, for a wooden door placed in-game")
    fill(out, kx1 - 1, 61, BRIDGE_Z, kx1, 62, BRIDGE_Z, "air")
    out.append("# 1-high steel steps along the inside of the north wall up to the roof")
    sz = kz1 - 2
    for i in range(KEEP_TOP - 61 + 1):
        fill(out, kx0 + 3 + i, 61, sz, kx0 + 3 + i, 61 + i, sz, STEEL)
    fill(out, kx0 + 3 + 4, KEEP_TOP, sz, kx0 + 3 + 6, KEEP_TOP, sz, "air")
    out.append("# roof parapet with merlons")
    ring(out, kx0, kz0, kx1, kz1, KEEP_TOP + 1, KEEP_TOP + 1, STEEL)
    for x in range(kx0, kx1 + 1):
        for z in (kz0, kz1):
            if x % 2 == 0:
                fill(out, x, KEEP_TOP + 2, z, x, KEEP_TOP + 2, z, STEEL)
    for z in range(kz0, kz1 + 1):
        for x in (kx0, kx1):
            if z % 2 == 0:
                fill(out, x, KEEP_TOP + 2, z, x, KEEP_TOP + 2, z, STEEL)
    out.append("# roofed nest over the stair hole")
    nx0, nz0, nx1, nz1 = KEEP_NEST
    n0, n1 = KEEP_TOP + 1, KEEP_TOP + 3
    fill(out, nx0, n0, nz0, nx1, n1 + 1, nz1, "air")
    fill(out, nx0 + 1, n0, nz0, nx1 - 1, n1, nz0, BARS, ns)
    fill(out, nx0 + 1, n0, nz1, nx1 - 1, n1, nz1, BARS, ns)
    fill(out, nx0, n0, nz0 + 1, nx0, n1, nz1 - 1, BARS, ew)
    fill(out, nx1, n0, nz0 + 1, nx1, n1, nz1 - 1, BARS, ew)
    for cx, cz in ((nx0, nz0), (nx1, nz0), (nx0, nz1), (nx1, nz1)):
        fill(out, cx, n0, cz, cx, n1, cz, STEEL)
    fill(out, (nx0 + nx1) // 2, n0, nz0, (nx0 + nx1) // 2, n0 + 1, nz0, "air")
    fill(out, nx0 - 1, n1 + 1, nz0 - 1, nx1 + 1, n1 + 1, nz1 + 1, STEEL)
    out.append("# catwalk from the keep roof to the south wall walk")
    fill(out, CATWALK_X, KEEP_TOP + 1, kz0, CATWALK_X, KEEP_TOP + 2, kz0, "air")
    fill(out, CATWALK_X, KEEP_TOP, Z0 + 3, CATWALK_X, KEEP_TOP, kz0 - 1, STEEL)
    out.append("# 1-wide bridge from the gate landing to the keep door")
    fill(out, kx1 + 1, 60, BRIDGE_Z, INNER[2] - 1, 60, BRIDGE_Z, STEEL)


def stations(out):
    """Workbench and combine station (both 2x2x1) in the keep room, along the
    south side and a block clear of the walls; the game adds the extra
    pieces and the tile entity itself when the parent block is placed."""
    out.append("# workstations in the keep room")
    fill(out, 533, 61, 330, 533, 61, 330, "workbench")
    fill(out, 539, 61, 330, 539, 61, 330, "combineStation")


def ledges(out):
    """2x2 steel ledges for shotgun turrets on the inside of the east wall,
    either side of the gate, 4 above the landing so zombies there can't reach."""
    out.append("# turret ledges either side of the gate")
    x0, x1 = INNER[2] - 1, INNER[2]
    fill(out, x0, 64, GATE_Z[0] - 3, x1, 64, GATE_Z[0] - 2, STEEL)
    fill(out, x0, 64, GATE_Z[1] + 2, x1, 64, GATE_Z[1] + 3, STEEL)


def repair(out):
    """Every solid block of the finished castle, in build order, reduced to
    its final state. Air is left out, so anything a player placed in the
    open spaces (doors, turrets, ladders) and the workstations are untouched;
    re-placing a block resets its damage."""
    full = []
    walls(full)
    towers(full, (0, 1))
    moat(full)
    keep(full, (0, 1))
    ledges(full)
    final = {}
    for line in full:
        f = line.split()
        if not f or f[0] != "fill":
            continue
        x0, y0, z0, x1, y1, z1 = map(int, f[1:7])
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for z in range(min(z0, z1), max(z0, z1) + 1):
                for x in range(min(x0, x1), max(x0, x1) + 1):
                    final[(x, y, z)] = tuple(f[7:])
    out.append("# repair: every solid castle block at full HP")
    # Merge runs along x of the same block to keep the file small.
    for (x, y, z), b in sorted(final.items(), key=lambda kv: (kv[0][1], kv[0][2], kv[0][0])):
        if b[0] == "air" or (x - 1, y, z) in final and final[(x - 1, y, z)] == b:
            continue
        x1 = x
        while final.get((x1 + 1, y, z)) == b:
            x1 += 1
        fill(out, x, y, z, x1, y, z, *b)


def bars_test(out):
    out.append("# bars rotation test: 3 in a row along x per rotation, inside the future keep")
    for i, rot in enumerate(range(4)):
        x = 530 + i * 4
        fill(out, x, 61, 340, x + 2, 62, 340, BARS, rot)


def main():
    phase = sys.argv[1]
    out = []
    if phase == "phase2a":
        walls(out)
        towers(out, None)
        bars_test(out)
    elif phase == "phase2b":
        towers(out, (int(sys.argv[2]), int(sys.argv[3])))
        out.append("# remove the bars rotation test")
        fill(out, 530, 61, 340, 544, 62, 340, "air")
    elif phase == "phase3":
        moat(out)
    elif phase == "phase3b":
        spikes(out)
    elif phase == "stations":
        stations(out)
    elif phase == "repair":
        repair(out)
    elif phase == "ledges":
        ledges(out)
    elif phase == "phase4":
        keep(out, (int(sys.argv[2]), int(sys.argv[3])))
    else:
        sys.exit("unknown phase " + phase)
    path = os.path.join(os.path.dirname(__file__), "..", "castle", phase + ".txt")
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")
    print(f"wrote {path}: {len(out)} lines")


if __name__ == "__main__":
    main()
