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

shutdown() {
    echo "[shutdown] Caught signal, stopping server..."
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

box64 "$GAME_DIR/7DaysToDieServer.x86_64" \
    -configfile=/tmp/serverconfig.xml \
    -logfile /dev/stdout \
    -batchmode \
    -nographics \
    -dedicated &

SERVER_PID=$!
echo "[server] PID $SERVER_PID"
wait "$SERVER_PID"
