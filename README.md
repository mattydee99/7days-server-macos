# 7 Days to Die — Docker Server

Runs a 7 Days to Die dedicated server in Docker on Apple Silicon (Mac Mini M4) using Docker Desktop + Rosetta 2.

## Why this setup

The 7D2D dedicated server is Linux-only (x86_64). On Apple Silicon:

- **SteamCMD inside a container doesn't work** — SteamCMD's own launcher binary is 32-bit. Rosetta 2 only handles 64-bit. 32-bit x86 falls back to QEMU, which crashes with `futex robust_list not initialized`.
- **The game server binary itself is 64-bit** and runs fine under Rosetta 2 in Docker Desktop.

The solution: download the Linux server files once on the host using the macOS SteamCMD (which is 64-bit and works fine), then `COPY` them into the Docker image. No SteamCMD inside the container at all.

## First-time setup

### 1. Download server files

Install SteamCMD via Homebrew if you haven't already:

```bash
brew install steamcmd
```

Download the Linux x86_64 server files:

```bash
steamcmd \
  +force_install_dir /Users/matty/2git/7days-docker/server \
  +@sSteamCmdForcePlatformType linux \
  +login anonymous \
  +app_update 294420 \
  +quit
```

This downloads ~16GB into `server/`. It's gitignored.

### 2. Configure environment

```bash
cp .env.example .env
```

Edit `.env` and fill in:

```
SERVER_PASSWORD=yourpassword
TELNET_PASSWORD=yourtelnetpassword
S3_BACKUP_BUCKET=7days-backups-YOUR_ACCOUNT_ID
AWS_ACCESS_KEY_ID=...
AWS_SECRET_ACCESS_KEY=...
AWS_DEFAULT_REGION=us-west-2
```

### 3. Build and run

```bash
docker compose build   # copies ~16GB into the image — takes a few minutes
docker compose up
```

## Updating the server

Re-run the SteamCMD download command (it only fetches changed files), then rebuild:

```bash
steamcmd \
  +force_install_dir /Users/matty/2git/7days-docker/server \
  +@sSteamCmdForcePlatformType linux \
  +login anonymous \
  +app_update 294420 \
  +quit

docker compose build
docker compose up
```

## Saves

Game saves are mounted at `./saves/` on the host (gitignored). On container start, the server picks them up automatically.

To restore from S3:

```bash
aws s3 sync s3://$S3_BACKUP_BUCKET/saves/ ./saves/
```

On clean shutdown (`docker compose down`), the entrypoint traps SIGTERM, waits for the server to stop, then syncs saves to S3 automatically. `stop_grace_period: 5m` in the compose file gives it time to finish.

## Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 26900 | TCP + UDP | Game (primary) |
| 26901 | UDP | Game |
| 26902 | UDP | Game |

## Project structure

```
.
├── config/
│   └── serverconfig.xml    # Server config — ${SERVER_PASSWORD} and ${TELNET_PASSWORD} substituted at startup
├── docker/
│   ├── Dockerfile           # Copies pre-downloaded server files into the image
│   └── entrypoint.sh        # Starts server, traps shutdown for S3 backup
├── saves/                   # Gitignored — game saves (host-mounted volume)
├── server/                  # Gitignored — pre-downloaded Linux server files (~16GB)
├── docker-compose.yml
├── .env                     # Gitignored — secrets
└── .env.example
```
