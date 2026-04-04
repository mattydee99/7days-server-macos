FROM --platform=linux/arm64 ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        gnupg \
        gettext-base \
        awscli \
    && rm -rf /var/lib/apt/lists/*

# Install box64 — translates x86_64 binaries on ARM64 natively
RUN curl -fsSL https://ryanfortner.github.io/box64-debs/box64.list \
        -o /etc/apt/sources.list.d/box64.list \
    && curl -fsSL https://ryanfortner.github.io/box64-debs/KEY.gpg \
        | gpg --dearmor -o /usr/share/keyrings/box64-debs-archive-keyring.gpg \
    && apt-get update \
    && apt-get install -y box64-arm64 \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash steam

# Copy pre-downloaded Linux x86_64 server files
COPY --chown=steam:steam server/ /opt/7days/

RUN mkdir -p /home/steam/.local/share/7DaysToDie/Saves \
    && chown -R steam:steam /home/steam/.local

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 26900/tcp 26900/udp 26901/udp 26902/udp

STOPSIGNAL SIGTERM

USER steam
ENTRYPOINT ["/entrypoint.sh"]
