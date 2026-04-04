FROM --platform=linux/amd64 ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        gettext-base \
        libatomic1 \
        libasound2 \
        libpulse0 \
        libx11-6 \
        libxcursor1 \
        libxrandr2 \
        libxi6 \
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
