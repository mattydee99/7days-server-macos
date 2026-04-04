FROM --platform=linux/arm64 ubuntu:22.04 AS box64-builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        python3 \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth=1 https://github.com/ptitSeb/box64 /box64 \
    && cmake -B /box64/build -S /box64 \
        -DARM_DYNAREC=ON \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    && cmake --build /box64/build -j$(nproc)

# ----

FROM --platform=linux/arm64 ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

COPY --from=box64-builder /box64/build/box64 /usr/local/bin/box64

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

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 26900/tcp 26900/udp 26901/udp 26902/udp

STOPSIGNAL SIGTERM

USER steam
ENTRYPOINT ["/entrypoint.sh"]
