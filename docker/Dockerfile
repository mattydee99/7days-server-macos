FROM --platform=linux/amd64 ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# SteamCMD dependencies
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        lib32gcc-s1 \
        lib32stdc++6 \
        libsdl2-2.0-0:i386 \
        awscli \
    && rm -rf /var/lib/apt/lists/*

# Dedicated user for the server
RUN useradd -m -s /bin/bash steam

# Install SteamCMD
RUN mkdir -p /opt/steamcmd && \
    curl -fsSL https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz | \
    tar -xz -C /opt/steamcmd && \
    chown -R steam:steam /opt/steamcmd

# Game and save directories (game files come in via volume at runtime)
RUN mkdir -p /opt/7days && chown steam:steam /opt/7days
RUN mkdir -p /home/steam/.local/share/7DaysToDie/Saves && \
    chown -R steam:steam /home/steam/.local

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 26900/tcp 26900/udp 26901/udp 26902/udp

STOPSIGNAL SIGTERM

USER steam
ENTRYPOINT ["/entrypoint.sh"]
