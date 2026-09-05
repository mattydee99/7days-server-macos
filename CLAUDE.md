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

## High-level architecture

### The problems this setup solves

1. **EOS SDK used to hang under box64** — the real EOS SDK's async completion callbacks would wedge indefinitely under default box64 dynarec settings. Fixed by setting `BOX64_DYNAREC_STRONGMEM=2`, `BOX64_DYNAREC_BLEEDING_EDGE=0`, `BOX64_DYNAREC_BIGBLOCK=0` (set as `ENV` in `docker/Dockerfile`). With these, the real, unmodified `libEOSSDK-Linux-Shipping.so` initializes and registers a session correctly — no stub, no wrapper needed.

2. **Mono JIT crashes under Rosetta 2 / QEMU** — Running the server with `--platform=linux/amd64` places JIT-compiled Mono code and native libraries more than 2 GB apart, which overflows x86 `CALL rel32` offsets. The fix is to run in an ARM64 container with box64, which manages its own x86_64 address space.

3. **Wrong working directory under box64** — Unity Addressables resolve asset bundle paths relative to the process CWD. box64 defaults CWD to `/`, so the entrypoint `cd /opt/7days` before launching.

### Key files and their roles

- `docker/Dockerfile` — single-stage `linux/arm64` build: installs box64 from ptitSeb's PPA, sets the `BOX64_DYNAREC_*` tuning env vars, and copies `server/` in with the real EOS SDK untouched.
- `docker/entrypoint.sh` — renders `serverconfig.xml` via `envsubst`, `cd`s to the game directory, launches the server under box64, and syncs saves to S3 on SIGTERM.
- `config/serverconfig.xml` — server configuration; environment variables from `.env` are substituted at container startup.
- `eos-stub/eos_stub.c` + `eos-stub/eos.map` — **historical artifact**, no longer used by `docker/Dockerfile`. Kept for reference; see "Why the stub/wrapper approaches failed" below.
- `docker-compose.yml` — defines the `7days` service, `linux/arm64` platform, ports, volume mounts for saves/config/game files, and 5-minute stop grace period.

### Volume layout

- `./server` → copied into the image at `/opt/7days` during build.
- `./config/serverconfig.xml` → mounted read-only at `/config/serverconfig.xml`.
- `./saves` → bind-mounted at `/home/steam/.local/share/7DaysToDie/Saves` and is gitignored.

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
