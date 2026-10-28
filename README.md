# 7 Days to Die Dedicated Server on macOS (Apple Silicon)

Run a **joinable 7 Days to Die dedicated server on an M-series Mac**. The official Linux x86_64 server runs in an ARM64 Docker container under the [box64](https://github.com/ptitSeb/box64) emulator, with the real, unmodified **EOS SDK**: no stub, and crossplay still works.

- ✅ EOS session registers, so the server shows up and clients can join
- ✅ Steam auth, saves and crossplay all work
- ✅ Optional nightly S3 backups, plus a backup on shutdown
- ✅ Builds server-side C# mods against the game's DLLs

Tested with game V 3.4.0 (b6, build 26167630) on a Mac Mini M4; real clients have joined and played on it.

**Quick start:** `git clone https://github.com/mattydee99/7days-server-macos.git`, download the server files with SteamCMD, `cp .env.example .env`, then `docker compose up -d --build`. Details below.

## Requirements

- An Apple Silicon Mac (M1 or later)
- Docker Desktop or OrbStack. Give the Docker VM **at least 16 GB of memory**: the server's memory use grows to ~4.5 GB over a long session, and with less headroom the VM's kernel can OOM-kill it (Docker Desktop → Settings → Resources → Memory).
- [SteamCMD](https://developer.valvesoftware.com/wiki/SteamCMD) to download the server files (~16 GB). The game files are not part of this repo.
- Optional: an S3 bucket and AWS credentials for save backups

## How it works

The 7D2D server binary is Linux x86_64-only. Getting it to run on ARM64, joinable, with a working EOS session required solving four problems:

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
  +force_install_dir /path/to/7days-server-macos/server \
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
BACKUP_RETAIN=7                        # nightly backups to keep
```

Leave `S3_BACKUP_BUCKET` empty to skip backups entirely.

Then edit `config/serverconfig.xml` to taste — at least `ServerName`, `ServerDescription`, `GameWorld` and `GameName`. Since 3.x, gameplay options such as blood moon frequency are set through the `SandboxCode` property (see `CLAUDE.md`).

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
  +force_install_dir /path/to/7days-server-macos/server \
  +@sSteamCmdForcePlatformType linux \
  +login anonymous \
  +app_update 294420 \
  +quit

docker compose up -d --build --force-recreate
```

---

## Saves

Game saves are bind-mounted to `./saves/` on the host (gitignored). They persist across container restarts and rebuilds.

If `S3_BACKUP_BUCKET` is set:

- **Nightly backups** run at 03:00 UTC (`docker/backup.sh`) into dated `s3://$S3_BACKUP_BUCKET/backups/<timestamp>/` prefixes, keeping the newest `BACKUP_RETAIN`. Trigger one on demand with `docker exec 7days backup.sh` (add `--retain N` to override retention for that run).
- **On clean shutdown** (`docker compose down`, or `docker stop`), the entrypoint traps SIGTERM and syncs saves to `s3://$S3_BACKUP_BUCKET/saves/` before exiting. The `stop_grace_period: 5m` in `docker-compose.yml` gives it time to finish.

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

## Server console

```bash
docker exec 7days console.sh "<command>"   # e.g. "help", "lp", "gso"
```

---

## Optional extras

- **CastleBuilder mod** (`mods/CastleBuilder/`) — a small server-side mod that adds a `castle` console command for surveying terrain and placing a generated fortified base (`tools/castle_plan.py` generates the layouts). It's compiled against the game's DLLs during `docker compose build`. It shows how to build a server-side mod in this setup; if you don't want it, delete `mods/CastleBuilder/`.
- **Claude Code skills** (`.claude/skills/`) — procedures for updating the game and running the castle tooling, for use with [Claude Code](https://claude.com/claude-code). `CLAUDE.md` holds the detailed troubleshooting notes (EOS CA-root checks, OOM diagnosis, `SandboxCode` format).
- **`PLAN.md`** — the debugging log of how this setup was worked out, including the approaches that didn't work.

---

## Project structure

```
.
├── config/
│   └── serverconfig.xml    # Server config — env vars substituted at startup
├── docker/
│   ├── Dockerfile          # arm64 build: compiles mods, box64 + BOX64_DYNAREC tuning, real EOS SDK untouched
│   ├── entrypoint.sh       # cd to game dir, box64 launch, nightly backup schedule, SIGTERM → S3 sync
│   ├── backup.sh           # Dated S3 backup + retention pruning
│   └── console.sh          # Send a command to the server console
├── mods/CastleBuilder/     # Optional example server-side mod
├── tools/castle_plan.py    # Layout generator for the castle mod
├── eos-stub/               # Historical artifact from an earlier (non-joinable) approach; unused by docker/Dockerfile
├── saves/                  # Gitignored — game saves (host-mounted)
├── server/                 # Gitignored — pre-downloaded Linux server files (~16GB)
├── docker-compose.yml
├── .env                    # Gitignored — secrets
└── .env.example
```

---

## License

MIT — see [LICENSE](LICENSE). This covers the files in this repo only. 7 Days to Die and its dedicated server are © The Fun Pimps, and the EOS SDK is © Epic Games; neither is included or redistributed here. This project isn't affiliated with either.
