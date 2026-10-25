# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

A Docker setup to run the Linux x86_64 7 Days to Die dedicated server on Apple Silicon (ARM64) via the [box64](https://github.com/ptitSeb/box64) x86_64 emulator. The server files are not committed; they are downloaded separately with SteamCMD and mounted into the container.

## Common commands

### First-time setup

1. Install SteamCMD:
   ```bash
   brew install steamcmd
   ```

2. Download the Linux x86_64 dedicated server files (~16 GB) into `server/`:
   ```bash
   steamcmd \
     +force_install_dir /path/to/7days-docker/server \
     +@sSteamCmdForcePlatformType linux \
     +login anonymous \
     +app_update 294420 \
     +quit
   ```

3. Configure secrets:
   ```bash
   cp .env.example .env
   # edit .env
   ```

### Build and run

```bash
# Build image and start container in the background
docker compose up -d --build

# Rebuild from scratch
docker compose up -d --build --no-cache

# Force recreate after a SteamCMD update
docker compose up -d --build --force-recreate

# View logs
docker logs -f 7days

# Stop cleanly (SIGTERM triggers S3 save sync)
docker compose down
```

### Build the image directly

```bash
docker build -f docker/Dockerfile -t 7days-docker-7days:latest .
```

### Inspect the running server

```bash
# Enter the running container
docker exec -it 7days bash

# Check box64 version inside the container
docker exec -it 7days box64 --version
```

### Backups

Nightly backups to S3 run automatically at 03:00 UTC (see `docker/backup.sh`). To trigger one on demand or with a different retention count:

```bash
docker exec 7days backup.sh --retain 5
```

`BACKUP_RETAIN` in `.env` sets the default retention (how many dated nightly backups are kept before older ones are pruned); changing it only requires `docker compose up -d` to pick up, not a rebuild.

### Updating the game

When a new 7 Days to Die build is released:

```bash
# 0. Back up the current save before touching anything
docker exec 7days backup.sh

# 1. Pull the new build into server/ with SteamCMD
steamcmd \
  +force_install_dir /path/to/7days-docker/server \
  +@sSteamCmdForcePlatformType linux \
  +login anonymous \
  +app_update 294420 \
  +quit

# 2. Rebuild the image and recreate the container
docker compose up -d --build --force-recreate

# 3. Verify EOS still works
docker logs -f 7days
```

Watch for `[EOS] Server registered, session: ...` (success) vs `EOS_NoConnection` / `Failed creating session` (failure). If EOS registration breaks after an update, don't assume a session-architecture regression — re-check CA roots first (Epic/Google rotate certs occasionally; see "Why the stub/wrapper approaches failed" below) and never fall back to the `LinuxArm64` EOS SDK or the old stub. Also be aware that a major version jump can silently start a fresh world instead of loading an old save (see "Notes for changes"), which is why step 0 backs up first. Finish by attempting a real client connection, not just the log line. A game update can also break the `CastleBuilder` mod's compile (it builds against the game's DLLs); the image build fails loudly if so.

### Server console and the CastleBuilder mod

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

Other subcommands: `castle scan`/`castle strip x1 z1 x2 z2` (list/remove trees, plants and rocks above the terrain, leaving terrain, water and steel), `castle poc`/`castle clear` (3x3x3 test cube). Check `lp` before building so no player is standing inside the area being changed.

**The castle** (built 2026-10-24 in Navezgane, save `3.2new`, near the pine/burnt forest border by the paved road): footprint x 522–562, z 318–352, ground y=60. All steel. 2-thick outer walls 8 high with footing to y=55 and an outward overhang, four 5x5 corner towers with barred, roofed nests, a 3x3 gate in the east wall (z 334–336), a 4-deep steel-lined moat around a 21x19 keep (x 528–548, z 326–344) whose roof is level with the wall walk, a 1-wide bridge from the gate landing to a doorway in the keep's east wall (the player puts a wooden door there as the zombies' weak point), iron spikes on the moat floor at x 549–558, z 330–340, steps back up at the gate, a catwalk from the keep roof to the south wall walk, a workbench and combine station in the keep, and two 2x2 turret ledges (top y=64) on the inside of the east wall either side of the gate. The player added a vault door in the keep doorway, three wooden doors on the bridge as consumable choke points, shotgun turrets on the ledges and ladders up to the wall walk. `tools/castle_plan.py` is the source of truth for the layout (phases `phase2a`, `phase2b 0 1`, `phase3`, `phase3b`, `phase4 0 1`, `stations`, `ledges`; `phase3c` was a one-off spike extension written by hand, and `moat()`/`spikes()` now generate the final spike area).

**Repairing the castle:** `python3 tools/castle_plan.py repair` writes every solid castle block (steel, bars, spikes) in its final state, then `castle build repair.txt repair-<n>` re-places them all at full HP. Air positions are left out, so player-placed blocks in open space and the workstations are untouched. A player block that was destroyed is not restored. No restart is needed, so it can be run between horde waves.

**Test spawns:** `spawnentityat <zombieName> 587 60 <z>` puts zombies on the road east of the gate, and `spawnwandering h` spawns a real wandering horde, which heads for players from wherever the game chooses. In testing (2026-10-25, 50+ zombies including ferals, spiders and a night horde), every zombie that reached the castle used the gate and the bridge. None touched the walls, and most died on the spikes after being shoved off the bridge while stacking at a door. Spiders jump but don't climb. Turrets on the y=64 ledges can't hit the landing or the nearest door: they tilt at most 45° down, and also need 15W of power and ammo.

Things learned building it:
- `steelShapes:<shape>` gives every shape in steel (10,000 HP), e.g. `steelShapes:barsCentered`. For bars, rotation 0 runs the panel along x (east–west), rotation 1 along z.
- Iron spikes (`trapSpikesIronDmg0`) have `Path="solid"`: zombie pathing treats them as obstacles, and they only have 200 HP each.
- Tile-entity blocks (`workbench`, `combineStation`) placed by `castle build` come out complete: the game's `OnBlockAdded` creates the multi-block children and the tile entity. Doors were not tried.
- Removed terrain is set to `MarchingCubes.DensityAir`; placed non-terrain blocks also use `DensityAir`.
- To check game APIs, decompile with ilspycmd in the .NET SDK container (`DOTNET_ROLL_FORWARD=Major`, `ilspycmd -r <Managed> -t <Type> Assembly-CSharp.dll`), memory-capped so it can't starve the server.

## High-level architecture

### The problems this setup solves

1. **EOS SDK used to hang under box64** — the real EOS SDK's async completion callbacks would wedge indefinitely under default box64 dynarec settings. Fixed by setting `BOX64_DYNAREC_STRONGMEM=2`, `BOX64_DYNAREC_BLEEDING_EDGE=0`, `BOX64_DYNAREC_BIGBLOCK=0` (set as `ENV` in `docker/Dockerfile`). With these, the real, unmodified `libEOSSDK-Linux-Shipping.so` initializes and registers a session correctly — no stub, no wrapper needed.

2. **Mono JIT crashes under Rosetta 2 / QEMU** — Running the server with `--platform=linux/amd64` places JIT-compiled Mono code and native libraries more than 2 GB apart, which overflows x86 `CALL rel32` offsets. The fix is to run in an ARM64 container with box64, which manages its own x86_64 address space.

3. **Wrong working directory under box64** — Unity Addressables resolve asset bundle paths relative to the process CWD. box64 defaults CWD to `/`, so the entrypoint `cd /opt/7days` before launching.

### Key files and their roles

- `docker/Dockerfile` — two-stage `linux/arm64` build. The first stage compiles each `mods/<Name>/src/*.cs` with the .NET SDK's `csc` against the game's managed DLLs (mods without `src/` are copied as-is). The second installs box64 from ptitSeb's PPA, sets the `BOX64_DYNAREC_*` tuning env vars, copies `server/` in with the real EOS SDK untouched, and adds the compiled mods to `/opt/7days/Mods/`.
- `mods/CastleBuilder/` — server-side mod adding the `castle` console command (see "Server console and the CastleBuilder mod").
- `tools/castle_plan.py` — generates the castle's plan files into `castle/`.
- `docker/console.sh` — runs one console command over the container-local telnet port and prints its output.
- `docker/entrypoint.sh` — renders `serverconfig.xml` via `envsubst`, `cd`s to the game directory, launches the server under box64, runs a background loop that triggers `backup.sh` nightly at 03:00 UTC, and syncs saves to S3 on SIGTERM.
- `docker/backup.sh` — syncs `Saves/` to a timestamped `s3://$S3_BACKUP_BUCKET/backups/<UTC timestamp>/` prefix, then prunes older dated prefixes beyond `$BACKUP_RETAIN` (default 7). Runs automatically every night; can also be run manually (`docker exec 7days backup.sh --retain 5`) to back up on demand or change retention for that run. Separate from, and does not replace, the SIGTERM shutdown sync to `s3://$S3_BACKUP_BUCKET/saves/`.
- `config/serverconfig.xml` — server configuration; environment variables from `.env` are substituted at container startup.
- `eos-stub/eos_stub.c` + `eos-stub/eos.map` — **historical artifact**, no longer used by `docker/Dockerfile`. Kept for reference; see "Why the stub/wrapper approaches failed" below.
- `docker-compose.yml` — defines the `7days` service, `linux/arm64` platform, ports, bind mounts for saves and config (game files are baked into the image, not mounted, so a rebuild picks up SteamCMD updates), and 5-minute stop grace period.

### Volume layout

- `./server` → copied into the image at `/opt/7days` during build.
- `./config/serverconfig.xml` → mounted read-only at `/config/serverconfig.xml`.
- `./saves` → bind-mounted at `/home/steam/.local/share/7DaysToDie/Saves` and is gitignored.
- `./castle` → bind-mounted at `/home/steam/castle`, gitignored. Holds castle plan files and undo logs; not included in the S3 backups.

### Network protocols

`serverconfig.xml` controls which networking stacks are active. The valid values for `ServerDisabledNetworkProtocols` are `LiteNetLib` and `SteamNetworking`. Do not add `EOS` there — it is not a valid protocol choice.

## Current status: WORKING

As of 2026-09-05 (game build 24994542, the "v3.2" era), the server starts,
registers a real EOS session (`[EOS] Server registered, session: ...`), and a
real game client has successfully connected by direct IP over LAN. No
`EOS_NoConnection` or `Failed creating session` errors.

### Why the stub/wrapper approaches failed (do not re-attempt without new evidence)

The original assumption was that a headless dedicated server fundamentally
couldn't complete EOS session registration under emulation. That was wrong.
The actual root cause was CA certificate trust, discovered by inspecting the
embedded cert store in each `.so` with `strings`:

- `api.epicgames.dev` currently serves a certificate issued by **Google Trust
  Services (CN=WE1)**.
- The real x86_64 `libEOSSDK-Linux-Shipping.so` has `GTS Root R1`–`R4` baked
  into its statically-linked CA bundle → validates fine.
- The **ARM64-native** `libEOSSDK-LinuxArm64-Shipping.so` (the one the old
  custom box64 wrapper redirected calls to) only has legacy roots (DigiCert,
  Starfield, GlobalSign, Comodo, Entrust, GoDaddy) — no Google Trust Services
  root at all. It can **never** validate that certificate, no matter how
  correctly the wrapper mechanically works.
- The EOS SDK's CA store is compiled into the `.so`, not read from the
  container's system `/etc/ssl/certs` — so updating the container's OS-level
  `ca-certificates` package cannot fix this for the ARM64-native SDK.

This means: always use the real x86_64 EOS SDK (shipped unmodified in
`server/`), never the `LinuxArm64` variant, and never a stub if you want a
joinable server.

### Container restarts: check for VM-level OOM before suspecting EOS/box64

On 2026-09-05 the container restarted unexpectedly after ~13 hours of uptime.
`docker inspect` was misleading — it showed `OOMKilled: false` and
`ExitCode: 0` at the container level, because the container's own cgroup
never hit a limit. The actual cause was a **host-VM-wide kernel OOM kill**:
the game process's RSS had climbed to ~4.5GB over the long session, and
Docker Desktop's VM (then capped at ~7.75GB total, shared with several
other unrelated containers) ran out of memory, so the Linux kernel killed
the largest process — `7DaysToDieServer`.

