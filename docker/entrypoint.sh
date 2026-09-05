#!/bin/bash
set -e

GAME_DIR="/opt/7days"
SAVES_DIR="/home/steam/.local/share/7DaysToDie/Saves"

backup() {
    if [ -n "$S3_BACKUP_BUCKET" ]; then
        echo "[backup] Syncing saves to s3://$S3_BACKUP_BUCKET/saves/ ..."
        aws s3 sync "$SAVES_DIR/" "s3://$S3_BACKUP_BUCKET/saves/" --delete
        echo "[backup] Done."
    else
        echo "[backup] S3_BACKUP_BUCKET not set, skipping."
    fi
}

nightly_backup_loop() {
    local hour=3 minute=0
    while true; do
        now_epoch="$(date -u +%s)"
        next_epoch="$(date -u -d "$(date -u +%Y-%m-%d) ${hour}:${minute}:00" +%s)"
        if [ "$next_epoch" -le "$now_epoch" ]; then
            next_epoch=$((next_epoch + 86400))
        fi
        sleep_seconds=$((next_epoch - now_epoch))
        echo "[backup] Next nightly backup in ${sleep_seconds}s (03:00 UTC)"
        sleep "$sleep_seconds"
        /usr/local/bin/backup.sh || echo "[backup] Nightly backup failed"
    done
}

shutdown() {
    echo "[shutdown] Caught signal, stopping server..."
    if [ -n "$NIGHTLY_PID" ] && kill -0 "$NIGHTLY_PID" 2>/dev/null; then
        kill -TERM "$NIGHTLY_PID" 2>/dev/null || true
    fi
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill -TERM "$SERVER_PID"
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    backup
    exit 0
}

trap shutdown SIGTERM SIGINT

echo "[server] Starting 7 Days to Die..."
envsubst < /config/serverconfig.xml > /tmp/serverconfig.xml

nightly_backup_loop &
NIGHTLY_PID=$!

cd "$GAME_DIR"
box64 ./7DaysToDieServer.x86_64 \
    -configfile=/tmp/serverconfig.xml \
    -batchmode \
    -nographics \
    -dedicated &

SERVER_PID=$!
echo "[server] PID $SERVER_PID"
wait "$SERVER_PID"
