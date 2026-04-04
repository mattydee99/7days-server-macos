FROM --platform=linux/arm64 ubuntu:22.04 AS fex-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        software-properties-common \
    && add-apt-repository ppa:fex-emu/fex \
    && apt-get update && apt-get install -y --no-install-recommends \
        fex-emu \
    && rm -rf /var/lib/apt/lists/*

# ----

# x86_64 rootfs — FEX needs a full guest sysroot to find libs at runtime
FROM --platform=linux/amd64 ubuntu:22.04 AS x86_64-rootfs

ENV DEBIAN_FRONTEND=noninteractive

# Install the x86_64 runtime libs that the 7D2D server needs
RUN apt-get update && apt-get install -y --no-install-recommends \
        libatomic1 \
        libasound2 \
        libpulse0 \
        libx11-6 \
        libxcursor1 \
        libxrandr2 \
        libxi6 \
        libdl-dev \
    && rm -rf /var/lib/apt/lists/*

# ----

FROM --platform=linux/arm64 ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# FEX-Emu binaries from builder stage
COPY --from=fex-builder /usr/bin/FEXBash /usr/bin/FEXBash
COPY --from=fex-builder /usr/bin/FEXInterpreter /usr/bin/FEXInterpreter
COPY --from=fex-builder /usr/lib/fex-emu/ /usr/lib/fex-emu/
COPY --from=fex-builder /usr/share/fex-emu/ /usr/share/fex-emu/

# x86_64 rootfs for FEX — copy the full amd64 userspace
COPY --from=x86_64-rootfs / /opt/fex-rootfs/

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        gettext-base \
        awscli \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash steam

COPY --chown=steam:steam server/ /opt/7days/

RUN mkdir -p /home/steam/.local/share/7DaysToDie/Saves \
    && chown -R steam:steam /home/steam/.local

# Point FEX at our x86_64 rootfs
ENV FEX_ROOTFS=/opt/fex-rootfs

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 26900/tcp 26900/udp 26901/udp 26902/udp

STOPSIGNAL SIGTERM

USER steam
ENTRYPOINT ["/entrypoint.sh"]
