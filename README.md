# 7 Days to Die — Docker Server on Apple Silicon

Runs a 7 Days to Die dedicated server in Docker on Apple Silicon (M-series Mac) using **box64** with the real, unmodified Linux **EOS SDK** — no stub, no crossplay trade-off.

## How it works

The 7D2D server binary is Linux x86_64-only. Getting it to run on ARM64, joinable, with a working EOS session required solving three problems:

1. **EOS SDK used to hang under box64** — the real EOS SDK's async completion callbacks would wedge indefinitely under box64's default dynarec settings. **Fix:** set `BOX64_DYNAREC_STRONGMEM=2`, `BOX64_DYNAREC_BLEEDING_EDGE=0`, `BOX64_DYNAREC_BIGBLOCK=0` (baked into `docker/Dockerfile`). With these, the real `libEOSSDK-Linux-Shipping.so` initializes and ticks correctly.

2. **EOS session registration used to fail with `EOS_NoConnection`** — this looked like a fundamental headless-server limitation but wasn't. `api.epicgames.dev` now serves a certificate issued by Google Trust Services; the real x86_64 EOS SDK has those roots in its embedded CA bundle and validates it fine. (The ARM64-native EOS SDK that ships alongside it does *not* have those roots — only legacy CAs — so an approach that redirects to that library instead can never work regardless of how it's wired up.) **Fix:** use the real x86_64 EOS SDK as shipped; don't replace it or redirect to the ARM64-native one.

