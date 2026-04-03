# 7 Days to Die — Docker Setup Plan

Goal: run the 7D2D dedicated server in Docker on a Mac Mini M4 (ARM64), using Rosetta 2
x86_64 emulation via OrbStack. Replace the OrbStack VM approach (which used QEMU and
caused SteamCMD segfaults).

---

## Architecture

### Dockerfile (`docker/Dockerfile`)
- `FROM ubuntu:22.04` with `--platform linux/amd64` (forces Rosetta, not QEMU)
- Install SteamCMD deps (i386 libs)
- Install SteamCMD
- Install 7D2D server via SteamCMD during image build (bakes ~15GB game into image)
- Entrypoint runs the server binary directly

### docker-compose.yml
- `platform: linux/amd64` to ensure Rosetta translation
- Ports: UDP 26900-26902, TCP 26900
- Volume mounts:
  - `./saves` → `/home/steam/.local/share/7DaysToDie/Saves` (save persistence)
  - `./config/serverconfig.xml` → inside container (Ansible patches this on the host)
- AWS credentials via env vars or mounted `~/.aws/credentials`

### Ansible role: `docker_7days`
Replaces the `steamcmd` + `7days_server` roles. Tasks:
- Ensure Docker is installed on the host
- Build the image (or pull from local registry)
- Deploy `docker-compose.yml`
- Patch `serverconfig.xml` on the host volume (same `lineinfile` approach as before)
- Start/restart the container

### Backup role (mostly unchanged)
- S3 backup script runs on container stop via a SIGTERM handler in the entrypoint
- Entrypoint traps SIGTERM → runs `aws s3 sync saves/ s3://bucket/saves/` → then stops server
- AWS credentials provided via environment variable or mounted credentials file

---

## Key Decisions

| Decision | Choice | Reason |
|----------|--------|--------|
| Bake game into image vs volume mount | Bake into image | Simpler entrypoint, no install on first run |
| SteamCMD install method | Manual tarball (same as before) | Works fine once in Rosetta context |
| Config management | Host-side volume + Ansible lineinfile | Keeps existing reconfigure.yml workflow |
| Graceful shutdown / backup | SIGTERM trap in entrypoint script | Docker stop → SIGTERM → backup → exit |
| Credentials | Mounted `~/.aws/credentials` or env vars | No instance profile available in container |

---

## File Structure

```
Docker-7DaysToDie/          ← this repo
├── docker/
│   ├── Dockerfile
│   └── entrypoint.sh       ← starts server, traps SIGTERM for backup
├── docker-compose.yml
├── config/
│   └── serverconfig.xml    ← patched by Ansible, mounted into container
├── saves/                  ← gitignored, persisted save data
└── PLAN.md                 ← this file
```

---

## Open Questions

- Does SteamCMD actually work under OrbStack's Rosetta path for Docker?
  (Test early — run `docker run --platform linux/amd64 ubuntu:22.04 uname -m` first)
- What's the stop timeout needed for S3 sync? (was 5 min on EC2 systemd)
  → Set `docker stop --time 300` or `stop_grace_period: 5m` in compose
- Do we keep the Ansible-managed approach or go pure Docker compose?
