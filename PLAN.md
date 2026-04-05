# 7 Days to Die — Docker on Apple Silicon

**Goal:** Run the 7D2D dedicated server in Docker on a Mac Mini M4 (ARM64). The server binary is Linux x86_64-only.

---

## ✅ SOLVED — Working Configuration

**Platform:** Apple Silicon (ARM64) — Mac Mini M4, any M-series Mac  
**Docker runtime:** OrbStack or Docker Desktop (both work)  
**Approach:** ARM64 container + box64 emulator + EOS stub

### The two-part secret sauce

**Problem 1 — EOS SDK hangs:** The Epic Online Services SDK fires async completion callbacks using x86_64 threading semantics that never work correctly under ARM emulation. The server hangs indefinitely on startup.

**Fix 1 — EOS stub:** Replace `libEOSSDK-Linux-Shipping.so` with a stub that exports all 668 EOS SDK symbols and fires every async callback immediately with `EOS_NoConnection`. The server sees a failed-but-instant EOS init, skips Epic features, and starts normally.

**Problem 2 — Mono JIT crash under Rosetta 2 / QEMU:** Any `--platform=linux/amd64` approach (Rosetta 2 or QEMU) places JIT-compiled Mono code and native libs more than 2GB apart in the virtual address space. Mono's x86 JIT emits `CALL rel32` instructions that physically cannot encode offsets larger than ±2GB. `setarch -R` and all other userspace workarounds fail because the hypervisor controls memory layout.

**Fix 2 — box64 in ARM64 container:** Switch to `--platform=linux/arm64` and use box64 to run the x86_64 binary. box64 runs as a native ARM64 process and manages its own x86_64 address space, keeping allocations within ±2GB. The Mono JIT crash does not occur.

**Problem 3 — Wrong working directory under box64:** Unity's Addressables system resolves bundle paths (`Data/Addressables/Standalone/...`) relative to the process working directory. Under box64, the CWD defaults to `/` rather than the game directory, so bundles can't be found. The server gets stuck loading assets.

**Fix 3 — `cd` before launch:** `cd /opt/7days` before invoking box64 so relative paths resolve correctly.

### Trade-offs
- No crossplay (EOS stub returns `EOS_NoConnection`)
- No Epic account features
- Everything else works: game logic, saves, S3 backup, Steam authentication, player connections

---

## What Was Tried and Failed

| Approach | Result | Root Cause |
|----------|--------|------------|
| box64 (pre-1.0 era) | Worked | EOS was less integrated then |
| box64 (v1.0+), no EOS fix | Hangs | EOS async callbacks never fire |
| `--platform=linux/amd64` + Rosetta 2 (OrbStack) | Crashes | Mono JIT: `x86-codegen.h:410` offset overflow |
| `--platform=linux/amd64` + Rosetta 2 (Docker Desktop) | Crashes | Same — both use the same Linux VM underneath |
| `setarch x86_64 -R` + `seccomp:unconfined` | Fails | Rosetta 2 ignores `ADDR_NO_RANDOMIZE` |
| FEX-Emu | Dead end | Requires 4KB kernel pages + SVE2 256-bit; Apple Silicon has neither |
| **box64 + EOS stub + `cd` to game dir** | **✅ Works** | |

---

## Key Files

```
7days-docker/
├── docker/
│   ├── Dockerfile          ← two-stage build: amd64 EOS stub + arm64 runtime with box64
│   └── entrypoint.sh       ← cd to game dir, box64 launch, SIGTERM → S3 backup
├── eos-stub/
│   ├── eos_stub.c          ← 668 EOS SDK stub functions (all return EOS_NoConnection)
│   └── eos.map             ← linker version script (exports EOS_* under EOSSDK version)
├── docker-compose.yml      ← platform: linux/arm64
├── config/
│   └── serverconfig.xml    ← mounted into container, env vars substituted at runtime
└── saves/                  ← gitignored, persisted save data
```

### Dockerfile highlights
- **Build stage:** `FROM --platform=linux/amd64` — compiles EOS stub with gcc as x86_64 `.so`
- **Runtime stage:** `FROM --platform=linux/arm64` — installs box64 from pi-apps PPA, ARM64 runtime libs
- EOS stub copied over the real SDK at `7DaysToDieServer_Data/Plugins/libEOSSDK-Linux-Shipping.so`
- box64 loads the x86_64 stub natively (box64 handles x86_64 `.so` files)

### entrypoint.sh highlights
- `envsubst` renders `serverconfig.xml` with env vars before launch
- `cd "$GAME_DIR"` — critical: sets CWD so Addressables relative paths resolve correctly
- `box64 ./7DaysToDieServer.x86_64` — runs the x86_64 binary via box64
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
1. Install game files into the `game_files` Docker volume (via SteamCMD or copy)
2. Copy `config/serverconfig.xml` and set env vars in `.env`
3. `docker compose up -d --build`

### Updates
```bash
# Update game files in volume, then:
docker compose up -d --build --force-recreate
```

### Monitoring
```bash
docker logs -f 7days
```

---

## Future: box64 Native EOS Wrapper (optional, restores crossplay)

The game ships both an x86_64 and a native ARM64 EOS SDK with identical 668 exported symbols:
- `7DaysToDieServer_Data/Plugins/libEOSSDK-Linux-Shipping.so` (x86_64)
- `7DaysToDieServer_Data/Plugins/libEOSSDK-LinuxArm64-Shipping.so` (native ARM64)

A box64 native library wrapper (like the existing SDL2/OpenAL wrappers in [ptitSeb/box64](https://github.com/ptitSeb/box64)) could intercept loads of the x86_64 EOS SDK and redirect to the ARM64 version. Native ARM64 execution means callbacks fire correctly, restoring crossplay without needing the stub.

This contribution path:
1. Study how box64 native wrappers work (`libSDL2`, `libopenal` in the box64 source)
2. Generate the wrapper from the 668 exported EOS symbols
3. Add calling convention bridging for async callback signatures
4. Submit PR to ptitSeb/box64
