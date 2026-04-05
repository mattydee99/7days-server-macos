# Build the EOS stub (x86_64)
FROM --platform=linux/amd64 ubuntu:22.04 AS eos-builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc \
        libc6-dev \
    && rm -rf /var/lib/apt/lists/*

COPY eos-stub/eos_stub.c /build/eos_stub.c
COPY eos-stub/eos.map    /build/eos.map

RUN gcc -m64 -shared -fPIC -nostartfiles \
        -Wl,--version-script=/build/eos.map \
        -o /build/libEOSSDK-Linux-Shipping.so \
        /build/eos_stub.c

# ----

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

RUN useradd -m -s /bin/bash steam

COPY --chown=steam:steam server/ /opt/7days/

# Replace the real EOS SDK with the stub so the server doesn't hang on startup
COPY --from=eos-builder /build/libEOSSDK-Linux-Shipping.so \
        /opt/7days/7DaysToDieServer_Data/Plugins/libEOSSDK-Linux-Shipping.so

RUN mkdir -p /home/steam/.local/share/7DaysToDie/Saves \
    && chown -R steam:steam /home/steam/.local

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 26900/tcp 26900/udp 26901/udp 26902/udp

STOPSIGNAL SIGTERM

USER steam
ENTRYPOINT ["/entrypoint.sh"]
