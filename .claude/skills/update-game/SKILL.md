---
name: update-game
description: Update the 7 Days to Die dedicated server to a new game build with SteamCMD, rebuild the container, and verify EOS registration and the CastleBuilder mod still work.
---

# Updating the game

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

Watch for `[EOS] Server registered, session: ...` (success) vs `EOS_NoConnection` / `Failed creating session` (failure). If EOS registration breaks after an update, don't assume a session-architecture regression — re-check CA roots first (Epic/Google rotate certs occasionally; see "Why the stub/wrapper approaches failed" in CLAUDE.md) and never fall back to the `LinuxArm64` EOS SDK or the old stub. Also be aware that a major version jump can silently start a fresh world instead of loading an old save (see "Notes for changes" in CLAUDE.md), which is why step 0 backs up first. Finish by attempting a real client connection, not just the log line. A game update can also break the `CastleBuilder` mod's compile (it builds against the game's DLLs); the image build fails loudly if so.