3. **Mono JIT crash under Rosetta 2 / QEMU** — any `--platform=linux/amd64` approach crashes with a Mono JIT assertion (`x86-codegen.h:410`) because the emulator places code segments more than 2GB apart, breaking x86 relative call instructions. **Fix:** use `--platform=linux/arm64` with [box64](https://github.com/ptitSeb/box64), which manages its own x86_64 address space and avoids the gap.

4. **Wrong working directory under box64** — Unity's Addressables system resolves asset bundle paths relative to the process CWD, which defaults to `/` under box64. **Fix:** `cd /opt/7days` before launching.

### Trade-offs
None currently known — crossplay/EOS session registration, Steam auth, saves, and S3 backup all work. Some harmless log noise from box64 about missing X11/SDL3 symbols used by mouse input, which a headless server doesn't need.

### Prior art

Public attempts at this fall into two camps, neither of which gets a joinable, EOS-registered *Linux* dedicated server running under emulation on Apple Silicon:

- **box64 attempts crash before EOS is even reached.** A [box64 GitHub issue](https://github.com/ptitSeb/box64/issues/102) documents someone trying the 7D2D dedicated server on ARM64 (Raspberry Pi 4) and hitting an unrecoverable `Unimplemented Opcode` segfault during Mono/.NET startup — no EOS registration, no fix posted, issue still open.
- **The common Mac workaround avoids the problem instead of solving it.** The [Steam Community Mac dedicated-server guide](https://steamcommunity.com/sharedfiles/filedetails/?id=3489942057) runs the *native ARM64 macOS game client* in `-dedicated -batchmode` mode (no emulation needed, since it's native) but explicitly **disables EOS/crossplay** in `platform.cfg` to dodge `EOS_NoConnection` errors, restricting the server to Steam-only connections.

This setup is the only one found (as of 2026-09) that keeps the *real* Linux x86_64 binary, keeps the *real* unmodified EOS SDK, keeps crossplay enabled, and gets a session to register under box64 — via the dynarec tuning and CA-root diagnosis above. This is based on search-engine visibility, not an exhaustive audit; there may be an unindexed forum/Discord thread that got further.

---

## First-time setup

### 1. Download server files

Install SteamCMD via Homebrew if you haven't already:

```bash
brew install steamcmd
```

Download the Linux x86_64 server files to `server/` (~16GB, gitignored):

```bash
steamcmd \
  +force_install_dir /path/to/7days-docker/server \
  +@sSteamCmdForcePlatformType linux \
  +login anonymous \
  +app_update 294420 \
  +quit
```

> SteamCMD runs natively on macOS (64-bit). No need to run it inside Docker.

### 2. Configure environment

```bash
cp .env.example .env
```

Edit `.env`:

```
SERVER_PASSWORD=yourpassword
TELNET_PASSWORD=yourtelnetpassword
S3_BACKUP_BUCKET=your-bucket-name      # optional, for save backups
AWS_ACCESS_KEY_ID=...
AWS_SECRET_ACCESS_KEY=...
AWS_DEFAULT_REGION=us-west-2
```

### 3. Build and run

```bash
docker compose up -d --build
```

box64 install takes a few minutes the first time; the server files (~16GB) get copied into the image layer on every build.

### 4. Monitor

```bash
docker logs -f 7days
```

A successful start looks like:

```
INF StartGame done
INF [EOS] Server registered, session: <session-id>, 56 attributes
INF [Steamworks.NET] GameServer.LogOn successful, SteamID=..., public IP=...
```

If instead you see `EOS_NoConnection` or `Failed creating session`, something is wrong with EOS session registration — see `CLAUDE.md` for the diagnostic approach (checking which EOS SDK variant is in use and its embedded CA roots).

---

## Transferring an existing save from another machine

7D2D client saves live at, depending on OS:
- **Windows:** `%USERPROFILE%\AppData\Roaming\7DaysToDie\Saves\<WorldName>\<GameName>\`
- **macOS (native client):** `~/Library/Application Support/7DaysToDie/Saves/<WorldName>/<GameName>/`
- **macOS via Whisky/CrossOver:** inside that compat layer's virtual `AppData\Roaming\7DaysToDie\Saves\...`
- **Linux:** `~/.local/share/7DaysToDie/Saves/<WorldName>/<GameName>/`

Copy that `<WorldName>/<GameName>/` folder into `./saves/<WorldName>/<GameName>/` in this project, then set `GameWorld` and `GameName` in `config/serverconfig.xml` to match those folder names exactly, and restart (`docker compose up -d` — a plain `docker restart` won't pick up config changes to env-substituted values, but `serverconfig.xml` itself is read fresh on every entrypoint run either way).

If the save uses a custom RWG-generated world (not one of the built-in maps like Navezgane or the Pregen maps), you'll also need the corresponding `GeneratedWorlds/<WorldName>/` folder, which doesn't currently have a volume mount in `docker-compose.yml`.

Note: loading an old save from a much earlier game version under a newer server build can silently start a fresh world instead — that's a game-side save-format compatibility issue, not specific to this Docker setup.

---

## Updating the server

Re-run SteamCMD (only fetches changed files), then rebuild:

```bash
steamcmd \
  +force_install_dir /path/to/7days-docker/server \
  +@sSteamCmdForcePlatformType linux \
  +login anonymous \
  +app_update 294420 \
  +quit

docker compose up -d --build --force-recreate
```

---

## Saves

Game saves are bind-mounted to `./saves/` on the host (gitignored). They persist across container restarts and rebuilds.

On clean shutdown (`docker compose down`, or `docker stop`), the entrypoint traps SIGTERM and syncs saves to S3 before exiting. The `stop_grace_period: 5m` in `docker-compose.yml` gives it time to finish.

To restore from S3:

```bash
aws s3 sync s3://$S3_BACKUP_BUCKET/saves/ ./saves/
```

---

## Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 26900 | TCP + UDP | Game (primary) |
| 26901 | UDP | Game |
| 26902 | UDP | Game |

---

## Project structure

```
.
├── config/
│   └── serverconfig.xml    # Server config — env vars substituted at startup
├── docker/
│   ├── Dockerfile          # Single-stage arm64 build: box64 + BOX64_DYNAREC tuning, real EOS SDK untouched
│   └── entrypoint.sh       # cd to game dir, box64 launch, SIGTERM → S3 backup
├── eos-stub/                # Historical artifact from an earlier (non-joinable) approach; unused by docker/Dockerfile
│   ├── eos_stub.c
│   └── eos.map
├── saves/                  # Gitignored — game saves (host-mounted)
├── server/                 # Gitignored — pre-downloaded Linux server files (~16GB)
├── docker-compose.yml
├── .env                    # Gitignored — secrets
└── .env.example
```
