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

# Rebuild from scratch (useful after changing the EOS stub)
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

### The three problems this setup solves

1. **EOS SDK hangs under ARM emulation** — Epic Online Services uses async callbacks with x86_64 threading semantics that deadlock when emulated. The fix is to replace `libEOSSDK-Linux-Shipping.so` with a stub (`eos-stub/eos_stub.c`) that exports all EOS symbols and returns `EOS_NoConnection` immediately.

2. **Mono JIT crashes under Rosetta 2 / QEMU** — Running the server with `--platform=linux/amd64` places JIT-compiled Mono code and native libraries more than 2 GB apart, which overflows x86 `CALL rel32` offsets. The fix is to run in an ARM64 container with box64, which manages its own x86_64 address space.

3. **Wrong working directory under box64** — Unity Addressables resolve asset bundle paths relative to the process CWD. box64 defaults CWD to `/`, so the entrypoint `cd /opt/7days` before launching.

### Key files and their roles

- `docker/Dockerfile` — two-stage build: compiles the x86_64 EOS stub in an amd64 stage, then installs box64 in an arm64 runtime stage and copies the stub over the real EOS SDK.
- `docker/entrypoint.sh` — renders `serverconfig.xml` via `envsubst`, `cd`s to the game directory, launches the server under box64, and syncs saves to S3 on SIGTERM.
- `config/serverconfig.xml` — server configuration; environment variables from `.env` are substituted at container startup.
- `eos-stub/eos_stub.c` + `eos-stub/eos.map` — EOS SDK stub and linker version script.
- `docker-compose.yml` — defines the `7days` service, `linux/arm64` platform, ports, volume mounts for saves/config/game files, and 5-minute stop grace period.

### Volume layout

- `./server` → copied into the image at `/opt/7days` during build.
- `./config/serverconfig.xml` → mounted read-only at `/config/serverconfig.xml`.
- `./saves` → bind-mounted at `/home/steam/.local/share/7DaysToDie/Saves` and is gitignored.

### Network protocols

`serverconfig.xml` controls which networking stacks are active. The valid values for `ServerDisabledNetworkProtocols` are `LiteNetLib` and `SteamNetworking`. Do not add `EOS` there — it is not a valid protocol choice.

## Current status caveat

As of the latest experiments, the **server process starts and logs `GameServer.LogOn successful`, but it does not become joinable/"ready"**. The EOS session-registration step still fails even with a native ARM64 EOS SDK loaded via a box64 wrapper. Do not treat "server is running" as "server is accepting players" without checking the logs for `Failed creating session modification` or attempting an actual client connection.

## Notes for changes

- The EOS stub is built as a 64-bit shared object from an amd64 compiler stage. Editing `eos-stub/eos_stub.c` requires a full image rebuild to take effect.
- Any change to `config/serverconfig.xml` takes effect on the next `docker compose up` because the file is mounted read-only live.
- `./saves` persists across rebuilds because it is a host bind mount.
- There is no automated test suite in this repo; validation is done by starting the container and reading `docker logs`.

## Next steps to investigate

The server starts but is not joinable. Three candidate paths to try next:

1. **Test a real Steam client connection by direct IP**
   The server reaches `[Steamworks.NET] GameServer.LogOn successful`, which means Steam networking is up. A Steam client connecting directly to the host IP on port 26900 might work even though EOS session registration fails. This requires running the actual 7 Days to Die game client and joining by IP/password.

2. **Hybrid wrapper + targeted session stubbing**
   Use the custom box64 native ARM64 EOS SDK wrapper (see `docker/Dockerfile.wrapper`) for most calls, but intercept `EOS_Sessions_CreateSessionModification` / `EOS_Sessions_UpdateSession` and force success with synthetic session data (session name, session ID). This may get the server past the "still initializing" state. Risk: player auth may still need valid Epic IDs.

3. **Investigate why EOS auth/login does not yield a user**
   The real ARM64 EOS SDK loads and reports `Initialize: Success`, but `QueryFile (PlatformOverrides)` returns `EOS_NoConnection` and session creation fails. Debug why `EOS_Connect_Login` / platform creation do not produce a logged-in `EOS_ProductUserId` in a headless dedicated-server context. This may involve inspecting callback struct layouts in the box64 wrapper, checking how 7D2D passes `EOS_Platform_Options` / `EOS_Connect_Credentials`, or confirming whether the dedicated server expects Epic product/deployment credentials.
