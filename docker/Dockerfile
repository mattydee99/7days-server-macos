# Compile our own mods (mods/<Name>/src/*.cs) against the game's managed DLLs.
# Mods without a src/ directory are copied as-is.
FROM --platform=linux/arm64 mcr.microsoft.com/dotnet/sdk:8.0 AS mods
COPY server/7DaysToDieServer_Data/Managed/ /managed/
COPY mods/ /src/
RUN CSC=$(ls /usr/share/dotnet/sdk/*/Roslyn/bincore/csc.dll | head -1) \
    && REFS=$(for f in /managed/*.dll; do printf -- '-r:%s ' "$f"; done) \
    && mkdir -p /out \
    && for dir in /src/*/; do \
        name=$(basename "$dir"); \
        mkdir -p "/out/$name"; \
        find "$dir" -mindepth 1 -maxdepth 1 ! -name src -exec cp -r {} "/out/$name/" \; ; \
        if [ -d "$dir/src" ]; then \
            dotnet "$CSC" -nologo -nostdlib -noconfig -target:library -langversion:9 -optimize \
                -nowarn:CS1701,CS1702 $REFS -out:"/out/$name/$name.dll" "$dir"/src/*.cs || exit 1; \
        fi; \
    done

FROM --platform=linux/arm64 ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install box64 from ptitSeb's PPA
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        gettext-base \
        awscli \
        gpg \
    && curl -fsSL https://pi-apps-coders.github.io/box64-debs/KEY.gpg \
        | gpg --dearmor -o /usr/share/keyrings/box64.gpg \
    && echo "deb [arch=arm64 signed-by=/usr/share/keyrings/box64.gpg] https://pi-apps-coders.github.io/box64-debs/debian ./" \
        > /etc/apt/sources.list.d/box64.list \
    && apt-get update && apt-get install -y --no-install-recommends \
        box64-generic-arm \
    && rm -rf /var/lib/apt/lists/*

# Runtime libs — box64 wraps x86_64 calls to these native ARM64 versions
RUN apt-get update && apt-get install -y --no-install-recommends \
        libatomic1 \
        libasound2 \
        libpulse0 \
        libx11-6 \
        libxcursor1 \
        libxrandr2 \
        libxi6 \
    && rm -rf /var/lib/apt/lists/*

# Tuning that avoids the historical box64 hang on the real EOS SDK's async
# callbacks. Without these, EOS_Platform_Tick can wedge indefinitely.
ENV BOX64_DYNAREC_STRONGMEM=2
ENV BOX64_DYNAREC_BLEEDING_EDGE=0
ENV BOX64_DYNAREC_BIGBLOCK=0

RUN useradd -m -s /bin/bash steam

# Real EOS SDK is left in place — it validates api.epicgames.dev's current
# certificate (issued by Google Trust Services) fine. Do not replace it with
# a stub: the stub can never register a real session, and the ARM64-native
# EOS SDK that ships alongside it has a stale embedded CA bundle that's
# missing the Google Trust Services roots entirely.
COPY --chown=steam:steam server/ /opt/7days/
COPY --from=mods --chown=steam:steam /out/ /opt/7days/Mods/

RUN mkdir -p /home/steam/.local/share/7DaysToDie/Saves \
    && chown -R steam:steam /home/steam/.local

COPY docker/entrypoint.sh /entrypoint.sh
COPY docker/backup.sh /usr/local/bin/backup.sh
COPY docker/console.sh /usr/local/bin/console.sh
RUN chmod +x /entrypoint.sh /usr/local/bin/backup.sh /usr/local/bin/console.sh

EXPOSE 26900/tcp 26900/udp 26901/udp 26902/udp

STOPSIGNAL SIGTERM

USER steam
ENTRYPOINT ["/entrypoint.sh"]
