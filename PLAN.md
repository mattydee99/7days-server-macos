# 7 Days to Die — Docker on Apple Silicon

**Goal:** Run the 7D2D dedicated server in Docker on a Mac Mini M4 (ARM64). The server binary is Linux x86_64-only.

---

## ✅ SOLVED — Working Configuration (confirmed 2026-09-05, game build 24994542 / "v3.2")

**Platform:** Apple Silicon (ARM64) — Mac Mini M4, any M-series Mac
**Docker runtime:** OrbStack or Docker Desktop (both work)
**Approach:** ARM64 container + box64 (tuned) + real x86_64 EOS SDK (no stub, no wrapper)

A game client successfully joined by direct IP over LAN, with EOS session
registration succeeding (`[EOS] Server registered, session: ...`) and no
`EOS_NoConnection` errors.

### The secret sauce

**Problem 1 — EOS SDK hangs under box64:** The real EOS SDK's async completion
callbacks used to wedge indefinitely under box64 emulation.

**Fix 1 — box64 dynarec tuning:** Set `BOX64_DYNAREC_STRONGMEM=2`,
`BOX64_DYNAREC_BLEEDING_EDGE=0`, `BOX64_DYNAREC_BIGBLOCK=0` (env vars in
`docker/Dockerfile`). With these, the real x86_64 `libEOSSDK-Linux-Shipping.so`
initializes and ticks correctly instead of hanging. No stub needed.

**Problem 2 — EOS session registration used to fail with `EOS_NoConnection`:**
This was previously misdiagnosed as a fundamental headless-dedicated-server
limitation. The actual cause: `api.epicgames.dev` now serves a certificate
issued by **Google Trust Services (CN=WE1)**. The real x86_64 EOS SDK has
`GTS Root R1-R4` in its statically-embedded CA bundle and validates this fine.
The **ARM64-native** EOS SDK (`libEOSSDK-LinuxArm64-Shipping.so`) does *not*
have Google Trust Services roots embedded — only legacy CAs (DigiCert,
Starfield, GlobalSign, Comodo, Entrust) — so it can never validate that
certificate. This is why the earlier "native ARM64 EOS SDK via a custom box64
wrapper" experiment (see `docker/Dockerfile.wrapper`, now removed) was a dead
end regardless of how correctly the wrapper itself was built.

**Fix 2 — use the real x86_64 EOS SDK, not the ARM64-native one, not a stub.**
box64 emulates the x86_64 SDK directly; no wrapper/redirection needed.

**Problem 3 — Mono JIT crash under Rosetta 2 / QEMU:** Any
`--platform=linux/amd64` approach places JIT-compiled Mono code and native
libs more than 2GB apart in the virtual address space, overflowing x86
`CALL rel32` offsets.

**Fix 3 — box64 in an ARM64 container:** box64 runs as a native ARM64 process
and manages its own x86_64 address space, keeping allocations within ±2GB.

**Problem 4 — Wrong working directory under box64:** Unity's Addressables
system resolves bundle paths relative to the process working directory, which
box64 defaults to `/`.

**Fix 4 — `cd /opt/7days` before launch** (in `docker/entrypoint.sh`).

### Trade-offs
- None currently known. Crossplay/EOS session registration, Steam auth, saves,
  and S3 backup all work with this configuration.
- Minor harmless log noise: `libMouseLib.so` fails to resolve some X11 symbols
  (mouse input isn't needed headless), and `libSDL3.so.0` is reported missing
  by box64. Neither affected server startup, session registration, or a real
  client connecting.

---

## What Was Tried and Failed (historical)

| Approach | Result | Root Cause |
|----------|--------|------------|
| box64 (pre-1.0 era) | Worked | EOS was less integrated then |
| box64 (v1.0+), no EOS fix | Hangs | EOS async callbacks never fire |
| `--platform=linux/amd64` + Rosetta 2 (OrbStack/Docker Desktop) | Crashes | Mono JIT: `x86-codegen.h:410` offset overflow |
| `setarch x86_64 -R` + `seccomp:unconfined` | Fails | Rosetta 2 ignores `ADDR_NO_RANDOMIZE` |
| FEX-Emu | Dead end | Requires 4KB kernel pages + SVE2 256-bit; Apple Silicon has neither |
| box64 + EOS stub (all calls return `EOS_NoConnection`) | Starts, but never joinable | Stub fakes init but can't create a real session; client stuck on "Server is still initializing" |
| box64 + custom wrapper redirecting to ARM64-native EOS SDK | Starts, session still fails | ARM64-native EOS SDK's embedded CA bundle lacks Google Trust Services roots needed to validate `api.epicgames.dev`'s current cert |
| **box64 (tuned dynarec) + real x86_64 EOS SDK, unmodified** | **✅ Works — session registers, client can join** | |

---

## Key Files

```
7days-docker/
├── docker/
│   ├── Dockerfile          ← single-stage arm64 build: box64 + tuned env vars, real EOS SDK left untouched
│   └── entrypoint.sh       ← cd to game dir, box64 launch, SIGTERM → S3 backup
├── eos-stub/                ← historical artifact from the stub approach; no longer used by docker/Dockerfile
│   ├── eos_stub.c
│   └── eos.map
├── docker-compose.yml      ← platform: linux/arm64
├── config/
│   └── serverconfig.xml    ← mounted into container, env vars substituted at runtime
└── saves/                  ← gitignored, persisted save data
```

### Dockerfile highlights
- Single `FROM --platform=linux/arm64 ubuntu:22.04` stage — no amd64 build stage needed anymore.
- Installs box64 from the pi-apps PPA, plus ARM64 runtime libs.
- Sets `BOX64_DYNAREC_STRONGMEM=2`, `BOX64_DYNAREC_BLEEDING_EDGE=0`, `BOX64_DYNAREC_BIGBLOCK=0`.
- Copies `server/` in as-is — the real `libEOSSDK-Linux-Shipping.so` is **not** replaced.

### entrypoint.sh highlights
- `envsubst` renders `serverconfig.xml` with env vars before launch
- `cd "$GAME_DIR"` — critical: sets CWD so Addressables relative paths resolve correctly
- `box64 ./7DaysToDieServer.x86_64` — runs the real x86_64 binary via box64
- SIGTERM handler triggers S3 backup of saves before exit

---

## Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 26900 | TCP + UDP | Game traffic |
| 26901 | UDP | Game traffic |
| 26902 | UDP | Game traffic |

---

## Deployment

### First time
1. Install game files into `server/` via SteamCMD (see `CLAUDE.md`).
2. Copy `config/serverconfig.xml` and set env vars in `.env`.
3. `docker compose up -d --build`

### Updates
```bash
# Update game files in server/ via SteamCMD, then:
docker compose up -d --build --force-recreate
```

### Monitoring
```bash
docker logs -f 7days
```

---

## Note on major game version upgrades

Jumping a major game version (e.g. v2.x saves under a v3.x server) can start a
fresh world/save rather than loading the old one — this is a game-side save
format compatibility issue, not a Docker/EOS issue.
