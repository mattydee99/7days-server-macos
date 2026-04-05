# 7 Days to Die — Docker Server on Apple Silicon

Runs a 7 Days to Die dedicated server in Docker on Apple Silicon (M-series Mac) using **box64** + an **EOS SDK stub**.

## How it works

The 7D2D server binary is Linux x86_64-only. Getting it to run on ARM64 required solving three problems:

1. **EOS SDK hangs** — Epic Online Services fires async callbacks using x86_64 threading semantics that never work under ARM emulation. The server would hang indefinitely on startup. **Fix:** replace `libEOSSDK-Linux-Shipping.so` with a stub that exports all 668 EOS symbols and returns `EOS_NoConnection` immediately. The server skips Epic features and starts normally.

2. **Mono JIT crash under Rosetta 2 / QEMU** — any `--platform=linux/amd64` approach crashes with a Mono JIT assertion (`x86-codegen.h:410`) because the emulator places code segments more than 2GB apart, breaking x86 relative call instructions. **Fix:** use `--platform=linux/arm64` with [box64](https://github.com/ptitSeb/box64), which manages its own x86_64 address space and avoids the gap.

3. **Wrong working directory under box64** — Unity's Addressables system resolves asset bundle paths relative to the process CWD, which defaults to `/` under box64. **Fix:** `cd /opt/7days` before launching.

### Trade-offs
- No crossplay or Epic account features (EOS stub returns `EOS_NoConnection`)
- Everything else works: game logic, saves, S3 backup, Steam authentication, player connections

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

The build compiles the EOS stub and installs box64 — takes a few minutes the first time.

### 4. Monitor

```bash
docker logs -f 7days
```

A successful start looks like:

```
[EOS] Initialize: Success
Starting dedicated server level=Navezgane ...
Dedicated server only build
```

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

On clean shutdown (`docker compose down`), the entrypoint traps SIGTERM and syncs saves to S3 before exiting. The `stop_grace_period: 5m` in `docker-compose.yml` gives it time to finish.

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
│   ├── Dockerfile          # Two-stage: builds EOS stub (amd64) + arm64 runtime with box64
│   └── entrypoint.sh       # cd to game dir, box64 launch, SIGTERM → S3 backup
├── eos-stub/
│   ├── eos_stub.c          # 668 EOS SDK stub functions (all return EOS_NoConnection)
│   └── eos.map             # Linker version script
├── saves/                  # Gitignored — game saves (host-mounted)
├── server/                 # Gitignored — pre-downloaded Linux server files (~16GB)
├── docker-compose.yml
├── .env                    # Gitignored — secrets
└── .env.example
```