If the container restarts again with no error/exception in `docker logs`
(just an abrupt `Killed` line from `entrypoint.sh`), check the VM's kernel
log first, before assuming an EOS/box64 regression:

```bash
docker run --rm --privileged --pid=host alpine sh -c 'dmesg | tail -60'
# look for a line containing "oom-kill" / "Out of memory: Killed process"
```

**Fix applied:** raised Docker Desktop's VM memory allocation to 16GB
(Docker Desktop → Settings → Resources → Memory) to give the game server
headroom alongside other containers sharing the same VM. This is a Docker
Desktop host setting, not something set in this repo's config.

## Notes for changes

- Any change to `config/serverconfig.xml` takes effect on the next `docker compose up` because the file is mounted read-only live.
- `./saves` persists across rebuilds because it is a host bind mount.
- There is no automated test suite in this repo; validation is done by starting the container, reading `docker logs` for `[EOS] Server registered, session:` (success) vs `EOS_NoConnection` / `Failed creating session` (failure), and ideally attempting a real client connection.
- A major game version jump (e.g. loading old v2.x saves under a v3.x server) can silently start a fresh world instead of loading the old save — this is a game-side save-format compatibility issue, unrelated to Docker/EOS.
- If SDK-embedded CA roots go stale again in the future (Epic/Google rotate certs again), re-run the `strings <lib>.so | grep -i "root ca\|certification authority\|GTS Root"` check against both EOS SDK variants before assuming it's a networking or session-architecture problem.
- If the container restarts unexpectedly, check for a VM-level OOM kill (see above) before assuming an EOS/box64 regression.
